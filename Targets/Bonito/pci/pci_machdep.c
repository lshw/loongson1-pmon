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

#include "include/bonito.h"

#include <pmon.h>

void pmcfgport(int on);
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
	if(!getenv("oldcfg"))newcfg=1;

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
	pd->pa.pa_iot->bus_base = BONITO_PCIIO_BASE_VA;
	//printf("pd->pa.pa_iot=%p,bus_base=0x%x\n",pd->pa.pa_iot,pd->pa.pa_iot->bus_base);
	pd->pa.pa_memt = pmalloc(sizeof(bus_space_tag_t));
	pd->pa.pa_memt->bus_reverse = 1;
	pd->pa.pa_dmat = &bus_dmamap_tag;
	pd->bridge.secbus = pb;
	_pci_head = pd;

	if(newcfg)
	{
	pb->minpcimemaddr  = 0x14000000;//很奇怪,不能和linux分配起始地址一致,否则xwindow中显示出问题
	pb->nextpcimemaddr = 0x1c000000; 
	pd->pa.pa_memt->bus_base = 0xa0000000;
	BONITO_PCIMAP =
	    BONITO_PCIMAP_WIN(0, PCI_MEM_SPACE_PCI_BASE+0x00000000) |	
	    BONITO_PCIMAP_WIN(1, PCI_MEM_SPACE_PCI_BASE+0x14000000) |
	    BONITO_PCIMAP_WIN(2, PCI_MEM_SPACE_PCI_BASE+0x18000000) |
	    BONITO_PCIMAP_PCIMAP_2;
	}
	else
	{
	/*pci地址和cpu地址不等,访问pci mem时要ioreamap转换,直接or 0xb0000000*/
	pb->minpcimemaddr  = 0x04000000;
	pb->nextpcimemaddr = 0x0c000000; 
	pd->pa.pa_memt->bus_base = 0xb0000000;
	BONITO_PCIMAP =
	    BONITO_PCIMAP_WIN(0, PCI_MEM_SPACE_PCI_BASE+0x00000000) |	
	    BONITO_PCIMAP_WIN(1, PCI_MEM_SPACE_PCI_BASE+0x04000000) |
	    BONITO_PCIMAP_WIN(2, PCI_MEM_SPACE_PCI_BASE+0x08000000) |
	    BONITO_PCIMAP_PCIMAP_2;
	}
	pb->minpciioaddr  = 0x0004000;
	pb->nextpciioaddr = BONITO_PCIIO_SIZE;
	pb->pci_mem_base   = 0xb0000000;
	pb->pci_io_base    = 0xbfd00000;
	pb->max_lat = 255;
	pb->fast_b2b = 1;
	pb->prefetch = 1;
	pb->bandwidth = 4000000;
	pb->ndev = 1;
	_pci_bushead = pb;
	_pci_bus[_max_pci_bus++] = pd;

	
	bus_dmamap_tag._dmamap_offs = 0;


	pci_local_mem_pci_base = 0x80000000;
	BONITO_PCIBASE0 = 0x80000000;
	BONITO_PCIBASE1 = 0;
	BONITO_PCIBASE2 = 0xf0000000;
    BONITO_PCI_REG(0x40)=0x80000000; //base0's mask register
    BONITO_PCI_REG(0x44)=0xf0000000; //base1's mask register

	return(1);
}

#define UART1_485 (1<<17)
#define UART1_422 (1<<18)
#define UART2_485 (1<<19)
#define UART2_422 (1<<20)
#define UART1_232 (1<<21)
#define UART2_232 (1<<0)
#define LAN1_EN (1<<8)
#define LAN2A_EN (1<<27)
#define LAN2B_EN (1<<28)
/*默认使能双网卡,两个串口都工作在232模式下*/
#define GPO_INITDATA  (UART1_232|UART2_232|LAN1_EN|LAN2B_EN)

#include "target/i82371eb.h"
#include "include/sbd.h"
#define PCI_MAP_IO			0X00000001
void pmcfgport(int on)
{
	pcitag_t tag;
	tag=_pci_make_tag(0,PCI_IDSEL_I82371,3);
if(on)
{
	_pci_conf_write(tag,I82371_PCI3_SMBBA,SMB_PORT|PCI_MAP_IO);
	_pci_conf_write(tag,I82371_PCI3_PMBA,PM_PORT|PCI_MAP_IO);
}
else
{
	_pci_conf_write(tag,I82371_PCI3_SMBBA,0);
	_pci_conf_write(tag,I82371_PCI3_PMBA,0);
}
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


#if 1
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


pcireg_t
_pci_conf_read(pcitag_t tag,int reg)
{
	return _pci_conf_readn(tag,reg,4);
}

void
_pci_conf_write(pcitag_t tag, int reg, pcireg_t data)
{
	return _pci_conf_writen(tag,reg,data,4);
}

#if 0
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

    /* clear aborts */
    BONITO_PCICMD |= PCI_STATUS_MASTER_ABORT | PCI_STATUS_MASTER_TARGET_ABORT;

    BONITO_PCIMAP_CFG = (addr >> 16) | type;
    data = *(volatile pcireg_t *)PHYS_TO_UNCACHED(BONITO_PCICFG_BASE | (addr & 0xfffc));


    if (BONITO_PCICMD & PCI_STATUS_MASTER_ABORT) {
	BONITO_PCICMD |= PCI_STATUS_MASTER_ABORT;
#if 1
	if (_pciverbose >= 1)
	    _pci_tagprintf (tag, "_pci_conf_read: reg=%x master abort\n", reg);
#endif
	return ~0;
    }

    if (BONITO_PCICMD & PCI_STATUS_MASTER_TARGET_ABORT) {
	BONITO_PCICMD |= PCI_STATUS_MASTER_TARGET_ABORT;
	if (_pciverbose >= 1)
	    _pci_tagprintf (tag, "_pci_conf_read: target abort\n");
	return ~0;
    }

    return data;
}

void
_pci_conf_writen(pcitag_t tag, int reg, pcireg_t data,int width)
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

    /* clear aborts */
    BONITO_PCICMD |= PCI_STATUS_MASTER_ABORT | PCI_STATUS_MASTER_TARGET_ABORT;

    BONITO_PCIMAP_CFG = (addr >> 16);
    *(volatile pcireg_t *)PHYS_TO_UNCACHED(BONITO_PCICFG_BASE | (addr & 0xfffc)) = data;

    if (BONITO_PCICMD & PCI_STATUS_MASTER_ABORT) {
	BONITO_PCICMD |= PCI_STATUS_MASTER_ABORT;
	if (_pciverbose >= 1)
	    _pci_tagprintf (tag, "_pci_conf_write: master abort\n");
    }

    if (BONITO_PCICMD & PCI_STATUS_MASTER_TARGET_ABORT) {
	BONITO_PCICMD |= PCI_STATUS_MASTER_TARGET_ABORT;
	if (_pciverbose >= 1)
	    _pci_tagprintf (tag, "_pci_conf_write: target abort\n");
    }
}
#else
#include "pci_linux.c"
#endif



void
pci_sync_cache(p, adr, size, rw)
	void *p;
	vm_offset_t adr;
	size_t size;
	int rw;
{
	CPU_IOFlushDCache(adr, size, rw);
}

