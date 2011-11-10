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

		
	regbase = (bus_space_handle_t)cf->ca_baseaddr;;
	if(ahci_init_one(regbase)){
		printf("ahci_init_one failed.\n");
	}
	
	linkmap = probe_ent->link_port_map;
	printf("linkmap=%x\n",linkmap);
	for (i = 0; i < 1; i++) {
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
