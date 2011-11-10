//=====================================================
//      Name:
//              ev8172pci.c
//      License:
//              2004, Copyright by Pengliangjin
//      Description:
//              Low level PCI routines for IT8172G
//
//=====================================================

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <sys/malloc.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/nppbreg.h>

#include <machine/bus.h>

#include <pmon.h>


#include "include/it8172.h"

/* PCI i/o regions in PCI space */
#define PCI_IO_SPACE_PCI_BASE   0x00000000

/* PCI mem regions in PCI space */
#define PCI_LOCAL_MEM_PCI_BASE  0x00000000  /* CPU Mem accessed from PCI */



//=====================================
//      External variables declaration
//=====================================
extern void *pmalloc __P((size_t ));
extern int _pciverbose;

//=====================================
//      Local  variables declaration
//=====================================
// PCI mem space allocation,top down
static pcireg_t pci_local_mem_pci_base;
void _pci_conf_writen __P((pcitag_t, int, pcireg_t, int));
extern int _pciverbose;
extern char hwethadr[6];
struct pci_device *_pci_bus[16];
int _max_pci_bus = 0;
pcireg_t _pci_conf_readn __P((pcitag_t, int, int));

// PCI I/O space allocation,bottom up

static int pcireserved;

/*
 * _pci_hwinit
 *
 * Set pci memory & io address limitation
 * for following pci resource allocation
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
        pd->pa.pa_iot->bus_base = PCI_IO_SPACE_BASE - PCIP_IO_SPACE_BASE;
        pd->pa.pa_memt = pmalloc(sizeof(bus_space_tag_t));
        pd->pa.pa_memt->bus_reverse = 1;
        pd->pa.pa_memt->bus_base = 0;
        pd->pa.pa_dmat = &bus_dmamap_tag;
        pd->bridge.secbus = pb;
        _pci_head = pd;

        pb->minpcimemaddr  = PCIP_MEM_SPACE_BASE;
        pb->nextpcimemaddr = PCIP_MEM_SPACE_BASE + PCI_MEM_SPACE_SIZE;
        pb->minpciioaddr  = PCIP_IO_SPACE_BASE;
        pb->nextpciioaddr = PCIP_IO_SPACE_BASE + PCI_IO_SPACE_SIZE;
        pb->pci_mem_base   = PCI_MEM_SPACE_BASE;
        pb->pci_io_base    = PCI_IO_SPACE_BASE;
        pb->max_lat = 255;
        pb->fast_b2b = 1;
        pb->prefetch = 1;
        pb->bandwidth = 4000000;
        pb->ndev = 1;
        _pci_bushead = pb;
        _pci_bus[_max_pci_bus++] = pd;

        bus_dmamap_tag._dmamap_offs = PCI_CPU_MEM_BASE;


        /*
         *  Enable PCI 0 as master to do config cycles.
         */
        stat = _pci_conf_read(_pci_make_tag(0, 0, 0), PCI_COMMAND_STATUS_REG);
        stat |= PCI_COMMAND_MASTER_ENABLE | PCI_COMMAND_MEM_ENABLE;
        _pci_conf_write(_pci_make_tag(0, 0, 0), PCI_COMMAND_STATUS_REG, stat);


        /* find out what devices we should access */
        pcireserved = 0;

    return initialise;
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


void
_pci_hwreinit (void)
{
}

void
_pci_flush (void)
{
    /* flush read-ahead fifos (!) */
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

int
_pci_canscan (pcitag_t tag)
{
        int bus, dev, function;
        _pci_break_tag (tag, &bus, &dev, &function);
        if((bus == 0 || bus == 1) && dev == 0) {
                return(0);              /* Ignore the Discovery itself */
        }
        return (1);
}

/*
 * IT8172G PCI Configuration read routine
 */
pcireg_t
_pci_conf_readn (tag, reg, width)
        pcitag_t        tag;
        int             reg;
        int             width;
{
    pcireg_t data;
    int bus, device, function;

    if (reg < 0 || reg >= 0x100) {
        return ~0;
    }

    _pci_break_tag (tag, &bus, &device, &function);

    if (bus == 0) {
        /* Type 0 configuration on onboard PCI bus */
        if (device > 20 || function > 7)
        return ~0;              /* device out of range */
    } else {
        /* Type 1 configuration on offboard PCI bus */
        if (bus > 255 || device > 31 || function > 7)
        return ~0;      /* device out of range */
    }

    IT_WRITE(IT_CONFADDR,
	(bus << IT_BUSNUM_SHF) |
	(device << IT_DEVNUM_SHF) |
	(function << IT_FUNCNUM_SHF) | (reg & ~0x03));

    IT_READ(IT_CONFDATA, data);

    switch (width) {
      case 1:
	data = ((data) >> ((reg & 3) << 3)) & 0x0ff;
	break;

      case 2:
	data = ((data) >> ((reg & 3) << 3)) & 0xffff;
	break;

      case 4:
	break;

      default:
	return ~0;
    }

    return data;
}

pcireg_t
_pci_conf_read(pcitag_t tag, int reg)
{
	return _pci_conf_readn(tag, reg, 4);
}

/*
 * IT8172G PCI Configuration write routine
 */
void
_pci_conf_writen (tag, reg, data, width)
        pcitag_t        tag;
        int             reg;
        pcireg_t                data;
        int             width;
{
    int bus, device, function;

    if ((reg & (width-1)) || reg < 0 || reg >= 0x100) {
                if (_pciverbose >= 1) {
                        _pci_tagprintf(tag, "_pci_conf_write: bad reg 0x%x\r\n", reg);
                }
                return;
    }

    _pci_break_tag (tag, &bus, &device, &function);

    if (bus == 0) {
                /* Type 0 configuration on onboard PCI bus */
                if (device > 20 || function > 7)
                return;         /* device out of range */
    } else {
                /* Type 1 configuration on offboard PCI bus */
                if (bus > 255 || device > 31 || function > 7)
                return; /* device out of range */
    }

        IT_WRITE(IT_CONFADDR,
                (bus << IT_BUSNUM_SHF) |
                (device << IT_DEVNUM_SHF) |
                (function << IT_FUNCNUM_SHF) | (reg & ~0x03));

        IT_WRITE(IT_CONFDATA, data);

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

