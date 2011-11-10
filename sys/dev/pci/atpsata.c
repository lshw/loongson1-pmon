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
#include "sata_8620.h"

/* to get some global routines like printf */
#include "etherboot.h"
/* to get the interface to the body of the program */
#define myudelay delay
static int atp_match (struct device *, void *, void *);
static void atp_attach (struct device *, struct device *, void *);

struct cfattach atp_ca = {
        sizeof(atp_sata_t),atp_match, atp_attach,
};
struct cfdriver atp_cd = {
        NULL, "atp", DV_DULL
};
#define PCI_VENDOR_SATA  0x1191
#define PCI_PRODUCT_SATA 0x000d
static int atp_match(
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

static void atp_attach(struct device * parent, struct device * self, void *aux)
{
	atp_sata_t *sc = (atp_sata_t * )self;
        struct pci_attach_args *pa = aux;
        pci_chipset_tag_t pc = pa->pa_pc;
        bus_space_tag_t iot = pa->pa_iot;
        bus_addr_t iobase;
        bus_size_t iosize;
        bus_addr_t ideaddr;
	atp_sata_info_t info;
	u32	iog_base;
	u32	ioaddr;
	int i;

        if (pci_io_find(pc, pa->pa_tag, 0x14, &iobase, &iosize))
        {
               printf(": can't find i/o space\n");
               return;
        }

        if (bus_space_map(iot, iobase, iosize, 0,  &sc->reg_base))
        {
               printf(": can't map i/o space\n");
               return;
        }

	ioaddr = sc->reg_base; 
        if (pci_io_find(pc, pa->pa_tag, 0x10, &iobase, &iosize))
        {
               printf(": can't find i/o space\n");
               return;
        }

        if (bus_space_map(iot, iobase, iosize, 0,  &ideaddr))
        {
               printf(": can't map i/o space\n");
               return;
        }

	iog_base = ioaddr + 0x100;
	outb(iog_base + 0x0004,0x01);//send COMRESET
	while(inb(iog_base + 0x0004) & 0x01)
	{
	      myudelay(10);
	}
	outb(iog_base + 0x0004,0x0);

	for(i = 0;i <= 1;i++)
	{
		info.sata_reg_base = ioaddr + i * 0x80;
		info.flags = i;
		info.aa_link.aa_type=0xff; //just for not match ide
		config_found(self,(void *)&info,NULL);
	}
 
	sc->sc_wdcdev.PIO_cap = 0;
	sc->sc_wdcdev.DMA_cap = 0;
	sc->sc_wdcdev.channels = sc->wdc_chanarray;
	sc->sc_wdcdev.nchannels = 1;
	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA32;//WDC_CAPABILITY_DATA16;
	sc->wdc_chanarray[0] = &sc->wdc_channel;
	sc->wdc_channel.channel = 0;
	sc->wdc_channel.wdc = &sc->sc_wdcdev;
	sc->wdc_channel.ch_queue =
	    malloc(sizeof(struct channel_queue), M_DEVBUF, M_NOWAIT);
	if (sc->wdc_channel.ch_queue == NULL) {
		printf("%s: "
			"cannot allocate memory for command queue",
		sc->sc_wdcdev.sc_dev.dv_xname);
		 return ;
	}
	 sc->wdc_channel.cmd_iot=iot;
	 sc->wdc_channel.ctl_iot=iot;
	 sc->wdc_channel.cmd_ioh= ideaddr+0x80;
	 sc->wdc_channel.ctl_ioh= ideaddr+0x8e;
			sc->wdc_channel.data32iot = sc->wdc_channel.cmd_iot;
			sc->wdc_channel.data32ioh = sc->wdc_channel.cmd_ioh;

	 wdcattach(sc->wdc_chanarray[0]);
}
