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

#include "include/ev64420.h"
#include "pmon/dev/gt64420reg.h"

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
	int	csbase,	cssize;
	int	remap;
	int	pcisize;
	int	bar0, enablemask;
} barlist[] = {
{ CS_0_BASE_ADDRESS, CS_0_SIZE,
  PCI_CS_0_BASE_ADDRESS_REMAP,
  PCI_CS_0_BANK_SIZE, 
  PCI_CS_0_BASE_ADDRESS_LOW, 0x1 },
{ CS_1_BASE_ADDRESS, CS_1_SIZE,
  PCI_CS_1_BASE_ADDRESS_REMAP, 
  PCI_CS_1_BANK_SIZE,
  PCI_CS_1_BASE_ADDRESS_LOW, 0x2 },
{ CS_2_BASE_ADDRESS, CS_2_SIZE,
  PCI_CS_2_BASE_ADDRESS_REMAP, 
  PCI_CS_2_BANK_SIZE,
  PCI_CS_2_BASE_ADDRESS_LOW, 0x4 },
{ CS_3_BASE_ADDRESS, CS_3_SIZE, 
  PCI_CS_3_BASE_ADDRESS_REMAP, 
  PCI_CS_3_BANK_SIZE, 
  PCI_CS_3_BASE_ADDRESS_LOW, 0x8 },
{ DEVCS_0_BASE_ADDRESS, DEVCS_0_SIZE,
  0, 
  PCI_DEVCS_0_BANK_SIZE, 
  PCI_DEVCS_0_BASE_ADDRESS_LOW, 0x10 },
{ DEVCS_1_BASE_ADDRESS, DEVCS_1_SIZE,
  0, 
  PCI_DEVCS_1_BANK_SIZE,
  PCI_DEVCS_1_BASE_ADDRESS_LOW, 0x20 },
{ DEVCS_2_BASE_ADDRESS, DEVCS_2_SIZE,
  0, 
  PCI_DEVCS_2_BANK_SIZE,
  PCI_DEVCS_2_BASE_ADDRESS_LOW, 0x40 },
{ DEVCS_3_BASE_ADDRESS, DEVCS_3_SIZE,
  0, 
  PCI_DEVCS_3_BANK_SIZE,
  PCI_DEVCS_3_BASE_ADDRESS_LOW, 0x80 },
{ BOOTCS_BASE_ADDRESS, BOOTCS_SIZE,
  0, 
  PCI_DEVCS_BOOT_BANK_SIZE,
  PCI_BOOTCS_BASE_ADDRESS_LOW, 0x100 },
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
	int enabler0;
	
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
	pd->pa.pa_iot->bus_base = PCI_IO_SPACE_BASE - PCIP_IO_SPACE_BASE;
#else
	pd->pa.pa_iot->bus_base = 0;
#endif
	pd->pa.pa_memt = pmalloc(sizeof(bus_space_tag_t));
	pd->pa.pa_memt->bus_reverse = 1;
#if 0
	pd->pa.pa_memt->bus_base = PCI_MEM_SPACE_BASE - PCIP_MEM_SPACE_BASE;
#endif
	pd->pa.pa_memt->bus_base = 0;
	pd->pa.pa_dmat = &bus_dmamap_tag;
	pd->bridge.secbus = pb;
	_pci_head = pd;

	pb->minpcimemaddr  = PCIP_MEM_SPACE_BASE; //最小地址从256M地址以上
	pb->nextpcimemaddr = PCIP_MEM_SPACE_BASE + PCI_MEM_SPACE_SIZE; //pci内存空间最多24M大小
	pb->minpciioaddr  = PCIP_IO_SPACE_BASE+0x9000;   //最小io地址为0
	pb->nextpciioaddr = PCIP_IO_SPACE_BASE + PCI_IO_SPACE_SIZE; //最大为1M pciio 空间.
	pb->pci_mem_base   = PCI_MEM_SPACE_BASE;	//cpu端看的pci mem基地址
	pb->pci_io_base    = PCI_IO_SPACE_BASE;  //cpu端看的pci io基地址
	pb->max_lat = 255;
	pb->fast_b2b = 1;
	pb->prefetch = 1;
	pb->bandwidth = 4000000;
	pb->ndev = 1;
	_pci_bushead = pb;
	_pci_bus[_max_pci_bus++] = pd;


	bus_dmamap_tag._dmamap_offs = PCI_CPU_MEM_BASE;


	/*
	 *  Enable PCI  as master to do config cycles.
	 */
	stat = _pci_conf_read(_pci_make_tag(0, 0, 0), PCI_COMMAND_STATUS_REG);
	stat |= PCI_COMMAND_MASTER_ENABLE | PCI_COMMAND_MEM_ENABLE;
	_pci_conf_write(_pci_make_tag(0, 0, 0), PCI_COMMAND_STATUS_REG, stat);

        /*
         *  Set up CPU to PCI mappings. Use only one I/O and MEM each.
	 *  All unused have been turned off in start.S
         */
#if 1
#define PCI_ORDING (1<<28)
#define CHANGE_SIZE(x) \
	val=GT_READ(PCI_ACCESS_CONTROL_SIZE_##x); \
	GT_WRITE(PCI_ACCESS_CONTROL_SIZE_##x,val|(1<<11));
	
	{int val;
	val=GT_READ(PCI_COMMAND);
	GT_WRITE(PCI_COMMAND,val|0x50000000);
	CHANGE_SIZE(0);
	CHANGE_SIZE(1);
	CHANGE_SIZE(2);
	CHANGE_SIZE(3);
	CHANGE_SIZE(4);
	CHANGE_SIZE(5);
	}
#else 
#define PCI_ORDING 0
#endif	
	GT_WRITE(PCI_I_O_BASE_ADDRESS, (UNCACHED_TO_PHYS(PCI_IO_SPACE_BASE) >> 16)|PCI_ORDING);
	GT_WRITE(PCI_I_O_SIZE, (PCI_IO_SPACE_SIZE - 1) >> 16);
	GT_WRITE(PCI_I_O_ADDRESS_REMAP, PCIP_IO_SPACE_BASE >> 16);
	GT_WRITE(PCI_MEMORY0_BASE_ADDRESS, (UNCACHED_TO_PHYS(PCI_MEM_SPACE_BASE) >> 16)|PCI_ORDING);
	GT_WRITE(PCI_MEMORY0_SIZE, ( PCI_MEM_SPACE_SIZE - 1) >> 16);
	/*不需要remap,因为pci地址空间和cpu地址空间相同*/

#if 0 /* zhb: for wrtrig test */
	    {
		    int zztemp;
	        zztemp = GT_READ(0x1424);
	        zztemp &= 0xfcff0fff;
	        GT_WRITE(0x1424, zztemp);
	        zztemp = GT_READ(0x1424);
	    }
#endif

#if 1
	GT_WRITE(PCI_MEMORY1_BASE_ADDRESS, UNCACHED_TO_PHYS(PCI_MEM_SPACELOW_BASE) >> 16);
	GT_WRITE(PCI_MEMORY1_SIZE, (PCI_MEM_SPACELOW_SIZE - 1) >> 16);
	GT_WRITE(PCI_MEMORY1_ADDRESS_REMAP, PCIP_MEM_SPACELOW_BASE >> 16);/*PCI低端1M地址映射*/
#endif

	GT_WRITE(PCI_BASE_ADDRESS_REGISTERS_ENABLE, PCI_BAR_ENABLE_DEFAULT); /*默认值为全部使能*/

        /*
         *  Set up mapping for PCI to localmem accesses.
	 *  config regs to find mapping and size. BAR and
	 *  size register should be set to match SDRAM SCS.
         */
	/*we stay here. */
	
	_pci_conf_write(_pci_make_tag(0, 0, 0), PCI_INTERNAL_REGISTERS_MEMORY_MAPPED_BASE_ADDRESS_LOW, 0xf4000000);
	
	enabler0 = GT_READ(BASE_ADDRESS_ENABLE_REG);

	for(i = 0; i < NBARS; i++) {
		u_int32_t csbase, cssize, enabler;
		pcitag_t tag;

		csbase = GT_READ(barlist[i].csbase);
		cssize = GT_READ(barlist[i].cssize);

		if(enabler0&barlist[i].enablemask) { /* Disabled */
			csbase = 0;
			cssize = 0;
		}
		else {
			csbase = csbase << 16;
			cssize = (cssize << 16) | 0xffff;
			printf("{%08x~%08x}\n", csbase, csbase+cssize);
		}

		tag = _pci_make_tag(0, 0, (barlist[i].bar0 & 0x700) >> 8);
		stat = _pci_conf_read(tag, barlist[i].bar0 & 0xff) & 0xffff;
		stat |= csbase & 0xfffff000;
		if (barlist[i].remap != 0)
			stat += PCI_CPU_MEM_BASE; 
		_pci_conf_write(tag, barlist[i].bar0 & 0xff, stat);
		GT_WRITE(barlist[i].pcisize, cssize & 0xfffff000);
#if 0 /*qiaochong:感觉不需要remap,pci地址和cpu地址相同即可*/
/*这是remap算法,很奇怪,这样pci不同的地址实际都映射到相同的cpu地址上了,为什么这样?*/
		if (barlist[i].remap != 0)
			GT_WRITE(barlist[i].remap, 0x0);
#endif
		if(enabler0&barlist[i].enablemask)
		{
			enabler = GT_READ(PCI_BASE_ADDRESS_REGISTERS_ENABLE);
			enabler |= barlist[i].enablemask;
			GT_WRITE(PCI_BASE_ADDRESS_REGISTERS_ENABLE, enabler);
		}
	}

	/*
	 *  This target uses the internal arbiter
	 */
	stat = GT_READ(PCI_ARBITER_CONTROL);
	stat |= 0x80000002|(0x7f<<14);
	GT_WRITE(PCI_ARBITER_CONTROL, stat);

	return(1);
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
	if (((bus == 0) && device == 0) || (device > 15)) {
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
	if (device > 15 || function > 7) {
		return ~0;		/* device out of range */
	}
	adr = (bus<<16) | (device << 11) | (function << 8) | (reg& 0xfc) | GT_IPCI_CFGADDR_ConfigEn;
	
		GT_WRITE(PCI_CONFIGURATION_ADDRESS, adr);
		data = GT_READ(PCI_CONFIGURATION_ADDRESS);
		if (data != adr)
			return 0xffffffff;
		if(width==4)
			data = GT_READ(PCI_CONFIGURATION_DATA_VIRTUAL_REGISTER);
		else if(width==2)
			data = GT_READ_WORD(PCI_CONFIGURATION_DATA_VIRTUAL_REGISTER+(reg&3));
		else data = GT_READ_BYTE(PCI_CONFIGURATION_DATA_VIRTUAL_REGISTER+(reg&3));
	
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
	if (device > 15 || function > 7) {
		return;		/* device out of range */
	}
	adr = (bus<<16) | (device << 11) | (function << 8) | (reg & 0xfc) | GT_IPCI_CFGADDR_ConfigEn;

		GT_WRITE(PCI_CONFIGURATION_ADDRESS, adr);
		if(width==4)
			GT_WRITE(PCI_CONFIGURATION_DATA_VIRTUAL_REGISTER, data);
		else if(width==2)
			GT_WRITE_WORD(PCI_CONFIGURATION_DATA_VIRTUAL_REGISTER+(reg&3), (unsigned short)data);
		else GT_WRITE_BYTE(PCI_CONFIGURATION_DATA_VIRTUAL_REGISTER+(reg&3), (unsigned char)data);
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

/*pc interrupt route can be set in linux kernel*/
#if 1
#undef PCI_INT_A
#undef PCI_INT_B
#undef PCI_INT_C
#undef PCI_INT_D
#define PCI_INT_A 1
#define PCI_INT_B 2
#define PCI_INT_C 3
#define PCI_INT_D 4

static struct pci_intline_routing pri_pci_bus = {
   0, 0, 0, /* Northbridge controller */
   { {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot  0: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot  1: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot  2: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0}, 	/* PCI Slot  3: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot  4: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0}, 	/* PCI Slot  5: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot  6: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot  7: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot  8: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot  9: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 10: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 11: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 12: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 13: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 14: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 15: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 16: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 17: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 18: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 19: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 20: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 21: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 22: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 23: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 24: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 25: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 26: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 27: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 28: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 29: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0},	/* PCI Slot 30: */
   {PCI_INT_0, PCI_INT_0, PCI_INT_0, PCI_INT_0} },	/* PCI Slot 31: */
   NULL
};

static int InitInterruptRouting __P((void)) __attribute__ ((constructor));
int InitInterruptRouting(void)
{
      _pci_inthead = &pri_pci_bus;

      return(0);
}
#endif

