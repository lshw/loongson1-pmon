/*	$Id: localbus.c,v 1.1.1.1 2006/09/14 01:59:09 root Exp $ */

/*
 * Copyright (c) 2001 Opsycon AB  (www.opsycon.se)
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
 *	This product includes software developed by Opsycon AB, Sweden.
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
 *  This is the localbus driver. It handles configuration of all
 *  devices on the Discovery localbus.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/tty.h>

#include <autoconf.h>

#include <pmon/dev/gt64240reg.h>

int localbusmatch __P((struct device *, void *, void *));
void localbusattach __P((struct device *, struct device *, void *));
int localbusprint __P((void *, const char *));
void localbusscan __P((struct device *, void *));


struct cfattach localbus_ca = {
	sizeof(struct device), localbusmatch, localbusattach
};

struct cfdriver localbus_cd = {
	NULL, "localbus", DV_DULL, 1
};

int
localbusmatch(parent, match, aux)
	struct device *parent;
	void *match;
	void *aux;
{
	return (1);
}

int
localbusprint(aux, localbus)
	void *aux;
	const char *localbus;
{
/* XXXX print flags */
	return (QUIET);
}


void
localbusscan(parent, match)
	struct device *parent;
	void *match;
{
	struct device *dev = match;
	struct cfdata *cf = dev->dv_cfdata;
	struct confargs lba;
	bushook_t lbus;

	if (cf->cf_fstate == FSTATE_STAR) {
		printf("localbus '*' devs not allowed!\n");
		free(dev, M_DEVBUF);
	}

	if (cf->cf_loc[0] == -1) {
		lba.ca_baseaddr = 0;
		lba.ca_intr = 0;
		lba.ca_nintr = 0;
	} else {    
		lba.ca_baseaddr = cf->cf_loc[0];
		lba.ca_nintr= cf->cf_loc[1];
		lba.ca_intr = 0;
	}

	lba.ca_bus = &lbus;
	lba.ca_bus->bh_type = BUS_LOCAL;
	lba.ca_bus->bh_matchname = NULL;

#if 0
	lba.ca_iot = &sys_config.local;
	lba.ca_memt = &sys_config.local;
#endif


	if ((*cf->cf_attach->ca_match)(parent, dev, &lba) > 0) {
		config_attach(parent, dev, &lba, localbusprint);
	}
	else {
		free(dev, M_DEVBUF);
	}
}

void
localbusattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	printf("\n");

	config_scan(localbusscan, self);
}

#if 0

void
gt64240_map()
{
	u_int32_t rval;

static int gt64240_initialized;

	if(!gt64240_initialized) {
		/*
		 *  Map GT register space. We map the entire chip at
		 *  once since functions are scattered all over the
		 *  address space and mapping functionwise would
		 *  require a lot of small mappings.
		 *  The objects gt64240_bt and gt64240_bh can
		 *  be used by all functions require access to the GT.
		 */
		if(bus_space_map(&sys_config.local, GT_REG(0), 65536,
				0, &gt64240_bh) != 0 ) {
				panic("localbus: unable to map GT64240 regs\n");
		}
		gt64240_bt = &sys_config.local;
		gt64240_initialized = 1;

		/*
		 *  Kill any active ethernet channels.
		 */
		GT_WRITE(ETHERNET0_PORT_CONFIGURATION_REGISTER, 0);
		GT_WRITE(ETHERNET1_PORT_CONFIGURATION_REGISTER, 0);
		GT_WRITE(ETHERNET2_PORT_CONFIGURATION_REGISTER, 0);
		GT_WRITE(ETHERNET0_SDMA_COMMAND_REGISTER, 0);
		GT_WRITE(ETHERNET1_SDMA_COMMAND_REGISTER, 0);
		GT_WRITE(ETHERNET2_SDMA_COMMAND_REGISTER, 0);

		/*
		 *  Initialize the interrupt controller. Route MPP pins.
		 */
		rval = GT_READ(MPP_CONTROL3);
		rval &= 0xff0f00ff;	/* Reset bits 29 and 27 */
		GT_WRITE(MPP_CONTROL3, rval);

		rval = GT_READ(MPP_CONTROL2);
		rval &= 0xf00fffff;	/* Reset bits 22 and 21 */
		GT_WRITE(MPP_CONTROL2, rval);

		rval = GT_READ(GPP_LEVEL_CONTROL);
		rval |= 0x28600000;	/* Active low transition == int */
		GT_WRITE(GPP_LEVEL_CONTROL, rval);

		rval = GT_READ(GPP_INTERRUPT_MASK);
		rval |= 0x28600000;	/* Enable PCI + Serial */
		GT_WRITE(GPP_INTERRUPT_MASK, rval);
		/* Clear all pending ints. */
		GT_WRITE(GPP_INTERRUPT_CAUSE, 0);
	}
	return;
}
#endif
