/*	$Id: pcivar.h,v 1.1.1.1 2006/09/14 01:59:08 root Exp $	*/
/*	$OpenBSD: pcivar.h,v 1.16 1999/07/18 03:20:18 csapuntz Exp $	*/
/*	$NetBSD: pcivar.h,v 1.23 1997/06/06 23:48:05 thorpej Exp $	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1994 Charles Hannum.  All rights reserved.
 * Copyright (c) 2001 Patrik Lindergren. All rights reserved.
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
 *	This product includes software developed by Charles Hannum.
 *      This product includes software developed by Patrik Lindergren.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#ifndef _DEV_PCI_PCIVAR_H_
#define	_DEV_PCI_PCIVAR_H_

/*
 * Definitions for PCI autoconfiguration.
 *
 * This file describes types and functions which are used for PCI
 * configuration.  Some of this information is machine-specific, and is
 * provided by pci_machdep.h.
 */

#include <machine/bus.h>
#include <dev/pci/pcireg.h>

/*
 * Structures and definitions needed by the machine-dependent header.
 */
typedef u_int32_t pcireg_t;		/* configuration space register XXX */
struct pcibus_attach_args;

#include <pci/pci_machdep.h>

/*
 * PCI bus attach arguments.
 */
struct pcibus_attach_args {
	char	*pba_busname;		/* XXX should be common */
	bus_space_tag_t pba_iot;	/* pci i/o space tag */
	bus_space_tag_t pba_memt;	/* pci mem space tag */
	bus_dma_tag_t pba_dmat;		/* DMA tag */
	pci_chipset_tag_t pba_pc;

	int		pba_bus;	/* PCI bus number */

	/*
	 * Interrupt swizzling information.  These fields
	 * are only used by secondary busses.
	 */
	u_int		pba_intrswiz;	/* how to swizzle pins */
	pcitag_t	pba_intrtag;	/* intr. appears to come from here */
};

/*
 * PCI device attach arguments.
 */
struct pci_attach_args {
	bus_space_tag_t pa_iot;		/* pci i/o space tag */
	bus_space_tag_t pa_memt;	/* pci mem space tag */
	bus_dma_tag_t pa_dmat;		/* DMA tag */
	pci_chipset_tag_t pa_pc;
	int		pa_flags;	/* flags; see below */
	int		pa_bus;

	u_int		pa_device;
	u_int		pa_function;
	pcitag_t	pa_tag;
	pcireg_t	pa_id, pa_class;

	/*
	 * Interrupt information.
	 *
	 * "Intrline" is used on systems whose firmware puts
	 * the right routing data into the line register in
	 * configuration space.  The rest are used on systems
	 * that do not.
	 */
	u_int		pa_intrswiz;	/* how to swizzle pins if ppb */
	pcitag_t	pa_intrtag;	/* intr. appears to come from here */
	pci_intr_pin_t	pa_intrpin;	/* intr. appears on this pin */
	pci_intr_line_t	pa_intrline;	/* intr. routing information */
};

/*
 * Flags given in the bus and device attachment args.
 *
 * OpenBSD doesn't actually use them yet -- csapuntz@cvs.openbsd.org
 */
#define	PCI_FLAGS_IO_ENABLED	0x01		/* I/O space is enabled */
#define	PCI_FLAGS_MEM_ENABLED	0x02		/* memory space is enabled */

/*
 *
 */
struct pci_quirkdata {
	pci_vendor_id_t		vendor;		/* Vendor ID */
	pci_product_id_t	product;	/* Product ID */
	int			quirks;		/* quirks; see below */
};
#define	PCI_QUIRK_MULTIFUNCTION		0x00000001

/*
 * Locators devices that attach to 'pcibus', as specified to config.
 */
#define	pcibuscf_bus		cf_loc[0]
#define	PCIBUS_UNK_BUS		-1		/* wildcarded 'bus' */

/*
 * Locators for PCI devices, as specified to config.
 */
#define	pcicf_dev		cf_loc[0]
#define	PCI_UNK_DEV		-1		/* wildcarded 'dev' */

#define	pcicf_function		cf_loc[1]
#define	PCI_UNK_FUNCTION	-1		/* wildcarded 'function' */

/*
 * Configuration space access and utility functions.  (Note that most,
 * e.g. make_tag, conf_read, conf_write are declared by pci_machdep.h.)
 */
pcireg_t pci_mapreg_type __P((pci_chipset_tag_t, pcitag_t, int));
int	pci_mapreg_info __P((pci_chipset_tag_t, pcitag_t, int, pcireg_t,
	    bus_addr_t *, bus_size_t *, int *));
int	pci_mapreg_map __P((struct pci_attach_args *, int, pcireg_t, int,
	    bus_space_tag_t *, bus_space_handle_t *, bus_addr_t *,
	    bus_size_t *, bus_size_t));


int	pci_io_find __P((pci_chipset_tag_t, pcitag_t, int, bus_addr_t *,
	    bus_size_t *));
int	pci_mem_find __P((pci_chipset_tag_t, pcitag_t, int, bus_addr_t *,
	    bus_size_t *, int *));

int pci_get_capability __P((pci_chipset_tag_t, pcitag_t, int,
			    int *, pcireg_t *));

/*
 * Helper functions for autoconfiguration.
 */
void	set_pci_isa_bridge_callback __P((void (*)(void *), void *));

struct pci_bus;
struct pci_device;
struct pci_memwin;

/*
 *  Structure describing a PCI BUS. An entire PCI BUS
 *  topology will be described by several structures
 *  linked together on a linked list.
 */
struct pci_bus {
	struct pci_bus *next;		/* next bus pointer */
	u_int8_t	min_gnt;        /* largest min grant */
	u_int8_t	max_lat;        /* smallest max latency */
	u_int8_t	devsel;         /* slowest devsel */
	u_int8_t	fast_b2b;       /* support fast b2b */
	u_int8_t	prefetch;       /* support prefetch */
	u_int8_t	freq66;         /* support 66MHz */
	u_int8_t	width64;        /* 64 bit bus */
	u_int8_t	bus;
	u_int8_t	ndev;           /* # devices on bus */
	u_int8_t	def_ltim;       /* default ltim counter */
	u_int8_t	max_ltim;       /* maximum ltim counter */
	int32_t		bandwidth;      /* # of .25us ticks/sec @ 33MHz */
	paddr_t		minpcimemaddr;	/* PCI allocation min mem for bus */
	paddr_t		nextpcimemaddr;	/* PCI allocation max mem for bus */
	paddr_t		minpciioaddr;	/* PCI allocation min i/o for bus */
	paddr_t		nextpciioaddr;	/* PCI allocation max i/o for bus */
	paddr_t		pci_mem_base;
	paddr_t		pci_io_base;
};

struct pci_intline_routing;

/* Index values */
#define PCI_INTLINE_A	0
#define PCI_INTLINE_B	1
#define PCI_INTLINE_C	2
#define PCI_INTLINE_D	3

struct pci_intline_routing {
    u_int8_t	bus;
    u_int8_t	device;
    u_int8_t	function;
    u_int8_t	intline[32][4];	/* interrupt line mapping */
    struct pci_intline_routing *next;
};

/*
 * PCI Memory/IO Space
 */
struct pci_win {
	struct pci_win	*next;
	int		reg;
	int		flags;
	vm_size_t	size;
	pcireg_t	address;
	struct pci_device *device;
};

/*
 * PCI Bridge parameters
 */
struct pci_bridge {
	u_int8_t		pribus_num;	/* Primary Bus number */
	u_int8_t		secbus_num;	/* Secondary Bus number */
	u_int8_t		subbus_num;	/* Sub. Bus number */
	struct pci_bus		*secbus;	/* Secondary PCI bus pointer */
	struct pci_device	*child;
	struct pci_win		*memspace;
	struct pci_win		*iospace;
};

/*
 * PCI Device parameters
 */
struct pci_device {
	struct pci_attach_args	pa;
	unsigned char		min_gnt;
	unsigned char		max_lat;
	unsigned char		int_line;
	unsigned char		disable;
	pcireg_t		stat;
	u_int8_t		intr_routing[4];
	struct pci_bridge  	bridge;
	struct pci_bus		*pcibus;
	struct pci_device	*next;
	struct pci_device	*parent;
};

extern struct pci_device *_pci_head;
extern struct pci_bus *_pci_bushead;
extern struct pci_intline_routing *_pci_inthead;
extern int InitInterruptRouting(void);

#endif /* _DEV_PCI_PCIVAR_H_ */
