/* $id$ */
/*
 * Copyright (c) 2001 Patrik Lindergren
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
 *	This product includes software developed by the Patrik Lindergren
 *	at IP Unplugged AB.
 * 4. Neither the name of the company nor the names of its contributors
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

#include <sys/param.h>
#include <sys/device.h>
#include <stdio.h>
#include <include/autoconf.h>

struct mainbus_softc {
	struct	device sc_dv;
	struct	bushook sc_bus;
};

/* Definition of the mainbus driver. */
static int mainbus_match (struct device *parent, void *match, void *aux);
static void mainbus_attach (struct device *parent, struct device *self, void *aux);
static int mbprint (void *, const char *);
static int mb_matchname(struct confargs *ca, char *name);

struct cfattach mainbus_ca = {
    sizeof(struct mainbus_softc), mainbus_match, mainbus_attach
};

struct cfdriver mainbus_cd = {
    NULL, "mainbus", DV_DULL, NULL, 0
};

static int
mainbus_match(parent, match, aux)
	struct device *parent;
	void *match;
	void *aux;
{
	/*
	 * That one mainbus is always here.
	 */
	return (1);
}

extern int _max_pci_bus;
static void
mainbus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct mainbus_softc *sc = (struct mainbus_softc *)self;
	struct confargs nca;
	int i;

	printf("\n");
	sc->sc_bus.bh_dv = (struct device *)sc;
	sc->sc_bus.bh_type = BUS_MAIN;
	sc->sc_bus.bh_intr_establish = NULL;
	sc->sc_bus.bh_intr_disestablish = NULL;
	sc->sc_bus.bh_matchname = mb_matchname;
        
        nca.ca_node = NULL;
        nca.ca_name = "localbus";
        nca.ca_bus = &sc->sc_bus;
        config_found(self, &nca, mbprint);
		
        nca.ca_node = NULL;
        nca.ca_name = "fd";
        nca.ca_bus = &sc->sc_bus;
        config_found(self, &nca, mbprint);

        nca.ca_node = NULL;
        nca.ca_name = "loopdev";
        nca.ca_bus = &sc->sc_bus;
        config_found(self, &nca, mbprint);

#ifndef NOPCI
	for(i=0;i<_max_pci_bus;i++)
	{
        nca.ca_node = NULL;
        nca.ca_name = "pcibr";
        nca.ca_bus = &sc->sc_bus;
        config_found(self, &nca, mbprint);
	}
#endif
}

static int
mbprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	if (pnp)
		return (QUIET);
	return (UNCONF);
}

static int
mb_matchname(ca, name)
	struct confargs *ca;
	char *name;
{
	return (strcmp(name, ca->ca_name) == 0);
}
