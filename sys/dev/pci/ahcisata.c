
/*
 * Copyright (C) 2008 Freescale Semiconductor, Inc.
 *		Dave Liu <daveliu@freescale.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#define DEBUG_RX
#include "bpfilter.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/syslog.h>

#include <ctype.h>

#if defined(__NetBSD__) || defined(__OpenBSD__)

#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>


#if defined(__OpenBSD__)
#endif

#include <vm/vm.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>


#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/if_fxpreg.h>

#else /* __FreeBSD__ */

#include <sys/sockio.h>


#include <vm/vm.h>              /* for vtophys */
#include <vm/vm_param.h>        /* for vtophys */
#include <vm/pmap.h>            /* for vtophys */
#include <machine/clock.h>      /* for DELAY */

#include <pci/pcivar.h>

#endif /* __NetBSD__ || __OpenBSD__ */

#if NGZIP > 0
#include <gzipfs.h>
#endif /* NGZIP */

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/ata/atavar.h>
#include <dev/ata/atareg.h>
#include <dev/ic/wdcreg.h>
#include <dev/ic/wdcvar.h>

#include <linux/libata.h>
#include <fis.h>
#include <part.h>
#include <pmon.h>

#include "ahcisata.h"
#include "ahci.h"
#include "ata.h"

#ifndef PHYSADDR
#define PHYSADDR(x) (((long)(x))&0x1fffffff)
#endif

#define cpu_to_le32(x) (x)
#define CFG_SATA_MAX_DEVICE 2
#define ahci_sata_lookup(dev)	\
	(struct ahci_sata_softc *)device_lookup(&ahcisata_cd, minor(dev))
block_dev_desc_t sata_dev_desc[CFG_SATA_MAX_DEVICE];


static unsigned char dev_count;
typedef struct port_to_dev{
	unsigned char dev_index;
	unsigned char port_index;
}port_to_dev_t;
static port_to_dev_t port_to_dev_info[CFG_SATA_MAX_DEVICE];

static int curr_device = -1;
static int lba48[32];
static int fault_timeout;

static int ahcisata_match(struct device *, void *, void *);
static void ahcisata_attach(struct device *, struct device *, void *);
static int ahci_sata_initialize(u32 reg,u32 flags);
static void ahci_set_feature(u8 port);
static int ahci_port_start(u8 port);
static int ata_scsiop_inquiry(int port);
static void ahci_fill_cmd_slot(struct ahci_ioports *pp, u32 opts);

extern unsigned long strtoul(char *nptr, char **endptr, int base);
extern int strcmp(char *s1, char *s2);

ulong ahci_sata_read(int dev, unsigned long blknr, lbaint_t blkcnt, void *buffer);
ulong ahci_sata_write(int dev, unsigned long blknr, lbaint_t blkcnt, void *buffer);

typedef struct ahci_sata_softc {
	/* General disk infos */
	struct device sc_dev;
	int dev;
	int bs, count;
	int lba48;
}ahci_sata_softc;

struct cfattach ahcisata_ca = {
	sizeof(ahci_sata_softc), ahcisata_match, ahcisata_attach,
};

struct cfdriver ahcisata_cd = {
	NULL, "ahcisata", DV_DISK
};

static __inline__ int __ilog2(unsigned int x)
{
	int lz;

	asm volatile (
			".set\tnoreorder\n\t"
			".set\tnoat\n\t"
			".set\tmips32\n\t"
			"clz\t%0,%1\n\t"
			".set\tmips0\n\t"
			".set\tat\n\t"
			".set\treorder"
			: "=r" (lz)
			: "r" (x));

	return 31 - lz;
}

static void ahci_start_engine(int port)
{
	struct ahci_ioports *pp = &(probe_ent->port[port]);
	volatile u8 *port_mmio = (volatile u8 *)pp->port_mmio;
	u32 tmp;
		
	/* start DMA */
	tmp = readl(port_mmio + PORT_CMD);
	tmp |= PORT_CMD_START;
	writel(tmp, port_mmio + PORT_CMD);
	readl(port_mmio + PORT_CMD);	/* flush */
}

/**
 *	ata_wait_register - wait until register value changes
 *	@reg: IO-mapped register
 *	@mask: Mask to apply to read register value
 *	@val: Wait condition
 *	@interval: polling interval in milliseconds
 *	@timeout: timeout in milliseconds
 *
 *	Waiting for some bits of register to change is a common
 *	operation for ATA controllers.  This function reads 32bit LE
 *	IO-mapped register @reg and tests for the following condition.
 *
 *	(*@reg & mask) != val
 *
 *	If the condition is met, it returns; otherwise, the process is
 *	repeated after @interval_msec until timeout.
 *
 *	RETURNS:
 *	The final register value.
 */
u32 ata_wait_register(void *reg, u32 mask, u32 val,
		      unsigned long interval, unsigned long timeout_msec)
{
	u32 tmp, i;

	tmp = readl(reg);

	for (i = 0; ((val = readl(reg)) & mask) && i < timeout_msec; i+=interval)
		msleep(interval);

	tmp = readl(reg);
	return tmp;
}

static int ahci_stop_engine(int port)
{
	struct ahci_ioports *pp = &(probe_ent->port[port]);
	volatile u8 *port_mmio = (volatile u8 *)pp->port_mmio;
	u32 tmp;

	tmp = readl(port_mmio + PORT_CMD);

	/* check if the HBA is idle */
	if((tmp & (PORT_CMD_START | PORT_CMD_LIST_ON)) == 0)
		return 0;

	writel_with_flush(tmp, port_mmio + PORT_CMD);

	tmp = ata_wait_register((void *)(port_mmio + PORT_CMD),
			PORT_CMD_LIST_ON, PORT_CMD_LIST_ON, 1, 500);

	if(tmp & PORT_CMD_LIST_ON)
		return -EIO;

	return 0;
}

static int ahci_kick_engine(int port, int force_restart)
{
	struct ahci_ioports *pp = &(probe_ent->port[port]);
	volatile u8 *port_mmio = (volatile u8 *)pp->port_mmio;
	u8 status = readl(port_mmio + PORT_TFDATA) & 0xFF;
	u32 tmp;
	int busy, rc;

	/* do we need to kick the port? */
	busy = status & (ATA_BUSY | ATA_DRQ);
	if (!busy && !force_restart)
		return 0;

	/* stop engine */
	rc = ahci_stop_engine(port);
	if (rc)
		goto out_restart;
	/* need to do CLO? */
	if (!busy) {
		rc = 0;
		goto out_restart;
	}

#if 0
	if (!(hpriv->cap & HOST_CAP_CLO)) {
		rc = -EOPNOTSUPP;
		goto out_restart;
	}
#endif
	/* perform CLO */
	tmp = readl(port_mmio + PORT_CMD);
	tmp |= PORT_CMD_CLO;
	writel_with_flush(tmp, port_mmio + PORT_CMD);

	rc = 0;
	tmp = ata_wait_register((void *)(port_mmio + PORT_CMD),
				PORT_CMD_CLO, PORT_CMD_CLO, 1, 500);
	if (tmp & PORT_CMD_CLO)
		rc = -EIO;

	/* restart engine */
 out_restart:
	ahci_start_engine(port);
	return rc;
}

static void ata_reset_fis_init(u8 pmp, int is_cmd, u8 *fis, int set)
{
	fis[0] = 0x27;
	fis[1] = pmp & 0xf;
	if(is_cmd)
		fis[1] |= (1 << 7);

	fis[2] = 0;
	fis[3] = 0;
	fis[4] = 0;
	fis[5] = 0;
	fis[6] = 0;
	fis[7] = 0;
	fis[8] = 0;
	fis[9] = 0;
	fis[10] = 0;
	fis[11] = 0;
	fis[12] = 0;
	fis[13] = 0;
	fis[14] = 0;
	if(set == 1)
		fis[15] |= ATA_SRST;
	else 
		fis[15] &= ~ATA_SRST;
	fis[16] = 0;
	fis[17] = 0;
	fis[18] = 0;
	fis[19] = 0;
}

static int ahci_exec_polled_cmd(int port, int pmp, int is_cmd, u16 flags,
		unsigned long timeout_msec, int set)
{
	const u32 cmd_fis_len = 5;	/* five dwords */
	struct ahci_ioports *pp = &(probe_ent->port[port]);
	volatile u8 *port_mmio = (volatile u8 *)pp->port_mmio;
	u32 tmp;
	u8 fis[20];

	//u8 *fis = (u8 *)pp->cmd_tbl;
	ata_reset_fis_init(pmp, is_cmd, fis, set);
	memcpy((unsigned char *)pp->cmd_tbl, fis, 20);
	ahci_fill_cmd_slot(pp, cmd_fis_len | flags | (pmp << 12));

	CPU_IOFlushDCache(pp->cmd_slot, 32, SYNC_W); /*32~256*/
	CPU_IOFlushDCache(pp->cmd_tbl, 0x60, SYNC_W);
	CPU_IOFlushDCache(pp->rx_fis,AHCI_RX_FIS_SZ,SYNC_R);
	/*issue & wait */
	writel_with_flush(1, port_mmio + PORT_CMD_ISSUE);

	if(timeout_msec){
		tmp = ata_wait_register((void *)(port_mmio + PORT_CMD_ISSUE), 0x1, 0x1, 1, timeout_msec);
		if(tmp & 0x1){
			ahci_kick_engine(port, 1);
			return -EBUSY;
		}
	} else
		readl(port_mmio + PORT_CMD_ISSUE);

	return 0;
}
#if 0
static int ahci_check_ready(int port)
{
	struct ahci_ioports *pp = &(probe_ent->port[port]);
	volatile u8 *port_mmio = (volatile u8 *)pp->port_mmio;
	u8 status = readl(port_mmio + PORT_TFDATA) & 0xFF;

	if(!(status & ATA_BUSY))
		return 1;

	/* 0xff indicates either no device or device not ready */
	if(status == 0xff)
		return -ENODEV;

	return 0;
}
#endif

int ahci_do_softreset(int port, unsigned int *class,
			     int pmp, unsigned long deadline,
			     int (*check_ready)(int port))
{
	const char *reason = NULL;
	unsigned long msecs;
	struct ahci_ioports *pp = &(probe_ent->port[port]);
	volatile u8 *port_mmio = (volatile u8 *)pp->port_mmio;
	int rc;

	/* prepare for SRST (AHCI-1.1 10.4.1) */
	rc = ahci_kick_engine(port, 1);
	if (rc && rc != -EOPNOTSUPP)
		printf("failed to reset engine (errno=%d)\n", rc);

	//ata_tf_init(link->device, &tf);

	/* issue the first D2H Register FIS */
	//tf.ctl |= ATA_SRST;
	if (ahci_exec_polled_cmd(port, pmp, 0,
			AHCI_CMD_RESET | AHCI_CMD_CLR_BUSY, msecs, 1)) {
		rc = -EIO;
		reason = "1st FIS failed";
		goto fail;
	}

	/* spec says at least 5us, but be generous and sleep for 1ms */
	/* fix me */
	msleep(1);

	/* issue the second D2H Register FIS */
	//tf.ctl &= ~ATA_SRST;
	ahci_exec_polled_cmd(port, pmp, 0, 0, 0, 0);
#if 0
	/* wait for link to become ready */
	rc = ata_wait_after_reset(link, deadline, check_ready);
	/* link occupied, -ENODEV too is an error */
	if (rc) {
		reason = "device not ready";
		goto fail;
	}
#endif
	/* fix me*/
	ssleep(1);
	*class = readl(port_mmio + PORT_SIG);

	if(*class == 0xeb140000)
		pp->is_atapi = 1;
	else
		pp->is_atapi = 0;
	printf("EXIT, class=%u\t is_atapi = %d\n", *class, pp->is_atapi);
	return 0;

 fail:
	printf("softreset failed (%s)\n", reason);
	return rc;
}


//hd_driveid_t *ataid[AHCI_MAX_PORTS];


int ahci_sata_initialize(u32 reg,u32 flags)
{
	int rc;
	int i=flags;
	ahci_sata_t *sata;
	struct ahci_ioports *pp = &(probe_ent->port[i]);
	volatile u8 *port_mmio = (volatile u8 *)pp->port_mmio;


	printf("Port %d: SIG=0x%x\n", i, readl(port_mmio + PORT_SIG));
	if(i < 0 || i > (CFG_SATA_MAX_DEVICE -1))
	{
		printf("The sata index %d is out of ranges\n", i);
		return -1;
	} 
	memset(&sata_dev_desc[i], 0, sizeof(struct block_dev_desc));
	sata_dev_desc[i].if_type = IF_TYPE_SATA;
	sata_dev_desc[i].dev = i;
	sata_dev_desc[i].part_type = PART_TYPE_UNKNOWN;
	sata_dev_desc[i].type = DEV_TYPE_HARDDISK;
	sata_dev_desc[i].lba = 0;
	sata_dev_desc[i].blksz = 512;
	sata_dev_desc[i].block_read = ahci_sata_read;
	sata_dev_desc[i].block_write = ahci_sata_write;

	sata = (ahci_sata_t *)malloc(sizeof(ahci_sata_t), M_DEVBUF, M_NOWAIT);
	if(!sata)
	{
		printf("alloc the sata device struct failed\n");
		return -1;
	}
	memset((void *)sata, 0, sizeof(ahci_sata_t));
	sata_dev_desc[i].priv = (void *)sata;
	sprintf(sata->name, "SATA%d", i);
	
	sata->reg_base = reg;

	rc = ahci_port_start(i);
	ahci_set_feature((u8) i);
	ata_scsiop_inquiry(i);
	
	curr_device = 0;
	return rc;
}

static int waiting_for_cmd_completed(volatile u8 *offset,
		int timeout_msec,
		u32 sign)
{
	int i;
	u32 status;

	for (i = 0; ((status = readl(offset)) & sign) && i < timeout_msec; i++)
		msleep(1);

	return (i < timeout_msec) ? 0 : -1;
}




#define MAX_DATA_BYTE_COUNT  (4*1024*1024)

static int ahci_fill_sg(u8 port, unsigned char *buf, int buf_len)
{
	struct ahci_ioports *pp = &(probe_ent->port[port]);
	struct ahci_sg *ahci_sg = pp->cmd_tbl_sg;
	u32 sg_count;
	int i;

	sg_count = ((buf_len - 1) / MAX_DATA_BYTE_COUNT) + 1;
	if (sg_count > AHCI_MAX_SG) {
		printf("Error:Too much sg!\n");
		return -1;
	}

	for (i = 0; i < sg_count; i++) {
		ahci_sg->addr =
			cpu_to_le32(PHYSADDR((u32) buf + i * MAX_DATA_BYTE_COUNT));
		ahci_sg->addr_hi = 0;
		ahci_sg->flags_size = cpu_to_le32(0x3fffff &
				(buf_len < MAX_DATA_BYTE_COUNT
				 ? (buf_len - 1)
				 : (MAX_DATA_BYTE_COUNT - 1)));
		ahci_sg++;
		buf_len -= MAX_DATA_BYTE_COUNT;
	}

	return sg_count;
}


static void ahci_fill_cmd_slot(struct ahci_ioports *pp, u32 opts)
{
	pp->cmd_slot->opts = cpu_to_le32(opts);
	pp->cmd_slot->status = 0;
	pp->cmd_slot->tbl_addr = cpu_to_le32(PHYSADDR(pp->cmd_tbl & 0xffffffff));
	pp->cmd_slot->tbl_addr_hi = 0;
}

static void ahci_set_feature(u8 port)
{
	struct ahci_ioports *pp = &(probe_ent->port[port]);
	volatile u8 *port_mmio = (volatile u8 *)pp->port_mmio;
	u32 cmd_fis_len = 5;	/* five dwords */
	u8 fis[20];

	/*set feature */
	memset(fis, 0, 20);
	fis[0] = 0x27;
	fis[1] = 1 << 7;
	fis[2] = ATA_CMD_SETF;
	fis[3] = SETFEATURES_XFER;
	fis[12] = __ilog2(probe_ent->udma_mask + 1) + 0x40 - 0x01;

	memcpy((unsigned char *)pp->cmd_tbl, fis, 20);
	ahci_fill_cmd_slot(pp, cmd_fis_len);

	CPU_IOFlushDCache(pp->cmd_slot, 32, SYNC_W); /*32~256*/
	CPU_IOFlushDCache(pp->cmd_tbl, 0x60, SYNC_W);
	CPU_IOFlushDCache(pp->rx_fis,AHCI_RX_FIS_SZ,SYNC_R);

	writel_with_flush(1, port_mmio + PORT_CMD_ISSUE);
	readl(port_mmio + PORT_CMD_ISSUE);

	if (waiting_for_cmd_completed(port_mmio + PORT_CMD_ISSUE, 150, 0x1)) {
		printf("set feature error!\n");
	}
}


static int ahci_port_start(u8 port)
	/* [31:0]CI
	 * This field is bit significant. Each bit corresponds to a command
	 * slot, where bit 0 corresponds to command slot 0. This field is 
	 * set by the software to indicate to the Port that a command has
	 * been built in system memory for a command slot and may be sent
	 * to the device.
	 */
{
	struct ahci_ioports *pp = &(probe_ent->port[port]);
	volatile u8 *port_mmio = (volatile u8 *)pp->port_mmio;
	u32 port_status;
	u32 mem;

	printf("Enter start port: %d\n", port);
	port_status = readl(port_mmio + PORT_SCR_STAT);
	printf("Port %d status: %x\n", port, port_status);
	if ((port_status & 0xf) != 0x03) {
		printf("No Link on this port!\n");
		return -1;
	}

	mem = (u32) malloc(AHCI_PORT_PRIV_DMA_SZ + 2048, M_DEVBUF, M_NOWAIT);
	if (!mem) {
		free(pp, M_DEVBUF);
		printf("No mem for table!\n");
		return -ENOMEM;
	}

	mem = (mem + 0x800) & (~0x7ff);	/* Aligned to 2048-bytes */
	memset((u8 *) mem, 0, AHCI_PORT_PRIV_DMA_SZ);

	/*
	 * First item in chunk of DMA memory: 32-slot command table,
	 * 32 bytes each in size
	 */
	pp->cmd_slot = (struct ahci_cmd_hdr *)mem;
	printf("cmd_slot = 0x%x\n", pp->cmd_slot);
	mem += (AHCI_CMD_SLOT_SZ + 224);

	/*
	 * Second item: Received-FIS area
	 */
	pp->rx_fis = mem;
	mem += AHCI_RX_FIS_SZ;

	/*
	 * Third item: data area for storing a single command
	 * and its scatter-gather table
	 */
	pp->cmd_tbl = mem;
	printf("cmd_tbl_dma = 0x%x\n", pp->cmd_tbl);

	mem += AHCI_CMD_TBL_HDR;
	pp->cmd_tbl_sg = (struct ahci_sg *)mem;

	writel_with_flush(PHYSADDR((u32) pp->cmd_slot), port_mmio + PORT_LST_ADDR);

	writel_with_flush(PHYSADDR(pp->rx_fis), port_mmio + PORT_FIS_ADDR);

	writel_with_flush(PORT_CMD_ICC_ACTIVE | PORT_CMD_FIS_RX |
			PORT_CMD_POWER_ON | PORT_CMD_SPIN_UP |
			PORT_CMD_START, port_mmio + PORT_CMD);

	printf("Exit start port %d\n", port);

	return 0;
}

void print_data(u8 *buf_data, int len)
{
	int i,k;
	int j;
	u8 *buf = buf_data;

	i = len / 16;
	k = len % 16;
	
	printf ("lxy: receive %d data is :\n", len);

	for (; i > 0; i--)
	{
		for (j = 0; j < 16; j++)
			printf ("%02x  ,", buf[j]);
		printf ("\n");
		buf += 16;
	}

	for (j = 0; j < k; j++) 
		printf ("%02x  ,", buf[j]);
	printf ("\n");
}

static int get_ahci_device_data(u8 port, u8 *fis, int fis_len, u8 *buf,
		int buf_len, u8 *cdb,int is_write)
{

	struct ahci_ioports *pp = &(probe_ent->port[port]);
	volatile u8 *port_mmio = (volatile u8 *)pp->port_mmio;
	u32 opts;
	u32 port_status;
	/* [31:0]CI
	 * This field is bit significant. Each bit corresponds to a command
	 * slot, where bit 0 corresponds to command slot 0. This field is 
	 * set by the software to indicate to the Port that a command has
	 * been built in system memory for a command slot and may be sent
	 * to the device.
	 */
	int sg_count;


	if (port > probe_ent->n_ports) {
		printf("Invaild port number %d\n", port);
		return -1;
	}

	port_status = readl(port_mmio + PORT_SCR_STAT);
	if ((port_status & 0xf) != 0x03) {
		printf("No Link on port %d!\n", port);
		return -1;
	}

	memcpy((unsigned char *)pp->cmd_tbl, fis, fis_len);

	if(cdb != NULL)
	{
		memset((unsigned char *)pp->cmd_tbl + AHCI_CMD_TBL_CDB, 0, 32);
		memcpy((unsigned char *)pp->cmd_tbl + AHCI_CMD_TBL_CDB, cdb, ATAPI_COMMAND_LEN);
	}
	sg_count = ahci_fill_sg(port, buf, buf_len);
	opts = (fis_len >> 2) | (sg_count << 16);
	if(cdb != NULL)
	{
		opts |= AHCI_CMD_ATAPI | AHCI_CMD_PREFETCH;
	}
	ahci_fill_cmd_slot(pp, opts);

	CPU_IOFlushDCache(pp->cmd_slot, 32, SYNC_W); /*32~256*/
	CPU_IOFlushDCache(pp->cmd_tbl, 0x60, SYNC_W);
	CPU_IOFlushDCache(pp->cmd_tbl_sg, sg_count*16 , SYNC_W);
	CPU_IOFlushDCache(pp->rx_fis,AHCI_RX_FIS_SZ,SYNC_R);
	CPU_IOFlushDCache(buf,buf_len,is_write?SYNC_W:SYNC_R);

	/* [31:0]CI
	 * This field is bit significant. Each bit corresponds to a command
	 * slot, where bit 0 corresponds to command slot 0. This field is 
	 * set by the software to indicate to the Port that a command has
	 * been built in system memory for a command slot and may be sent
	 * to the device.
	 */
	writel_with_flush(1, port_mmio + PORT_CMD_ISSUE);
	
	if (waiting_for_cmd_completed(port_mmio + PORT_CMD_ISSUE, 150, 0x1)) {
		printf("timeout exit!\n");
		return -1;
	}
//	printf("get_ahci_device_data: %d byte transferred.\n", pp->cmd_slot->status);

//	print_data(buf, pp->cmd_slot->status);
	/* Indicates the current byte count that has transferred on device
	 * writes(system memory to device) or 
	 * device reads(device to system memory).
	 */
	return pp->cmd_slot->status;
}

static void dump_ataid(hd_driveid_t *ataid)
{
	printf("(49)ataid->capability = 0x%x\n", ataid->capability);
	printf("(53)ataid->field_valid =0x%x\n", ataid->field_valid);
	printf("(63)ataid->dma_mword = 0x%x\n", ataid->dma_mword);
	printf("(64)ataid->eide_pio_modes = 0x%x\n", ataid->eide_pio_modes);
	printf("(75)ataid->queue_depth = 0x%x\n", ataid->queue_depth);
	printf("(80)ataid->major_rev_num = 0x%x\n", ataid->major_rev_num);
	printf("(81)ataid->minor_rev_num = 0x%x\n", ataid->minor_rev_num);
	printf("(82)ataid->command_set_1 = 0x%x\n", ataid->command_set_1);
	printf("(83)ataid->command_set_2 = 0x%x\n", ataid->command_set_2);
	printf("(84)ataid->cfsse = 0x%x\n", ataid->cfsse);
	printf("(85)ataid->cfs_enable_1 = 0x%x\n", ataid->cfs_enable_1);
	printf("(86)ataid->cfs_enable_2 = 0x%x\n", ataid->cfs_enable_2);
	printf("(87)ataid->csf_default = 0x%x\n", ataid->csf_default);
	printf("(88)ataid->dma_ultra = 0x%x\n", ataid->dma_ultra);
	printf("(93)ataid->hw_config = 0x%x\n", ataid->hw_config);
}

static u64 sata_id_n_sectors(u16 *id)
{
	if (ata_id_has_lba(id)) {
		if (ata_id_has_lba48(id))
			return ata_id_u64(id, ATA_ID_LBA48_SECTORS);
		else
			return ata_id_u32(id, ATA_ID_LBA_SECTORS);
	} else {
		return 0;
	}
}
/*
 * SCSI INQUIRY command operation.
 */
static int ata_scsiop_inquiry(int port)
{
	u8 fis[20];
	u8 *tmpid;
	
	memset(fis, 0, 20);
	/* Construct the FIS */
	fis[0] = 0x27;		/* Host to device FIS. */
	fis[1] = 1 << 7;	/* Command FIS. */
	fis[2] = ATA_CMD_IDENT;	/* Command byte. */

	/* Read id from sata */
	if (!(tmpid = malloc(sizeof(hd_driveid_t), M_DEVBUF, M_NOWAIT)) ){
		printf("malloc in ata_scsiop_inquiry failed.\n");
		return -ENOMEM;
	}

	if (!get_ahci_device_data(port, (u8 *) & fis, 20,
			tmpid, sizeof(hd_driveid_t), NULL, 0)) {
		printf("scsi_ahci: SCSI inquiry command failure.\n");
		return -EIO;
	}
#if 0
	if (ataid[port])
		free(ataid[port], M_DEVBUF);
	ataid[port] = (hd_driveid_t *) tmpid;
#endif
	sata_dev_desc[port].lba = sata_id_n_sectors((u16 *)tmpid);
	if(ata_id_has_lba48((u16 *)tmpid))
	{
		sata_dev_desc[port].lba48 = 1;
		lba48[port] = 1;
	}
	dump_ataid(/*ataid[port]*/ (hd_driveid_t *)tmpid );
	return 0;
}



static int ahcisata_match(struct device *parent,
			void *match,
			void *aux
			)
{
	ahci_sata_info_t info;
	int err, class;
	
	info = *(ahci_sata_info_t *)aux;

	port_to_dev_info[dev_count].dev_index = dev_count;
	port_to_dev_info[dev_count++].port_index = info.flags;

	err = ahci_sata_initialize(info.sata_reg_base, info.flags);
	if(err)
		return 0;
	else{
		ahci_do_softreset(info.flags, &class, 0, 0, NULL);
		return 1;
	}
}


static void ahcisata_attach(struct device *parent,
			struct device *self,
			void *aux
			)
{
	return ;
}

static void make_read_command_packet(u8 *pc, int nframes, int sector)
{
	pc[0] = GPCMD_READ_10; 
	pc[7] = (nframes >> 8) & 0xff;	//How many blocks 
	pc[8] = nframes & 0xff;
	pc[2] = (sector >> 24) & 0xff;	//Block start address 
	pc[3] = (sector >> 16) & 0xff; 
	pc[4] = (sector >> 8) & 0xff;
	pc[5] = (sector >> 0) & 0xff;
}

static u32 ahci_sata_rw_cmd_ext(int dev, u32 start, u32 blkcnt, u8 *buffer, int is_write)
{
	//ahci_sata_t *sata = (ahci_sata_t *)sata_dev_desc[dev].priv;
	struct ahci_ioports *pp = &(probe_ent->port[dev]);
	//volatile u8 *port_mmio = (volatile u8 *)pp->port_mmio;
	struct sata_fis_h2d h2d;
	struct cfis *cfis;
	u64 block;
	u8 *pc;

	block = (u64)start;
	cfis = (struct cfis *)&h2d;

	memset((void *)cfis, 0, sizeof(struct cfis));

	cfis->fis_type = SATA_FIS_TYPE_REGISTER_H2D;
	((u8 *)cfis)[1] = 0x80; /* is command */

	((u8 *)cfis)[2] = (is_write) ? 0xCA
		: 0xC8;

	cfis->lba_high_exp = (block >> 40) & 0xff;
	cfis->lba_mid_exp = (block >> 32) & 0xff;
	cfis->lba_low_exp = (block >> 24) & 0xff;
	cfis->lba_high = (block >> 16) & 0xff;
	cfis->lba_mid = (block >> 8) & 0xff;
	cfis->lba_low = block & 0xff;
	cfis->device = 0xe0;
	cfis->sector_count_exp = (blkcnt >> 8) & 0xff;
	cfis->sector_count = blkcnt & 0xff;

	pc = NULL;
	if(pp->is_atapi)
	{
		pc = (u8 *)malloc(ATAPI_COMMAND_LEN, M_DEVBUF, M_NOWAIT);
		if(pc == NULL)
		{
			printf("%s:%d malloc failed.\n", __FILE__, __LINE__);
			return -1;
		}
		memset(pc, 0, ATAPI_COMMAND_LEN);
		make_read_command_packet(pc, block, blkcnt);	
	}

	//atp_sata_exec_cmd(sata, cfis, CMD_ATA, 0, buffer, ATA_SECT_SIZE * blkcnt);
	get_ahci_device_data(dev, (u8 *)cfis, sizeof(struct cfis), buffer, ATA_SECT_SIZE * blkcnt, pc, is_write);
	return blkcnt;
}

static u32 ata_low_level_rw_lba48(int dev, u32 blknr, u32 blkcnt, void *buffer, int is_write)
{
	u32 start, blks;
	u8 *addr;
	int max_blks;

	start = blknr;
	blks = blkcnt;
	addr = (u8 *)buffer;

	max_blks = ATA_MAX_SECTORS_LBA48;
	do {
		if (blks > max_blks)
		{
			ahci_sata_rw_cmd_ext(dev, start, max_blks, addr, is_write);
			start += max_blks;
			blks -= max_blks;
			addr += ATA_SECT_SIZE * max_blks;
		} else
		{
			ahci_sata_rw_cmd_ext(dev, start, blks, addr, is_write);
			start += blks;
			blks = 0;
			addr += ATA_SECT_SIZE * blks;
		}
	} while (blks != 0);

	return blkcnt;
}

static u32 ahci_sata_rw_cmd(int dev, u32 start, u32 blkcnt, u8 *buffer, int is_write)
{
	//ahci_sata_t *sata = (ahci_sata_t *)sata_dev_desc[dev].priv;
	struct ahci_ioports *pp = &(probe_ent->port[dev]);
	struct sata_fis_h2d h2d;
	struct cfis *cfis;
	u8 *pc;
	u32 block;

	block = start;
	cfis = (struct cfis *)&h2d;

	memset((void *)cfis, 0, sizeof(struct cfis));

	cfis->fis_type = SATA_FIS_TYPE_REGISTER_H2D;
	((u8 *)cfis)[1] = 1 << 7; /* is command */
	cfis->command = (is_write) ? 0xca: 0xc8;
	cfis->device = 0xe0;

	cfis->device |= (block >> 24) & 0xf;
	cfis->lba_high = (block >> 16) & 0xff;
	cfis->lba_mid = (block >> 8) & 0xff;
	cfis->lba_low = block & 0xff;
	cfis->sector_count = (u8)(blkcnt & 0xff);

	if(pp->is_atapi)
	{
		pc = (u8 *)malloc(ATAPI_COMMAND_LEN, M_DEVBUF, M_NOWAIT);
		if(pc == NULL)
		{
			printf("%s:%d malloc failed.\n", __FILE__, __LINE__);
			return -1;
		}
		memset(pc, 0, ATAPI_COMMAND_LEN);
		make_read_command_packet(pc, block, blkcnt);	
	}

	//atp_sata_exec_cmd(sata, cfis, CMD_ATA, 0, buffer, ATA_SECT_SIZE * blkcnt);
	get_ahci_device_data(dev, (u8 *)cfis, sizeof(struct cfis), buffer, ATA_SECT_SIZE * blkcnt, pc, is_write);
	return blkcnt;
}

static u32 ata_low_level_rw_lba28(int dev, u32 blknr, u32 blkcnt, void *buffer, int is_write)
{
	u32 start, blks;
	u8 *addr;
	int max_blks;

	start = blknr;
	blks = blkcnt;
	addr = (u8 *)buffer;

	max_blks = ATA_MAX_SECTORS;
	do {
		if (blks > max_blks)
		{
			ahci_sata_rw_cmd(dev, start, max_blks, addr, is_write);
			start += max_blks;
			blks -= max_blks;
			addr += ATA_SECT_SIZE * max_blks;
		} else
		{
			ahci_sata_rw_cmd(dev, start, blks, addr, is_write);
			start += blks;
			blks = 0;
			addr += ATA_SECT_SIZE * blks;
		}
	} while (blks != 0);

	return blkcnt;
}

/*
 * SATA interface between low level driver and command layer
 * */
ulong ahci_sata_read(int dev, 
		unsigned long blknr, 
		lbaint_t blkcnt, 
		void *buffer)
{
	u32 rc;
	dev = port_to_dev_info[dev].port_index;
	if(lba48[dev]){
		rc = ata_low_level_rw_lba48(dev, blknr, blkcnt, buffer, READ_CMD);		
	} else {
		rc = ata_low_level_rw_lba28(dev, blknr, blkcnt, buffer, READ_CMD);
	}
	return rc;
}

ulong ahci_sata_write(int dev, 
		unsigned long blknr, 
		lbaint_t blkcnt, 
		void *buffer)
{
	u32 rc;
	dev = port_to_dev_info[dev].port_index;
	if(lba48[dev]){
		rc = ata_low_level_rw_lba48(dev, blknr, blkcnt, buffer, WRITE_CMD);		
	} else {
		rc = ata_low_level_rw_lba28(dev, blknr, blkcnt, buffer, WRITE_CMD);
	}
	return rc;
}

/******************satafs***********************/
/*
 * Supported paths
 * 	/dev/sata
 */

#include <fcntl.h>
#include <file.h>
#include <sys/buf.h>
#include <ramfile.h>
#include <sys/unistd.h>
#undef _KERNEL_
#include <errno.h>


void ahci_sata_strategy(struct buf *bp)
{
	struct ahci_sata_softc *priv;
	unsigned int blkno, blkcnt;
	int ret;

	priv = ahci_sata_lookup(bp->b_dev);
	blkno = bp->b_blkno;
	
	blkno = blkno / (priv->bs/DEV_BSIZE);
	blkcnt = howmany(bp->b_bcount, priv->bs);

	/* Valid request ? */
	if(bp->b_blkno < 0 ||
			(bp->b_bcount % priv->bs) != 0 ||
			(bp->b_bcount / priv->bs) >= (1 << NBBY)) {
		bp->b_error = EINVAL;
		printf("Invalid request\n");
		goto bad;
	}

	/* If it's a null transfer, return immediately. */
	if(bp->b_bcount == 0)
		goto done;
	
	if(bp->b_flags & B_READ){
		fault_timeout = 0;
		ret = ahci_sata_read(priv->dev, blkno, blkcnt, bp->b_data);
		if(ret != blkcnt || fault_timeout)
			bp->b_flags |= B_ERROR;
		dotik(30000, 0);
	}
	else
	{
		fault_timeout = 0;
		ret = ahci_sata_write(priv->dev, blkno, blkcnt, bp->b_data);
		if(ret != blkcnt || fault_timeout)
			bp->b_flags |= B_ERROR;
		dotik(30000, 0);
	}
done:
	biodone(bp);
	return;
bad:
	bp->b_flags |= B_ERROR;
	biodone(bp);
}

int ahcisata_open(
		dev_t dev,
		int flag, int fmt,
		struct proc *p)
{
	//char sata_cmd[0x200];
	//char *sata_env;
	struct ahci_sata_softc *priv;
	priv=ahci_sata_lookup(dev);
	if(!priv)return -1;
	priv->bs=ATA_SECT_SIZE;
	priv->count=-1;
	return 0;
}


	int
ahcisata_close( dev_t dev,
		int flag, int fmt,
		struct proc *p)
{
	return 0;
}

int
ahcisata_read(
		dev_t dev,
		struct uio *uio,
		int flags)
{
	return physio(ahci_sata_strategy, NULL, dev, B_READ, minphys, uio);
}

int
ahcisata_write(
		dev_t dev,
		struct uio *uio,
		int flags)
{
	return (physio(ahci_sata_strategy, NULL, dev, B_WRITE, minphys, uio));
}
/******************** satafs end *********************/

int cmd_sata_ahci(int argc, char *argv[])
{
	int rc = 0;
	int i=0;
	switch (argc) {
		case 0:
		case 1:
			printf("Hello Sata!!!\n");
			return 1;
		case 2:
			if (strncmp(argv[1],"inf", 3) == 0) {
				int i;
				printf("\n");
				for (i = 0; i < CFG_SATA_MAX_DEVICE; ++i) {
					if (sata_dev_desc[i].type == DEV_TYPE_UNKNOWN)
					{
						printf("sata_dev_desc[%d].type = %d\n",i,sata_dev_desc[i].type);
						continue;
					}
					printf ("SATA device %d:\n", i);
				}
				return 0;
			} else if (strncmp(argv[1],"dev", 3) == 0) {
				if ((curr_device < 0) || (curr_device >= CFG_SATA_MAX_DEVICE)) {
					printf("dev-curr_device=%d\n",curr_device);
					printf("no SATA devices available\n");
					return 1;
				}
				printf("SATA device %d:\n", curr_device);
				return 0;
			} else if (strncmp(argv[1],"part",4) == 0) {
				int dev, ok;

				for (ok = 0, dev = 0; dev < CFG_SATA_MAX_DEVICE; ++dev) {
					if (sata_dev_desc[dev].part_type != PART_TYPE_UNKNOWN) {
						++ok;
					}
				}
				if (!ok) {
					printf("\nno SATA devices available\n");
					rc ++;
				}
				return rc;
			}
			return 1;
		case 3:
			if (strncmp(argv[1], "dev", 3) == 0) {
				int dev = (int)strtoul(argv[2], NULL, 10);

				printf("SATA device %d:\n", dev);
				if (dev >= CFG_SATA_MAX_DEVICE) {
					printf ("unknown device\n");
					return 1;
				}

				if (sata_dev_desc[dev].type == DEV_TYPE_UNKNOWN)
					return 1;

				curr_device = dev;

				printf("... is now current device\n");

				return 0;
			} else if (strncmp(argv[1], "part", 4) == 0) {
				int dev = (int)strtoul(argv[2], NULL, 10);

				if (sata_dev_desc[dev].part_type != PART_TYPE_UNKNOWN) {
				} else {
					printf("\nSATA device %d not available\n", dev);
					rc = 1;
				}
				return rc;
			}
			return 1;

		default: /* at least 4 args */
			if (strcmp(argv[1], "read") == 0) {
				ulong addr = strtoul(argv[2], NULL, 16);
				ulong cnt = strtoul(argv[4], NULL, 16);
				ulong n;
				lbaint_t blk = strtoul(argv[3], NULL, 16);

				printf("\nSATA read: device %d block # %ld, count %ld ... ",
						curr_device, blk, cnt);

				n = ahci_sata_read(curr_device, blk, cnt, (u32 *)addr);
				printf("the buffer address is 0x%x\n",addr);
				for(i=0;i<n*ATA_SECT_SIZE;)
				{
					printf("%8x",*((u32 *)addr+i));
					i++;
					if(i%8==0)
						printf("\n");

				}

				/* flush cache after read */
				flush_cache(addr, cnt * sata_dev_desc[curr_device].blksz);

				printf("n = %d,cnt = %d\n",n,cnt);
				printf("%ld blocks read: %s\n", n, (n==cnt) ? "OK" : "ERROR");
				return (n == cnt) ? 0 : 1;
			} else if (strcmp(argv[1], "write") == 0) {
				ulong addr = strtoul(argv[2], NULL, 16);
				ulong cnt = strtoul(argv[4], NULL, 16);
				ulong n;

				lbaint_t blk = strtoul(argv[3], NULL, 16);

				printf("\nSATA write: device %d block # %ld, count %ld ... ",
						curr_device, blk, cnt);

				n = ahci_sata_write(curr_device, blk, cnt, (u32 *)addr);

				printf("%ld blocks written: %s\n",
						n, (n == cnt) ? "OK" : "ERROR");
				return (n == cnt) ? 0 : 1;
			} else {
				rc = 1;
			}

			return rc;
	}
}
/*
 *  *  Command table registration
 *   *  ==========================
 *    */

static const Cmd Cmds[] =
{
	{"ahcisata"},
	{"ahcisata", "[info device part read write]", 0, "ahci sata read write", cmd_sata_ahci, 1, 99, 0},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

	void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

