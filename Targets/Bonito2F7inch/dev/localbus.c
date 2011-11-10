/*	$Id: localbus.c,v 1.1.1.1 2006/06/29 06:43:25 cpu Exp $ */

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

