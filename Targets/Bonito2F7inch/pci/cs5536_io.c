/*
 * cs5536_io.h
 * some basic access of msr read/write and gpio read/write. 
 * this access function only suitable before the virtual support module(VSM)
 * working for some simple debugs.
 *
 * Author : jlliu <liujl@lemote.com>
 * Date : 07-07-04
 *
 */
#include <sys/linux/types.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/nppbreg.h>

#include <machine/bus.h>

#include <include/bonito.h>
#include <include/cs5536_pci.h>
#include <pmon.h>


/******************************************************************************/

/*
 * rdmsr : read 64bits data from the cs5536 MSR register
 */
void _rdmsr(u32 msr, u32 *hi, u32 *lo)
{
	u32 type = 0x00000;
	u32 addr;
	
	addr = (PCI_BUS_CS5536 << 16) | (1 << (PCI_IDSEL_CS5536 + 11) ) | (0 << 8) | 0xf4;
	BONITO_PCICMD |= PCI_STATUS_MASTER_ABORT | PCI_STATUS_MASTER_TARGET_ABORT;
	BONITO_PCIMAP_CFG = (addr >> 16) | type;
	*(volatile pcireg_t *)PHYS_TO_UNCACHED(BONITO_PCICFG_BASE | (addr & 0xfffc)) = msr;
	if (BONITO_PCICMD & PCI_STATUS_MASTER_ABORT) {
		BONITO_PCICMD |= PCI_STATUS_MASTER_ABORT;
    	}
	if (BONITO_PCICMD & PCI_STATUS_MASTER_TARGET_ABORT) {
		BONITO_PCICMD |= PCI_STATUS_MASTER_TARGET_ABORT;
    	}

	addr = (PCI_BUS_CS5536 << 16) | (1 << (PCI_IDSEL_CS5536 + 11) ) | (0 << 8) | 0xf8;
	BONITO_PCICMD |= PCI_STATUS_MASTER_ABORT | PCI_STATUS_MASTER_TARGET_ABORT;
	BONITO_PCIMAP_CFG = (addr >> 16) | type;
	*lo = *(volatile pcireg_t *)PHYS_TO_UNCACHED(BONITO_PCICFG_BASE | (addr & 0xfffc));
	if (BONITO_PCICMD & PCI_STATUS_MASTER_ABORT) {
		BONITO_PCICMD |= PCI_STATUS_MASTER_ABORT;
    	}
	if (BONITO_PCICMD & PCI_STATUS_MASTER_TARGET_ABORT) {
		BONITO_PCICMD |= PCI_STATUS_MASTER_TARGET_ABORT;
    	}

	addr = (PCI_BUS_CS5536 << 16) | (1 << (PCI_IDSEL_CS5536 + 11) ) | (0 << 8) | 0xfc;
	BONITO_PCICMD |= PCI_STATUS_MASTER_ABORT | PCI_STATUS_MASTER_TARGET_ABORT;
	BONITO_PCIMAP_CFG = (addr >> 16) | type;
	*hi = *(volatile pcireg_t *)PHYS_TO_UNCACHED(BONITO_PCICFG_BASE | (addr & 0xfffc));
	if (BONITO_PCICMD & PCI_STATUS_MASTER_ABORT) {
		BONITO_PCICMD |= PCI_STATUS_MASTER_ABORT;
    	}
	if (BONITO_PCICMD & PCI_STATUS_MASTER_TARGET_ABORT) {
		BONITO_PCICMD |= PCI_STATUS_MASTER_TARGET_ABORT;
    	}

	return;	
}

/*
 * wrmsr : write 64bits data to the cs5536 MSR register
 */
void _wrmsr(u32 msr, u32 hi, u32 lo)
{
	u32 type = 0x00000;
	u32 addr;
	
	addr = (PCI_BUS_CS5536 << 16) | (1 << (PCI_IDSEL_CS5536 + 11) ) | (0 << 8) | 0xf4;
	BONITO_PCICMD |= PCI_STATUS_MASTER_ABORT | PCI_STATUS_MASTER_TARGET_ABORT;
	BONITO_PCIMAP_CFG = (addr >> 16) | type;
	*(volatile pcireg_t *)PHYS_TO_UNCACHED(BONITO_PCICFG_BASE | (addr & 0xfffc)) = msr;
	if (BONITO_PCICMD & PCI_STATUS_MASTER_ABORT) {
		BONITO_PCICMD |= PCI_STATUS_MASTER_ABORT;
    	}
	if (BONITO_PCICMD & PCI_STATUS_MASTER_TARGET_ABORT) {
		BONITO_PCICMD |= PCI_STATUS_MASTER_TARGET_ABORT;
    	}

	addr = (PCI_BUS_CS5536 << 16) | (1 << (PCI_IDSEL_CS5536 + 11) ) | (0 << 8) | 0xf8;
	BONITO_PCICMD |= PCI_STATUS_MASTER_ABORT | PCI_STATUS_MASTER_TARGET_ABORT;
	BONITO_PCIMAP_CFG = (addr >> 16) | type;
	*(volatile pcireg_t *)PHYS_TO_UNCACHED(BONITO_PCICFG_BASE | (addr & 0xfffc)) = lo;
	if (BONITO_PCICMD & PCI_STATUS_MASTER_ABORT) {
		BONITO_PCICMD |= PCI_STATUS_MASTER_ABORT;
    	}
	if (BONITO_PCICMD & PCI_STATUS_MASTER_TARGET_ABORT) {
		BONITO_PCICMD |= PCI_STATUS_MASTER_TARGET_ABORT;
    	}


	addr = (PCI_BUS_CS5536 << 16) | (1 << (PCI_IDSEL_CS5536 + 11) ) | (0 << 8) | 0xfc;
	BONITO_PCICMD |= PCI_STATUS_MASTER_ABORT | PCI_STATUS_MASTER_TARGET_ABORT;
	BONITO_PCIMAP_CFG = (addr >> 16) | type;
	*(volatile pcireg_t *)PHYS_TO_UNCACHED(BONITO_PCICFG_BASE | (addr & 0xfffc)) = hi;
	if (BONITO_PCICMD & PCI_STATUS_MASTER_ABORT) {
		BONITO_PCICMD |= PCI_STATUS_MASTER_ABORT;
    	}
	if (BONITO_PCICMD & PCI_STATUS_MASTER_TARGET_ABORT) {
		BONITO_PCICMD |= PCI_STATUS_MASTER_TARGET_ABORT;
    	}


	return;	
}
