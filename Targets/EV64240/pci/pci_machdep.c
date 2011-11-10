/*	$Id: pci_machdep.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <sys/malloc.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/nppbreg.h>

#include <machine/bus.h>

#include "include/ev64240.h"
#include "pmon/dev/gt64240reg.h"

#include <pmon.h>

extern void *pmalloc __P((size_t ));

/* PCI i/o regions in PCI space */
#define PCI_IO_SPACE_PCI_BASE	0x00000000

/* PCI mem regions in PCI space */
#define PCI_LOCAL_MEM_PCI_BASE	0x00000000  /* CPU Mem accessed from PCI */

/* soft versions of above */
static pcireg_t pci_local_mem_pci_base;

pcireg_t _pci_conf_readn __P((pcitag_t, int, int));
void _pci_conf_writen __P((pcitag_t, int, pcireg_t, int));
extern int _pciverbose;

extern char hwethadr[6];

struct pci_device *_pci_bus[16];
int _max_pci_bus = 0;

#define PCI_BAR_ENABLE_DEFAULT	0xfffffc00

struct bartab {
	int	scslow,	scshigh;
	int	remap0, remap1;
	int	pci0size, pci1size;
	int	bar0, enablemask;
} barlist[] = {
{ SCS_0_LOW_DECODE_ADDRESS, SCS_0_HIGH_DECODE_ADDRESS,
  PCI_0SCS_0_BASE_ADDRESS_REMAP, PCI_1SCS_0_BASE_ADDRESS_REMAP,
  PCI_0SCS_0_BANK_SIZE, PCI_1SCS_0_BANK_SIZE,
  PCI_SCS_0_BASE_ADDRESS, 0x1 },
{ SCS_1_LOW_DECODE_ADDRESS, SCS_1_HIGH_DECODE_ADDRESS,
  PCI_0SCS_1_BASE_ADDRESS_REMAP, PCI_1SCS_1_BASE_ADDRESS_REMAP,
  PCI_0SCS_1_BANK_SIZE, PCI_1SCS_1_BANK_SIZE,
  PCI_SCS_1_BASE_ADDRESS, 0x2 },
{ SCS_2_LOW_DECODE_ADDRESS, SCS_2_HIGH_DECODE_ADDRESS,
  PCI_0SCS_2_BASE_ADDRESS_REMAP, PCI_1SCS_2_BASE_ADDRESS_REMAP,
  PCI_0SCS_2_BANK_SIZE, PCI_1SCS_2_BANK_SIZE,
  PCI_SCS_2_BASE_ADDRESS, 0x4 },
{ SCS_3_LOW_DECODE_ADDRESS, SCS_3_HIGH_DECODE_ADDRESS, 
  PCI_0SCS_3_BASE_ADDRESS_REMAP, PCI_1SCS_3_BASE_ADDRESS_REMAP,
  PCI_0SCS_3_BANK_SIZE, PCI_1SCS_3_BANK_SIZE,
  PCI_SCS_3_BASE_ADDRESS, 0x8 },
{ CS_0_LOW_DECODE_ADDRESS, CS_0_HIGH_DECODE_ADDRESS,
  0, 0,
  PCI_0CS_0_BANK_SIZE, PCI_1CS_0_BANK_SIZE,
  PCI_CS_0_BASE_ADDRESS, 0x10 },
{ CS_1_LOW_DECODE_ADDRESS, CS_1_HIGH_DECODE_ADDRESS,
  0, 0,
  PCI_0CS_1_BANK_SIZE, PCI_1CS_1_BANK_SIZE,
  PCI_CS_1_BASE_ADDRESS, 0x20 },
{ CS_2_LOW_DECODE_ADDRESS, CS_2_HIGH_DECODE_ADDRESS,
  0, 0,
  PCI_0CS_2_BANK_SIZE, PCI_1CS_2_BANK_SIZE,
  PCI_CS_2_BASE_ADDRESS, 0x40 },
{ CS_3_LOW_DECODE_ADDRESS, CS_3_HIGH_DECODE_ADDRESS,
  0, 0,
  PCI_0CS_3_BANK_SIZE, PCI_1CS_3_BANK_SIZE,
  PCI_CS_3_BASE_ADDRESS, 0x80 },
{ BOOTCS_LOW_DECODE_ADDRESS, BOOTCS_HIGH_DECODE_ADDRESS,
  0, 0,
  PCI_0CS_BOOT_BANK_SIZE, PCI_1CS_BOOT_BANK_SIZE,
  PCI_BOOTCS_BASE_ADDRESS, 0x100 },
};

#define NBARS (sizeof(barlist) / sizeof(struct bartab))

/*
 * Called to initialise the bridge at the beginning of time
 */
int
_pci_hwinit (initialise, iot, memt)
	int initialise;
	bus_space_tag_t iot;
	bus_space_tag_t memt;
{
	pcireg_t stat;
	struct pci_device *pd;
	struct pci_bus *pb;
	int i;
	
	/*
	 *  Where local memory starts seen from PCI.
	 */
	pci_local_mem_pci_base = PCI_LOCAL_MEM_PCI_BASE;

	if (!initialise) {
		return(0);
	}

	/*
	 *  Allocate and initialize PCI bus heads.
	 */

	/*
	 * PCI Bus 0
	 */
	pd = pmalloc(sizeof(struct pci_device));
	pb = pmalloc(sizeof(struct pci_bus));
	if(pd == NULL || pb == NULL) {
		printf("pci: can't alloc memory. pci not initialized\n");
		return(-1);
	}

	pd->pa.pa_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;
	pd->pa.pa_iot = pmalloc(sizeof(bus_space_tag_t));
	pd->pa.pa_iot->bus_reverse = 1;
#if 1 
	pd->pa.pa_iot->bus_base = PCI0_IO_SPACE_BASE - PCI0P_IO_SPACE_BASE;
#else
	pd->pa.pa_iot->bus_base = 0;
#endif
	pd->pa.pa_memt = pmalloc(sizeof(bus_space_tag_t));
	pd->pa.pa_memt->bus_reverse = 1;
#if 0
	pd->pa.pa_memt->bus_base = PCI0_MEM_SPACE_BASE - PCI0P_MEM_SPACE_BASE;
#endif
	pd->pa.pa_memt->bus_base = 0;
	pd->pa.pa_dmat = &bus_dmamap_tag;
	pd->bridge.secbus = pb;
	_pci_head = pd;

	pb->minpcimemaddr  = PCI0P_MEM_SPACE_BASE;
	pb->nextpcimemaddr = PCI0P_MEM_SPACE_BASE + PCI0_MEM_SPACE_SIZE;
	pb->minpciioaddr  = PCI0P_IO_SPACE_BASE;
	pb->nextpciioaddr = PCI0P_IO_SPACE_BASE + PCI0_IO_SPACE_SIZE;
	pb->pci_mem_base   = PCI0_MEM_SPACE_BASE;
	pb->pci_io_base    = PCI0_IO_SPACE_BASE;
	pb->max_lat = 255;
	pb->fast_b2b = 1;
	pb->prefetch = 1;
	pb->bandwidth = 4000000;
	pb->ndev = 1;
	_pci_bushead = pb;
	_pci_bus[_max_pci_bus++] = pd;

	/*
	 * PCI Bus 1
	 */
	pd = pmalloc(sizeof(struct pci_device));
	pb = pmalloc(sizeof(struct pci_bus));
	if(pd == NULL || pb == NULL) {
		printf("pci: can't alloc memory. pci 1 not initialized\n");
		return(-1);
	}

	*pd = *_pci_head;
	pd->pa.pa_iot = pmalloc(sizeof(bus_space_tag_t));
	pd->pa.pa_iot->bus_reverse = 1;
#if 1
	pd->pa.pa_iot->bus_base = PCI1_IO_SPACE_BASE - PCI1P_IO_SPACE_BASE;
#else
	pd->pa.pa_iot->bus_base = 0;
#endif
	pd->pa.pa_memt = pmalloc(sizeof(bus_space_tag_t));
	pd->pa.pa_memt->bus_reverse = 1;
#if 0
	pd->pa.pa_memt->bus_base = PCI1_MEM_SPACE_BASE - PCI1P_MEM_SPACE_BASE;
#endif
	pd->pa.pa_memt->bus_base = 0;
	pd->bridge.secbus = pb;
	_pci_head->next = pd;

	*pb = *_pci_bushead;
	pb->minpcimemaddr  = PCI1P_MEM_SPACE_BASE;
	pb->nextpcimemaddr = PCI1P_MEM_SPACE_BASE + PCI1_MEM_SPACE_SIZE;
	pb->minpciioaddr  = PCI1P_IO_SPACE_BASE;
	pb->nextpciioaddr = PCI1P_IO_SPACE_BASE + 0xA000;/*PCI1_IO_SPACE_SIZE;*/
	pb->pci_mem_base   = PCI1_MEM_SPACE_BASE;
	pb->pci_io_base    = PCI1_IO_SPACE_BASE;

	_pci_bushead->next = pb;
	_pci_bus[_max_pci_bus++] = pd;

	bus_dmamap_tag._dmamap_offs = PCI0_CPU_MEM_BASE;


	/*
	 *  Enable PCI 0 as master to do config cycles.
	 */
	stat = _pci_conf_read(_pci_make_tag(0, 0, 0), PCI_COMMAND_STATUS_REG);
	stat |= PCI_COMMAND_MASTER_ENABLE | PCI_COMMAND_MEM_ENABLE;
	_pci_conf_write(_pci_make_tag(0, 0, 0), PCI_COMMAND_STATUS_REG, stat);

	/*
	 *  Enable PCI 1 as master to do config cycles.
	 */
	stat = _pci_conf_read(_pci_make_tag(1, 0, 0), PCI_COMMAND_STATUS_REG);
	stat |= PCI_COMMAND_MASTER_ENABLE | PCI_COMMAND_MEM_ENABLE;
	_pci_conf_write(_pci_make_tag(1, 0, 0), PCI_COMMAND_STATUS_REG, stat);
        /*
         *  Set up CPU to PCI mappings. Use only one I/O and MEM each.
	 *  All unused have been turned off in start.S
         */
	GT_WRITE(PCI_0I_O_LOW_DECODE_ADDRESS, UNCACHED_TO_PHYS(PCI0_IO_SPACE_BASE) >> 20);
	GT_WRITE(PCI_0I_O_HIGH_DECODE_ADDRESS,
		 (UNCACHED_TO_PHYS(PCI0_IO_SPACE_BASE) + PCI0_IO_SPACE_SIZE - 1) >> 20);
	GT_WRITE(PCI_0I_O_ADDRESS_REMAP, PCI0P_IO_SPACE_BASE >> 20);
	GT_WRITE(PCI_0MEMORY0_LOW_DECODE_ADDRESS, UNCACHED_TO_PHYS(PCI0_MEM_SPACE_BASE) >> 20);
	GT_WRITE(PCI_0MEMORY0_HIGH_DECODE_ADDRESS,
		 (UNCACHED_TO_PHYS(PCI0_MEM_SPACE_BASE) + PCI0_MEM_SPACE_SIZE - 1) >> 20);
#if 0
	GT_WRITE(PCI_0MEMORY0_ADDRESS_REMAP, PCI0P_MEM_SPACE_BASE >> 20);
#endif
	GT_WRITE(PCI_1I_O_LOW_DECODE_ADDRESS, UNCACHED_TO_PHYS(PCI1_IO_SPACE_BASE) >> 20);
	GT_WRITE(PCI_1I_O_HIGH_DECODE_ADDRESS,
		 (UNCACHED_TO_PHYS(PCI1_IO_SPACE_BASE) + PCI1_IO_SPACE_SIZE - 1) >> 20);
	GT_WRITE(PCI_1I_O_ADDRESS_REMAP, PCI1P_IO_SPACE_BASE >> 20);
	GT_WRITE(PCI_1MEMORY0_LOW_DECODE_ADDRESS, UNCACHED_TO_PHYS(PCI1_MEM_SPACE_BASE) >> 20);
	GT_WRITE(PCI_1MEMORY0_HIGH_DECODE_ADDRESS,
		 (UNCACHED_TO_PHYS(PCI1_MEM_SPACE_BASE) + PCI1_MEM_SPACE_SIZE - 1) >> 20);
#if 0
	GT_WRITE(PCI_1MEMORY0_ADDRESS_REMAP, PCI1P_MEM_SPACE_BASE >> 20);
#endif

#if 1
	GT_WRITE(PCI_1MEMORY1_LOW_DECODE_ADDRESS, UNCACHED_TO_PHYS(PCI1_MEM_SPACELOW_BASE) >> 20);
	GT_WRITE(PCI_1MEMORY1_HIGH_DECODE_ADDRESS,
		 (UNCACHED_TO_PHYS(PCI1_MEM_SPACELOW_BASE) + PCI1_MEM_SPACELOW_SIZE - 1) >> 20);
	GT_WRITE(PCI_1MEMORY1_ADDRESS_REMAP, PCI1P_MEM_SPACELOW_BASE >> 20);
#endif

	GT_WRITE(PCI_0BASE_ADDRESS_REGISTERS_ENABLE, PCI_BAR_ENABLE_DEFAULT);
	GT_WRITE(PCI_1BASE_ADDRESS_REGISTERS_ENABLE, PCI_BAR_ENABLE_DEFAULT);


        /*
         *  Set up mapping for PCI to localmem accesses.
	 *  config regs to find mapping and size. BAR and
	 *  size register should be set to match SDRAM SCS.
         */
	
	_pci_conf_write(_pci_make_tag(0, 0, 0), PCI_INTERNAL_REGISTERS_MEMORY_MAPPED_BASE_ADDRESS, 0xf4000000);
	_pci_conf_write(_pci_make_tag(1, 0, 0), PCI_INTERNAL_REGISTERS_MEMORY_MAPPED_BASE_ADDRESS, 0xf4000000);
		
	for(i = 0; i < NBARS; i++) {
		u_int32_t baselo, basehi, enabler;
		pcitag_t tag;

		baselo = GT_READ(barlist[i].scslow);
		basehi = GT_READ(barlist[i].scshigh);
		if(baselo > basehi) { /* Disabled */
			baselo = 0;
			basehi = 0;
		}
		else {
			baselo = baselo << 20;
			basehi = (basehi << 20) | 0xfffff;
			printf("{%08x~%08x}\n", baselo, basehi);
		}
		
		tag = _pci_make_tag(0, 0, (barlist[i].bar0 & 0x700) >> 8);
		stat = _pci_conf_read(tag, barlist[i].bar0 & 0xff) & 0xffff;
		stat |= baselo & 0xfffff000;
		if (barlist[i].remap0 != 0)
			stat += PCI0_CPU_MEM_BASE;
		_pci_conf_write(tag, barlist[i].bar0 & 0xff, stat);
		tag = _pci_make_tag(1, 0, (barlist[i].bar0 & 0x700) >> 8);
		stat = _pci_conf_read(tag, barlist[i].bar0 & 0xff) & 0xffff;
		stat |= baselo & 0xfffff000;
		if (barlist[i].remap1 != 0)
			stat += PCI1_CPU_MEM_BASE;
		_pci_conf_write(tag, barlist[i].bar0 & 0xff, stat);
		GT_WRITE(barlist[i].pci0size, (basehi - baselo) & 0xfffff000);
		GT_WRITE(barlist[i].pci1size, (basehi - baselo) & 0xfffff000);
		if (barlist[i].remap0 != 0)
			GT_WRITE(barlist[i].remap0, 0x0);
		if (barlist[i].remap1 != 0)
			GT_WRITE(barlist[i].remap1, 0x0);
		if(!baselo && !basehi){
			enabler = GT_READ(PCI_0BASE_ADDRESS_REGISTERS_ENABLE);
			enabler |= barlist[i].enablemask;
			GT_WRITE(PCI_0BASE_ADDRESS_REGISTERS_ENABLE, enabler);
			GT_WRITE(PCI_1BASE_ADDRESS_REGISTERS_ENABLE, enabler);
		}
	}

	/*
         *  This target uses the internal arbiter
         */
        stat = GT_READ(PCI_0ARBITER_CONTROL);
        stat |= 0x80000000;
        GT_WRITE(PCI_0ARBITER_CONTROL, stat);
        stat = GT_READ(PCI_1ARBITER_CONTROL);
        stat |= 0x80000000;
        GT_WRITE(PCI_1ARBITER_CONTROL, stat); 

	return(2);
}


/*
 * Called to reinitialise the bridge after we've scanned each PCI device
 * and know what is possible. We also set up the interrupt controller
 * routing and level control registers.
 */
void
_pci_hwreinit (void)
{
}

void
_pci_flush (void)
{
}


/*
 *  Map the CPU virtual address of an area of local memory to a PCI
 *  address that can be used by a PCI bus master to access it.
 */
vm_offset_t
_pci_dmamap(va, len)
	vm_offset_t va;
	unsigned int len;
{
#if 0
	return(VA_TO_PA(va) + bus_dmamap_tag._dmamap_offs);
#endif
	return(pci_local_mem_pci_base + VA_TO_PA (va));
}

#if 0
/*
 *  Map the PCI address of an area of local memory to a CPU physical
 *  address.
 */
vm_offset_t
_pci_cpumap(pcia, len)
	vm_offset_t pcia;
	unsigned int len;
{
	return PA_TO_VA(pcia - pci_local_mem_pci_base);
}
#endif


/*
 *  Make pci tag from bus, device and function data.
 */
pcitag_t
_pci_make_tag(bus, device, function)
	int bus;
	int device;
	int function;
{
	pcitag_t tag;

	tag = (bus << 16) | (device << 11) | (function << 8);
	return(tag);
}

/*
 *  Break up a pci tag to bus, device function components.
 */
void
_pci_break_tag(tag, busp, devicep, functionp)
	pcitag_t tag;
	int *busp;
	int *devicep;
	int *functionp;
{
	if (busp) {
		*busp = (tag >> 16) & 255;
	}
	if (devicep) {
		*devicep = (tag >> 11) & 31;
	}
	if (functionp) {
		*functionp = (tag >> 8) & 7;
	}
}

int
_pci_canscan (pcitag_t tag)
{
	int bus, device, function;

	_pci_break_tag (tag, &bus, &device, &function); 
	if((bus == 0 || bus == 1) && device == 0) {
		return(0);		/* Ignore the Discovery itself */
	}
	return (1);
}

/*
 *  Read a value form PCI configuration space.
 */
pcireg_t
_pci_conf_readn(tag, reg, width)
	pcitag_t tag;
	int reg;
	int width;
{
	pcireg_t data;
	u_int32_t adr;
	int bus, device, function;
	if (reg & (width-1) || reg < 0 || reg >= 0x100) {
		if (_pciverbose >= 1) {
			_pci_tagprintf (tag, "_pci_conf_read: bad reg 0x%x\r\n", reg);
		}
		return ~0;
	}

	_pci_break_tag (tag, &bus, &device, &function); 
	/* Type 0 configuration on onboard PCI bus */
	if (device > 29 || function > 7) {
		return ~0;		/* device out of range */
	}
	adr = (device << 11) | (function << 8) | reg | GT_IPCI_CFGADDR_ConfigEn;
	if(bus == 0) {
		GT_WRITE(PCI_0CONFIGURATION_ADDRESS, adr);
		data = GT_READ(PCI_0CONFIGURATION_ADDRESS);
		if (data != adr)
			return 0xffffffff;
		if(width==4)
			data = GT_READ(PCI_0CONFIGURATION_DATA_VIRTUAL_REGISTER);
		else if(width==2)
			data = GT_READ_WORD(PCI_0CONFIGURATION_DATA_VIRTUAL_REGISTER);
		else data = GT_READ_BYTE(PCI_0CONFIGURATION_DATA_VIRTUAL_REGISTER);
	}
	else {
		GT_WRITE(PCI_1CONFIGURATION_ADDRESS, adr);
		data = GT_READ(PCI_1CONFIGURATION_ADDRESS);
		if (data != adr)
			return 0xffffffff;
		if(width==4)
			data = GT_READ(PCI_1CONFIGURATION_DATA_VIRTUAL_REGISTER);
		else if(width==2)
			data = GT_READ_WORD(PCI_1CONFIGURATION_DATA_VIRTUAL_REGISTER);
		else
			data = GT_READ_BYTE(PCI_1CONFIGURATION_DATA_VIRTUAL_REGISTER);
			
	}
	return data;
}


pcireg_t
_pci_conf_read(pcitag_t tag, int reg)
{
	return _pci_conf_readn(tag, reg, 4);
}

/*
 *  Write a value to PCI configuration space.
 */
void
_pci_conf_writen(tag, reg, data, width)
	pcitag_t tag;
	int reg;
	pcireg_t data;
	int width;
{
	u_int32_t adr;
	int bus, device, function;

	if (reg & (width-1) || reg < 0 || reg >= 0x100) {
		if (_pciverbose >= 1) { 
			_pci_tagprintf(tag, "_pci_conf_write: bad reg 0x%x\r\n", reg);
		}
		return;
	}

	_pci_break_tag (tag, &bus, &device, &function);

	/* Type 0 configuration on onboard PCI buses */
	if (device > 29 || function > 7) {
		return;		/* device out of range */
	}
	adr = (device << 11) | (function << 8) | (reg & 0xfc) | GT_IPCI_CFGADDR_ConfigEn;

	if(bus == 0) {
		GT_WRITE(PCI_0CONFIGURATION_ADDRESS, adr);
		if(width==4)
			GT_WRITE(PCI_0CONFIGURATION_DATA_VIRTUAL_REGISTER, data);
		else if(width==2)
			GT_WRITE_WORD(PCI_0CONFIGURATION_DATA_VIRTUAL_REGISTER, (unsigned short)data);
		else GT_WRITE_BYTE(PCI_0CONFIGURATION_DATA_VIRTUAL_REGISTER, (unsigned char)data);
	}
	else {
		GT_WRITE(PCI_1CONFIGURATION_ADDRESS, adr);
		if(width==4)
			GT_WRITE(PCI_1CONFIGURATION_DATA_VIRTUAL_REGISTER, data);
		else if(width==2)
			GT_WRITE_WORD(PCI_1CONFIGURATION_DATA_VIRTUAL_REGISTER,(unsigned short)data);
		else GT_WRITE_WORD(PCI_1CONFIGURATION_DATA_VIRTUAL_REGISTER, (unsigned char)data);
	}
}

void
_pci_conf_write(pcitag_t tag, int reg, pcireg_t data)
{
    _pci_conf_writen (tag, reg, data, 4);
}


void
pci_sync_cache(p, adr, size, rw)
	void *p;
	vm_offset_t adr;
	size_t size;
	int rw;
{
	CPU_IOFlushDCache(adr, size, rw);
}

