/* $OpenBSD: mpcpcibus.c,v 1.16 2000/04/01 15:38:21 rahnds Exp $ */

/*
 * Copyright (c) 1997 Per Fogelstrom
 * Copyright (c) 2001 ipUnplugged AB
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
 *	This product includes software developed under OpenBSD for RTMX Inc
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
 * PCI BUS Bridge driver. Although originally written for the MPC106
 * this code will today handle a various number of bridge types.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <vm/vm.h>

#include <include/autoconf.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <pmon/dev/pcibrvar.h>

int	 pcibrmatch __P((struct device *, void *, void *));
void	 pcibrattach __P((struct device *, struct device *, void *));

void	 mpc_attach_hook __P((struct device *, struct device *,
				struct pcibus_attach_args *));
int	 mpc_bus_maxdevs __P((void *, int));
pcitag_t mpc_make_tag __P((void *, int, int, int));
void	 mpc_decompose_tag __P((void *, pcitag_t, int *, int *, int *));
pcireg_t mpc_conf_read __P((void *, pcitag_t, int));
void	 mpc_conf_write __P((void *, pcitag_t, int, pcireg_t));

int      mpc_intr_map __P((void *, pcitag_t, int, int, pci_intr_handle_t *));
const char *mpc_intr_string __P((void *, pci_intr_handle_t));
void     *mpc_intr_establish __P((void *, pci_intr_handle_t,
            int, int (*func)(void *), void *, char *));
void     mpc_intr_disestablish __P((void *, void *));

struct cfattach pcibr_ca = {
        sizeof(struct pcibr_softc), pcibrmatch, pcibrattach,
};

struct cfdriver pcibr_cd = {
	NULL, "pcibr", DV_DULL,
};

static int      pcibrprint __P((void *, const char *pnp));
extern struct tgt_bus_space def_bus_iot;
extern struct tgt_bus_space def_bus_memt;
extern int monarch_mode;

int
pcibrmatch(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct confargs *ca = aux;
	int found = 0;

	if (!monarch_mode)
		return 0;

	if (strcmp(ca->ca_name, pcibr_cd.cd_name) != 0)
		return (found);

	found = 1;

	return found;
}

void
pcibrattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pcibr_softc *sc = (struct pcibr_softc *)self;
        /*	struct pcibr_config *lcp; */
	struct pcibus_attach_args pba;

        printf("\n");

	/*
	 *  Generic.
	 */
#if defined(NEWPCIROOT)
        sc->sc_iobus_space = *_pci_bus[sc->sc_dev.dv_unit]->pa.pa_iot;
	sc->sc_membus_space = *_pci_bus[sc->sc_dev.dv_unit]->pa.pa_memt;
	sc->sc_dmatag = *_pci_bus[sc->sc_dev.dv_unit]->pa.pa_dmat;;
#else
        sc->sc_iobus_space = def_bus_iot;
	sc->sc_membus_space = def_bus_memt;
	sc->sc_dmatag = bus_dmamap_tag;;
#endif

#if 0
	char *bridge;


        lcp = sc->sc_pcibr;

        lcp->lc_pc.pc_conf_v = lcp;
        lcp->lc_pc.pc_attach_hook = mpc_attach_hook;
        lcp->lc_pc.pc_bus_maxdevs = mpc_bus_maxdevs;
        lcp->lc_pc.pc_make_tag = mpc_make_tag;
        lcp->lc_pc.pc_decompose_tag = mpc_decompose_tag;
        lcp->lc_pc.pc_conf_read = mpc_conf_read;
        lcp->lc_pc.pc_conf_write = mpc_conf_write;
        lcp->lc_pc.pc_ether_hw_addr = NULL;
        lcp->lc_iot = &sc->sc_iobus_space;
        lcp->lc_memt = &sc->sc_membus_space;
	lcp->lc_dmat = &sc->sc_dmatag;

        lcp->lc_pc.pc_intr_v = lcp;
        lcp->lc_pc.pc_intr_map = mpc_intr_map;
        lcp->lc_pc.pc_intr_string = mpc_intr_string;
        lcp->lc_pc.pc_intr_establish = NULL;
        lcp->lc_pc.pc_intr_disestablish = NULL;
#endif        

	pba.pba_busname = "pci";
	pba.pba_iot = &sc->sc_iobus_space;
	pba.pba_memt = &sc->sc_membus_space;
	pba.pba_dmat = &sc->sc_dmatag;
	pba.pba_pc = NULL;
	pba.pba_bus = sc->sc_dev.dv_unit;
	config_found(self, &pba, pcibrprint);
}

static int
pcibrprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct pcibus_attach_args *pba = aux;

	if(pnp)
		printf("%s at %s", pba->pba_busname, pnp);
	printf(" bus %d", pba->pba_bus);
	return(UNCONF);
}

