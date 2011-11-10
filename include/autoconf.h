/*	$OpenBSD: autoconf.h,v 1.6 1999/11/09 04:13:54 rahnds Exp $ */

/*
 * Copyright (c) 1997 Per Fogelstrom
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
 *	This product includes software developed under OpenBSD for RTMX inc
 *      by Per Fogelstrom, Opsycon AB.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/*
 * Machine-dependent structures of autoconfiguration
 */

#ifndef _MACHINE_AUTOCONF_H_
#define _MACHINE_AUTOCONF_H_

#include <machine/bus.h>
#if 0
/*
 *   System types.
 */
#define OFWMACH         0       /* Openfirmware drivers */
#define	POWER4e		1	/* V.I Power.4e board */
#define	PWRSTK		2	/* Motorola Powerstack series */
#define	APPL		3	/* Apple PowerMac machines (OFW?) */
#define	PALOMARII	4
#define	DENALI		5
#define	K2		6

struct sys_rec {
	int	system_type;
	struct ppc_bus_space local;
	struct ppc_bus_space isa_io;
	struct ppc_bus_space isa_mem;
	struct ppc_bus_space pci_io;
	struct ppc_bus_space pci_mem;

	int	cons_baudclk;
	int	cons_ioaddr[4];
};

extern struct sys_rec sys_config;
#endif
/**/
struct confargs;

typedef int (*intr_handler_t) __P((void *));

typedef struct bushook {
	struct	device *bh_dv;
	int	bh_type;
	void	*(*bh_intr_establish) __P((void *, u_long, int, int,
			int (*)(void *), void *, char *));
	void	(*bh_intr_disestablish) __P((void *, void *));
	int	(*bh_matchname)	__P((struct confargs *, char *));
} bushook_t;

#define	BUS_MAIN	1		/* mainbus */
#define	BUS_ISABR	2		/* ISA Bridge Bus */
#define	BUS_PCIBR	3		/* PCI bridge */
#define	BUS_VMEBR	4		/* VME bridge */
#define	BUS_LOCAL	5		/* VME bridge */

#define	BUS_INTR_ESTABLISH(ca, a, b, c, d, e, f, h)			\
	    (*(ca)->ca_bus->bh_intr_establish)((a),(b),(c),(d),(e),(f),(h))
#define	BUS_INTR_DISESTABLISH(ca)					\
	    (*(ca)->ca_bus->bh_intr_establish)(ca)
#define	BUS_MATCHNAME(ca, name)						\
	    (((ca)->ca_bus->bh_matchname) ?				\
	    (*(ca)->ca_bus->bh_matchname)((ca), (name)) :		\
	    -1)

struct confargs {
	char	*ca_name;		/* Device name. */
	bushook_t *ca_bus;		/* bus device resides on. */
	bus_space_tag_t ca_iot;
	bus_space_tag_t ca_memt; /* XXX */
	u_int32_t ca_node;
	int ca_nreg;
	u_int32_t *ca_reg;
	int ca_nintr;
	int32_t *ca_intr;
	u_int ca_baseaddr;

};

void	set_clockintr __P((void (*)(struct clockframe *)));
void	set_iointr __P((void (*)(void *, int)));

void	configure __P((void));

#endif /* _MACHINE_AUTOCONF_H_ */
