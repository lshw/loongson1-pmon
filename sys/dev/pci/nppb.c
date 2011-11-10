/*      $Id: nppb.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $     */

/*
 * Copyright (c) 2000 Per Fogelstrom, Opsycon AB  (www.opsycon.se)
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
 *	This product includes software developed by
 *	Per Fogelstrom, Opsycon AB, Sweden.
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
 * PCI bridge 2155x 'driver'.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/nppbreg.h>

extern int _pciverbose;

struct nppb_softc {
	struct device	sc_dev;
	pci_chipset_tag_t sc_pc;
	bus_space_tag_t	sc_iot;
	bus_space_handle_t sc_ioh;
	pcitag_t	sc_tag;
};


int	nppb_match __P((struct device *, void *, void *));
void	nppb_attach __P((struct device *, struct device *, void *));

struct cfattach nppb_ca = {
	sizeof(struct nppb_softc), nppb_match, nppb_attach
};

struct        cfdriver nppb_cd = {
      NULL, "nppb", DV_DULL
};

int
nppb_match(parent, match, aux)
	struct device *parent;
	void *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_DEC &&
	    PCI_PRODUCT(pa->pa_id) == 0x0046)
		return (1);

	return (0);
}

void
nppb_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pci_attach_args *pa = aux;
	struct nppb_softc *sc = (struct nppb_softc *)self;
	u_int32_t ioaddr;
	size_t iosize;
	pcitag_t tag;
	pcireg_t rval;

	sc->sc_pc = pa->pa_pc;
	sc->sc_iot = pa->pa_iot;
	sc->sc_tag = tag = pa->pa_tag;

	if(pci_io_find(pa->pa_pc, pa->pa_tag, 0x14, &ioaddr, &iosize) != 0) {
		printf(": can't find i/o space\n");
		return;
	}

	if(bus_space_map(pa->pa_iot, ioaddr, iosize, 0, &sc->sc_ioh) != 0) {
		printf(": couldn't map i/o space\n");
		return;
	}

#ifdef PMON
	/*
	 * Set up local to cPCI translations. We usually deal with
	 * 256Mb windows to PCI and make the assumption that we can
	 * map parts of it to the cPCI space.
	 */

	/* Upstream Non prefetchable and I/O to cPCI bus maps 1:1 */
	_pci_conf_write(tag, cfgoffs(ds.up_mem0_tran), PCI_BRIDGE_IO_BASE);
	_pci_conf_write(tag, cfgoffs(ds.up_mem1_tran), PCI_BRIDGE_MEM_BASE);


	/* Downstream Non prefetchable to RAM */
	_pci_conf_write(tag, cfgoffs(ds.down_mem2_tran), 0);

	/* Downstream Prefetchable to RAM */
	_pci_conf_write(tag, cfgoffs(ds.down_mem3_tran), PCI_TO_LOCAL_RAM_BASE);

	/* Downstream I/O to PCI bus */
	_pci_conf_write(tag, cfgoffs(ds.down_mem1_tran), PCI_TO_LOCAL_RAM_BASE);

	/* Enable Upstream BAR's */
	rval = -PCI_BRIDGE_IO_SIZE | PCI_MAPREG_TYPE_IO;
	_pci_conf_write(tag, cfgoffs(ds.up_mem0_setup), rval);
	rval = -PCI_BRIDGE_MEM_SIZE | PCI_MAPREG_TYPE_MEM;
	_pci_conf_write(tag, cfgoffs(ds.up_mem1_setup), rval);

	/* Enable Downstream BAR's */
	rval = -cPCI_BRIDGE_IO_SIZE | PCI_MAPREG_TYPE_IO;
	_pci_conf_write(tag, cfgoffs(ds.down_mem1_setup), rval);
	rval = -cPCI_BRIDGE_MEM_SIZE | PCI_MAPREG_TYPE_MEM;
	_pci_conf_write(tag, cfgoffs(ds.down_mem2_setup), rval);
	rval = -cPCI_BRIDGE_MEM_SIZE | PCI_MAPREG_TYPE_MEM;
	_pci_conf_write(tag, cfgoffs(ds.down_mem3_setup), rval);

	/* Map cPCI upstream */
	_pci_conf_write(tag, cfgoffs(local.up_mem0_bar), PCI_BRIDGE_IO_BASE);
	_pci_conf_write(tag, cfgoffs(local.up_mem1_bar), PCI_BRIDGE_MEM_BASE);
#ifdef PCI_BRIDGE_PMEM_BASE
	_pci_conf_write(tag, cfgoffs(local.up_mem2_bar), PCI_BRIDGE_PMEM_BASE);
#endif

	/* Map cPCI downstream */
	_pci_conf_write(tag, cfgoffs(sub.down_mem1_bar), cPCI_BRIDGE_IO_BASE);
	_pci_conf_write(tag, cfgoffs(sub.down_mem2_bar), cPCI_BRIDGE_MEM_BASE);
	_pci_conf_write(tag, cfgoffs(sub.down_mem3_bar), cPCI_BRIDGE_PMEM_BASE);

	/* Clear lock */
	rval = _pci_conf_read(tag, cfgoffs(ds.chip_control));
	rval &=  ~PRIM_LOCKOUT;
	_pci_conf_write(tag, cfgoffs(ds.chip_control), rval);

	/* Enable cPCI side */
	rval = _pci_conf_read(tag, cfgoffs(sub.command));
	rval |= PCI_COMMAND_MASTER_ENABLE | PCI_COMMAND_IO_ENABLE |
		PCI_COMMAND_MEM_ENABLE | PCI_COMMAND_PARITY_ENABLE |
		PCI_COMMAND_SERR_ENABLE | PCI_COMMAND_BACKTOBACK_ENABLE;
	_pci_conf_write(tag, cfgoffs(sub.command), rval);

	/* Clear lock */
	rval = _pci_conf_read(tag, cfgoffs(ds.chip_control));
	rval &=  ~PRIM_LOCKOUT;
	_pci_conf_write(tag, cfgoffs(ds.chip_control), rval);
#endif

	printf(": PCI-PCI bridge, non-transparent");

	printf("\n");

}


pcireg_t
nppb_config_read(tag, reg)
	pcitag_t tag;
	int reg;
{
	int bus, dev, func, adr;
	pcireg_t rval, cval;
	struct nppb_softc *sc = nppb_cd.cd_devs[0];

	if(reg & 3) {
		return(-1);
	}

	pci_break_tag(tag, &bus, &dev, &func);

	if(func < 0 || func > 7 ||
	   reg < 0 || reg > 255) {
		return(-1);
	}

	if(bus == 1 && dev >= 0 && dev < 15) {
		/* Do config type 0 cycles */
		adr = 0x10000 << dev |  func << 8 | reg;
	}
	else if(bus > 1 && bus < 256 && dev >= 0 && dev < 32) {
		/* Do config type 1 cycles */
		adr = pci_make_tag(bus - 1, dev, func) | reg | 1;
	}
	else {
		return(-1);
	}

	/* Turn off master abort == error */
	rval = _pci_conf_read(sc->sc_tag, cfgoffs(ds.chip_control));
	_pci_conf_write(sc->sc_tag, cfgoffs(ds.chip_control), rval & ~1);

	bus_space_read_2(sc->sc_iot, sc->sc_ioh, csroffs(cnfg_own) + 2);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, csroffs(up_addr), adr);
	cval = bus_space_read_4(sc->sc_iot, sc->sc_ioh, csroffs(up_data));

	_pci_conf_write(sc->sc_tag, cfgoffs(ds.chip_control), rval);

	return(cval);
}

int
nppb_config_write(tag, reg, data)
	pcitag_t tag;
	int reg;
	pcireg_t data;
{
	int bus, dev, func, adr;
	pcireg_t rval;
	struct nppb_softc *sc = nppb_cd.cd_devs[0];

	if(reg & 3) {
		return(-1);
	}

	pci_break_tag(tag, &bus, &dev, &func);

	if(func < 0 || func > 7 ||
	   reg < 0 || reg > 255) {
		return(-1);
	}

	if(bus == 1 && dev >= 0 && dev < 15) {
		/* Do config type 0 cycles */
		adr = 0x10000 << dev |  func << 8 | reg;
	}
	else if(bus > 1 && bus < 256 && dev >= 0 && dev < 32) {
		/* Do config type 1 cycles */
		adr = pci_make_tag(bus - 1, dev, func) | reg | 1;
	}
	else {
		return(-1);
	}

	/* Turn off master abort == error */
	rval = _pci_conf_read(sc->sc_tag, cfgoffs(ds.chip_control));
	_pci_conf_write(sc->sc_tag, cfgoffs(ds.chip_control), rval & ~1);

	bus_space_read_2(sc->sc_iot, sc->sc_ioh, csroffs(cnfg_own) + 2);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, csroffs(up_addr), adr);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, csroffs(up_data), data);

	_pci_conf_write(sc->sc_tag, cfgoffs(ds.chip_control), rval);

	return(0);
}

int nppbscan __P((int));

int nppbscan(ptr)
	int ptr;
{
	int i,j,k;

	for(j = 0; j < 255; j++) {
		for(i = 0; i < 32; i++) {
			k = nppb_config_read((j << 16) | (i << 11), 0);
			if(k != -1 && k != -2) {
				printf("bus %d dev %d -> %p\n", j, i, k);
			}
		}
	}
	return(0);
}


#ifdef PMON
/*
 *  This code is called from low level setup before device
 *  initialisation but after scan of PCI BUS 0.
 *
 *  The table below deals with the serial rom initialization such that
 *  if we have no serial rom, the 21554 is correctly setup anyway.
 */

struct nppb_init {
	u_int32_t csroffs;
	u_int32_t value;
	u_int32_t mask;
};

struct nppb_init nppb_devspec[] = {
	{ cfgoffs(ds.cnfg_own),		0x02020000, 0xfcfcfefe },
	{ cfgoffs(ds.down_mem0_tran),	0x00000000, 0x00000fff },
	{ cfgoffs(ds.down_mem1_tran),	0x00000000, 0x0000003f },
	{ cfgoffs(ds.down_mem2_tran),	0x00000000, 0x00000fff },
	{ cfgoffs(ds.down_mem3_tran),	0x00000000, 0x00000fff },
	{ cfgoffs(ds.up_mem0_tran),	0x00000000, 0x0000003f },
	{ cfgoffs(ds.up_mem1_tran),	0x00000000, 0x0000003f },
	{ cfgoffs(ds.down_mem0_setup),	0x00000000, 0x00000ff0 },
	{ cfgoffs(ds.down_mem1_setup),	0x00000000, 0x00000030 },
	{ cfgoffs(ds.down_mem2_setup),	0x00000000, 0x00000ff0 },
	{ cfgoffs(ds.down_mem3_setup),	0x00000000, 0x00000ff0 },
	{ cfgoffs(ds.down_upper32),	0x00000000, 0x00000000 },
	{ cfgoffs(ds.pri_exp_rom),	0x00000000, 0xfe000fff },
	{ cfgoffs(ds.up_mem0_setup),	0x00000000, 0x00000030 },
	{ cfgoffs(ds.up_mem1_setup),	0x00000000, 0x00000ff0 },
	{ cfgoffs(ds.chip_control),	0x003c0481, 0x00003000 },
	{ cfgoffs(ds.chip_status),	0x0200f0f0, 0xfc00f0f0 },
	{ cfgoffs(ds.serr_disables),	0x00000000, 0xffff8080 },
	{ cfgoffs(ds.reset_control),	0x00000000, 0xfffffff8 },
	{ cfgoffs(ds.power_cap),	0x00000000, 0xf9c00000 },
	{ cfgoffs(ds.power_csr),	0x00000000, 0x00000000 },
	{ cfgoffs(ds.vpd_addr),		0x00000000, 0x7e000000 },
	{ cfgoffs(ds.vpd_data),		0x00000000, 0x00000000 },
	{ cfgoffs(ds.hot_swap),		0x00c00000, 0xff350000 },
};
#define nppb_devspec_size (sizeof(nppb_devspec) / sizeof(struct nppb_init))

void
_nppb_pmon_init(tag)
	pcitag_t tag;
{
	int i;
	pcireg_t rval;

	for(i = 0; i < nppb_devspec_size; i++) {
		rval = _pci_conf_read(tag, nppb_devspec[i].csroffs);
		rval &= nppb_devspec[i].mask;
		rval |= nppb_devspec[i].value;
		_pci_conf_write(tag, nppb_devspec[i].csroffs, rval);
	}
	rval = _pci_conf_read(tag, cfgoffs(sub.command));
	rval |= PCI_COMMAND_MASTER_ENABLE;
	_pci_conf_write(tag, cfgoffs(sub.command), rval);
}

pcireg_t
_nppb_config_read(btag, tag, reg)
	pcitag_t btag;
	pcitag_t tag;
	int reg;
{
	int bus, dev, func, adr;
	pcireg_t rval, cval;

	if(reg & 3) {
		return(-1);
	}

	pci_break_tag(tag, &bus, &dev, &func);

	/* This really never happen with the limits we use, but keep in */
	if(func < 0 || func > 7 ||
	   reg < 0 || reg > 255) {
		return(-1);
	}

	if(bus == 1 && dev >= 0 && dev < 15) {
		/* Do config type 0 cycles */
		adr = 0x10000 << dev |  func << 8 | reg;
	}
	else if(bus > 1 && bus < 256 && dev >= 0 && dev < 32) {
		/* Do config type 1 cycles */
		adr = pci_make_tag(bus - 1, dev, func) | reg | 1;
	}
	else {
		return(-1);
	}

	/* Turn off master abort == error */
	rval = _pci_conf_read(btag, cfgoffs(ds.chip_control));
	_pci_conf_write(btag, cfgoffs(ds.chip_control), rval & ~1);
	/* Reset bus error status */
	cval = _pci_conf_read(btag, cfgoffs(sub.command));
	_pci_conf_write(btag, cfgoffs(sub.command), cval);

	(void)_pci_conf_read(btag, cfgoffs(ds.cnfg_own));	/* Semaphore */

	_pci_conf_write(btag, cfgoffs(ds.up_addr), adr);
	cval = _pci_conf_read(btag, cfgoffs(ds.up_data));

	_pci_conf_write(btag, cfgoffs(ds.chip_control), rval);

	/* Check for bus errors */
	rval = _pci_conf_read(btag, cfgoffs(sub.command));
	if(rval & (PCI_STATUS_TARGET_TARGET_ABORT |
		   PCI_STATUS_MASTER_TARGET_ABORT | PCI_STATUS_MASTER_ABORT |
		   PCI_STATUS_SPECIAL_ERROR | PCI_STATUS_PARITY_DETECT)) {
		cval = ~0;	/* Error detected */
	}

	return(cval);
}

int
_nppb_config_write(btag, tag, reg, data)
	pcitag_t btag;
	pcitag_t tag;
	int reg;
	pcireg_t data;
{
	int bus, dev, func, adr;
	pcireg_t rval;

	if(reg & 3) {
		return(-1);
	}

	pci_break_tag(tag, &bus, &dev, &func);

	/* This really never happen with the limits we use, but keep in */
	if(func < 0 || func > 7 ||
	   reg < 0 || reg > 255) {
		return(-1);
	}

	if(bus == 1 && dev >= 0 && dev < 15) {
		/* Do config type 0 cycles */
		adr = 0x10000 << dev |  func << 8 | reg;
	}
	else if(bus > 1 && bus < 256 && dev >= 0 && dev < 32) {
		/* Do config type 1 cycles */
		adr = pci_make_tag(bus - 1, dev, func) | reg | 1;
	}
	else {
		return(-1);
	}

	/* Turn off master abort == error */
	rval = _pci_conf_read(btag, cfgoffs(ds.chip_control));
	_pci_conf_write(btag, cfgoffs(ds.chip_control), rval & ~1);

	(void)_pci_conf_read(btag, cfgoffs(ds.cnfg_own));	/* Semaphore */

	_pci_conf_write(btag, cfgoffs(ds.up_addr), adr);
	_pci_conf_write(btag, cfgoffs(ds.up_data), data);

	_pci_conf_write(btag, cfgoffs(ds.chip_control), rval);

	return(0);
}
#endif
