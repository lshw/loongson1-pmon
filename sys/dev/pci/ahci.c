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
#include <autoconf.h>

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
#include "ahci.h"
#include "ahcisata.h"

#define	AHCI_CLOCK_25MHZ	0x34682650
#define	AHCI_CLOCK_50MHZ	0x30682650
#define	AHCI_CLOCK_100MHZ	0x38682650
#define	AHCI_CLOCK_125MHZ	0x38502650

struct ahci_probe_ent *probe_ent = NULL;

/* to get some global routines like printf */
#include "etherboot.h"

static int ahci_host_init(struct ahci_probe_ent *probe_ent);
static int ahci_init_one(u32 regbase);


static int ahci_match (struct device *, void *, void *);
static void ahci_attach (struct device *, struct device *, void *);

static inline u32 ahci_port_base(u32 base, u32 port)
{
	return base + 0x100 + (port * 0x80);
}


static void ahci_setup_port(struct ahci_ioports *port, unsigned long base,
		unsigned int port_idx)
{
	base = ahci_port_base(base, port_idx);

	port->cmd_addr = base;
	port->scr_addr = base + PORT_SCR;
}

struct cfattach ahci_ca = {
        sizeof(ahci_sata_t),ahci_match, ahci_attach,
};
struct cfdriver ahci_cd = {
        NULL, "ahci", DV_DULL
};

static int ahci_match(
                struct device *parent,
                void   *match,
                void * aux
                )
{
        struct pci_attach_args *pa = aux;
        if(PCI_VENDOR(pa->pa_id) == PCI_VENDOR_SATA &&
                PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SATA)
                return 1;
        return 0;
}

static void ahci_attach(struct device * parent, struct device * self, void *aux)
{
	struct pci_attach_args *pa = aux;	
	//struct pci_chipset_tag_t *pc = pa->pa_pc;
	bus_space_tag_t memt = pa->pa_memt;
	bus_addr_t membasep;
	bus_size_t memsizep;
	//int reg;
	int i;
	u32 linkmap;
	ahci_sata_info_t info;

	printf("\n~~~~~~~~~~~~~~~~~ahcisata_attach~~~~~~~~~~~~~~~~~~\n");
	if(pci_mem_find(NULL, pa->pa_tag, 0x24, &membasep, &memsizep, NULL))
	{
		printf(" Can't find mem space\n");
		return;
	}
	printf("Found memory space: memt->bus_base=0x%x, baseaddr=0x%x size=0x%x\n", 
			memt->bus_base, (u32)(membasep), (u32)(memsizep));
		
	if(ahci_init_one((u32)(memt->bus_base + (u32)(membasep)))){
		printf("ahci_init_one failed.\n");
	}
	
	linkmap = probe_ent->link_port_map;
	printf("linkmap=%x\n",linkmap);
	for (i = 0; i < 1; i++) {
		if (((linkmap >> i) & 0x01)) {
			info.sata_reg_base = memt->bus_base + (u32)(membasep) + 100 + i * 0x80;
			info.flags = i;
			info.aa_link.aa_type=0xff; //just for not match ide
			config_found(self,(void *)&info,NULL);
		}
	}

	printf("\n~~~~~~~~~~~~~~~~~ahcisata_attach~~~~~~~~~~~~~~~~~~\n");
}
#if 1
static int lahci_match(
                struct device *parent,
                void   *match,
                void * aux
                )
{
 return 1;
}

static void lahci_attach(struct device * parent, struct device * self, void *aux)
{
	struct confargs *cf = aux;
	bus_space_handle_t regbase;
	int i;
	u32 linkmap;
	ahci_sata_info_t info;

	*(volatile int *)0xbfd00420 &= ~(1 << 22);

 /*ls1a adjust sata phy clock added by menghaibo*/
//        *(volatile int *)0xbfd00424 |= 0x80000000;
//        *(volatile int *)0xbfd00418  = 0x38682650;
        *(volatile int *)0xbfd00418  = AHCI_CLOCK_125MHZ;
        *(volatile int *)0xbfe30000 &= 0x0;
		
	regbase = (bus_space_handle_t)cf->ca_baseaddr;;
	if(ahci_init_one(regbase)){
		printf("ahci_init_one failed.\n");
	}
	
	linkmap = probe_ent->link_port_map;
	printf("linkmap=%x\n",linkmap);
	for (i = 0; i < 2; i++) {
		if (((linkmap >> i) & 0x01)) {
			info.sata_reg_base = regbase + 100 + i * 0x80;
			info.flags = i;
			info.aa_link.aa_type=0xff; //just for not match ide
			config_found(self,(void *)&info,NULL);
		}
	}
}

struct cfattach lahci_ca = {
        sizeof(ahci_sata_t),lahci_match, lahci_attach,
};

struct cfdriver lahci_cd = {
        NULL, "ahci", DV_DULL
};
#endif

static void ahci_enable_ahci(void *mmio)
{
	int i;
	u32 tmp;

	/* turn on AHCI_EN */
	tmp = readl(mmio + HOST_CTL);
	if (tmp & HOST_AHCI_EN)
		return;

	/* Some controllers need AHCI_EN to be written multiple times.
	 * Try a few times before giving up.
	 */
	for (i = 0; i < 5; i++) {
		tmp |= HOST_AHCI_EN;
		writel(tmp, mmio + HOST_CTL);
		tmp = readl(mmio + HOST_CTL);	/* flush && sanity check */
		if (tmp & HOST_AHCI_EN)
			return;
		msleep(10);
	}
}

static int ahci_host_init(struct ahci_probe_ent *probe_ent)
{
	volatile u8 *mmio = (volatile u8 *)probe_ent->mmio_base;
	u32 tmp, cap_save;
	//u16 tmp16;
	int i, j;
	volatile u8 *port_mmio;
	//unsigned short vendor;

	cap_save = readl(mmio + HOST_CAP);
	cap_save &= ((1 << 28) | (1 << 17));
	cap_save |= (1 << 27);
	
	/*
	 * Make sure AHCI mode is enabled before accessing CAP
	 * */
	ahci_enable_ahci(mmio);

	/* global controller reset */
	tmp = readl(mmio + HOST_CTL);
	if ((tmp & HOST_RESET) == 0){
		printf("Global controller reset.\n");
		writel_with_flush(tmp | HOST_RESET, mmio + HOST_CTL);
	}

	/* reset must complete within 1 second, or
	 * the hardware should be considered fried.
	 */
	ssleep(1);

	tmp = readl(mmio + HOST_CTL);
	if (tmp & HOST_RESET) {
		printf("controller reset failed (0x%x)\n", tmp);
		return -1;
	}

	//writel_with_flush(HOST_AHCI_EN, mmio + HOST_CTL);
	writel(cap_save, mmio + HOST_CAP);
	writel_with_flush(0xf, mmio + HOST_PORTS_IMPL);


	probe_ent->cap = readl(mmio + HOST_CAP);
	probe_ent->port_map = readl(mmio + HOST_PORTS_IMPL);
	probe_ent->n_ports = (probe_ent->cap & 0x1f) + 1;

	printf("cap 0x%x  port_map 0x%x  n_ports %d\n",
			probe_ent->cap, probe_ent->port_map, probe_ent->n_ports);

	for (i = 0; i < probe_ent->n_ports; i++) {
		probe_ent->port[i].port_mmio = ahci_port_base((u32) mmio, i);
		port_mmio = (u8 *) probe_ent->port[i].port_mmio;
		ahci_setup_port(&probe_ent->port[i], (unsigned long)mmio, i);

		/* make sure port is not active */
		tmp = readl(port_mmio + PORT_CMD);
		if (tmp & (PORT_CMD_LIST_ON | PORT_CMD_FIS_ON |
					PORT_CMD_FIS_RX | PORT_CMD_START)) {
			tmp &= ~(PORT_CMD_LIST_ON | PORT_CMD_FIS_ON |
					PORT_CMD_FIS_RX | PORT_CMD_START);
			writel_with_flush(tmp, port_mmio + PORT_CMD);

			/* spec says 500 msecs for each bit, so
			 * this is slightly incorrect.
			 */
			msleep(500);
		}

		tmp = readl(port_mmio + PORT_SCR);
		if((tmp & 0x3)){
			tmp &= ~0x3;
			writel_with_flush(tmp, port_mmio + PORT_SCR);
			msleep(500);
		}

		writel(PORT_CMD_SPIN_UP, port_mmio + PORT_CMD);

		j = 0;
		while (j < 100) {
			msleep(10);
			tmp = readl(port_mmio + PORT_SCR_STAT);
			if ((tmp & 0xf) == 0x3)
				break;
			j++;
		}
		printf ("j = %ld !\n", j);

		tmp = readl(port_mmio + PORT_SCR_ERR);
		printf("PORT_SCR_ERR 0x%x\n", tmp);
		writel(tmp, port_mmio + PORT_SCR_ERR);

		/* ack any pending irq events for this port */
		tmp = readl(port_mmio + PORT_IRQ_STAT);
		printf("PORT_IRQ_STAT 0x%x\n", tmp);
		if (tmp)
			writel(tmp, port_mmio + PORT_IRQ_STAT);

		writel(1 << i, mmio + HOST_IRQ_STAT);

		/* set irq mask (enables interrupts) */
		writel(DEF_PORT_IRQ, port_mmio + PORT_IRQ_MASK);

		/*register linkup ports */
		tmp = readl(port_mmio + PORT_SCR_STAT);
		printf("Port %d status: 0x%x\n", i, tmp);
		if ((tmp & 0xf) == 0x03)
			probe_ent->link_port_map |= (0x01 << i);
	}

	tmp = readl(mmio + HOST_CTL);
	printf("HOST_CTL 0x%x\n", tmp);
	writel(tmp | HOST_IRQ_EN, mmio + HOST_CTL);
	tmp = readl(mmio + HOST_CTL);
	printf("HOST_CTL 0x%x\n", tmp);

	return 0;
}

static void ahci_print_info(struct ahci_probe_ent *probe_ent)
{
	volatile u8 *mmio = (volatile u8 *)probe_ent->mmio_base;
	u32 vers, cap, impl, speed;
	const char *speed_s;
	//u16 cc;
	const char *scc_s;

	vers = readl(mmio + HOST_VERSION);
	cap = probe_ent->cap;
	impl = probe_ent->port_map;

	speed = (cap >> 20) & 0xf;
	if (speed == 1)
		speed_s = "1.5";
	else if (speed == 2)
		speed_s = "3";
	else
		speed_s = "?";

	scc_s = "SATA";

	printf("AHCI %02x%02x.%02x%02x "
			"%u slots %u ports %s Gbps 0x%x impl %s mode\n",
			(vers >> 24) & 0xff,
			(vers >> 16) & 0xff,
			(vers >> 8) & 0xff,
			vers & 0xff,
			((cap >> 8) & 0x1f) + 1, (cap & 0x1f) + 1, speed_s, impl, scc_s);

	printf("flags: "
			"%s%s%s%s%s%s"
			"%s%s%s%s%s%s%s\n",
			cap & (1 << 31) ? "64bit " : "",
			cap & (1 << 30) ? "ncq " : "",
			cap & (1 << 28) ? "ilck " : "",
			cap & (1 << 27) ? "stag " : "",
			cap & (1 << 26) ? "pm " : "",
			cap & (1 << 25) ? "led " : "",
			cap & (1 << 24) ? "clo " : "",
			cap & (1 << 19) ? "nz " : "",
			cap & (1 << 18) ? "only " : "",
			cap & (1 << 17) ? "pmp " : "",
			cap & (1 << 15) ? "pio " : "",
			cap & (1 << 14) ? "slum " : "",
			cap & (1 << 13) ? "part " : "");
}

static int ahci_init_one(u32 regbase)
{
	//u16 vendor;
	int rc;

	//memset((void *)ataid, 0, sizeof(hd_driveid_t *) * AHCI_MAX_PORTS);
#if MY_MALLOC
	probe_ent = malloc(sizeof(struct ahci_probe_ent));
#else
	probe_ent = malloc(sizeof(struct ahci_probe_ent), M_DEVBUF, M_NOWAIT);
#endif
	memset(probe_ent, 0, sizeof(struct ahci_probe_ent));

	probe_ent->host_flags = ATA_FLAG_SATA
		| ATA_FLAG_NO_LEGACY
		| ATA_FLAG_MMIO
		| ATA_FLAG_PIO_DMA
		| ATA_FLAG_NO_ATAPI;
	probe_ent->pio_mask = 0x1f;
	probe_ent->udma_mask = 0x7f;	/*Fixme,assume to support UDMA6 */

	probe_ent->mmio_base = regbase;


	/* Take from kernel:
	 * JMicron-specific fixup:
	 * make sure we're in AHCI mode
	 */
	/* initialize adapter */
	rc = ahci_host_init(probe_ent);
	if (rc)
		goto err_out;

	ahci_print_info(probe_ent);

	return 0;

err_out:
	return rc;
}

#if 0
int cmd_msqt_sata __P((int, char *[]));
/*
 * Motherboard Signal Quality Test for ATP SATA Controller
 */


int cmd_msqt_sata(int ac, char *av[])
{
	u16 word,P0TxTEST;
	u16 output;
//	device_t dev = _pci_make_tag(0, 0x11, 0);
	int addr,addr1;
	/* enable sata0,sata1,sata2,sata3,sata4,sata5 phyctrl*/
	if (!strcmp(*(av + 1), "port0")){
		printf("Enable port0\n");
		addr = 0xb0;
		if(!strcmp(*(av + 3), "1.5G")) {
			addr1 = 0x88;
		}else{
			addr1 = 0xa0;
		}
		word = pci_read_config16(dev, 0xb0);
		word |= 0x01;
		pci_write_config16(dev, 0xb0, word);
	}else if (!strcmp(*(av + 1), "port1")){
		printf("Enable port1\n");
		addr = 0xb8;
		if(!strcmp(*(av + 3), "1.5G")) {
			addr1 = 0x8c;
		}else{
			addr1 = 0xa2;
		}
		word = pci_read_config16(dev, 0xb8);
		word |= 0x01;
		pci_write_config16(dev, 0xb8, word);
	}else if (!strcmp(*(av + 1), "port2")){
		printf("Enable port2\n");
		addr = 0xc0;
		if(!strcmp(*(av + 3), "1.5G")) {
			addr1 = 0x90;
		}else{
			addr1 = 0xa4;
		}
		word = pci_read_config16(dev, 0xc0);
		word |= 0x01;
		pci_write_config16(dev, 0xc0, word);
	}else if (!strcmp(*(av + 1), "port3")){
		printf("Enable port3\n");
		addr = 0xc8;
		if(!strcmp(*(av + 3), "1.5G")) {
			addr1 = 0x94;
		}else{
			addr1 = 0xa6;
		}
		word = pci_read_config16(dev, 0xc8);
		word |= 0x01;
		pci_write_config16(dev, 0xc8, word);
	}else if (!strcmp(*(av + 1), "port4")){
		printf("Enable port4\n");
		addr = 0xd0;
		if(!strcmp(*(av + 3), "1.5G")) {
			addr1 = 0x98;
		}else{
			addr1 = 0xa8;
		}
		word = pci_read_config16(dev, 0xd0);
		word |= 0x01;
		pci_write_config16(dev, 0xd0, word);
	}else if (!strcmp(*(av + 1), "port5")){
		printf("Enable port5\n");
		addr = 0xd8;
		if(!strcmp(*(av + 3), "1.5G")) {
			addr1 = 0x9c;
		}else{
			addr1 = 0xaa;
		}
		word = pci_read_config16(dev, 0xd8);
		word |= 0x01;
		pci_write_config16(dev, 0xd8, word);
	}else
		printf("Please choose correct test port\n");


	/* set the tx test pattern  */
	P0TxTEST = pci_read_config16(dev, addr);
	P0TxTEST = ~(0xf << 2) & P0TxTEST;

	if(!strcmp(*(av + 2), "align")) {
		printf("TX test pattern is align.\n");
		P0TxTEST |= 0x0 << 2 ; /* align */
	}
	else if(!strcmp(*(av + 2), "D10.2")) {
		printf("TX test pattern is D10.2.\n");
		P0TxTEST |= 0x1 << 2 ; /* D10.2 */
	}
	else if(!strcmp(*(av + 2), "sync")) {
		printf("TX test pattern is sync.\n");
		P0TxTEST |= 0x2 << 2 ; /* sync */
	}
	else if(!strcmp(*(av + 2), "lbp")) {
		printf("TX test pattern is lbp.\n");
		P0TxTEST |= 0x3 << 2 ; /* lbp */
	}
	else if(!strcmp(*(av + 2), "mftp")) {
		printf("TX test pattern is mftp.\n");
		P0TxTEST |= 0x4 << 2 ; /* mftp */
	}
	else if(!strcmp(*(av + 2), "20bit")) {
		printf("TX test pattern is 20bit.\n");
		P0TxTEST |= 0x5 << 2 ; /* 20bit*/
	}
	else if(!strcmp(*(av + 2), "fferlb")) {
		printf("TX test pattern is fferlb.\n");
		P0TxTEST |= 0x6 << 2 ; /* fferlb */
	}
	else if(!strcmp(*(av + 2), "Tmode")) {
		printf("TX test pattern is Tmode.\n");
		P0TxTEST |= 0x7 << 2 ; /* Tmode */
	}
	else
		printf("Please choose correct test pattern: align/D10.2/sync/lbp/mftp/20bit/fferlb/Tmode.\n");

	P0TxTEST &= ~(0x1 << 1);

	if(!strcmp(*(av + 3), "1.5G")) {
		printf("speed is 1.5G.\n");
		P0TxTEST &= ~(0x1 << 1) ; /* 1.5G gen1 */
	}else if(!strcmp(*(av + 3), "3G")) {
		printf("speed is 3G.\n");
		P0TxTEST |= (0x1 << 1) ; /* 3G gen11 */
	}else
		printf("Please choose correct speed : 1.5G/3G\n");

	output = pci_read_config16(dev, addr1);
	output &= ~(0x1f << 0);

	if(!strcmp(*(av + 4), "1")) {
		printf("Driver Nominal Output 400mv.\n");
		output |= (0x10 << 0) ; /* 400mv */
	}else if(!strcmp(*(av + 4), "2")) {
		printf("Driver Nominal Output 450mv.\n");
		output |= (0x12 << 0) ; /* 450mv */
	}else if(!strcmp(*(av + 4), "3")) {
		printf("Driver Nominal Output 500mv.\n");
		output |= (0x14 << 0) ; /* 500mv */
	}else if(!strcmp(*(av + 4), "4")) {
		printf("Driver Nominal Output 550mv.\n");
		output |= (0x16 << 0) ; /* 550mv */
	}else if(!strcmp(*(av + 4), "5")) {
		printf("Driver Nominal Output 600mv.\n");
		output |= (0x18 << 0) ; /* 600mv */
	}else if(!strcmp(*(av + 4), "6")) {
		printf("Driver Nominal Output 650mv.\n");
		output |= (0x1a << 0) ; /* 650mv */
	}else if(!strcmp(*(av + 4), "7")) {
		printf("Driver Nominal Output 700mv.\n");
		output |= (0x1c << 0) ; /* 700mv */
	}else if(!strcmp(*(av + 4), "8")) {
		printf("Driver Nominal Output 750mv.\n");
		output |= (0x1e << 0) ; /* 750mv */
	}else
		printf("Please choose correct Nominal Output [ 1/2/3/4/5/6/7/8]\n");

	pci_write_config16(dev, addr, P0TxTEST);
	pci_write_config16(dev, addr1, output);

	printf("High frequency pattern.\n");

	return 0;
}

#if 0
int cmd_msqt_sata(int ac, char *av[])
{
	u16 word,P0TxTEST;
	u16 output;
	device_t dev = _pci_make_tag(0, 0x11, 0);
	int addr,addr1;
	/* enable sata0,sata1,sata2,sata3,sata4,sata5 phyctrl*/
	if (!strcmp(*(av + 1), "port0")){
		printf("Enable port0\n");
		addr = 0xb0;
		if(!strcmp(*(av + 3), "1.5G")) {
			addr1 = 0x88;
		}else{
			addr1 = 0xa0;
		}
		word = pci_read_config16(dev, 0xb0);
		word |= 0x01;
		pci_write_config16(dev, 0xb0, word);
	}else if (!strcmp(*(av + 1), "port1")){
		printf("Enable port1\n");
		addr = 0xb8;
		if(!strcmp(*(av + 3), "1.5G")) {
			addr1 = 0x8c;
		}else{
			addr1 = 0xa2;
		}
		word = pci_read_config16(dev, 0xb8);
		word |= 0x01;
		pci_write_config16(dev, 0xb8, word);
	}else if (!strcmp(*(av + 1), "port2")){
		printf("Enable port2\n");
		addr = 0xc0;
		if(!strcmp(*(av + 3), "1.5G")) {
			addr1 = 0x90;
		}else{
			addr1 = 0xa4;
		}
		word = pci_read_config16(dev, 0xc0);
		word |= 0x01;
		pci_write_config16(dev, 0xc0, word);
	}else if (!strcmp(*(av + 1), "port3")){
		printf("Enable port3\n");
		addr = 0xc8;
		if(!strcmp(*(av + 3), "1.5G")) {
			addr1 = 0x94;
		}else{
			addr1 = 0xa6;
		}
		word = pci_read_config16(dev, 0xc8);
		word |= 0x01;
		pci_write_config16(dev, 0xc8, word);
	}else if (!strcmp(*(av + 1), "port4")){
		printf("Enable port4\n");
		addr = 0xd0;
		if(!strcmp(*(av + 3), "1.5G")) {
			addr1 = 0x98;
		}else{
			addr1 = 0xa8;
		}
		word = pci_read_config16(dev, 0xd0);
		word |= 0x01;
		pci_write_config16(dev, 0xd0, word);
	}else if (!strcmp(*(av + 1), "port5")){
		printf("Enable port5\n");
		addr = 0xd8;
		if(!strcmp(*(av + 3), "1.5G")) {
			addr1 = 0x9c;
		}else{
			addr1 = 0xaa;
		}
		word = pci_read_config16(dev, 0xd8);
		word |= 0x01;
		pci_write_config16(dev, 0xd8, word);
	}else
		printf("Please choose correct test port\n");


	/* set the tx test pattern  */
	P0TxTEST = pci_read_config16(dev, addr);
	P0TxTEST = ~(0xf << 2) & P0TxTEST;

	if(!strcmp(*(av + 2), "align")) {
		printf("TX test pattern is align.\n");
		P0TxTEST |= 0x0 << 2 ; /* align */
	}
	else if(!strcmp(*(av + 2), "D10.2")) {
		printf("TX test pattern is D10.2.\n");
		P0TxTEST |= 0x1 << 2 ; /* D10.2 */
	}
	else if(!strcmp(*(av + 2), "sync")) {
		printf("TX test pattern is sync.\n");
		P0TxTEST |= 0x2 << 2 ; /* sync */
	}
	else if(!strcmp(*(av + 2), "lbp")) {
		printf("TX test pattern is lbp.\n");
		P0TxTEST |= 0x3 << 2 ; /* lbp */
	}
	else if(!strcmp(*(av + 2), "mftp")) {
		printf("TX test pattern is mftp.\n");
		P0TxTEST |= 0x4 << 2 ; /* mftp */
	}
	else if(!strcmp(*(av + 2), "20bit")) {
		printf("TX test pattern is 20bit.\n");
		P0TxTEST |= 0x5 << 2 ; /* 20bit*/
	}
	else if(!strcmp(*(av + 2), "fferlb")) {
		printf("TX test pattern is fferlb.\n");
		P0TxTEST |= 0x6 << 2 ; /* fferlb */
	}
	else if(!strcmp(*(av + 2), "Tmode")) {
		printf("TX test pattern is Tmode.\n");
		P0TxTEST |= 0x7 << 2 ; /* Tmode */
	}
	else
		printf("Please choose correct test pattern: align/D10.2/sync/lbp/mftp/20bit/fferlb/Tmode.\n");

	P0TxTEST &= ~(0x1 << 1);

	if(!strcmp(*(av + 3), "1.5G")) {
		printf("speed is 1.5G.\n");
		P0TxTEST &= ~(0x1 << 1) ; /* 1.5G gen1 */
	}else if(!strcmp(*(av + 3), "3G")) {
		printf("speed is 3G.\n");
		P0TxTEST |= (0x1 << 1) ; /* 3G gen11 */
	}else
		printf("Please choose correct speed : 1.5G/3G\n");

	output = pci_read_config16(dev, addr1);
	output &= ~(0x1f << 0);

	if(!strcmp(*(av + 4), "1")) {
		printf("Driver Nominal Output 400mv.\n");
		output |= (0x10 << 0) ; /* 400mv */
	}else if(!strcmp(*(av + 4), "2")) {
		printf("Driver Nominal Output 450mv.\n");
		output |= (0x12 << 0) ; /* 450mv */
	}else if(!strcmp(*(av + 4), "3")) {
		printf("Driver Nominal Output 500mv.\n");
		output |= (0x14 << 0) ; /* 500mv */
	}else if(!strcmp(*(av + 4), "4")) {
		printf("Driver Nominal Output 550mv.\n");
		output |= (0x16 << 0) ; /* 550mv */
	}else if(!strcmp(*(av + 4), "5")) {
		printf("Driver Nominal Output 600mv.\n");
		output |= (0x18 << 0) ; /* 600mv */
	}else if(!strcmp(*(av + 4), "6")) {
		printf("Driver Nominal Output 650mv.\n");
		output |= (0x1a << 0) ; /* 650mv */
	}else if(!strcmp(*(av + 4), "7")) {
		printf("Driver Nominal Output 700mv.\n");
		output |= (0x1c << 0) ; /* 700mv */
	}else if(!strcmp(*(av + 4), "8")) {
		printf("Driver Nominal Output 750mv.\n");
		output |= (0x1e << 0) ; /* 750mv */
	}else
		printf("Please choose correct Nominal Output [ 1/2/3/4/5/6/7/8]\n");

	pci_write_config16(dev, addr, P0TxTEST);
	pci_write_config16(dev, addr1, output);

	printf("High frequency pattern.\n");

	return 0;
}

#if 1
int cmd_msqt_sata1(int ac, char *av[])
{

	u16 word;
	u32 output;
	device_t dev = _pci_make_tag(0, 0x11, 0);
	int addr;
	/* enable sata0,sata1,sata2,sata3,sata4,sata5 phyctrl*/
	if (!strcmp(*(av + 1), "port0")){
		printf("Enable port0 TX pre-emphasis driver swing\n");
		if(!strcmp(*(av + 2), "1.5G")) {
			addr = 0x88;
		}else{
			addr = 0xa0;
		}
	}else if (!strcmp(*(av + 1), "port1")){
		printf("Enable port1 TX pre-emphasis driver swing\n");
		if(!strcmp(*(av + 2), "1.5G")) {
			addr = 0x8c;
		}else{
			addr = 0xa2;
		}
	}else if (!strcmp(*(av + 1), "port2")){
		printf("Enable port2 TX pre-emphasis driver swing\n");
		if(!strcmp(*(av + 2), "1.5G")) {
			addr = 0x90;
		}else{
			addr = 0xa4;
		}
	}else if (!strcmp(*(av + 1), "port3")){
		printf("Enable port3 TX pre-emphasis driver swing\n");
		if(!strcmp(*(av + 2), "1.5G")) {
			addr = 0x94;
		}else{
			addr = 0xa6;
		}
	}else if (!strcmp(*(av + 1), "port4")){
		printf("Enable port4 TX pre-emphasis driver swing\n");
		if(!strcmp(*(av + 2), "1.5G")) {
			addr = 0x98;
		}else{
			addr = 0xa8;
		}
	}else if (!strcmp(*(av + 1), "port5")){
		printf("Enable port5 TX pre-emphasis driver swing\n");
		if(!strcmp(*(av + 2), "1.5G")) {
			addr = 0x9c;
		}else{
			addr = 0xaa;
		}
	}else
		printf("Please choose correct test port\n");

	word = pci_read_config16(dev, addr);
	word |= 0x01 << 13;
	pci_write_config16(dev, addr, word);

	output = pci_read_config16(dev, addr);
	output &= ~(0x7 << 5);

	if(!strcmp(*(av + 3), "1")) {
		printf("Driver Nominal Output 0mv.\n");
		output |= (0x0 << 5) ; /* 0mv */
	}else if(!strcmp(*(av + 3), "2")) {
		printf("Driver Nominal Output 25mv.\n");
		output |= (0x1 << 5) ; /* 25mv */
	}else if(!strcmp(*(av + 3), "3")) {
		printf("Driver Nominal Output 50mv.\n");
		output |= (0x2 << 5) ; /* 50mv */
	}else if(!strcmp(*(av + 3), "4")) {
		printf("Driver Nominal Output 75mv.\n");
		output |= (0x3 << 5) ; /* 75mv */
	}else if(!strcmp(*(av + 3), "5")) {
		printf("Driver Nominal Output 100mv.\n");
		output |= (0x4 << 5) ; /* 100mv */
	}else if(!strcmp(*(av + 3), "6")) {
		printf("Driver Nominal Output 125mv.\n");
		output |= (0x5 << 5) ; /* 125mv */
	}else if(!strcmp(*(av + 3), "7")) {
		printf("Driver Nominal Output 150mv.\n");
		output |= (0x6 << 5) ; /* 150mv */
	}else if(!strcmp(*(av + 3), "8")) {
		printf("Driver Nominal Output 175mv.\n");
		output |= (0x7 << 5) ; /* 175mv */
	}else
		printf("Please choose correct Nominal Output [ 1/2/3/4/5/6/7/8]\n");

	pci_write_config16(dev, addr, output);

	return 0;

}
#endif
#endif

static const Cmd Cmds[] =
{
	{"Motherboard Signal Quality Test (msqt)"},
	{"msqt_sata",	"port[0/1/2/3/4/5] pattern[align/D10.2/sync/lbp/mftp/20bit/fferlb/Tmode] speed[ 1.5G/3G ] Nominal Output[ 1/2/3/4/5/6/7/8 ]", 0,
		"Motherboard Signal Quality Test for SATA", cmd_msqt_sata, 5, 5, 0},
	{"msqt_sata1",	"port[0/1/2/3/4/5]  speed[ 1.5G/3G ] Nominal Output[ 1/2/3/4/5/6/7/8 ]", 0,
		"Motherboard Signal Quality Test for SATA TX pre-emphasis driver swing", cmd_msqt_sata1, 4, 4, 0},
	{0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
#endif
