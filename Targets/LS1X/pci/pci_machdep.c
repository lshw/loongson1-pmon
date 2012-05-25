/*	$Id: pci_machdep.c,v 1.1.1.1 2006/09/14 01:59:09 root Exp $ */

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

#include "include/fcr.h"

#include <pmon.h>

#define FCR_CFG_BASE			0x1c100000	
#define FCR_PCI_IO_BASE		0x1c000000
#define FCR_PCI_MEM_BASE		0x10000000
#define PCIB_HEADER_BASE 0x1c180000
#define PCIB_CONTROL_BASE 0x1fd01100

#define PCI_OXARB_CONFIG *(volatile int *)PHYS_TO_UNCACHED(0x1fd0110c)
#define PCI_OXARB_STATUS *(volatile int *)PHYS_TO_UNCACHED(0x1fd01100)
#define PCIMAP *(volatile int *)PHYS_TO_UNCACHED(0x1fd01114)
#define PCIMAP_CFG *(volatile int *)PHYS_TO_UNCACHED(0x1fd01120)

#define BONITO_PCICMD		*((unsigned long *)(0xbc100000+0x4))
#define PCIB_HEADER(x) *(volatile int *)PHYS_TO_UNCACHED(PCIB_HEADER_BASE+x)


extern void *pmalloc __P((size_t ));

extern int _pciverbose;

extern char hwethadr[6];
struct pci_device *_pci_bus[16];
int _max_pci_bus = 0;


/* PCI mem regions in PCI space */

/* soft versions of above */
static pcireg_t pci_local_mem_pci_base;


/****************************/
/*initial PCI               */
/****************************/

int
_pci_hwinit(initialise, iot, memt)
	int initialise;
	bus_space_tag_t iot;
	bus_space_tag_t memt;
{
	/*pcireg_t stat;*/
	struct pci_device *pd;
	struct pci_bus *pb;
	int newcfg=0;
	if(getenv("newcfg"))newcfg=1;

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
//	pd->pa.pa_iot->bus_base = 0xbfd00000;
//sw
	pd->pa.pa_iot->bus_base = 0xbc000000;
//	printf("pd->pa.pa_iot=%p,bus_base=0x%x\n",pd->pa.pa_iot,pd->pa.pa_iot->bus_base);
	pd->pa.pa_memt = pmalloc(sizeof(bus_space_tag_t));
	pd->pa.pa_memt->bus_reverse = 1;
	pd->pa.pa_dmat = &bus_dmamap_tag;
	pd->bridge.secbus = pb;
	_pci_head = pd;

	pb->minpcimemaddr  = FCR_PCI_MEM_BASE+0x04000000;
	pb->nextpcimemaddr = 0x17000000; 
//	pd->pa.pa_memt->bus_base = 0xa0000000;		??
	pd->pa.pa_memt->bus_base = 0xb0000000;
	pb->minpciioaddr  = 0x0004000;
	pb->nextpciioaddr = 0x100000;
	pb->pci_mem_base   = 0xb0000000;
	pb->pci_io_base    = 0xbc000000;
	pb->max_lat = 255;
	pb->fast_b2b = 1;
	pb->prefetch = 1;
	pb->bandwidth = 4000000;
	pb->ndev = 1;
	_pci_bushead = pb;
	_pci_bus[_max_pci_bus++] = pd;

	
	bus_dmamap_tag._dmamap_offs = 0;


	pci_local_mem_pci_base = 0x80000000;

//sw: code form 2f	
/*set pci base0 address and window size*/
//	printf("\n\n==1150: %x 1154: %x\n\n\n",	*(volatile unsigned long*)(0xbfd00000+0x1150),*(volatile unsigned long*)(0xbfd00000+0x1154));
//	printf("==win base: %x  win mmap: %x\n",*(unsigned long *)(0xbfd00120),*(unsigned long *)(0xbfd001a0));

	/*setup  pci base bar,enable io and mem*/
	PCIB_HEADER(0x24) = 0;
	PCIB_HEADER(0x20) = 0x80000000;
	PCIB_HEADER(4) = 0x7;
	PCIMAP_CFG = 0x1; //?
	PCI_OXARB_CONFIG  = 0x1;
	/*setup pci window*/
	PCIMAP = 0x6144;

	printf("\n\n==1150: %x 1154: %x\n\n\n",	*(volatile unsigned long*)(0xbfd00000+0x1150),*(volatile unsigned long*)(0xbfd00000+0x1154));
	printf("==win base: %x  win mmap: %x\n",*(unsigned long *)(0xbfd00120),*(unsigned long *)(0xbfd001a0));


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

pcireg_t
_pci_conf_read(pcitag_t tag,int reg)
{
	return _pci_conf_readn(tag,reg,4);
}

pcireg_t
_pci_conf_readn(pcitag_t tag, int reg, int width)
{
    u_int32_t addr, type;
    pcireg_t data;
    int bus, device, function;

    if ((reg & (width-1)) || reg < 0 || reg >= 0x100) {
	if (_pciverbose >= 1)
	    _pci_tagprintf (tag, "_pci_conf_read: bad reg 0x%x\n", reg);
	return ~0;
    }

    _pci_break_tag (tag, &bus, &device, &function); 
    if (bus == 0) {
	/* Type 0 configuration on onboard PCI bus */
	if (device > 20 || function > 7)
	    return ~0;		/* device out of range */
	addr = (1 << (device+11)) | (function << 8) | reg;
	type = 0x00000;
    }
    else {
	/* Type 1 configuration on offboard PCI bus */
	if (bus > 255 || device > 31 || function > 7)
	    return ~0;	/* device out of range */
	addr = (bus << 16) | (device << 11) | (function << 8) | reg;
	type = 0x10000;
    }

   
    PCIMAP_CFG  = (addr >> 16) | type;

    data = *(volatile pcireg_t *)PHYS_TO_UNCACHED(FCR_CFG_BASE | (addr & 0xfffc));

    data = data >> ((addr & 3) << 3);

    return data;
}


void _pci_conf_writen(pcitag_t tag, int reg, pcireg_t data,int width);

void
_pci_conf_write(pcitag_t tag, int reg, pcireg_t data)
{
	return _pci_conf_writen(tag,reg,data,4);
}


void _pci_conf_writen(pcitag_t tag, int reg, pcireg_t data,int width)
{
    u_int32_t addr, type;
    int bus, device, function;

    if ((reg &(width-1)) || reg < 0 || reg >= 0x100) {
	if (_pciverbose >= 1)
	    _pci_tagprintf (tag, "_pci_conf_write: bad reg %x\n", reg);
	return;
    }

    _pci_break_tag (tag, &bus, &device, &function);

    if (bus == 0) {
	/* Type 0 configuration on onboard PCI bus */
	if (device > 20 || function > 7)
	    return;		/* device out of range */
	addr = (1 << (device+11)) | (function << 8) | reg;
	type = 0x00000;
    }
    else {
	/* Type 1 configuration on offboard PCI bus */
	if (bus > 255 || device > 31 || function > 7)
	    return;	/* device out of range */
	addr = (bus << 16) | (device << 11) | (function << 8) | reg;
	type = 0x10000;
    }

    PCIMAP_CFG  = (addr >> 16) | type;
	
    {
      pcireg_t ori = *(volatile pcireg_t *)PHYS_TO_UNCACHED(FCR_CFG_BASE | (addr & 0xfffc));
      pcireg_t mask = 0x0;

      if (width == 2) {
		if (addr & 3) mask = 0xffff; 
		else mask = 0xffff0000;
	      }else if (width == 1) {
		if ((addr & 3) == 1) {
		  mask = 0xffff00ff;
		}else if ((addr & 3) == 2) {
		  mask = 0xff00ffff;
		}else if ((addr & 3) == 3) {
		  mask = 0x00ffffff;
		}else{
		  mask = 0xffffff00;
		}
	}

      data = data << ((addr & 3) << 3);
      data = (ori & mask) | data;
 	     
      *(volatile pcireg_t *)PHYS_TO_UNCACHED(FCR_CFG_BASE | (addr & 0xfffc)) = data;
	}
      
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



//sw: dbg
int  dump_pci(int argc,char **argv)
{
	int  i,j;
	unsigned long res[4];

	
	printf("-=== dump bridge regs\n");

	for (j = 0;j < 0x40;j = j+4)
	{
		for (i = 0;i < 4;i++)
			res[i] = *(unsigned long *)(0xbc180000+j);
		printf("==ddr: %x  val: %x\n",(0xbc180000+j),res[3]);
	}

	printf("-=== \n");

	printf("-=== dump config space\n");
	for (i = 0;i < 0x40;i = i+4)
	{
		PCIMAP_CFG  = 1;
		printf("==addr: %x  val: %x\n",0xbc100000+i,*(unsigned long *)(0xbc100000+i));
	}

	printf("==pci hitsel2(1150): %x  1154: %x\n",*(unsigned long *)(0xbfd01150),*(unsigned long *)(0xbfd01154));
	printf("==pci winbase: %x pci winmmap: %x\n",*(unsigned long *)(0xbfd00120),*(unsigned long *)(0xbfd001a0));

//sw: dbg
/*
	*(volatile unsigned long*)(0xbc180024) = 0x0;
	*(volatile unsigned long*)(0xbc180020) = 0x80000000;
	*(volatile unsigned long*)(0xbfd01154) = 0xffffffff;
	*(volatile unsigned long*)(0xbfd01150) = 0xff00000c;
	*(volatile unsigned long*)(0xbc180004) = 0x7;
	*(volatile unsigned long*)(0xbfd01120) = 0x1;
	*(volatile unsigned long*)(0xbfd0110c) = 0x1;
*/
return 0;
}

static const Cmd pciCmds[] =
{
	{"MyCmds"},
	{"dump_pci", "", NULL,
		    "dump sb pci-bridge regs", dump_pci, 1, 2, 0},
	{0, 0}
};


static void init_pcicmd __P((void)) __attribute__ ((constructor));

static void
init_pcicmd()
{
	cmdlist_expand(pciCmds, 1);
}
