/*	$NetBSD: bus_dma.c,v 1.13 1999/11/13 00:30:38 thorpej Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/mbuf.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pci/pcivar.h>

int	_dmamap_load_buffer __P((bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct proc *, int, paddr_t *, int *, int));

struct tgt_bus_dma_tag bus_dmamap_tag = {
	NULL,	/* cookie... */
	_dmamap_create,
	_dmamap_destroy,
	_dmamap_load,
	_dmamap_load_mbuf,
	NULL,	/* _dmamap_load_uio */
	NULL,	/* _dmamap_load_raw */
	_dmamap_unload,
	_dmamap_sync,
	_dmamem_alloc,
	_dmamem_free,
	_dmamem_map,
	_dmamem_unmap,
	NULL,	/* _dmamem_mmap */
	NULL	/* _dmamap_offs */
};

#if defined(NEWPCIROOT)
#define BPADDR(dm, x)	(VA_TO_PA(x) + dm->_dmamap_offs)
#define BVADDR(dm, x)	(PA_TO_VA((x) - dm->_dmamap_offs))
#else
#define BPADDR(dm, x)	VA_TO_PA(x)
#define BVADDR(dm, x)	PA_TO_VA(x)
#endif
	
/*
 * Common function for DMA map creation.  May be called by bus-specific
 * DMA map creation functions.
 */
int
_dmamap_create(t, size, nsegments, maxsegsz, boundary, flags, dmamp)
	void *t;
	bus_size_t size;
	int nsegments;
	bus_size_t maxsegsz;
	bus_size_t boundary;
	int flags;
	bus_dmamap_t *dmamp;
{
	struct tgt_bus_dmamap *map;
	void *mapstore;
	size_t mapsize;

	/*
	 * Allocate and initialize the DMA map.  The end of the map
	 * is a variable-sized array of segments, so we allocate enough
	 * room for them in one shot.
	 *
	 * Note we don't preserve the WAITOK or NOWAIT flags.  Preservation
	 * of ALLOCNOW notifies others that we've reserved these resources,
	 * and they are not to be freed.
	 *
	 * The bus_dmamap_t includes one bus_dma_segment_t, hence
	 * the (nsegments - 1).
	 */
	mapsize = sizeof(struct tgt_bus_dmamap) +
	    (sizeof(bus_dma_segment_t) * (nsegments - 1));
	if ((mapstore = malloc(mapsize, M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	bzero(mapstore, mapsize);
	map = (struct tgt_bus_dmamap *)mapstore;
	map->_dm_size = size;
	map->_dm_segcnt = nsegments;
	map->_dm_maxsegsz = maxsegsz;
	map->_dm_boundary = boundary;
	map->_dm_flags = flags & ~(BUS_DMA_WAITOK|BUS_DMA_NOWAIT);
	map->dm_nsegs = 0;

	*dmamp = map;
	return (0);
}

/*
 * Common function for DMA map destruction.  May be called by bus-specific
 * DMA map destruction functions.
 */
void
_dmamap_destroy(t, map)
	void *t;
	bus_dmamap_t map;
{

	free(map, M_DMAMAP);
}

/*
 * Utility function to load a linear buffer.  lastaddrp holds state
 * between invocations (for multiple-buffer loads).  segp contains
 * the starting segment on entrance, and the ending segment on exit.
 * first indicates if this is the first invocation of this function.
 */
int
_dmamap_load_buffer(t, map, buf, buflen, p, flags, lastaddrp, segp, first)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
	paddr_t *lastaddrp;
	int *segp;
	int first;
{
	bus_size_t sgsize;
	bus_addr_t curaddr, lastaddr, baddr, bmask;
	vaddr_t vaddr = (vaddr_t)buf;
	vaddr_t daddr;
	int seg;

	if(p) {
		panic("_dmamap_load_buffer: proc?");
	}

	lastaddr = *lastaddrp;
	bmask = ~(map->_dm_boundary - 1);

	for (seg = *segp; buflen > 0 ; ) {

		/*
		 * Compute the segment size, and adjust counts.
		 */
		sgsize = NBPG - ((u_long)vaddr & PGOFSET);
		if (buflen < sgsize)
			sgsize = buflen;

		curaddr = BPADDR(t, vaddr);
		daddr = 0;
#if defined(NON_DMA_REGIONS)
		if (!IS_DMA_ABLE(vaddr)) {
			daddr = (vaddr_t)malloc(sgsize, M_DMAMAP,
			    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK);
			curaddr = BPADDR(t, daddr);
		}
#endif
		/*
		 * Make sure we don't cross any boundaries.
		 */
		if (map->_dm_boundary > 0) {
			baddr = (curaddr + map->_dm_boundary) & bmask;
			if (sgsize > (baddr - curaddr))
				sgsize = (baddr - curaddr);
		}

		/*
		 * Insert chunk into a segment, coalescing with
		 * the previous segment if possible.
		 */
		if (first) {
			map->dm_segs[seg].ds_raddr = vaddr;
			map->dm_segs[seg].ds_daddr = daddr;
			map->dm_segs[seg].ds_addr = curaddr;
			map->dm_segs[seg].ds_len = sgsize;
			first = 0;
		} else {
			if (map->dm_segs[seg].ds_daddr == 0 &&
			    curaddr == lastaddr &&
			    (map->dm_segs[seg].ds_len + sgsize) <=
			     map->_dm_maxsegsz &&
			    (map->_dm_boundary == 0 ||
			     (map->dm_segs[seg].ds_addr & bmask) ==
			     (curaddr & bmask)))
				map->dm_segs[seg].ds_len += sgsize;
			else {
				if (++seg >= map->_dm_segcnt)
					break;
				map->dm_segs[seg].ds_raddr = vaddr;
				map->dm_segs[seg].ds_daddr = daddr;
				map->dm_segs[seg].ds_addr = curaddr;
				map->dm_segs[seg].ds_len = sgsize;
			}
		}

		lastaddr = curaddr + sgsize;
		vaddr += sgsize;
		buflen -= sgsize;
	}

	*segp = seg;
	*lastaddrp = lastaddr;

	/*
	 * Did we fit?
	 */
	if (buflen != 0)
		return (EFBIG);		/* XXX better return value here? */

	return (0);
}

/*
 * Common function for loading a DMA map with a linear buffer.  May
 * be called by bus-specific DMA map load functions.
 */
int
_dmamap_load(t, map, buf, buflen, p, flags)
	void *t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	paddr_t lastaddr;
	int seg, error;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_nsegs = 0;

	if (buflen > map->_dm_size)
		return (EINVAL);

	seg = 0;
	error = _dmamap_load_buffer(t, map, buf, buflen, p, flags,
		&lastaddr, &seg, 1);
	if (error == 0) {
		map->dm_nsegs = seg + 1;
	}
	return (error);
}

/*
 * Like _dmamap_load(), but for mbufs.
 */
int
_dmamap_load_mbuf(t, map, m0, flags)
	void *t;
	bus_dmamap_t map;
	struct mbuf *m0;
	int flags;
{
	paddr_t lastaddr;
	int seg, error, first;
	struct mbuf *m;

	/*
	 * Make sure that on error condition we return "no valid mappings."
	 */
	map->dm_nsegs = 0;

#ifdef DIAGNOSTIC
	if ((m0->m_flags & M_PKTHDR) == 0)
		panic("_dmamap_load_mbuf: no packet header");
#endif

	if (m0->m_pkthdr.len > map->_dm_size)
		return (EINVAL);

	first = 1;
	seg = 0;
	error = 0;
	for (m = m0; m != NULL && error == 0; m = m->m_next) {
		error = _dmamap_load_buffer(t, map, m->m_data, m->m_len,
		    NULL, flags, &lastaddr, &seg, first);
		first = 0;
	}
	if (error == 0) {
		map->dm_nsegs = seg + 1;
	}
	return (error);
}

/*
 * Common function for unloading a DMA map.  May be called by
 * chipset-specific DMA map unload functions.
 */
void
_dmamap_unload(t, map)
	void *t;
	bus_dmamap_t map;
{
#if defined(NON_DMA_REGIONS)
	int i;
	for (i = map->dm_nsegs; i--; ) {
		if (map->dm_segs[i].ds_daddr)
			free((void *)map->dm_segs[i].ds_daddr, M_DMAMAP);
	}
#endif
	map->dm_nsegs = 0;
}

/*
 * Common function for DMA map synchronization.  May be called
 * by chipset-specific DMA map synchronization functions.
 */
void
_dmamap_sync(t, map, offset, len, op)
	void *t;
	bus_dmamap_t map;
	bus_addr_t offset;
	bus_size_t len;
	int op;
{
#if defined(NOSNOOP) || defined(NON_DMA_REGIONS)
	bus_dma_tag_t tp;
        int i;

	tp = t;
	if(op & BUS_DMASYNC_PREWRITE) {
		for (i = map->dm_nsegs; i--; ) {
#if defined(NON_DMA_REGIONS)
			if (map->dm_segs[i].ds_daddr) {
				bcopy((void *)map->dm_segs[i].ds_raddr,
				    (void *)map->dm_segs[i].ds_daddr,
				    map->dm_segs[i].ds_len);
			}
#endif
#if defined(NOSNOOP)
			CACHESYNC((void *)BVADDR(tp, map->dm_segs[i].ds_addr),
				  map->dm_segs[i].ds_len, SYNC_W);
		}
#endif
	}
	if(op & BUS_DMASYNC_POSTWRITE) {
		/* Nothing (placeholder) */
	}
#if defined(NOSNOOP)
	if(op & (BUS_DMASYNC_PREREAD | BUS_DMASYNC_POSTREAD)) {
		for (i = map->dm_nsegs; i--; ) {
			CACHESYNC((void *)BVADDR(tp, map->dm_segs[i].ds_addr),
				  map->dm_segs[i].ds_len, SYNC_R);
		}
	}
#endif
#if defined(NON_DMA_REGIONS)
	if(op & BUS_DMASYNC_POSTREAD) {
		for (i = map->dm_nsegs; i--; ) {
			if (map->dm_segs[i].ds_daddr) {
				bcopy((void *)map->dm_segs[i].ds_daddr,
				    (void *)map->dm_segs[i].ds_raddr,
				    map->dm_segs[i].ds_len);
			}
		}
	}
#endif /* NON_DMA_REGIONS */
#endif /* NOSNOOP || NON_DMA_REGIONS */
}

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */
int
_dmamem_alloc(t, size, alignment, boundary, segs, nsegs, rsegs, flags)
	void *t;
	bus_size_t size, alignment, boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
{
	paddr_t curaddr;
	int curseg;

	/* Always round the size. */
	size = round_page(size);

	if ((curaddr = (paddr_t)malloc(size, M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	curseg = 0;

	segs[curseg].ds_addr = curaddr;
	segs[curseg].ds_len = size;

	*rsegs = curseg + 1;

	return (0);
}

/*
 * Common function for freeing DMA-safe memory.  May be called by
 * bus-specific DMA memory free functions.
 */
void
_dmamem_free(t, segs, nsegs)
	void *t;
	bus_dma_segment_t *segs;
	int nsegs;
{
	bus_addr_t addr;
	int curseg;

	for (curseg = 0; curseg < nsegs; curseg++) {
		free((void *)addr, M_DMAMAP);
	}
}

/*
 * Common function for mapping DMA-safe memory.  May be called by
 * bus-specific DMA memory map functions.
 */
int
_dmamem_map(t, segs, nsegs, size, kvap, flags)
	void *t;
	bus_dma_segment_t *segs;
	int nsegs;
	size_t size;
	caddr_t *kvap;
	int flags;
{
	*kvap = (caddr_t)segs[0].ds_addr;

	if(nsegs > 1) {
		printf("Whoa!!! bus_dmamem_map: more than one segment!!!\n");
	}

	return (0);
}

/*
 * Common function for unmapping DMA-safe memory.  May be called by
 * bus-specific DMA memory unmapping functions.
 */
void
_dmamem_unmap(t, kva, size)
	void *t;
	caddr_t kva;
	size_t size;
{
}

