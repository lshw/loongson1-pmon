/*	$Id: pci_machdep.h,v 1.1.1.1 2006/09/14 01:59:09 root Exp $ */

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
 *	This product includes software developed by Opsycon AB.
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

#include <target/pmon_target.h>

#include <pmon.h>

/*
 * Types provided to machine-independent PCI code
 */
typedef struct arch_pci_chipset *pci_chipset_tag_t;
typedef u_long pcitag_t;
typedef u_long pci_intr_handle_t;

void	 _pci_flush __P((void));
int	 _pci_hwinit __P((int, bus_space_tag_t, bus_space_tag_t));
void	 _pci_hwreinit __P((void));
pcitag_t _pci_make_tag __P((int, int, int));
void	 _pci_break_tag __P((pcitag_t, int *, int *, int *));
pcireg_t _pci_conf_readn __P((pcitag_t, int,int));
void	 _pci_conf_writen __P((pcitag_t, int, pcireg_t,int));
pcireg_t _pci_conf_read __P((pcitag_t, int));
void	 _pci_conf_write __P((pcitag_t, int, pcireg_t));
int	 _pci_map_port __P((pcitag_t, int, unsigned int *));
int	 _pci_map_io __P((pcitag_t, int, vm_offset_t *, vm_offset_t *));
int	 _pci_map_mem __P((pcitag_t, int, vm_offset_t *, vm_offset_t *));
void	*_pci_map_int __P((pcitag_t, int, int (*)(void *), void *));
void	 _pci_devinfo __P((pcireg_t, pcireg_t, int *, char *));
void	 _pci_businit __P((int));
void	 _pci_devinit __P((int));
vm_offset_t _pci_cpumap __P((vm_offset_t, unsigned int));
vm_offset_t _pci_dmamap __P((vm_offset_t, unsigned int));

void	 _pci_bdfprintf (int bus, int device, int function, const char *fmt, ...);
void	 _pci_tagprintf (pcitag_t tag, const char *fmt, ...);
int	 _pci_canscan __P((pcitag_t tag));

/* sigh... compatibility */
#define pci_hwinit			_pci_hwinit
#define pci_hwreinit			_pci_hwreinit
#define pci_make_tag			_pci_make_tag
#define pci_break_tag			_pci_break_tag
#define pci_conf_read(a, b, c)		_pci_conf_read(b, c)
#define pci_conf_write(a, b, c, d)	_pci_conf_write(b, c, d)
#define pci_map_port			_pci_map_port
#define pci_map_io			_pci_map_io
#define pci_map_mem			_pci_map_mem
#define pci_map_int			_pci_map_int
#define pci_devinfo			_pci_devinfo
#define pci_configure			_pci_configure
#define pci_allocate_mem		_pci_allocate_mem
#define pci_allocate_io			_pci_allocate_io
#define	vtophys(p)			_pci_dmamap((vm_offset_t)p, 1)
#define pci_decompose_tag(a, b, c, d, e)	_pci_break_tag(b, c, d, e)

#define	pci_intr_map(a, b, c, d, e)		(*e = -1, 0)
#define	pci_intr_string(a,b)			("generic poll")
#define	pci_intr_establish(a, b, c, d, e, f)	tgt_poll_register((c), (d), (e))
#define	pci_attach_hook(parent, self, pba)
#define	pci_bus_maxdevs(c, b)		32

int  pci_ether_hw_addr __P((void *sc, u_int8_t *, u_int8_t, u_int8_t));
void pci_sync_cache __P((void *, vm_offset_t, size_t, int));

#define CACHESYNC(a, l, h) pci_sync_cache((void *)NULL, (vm_offset_t)a, l ,h)

#define	NEED_PCI_SYNC_CACHE_FUNC

#define	SYNC_R	0	/* Sync caches for reading data */
#define	SYNC_W	1	/* Sync caches for writing data */

#define	PCI_FIRST_DEVICE 1		/* Which device to scan first */
#define	PCI_FIRST_BUS	0
#define	PCI_INT_0	0
#define	PCI_INT_A	11
#define	PCI_INT_B	10
#define	PCI_INT_C	9
#define	PCI_INT_D	8
#define	PCI_CACHE_LINE_SIZE 8		/* expressed in 32 bit words */

#define NEWPCIROOT
extern struct pci_device *_pci_bus[];

/*
 *  Any physical to virtual conversion CPU
 */
#define VA_TO_PA(x)     UNCACHED_TO_PHYS(x)
#define PA_TO_VA(x)     PHYS_TO_CACHED(x)
