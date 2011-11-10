/*      $OpenBSD: pciide.c,v 1.23 2000/05/04 19:42:53 millert Exp $     */
/*	$NetBSD: pciide.c,v 1.48 1999/11/28 20:05:18 bouyer Exp $	*/

/*
 * Copyright (c) 1999 Manuel Bouyer.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * Copyright (c) 1996, 1998 Christopher G. Demetriou.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * PCI IDE controller driver.
 *
 * Author: Christopher G. Demetriou, March 2, 1998 (derived from NetBSD
 * sys/dev/pci/ppb.c, revision 1.16).
 *
 * See "PCI IDE Controller Specification, Revision 1.0 3/4/94" and
 * "Programming Interface for Bus Master IDE Controller, Revision 1.0
 * 5/16/94" from the PCI SIG.
 *
 */

#define DEBUG_DMA   0x01
#define DEBUG_XFERS  0x02
#define DEBUG_FUNCS  0x08
#define DEBUG_PROBE  0x10

#ifdef WDCDEBUG
int wdcdebug_pciide_mask = DEBUG_DMA|DEBUG_XFERS|DEBUG_FUNCS|DEBUG_PROBE;
#define WDCDEBUG_PRINT(args, level) \
	printf args
#else
#define WDCDEBUG_PRINT(args, level)
#endif
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/vm_kern.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>
#include <dev/pci/pciide_piix_reg.h>
#include <dev/pci/pciide_amd_reg.h>
#include <dev/pci/pciide_apollo_reg.h>
#ifndef PMON
#endif
#include <dev/pci/pciide_cmd_reg.h>
#ifndef PMON
#include <dev/pci/pciide_cy693_reg.h>
#include <dev/pci/pciide_sis_reg.h>
#endif
#include <dev/pci/pciide_acer_reg.h>
#include <dev/pci/pciide_pdc202xx_reg.h>
#include <dev/ata/atavar.h>
#include <dev/ic/wdcreg.h>
#include <dev/ic/wdcvar.h>

#if BYTE_ORDER == BIG_ENDIAN
#if 0
#include <machine/bswap.h> 
#endif
#define	htopci(x)	bswap32(x)
#define	pcitoh(x)	bswap32(x)
#else 
#define	htopci(x)	(x)
#define	pcitoh(x)	(x)
#endif

/* inlines for reading/writing 8-bit PCI registers */
static __inline u_int8_t pciide_pci_read __P((pci_chipset_tag_t, pcitag_t,
					      int));
static __inline void pciide_pci_write __P((pci_chipset_tag_t, pcitag_t,
					   int, u_int8_t));

static __inline u_int8_t
pciide_pci_read(pc, pa, reg)
	pci_chipset_tag_t pc;
	pcitag_t pa;
	int reg;
{

	return (pci_conf_read(pc, pa, (reg & ~0x03)) >>
	    ((reg & 0x03) * 8) & 0xff);
}

static __inline void
pciide_pci_write(pc, pa, reg, val)
	pci_chipset_tag_t pc;
	pcitag_t pa;
	int reg;
	u_int8_t val;
{
	pcireg_t pcival;

	pcival = pci_conf_read(pc, pa, (reg & ~0x03));
	pcival &= ~(0xff << ((reg & 0x03) * 8));
	pcival |= (val << ((reg & 0x03) * 8));
	pci_conf_write(pc, pa, (reg & ~0x03), pcival);
}

struct pciide_softc {
	struct wdc_softc	sc_wdcdev;	/* common wdc definitions */
	pci_chipset_tag_t	sc_pc;		/* PCI registers info */
	pcitag_t		sc_tag;
	void			*sc_pci_ih;	/* PCI interrupt handle */
	int			sc_dma_ok;	/* bus-master DMA info */
	bus_space_tag_t		sc_dma_iot;
	bus_space_handle_t	sc_dma_ioh;
	bus_dma_tag_t		sc_dmat;
	/* Chip description */
	const struct pciide_product_desc *sc_pp;
	/* common definitions */
	struct channel_softc *wdc_chanarray[PCIIDE_NUM_CHANNELS];
	/* internal bookkeeping */
	struct pciide_channel {			/* per-channel data */
		struct channel_softc wdc_channel; /* generic part */
		char		*name;
		int		hw_ok;		/* hardware mapped & OK? */
		int		compat;		/* is it compat? */
		void		*ih;		/* compat or pci handle */
		/* DMA tables and DMA map for xfer, for each drive */
		struct pciide_dma_maps {
			bus_dmamap_t    dmamap_table;
			struct idedma_table *dma_table;
			bus_dmamap_t    dmamap_xfer;
		} dma_maps[2];
	} pciide_channels[PCIIDE_NUM_CHANNELS];
};

void default_chip_map __P((struct pciide_softc*, struct pci_attach_args*));

void piix_chip_map __P((struct pciide_softc*, struct pci_attach_args*));
void piix_setup_channel __P((struct channel_softc*));
void piix3_4_setup_channel __P((struct channel_softc*));

static u_int32_t piix_setup_idetim_timings __P((u_int8_t, u_int8_t, u_int8_t));
static u_int32_t piix_setup_idetim_drvs __P((struct ata_drive_datas*));
static u_int32_t piix_setup_sidetim_timings __P((u_int8_t, u_int8_t, u_int8_t));

void amd756_chip_map __P((struct pciide_softc*, struct pci_attach_args*));
void amd756_setup_channel __P((struct channel_softc*));

void amdcs5536_chip_map __P((struct pciide_softc*, struct pci_attach_args*));
void amdcs5536_setup_channel __P((struct channel_softc*));
#ifndef PMON
void apollo_chip_map __P((struct pciide_softc*, struct pci_attach_args*));
void apollo_setup_channel __P((struct channel_softc*));
#endif

void cmd_chip_map __P((struct pciide_softc*, struct pci_attach_args*));
void cmd0643_6_chip_map __P((struct pciide_softc*, struct pci_attach_args*));
void cmd0643_6_setup_channel __P((struct channel_softc*));
void cmd_channel_map __P((struct pci_attach_args *,
			struct pciide_softc *, int));
int  cmd_pci_intr __P((void *));

void cmd680_chip_map __P((struct pciide_softc*, struct pci_attach_args*)); /*zxj:from NETBSD:20090325*/
void cmd680_setup_channel __P((struct channel_softc*));
void cmd680_channel_map __P((struct pci_attach_args *,
			struct pciide_softc *, int));

#ifndef PMON
void cy693_chip_map __P((struct pciide_softc*, struct pci_attach_args*));
void cy693_setup_channel __P((struct channel_softc*));

void sis_chip_map __P((struct pciide_softc*, struct pci_attach_args*));
void sis_setup_channel __P((struct channel_softc*));
#endif

void acer_chip_map __P((struct pciide_softc*, struct pci_attach_args*));
void acer_setup_channel __P((struct channel_softc*));
int  acer_pci_intr __P((void *));

void pdc202xx_chip_map __P((struct pciide_softc*, struct pci_attach_args*));
void pdc202xx_setup_channel __P((struct channel_softc*));
void pdc20268_setup_channel __P((struct channel_softc*));
int  pdc202xx_pci_intr __P((void *));
int  pdc20265_pci_intr __P((void *));
 
#if !defined(PMON)||defined(IDE_DMA)
void pciide_channel_dma_setup __P((struct pciide_channel *));
int  pciide_dma_table_setup __P((struct pciide_softc*, int, int));
int  pciide_dma_init __P((void*, int, int, void *, size_t, int));
void pciide_dma_start __P((void*, int, int, int));
int  pciide_dma_finish __P((void*, int, int, int));
#else
#define pciide_channel_dma_setup(cp)	/* Nothing */
#endif
void pciide_print_modes __P((struct pciide_channel *));
void pciide_print_channels __P((int, pcireg_t));;

struct pciide_product_desc {
	u_int32_t ide_product;
	u_short ide_flags;
	/* map and setup chip, probe drives */
	void (*chip_map) __P((struct pciide_softc*, struct pci_attach_args*));
};

/* Flags for ide_flags */
#define IDE_PCI_CLASS_OVERRIDE 0x0001 /* accept even if class != pciide */

/* Default product description for devices not known from this controller */
const struct pciide_product_desc default_product_desc = {
	0,				/* Generic PCI IDE controller */
	0,
	default_chip_map
};

const struct pciide_product_desc pciide_intel_products[] =  {
	{ PCI_PRODUCT_INTEL_82092AA,	/* Intel 82092AA IDE */
	  0,
	  default_chip_map
	},
	{ PCI_PRODUCT_INTEL_82371FB_IDE, /* Intel 82371FB IDE (PIIX) */
	  0,
	  piix_chip_map
	},
	{ PCI_PRODUCT_INTEL_82371SB_IDE, /* Intel 82371SB IDE (PIIX3) */
	  0,
	  piix_chip_map
	},
	{ PCI_PRODUCT_INTEL_82371AB_IDE, /* Intel 82371AB IDE (PIIX4) */
	  0,
	  piix_chip_map
	},
	{ PCI_PRODUCT_INTEL_82801AA_IDE, /* Intel 82801AA IDE (ICH) */
	  0,
	  piix_chip_map
	},
	{ PCI_PRODUCT_INTEL_82801AB_IDE, /* Intel 82801AB IDE (ICH0) */
	  0,
	  piix_chip_map
	},
};

const struct pciide_product_desc pciide_amd_products[] =  {
	{ PCI_PRODUCT_AMD_PBC756_IDE,	/* AMD 756 */
	  0,
	  amd756_chip_map
	},
	{ PCI_PRODUCT_AMD_CS5536_IDE,   /* AMD CS5536 */
	  0,
	  amdcs5536_chip_map
	},
};

const struct pciide_product_desc pciide_cmd_products[] =  {
	{ PCI_PRODUCT_CMDTECH_640,	/* CMD Technology PCI0640 */
	  0,
	  cmd_chip_map
	},
	{ PCI_PRODUCT_CMDTECH_643,	/* CMD Technology PCI0643 */
	  0,
	  cmd0643_6_chip_map
	},
	{ PCI_PRODUCT_CMDTECH_646,	/* CMD Technology PCI0646 */
	  0,
	  cmd0643_6_chip_map
	},
	{ PCI_PRODUCT_CMDTECH_648,	/* CMD Technology PCI0648 */
	  0,
	  cmd0643_6_chip_map
	},
	{ PCI_PRODUCT_CMDTECH_680,  /* SIIMAGE 680: from NETBSD: 20090325: zxj*/
	  /*IDE_PCI_CLASS_OVERRIDE*/0,
	  //cmd680_chip_map
	  default_chip_map
	}
};

#ifndef PMON
const struct pciide_product_desc pciide_via_products[] =  {
	{ PCI_PRODUCT_VIATECH_VT82C586_IDE, /* VIA VT82C586 (Apollo VP) IDE */
	  0,
	  apollo_chip_map
	 },
	{ PCI_PRODUCT_VIATECH_VT82C586A_IDE, /* VIA VT82C586A IDE */
	  0,
	  apollo_chip_map
	}
};

const struct pciide_product_desc pciide_cypress_products[] =  {
	{ PCI_PRODUCT_CONTAQ_82C693,	/* Contaq CY82C693 IDE */
	  0,
	  cy693_chip_map
	}
};

const struct pciide_product_desc pciide_sis_products[] =  {
	{ PCI_PRODUCT_SIS_5513,		/* SIS 5513 EIDE */
	  0,
	  sis_chip_map
	}
};
#endif

const struct pciide_product_desc pciide_acer_products[] =  {
	{ PCI_PRODUCT_ALI_M5229,	/* Acer Labs M5229 UDMA IDE */
	  0,
	  acer_chip_map
	}
};

const struct pciide_product_desc pciide_promise_products[] =  {
	{ PCI_PRODUCT_PROMISE_PDC20246,
	  IDE_PCI_CLASS_OVERRIDE,
	  pdc202xx_chip_map,
	},
	{ PCI_PRODUCT_PROMISE_PDC20262,
	  IDE_PCI_CLASS_OVERRIDE,
	  pdc202xx_chip_map,
	},
	{ PCI_PRODUCT_PROMISE_PDC20265,
	  IDE_PCI_CLASS_OVERRIDE,
	  pdc202xx_chip_map,
	},
	{ PCI_PRODUCT_PROMISE_PDC20267,
	  IDE_PCI_CLASS_OVERRIDE,
	  pdc202xx_chip_map,
	},
	{ PCI_PRODUCT_PROMISE_PDC20268,
	  IDE_PCI_CLASS_OVERRIDE,
	  default_chip_map,
	},
	{ PCI_PRODUCT_PROMISE_PDC20268R,
	  IDE_PCI_CLASS_OVERRIDE,
	  default_chip_map,
	},
	{ PCI_PRODUCT_PROMISE_PDC20269,
	  IDE_PCI_CLASS_OVERRIDE,
	  pdc202xx_chip_map,
	}
};

struct pciide_vendor_desc {
	u_int32_t ide_vendor;
	const struct pciide_product_desc *ide_products;
	int ide_nproducts;
};

const struct pciide_vendor_desc pciide_vendors[] = {
	{ PCI_VENDOR_INTEL, pciide_intel_products,
	  sizeof(pciide_intel_products)/sizeof(pciide_intel_products[0]) },
	{ PCI_VENDOR_AMD, pciide_amd_products,
	  sizeof(pciide_amd_products)/sizeof(pciide_amd_products[0]) },
	{ PCI_VENDOR_CMDTECH, pciide_cmd_products,
	  sizeof(pciide_cmd_products)/sizeof(pciide_cmd_products[0]) },
#ifndef PMON
	{ PCI_VENDOR_VIATECH, pciide_via_products,
	  sizeof(pciide_via_products)/sizeof(pciide_via_products[0]) },
	{ PCI_VENDOR_CONTAQ, pciide_cypress_products,
	  sizeof(pciide_cypress_products)/sizeof(pciide_cypress_products[0]) },
	{ PCI_VENDOR_SIS, pciide_sis_products,
	  sizeof(pciide_sis_products)/sizeof(pciide_sis_products[0]) },
#endif
	{ PCI_VENDOR_ALI, pciide_acer_products,
	  sizeof(pciide_acer_products)/sizeof(pciide_acer_products[0]) },
	{ PCI_VENDOR_PROMISE, pciide_promise_products,
	  sizeof(pciide_promise_products)/sizeof(pciide_promise_products[0]) }
};

#define	PCIIDE_CHANNEL_NAME(chan)	((chan) == 0 ? "ch 0" : "ch 1")

/* options passed via the 'flags' config keyword */
#define PCIIDE_OPTIONS_DMA	0x01

#ifndef __OpenBSD__
int	pciide_match __P((struct device *, struct cfdata *, void *));
#else
int	pciide_match __P((struct device *, void *, void *));
#endif
void	pciide_attach __P((struct device *, struct device *, void *));

struct cfattach pciide_ca = {
	sizeof(struct pciide_softc), pciide_match, pciide_attach
};

#ifdef __OpenBSD__
struct        cfdriver pciide_cd = {
      NULL, "pciide", DV_DULL
};
#endif
int	pciide_chipen __P((struct pciide_softc *, struct pci_attach_args *));
int	pciide_mapregs_compat __P(( struct pci_attach_args *,
	    struct pciide_channel *, int, bus_size_t *, bus_size_t*));
int	pciide_mapregs_native __P((struct pci_attach_args *, 
	    struct pciide_channel *, bus_size_t *, bus_size_t *,
	    int (*pci_intr) __P((void *))));
void	pciide_mapreg_dma __P((struct pciide_softc *,
	    struct pci_attach_args *));
int	pciide_chansetup __P((struct pciide_softc *, int, pcireg_t));
void	pciide_mapchan __P((struct pci_attach_args *,
	    struct pciide_channel *, pcireg_t, bus_size_t *, bus_size_t *,
	    int (*pci_intr) __P((void *))));
int	pciiide_chan_candisable __P((struct pciide_channel *));
void	pciide_map_compat_intr __P(( struct pci_attach_args *,
	    struct pciide_channel *, int, int));
int	pciide_print __P((void *, const char *pnp));
int	pciide_compat_intr __P((void *));
int	pciide_pci_intr __P((void *));
const struct pciide_product_desc* pciide_lookup_product __P((u_int32_t));

const struct pciide_product_desc *
pciide_lookup_product(id)
	u_int32_t id;
{
	const struct pciide_product_desc *pp;
	const struct pciide_vendor_desc *vp;
	int i;

	for (i = 0, vp = pciide_vendors;
	    i < sizeof(pciide_vendors)/sizeof(pciide_vendors[0]);
	    vp++, i++)
		if (PCI_VENDOR(id) == vp->ide_vendor)
			break;

	if (i == sizeof(pciide_vendors)/sizeof(pciide_vendors[0]))
		return NULL;

	for (pp = vp->ide_products, i = 0; i < vp->ide_nproducts; pp++, i++)
		if (PCI_PRODUCT(id) == pp->ide_product)
			break;

	if (i == vp->ide_nproducts)
		return NULL;
	return pp;
}

int
pciide_match(parent, match, aux)
	struct device *parent;
#ifdef __OpenBSD__
	void *match;
#else
	struct cfdata *match;
#endif
	void *aux;
{
	struct pci_attach_args *pa = aux;
	const struct pciide_product_desc *pp;
	int bus, device, function;
	
	_pci_break_tag(pa->pa_tag, &bus, &device, &function);
	
	/*
	 * Check the ID register to see that it's a PCI IDE controller.
	 * If it is, we assume that we can deal with it; it _should_
	 * work in a standardized way...
	 */
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_MASS_STORAGE &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_MASS_STORAGE_IDE) {
		return (1);
	}

	/*
 	 * Some controllers (e.g. promise Ultra-33) don't claim to be PCI IDE
	 * controllers. Let see if we can deal with it anyway.
	 */
	pp = pciide_lookup_product(pa->pa_id);
	if (pp  && (pp->ide_flags & IDE_PCI_CLASS_OVERRIDE)) {
		return (1);
	}

	return (0);
}

#include <sys/dev/pci/pcireg.h>
void
pciide_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pci_attach_args *pa = aux;
#ifndef PMON
	pci_chipset_tag_t pc = pa->pa_pc;
#endif
	pcitag_t tag = pa->pa_tag;
	struct pciide_softc *sc = (struct pciide_softc *)self;
	pcireg_t csr;
	char devinfo[256];

	sc->sc_pp = pciide_lookup_product(pa->pa_id);
	if (sc->sc_pp == NULL) {
		sc->sc_pp = &default_product_desc;
		pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	}

	sc->sc_pc = pa->pa_pc;
	sc->sc_tag = pa->pa_tag;


#ifdef WDCDEBUG
       if (wdcdebug_pciide_mask & DEBUG_PROBE)
               printf("sc_pc %x, sc_tag %x\n", sc->sc_pc, sc->sc_tag);
#endif

	sc->sc_pp->chip_map(sc, pa);

	if (sc->sc_dma_ok) {
		csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
		csr |= PCI_COMMAND_MASTER_ENABLE;
		pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, csr);
	}

	WDCDEBUG_PRINT(("pciide: command/status register=%x\n",
	    pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG)), DEBUG_PROBE);
}

/* tell wether the chip is enabled or not */
int
pciide_chipen(sc, pa)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
{
	pcireg_t csr;
	if ((pa->pa_flags & PCI_FLAGS_IO_ENABLED) == 0) {
		csr = pci_conf_read(sc->sc_pc, sc->sc_tag,
		PCI_COMMAND_STATUS_REG);
	printf(": device disabled (at %s)\n",
		(csr & PCI_COMMAND_IO_ENABLE) == 0 ?
		"device" : "bridge");
		return 0;
	}
	return 1;
}


int
pciide_mapregs_compat(pa, cp, compatchan, cmdsizep, ctlsizep)
	struct pci_attach_args *pa;
	struct pciide_channel *cp;
	int compatchan;
	bus_size_t *cmdsizep, *ctlsizep;
{
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.wdc;
	struct channel_softc *wdc_cp = &cp->wdc_channel;

	cp->compat = 1;
	*cmdsizep = PCIIDE_COMPAT_CMD_SIZE;
	*ctlsizep = PCIIDE_COMPAT_CTL_SIZE;

	wdc_cp->cmd_iot = pa->pa_iot;

	if (bus_space_map(wdc_cp->cmd_iot, PCIIDE_COMPAT_CMD_BASE(compatchan),
	    PCIIDE_COMPAT_CMD_SIZE, 0, &wdc_cp->cmd_ioh) != 0) {
		printf("%s: couldn't map %s cmd regs\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
		return (0);
	}

	wdc_cp->ctl_iot = pa->pa_iot;

	if (bus_space_map(wdc_cp->ctl_iot, PCIIDE_COMPAT_CTL_BASE(compatchan),
	    PCIIDE_COMPAT_CTL_SIZE, 0, &wdc_cp->ctl_ioh) != 0) {
		printf("%s: couldn't map %s ctl regs\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
		bus_space_unmap(wdc_cp->cmd_iot, wdc_cp->cmd_ioh,
		    PCIIDE_COMPAT_CMD_SIZE);
		return (0);
	}

	return (1);
}

int
pciide_mapregs_native(pa, cp, cmdsizep, ctlsizep, pci_intr)
	struct pci_attach_args * pa;
	struct pciide_channel *cp;
	bus_size_t *cmdsizep, *ctlsizep;
	int (*pci_intr) __P((void *));
{
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.wdc;
	struct channel_softc *wdc_cp = &cp->wdc_channel;
	const char *intrstr;
	pci_intr_handle_t intrhandle;

	cp->compat = 0;

	if (sc->sc_pci_ih == NULL) {
		if (pci_intr_map(pa->pa_pc, pa->pa_intrtag, pa->pa_intrpin,
		    pa->pa_intrline, &intrhandle) != 0) {
			printf("%s: couldn't map native-PCI interrupt\n",
			    sc->sc_wdcdev.sc_dev.dv_xname);
			return 0;
		}	
		intrstr = pci_intr_string(pa->pa_pc, intrhandle);
#ifdef __OpenBSD__
		sc->sc_pci_ih = pci_intr_establish(pa->pa_pc,
		    intrhandle, IPL_BIO, pci_intr, sc,
		    sc->sc_wdcdev.sc_dev.dv_xname);
#else
		sc->sc_pci_ih = pci_intr_establish(pa->pa_pc,
		    intrhandle, IPL_BIO, pci_intr, sc);
#endif
		if (sc->sc_pci_ih != NULL) {
			printf("%s: using %s for native-PCI interrupt\n",
			    sc->sc_wdcdev.sc_dev.dv_xname,
			    intrstr ? intrstr : "unknown interrupt");
		} else {
			printf("%s: couldn't establish native-PCI interrupt",
			    sc->sc_wdcdev.sc_dev.dv_xname);
			if (intrstr != NULL)
				printf(" at %s", intrstr);
			printf("\n");
			return 0;
		}
	}
	cp->ih = sc->sc_pci_ih;
	if (pci_mapreg_map(pa, PCIIDE_REG_CMD_BASE(wdc_cp->channel),
	    PCI_MAPREG_TYPE_IO, 0,
	    &wdc_cp->cmd_iot, &wdc_cp->cmd_ioh, NULL, cmdsizep, 0) != 0) {
		printf("%s: couldn't map %s cmd regs\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
		return 0;
	}

	if (pci_mapreg_map(pa, PCIIDE_REG_CTL_BASE(wdc_cp->channel),
	    PCI_MAPREG_TYPE_IO, 0,
	    &wdc_cp->ctl_iot, &wdc_cp->ctl_ioh, NULL, ctlsizep, 0) != 0) {
		printf("%s: couldn't map %s ctl regs\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
		bus_space_unmap(wdc_cp->cmd_iot, wdc_cp->cmd_ioh, *cmdsizep);
		return 0;
	}
	return (1);
}

void
pciide_mapreg_dma(sc, pa)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
{
#if !defined(PMON)||defined(IDE_DMA)
	/*
	 * Map DMA registers
	 *
	 * Note that sc_dma_ok is the right variable to test to see if
	 * DMA can be done.  If the interface doesn't support DMA,
	 * sc_dma_ok will never be non-zero.  If the DMA regs couldn't
	 * be mapped, it'll be zero.  I.e., sc_dma_ok will only be
	 * non-zero if the interface supports DMA and the registers
	 * could be mapped.
	 *
	 * XXX Note that despite the fact that the Bus Master IDE specs
	 * XXX say that "The bus master IDE function uses 16 bytes of IO
	 * XXX space," some controllers (at least the United
	 * XXX Microelectronics UM8886BF) place it in memory space.
	 * XXX eventually, we should probably read the register and check
	 * XXX which type it is.  Either that or 'quirk' certain devices.
	 */

	sc->sc_dma_ok = (pci_mapreg_map(pa,
	    PCIIDE_REG_BUS_MASTER_DMA, PCI_MAPREG_TYPE_IO, 0,
	    &sc->sc_dma_iot, &sc->sc_dma_ioh, NULL, NULL,0) == 0);
	sc->sc_dmat = pa->pa_dmat;
	if (sc->sc_dma_ok == 0) {
		printf(", (unuseable)"); /* couldn't map registers */
	} else {
		sc->sc_wdcdev.dma_arg = sc;
		sc->sc_wdcdev.dma_init = pciide_dma_init;
		sc->sc_wdcdev.dma_start = pciide_dma_start;
		sc->sc_wdcdev.dma_finish = pciide_dma_finish;
	}
#else
	sc->sc_dma_ok = 0;
#endif
}

int
pciide_compat_intr(arg)
	void *arg;
{
	struct pciide_channel *cp = arg;

#ifdef DIAGNOSTIC
	/* should only be called for a compat channel */
	if (cp->compat == 0)
		panic("pciide compat intr called for non-compat chan %p\n", cp);
#endif
	return (wdcintr(&cp->wdc_channel));
}

int
pciide_pci_intr(arg)
	void *arg;
{
	struct pciide_softc *sc = arg;
	struct pciide_channel *cp;
	struct channel_softc *wdc_cp;
	int i, rv, crv;

	rv = 0;
	for (i = 0; i < sc->sc_wdcdev.nchannels; i++) {
		cp = &sc->pciide_channels[i];
		wdc_cp = &cp->wdc_channel;

		/* If a compat channel skip. */
		if (cp->compat)
			continue;
		/* if this channel not waiting for intr, skip */
		if ((wdc_cp->ch_flags & WDCF_IRQ_WAIT) == 0)
			continue;

		crv = wdcintr(wdc_cp);
		if (crv == 0)
			;		/* leave rv alone */
		else if (crv == 1)
			rv = 1;		/* claim the intr */
		else if (rv == 0)	/* crv should be -1 in this case */
			rv = crv;	/* if we've done no better, take it */
	}
	return (rv);
}

#if !defined(PMON)||defined(IDE_DMA)
void
pciide_channel_dma_setup(cp)
	struct pciide_channel *cp;
{
	int drive;
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.wdc;
	struct ata_drive_datas *drvp;

	for (drive = 0; drive < 2; drive++) {
		drvp = &cp->wdc_channel.ch_drive[drive];
		/* If no drive, skip */
		if ((drvp->drive_flags & DRIVE) == 0)
			continue;
		/* setup DMA if needed */
		if (((drvp->drive_flags & DRIVE_DMA) == 0 &&
		    (drvp->drive_flags & DRIVE_UDMA) == 0) ||
		    sc->sc_dma_ok == 0) {
			drvp->drive_flags &= ~(DRIVE_DMA | DRIVE_UDMA);
			continue;
		}
		if (pciide_dma_table_setup(sc, cp->wdc_channel.channel, drive)
		    != 0) {
			/* Abort DMA setup */
			drvp->drive_flags &= ~(DRIVE_DMA | DRIVE_UDMA);
			continue;
		}
	}
}

int
pciide_dma_table_setup(sc, channel, drive)
	struct pciide_softc *sc;
	int channel, drive;
{
	bus_dma_segment_t seg;
	int error, rseg;
	const bus_size_t dma_table_size =
	    sizeof(struct idedma_table) * NIDEDMA_TABLES;
	struct pciide_dma_maps *dma_maps =
	    &sc->pciide_channels[channel].dma_maps[drive];

	/* If table was already allocated, just return */
	if (dma_maps->dma_table)
		return 0;

	/* Allocate memory for the DMA tables and map it */
	if ((error = bus_dmamem_alloc(sc->sc_dmat, dma_table_size,
	    IDEDMA_TBL_ALIGN, IDEDMA_TBL_ALIGN, &seg, 1, &rseg,
	    BUS_DMA_NOWAIT)) != 0) {
		printf("%s:%d: unable to allocate table DMA for "
		    "drive %d, error=%d\n", sc->sc_wdcdev.sc_dev.dv_xname,
		    channel, drive, error);
		return error;
	}

	if ((error = bus_dmamem_map(sc->sc_dmat, &seg, rseg,
	    dma_table_size,
	    (caddr_t *)&dma_maps->dma_table,
	    BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) != 0) {
		printf("%s:%d: unable to map table DMA for"
		    "drive %d, error=%d\n", sc->sc_wdcdev.sc_dev.dv_xname,
		    channel, drive, error);
		return error;
	}

	WDCDEBUG_PRINT(("pciide_dma_table_setup: table at %p len %ld, "
	    "phy 0x%lx\n", dma_maps->dma_table, dma_table_size,
	    seg.ds_addr), DEBUG_PROBE);

	/* Create and load table DMA map for this disk */
	if ((error = bus_dmamap_create(sc->sc_dmat, dma_table_size,
	    1, dma_table_size, IDEDMA_TBL_ALIGN, BUS_DMA_NOWAIT,
	    &dma_maps->dmamap_table)) != 0) {
		printf("%s:%d: unable to create table DMA map for "
		    "drive %d, error=%d\n", sc->sc_wdcdev.sc_dev.dv_xname,
		    channel, drive, error);
		return error;
	}
	if ((error = bus_dmamap_load(sc->sc_dmat,
	    dma_maps->dmamap_table,
	    dma_maps->dma_table,
	    dma_table_size, NULL, BUS_DMA_NOWAIT)) != 0) {
		printf("%s:%d: unable to load table DMA map for "
		    "drive %d, error=%d\n", sc->sc_wdcdev.sc_dev.dv_xname,
		    channel, drive, error);
		return error;
	}
	WDCDEBUG_PRINT(("pciide_dma_table_setup: phy addr of table 0x%lx\n",
	    dma_maps->dmamap_table->dm_segs[0].ds_addr), DEBUG_PROBE);
	/* Create a xfer DMA map for this drive */
	if ((error = bus_dmamap_create(sc->sc_dmat, IDEDMA_BYTE_COUNT_MAX,
	    NIDEDMA_TABLES, IDEDMA_BYTE_COUNT_MAX, IDEDMA_BYTE_COUNT_ALIGN,
	    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
	    &dma_maps->dmamap_xfer)) != 0) {
		printf("%s:%d: unable to create xfer DMA map for "
		    "drive %d, error=%d\n", sc->sc_wdcdev.sc_dev.dv_xname,
		    channel, drive, error);
		return error;
	}
	return 0;
}

int
pciide_dma_init(v, channel, drive, databuf, datalen, flags)
	void *v;
	int channel, drive;
	void *databuf;
	size_t datalen;
	int flags;
{
	struct pciide_softc *sc = v;
	int error, seg;
	struct pciide_dma_maps *dma_maps =
	    &sc->pciide_channels[channel].dma_maps[drive];

	error = bus_dmamap_load(sc->sc_dmat,
	    dma_maps->dmamap_xfer,
	    databuf, datalen, NULL, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s:%d: unable to load xfer DMA map for"
		    "drive %d, error=%d\n", sc->sc_wdcdev.sc_dev.dv_xname,
		    channel, drive, error);
		return error;
	}
#if !defined(__OpenBSD__)||defined(IDE_DMA)
	bus_dmamap_sync(sc->sc_dmat, dma_maps->dmamap_xfer,
	    0,
	    dma_maps->dmamap_xfer->dm_mapsize,		
	    (flags & WDC_DMA_READ) ?
	    BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);
#else
	bus_dmamap_sync(sc->sc_dmat, dma_maps->dmamap_xfer,
	    (flags & WDC_DMA_READ) ?
	    BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);
#endif
	for (seg = 0; seg < dma_maps->dmamap_xfer->dm_nsegs; seg++) {
#ifdef DIAGNOSTIC
		/* A segment must not cross a 64k boundary */
		{
		u_long phys = dma_maps->dmamap_xfer->dm_segs[seg].ds_addr;
		u_long len = dma_maps->dmamap_xfer->dm_segs[seg].ds_len;
		if ((phys & ~IDEDMA_BYTE_COUNT_MASK) !=
		    ((phys + len - 1) & ~IDEDMA_BYTE_COUNT_MASK)) {
			printf("pciide_dma: segment %d physical addr 0x%lx"
			    " len 0x%lx not properly aligned\n",
			    seg, phys, len);
			panic("pciide_dma: buf align");
		}
		}
#endif
		dma_maps->dma_table[seg].base_addr =
		    htopci(dma_maps->dmamap_xfer->dm_segs[seg].ds_addr | 0x00000000);
		dma_maps->dma_table[seg].byte_count =
		    htopci(dma_maps->dmamap_xfer->dm_segs[seg].ds_len &
		    IDEDMA_BYTE_COUNT_MASK);
		WDCDEBUG_PRINT(("\t seg %d len %d addr 0x%x\n",
		   seg, pcitoh(dma_maps->dma_table[seg].byte_count),
		   pcitoh(dma_maps->dma_table[seg].base_addr)), DEBUG_DMA);

	}
	dma_maps->dma_table[dma_maps->dmamap_xfer->dm_nsegs -1].byte_count |=
	    htopci(IDEDMA_BYTE_COUNT_EOT);

#if !defined(__OpenBSD__) || defined(IDE_DMA)
	bus_dmamap_sync(sc->sc_dmat, dma_maps->dmamap_table, 
	    0,
	    dma_maps->dmamap_table->dm_mapsize,
	    BUS_DMASYNC_PREWRITE);
#else
	bus_dmamap_sync(sc->sc_dmat, dma_maps->dmamap_table, 
	    BUS_DMASYNC_PREWRITE);
#endif

	/* Maps are ready. Start DMA function */
#ifdef DIAGNOSTIC
	if (dma_maps->dmamap_table->dm_segs[0].ds_addr & ~IDEDMA_TBL_MASK) {
		printf("pciide_dma_init: addr 0x%lx not properly aligned\n",
		    dma_maps->dmamap_table->dm_segs[0].ds_addr);
		panic("pciide_dma_init: table align");
	}
#endif

	/* Clear status bits */
	bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
	    IDEDMA_CTL + IDEDMA_SCH_OFFSET * channel,
	    bus_space_read_1(sc->sc_dma_iot, sc->sc_dma_ioh,
		IDEDMA_CTL + IDEDMA_SCH_OFFSET * channel));
/* XXX CMD MRDMODE pending bits clear */
	bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
	    (IDEDMA_CTL-1) + IDEDMA_SCH_OFFSET * channel,
	    bus_space_read_1(sc->sc_dma_iot, sc->sc_dma_ioh,
		(IDEDMA_CTL-1) + IDEDMA_SCH_OFFSET * channel));
	/* Write table addr */
//printf("dma table start at 0x%08x\n", dma_maps->dmamap_table->dm_segs[0].ds_addr);
	bus_space_write_4(sc->sc_dma_iot, sc->sc_dma_ioh,
	    IDEDMA_TBL + IDEDMA_SCH_OFFSET * channel,
	    dma_maps->dmamap_table->dm_segs[0].ds_addr/* | 0x80000000*/); /* XXX */
	/* set read/write */
	bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
	    IDEDMA_CMD + IDEDMA_SCH_OFFSET * channel,
	    (flags & WDC_DMA_READ) ? IDEDMA_CMD_WRITE: 0);
	return 0;
}

void
pciide_dma_start(v, channel, drive, flags)
	void *v;
	int channel, drive, flags;
{
	struct pciide_softc *sc = v;

	WDCDEBUG_PRINT(("pciide_dma_start\n"),DEBUG_XFERS);
	bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
	    IDEDMA_CMD + IDEDMA_SCH_OFFSET * channel,
	    bus_space_read_1(sc->sc_dma_iot, sc->sc_dma_ioh,
		IDEDMA_CMD + IDEDMA_SCH_OFFSET * channel) | IDEDMA_CMD_START);
}

int
pciide_dma_finish(v, channel, drive, flags)
	void *v;
	int channel, drive;
	int flags;
{
	struct pciide_softc *sc = v;
	u_int8_t status;
	struct pciide_dma_maps *dma_maps =
	    &sc->pciide_channels[channel].dma_maps[drive];

	/* Unload the map of the data buffer */
#if !defined(__OpenBSD__)||defined(IDE_DMA)
	bus_dmamap_sync(sc->sc_dmat, dma_maps->dmamap_xfer, 
	    0,
	    dma_maps->dmamap_xfer->dm_mapsize,
	    (flags & WDC_DMA_READ) ?
	    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
#else
	bus_dmamap_sync(sc->sc_dmat, dma_maps->dmamap_xfer, 
	    (flags & WDC_DMA_READ) ?
	    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
#endif

	bus_dmamap_unload(sc->sc_dmat, dma_maps->dmamap_xfer);

	status = bus_space_read_1(sc->sc_dma_iot, sc->sc_dma_ioh,
	    IDEDMA_CTL + IDEDMA_SCH_OFFSET * channel);
	WDCDEBUG_PRINT(("pciide_dma_finish: status 0x%x\n", status),
	    DEBUG_XFERS);
	WDCDEBUG_PRINT(("pciide: command/status register=%x\n",
	    pci_conf_read(sc->sc_pc, sc->sc_tag, PCI_COMMAND_STATUS_REG)), DEBUG_XFERS);

	/* stop DMA channel */
	bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
	    IDEDMA_CMD + IDEDMA_SCH_OFFSET * channel,
	    bus_space_read_1(sc->sc_dma_iot, sc->sc_dma_ioh,
		IDEDMA_CMD + IDEDMA_SCH_OFFSET * channel) & ~IDEDMA_CMD_START);

	/* Clear status bits */
	bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
	    IDEDMA_CTL + IDEDMA_SCH_OFFSET * channel,
	    status);

	if ((status & IDEDMA_CTL_ERR) != 0) {
		printf("%s:%d:%d: Bus-Master DMA error: status=0x%x\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, channel, drive, status);
		return -1;
	}

	if ((flags & WDC_DMA_POLL) == 0 && (status & IDEDMA_CTL_INTR) == 0) {
		printf("%s:%d:%d: Bus-Master DMA error: missing interrupt, "
		    "status=0x%x\n", sc->sc_wdcdev.sc_dev.dv_xname, channel,
		    drive, status);
		return -1;
	}

	if ((status & IDEDMA_CTL_ACT) != 0) {
		/* data underrun, may be a valid condition for ATAPI */
		return 1;
	}
	return 0;
}
#endif

/* some common code used by several chip_map */
int
pciide_chansetup(sc, channel, interface)
	struct pciide_softc *sc;
	int channel;
	pcireg_t interface;
{
	struct pciide_channel *cp = &sc->pciide_channels[channel];
	sc->wdc_chanarray[channel] = &cp->wdc_channel;
	cp->name = PCIIDE_CHANNEL_NAME(channel);
	cp->wdc_channel.channel = channel;
	cp->wdc_channel.wdc = &sc->sc_wdcdev;
	cp->wdc_channel.ch_queue =
	    malloc(sizeof(struct channel_queue), M_DEVBUF, M_NOWAIT);
	if (cp->wdc_channel.ch_queue == NULL) {
		printf("%s: %s "
			"cannot allocate memory for command queue",
		sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
		 return 0;
	}

	cp->hw_ok = 1;

	return 1;

}


/* some common code used by several chip channel_map */
void
pciide_mapchan(pa, cp, interface, cmdsizep, ctlsizep, pci_intr)
	struct pci_attach_args *pa;
	struct pciide_channel *cp;
	pcireg_t interface;
	bus_size_t *cmdsizep, *ctlsizep;
	int (*pci_intr) __P((void *));
{
	struct channel_softc *wdc_cp = &cp->wdc_channel;

	if (interface & PCIIDE_INTERFACE_PCI(wdc_cp->channel))
		cp->hw_ok = pciide_mapregs_native(pa, cp, cmdsizep, ctlsizep,
		    pci_intr);
	else
		cp->hw_ok = pciide_mapregs_compat(pa, cp,
		    wdc_cp->channel, cmdsizep, ctlsizep);
	if (cp->hw_ok == 0)
		return;
	wdc_cp->data32iot = wdc_cp->cmd_iot;
	wdc_cp->data32ioh = wdc_cp->cmd_ioh;
	wdcattach(wdc_cp);
}

/*
 * Generic code to call to know if a channel can be disabled. Return 1
 * if channel can be disabled, 0 if not
 */
int
pciiide_chan_candisable(cp)
	struct pciide_channel *cp;
{
	struct channel_softc *wdc_cp = &cp->wdc_channel;

	if ((wdc_cp->ch_drive[0].drive_flags & DRIVE) == 0 &&
	    (wdc_cp->ch_drive[1].drive_flags & DRIVE) == 0) {
		cp->hw_ok = 0;
		return 1;
	}
	return 0;
}

/*
 * generic code to map the compat intr if hw_ok=1 and it is a compat channel.
 * Set hw_ok=0 on failure
 */
#define	pciide_machdep_compat_intr_establish(a, b, c, d, e)	tgt_poll_register((c), (d), (e))
void
pciide_map_compat_intr(pa, cp, compatchan, interface)
	struct pci_attach_args *pa;
	struct pciide_channel *cp;
	int compatchan, interface;
{
#if !defined(PMON)||defined(IDE_DMA)
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.wdc;
	struct channel_softc *wdc_cp = &cp->wdc_channel;

	if (cp->hw_ok == 0)
		return;
	if ((interface & PCIIDE_INTERFACE_PCI(wdc_cp->channel)) != 0)
		return;

	cp->ih = pciide_machdep_compat_intr_establish(&sc->sc_wdcdev.sc_dev,
	    pa, compatchan, pciide_compat_intr, cp);
	if (cp->ih == NULL) {
		printf("%s: no compatibility interrupt for use by %s\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
		cp->hw_ok = 0;
	}
#endif
}

void
pciide_print_channels(nchannels, interface)
	int nchannels;
	pcireg_t interface;
{
	int i;

	for (i = 0; i < nchannels; i++) {
		printf(", %s %s to %s", PCIIDE_CHANNEL_NAME(i),
		    (interface & PCIIDE_INTERFACE_SETTABLE(i)) ?   
		    "cfg" : "wired",
		    (interface & PCIIDE_INTERFACE_PCI(i)) ? "native-PCI" :
		    "compat");
		    }

	printf("\n");
}

void
pciide_print_modes(cp)
	struct pciide_channel *cp;
{
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.wdc;
	int drive;
	struct channel_softc *chp;
	struct ata_drive_datas *drvp;

	chp = &cp->wdc_channel;
	for (drive = 0; drive < 2; drive++) {
		drvp = &chp->ch_drive[drive];
		if ((drvp->drive_flags & DRIVE) == 0)
			continue;
		printf("%s(%s:%d:%d): using PIO mode %d",
		    drvp->drive_name,
		    sc->sc_wdcdev.sc_dev.dv_xname,
		    chp->channel, drive, drvp->PIO_mode);
		if (drvp->drive_flags & DRIVE_DMA)
			printf(", DMA mode %d", drvp->DMA_mode);
		if (drvp->drive_flags & DRIVE_UDMA)
			printf(", Ultra-DMA mode %d", drvp->UDMA_mode);
		if (drvp->drive_flags & (DRIVE_DMA | DRIVE_UDMA))
			printf(" (using DMA data transfers)");
		printf("\n");
	}
}

void
default_chip_map(sc, pa)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
{
	struct pciide_channel *cp;
	pcireg_t interface = PCI_INTERFACE(pci_conf_read(sc->sc_pc,
				    sc->sc_tag, PCI_CLASS_REG));
	pcireg_t csr;
	int channel;
#if !defined(PMON)||defined(IDE_DMA)
	int drive;
	struct ata_drive_datas *drvp;
	u_int8_t idedma_ctl;
#endif
	bus_size_t cmdsize, ctlsize;
	char *failreason;

	if (pciide_chipen(sc, pa) == 0)
		return;

	if (interface & PCIIDE_INTERFACE_BUS_MASTER_DMA) {
		printf(": DMA");
#ifndef IDE_DMA
		if (sc->sc_pp == &default_product_desc &&
		    (sc->sc_wdcdev.sc_dev.dv_cfdata->cf_flags &
		    PCIIDE_OPTIONS_DMA) == 0) {
			printf(" (unsupported)");
			sc->sc_dma_ok = 0;
		} else 
#endif
		{
			pciide_mapreg_dma(sc, pa);
		if (sc->sc_dma_ok != 0)
			printf(", (partial support)");
		}
	} else {
		printf(": no DMA");
		sc->sc_dma_ok = 0;
	}
	if (sc->sc_dma_ok)
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_DMA;
	sc->sc_wdcdev.PIO_cap = 0;
	sc->sc_wdcdev.DMA_cap = 0;
	sc->sc_wdcdev.channels = sc->wdc_chanarray;
	sc->sc_wdcdev.nchannels = PCIIDE_NUM_CHANNELS;
	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16;

	pciide_print_channels(sc->sc_wdcdev.nchannels, interface);

	for (channel = 0; channel < sc->sc_wdcdev.nchannels; channel++) {
		cp = &sc->pciide_channels[channel];
		if (pciide_chansetup(sc, channel, interface) == 0)
		    continue;
		if (interface & PCIIDE_INTERFACE_PCI(channel)) {
			cp->hw_ok = pciide_mapregs_native(pa, cp, &cmdsize,
			    &ctlsize, pciide_pci_intr);
		} else {
			cp->hw_ok = pciide_mapregs_compat(pa, cp,
			    channel, &cmdsize, &ctlsize);
		}
		if (cp->hw_ok == 0)
			continue;
		/*
		 * Check to see if something appears to be there.
		 */
		failreason = NULL;
		if (!wdcprobe(&cp->wdc_channel)) {
			failreason = "not responding; disabled or no drives?";
			goto next;
		}
		/*
		 * Now, make sure it's actually attributable to this PCI IDE
		 * channel by trying to access the channel again while the
		 * PCI IDE controller's I/O space is disabled.  (If the
		 * channel no longer appears to be there, it belongs to
		 * this controller.)  YUCK!
		 */
		csr = pci_conf_read(sc->sc_pc, sc->sc_tag,
	  	    PCI_COMMAND_STATUS_REG);
		pci_conf_write(sc->sc_pc, sc->sc_tag, PCI_COMMAND_STATUS_REG,
		    csr & ~PCI_COMMAND_IO_ENABLE);
		if (wdcprobe(&cp->wdc_channel))
			failreason = "other hardware responding at addresses";
#ifdef FOR_GXEMUL
		failreason=0;
#endif
		pci_conf_write(sc->sc_pc, sc->sc_tag,
		    PCI_COMMAND_STATUS_REG, csr);
next:
		if (failreason) {
			printf("%s: %s ignored (%s)\n",
			    sc->sc_wdcdev.sc_dev.dv_xname, cp->name,
			    failreason);
			cp->hw_ok = 0;
			bus_space_unmap(cp->wdc_channel.cmd_iot,
			    cp->wdc_channel.cmd_ioh, cmdsize);
			bus_space_unmap(cp->wdc_channel.ctl_iot,
			    cp->wdc_channel.ctl_ioh, ctlsize);
		} else {
			pciide_map_compat_intr(pa, cp, channel, interface);
		}
		if (cp->hw_ok) {
			cp->wdc_channel.data32iot = cp->wdc_channel.cmd_iot;
			cp->wdc_channel.data32ioh = cp->wdc_channel.cmd_ioh;
			wdcattach(&cp->wdc_channel);
		}
	}

#if !defined(PMON)||defined(IDE_DMA)
	if (sc->sc_dma_ok == 0)
		return;

	/* Allocate DMA maps */
	for (channel = 0; channel < sc->sc_wdcdev.nchannels; channel++) {
		idedma_ctl = 0;
		cp = &sc->pciide_channels[channel];
		for (drive = 0; drive < 2; drive++) {
			drvp = &cp->wdc_channel.ch_drive[drive];
			/* If no drive, skip */
			if ((drvp->drive_flags & DRIVE) == 0)
				continue;
			if ((drvp->drive_flags & DRIVE_DMA) == 0)
				continue;
			if (pciide_dma_table_setup(sc, channel, drive) != 0) {
				/* Abort DMA setup */
				printf("%s:%d:%d: cannot allocate DMA maps, "
				    "using PIO transfers\n",
				    sc->sc_wdcdev.sc_dev.dv_xname,
				    channel, drive);
				drvp->drive_flags &= ~DRIVE_DMA;
			}
			printf("%s:%d:%d: using DMA data transfers\n",
			    sc->sc_wdcdev.sc_dev.dv_xname,
			    channel, drive);
			idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
		}
		if (idedma_ctl != 0) {
			/* Add software bits in status register */
			bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
			    IDEDMA_CTL + (IDEDMA_SCH_OFFSET * channel),
			    idedma_ctl);
		}
	}
#endif

}

void
piix_chip_map(sc, pa)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
{
	struct pciide_channel *cp;
	int channel;
	u_int32_t idetim;
	bus_size_t cmdsize, ctlsize;

	pcireg_t interface = PCI_INTERFACE(pci_conf_read(sc->sc_pc,
					    sc->sc_tag, PCI_CLASS_REG)); 

	if (pciide_chipen(sc, pa) == 0)
		return;

	printf(": DMA");
	pciide_mapreg_dma(sc, pa);
	if (sc->sc_dma_ok) {
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_DMA;
		switch(sc->sc_pp->ide_product) {
		case PCI_PRODUCT_INTEL_82371AB_IDE:
		case PCI_PRODUCT_INTEL_82801AA_IDE:
		case PCI_PRODUCT_INTEL_82801AB_IDE:
			sc->sc_wdcdev.cap |= WDC_CAPABILITY_UDMA;
		}
	}

	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16 | WDC_CAPABILITY_DATA32 |
	    WDC_CAPABILITY_MODE;
	sc->sc_wdcdev.PIO_cap = 4;
	sc->sc_wdcdev.DMA_cap = 2;
	sc->sc_wdcdev.UDMA_cap =
	    (sc->sc_pp->ide_product == PCI_PRODUCT_INTEL_82801AA_IDE) ? 4 : 2;
	if (sc->sc_pp->ide_product == PCI_PRODUCT_INTEL_82371FB_IDE)
		sc->sc_wdcdev.set_modes = piix_setup_channel;
	else
		sc->sc_wdcdev.set_modes = piix3_4_setup_channel;
	sc->sc_wdcdev.channels = sc->wdc_chanarray;
	sc->sc_wdcdev.nchannels = PCIIDE_NUM_CHANNELS;

	pciide_print_channels(sc->sc_wdcdev.nchannels, interface);

	WDCDEBUG_PRINT(("piix_setup_chip: old idetim=0x%x",
	    pci_conf_read(sc->sc_pc, sc->sc_tag, PIIX_IDETIM)),
	    DEBUG_PROBE);
	if (sc->sc_pp->ide_product != PCI_PRODUCT_INTEL_82371FB_IDE) {
		WDCDEBUG_PRINT((", sidetim=0x%x",
		    pci_conf_read(sc->sc_pc, sc->sc_tag, PIIX_SIDETIM)),
		    DEBUG_PROBE);
		if (sc->sc_wdcdev.cap & WDC_CAPABILITY_UDMA) {
			WDCDEBUG_PRINT((", udamreg 0x%x",
			    pci_conf_read(sc->sc_pc, sc->sc_tag, PIIX_UDMAREG)),
			    DEBUG_PROBE);
		}
		if (sc->sc_pp->ide_product == PCI_PRODUCT_INTEL_82801AA_IDE ||
		    sc->sc_pp->ide_product == PCI_PRODUCT_INTEL_82801AB_IDE) {
			WDCDEBUG_PRINT((", IDE_CONTROL 0x%x",
			    pci_conf_read(sc->sc_pc, sc->sc_tag, PIIX_CONFIG)),
			    DEBUG_PROBE);
		}

	}
	WDCDEBUG_PRINT(("\n"), DEBUG_PROBE);

#ifdef PMON        
        pci_conf_write(sc->sc_pc, sc->sc_tag, PIIX_IDETIM,
                       0x80008000);
#endif /* PMON */        

	for (channel = 0; channel < sc->sc_wdcdev.nchannels; channel++) {
		cp = &sc->pciide_channels[channel];
		/* PIIX is compat-only */
		if (pciide_chansetup(sc, channel, 0) == 0)
			continue;
		idetim = pci_conf_read(sc->sc_pc, sc->sc_tag, PIIX_IDETIM);
		if ((PIIX_IDETIM_READ(idetim, channel) &
		    PIIX_IDETIM_IDE) == 0) {
			printf("%s: %s ignored (disabled)\n",
			    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
			continue;
		}
		/* PIIX are compat-only pciide devices */
		pciide_mapchan(pa, cp, 0, &cmdsize, &ctlsize, pciide_pci_intr);
		if (cp->hw_ok == 0)
			continue;
		if (pciiide_chan_candisable(cp)) {
			idetim = PIIX_IDETIM_CLEAR(idetim, PIIX_IDETIM_IDE,
			    channel);
			pci_conf_write(sc->sc_pc, sc->sc_tag, PIIX_IDETIM,
			    idetim);
		}
		pciide_map_compat_intr(pa, cp, channel, 0);
		if (cp->hw_ok == 0)
			continue;
		sc->sc_wdcdev.set_modes(&cp->wdc_channel);
	}

	WDCDEBUG_PRINT(("piix_setup_chip: idetim=0x%x",
	    pci_conf_read(sc->sc_pc, sc->sc_tag, PIIX_IDETIM)),
	    DEBUG_PROBE);
	if (sc->sc_pp->ide_product != PCI_PRODUCT_INTEL_82371FB_IDE) {
		WDCDEBUG_PRINT((", sidetim=0x%x",
		    pci_conf_read(sc->sc_pc, sc->sc_tag, PIIX_SIDETIM)),
		    DEBUG_PROBE);
		if (sc->sc_wdcdev.cap & WDC_CAPABILITY_UDMA) {
			WDCDEBUG_PRINT((", udamreg 0x%x",
			    pci_conf_read(sc->sc_pc, sc->sc_tag, PIIX_UDMAREG)),
			        DEBUG_PROBE);
		}
		if (sc->sc_pp->ide_product == PCI_PRODUCT_INTEL_82801AA_IDE ||
		    sc->sc_pp->ide_product == PCI_PRODUCT_INTEL_82801AB_IDE) {
			WDCDEBUG_PRINT((", IDE_CONTROL 0x%x",
			    pci_conf_read(sc->sc_pc, sc->sc_tag, PIIX_CONFIG)),
			DEBUG_PROBE);
		}
	}
	WDCDEBUG_PRINT(("\n"), DEBUG_PROBE);
}

void
piix_setup_channel(chp)
	struct channel_softc *chp;
{
	u_int8_t mode[2], drive;
	u_int32_t oidetim, idetim, idedma_ctl;
	struct pciide_channel *cp = (struct pciide_channel*)chp;
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.wdc;
	struct ata_drive_datas *drvp = cp->wdc_channel.ch_drive;
 
	oidetim = pci_conf_read(sc->sc_pc, sc->sc_tag, PIIX_IDETIM);
	idetim = PIIX_IDETIM_CLEAR(oidetim, 0xffff, chp->channel);
	idedma_ctl = 0;

	/* set up new idetim: Enable IDE registers decode */
	idetim = PIIX_IDETIM_SET(idetim, PIIX_IDETIM_IDE,
	    chp->channel);

	/* setup DMA */
	pciide_channel_dma_setup(cp);

	/*
	 * Here we have to mess up with drives mode: PIIX can't have
	 * different timings for master and slave drives.
	 * We need to find the best combination.
	 */

	/* If both drives supports DMA, take the lower mode */
	if ((drvp[0].drive_flags & DRIVE_DMA) &&
	    (drvp[1].drive_flags & DRIVE_DMA)) {
		mode[0] = mode[1] =
		    min(drvp[0].DMA_mode, drvp[1].DMA_mode);
		    drvp[0].DMA_mode = mode[0];
		    drvp[1].DMA_mode = mode[1];
		goto ok;
	}
	/*
	 * If only one drive supports DMA, use its mode, and
	 * put the other one in PIO mode 0 if mode not compatible
	 */
	if (drvp[0].drive_flags & DRIVE_DMA) {
		mode[0] = drvp[0].DMA_mode;
		mode[1] = drvp[1].PIO_mode;
		if (piix_isp_pio[mode[1]] != piix_isp_dma[mode[0]] ||
		    piix_rtc_pio[mode[1]] != piix_rtc_dma[mode[0]])
			mode[1] = drvp[1].PIO_mode = 0;
		goto ok;
	}
	if (drvp[1].drive_flags & DRIVE_DMA) {
		mode[1] = drvp[1].DMA_mode;
		mode[0] = drvp[0].PIO_mode;
		if (piix_isp_pio[mode[0]] != piix_isp_dma[mode[1]] ||
		    piix_rtc_pio[mode[0]] != piix_rtc_dma[mode[1]])
			mode[0] = drvp[0].PIO_mode = 0;
		goto ok;
	}
	/*
	 * If both drives are not DMA, takes the lower mode, unless
	 * one of them is PIO mode < 2
	 */
	if (drvp[0].PIO_mode < 2) {
		mode[0] = drvp[0].PIO_mode = 0;
		mode[1] = drvp[1].PIO_mode;
	} else if (drvp[1].PIO_mode < 2) {
		mode[1] = drvp[1].PIO_mode = 0;
		mode[0] = drvp[0].PIO_mode;
	} else {
		mode[0] = mode[1] =
		    min(drvp[1].PIO_mode, drvp[0].PIO_mode);
		drvp[0].PIO_mode = mode[0];
		drvp[1].PIO_mode = mode[1];
	}
ok:	/* The modes are setup */
	for (drive = 0; drive < 2; drive++) {
		if (drvp[drive].drive_flags & DRIVE_DMA) {
			idetim |= piix_setup_idetim_timings(
			    mode[drive], 1, chp->channel);
			goto end;
		}
	}
	/* If we are there, none of the drives are DMA */
	if (mode[0] >= 2)
		idetim |= piix_setup_idetim_timings(
		    mode[0], 0, chp->channel);
	else 
		idetim |= piix_setup_idetim_timings(
		    mode[1], 0, chp->channel);
end:	/*
	 * timing mode is now set up in the controller. Enable
	 * it per-drive
	 */
	for (drive = 0; drive < 2; drive++) {
		/* If no drive, skip */
		if ((drvp[drive].drive_flags & DRIVE) == 0)
			continue;
		idetim |= piix_setup_idetim_drvs(&drvp[drive]);
		if (drvp[drive].drive_flags & DRIVE_DMA)
			idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
	}
	if (idedma_ctl != 0) {
		/* Add software bits in status register */
		bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
		    IDEDMA_CTL + (IDEDMA_SCH_OFFSET * chp->channel),
		    idedma_ctl);
	}
	pci_conf_write(sc->sc_pc, sc->sc_tag, PIIX_IDETIM, idetim);
	pciide_print_modes(cp);
}

void
piix3_4_setup_channel(chp)
	struct channel_softc *chp;
{
	struct ata_drive_datas *drvp;
	u_int32_t oidetim, idetim, sidetim, udmareg, ideconf, idedma_ctl;
	struct pciide_channel *cp = (struct pciide_channel*)chp;
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.wdc;
	int drive;
	int channel = chp->channel;

	oidetim = pci_conf_read(sc->sc_pc, sc->sc_tag, PIIX_IDETIM);
	sidetim = pci_conf_read(sc->sc_pc, sc->sc_tag, PIIX_SIDETIM);
	udmareg = pci_conf_read(sc->sc_pc, sc->sc_tag, PIIX_UDMAREG);
	ideconf = pci_conf_read(sc->sc_pc, sc->sc_tag, PIIX_CONFIG);
	idetim = PIIX_IDETIM_CLEAR(oidetim, 0xffff, channel);
	sidetim &= ~(PIIX_SIDETIM_ISP_MASK(channel) |
	    PIIX_SIDETIM_RTC_MASK(channel));

	idedma_ctl = 0;
	/* If channel disabled, no need to go further */
	if ((PIIX_IDETIM_READ(oidetim, channel) & PIIX_IDETIM_IDE) == 0)
		return;
	/* set up new idetim: Enable IDE registers decode */
	idetim = PIIX_IDETIM_SET(idetim, PIIX_IDETIM_IDE, channel);

	/* setup DMA if needed */
	pciide_channel_dma_setup(cp);

	for (drive = 0; drive < 2; drive++) {
		udmareg &= ~(PIIX_UDMACTL_DRV_EN(channel, drive) |
		    PIIX_UDMATIM_SET(0x3, channel, drive));
		drvp = &chp->ch_drive[drive];
		/* If no drive, skip */
		if ((drvp->drive_flags & DRIVE) == 0)
			continue;
		if (((drvp->drive_flags & DRIVE_DMA) == 0 &&
		    (drvp->drive_flags & DRIVE_UDMA) == 0))
			goto pio;

		if (sc->sc_pp->ide_product == PCI_PRODUCT_INTEL_82801AA_IDE ||
		    sc->sc_pp->ide_product == PCI_PRODUCT_INTEL_82801AB_IDE) {
		    ideconf |= PIIX_CONFIG_PINGPONG;
		}
		if (sc->sc_pp->ide_product == PCI_PRODUCT_INTEL_82801AA_IDE) {
			/* setup Ultra/66 */
			if (drvp->UDMA_mode > 2 &&
			    (ideconf & PIIX_CONFIG_CR(channel, drive)) == 0)
				drvp->UDMA_mode = 2;
			if (drvp->UDMA_mode > 2)
				ideconf |= PIIX_CONFIG_UDMA66(channel, drive);
			else
				ideconf &= ~PIIX_CONFIG_UDMA66(channel, drive);
		}

		if ((chp->wdc->cap & WDC_CAPABILITY_UDMA) &&
		    (drvp->drive_flags & DRIVE_UDMA)) {
			/* use Ultra/DMA */
			drvp->drive_flags &= ~DRIVE_DMA;
			udmareg |= PIIX_UDMACTL_DRV_EN( channel,drive);
			udmareg |= PIIX_UDMATIM_SET(
			    piix4_sct_udma[drvp->UDMA_mode], channel, drive);
		} else {
			/* use Multiword DMA */
			drvp->drive_flags &= ~DRIVE_UDMA;
			if (drive == 0) {
				idetim |= piix_setup_idetim_timings(
				    drvp->DMA_mode, 1, channel);
			} else {
				sidetim |= piix_setup_sidetim_timings(
					drvp->DMA_mode, 1, channel);
				idetim =PIIX_IDETIM_SET(idetim,
				    PIIX_IDETIM_SITRE, channel);
			}
		}
		idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
	
pio:		/* use PIO mode */
		idetim |= piix_setup_idetim_drvs(drvp);
		if (drive == 0) {
			idetim |= piix_setup_idetim_timings(
			    drvp->PIO_mode, 0, channel);
		} else {
			sidetim |= piix_setup_sidetim_timings(
				drvp->PIO_mode, 0, channel);
			idetim =PIIX_IDETIM_SET(idetim,
			    PIIX_IDETIM_SITRE, channel);
		}
	}
	if (idedma_ctl != 0) {
		/* Add software bits in status register */
		bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
		    IDEDMA_CTL + (IDEDMA_SCH_OFFSET * channel),
		    idedma_ctl);
	}
	pci_conf_write(sc->sc_pc, sc->sc_tag, PIIX_IDETIM, idetim);
	pci_conf_write(sc->sc_pc, sc->sc_tag, PIIX_SIDETIM, sidetim);
	pci_conf_write(sc->sc_pc, sc->sc_tag, PIIX_UDMAREG, udmareg);
	pci_conf_write(sc->sc_pc, sc->sc_tag, PIIX_CONFIG, ideconf);
	pciide_print_modes(cp);
}


/* setup ISP and RTC fields, based on mode */
static u_int32_t
piix_setup_idetim_timings(mode, dma, channel)
	u_int8_t mode;
	u_int8_t dma;
	u_int8_t channel;
{
	
	if (dma)
		return PIIX_IDETIM_SET(0,
		    PIIX_IDETIM_ISP_SET(piix_isp_dma[mode]) | 
		    PIIX_IDETIM_RTC_SET(piix_rtc_dma[mode]),
		    channel);
	else 
		return PIIX_IDETIM_SET(0,
		    PIIX_IDETIM_ISP_SET(piix_isp_pio[mode]) | 
		    PIIX_IDETIM_RTC_SET(piix_rtc_pio[mode]),
		    channel);
}

/* setup DTE, PPE, IE and TIME field based on PIO mode */
static u_int32_t
piix_setup_idetim_drvs(drvp)
	struct ata_drive_datas *drvp;
{
	u_int32_t ret = 0;
	struct channel_softc *chp = drvp->chnl_softc;
	u_int8_t channel = chp->channel;
	u_int8_t drive = drvp->drive;

	/*
	 * If drive is using UDMA, timings setups are independant
	 * So just check DMA and PIO here.
	 */
	if (drvp->drive_flags & DRIVE_DMA) {
		/* if mode = DMA mode 0, use compatible timings */
		if ((drvp->drive_flags & DRIVE_DMA) &&
		    drvp->DMA_mode == 0) {
			drvp->PIO_mode = 0;
			return ret;
		}
		ret = PIIX_IDETIM_SET(ret, PIIX_IDETIM_TIME(drive), channel);
		/*
		 * PIO and DMA timings are the same, use fast timings for PIO
		 * too, else use compat timings.
		 */
		if ((piix_isp_pio[drvp->PIO_mode] !=
		    piix_isp_dma[drvp->DMA_mode]) ||
		    (piix_rtc_pio[drvp->PIO_mode] !=
		    piix_rtc_dma[drvp->DMA_mode]))
			drvp->PIO_mode = 0;
		/* if PIO mode <= 2, use compat timings for PIO */
		if (drvp->PIO_mode <= 2) {
			ret = PIIX_IDETIM_SET(ret, PIIX_IDETIM_DTE(drive),
			    channel);
			return ret;
		}
	}

	/*
	 * Now setup PIO modes. If mode < 2, use compat timings.
	 * Else enable fast timings. Enable IORDY and prefetch/post
	 * if PIO mode >= 3.
	 */

	if (drvp->PIO_mode < 2)
		return ret;

	ret = PIIX_IDETIM_SET(ret, PIIX_IDETIM_TIME(drive), channel);
	if (drvp->PIO_mode >= 3) {
		ret = PIIX_IDETIM_SET(ret, PIIX_IDETIM_IE(drive), channel);
		ret = PIIX_IDETIM_SET(ret, PIIX_IDETIM_PPE(drive), channel);
	}
	return ret;
}

/* setup values in SIDETIM registers, based on mode */
static u_int32_t
piix_setup_sidetim_timings(mode, dma, channel)
	u_int8_t mode;
	u_int8_t dma;
	u_int8_t channel;
{
	if (dma)
		return PIIX_SIDETIM_ISP_SET(piix_isp_dma[mode], channel) |
		    PIIX_SIDETIM_RTC_SET(piix_rtc_dma[mode], channel);
	else 
		return PIIX_SIDETIM_ISP_SET(piix_isp_pio[mode], channel) |
		    PIIX_SIDETIM_RTC_SET(piix_rtc_pio[mode], channel);
}

void
amd756_chip_map(sc, pa)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
{
	struct pciide_channel *cp;
	pcireg_t interface = PCI_INTERFACE(pci_conf_read(sc->sc_pc,
				    sc->sc_tag, PCI_CLASS_REG));
	int channel;
	pcireg_t chanenable;
	bus_size_t cmdsize, ctlsize;

	if (pciide_chipen(sc, pa) == 0)
		return;

	printf(": DMA");
	pciide_mapreg_dma(sc, pa);

	if (sc->sc_dma_ok)
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_DMA | WDC_CAPABILITY_UDMA;
	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16 | WDC_CAPABILITY_DATA32 |
			     WDC_CAPABILITY_MODE;
	sc->sc_wdcdev.PIO_cap = 4;
	sc->sc_wdcdev.DMA_cap = 2;
	sc->sc_wdcdev.UDMA_cap = 4;
	sc->sc_wdcdev.set_modes = amd756_setup_channel;
	sc->sc_wdcdev.channels = sc->wdc_chanarray;
	sc->sc_wdcdev.nchannels = PCIIDE_NUM_CHANNELS;
	chanenable = pci_conf_read(sc->sc_pc, sc->sc_tag, AMD756_CHANSTATUS_EN);

	pciide_print_channels(sc->sc_wdcdev.nchannels, interface);

	for (channel = 0; channel < sc->sc_wdcdev.nchannels; channel++) {
		cp = &sc->pciide_channels[channel];
		if (pciide_chansetup(sc, channel, interface) == 0)
			continue;

		if ((chanenable & AMD756_CHAN_EN(channel)) == 0) {
			printf("%s: %s ignored (disabled)\n",
			    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
			continue;
		}
		pciide_mapchan(pa, cp, interface, &cmdsize, &ctlsize,
		    pciide_pci_intr);

		if (pciiide_chan_candisable(cp))
			chanenable &= ~AMD756_CHAN_EN(channel);
		pciide_map_compat_intr(pa, cp, channel, interface);
		if (cp->hw_ok == 0)
			continue;

		amd756_setup_channel(&cp->wdc_channel);
	}
	pci_conf_write(sc->sc_pc, sc->sc_tag, AMD756_CHANSTATUS_EN,
	    chanenable);
	return;
}

static unsigned int amd_ide_clock;

void
amdcs5536_chip_map (sc, pa)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
{
	struct pciide_channel *cp;
	//pcireg_t interface = PCI_INTERFACE(pci_conf_read(sc->sc_pc,
	//			    sc->sc_tag, PCI_CLASS_REG));
	pcireg_t interface = 0x00;
	int channel;
	bus_size_t cmdsize, ctlsize;

	if (pciide_chipen(sc, pa) == 0)
		return;

	amd_ide_clock = 33333; /*Assume 33M bus clock*/

	printf(": DMA");
	pciide_mapreg_dma(sc, pa);

	if (sc->sc_dma_ok)
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_DMA | WDC_CAPABILITY_UDMA;
	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16 | WDC_CAPABILITY_DATA32 |
			     WDC_CAPABILITY_MODE;
	sc->sc_wdcdev.PIO_cap = 4;
	sc->sc_wdcdev.DMA_cap = 0; /*FIXME Yanhua */
	sc->sc_wdcdev.UDMA_cap = 0;
	sc->sc_wdcdev.set_modes = amdcs5536_setup_channel;
	sc->sc_wdcdev.channels = sc->wdc_chanarray;
	sc->sc_wdcdev.nchannels = PCIIDE_NUM_CHANNELS;
	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16;

	pciide_print_channels(sc->sc_wdcdev.nchannels, interface);

	for (channel = 0; channel < sc->sc_wdcdev.nchannels; channel++) {
		cp = &sc->pciide_channels[channel];
		if (pciide_chansetup(sc, channel, interface) == 0)
			continue;
		/* Maybe we need to detect the chanel is enabled. Yanhua */

		/* Really initialization work begin here */
		pciide_mapchan(pa, cp, interface, &cmdsize, &ctlsize,
		    pciide_pci_intr);

		pciide_map_compat_intr(pa, cp, channel, interface);
		if (cp->hw_ok == 0)
			continue;

		amdcs5536_setup_channel(&cp->wdc_channel);
	}
	return;
}

void amdcs5536_setup_channel(chp) 
		struct channel_softc* chp;
{
	u_int32_t drive_reg, cast_reg;
	int mode, drive;
	struct ata_drive_datas *drvp;
	struct pciide_channel *cp = (struct pciide_channel*)chp;
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.wdc;
	
	cast_reg = pci_conf_read(sc->sc_pc, sc->sc_tag, AMDCS5536_ADDRESS_SETUP);
	drive_reg = pci_conf_read(sc->sc_pc, sc->sc_tag, AMDCS5536_DRIVE_TIMING);
	cast_reg &= 0x00ffff0f;
	drive_reg &= 0x0000ffff;

	for (drive = 0; drive < 1; drive++) { 
		drvp = &chp->ch_drive[drive];
		drvp->PIO_mode &= 0x7;
		if(drvp->PIO_mode >4)
			drvp->PIO_mode = 4;

		mode = drvp->PIO_mode;
		cast_reg |= (amdcs5536_pio_set[mode].cyc << 28) | 
				(amdcs5536_pio_set[mode].cyc <<24 ) |
				(amdcs5536_pio_set[mode].setup <<6) |
				(amdcs5536_pio_set[mode].setup <<4) ;

		drive_reg |= (amdcs5536_pio_set[mode].data <<24) |
				(amdcs5536_pio_set[mode].data <<16); 
	}
	pci_conf_write(sc->sc_pc, sc->sc_tag, AMDCS5536_ADDRESS_SETUP, cast_reg);
	pci_conf_write(sc->sc_pc, sc->sc_tag, AMDCS5536_DRIVE_TIMING, drive_reg);
}
	  
void
amd756_setup_channel(chp)
	struct channel_softc *chp;
{
	u_int32_t udmatim_reg, datatim_reg;
	u_int8_t idedma_ctl;
	int mode, drive;
	struct ata_drive_datas *drvp;
	struct pciide_channel *cp = (struct pciide_channel*)chp;
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.wdc;

	idedma_ctl = 0;
	datatim_reg = pci_conf_read(sc->sc_pc, sc->sc_tag, AMD756_DATATIM);
	udmatim_reg = pci_conf_read(sc->sc_pc, sc->sc_tag, AMD756_UDMA);
	datatim_reg &= ~AMD756_DATATIM_MASK(chp->channel);
	udmatim_reg &= ~AMD756_UDMA_MASK(chp->channel);

	/* setup DMA if needed */
	pciide_channel_dma_setup(cp);

	for (drive = 0; drive < 2; drive++) {
		drvp = &chp->ch_drive[drive];
		/* If no drive, skip */
		if ((drvp->drive_flags & DRIVE) == 0)
			continue;
		/* add timing values, setup DMA if needed */
		if (((drvp->drive_flags & DRIVE_DMA) == 0 &&
		    (drvp->drive_flags & DRIVE_UDMA) == 0)) {
			mode = drvp->PIO_mode;
			goto pio;
		}
		if ((chp->wdc->cap & WDC_CAPABILITY_UDMA) &&
		    (drvp->drive_flags & DRIVE_UDMA)) {
			/* use Ultra/DMA */
			drvp->drive_flags &= ~DRIVE_DMA;
			udmatim_reg |= AMD756_UDMA_EN(chp->channel, drive) |
			    AMD756_UDMA_EN_MTH(chp->channel, drive) |
			    AMD756_UDMA_TIME(chp->channel, drive,
				amd756_udma_tim[drvp->UDMA_mode]);
			/* can use PIO timings, MW DMA unused */
			mode = drvp->PIO_mode;
		} else {
			/* use Multiword DMA */
			drvp->drive_flags &= ~DRIVE_UDMA;
			/* mode = min(pio, dma+2) */
			if (drvp->PIO_mode <= (drvp->DMA_mode +2))
				mode = drvp->PIO_mode;
			else
				mode = drvp->DMA_mode + 2;
		}
		idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);

pio:		/* setup PIO mode */
		if (mode <= 2) {
			drvp->DMA_mode = 0;
			drvp->PIO_mode = 0;
			mode = 0;
		} else {
			drvp->PIO_mode = mode;
			drvp->DMA_mode = mode - 2;
		}
		datatim_reg |=
		    AMD756_DATATIM_PULSE(chp->channel, drive,
			amd756_pio_set[mode]) |
		    AMD756_DATATIM_RECOV(chp->channel, drive,
			amd756_pio_rec[mode]);
	}
	if (idedma_ctl != 0) {
		/* Add software bits in status register */
		bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
		    IDEDMA_CTL + (IDEDMA_SCH_OFFSET * chp->channel),
		    idedma_ctl);
	}
	pciide_print_modes(cp);
	pci_conf_write(sc->sc_pc, sc->sc_tag, AMD756_DATATIM, datatim_reg);
	pci_conf_write(sc->sc_pc, sc->sc_tag, AMD756_UDMA, udmatim_reg);
}

#ifndef PMON
void
apollo_chip_map(sc, pa)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
{
	struct pciide_channel *cp;
	pcireg_t interface = PCI_INTERFACE(pci_conf_read(sc->sc_pc,
				    sc->sc_tag, PCI_CLASS_REG));
	int channel;
	u_int32_t ideconf;
	bus_size_t cmdsize, ctlsize;

	if (pciide_chipen(sc, pa) == 0)
		return;
	printf(": DMA");
	pciide_mapreg_dma(sc, pa);
	if (sc->sc_dma_ok) {
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_DMA;
		if (sc->sc_pp->ide_product == PCI_PRODUCT_VIATECH_VT82C586A_IDE)
			sc->sc_wdcdev.cap |= WDC_CAPABILITY_UDMA;
	}
	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA32 | WDC_CAPABILITY_MODE;
	sc->sc_wdcdev.PIO_cap = 4;
	sc->sc_wdcdev.DMA_cap = 2;
	sc->sc_wdcdev.UDMA_cap = 2;
	sc->sc_wdcdev.set_modes = apollo_setup_channel;
	sc->sc_wdcdev.channels = sc->wdc_chanarray;
	sc->sc_wdcdev.nchannels = PCIIDE_NUM_CHANNELS;
	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16;

	pciide_print_channels(sc->sc_wdcdev.nchannels, interface);
	
	WDCDEBUG_PRINT(("apollo_chip_map: old APO_IDECONF=0x%x, "
	    "APO_CTLMISC=0x%x, APO_DATATIM=0x%x, APO_UDMA=0x%x\n",
	    pci_conf_read(sc->sc_pc, sc->sc_tag, APO_IDECONF),
	    pci_conf_read(sc->sc_pc, sc->sc_tag, APO_CTLMISC),
	    pci_conf_read(sc->sc_pc, sc->sc_tag, APO_DATATIM),
	    pci_conf_read(sc->sc_pc, sc->sc_tag, APO_UDMA)),
	    DEBUG_PROBE);

	for (channel = 0; channel < sc->sc_wdcdev.nchannels; channel++) {
		cp = &sc->pciide_channels[channel];
		if (pciide_chansetup(sc, channel, interface) == 0)
			continue;

		ideconf = pci_conf_read(sc->sc_pc, sc->sc_tag, APO_IDECONF);
		if ((ideconf & APO_IDECONF_EN(channel)) == 0) {
			printf("%s: %s ignored (disabled)\n",
			    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
			continue;
		}
		pciide_mapchan(pa, cp, interface, &cmdsize, &ctlsize,
		    pciide_pci_intr);
		if (cp->hw_ok == 0)
			continue;
		if (pciiide_chan_candisable(cp)) {
			ideconf &= ~APO_IDECONF_EN(channel);
			pci_conf_write(sc->sc_pc, sc->sc_tag, APO_IDECONF,
				    ideconf);
		}
		pciide_map_compat_intr(pa, cp, channel, interface);

		if (cp->hw_ok == 0)
			continue;
		apollo_setup_channel(&sc->pciide_channels[channel].wdc_channel);
	}
	WDCDEBUG_PRINT(("apollo_chip_map: APO_DATATIM=0x%x, APO_UDMA=0x%x\n",
	    pci_conf_read(sc->sc_pc, sc->sc_tag, APO_DATATIM),
	    pci_conf_read(sc->sc_pc, sc->sc_tag, APO_UDMA)), DEBUG_PROBE);
}

void
apollo_setup_channel(chp)
	struct channel_softc *chp;
{
	u_int32_t udmatim_reg, datatim_reg;
	u_int8_t idedma_ctl;
	int mode, drive;
	struct ata_drive_datas *drvp;
	struct pciide_channel *cp = (struct pciide_channel*)chp;
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.wdc;

	idedma_ctl = 0;
	datatim_reg = pci_conf_read(sc->sc_pc, sc->sc_tag, APO_DATATIM);
	udmatim_reg = pci_conf_read(sc->sc_pc, sc->sc_tag, APO_UDMA);
	datatim_reg &= ~APO_DATATIM_MASK(chp->channel);
	udmatim_reg &= ~AP0_UDMA_MASK(chp->channel);

	/* setup DMA if needed */
	pciide_channel_dma_setup(cp);

	for (drive = 0; drive < 2; drive++) {
		drvp = &chp->ch_drive[drive];
		/* If no drive, skip */
		if ((drvp->drive_flags & DRIVE) == 0)
			continue;
		/* add timing values, setup DMA if needed */
		if (((drvp->drive_flags & DRIVE_DMA) == 0 &&
		    (drvp->drive_flags & DRIVE_UDMA) == 0)) {
			mode = drvp->PIO_mode;
			goto pio;
		}
		if ((chp->wdc->cap & WDC_CAPABILITY_UDMA) &&
		    (drvp->drive_flags & DRIVE_UDMA)) {
			/* use Ultra/DMA */
			drvp->drive_flags &= ~DRIVE_DMA;
			udmatim_reg |= APO_UDMA_EN(chp->channel, drive) |
			    APO_UDMA_EN_MTH(chp->channel, drive) |
			    APO_UDMA_TIME(chp->channel, drive,
				apollo_udma_tim[drvp->UDMA_mode]);
			/* can use PIO timings, MW DMA unused */
			mode = drvp->PIO_mode;
		} else {
			/* use Multiword DMA */
			drvp->drive_flags &= ~DRIVE_UDMA;
			/* mode = min(pio, dma+2) */
			if (drvp->PIO_mode <= (drvp->DMA_mode +2))
				mode = drvp->PIO_mode;
			else
				mode = drvp->DMA_mode + 2;
		}
		idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);

pio:		/* setup PIO mode */
		if (mode <= 2) {
			drvp->DMA_mode = 0;
			drvp->PIO_mode = 0;
			mode = 0;
		} else {
			drvp->PIO_mode = mode;
			drvp->DMA_mode = mode - 2;
		}
		datatim_reg |=
		    APO_DATATIM_PULSE(chp->channel, drive,
			apollo_pio_set[mode]) |
		    APO_DATATIM_RECOV(chp->channel, drive,
			apollo_pio_rec[mode]);
	}
	if (idedma_ctl != 0) {
		/* Add software bits in status register */
		bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
		    IDEDMA_CTL + (IDEDMA_SCH_OFFSET * chp->channel),
		    idedma_ctl);
	}
	pciide_print_modes(cp);
	pci_conf_write(sc->sc_pc, sc->sc_tag, APO_DATATIM, datatim_reg);
	pci_conf_write(sc->sc_pc, sc->sc_tag, APO_UDMA, udmatim_reg);
}
#endif

void
cmd_channel_map(pa, sc, channel)
	struct pci_attach_args *pa;
	struct pciide_softc *sc;
	int channel;
{
	struct pciide_channel *cp = &sc->pciide_channels[channel];
	bus_size_t cmdsize, ctlsize;
	u_int8_t ctrl = pciide_pci_read(sc->sc_pc, sc->sc_tag, CMD_CTRL);
	pcireg_t interface =
	    PCI_INTERFACE(pci_conf_read(sc->sc_pc, sc->sc_tag, PCI_CLASS_REG));

	sc->wdc_chanarray[channel] = &cp->wdc_channel;
	cp->name = PCIIDE_CHANNEL_NAME(channel);
	cp->wdc_channel.channel = channel;
	cp->wdc_channel.wdc = &sc->sc_wdcdev;

	if (channel > 0) {
		cp->wdc_channel.ch_queue =
		    sc->pciide_channels[0].wdc_channel.ch_queue;
	} else {
		cp->wdc_channel.ch_queue =
		    malloc(sizeof(struct channel_queue), M_DEVBUF, M_NOWAIT);
	}
	if (cp->wdc_channel.ch_queue == NULL) {
		printf(
		    "%s: %s cannot allocate memory for command queue",
		    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
		return;
	}

	/*
	 * with a CMD PCI64x, if we get here, the first channel is enabled:
	 * there's no way to disable the first channel without disabling
	 * the whole device
	 */
	 if (channel != 0 && (ctrl & CMD_CTRL_2PORT) == 0) {
		printf("%s: %s ignored (disabled)\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
		return;
	}

	pciide_mapchan(pa, cp, interface, &cmdsize, &ctlsize, cmd_pci_intr);
	if (cp->hw_ok == 0)
		return;
	if (channel == 1) {
		if (pciiide_chan_candisable(cp)) {
			ctrl &= ~CMD_CTRL_2PORT;
			pciide_pci_write(pa->pa_pc, pa->pa_tag,
			    CMD_CTRL, ctrl);
		}
	}
	pciide_map_compat_intr(pa, cp, channel, interface);

}

int
cmd_pci_intr(arg)
	void *arg;
{
	struct pciide_softc *sc = arg;
	struct pciide_channel *cp;
	struct channel_softc *wdc_cp;
	int i, rv, crv; 
	u_int32_t priirq, secirq;

	rv = 0;
	priirq = pciide_pci_read(sc->sc_pc, sc->sc_tag, CMD_CONF);
	secirq = pciide_pci_read(sc->sc_pc, sc->sc_tag, CMD_ARTTIM23);
	for (i = 0; i < sc->sc_wdcdev.nchannels; i++) {
		cp = &sc->pciide_channels[i];
		wdc_cp = &cp->wdc_channel;
		/* If a compat channel skip. */
		if (cp->compat)
			continue;
		if ((i == 0 && (priirq & CMD_CONF_DRV0_INTR)) ||
		    (i == 1 && (secirq & CMD_ARTTIM23_IRQ))) {
			crv = wdcintr(wdc_cp);
#ifdef PMON
			if (crv == 0)
				rv = 0;
			else
				rv = 1;
#else
			if (crv == 0)
				printf("%s:%d: bogus intr\n",
				    sc->sc_wdcdev.sc_dev.dv_xname, i);
			else
				rv = 1;
#endif /* PMON */
		}
	}
	return rv;
}

void
cmd_chip_map(sc, pa)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
{
	int channel;
	pcireg_t interface =
	    PCI_INTERFACE(pci_conf_read(sc->sc_pc, sc->sc_tag, PCI_CLASS_REG));

	/*
 	 * For a CMD PCI064x, the use of PCI_COMMAND_IO_ENABLE
	 * and base adresses registers can be disabled at
 	 * hardware level. In this case, the device is wired
	 * in compat mode and its first channel is always enabled,
	 * but we can't rely on PCI_COMMAND_IO_ENABLE.
	 * In fact, it seems that the first channel of the CMD PCI0640
	 * can't be disabled.
 	 */

#ifdef PCIIDE_CMD064x_DISABLE
	if (pciide_chipen(sc, pa) == 0)
		return;
#endif

	printf(": no DMA");
	sc->sc_dma_ok = 0;

	sc->sc_wdcdev.channels = sc->wdc_chanarray;
	sc->sc_wdcdev.nchannels = PCIIDE_NUM_CHANNELS;
	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16;

	pciide_print_channels(sc->sc_wdcdev.nchannels, interface);

	for (channel = 0; channel < sc->sc_wdcdev.nchannels; channel++) {
		cmd_channel_map(pa, sc, channel);
	}
}

void
cmd0643_6_chip_map(sc, pa)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
{
	struct pciide_channel *cp;
	int channel;
	pcireg_t interface =
	    PCI_INTERFACE(pci_conf_read(sc->sc_pc, sc->sc_tag, PCI_CLASS_REG));

	/*
	 * For a CMD PCI064x, the use of PCI_COMMAND_IO_ENABLE
	 * and base adresses registers can be disabled at
	 * hardware level. In this case, the device is wired
	 * in compat mode and its first channel is always enabled,
 	 * but we can't rely on PCI_COMMAND_IO_ENABLE.
	 * In fact, it seems that the first channel of the CMD PCI0640
	 * can't be disabled.
	*/

#ifdef PCIIDE_CMD064x_DISABLE
	if (pciide_chipen(sc, pa) == 0)
		return;
#endif
	printf(": DMA");
	pciide_mapreg_dma(sc, pa);
	if (sc->sc_dma_ok)
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_DMA;

	sc->sc_wdcdev.channels = sc->wdc_chanarray;
	sc->sc_wdcdev.nchannels = PCIIDE_NUM_CHANNELS;
	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16 | WDC_CAPABILITY_DATA32 |
	    WDC_CAPABILITY_MODE;
	sc->sc_wdcdev.PIO_cap = 4;
	sc->sc_wdcdev.DMA_cap = 2;
	sc->sc_wdcdev.set_modes = cmd0643_6_setup_channel;

	pciide_print_channels(sc->sc_wdcdev.nchannels, interface);

	WDCDEBUG_PRINT(("cmd0643_6_chip_map: old timings reg 0x%x 0x%x\n",
		pci_conf_read(sc->sc_pc, sc->sc_tag, 0x54),
		pci_conf_read(sc->sc_pc, sc->sc_tag, 0x58)),
		DEBUG_PROBE);
	for (channel = 0; channel < sc->sc_wdcdev.nchannels; channel++) {
		cp = &sc->pciide_channels[channel];
		cmd_channel_map(pa, sc, channel);
		if (cp->hw_ok == 0)
			continue;
		cmd0643_6_setup_channel(&cp->wdc_channel);
	}
#if 0
	pciide_pci_write(sc->sc_pc, sc->sc_tag, CMD_DMA_MODE, CMD_DMA_MULTIPLE);
#else
	pciide_pci_write(sc->sc_pc, sc->sc_tag, CMD_DMA_MODE, CMD_DMA);
#endif
	WDCDEBUG_PRINT(("cmd0643_6_chip_map: timings reg now 0x%x 0x%x\n",
	    pci_conf_read(sc->sc_pc, sc->sc_tag, 0x54),
	    pci_conf_read(sc->sc_pc, sc->sc_tag, 0x58)),
	    DEBUG_PROBE);
}

void
cmd0643_6_setup_channel(chp)
	struct channel_softc *chp;
{
	struct ata_drive_datas *drvp;
	u_int8_t tim;
	u_int32_t idedma_ctl;
	int drive;
	struct pciide_channel *cp = (struct pciide_channel*)chp;
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.wdc;

	idedma_ctl = 0;
	/* setup DMA if needed */
	pciide_channel_dma_setup(cp);

	for (drive = 0; drive < 2; drive++) {
		drvp = &chp->ch_drive[drive];
		/* If no drive, skip */
		if ((drvp->drive_flags & DRIVE) == 0)
			continue;
		/* add timing values, setup DMA if needed */
		tim = cmd0643_6_data_tim_pio[drvp->PIO_mode];
		if (drvp->drive_flags & DRIVE_DMA) {
			/*
			 * use Multiword DMA.
			 * Timings will be used for both PIO and DMA, so adjust
			 * DMA mode if needed
			 */
			if (drvp->PIO_mode >= 3 &&
			    (drvp->DMA_mode + 2) > drvp->PIO_mode) {
				drvp->DMA_mode = drvp->PIO_mode - 2;
			}
			tim = cmd0643_6_data_tim_dma[drvp->DMA_mode];
			idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
		}
		pciide_pci_write(sc->sc_pc, sc->sc_tag,
		    CMD_DATA_TIM(chp->channel, drive), tim);
	}
	if (idedma_ctl != 0) {
		/* Add software bits in status register */
		bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
		    IDEDMA_CTL + (IDEDMA_SCH_OFFSET * chp->channel),
		    idedma_ctl);
	}
	pciide_print_modes(cp);
}

#define prom_printf printf

/****************************** copy from NETBSD : for siimage 0680: 20090325: zxj *******/
void cmd680_chip_map(sc, pa)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
{	 
	struct pciide_channel *cp;
	int channel;

	if (pciide_chipen(sc, pa) == 0)
		return;
	prom_printf("%s: 680 bus-master DMA support present",
	    sc->sc_wdcdev.sc_dev.dv_xname);
	pciide_mapreg_dma(sc, pa);
	prom_printf("\n");
	sc->sc_wdcdev.cap = WDC_CAPABILITY_DATA16 | WDC_CAPABILITY_DATA32 |
	    WDC_CAPABILITY_MODE;
#ifndef PMON
	if (sc->sc_dma_ok) {
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_DMA | WDC_CAPABILITY_IRQACK;
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_UDMA;
		sc->sc_wdcdev.UDMA_cap = 6;
		sc->sc_wdcdev.irqack = pciide_irqack;
	}
#endif
	sc->sc_wdcdev.channels = sc->wdc_chanarray;
	sc->sc_wdcdev.nchannels = PCIIDE_NUM_CHANNELS;
	sc->sc_wdcdev.PIO_cap = 4;
	sc->sc_wdcdev.DMA_cap = 2;
	sc->sc_wdcdev.set_modes = cmd680_setup_channel;

	pciide_pci_write(sc->sc_pc, sc->sc_tag, 0x80, 0x00);
	pciide_pci_write(sc->sc_pc, sc->sc_tag, 0x84, 0x00);
	pciide_pci_write(sc->sc_pc, sc->sc_tag, 0x8a,
	    pciide_pci_read(sc->sc_pc, sc->sc_tag, 0x8a) | 0x01);
	for (channel = 0; channel < sc->sc_wdcdev.nchannels; channel++) {
		cp = &sc->pciide_channels[channel];
		cmd680_channel_map(pa, sc, channel);
		if (cp->hw_ok == 0)
			continue;
		cmd680_setup_channel(&cp->wdc_channel);
	}
}

void
cmd680_channel_map(pa, sc, channel)
	struct pci_attach_args *pa;
	struct pciide_softc *sc;
	int channel;
{
	struct pciide_channel *cp = &sc->pciide_channels[channel];
	bus_size_t cmdsize, ctlsize;
	int interface, i, reg;
	static const u_int8_t init_val[] =
	    {             0x8a, 0x32, 0x8a, 0x32, 0x8a, 0x32,
	      0x92, 0x43, 0x92, 0x43, 0x09, 0x40, 0x09, 0x40 };

	if (PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_MASS_STORAGE_IDE) {
		interface = PCIIDE_INTERFACE_SETTABLE(0) |
		    PCIIDE_INTERFACE_SETTABLE(1);
		interface |= PCIIDE_INTERFACE_PCI(0) |
		    PCIIDE_INTERFACE_PCI(1);
	} else {
		interface = PCI_INTERFACE(pa->pa_class);
	}

	sc->wdc_chanarray[channel] = &cp->wdc_channel;
	cp->name = PCIIDE_CHANNEL_NAME(channel);
	cp->wdc_channel.channel = channel;
	cp->wdc_channel.wdc = &sc->sc_wdcdev;

	cp->wdc_channel.ch_queue =
	    malloc(sizeof(struct channel_queue), M_DEVBUF, M_NOWAIT);
	if (cp->wdc_channel.ch_queue == NULL) {
		prom_printf("%s %s channel: "
		    "can't allocate memory for command queue",
		    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
		    return;
	}

	/* XXX */
	reg = 0xa2 + channel * 16;
	for (i = 0; i < sizeof(init_val); i++)
		pciide_pci_write(sc->sc_pc, sc->sc_tag, reg + i, init_val[i]);

	prom_printf("%s: %s channel %s to %s mode\n",
	    sc->sc_wdcdev.sc_dev.dv_xname, cp->name,
	    (interface & PCIIDE_INTERFACE_SETTABLE(channel)) ?
	    "configured" : "wired",
	    (interface & PCIIDE_INTERFACE_PCI(channel)) ?
	    "native-PCI" : "compatibility");

	pciide_mapchan(pa, cp, interface, &cmdsize, &ctlsize, pciide_pci_intr);
	if (cp->hw_ok == 0)
		return;
	pciide_map_compat_intr(pa, cp, channel, interface);
}

void
cmd680_setup_channel(chp)
	struct channel_softc *chp;
{
	struct ata_drive_datas *drvp;
	u_int8_t mode, off, scsc;
	u_int16_t val;
	u_int32_t idedma_ctl;
	int drive;
	struct pciide_channel *cp = (struct pciide_channel*)chp;
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.wdc;
	pci_chipset_tag_t pc = sc->sc_pc;
	pcitag_t pa = sc->sc_tag;
	static const u_int8_t udma2_tbl[] =
	    { 0x0f, 0x0b, 0x07, 0x06, 0x03, 0x02, 0x01 };
	static const u_int8_t udma_tbl[] =
	    { 0x0c, 0x07, 0x05, 0x04, 0x02, 0x01, 0x00 };
	static const u_int16_t dma_tbl[] =
	    { 0x2208, 0x10c2, 0x10c1 };
	static const u_int16_t pio_tbl[] =
	    { 0x328a, 0x2283, 0x1104, 0x10c3, 0x10c1 };

	idedma_ctl = 0;
	pciide_channel_dma_setup(cp);
	mode = pciide_pci_read(pc, pa, 0x80 + chp->channel * 4);

	prom_printf("aaaaaaaaaaaaaaaaaaaaa!!!!!!!!!! cmd680_setup_channel\n"); /*zxj*/
	for (drive = 0; drive < 2; drive++) {
		drvp = &chp->ch_drive[drive];
		/* If no drive, skip */
		prom_printf("aaaaaaaaaaaaaaaa before,flag is %x\n",drvp->drive_flags);
		if ((drvp->drive_flags & DRIVE) == 0)
			continue;
		prom_printf("aaaaaaaaaaaaaaaa after\n");
		mode &= ~(0x03 << (drive * 4));
		if (drvp->drive_flags & DRIVE_UDMA) {
			drvp->drive_flags &= ~DRIVE_DMA;
			off = 0xa0 + chp->channel * 16;
			if (drvp->UDMA_mode > 2 &&
			    (pciide_pci_read(pc, pa, off) & 0x01) == 0)
				drvp->UDMA_mode = 2;
			scsc = pciide_pci_read(pc, pa, 0x8a);
			if (drvp->UDMA_mode == 6 && (scsc & 0x30) == 0) {
				pciide_pci_write(pc, pa, 0x8a, scsc | 0x01);
				scsc = pciide_pci_read(pc, pa, 0x8a);
				if ((scsc & 0x30) == 0)
					drvp->UDMA_mode = 5;
			}
			mode |= 0x03 << (drive * 4);
			off = 0xac + chp->channel * 16 + drive * 2;
			val = pciide_pci_read(pc, pa, off) & ~0x3f;
			if (scsc & 0x30)
				val |= udma2_tbl[drvp->UDMA_mode];
			else
				val |= udma_tbl[drvp->UDMA_mode];
			pciide_pci_write(pc, pa, off, val);
			idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
		} else if (drvp->drive_flags & DRIVE_DMA) {
			mode |= 0x02 << (drive * 4);
			off = 0xa8 + chp->channel * 16 + drive * 2;
			val = dma_tbl[drvp->DMA_mode];
			pciide_pci_write(pc, pa, off, val & 0xff);
			pciide_pci_write(pc, pa, off, val >> 8);
			idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
		} else {
			mode |= 0x01 << (drive * 4);
			off = 0xa4 + chp->channel * 16 + drive * 2;
			val = pio_tbl[drvp->PIO_mode];
			pciide_pci_write(pc, pa, off, val & 0xff);
			pciide_pci_write(pc, pa, off, val >> 8);
		}
	}

	pciide_pci_write(pc, pa, 0x80 + chp->channel * 4, mode);
	if (idedma_ctl != 0) {
		/* Add software bits in status register */
		bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
		    IDEDMA_CTL + (IDEDMA_SCH_OFFSET * chp->channel),
		    idedma_ctl);
	}
	prom_printf("aaaaaaaaaaaaaaaaa!!!!!!!!!!!!!! before print_modes\n"); /*zxj*/
	pciide_print_modes(cp);
}



/************************************************************************************/


#ifndef PMON
void
cy693_chip_map(sc, pa)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
{       
	struct pciide_channel *cp;
	pcireg_t interface = PCI_INTERFACE(pci_conf_read(sc->sc_pc,
				    sc->sc_tag, PCI_CLASS_REG));
	int compatchan;

	if (pciide_chipen(sc, pa) == 0)
		return;
	/*
	 * this chip has 2 PCI IDE functions, one for primary and one for
	 * secondary. So we need to call pciide_mapregs_compat() with
	 * the real channel
	 */
	if (pa->pa_function == 1) {
		compatchan = 0;
	} else if (pa->pa_function == 2) {
		compatchan = 1;
	} else {
		printf("%s: unexpected PCI function %d\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, pa->pa_function);
		return;
	}
	if (interface & PCIIDE_INTERFACE_BUS_MASTER_DMA) {
		printf(": DMA");
		pciide_mapreg_dma(sc, pa);
	} else {
		printf(": no DMA");
		sc->sc_dma_ok = 0;
	}

	pciide_print_channels(sc->sc_wdcdev.nchannels, interface);

	if (sc->sc_dma_ok)
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_DMA;
	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16 | WDC_CAPABILITY_DATA32 |
	    WDC_CAPABILITY_MODE;
	sc->sc_wdcdev.PIO_cap = 4;
	sc->sc_wdcdev.DMA_cap = 2;
	sc->sc_wdcdev.set_modes = cy693_setup_channel;

	sc->sc_wdcdev.channels = sc->wdc_chanarray;
	sc->sc_wdcdev.nchannels = 1;

	/* Only one channel for this chip; if we are here it's enabled */
	cp = &sc->pciide_channels[0];
		sc->wdc_chanarray[0] = &cp->wdc_channel;
	cp->name = PCIIDE_CHANNEL_NAME(0);
	cp->wdc_channel.channel = 0;
	cp->wdc_channel.wdc = &sc->sc_wdcdev;
	cp->wdc_channel.ch_queue =
	    malloc(sizeof(struct channel_queue), M_DEVBUF, M_NOWAIT);
	if (cp->wdc_channel.ch_queue == NULL) {
		printf("%s: %s"
		    "cannot allocate memory for command queue",
		sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
		return;
	}

	cp->wdc_channel.data32iot = cp->wdc_channel.cmd_iot;
	cp->wdc_channel.data32ioh = cp->wdc_channel.cmd_ioh;
	wdcattach(&cp->wdc_channel);
	if (pciiide_chan_candisable(cp)) {
		pci_conf_write(sc->sc_pc, sc->sc_tag,
		    PCI_COMMAND_STATUS_REG, 0);
	}
	pciide_map_compat_intr(pa, cp, compatchan, interface);
	if (cp->hw_ok == 0)
		return;
	WDCDEBUG_PRINT(("cy693_chip_map: old timings reg 0x%x\n",
	    pci_conf_read(sc->sc_pc, sc->sc_tag, CY_CMD_CTRL)),DEBUG_PROBE);
	cy693_setup_channel(&cp->wdc_channel);
	WDCDEBUG_PRINT(("cy693_chip_map: new timings reg 0x%x\n",
	    pci_conf_read(sc->sc_pc, sc->sc_tag, CY_CMD_CTRL)), DEBUG_PROBE);
}

void
cy693_setup_channel(chp)
	struct channel_softc *chp;
{
	struct ata_drive_datas *drvp;
	int drive;
	u_int32_t cy_cmd_ctrl;
	u_int32_t idedma_ctl;
	struct pciide_channel *cp = (struct pciide_channel*)chp;
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.wdc;
	int dma_mode = -1;

	cy_cmd_ctrl = idedma_ctl = 0;

	/* setup DMA if needed */
	pciide_channel_dma_setup(cp);

	for (drive = 0; drive < 2; drive++) {
		drvp = &chp->ch_drive[drive];
		/* If no drive, skip */
		if ((drvp->drive_flags & DRIVE) == 0)
			continue;
		/* add timing values, setup DMA if needed */
		if (drvp->drive_flags & DRIVE_DMA) {
			idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
			/* use Multiword DMA */
			if (dma_mode == -1 || dma_mode > drvp->DMA_mode)
				dma_mode = drvp->DMA_mode;
		}
		cy_cmd_ctrl |= (cy_pio_pulse[drvp->PIO_mode] <<
		    CY_CMD_CTRL_IOW_PULSE_OFF(drive));
		cy_cmd_ctrl |= (cy_pio_rec[drvp->PIO_mode] <<
		    CY_CMD_CTRL_IOW_REC_OFF(drive));
		cy_cmd_ctrl |= (cy_pio_pulse[drvp->PIO_mode] <<
		    CY_CMD_CTRL_IOR_PULSE_OFF(drive));
		cy_cmd_ctrl |= (cy_pio_rec[drvp->PIO_mode] <<
		    CY_CMD_CTRL_IOR_REC_OFF(drive));
	}
	pci_conf_write(sc->sc_pc, sc->sc_tag, CY_CMD_CTRL, cy_cmd_ctrl);
	chp->ch_drive[0].DMA_mode = dma_mode;
	chp->ch_drive[1].DMA_mode = dma_mode;
	pciide_print_modes(cp);
	if (idedma_ctl != 0) {
		/* Add software bits in status register */
		bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
		    IDEDMA_CTL, idedma_ctl);
	}
}

void
sis_chip_map(sc, pa)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
{
	struct pciide_channel *cp;
	int channel;
	u_int8_t sis_ctr0 = pciide_pci_read(sc->sc_pc, sc->sc_tag, SIS_CTRL0);
	pcireg_t interface = PCI_INTERFACE(pci_conf_read(sc->sc_pc,
				    sc->sc_tag, PCI_CLASS_REG));
	pcireg_t rev = PCI_REVISION(pci_conf_read(sc->sc_pc, sc->sc_tag,
			    PCI_CLASS_REG));
	bus_size_t cmdsize, ctlsize;

	if (pciide_chipen(sc, pa) == 0)
		return;

	printf(": DMA");
	pciide_mapreg_dma(sc, pa);

	if (sc->sc_dma_ok) {
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_DMA;
		if (rev >= 0xd0)
			sc->sc_wdcdev.cap |= WDC_CAPABILITY_UDMA;
	}

	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16 | WDC_CAPABILITY_DATA32 |
	    WDC_CAPABILITY_MODE;
	sc->sc_wdcdev.PIO_cap = 4;
	sc->sc_wdcdev.DMA_cap = 2;
	if (sc->sc_wdcdev.cap & WDC_CAPABILITY_UDMA)
		sc->sc_wdcdev.UDMA_cap = 2;
	sc->sc_wdcdev.set_modes = sis_setup_channel;
	sc->sc_wdcdev.channels = sc->wdc_chanarray;
	sc->sc_wdcdev.nchannels = PCIIDE_NUM_CHANNELS;

	pciide_print_channels(sc->sc_wdcdev.nchannels, interface);

	pciide_pci_write(sc->sc_pc, sc->sc_tag, SIS_MISC,
	    pciide_pci_read(sc->sc_pc, sc->sc_tag, SIS_MISC) |
	    SIS_MISC_TIM_SEL | SIS_MISC_FIFO_SIZE);

	for (channel = 0; channel < sc->sc_wdcdev.nchannels; channel++) {
		cp = &sc->pciide_channels[channel];
		if (pciide_chansetup(sc, channel, interface) == 0)
			continue;
		if ((channel == 0 && (sis_ctr0 & SIS_CTRL0_CHAN0_EN) == 0) ||
	 	    (channel == 1 && (sis_ctr0 & SIS_CTRL0_CHAN1_EN) == 0)) {
			printf("%s: %s ignored (disabled)\n",
			    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
			continue;
		}
		pciide_mapchan(pa, cp, interface, &cmdsize, &ctlsize,
		    pciide_pci_intr);
		if (cp->hw_ok == 0)
			continue;
		if (pciiide_chan_candisable(cp)) {
			if (channel == 0)
				sis_ctr0 &= ~SIS_CTRL0_CHAN0_EN;
			else
				sis_ctr0 &= ~SIS_CTRL0_CHAN1_EN;
			pciide_pci_write(sc->sc_pc, sc->sc_tag, SIS_CTRL0,
			    sis_ctr0);
		}
		pciide_map_compat_intr(pa, cp, channel, interface);
		if (cp->hw_ok == 0)
			continue;
		sis_setup_channel(&cp->wdc_channel);
	}
}

void
sis_setup_channel(chp)
	struct channel_softc *chp;
{
	struct ata_drive_datas *drvp;
	int drive;
	u_int32_t sis_tim;
	u_int32_t idedma_ctl;
	struct pciide_channel *cp = (struct pciide_channel*)chp;
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.wdc;

	WDCDEBUG_PRINT(("sis_setup_channel: old timings reg for "
	    "channel %d 0x%x\n", chp->channel, 
	    pci_conf_read(sc->sc_pc, sc->sc_tag, SIS_TIM(chp->channel))),
	    DEBUG_PROBE);
	sis_tim = 0;
	idedma_ctl = 0;
	/* setup DMA if needed */
	pciide_channel_dma_setup(cp);

	for (drive = 0; drive < 2; drive++) {
		drvp = &chp->ch_drive[drive];
		/* If no drive, skip */
		if ((drvp->drive_flags & DRIVE) == 0)
			continue;
		/* add timing values, setup DMA if needed */
		if ((drvp->drive_flags & DRIVE_DMA) == 0 &&
		    (drvp->drive_flags & DRIVE_UDMA) == 0)
			goto pio;

		if (drvp->drive_flags & DRIVE_UDMA) {
			/* use Ultra/DMA */
			drvp->drive_flags &= ~DRIVE_DMA;
			sis_tim |= sis_udma_tim[drvp->UDMA_mode] << 
			    SIS_TIM_UDMA_TIME_OFF(drive);
			sis_tim |= SIS_TIM_UDMA_EN(drive);
		} else {
			/*
			 * use Multiword DMA
			 * Timings will be used for both PIO and DMA,
			 * so adjust DMA mode if needed
			 */
			if (drvp->PIO_mode > (drvp->DMA_mode + 2))
				drvp->PIO_mode = drvp->DMA_mode + 2;
			if (drvp->DMA_mode + 2 > (drvp->PIO_mode))
				drvp->DMA_mode = (drvp->PIO_mode > 2) ?
				    drvp->PIO_mode - 2 : 0;
			if (drvp->DMA_mode == 0)
				drvp->PIO_mode = 0;
		}
		idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
pio:		sis_tim |= sis_pio_act[drvp->PIO_mode] <<
		    SIS_TIM_ACT_OFF(drive);
		sis_tim |= sis_pio_rec[drvp->PIO_mode] <<
		    SIS_TIM_REC_OFF(drive);
	}
	WDCDEBUG_PRINT(("sis_setup_channel: new timings reg for "
	    "channel %d 0x%x\n", chp->channel, sis_tim), DEBUG_PROBE);
	pci_conf_write(sc->sc_pc, sc->sc_tag, SIS_TIM(chp->channel), sis_tim);
	if (idedma_ctl != 0) {
		/* Add software bits in status register */
		bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
		    IDEDMA_CTL, idedma_ctl);
	}
	pciide_print_modes(cp);
}
#endif

void
acer_chip_map(sc, pa)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
{
	struct pciide_channel *cp;
	int channel;
	pcireg_t cr, interface;
	bus_size_t cmdsize, ctlsize;
	u_int8_t pif, cfg;

	if (pciide_chipen(sc, pa) == 0)
		return;

	printf(": Acer");
	pciide_mapreg_dma(sc, pa);

	if (sc->sc_dma_ok)
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_DMA | WDC_CAPABILITY_UDMA;

	sc->sc_wdcdev.cap |= WDC_CAPABILITY_DATA16 | WDC_CAPABILITY_DATA32 |
	    WDC_CAPABILITY_MODE;
	sc->sc_wdcdev.PIO_cap = 4;
	sc->sc_wdcdev.DMA_cap = 2;
	sc->sc_wdcdev.UDMA_cap = 2;
	sc->sc_wdcdev.set_modes = acer_setup_channel;
	sc->sc_wdcdev.channels = sc->wdc_chanarray;
	sc->sc_wdcdev.nchannels = PCIIDE_NUM_CHANNELS;

#ifdef PMON
	pciide_pci_write(sc->sc_pc, sc->sc_tag, ACER_CCAR3, 0x03);
	pif = pciide_pci_read(sc->sc_pc, sc->sc_tag, 0x09);
	if(!(pif & 0x40)) {
		cfg = pciide_pci_read(sc->sc_pc, sc->sc_tag, ACER_CCAR2);
		pciide_pci_write(sc->sc_pc, sc->sc_tag, ACER_CCAR2,
					cfg & ~ACER_CHANSTATUSREGS_RO);
		pciide_pci_write(sc->sc_pc, sc->sc_tag, 0x09, pif | 0x40);
		pciide_pci_write(sc->sc_pc, sc->sc_tag, ACER_CCAR2, cfg);
	}
	pciide_pci_write(sc->sc_pc, sc->sc_tag, 0x09, 0xda);

#else
	pciide_pci_write(sc->sc_pc, sc->sc_tag, ACER_CDRC,
	    (pciide_pci_read(sc->sc_pc, sc->sc_tag, ACER_CDRC) |
		ACER_CDRC_DMA_EN) & ~ACER_CDRC_FIFO_DISABLE);


	/* Enable "microsoft register bits" R/W. */
	pciide_pci_write(sc->sc_pc, sc->sc_tag, ACER_CCAR3,
	    pciide_pci_read(sc->sc_pc, sc->sc_tag, ACER_CCAR3) | ACER_CCAR3_PI);
	pciide_pci_write(sc->sc_pc, sc->sc_tag, ACER_CCAR1,
	    pciide_pci_read(sc->sc_pc, sc->sc_tag, ACER_CCAR1) &
	    ~(ACER_CHANSTATUS_RO|PCIIDE_CHAN_RO(0)|PCIIDE_CHAN_RO(1)));
	pciide_pci_write(sc->sc_pc, sc->sc_tag, ACER_CCAR2,
	    pciide_pci_read(sc->sc_pc, sc->sc_tag, ACER_CCAR2) &
	    ~ACER_CHANSTATUSREGS_RO);
	pci_conf_write(sc->sc_pc, sc->sc_tag, PCI_CLASS_REG, cr);
#endif
	cr = pci_conf_read(sc->sc_pc, sc->sc_tag, PCI_CLASS_REG);
	cr |= (PCIIDE_CHANSTATUS_EN << PCI_INTERFACE_SHIFT);
	/* Don't use cr, re-read the real register content instead */
	interface = PCI_INTERFACE(pci_conf_read(sc->sc_pc, sc->sc_tag,
	    PCI_CLASS_REG));

	pciide_print_channels(sc->sc_wdcdev.nchannels, interface);

	for (channel = 0; channel < sc->sc_wdcdev.nchannels; channel++) {
		cp = &sc->pciide_channels[channel];
		if (pciide_chansetup(sc, channel, interface) == 0)
			continue;
		if ((interface & PCIIDE_CHAN_EN(channel)) == 0) {
			printf("%s: %s ignored (disabled)\n",
			    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
			continue;
		}
		pciide_mapchan(pa, cp, interface, &cmdsize, &ctlsize,
		    acer_pci_intr);
		if (cp->hw_ok == 0)
			continue;
		if (pciiide_chan_candisable(cp)) {
			cr &= ~(PCIIDE_CHAN_EN(channel) << PCI_INTERFACE_SHIFT);
			pci_conf_write(sc->sc_pc, sc->sc_tag,
			    PCI_CLASS_REG, cr);
		}
		pciide_map_compat_intr(pa, cp, channel, interface);
		acer_setup_channel(&cp->wdc_channel);
	}
}

void
acer_setup_channel(chp)
	struct channel_softc *chp;
{
	struct ata_drive_datas *drvp;
	int drive;
	u_int32_t acer_fifo_udma;
	u_int32_t idedma_ctl;
	struct pciide_channel *cp = (struct pciide_channel*)chp;
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.wdc;

	idedma_ctl = 0;
	acer_fifo_udma = pci_conf_read(sc->sc_pc, sc->sc_tag, ACER_FTH_UDMA);
	WDCDEBUG_PRINT(("acer_setup_channel: old fifo/udma reg 0x%x\n", 
	    acer_fifo_udma), DEBUG_PROBE);
	/* setup DMA if needed */
	pciide_channel_dma_setup(cp);

	for (drive = 0; drive < 2; drive++) {
		drvp = &chp->ch_drive[drive];
		/* If no drive, skip */
		if ((drvp->drive_flags & DRIVE) == 0)
			continue;
		WDCDEBUG_PRINT(("acer_setup_channel: old timings reg for "
		    "channel %d drive %d 0x%x\n", chp->channel, drive,
		    pciide_pci_read(sc->sc_pc, sc->sc_tag,
		    ACER_IDETIM(chp->channel, drive))), DEBUG_PROBE);
		/* clear FIFO/DMA mode */
		acer_fifo_udma &= ~(ACER_FTH_OPL(chp->channel, drive, 0x3) |
		    ACER_UDMA_EN(chp->channel, drive) |
		    ACER_UDMA_TIM(chp->channel, drive, 0x7));

		/* add timing values, setup DMA if needed */
		if ((drvp->drive_flags & DRIVE_DMA) == 0 &&
		    (drvp->drive_flags & DRIVE_UDMA) == 0) {
			acer_fifo_udma |=
			    ACER_FTH_OPL(chp->channel, drive, 0x1);
			goto pio;
		}

		acer_fifo_udma |= ACER_FTH_OPL(chp->channel, drive, 0x2);
		if (drvp->drive_flags & DRIVE_UDMA) {
			/* use Ultra/DMA */
			drvp->drive_flags &= ~DRIVE_DMA;
			acer_fifo_udma |= ACER_UDMA_EN(chp->channel, drive);
			acer_fifo_udma |= 
			    ACER_UDMA_TIM(chp->channel, drive,
				acer_udma[drvp->UDMA_mode]);
		} else {
			/*
			 * use Multiword DMA
			 * Timings will be used for both PIO and DMA,
			 * so adjust DMA mode if needed
			 */
			if (drvp->PIO_mode > (drvp->DMA_mode + 2))
				drvp->PIO_mode = drvp->DMA_mode + 2;
			if (drvp->DMA_mode + 2 > (drvp->PIO_mode))
				drvp->DMA_mode = (drvp->PIO_mode > 2) ?
				    drvp->PIO_mode - 2 : 0;
			if (drvp->DMA_mode == 0)
				drvp->PIO_mode = 0;
		}
		idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
pio:		pciide_pci_write(sc->sc_pc, sc->sc_tag,
		    ACER_IDETIM(chp->channel, drive),
		    acer_pio[drvp->PIO_mode]);
	}
	WDCDEBUG_PRINT(("acer_setup_channel: new fifo/udma reg 0x%x\n",
	    acer_fifo_udma), DEBUG_PROBE);
	pci_conf_write(sc->sc_pc, sc->sc_tag, ACER_FTH_UDMA, acer_fifo_udma);
	if (idedma_ctl != 0) {
		/* Add software bits in status register */
		bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
		    IDEDMA_CTL, idedma_ctl);
	}
	pciide_print_modes(cp);
}

int
acer_pci_intr(arg)
	void *arg;
{
	struct pciide_softc *sc = arg;
	struct pciide_channel *cp;
	struct channel_softc *wdc_cp;
	int i, rv, crv; 
	u_int32_t chids;

	rv = 0;
	chids = pciide_pci_read(sc->sc_pc, sc->sc_tag, ACER_CHIDS);
	for (i = 0; i < sc->sc_wdcdev.nchannels; i++) {
		cp = &sc->pciide_channels[i];
		wdc_cp = &cp->wdc_channel;
		/* If a compat channel skip. */
		if (cp->compat)
			continue;
		if (chids & ACER_CHIDS_INT(i)) {
			crv = wdcintr(wdc_cp);
#ifdef PMON
			if (crv == 0)
				rv = 0;
			else
				rv = 1;
#else
			if (crv == 0)
				printf("%s:%d: bogus intr\n",
				    sc->sc_wdcdev.sc_dev.dv_xname, i);
			else
				rv = 1;
#endif /* PMON */
		}
	}
	return rv;
}

/* Macros to test product */
#define PDC_IS_262(sc)							\
	((sc)->sc_pp->ide_product == PCI_PRODUCT_PROMISE_PDC20262 ||	\
	(sc)->sc_pp->ide_product == PCI_PRODUCT_PROMISE_PDC20265  ||	\
	(sc)->sc_pp->ide_product == PCI_PRODUCT_PROMISE_PDC20267)
#define PDC_IS_265(sc)							\
	((sc)->sc_pp->ide_product == PCI_PRODUCT_PROMISE_PDC20265 ||	\
	(sc)->sc_pp->ide_product == PCI_PRODUCT_PROMISE_PDC20267  ||	\
	(sc)->sc_pp->ide_product == PCI_PRODUCT_PROMISE_PDC20268  ||	\
	(sc)->sc_pp->ide_product == PCI_PRODUCT_PROMISE_PDC20268R ||	\
	(sc)->sc_pp->ide_product == PCI_PRODUCT_PROMISE_PDC20269)
#define PDC_IS_268(sc)							\
	((sc)->sc_pp->ide_product == PCI_PRODUCT_PROMISE_PDC20268 ||	\
	(sc)->sc_pp->ide_product == PCI_PRODUCT_PROMISE_PDC20268R ||	\
	(sc)->sc_pp->ide_product == PCI_PRODUCT_PROMISE_PDC20269)

void
pdc202xx_chip_map(sc, pa)
	struct pciide_softc *sc;
	struct pci_attach_args *pa;
{
	struct pciide_channel *cp;
	int channel;
	pcireg_t interface, st, mode;
	bus_size_t cmdsize, ctlsize;
	int pdc_is_262, pdc_is_265, pdc_is_268;

	pdc_is_262 = PDC_IS_262(sc);
	pdc_is_265 = PDC_IS_265(sc);
	pdc_is_268 = PDC_IS_268(sc);

	if (!pdc_is_268) {
		st = pci_conf_read(sc->sc_pc, sc->sc_tag, PDC2xx_STATE);
		WDCDEBUG_PRINT(("pdc202xx_setup_chip: controller state 0x%x\n",
		    st), DEBUG_PROBE);
	}
	if (pciide_chipen(sc, pa) == 0)
		return;

	/* turn off  RAID mode */
	if (!pdc_is_268)
		st &= ~PDC2xx_STATE_IDERAID;

	/*
 	 * can't rely on the PCI_CLASS_REG content if the chip was in raid
	 * mode. We have to fake interface
	 */
	interface = PCIIDE_INTERFACE_SETTABLE(0) | PCIIDE_INTERFACE_SETTABLE(1);
	if (pdc_is_268 || (st & PDC2xx_STATE_NATIVE))
		interface |= PCIIDE_INTERFACE_PCI(0) | PCIIDE_INTERFACE_PCI(1);

	printf(": DMA");
	pciide_mapreg_dma(sc, pa);

	sc->sc_wdcdev.cap = WDC_CAPABILITY_DATA16 | WDC_CAPABILITY_DATA32 |
	    WDC_CAPABILITY_MODE | WDC_CAPABILITY_NO_ATAPI_DMA;
#ifndef PMON
	if (sc->sc_dma_ok) {
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_DMA | WDC_CAPABILITY_UDMA;
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_IRQACK;
		sc->sc_wdcdev.irqack = pciide_irqack;
	}
#endif
	sc->sc_wdcdev.PIO_cap = 4;
	sc->sc_wdcdev.DMA_cap = 2;
	if (pdc_is_265)
		sc->sc_wdcdev.UDMA_cap = 5;
	else if (pdc_is_262)
		sc->sc_wdcdev.UDMA_cap = 4;
	else
		sc->sc_wdcdev.UDMA_cap = 2;
	sc->sc_wdcdev.set_modes = pdc_is_268 ?
			pdc20268_setup_channel : pdc202xx_setup_channel;
	sc->sc_wdcdev.channels = sc->wdc_chanarray;
	sc->sc_wdcdev.nchannels = PCIIDE_NUM_CHANNELS;

	pciide_print_channels(sc->sc_wdcdev.nchannels, interface);
	if (!pdc_is_268) {
		/* setup failsafe defaults */
		mode = 0;
		mode = PDC2xx_TIM_SET_PA(mode, pdc2xx_pa[0]);
		mode = PDC2xx_TIM_SET_PB(mode, pdc2xx_pb[0]);
		mode = PDC2xx_TIM_SET_MB(mode, pdc2xx_dma_mb[0]);
		mode = PDC2xx_TIM_SET_MC(mode, pdc2xx_dma_mc[0]);
		for (channel = 0;
		     channel < sc->sc_wdcdev.nchannels;
		     channel++) {
			WDCDEBUG_PRINT(("pdc202xx_setup_chip: channel %d "
			    "drive 0 initial timings  0x%x, now 0x%x\n",
			    channel, pci_conf_read(sc->sc_pc, sc->sc_tag,
			    PDC2xx_TIM(channel, 0)), mode | PDC2xx_TIM_IORDYp),
			    DEBUG_PROBE);
			pci_conf_write(sc->sc_pc, sc->sc_tag,
			    PDC2xx_TIM(channel, 0), mode | PDC2xx_TIM_IORDYp);
			WDCDEBUG_PRINT(("pdc202xx_setup_chip: channel %d "
			    "drive 1 initial timings  0x%x, now 0x%x\n",
			    channel, pci_conf_read(sc->sc_pc, sc->sc_tag,
	 		    PDC2xx_TIM(channel, 1)), mode), DEBUG_PROBE);
			pci_conf_write(sc->sc_pc, sc->sc_tag,
			    PDC2xx_TIM(channel, 1), mode);
		}

		mode = PDC2xx_SCR_DMA;
		if (pdc_is_262) {
			mode = PDC2xx_SCR_SET_GEN(mode, PDC262_SCR_GEN_LAT);
		} else {
			/* the BIOS set it up this way */
			mode = PDC2xx_SCR_SET_GEN(mode, 0x1);
		}
		mode = PDC2xx_SCR_SET_I2C(mode, 0x3); /* ditto */
		mode = PDC2xx_SCR_SET_POLL(mode, 0x1); /* ditto */
		WDCDEBUG_PRINT(("pdc202xx_setup_chip: initial SCR  0x%x, "
		    "now 0x%x\n",
		    bus_space_read_4(sc->sc_dma_iot, sc->sc_dma_ioh,
			PDC2xx_SCR),
		    mode), DEBUG_PROBE);
		bus_space_write_4(sc->sc_dma_iot, sc->sc_dma_ioh,
		    PDC2xx_SCR, mode);

		/* controller initial state register is OK even without BIOS */
		/* Set DMA mode to IDE DMA compatibility */
		mode =
		    bus_space_read_1(sc->sc_dma_iot, sc->sc_dma_ioh, PDC2xx_PM);
		WDCDEBUG_PRINT(("pdc202xx_setup_chip: primary mode 0x%x", mode),
		    DEBUG_PROBE);
		bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh, PDC2xx_PM,
		    mode | 0x1);
		mode =
		    bus_space_read_1(sc->sc_dma_iot, sc->sc_dma_ioh, PDC2xx_SM);
		WDCDEBUG_PRINT((", secondary mode 0x%x\n", mode ), DEBUG_PROBE);
		bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh, PDC2xx_SM,
		    mode | 0x1);
	}

	for (channel = 0; channel < sc->sc_wdcdev.nchannels; channel++) {
		cp = &sc->pciide_channels[channel];
		if (pciide_chansetup(sc, channel, interface) == 0)
			continue;
		if (!pdc_is_268 && (st & (pdc_is_262 ?
		    PDC262_STATE_EN(channel):PDC246_STATE_EN(channel))) == 0) {
			printf("%s: %s channel ignored (disabled)\n",
			    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
			continue;
		}
		pciide_map_compat_intr(pa, cp, channel, interface);
		if (cp->hw_ok == 0)
			continue;
		if (pdc_is_265)
			pciide_mapchan(pa, cp, interface, &cmdsize, &ctlsize,
			    pdc20265_pci_intr);
		else
			pciide_mapchan(pa, cp, interface, &cmdsize, &ctlsize,
			    pdc202xx_pci_intr);
		if (cp->hw_ok == 0) {
#ifndef PMON
			pciide_unmap_compat_intr(pa, cp, channel, interface);
#endif
			continue;
		}
#ifndef PMON
		if (!pdc_is_268 && pciide_chan_candisable(cp)) {
			st &= ~(pdc_is_262 ?
			    PDC262_STATE_EN(channel):PDC246_STATE_EN(channel));
			pciide_unmap_compat_intr(pa, cp, channel, interface);
		}
#endif
		if (pdc_is_268)
			pdc20268_setup_channel(&cp->wdc_channel);
		else
			pdc202xx_setup_channel(&cp->wdc_channel);
	}
	if (!pdc_is_268) {
		WDCDEBUG_PRINT(("pdc202xx_setup_chip: new controller state "
		    "0x%x\n", st), DEBUG_PROBE);
		pci_conf_write(sc->sc_pc, sc->sc_tag, PDC2xx_STATE, st);
	}
	return;
}

void
pdc202xx_setup_channel(chp)
	struct channel_softc *chp;
{
	struct ata_drive_datas *drvp;
	int drive;
	pcireg_t mode, st;
	u_int32_t idedma_ctl, scr, atapi;
	struct pciide_channel *cp = (struct pciide_channel*)chp;
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.wdc;
	int channel = chp->channel;

	/* setup DMA if needed */
	pciide_channel_dma_setup(cp);

	idedma_ctl = 0;
	WDCDEBUG_PRINT(("pdc202xx_setup_channel %s: scr 0x%x\n",
	    sc->sc_wdcdev.sc_dev.dv_xname,
	    bus_space_read_1(sc->sc_dma_iot, sc->sc_dma_ioh, PDC262_U66)),
	    DEBUG_PROBE);

	/* Per channel settings */
	if (PDC_IS_262(sc)) {
		scr = bus_space_read_1(sc->sc_dma_iot, sc->sc_dma_ioh,
		    PDC262_U66);
		st = pci_conf_read(sc->sc_pc, sc->sc_tag, PDC2xx_STATE);
		/* Trim UDMA mode */
		if ((st & PDC262_STATE_80P(channel)) != 0 ||
		    (chp->ch_drive[0].drive_flags & DRIVE_UDMA &&
		    chp->ch_drive[0].UDMA_mode <= 2) ||
		    (chp->ch_drive[1].drive_flags & DRIVE_UDMA &&
		    chp->ch_drive[1].UDMA_mode <= 2)) {
			if (chp->ch_drive[0].UDMA_mode > 2)
				chp->ch_drive[0].UDMA_mode = 2;
			if (chp->ch_drive[1].UDMA_mode > 2)
				chp->ch_drive[1].UDMA_mode = 2;
		}
		/* Set U66 if needed */
		if ((chp->ch_drive[0].drive_flags & DRIVE_UDMA &&
		    chp->ch_drive[0].UDMA_mode > 2) ||
		    (chp->ch_drive[1].drive_flags & DRIVE_UDMA &&
		    chp->ch_drive[1].UDMA_mode > 2))
			scr |= PDC262_U66_EN(channel);
		else
			scr &= ~PDC262_U66_EN(channel);
		bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
		    PDC262_U66, scr);
		WDCDEBUG_PRINT(("pdc202xx_setup_channel %s:%d: ATAPI 0x%x\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, channel,
		    bus_space_read_4(sc->sc_dma_iot, sc->sc_dma_ioh,
		    PDC262_ATAPI(channel))), DEBUG_PROBE);
		if (chp->ch_drive[0].drive_flags & DRIVE_ATAPI ||
		    chp->ch_drive[1].drive_flags & DRIVE_ATAPI) {
			if (((chp->ch_drive[0].drive_flags & DRIVE_UDMA) &&
			    !(chp->ch_drive[1].drive_flags & DRIVE_UDMA) &&
			    (chp->ch_drive[1].drive_flags & DRIVE_DMA)) ||
			    ((chp->ch_drive[1].drive_flags & DRIVE_UDMA) &&
			    !(chp->ch_drive[0].drive_flags & DRIVE_UDMA) &&
			    (chp->ch_drive[0].drive_flags & DRIVE_DMA)))
				atapi = 0;
			else
				atapi = PDC262_ATAPI_UDMA;
			bus_space_write_4(sc->sc_dma_iot, sc->sc_dma_ioh,
			    PDC262_ATAPI(channel), atapi);
		}
	}
	for (drive = 0; drive < 2; drive++) {
		drvp = &chp->ch_drive[drive];
		/* If no drive, skip */
		if ((drvp->drive_flags & DRIVE) == 0)
			continue;
		mode = 0;
		if (drvp->drive_flags & DRIVE_UDMA) {
			/* use Ultra/DMA */
			drvp->drive_flags &= ~DRIVE_DMA;
			mode = PDC2xx_TIM_SET_MB(mode,
			   pdc2xx_udma_mb[drvp->UDMA_mode]);
			mode = PDC2xx_TIM_SET_MC(mode,
			   pdc2xx_udma_mc[drvp->UDMA_mode]);
			idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
		} else if (drvp->drive_flags & DRIVE_DMA) {
			mode = PDC2xx_TIM_SET_MB(mode,
			    pdc2xx_dma_mb[drvp->DMA_mode]);
			mode = PDC2xx_TIM_SET_MC(mode,
			   pdc2xx_dma_mc[drvp->DMA_mode]);
			idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
		} else {
			mode = PDC2xx_TIM_SET_MB(mode,
			    pdc2xx_dma_mb[0]);
			mode = PDC2xx_TIM_SET_MC(mode,
			    pdc2xx_dma_mc[0]);
		}
		mode = PDC2xx_TIM_SET_PA(mode, pdc2xx_pa[drvp->PIO_mode]);
		mode = PDC2xx_TIM_SET_PB(mode, pdc2xx_pb[drvp->PIO_mode]);
		if (drvp->drive_flags & DRIVE_ATA)
			mode |= PDC2xx_TIM_PRE;
		mode |= PDC2xx_TIM_SYNC | PDC2xx_TIM_ERRDY;
		if (drvp->PIO_mode >= 3) {
			mode |= PDC2xx_TIM_IORDY;
			if (drive == 0)
				mode |= PDC2xx_TIM_IORDYp;
		}
		WDCDEBUG_PRINT(("pdc202xx_setup_channel: %s:%d:%d "
		    "timings 0x%x\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, 
		    chp->channel, drive, mode), DEBUG_PROBE);
		    pci_conf_write(sc->sc_pc, sc->sc_tag,
		    PDC2xx_TIM(chp->channel, drive), mode);
	}
	if (idedma_ctl != 0) {
		/* Add software bits in status register */
		bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
		    IDEDMA_CTL, idedma_ctl);
	}
	pciide_print_modes(cp);
}

void
pdc20268_setup_channel(chp)
	struct channel_softc *chp;
{
	struct ata_drive_datas *drvp;
	int drive, cable;
	u_int32_t idedma_ctl;
	struct pciide_channel *cp = (struct pciide_channel*)chp;
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.wdc;

	/* check 80 pins cable */
	bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh, PDC268_REG0, 0x0b);
	cable = bus_space_read_1(sc->sc_dma_iot, sc->sc_dma_ioh,
	    PDC268_REG1) & 0x04;

	/* setup DMA if needed */
	pciide_channel_dma_setup(cp);

	idedma_ctl = 0;
	
	for (drive = 0; drive < 2; drive++) {
		drvp = &chp->ch_drive[drive];
		/* If no drive, skip */
		if ((drvp->drive_flags & DRIVE) == 0)
			continue;
		if (drvp->drive_flags & DRIVE_UDMA) {
			/* use Ultra/DMA */
			drvp->drive_flags &= ~DRIVE_DMA;
			idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
			if (cable && drvp->UDMA_mode > 2)
				drvp->UDMA_mode = 2;
		} else if (drvp->drive_flags & DRIVE_DMA) {
			idedma_ctl |= IDEDMA_CTL_DRV_DMA(drive);
		}
	}
	/* nothing to do to setup modes, the controller snoop SET_FEATURE cmd */
	if (idedma_ctl != 0) {
		/* Add software bits in status register */
		bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
		    IDEDMA_CTL, idedma_ctl);
	}
	pciide_print_modes(cp);
}

int
pdc202xx_pci_intr(arg)
	void *arg;
{
	struct pciide_softc *sc = arg;
	struct pciide_channel *cp;
	struct channel_softc *wdc_cp;
	int i, rv, crv; 
	u_int32_t scr;

	rv = 0;
	scr = bus_space_read_4(sc->sc_dma_iot, sc->sc_dma_ioh, PDC2xx_SCR);
	for (i = 0; i < sc->sc_wdcdev.nchannels; i++) {
		cp = &sc->pciide_channels[i];
		wdc_cp = &cp->wdc_channel;
		/* If a compat channel skip. */
		if (cp->compat)
			continue;
		if (scr & PDC2xx_SCR_INT(i)) {
			crv = wdcintr(wdc_cp);
#ifdef PMON
			if (crv == 0)
				rv = 0;
			else
				rv = 1;
#else
			if (crv == 0)
				printf("%s:%d: bogus intr (reg 0x%x)\n",
				    sc->sc_wdcdev.sc_dev.dv_xname, i, scr);
			else
				rv = 1;
#endif /* PMON */
		}
	}
	return rv;
}

int
pdc20265_pci_intr(arg)
	void *arg;
{
	struct pciide_softc *sc = arg;
	struct pciide_channel *cp;
	struct channel_softc *wdc_cp;
	int i, rv, crv; 
	u_int32_t dmastat;

	/* process our own interrupts only during IRQ sharing */
	if (PDC_IS_268(sc)) {
		bus_space_write_1(sc->sc_dma_iot, sc->sc_dma_ioh,
		    PDC268_REG0, 0xb);
		if (!(bus_space_read_1(sc->sc_dma_iot, sc->sc_dma_ioh,
		    PDC268_REG1) & 0x20))
			return 0;
	}

	rv = 0;
	for (i = 0; i < sc->sc_wdcdev.nchannels; i++) {
		cp = &sc->pciide_channels[i];
		wdc_cp = &cp->wdc_channel;
		/* If a compat channel skip. */
		if (cp->compat)
			continue;
		/*
		 * The Ultra/100 seems to assert PDC2xx_SCR_INT * spuriously,
		 * however it asserts INT in IDEDMA_CTL even for non-DMA ops.
		 * So use it instead (requires 2 reg reads instead of 1,
		 * but we can't do it another way).
		 */
		dmastat = bus_space_read_1(sc->sc_dma_iot,
		    sc->sc_dma_ioh, IDEDMA_CTL + IDEDMA_SCH_OFFSET * i);
		if ((dmastat & IDEDMA_CTL_INTR) == 0)
			continue;

		crv = wdcintr(wdc_cp);
#ifdef PMON
		if (crv == 0)
			rv = 0;
		else
			rv = 1;
#else
		if (crv == 0)
			printf("%s:%d: bogus intr\n",
			    sc->sc_wdcdev.sc_dev.dv_xname, i);
		else
			rv = 1;
#endif /* PMON */
	}
	return rv;
}
