/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Inline assembly cache operations.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 * Copyright (C) 1997 - 2002 Ralf Baechle (ralf@gnu.org)
 * Copyright (C) 2004 Ralf Baechle (ralf@linux-mips.org)
 */
#ifndef _ASM_R4KCACHE_H
#define _ASM_R4KCACHE_H
#define CKSEG0 0xffffffff80000000ULL
#ifndef __STR
#define __STR(x) #x
#endif
#ifndef STR
#define STR(x) __STR(x)
#endif

#ifndef cpu_dcache_line_size
#define cpu_dcache_line_size()	current_cpu_data.dcache.linesz
#endif
#ifndef cpu_icache_line_size
#define cpu_icache_line_size()	current_cpu_data.icache.linesz
#endif
#ifndef cpu_scache_line_size
#define cpu_scache_line_size()	current_cpu_data.scache.linesz
#endif

struct cache_desc {
	unsigned short linesz;	/* Size of line in bytes */
	unsigned short ways;	/* Number of ways */
	unsigned short sets;	/* Number of lines per set */
	unsigned int waysize;	/* Bytes per way */
	unsigned int waybit;	/* Bits to select in a cache set */
	unsigned int flags;	/* Flags describing cache properties */
};

struct cpuinfo_mips {
	struct cache_desc	icache;	/* Primary I-cache */
	struct cache_desc	dcache;	/* Primary D or combined I/D cache */
	struct cache_desc	scache;	/* Secondary cache */
	struct cache_desc	tcache;	/* Tertiary/split secondary cache */
} current_cpu_data=
{
{32,4,64*1024/32/4,64*1024/4,0,0},
{32,4,64*1024/32/4,64*1024/4,0,0},
{32,4,512*1024/32/4,512*1024/4,0,0},
{0,},
};

//#include <asm/asm.h>
#include "cacheops.h"
//#include <asm/cpu-features.h>
//#include <asm/mipsmtregs.h>

/*
 * This macro return a properly sign-extended address suitable as base address
 * for indexed cache operations.  Two issues here:
 *
 *  - The MIPS32 and MIPS64 specs permit an implementation to directly derive
 *    the index bits from the virtual address.  This breaks with tradition
 *    set by the R4000.  To keep unpleasant surprises from happening we pick
 *    an address in KSEG0 / CKSEG0.
 *  - We need a properly sign extended address for 64-bit code.  To get away
 *    without ifdefs we let the compiler do it by a type cast.
 */
#define INDEX_BASE	CKSEG0

#define cache_op(op,addr)						\
	__asm__ __volatile__(						\
	"	.set	push					\n"	\
	"	.set	noreorder				\n"	\
	"	.set	mips3\n\t				\n"	\
	"	cache	%0, %1					\n"	\
	"	.set	pop					\n"	\
	:								\
	: "i" (op), "R" (*(unsigned char *)(addr)))

#ifdef CONFIG_MIPS_MT
/*
 * Temporary hacks for SMTC debug. Optionally force single-threaded
 * execution during I-cache flushes.
 */

#define PROTECT_CACHE_FLUSHES 1

#ifdef PROTECT_CACHE_FLUSHES

extern int mt_protiflush;
extern int mt_protdflush;
extern void mt_cflush_lockdown(void);
extern void mt_cflush_release(void);

#define BEGIN_MT_IPROT \
	unsigned long long flags = 0;			\
	unsigned long long mtflags = 0;			\
	if(mt_protiflush) {				\
		local_irq_save(flags);			\
		ehb();					\
		mtflags = dvpe();			\
		mt_cflush_lockdown();			\
	}

#define END_MT_IPROT \
	if(mt_protiflush) {				\
		mt_cflush_release();			\
		evpe(mtflags);				\
		local_irq_restore(flags);		\
	}

#define BEGIN_MT_DPROT \
	unsigned long long flags = 0;			\
	unsigned long long mtflags = 0;			\
	if(mt_protdflush) {				\
		local_irq_save(flags);			\
		ehb();					\
		mtflags = dvpe();			\
		mt_cflush_lockdown();			\
	}

#define END_MT_DPROT \
	if(mt_protdflush) {				\
		mt_cflush_release();			\
		evpe(mtflags);				\
		local_irq_restore(flags);		\
	}

#else

#define BEGIN_MT_IPROT
#define BEGIN_MT_DPROT
#define END_MT_IPROT
#define END_MT_DPROT

#endif /* PROTECT_CACHE_FLUSHES */

#define __iflush_prologue						\
	unsigned long long redundance;					\
	extern int mt_n_iflushes;					\
	BEGIN_MT_IPROT							\
	for (redundance = 0; redundance < mt_n_iflushes; redundance++) {

#define __iflush_epilogue						\
	END_MT_IPROT							\
	}

#define __dflush_prologue						\
	unsigned long long redundance;					\
	extern int mt_n_dflushes;					\
	BEGIN_MT_DPROT							\
	for (redundance = 0; redundance < mt_n_dflushes; redundance++) {

#define __dflush_epilogue \
	END_MT_DPROT	 \
	}

#define __inv_dflush_prologue __dflush_prologue
#define __inv_dflush_epilogue __dflush_epilogue
#define __sflush_prologue {
#define __sflush_epilogue }
#define __inv_sflush_prologue __sflush_prologue
#define __inv_sflush_epilogue __sflush_epilogue

#else /* CONFIG_MIPS_MT */

#define __iflush_prologue {
#define __iflush_epilogue }
#define __dflush_prologue {
#define __dflush_epilogue }
#define __inv_dflush_prologue {
#define __inv_dflush_epilogue }
#define __sflush_prologue {
#define __sflush_epilogue }
#define __inv_sflush_prologue {
#define __inv_sflush_epilogue }

#endif /* CONFIG_MIPS_MT */

static inline void flush_icache_line_indexed(unsigned long long addr)
{
	__iflush_prologue
	cache_op(Index_Invalidate_I, addr);
	__iflush_epilogue
}

static inline void flush_dcache_line_indexed(unsigned long long addr)
{
	__dflush_prologue
	cache_op(Index_Writeback_Inv_D, addr);
	__dflush_epilogue
}

static inline void flush_scache_line_indexed(unsigned long long addr)
{
	cache_op(Index_Writeback_Inv_SD, addr);
}

static inline void flush_icache_line(unsigned long long addr)
{
	__iflush_prologue
	cache_op(Hit_Invalidate_I, addr);
	__iflush_epilogue
}

static inline void flush_dcache_line(unsigned long long addr)
{
	__dflush_prologue
	cache_op(Hit_Writeback_Inv_D, addr);
	__dflush_epilogue
}

static inline void invalidate_dcache_line(unsigned long long addr)
{
	__dflush_prologue
	cache_op(Hit_Invalidate_D, addr);
	__dflush_epilogue
}

static inline void invalidate_scache_line(unsigned long long addr)
{
	cache_op(Hit_Invalidate_SD, addr);
}

static inline void flush_scache_line(unsigned long long addr)
{
	cache_op(Hit_Writeback_Inv_SD, addr);
}

#define protected_cache_op(op,addr)				\
	__asm__ __volatile__(					\
	"	.set	push			\n"		\
	"	.set	noreorder		\n"		\
	"	.set	mips3			\n"		\
	"1:	cache	%0, (%1)		\n"		\
	"2:	.set	pop			\n"		\
	"	.section __ex_table,\"a\"	\n"		\
	"	"STR(PTR)" 1b, 2b		\n"		\
	"	.previous"					\
	:							\
	: "i" (op), "r" (addr))

/*
 * The next two are for badland addresses like signal trampolines.
 */
static inline void protected_flush_icache_line(unsigned long long addr)
{
	protected_cache_op(Hit_Invalidate_I, addr);
}

/*
 * R10000 / R12000 hazard - these processors don't support the Hit_Writeback_D
 * cacheop so we use Hit_Writeback_Inv_D which is supported by all R4000-style
 * caches.  We're talking about one cacheline unnecessarily getting invalidated
 * here so the penalty isn't overly hard.
 */
static inline void protected_writeback_dcache_line(unsigned long long addr)
{
	protected_cache_op(Hit_Writeback_Inv_D, addr);
}

static inline void protected_writeback_scache_line(unsigned long long addr)
{
	protected_cache_op(Hit_Writeback_Inv_SD, addr);
}

/*
 * This one is RM7000-specific
 */
static inline void invalidate_tcache_page(unsigned long long addr)
{
	cache_op(Page_Invalidate_T, addr);
}

#define cache16_unroll32(base,op)					\
	__asm__ __volatile__(						\
	"	.set push					\n"	\
	"	.set noreorder					\n"	\
	"	.set mips3					\n"	\
	"	cache %1, 0x000(%0); cache %1, 0x010(%0)	\n"	\
	"	cache %1, 0x020(%0); cache %1, 0x030(%0)	\n"	\
	"	cache %1, 0x040(%0); cache %1, 0x050(%0)	\n"	\
	"	cache %1, 0x060(%0); cache %1, 0x070(%0)	\n"	\
	"	cache %1, 0x080(%0); cache %1, 0x090(%0)	\n"	\
	"	cache %1, 0x0a0(%0); cache %1, 0x0b0(%0)	\n"	\
	"	cache %1, 0x0c0(%0); cache %1, 0x0d0(%0)	\n"	\
	"	cache %1, 0x0e0(%0); cache %1, 0x0f0(%0)	\n"	\
	"	cache %1, 0x100(%0); cache %1, 0x110(%0)	\n"	\
	"	cache %1, 0x120(%0); cache %1, 0x130(%0)	\n"	\
	"	cache %1, 0x140(%0); cache %1, 0x150(%0)	\n"	\
	"	cache %1, 0x160(%0); cache %1, 0x170(%0)	\n"	\
	"	cache %1, 0x180(%0); cache %1, 0x190(%0)	\n"	\
	"	cache %1, 0x1a0(%0); cache %1, 0x1b0(%0)	\n"	\
	"	cache %1, 0x1c0(%0); cache %1, 0x1d0(%0)	\n"	\
	"	cache %1, 0x1e0(%0); cache %1, 0x1f0(%0)	\n"	\
	"	.set pop					\n"	\
		:							\
		: "r" (base),						\
		  "i" (op));

#define cache32_unroll32(base,op)					\
	__asm__ __volatile__(						\
	"	.set push					\n"	\
	"	.set noreorder					\n"	\
	"	.set mips3					\n"	\
	"	cache %1, 0x000(%0); cache %1, 0x020(%0)	\n"	\
	"	cache %1, 0x040(%0); cache %1, 0x060(%0)	\n"	\
	"	cache %1, 0x080(%0); cache %1, 0x0a0(%0)	\n"	\
	"	cache %1, 0x0c0(%0); cache %1, 0x0e0(%0)	\n"	\
	"	cache %1, 0x100(%0); cache %1, 0x120(%0)	\n"	\
	"	cache %1, 0x140(%0); cache %1, 0x160(%0)	\n"	\
	"	cache %1, 0x180(%0); cache %1, 0x1a0(%0)	\n"	\
	"	cache %1, 0x1c0(%0); cache %1, 0x1e0(%0)	\n"	\
	"	cache %1, 0x200(%0); cache %1, 0x220(%0)	\n"	\
	"	cache %1, 0x240(%0); cache %1, 0x260(%0)	\n"	\
	"	cache %1, 0x280(%0); cache %1, 0x2a0(%0)	\n"	\
	"	cache %1, 0x2c0(%0); cache %1, 0x2e0(%0)	\n"	\
	"	cache %1, 0x300(%0); cache %1, 0x320(%0)	\n"	\
	"	cache %1, 0x340(%0); cache %1, 0x360(%0)	\n"	\
	"	cache %1, 0x380(%0); cache %1, 0x3a0(%0)	\n"	\
	"	cache %1, 0x3c0(%0); cache %1, 0x3e0(%0)	\n"	\
	"	.set pop					\n"	\
		:							\
		: "r" (base),						\
		  "i" (op));

#define cache64_unroll32(base,op)					\
	__asm__ __volatile__(						\
	"	.set push					\n"	\
	"	.set noreorder					\n"	\
	"	.set mips3					\n"	\
	"	cache %1, 0x000(%0); cache %1, 0x040(%0)	\n"	\
	"	cache %1, 0x080(%0); cache %1, 0x0c0(%0)	\n"	\
	"	cache %1, 0x100(%0); cache %1, 0x140(%0)	\n"	\
	"	cache %1, 0x180(%0); cache %1, 0x1c0(%0)	\n"	\
	"	cache %1, 0x200(%0); cache %1, 0x240(%0)	\n"	\
	"	cache %1, 0x280(%0); cache %1, 0x2c0(%0)	\n"	\
	"	cache %1, 0x300(%0); cache %1, 0x340(%0)	\n"	\
	"	cache %1, 0x380(%0); cache %1, 0x3c0(%0)	\n"	\
	"	cache %1, 0x400(%0); cache %1, 0x440(%0)	\n"	\
	"	cache %1, 0x480(%0); cache %1, 0x4c0(%0)	\n"	\
	"	cache %1, 0x500(%0); cache %1, 0x540(%0)	\n"	\
	"	cache %1, 0x580(%0); cache %1, 0x5c0(%0)	\n"	\
	"	cache %1, 0x600(%0); cache %1, 0x640(%0)	\n"	\
	"	cache %1, 0x680(%0); cache %1, 0x6c0(%0)	\n"	\
	"	cache %1, 0x700(%0); cache %1, 0x740(%0)	\n"	\
	"	cache %1, 0x780(%0); cache %1, 0x7c0(%0)	\n"	\
	"	.set pop					\n"	\
		:							\
		: "r" (base),						\
		  "i" (op));

#define cache128_unroll32(base,op)					\
	__asm__ __volatile__(						\
	"	.set push					\n"	\
	"	.set noreorder					\n"	\
	"	.set mips3					\n"	\
	"	cache %1, 0x000(%0); cache %1, 0x080(%0)	\n"	\
	"	cache %1, 0x100(%0); cache %1, 0x180(%0)	\n"	\
	"	cache %1, 0x200(%0); cache %1, 0x280(%0)	\n"	\
	"	cache %1, 0x300(%0); cache %1, 0x380(%0)	\n"	\
	"	cache %1, 0x400(%0); cache %1, 0x480(%0)	\n"	\
	"	cache %1, 0x500(%0); cache %1, 0x580(%0)	\n"	\
	"	cache %1, 0x600(%0); cache %1, 0x680(%0)	\n"	\
	"	cache %1, 0x700(%0); cache %1, 0x780(%0)	\n"	\
	"	cache %1, 0x800(%0); cache %1, 0x880(%0)	\n"	\
	"	cache %1, 0x900(%0); cache %1, 0x980(%0)	\n"	\
	"	cache %1, 0xa00(%0); cache %1, 0xa80(%0)	\n"	\
	"	cache %1, 0xb00(%0); cache %1, 0xb80(%0)	\n"	\
	"	cache %1, 0xc00(%0); cache %1, 0xc80(%0)	\n"	\
	"	cache %1, 0xd00(%0); cache %1, 0xd80(%0)	\n"	\
	"	cache %1, 0xe00(%0); cache %1, 0xe80(%0)	\n"	\
	"	cache %1, 0xf00(%0); cache %1, 0xf80(%0)	\n"	\
	"	.set pop					\n"	\
		:							\
		: "r" (base),						\
		  "i" (op));

/* build blast_xxx, blast_xxx_page, blast_xxx_page_indexed */
#define __BUILD_BLAST_CACHE(pfx, desc, indexop, hitop, lsize) \
static inline void blast_##pfx##cache##lsize(void)			\
{									\
	unsigned long long start = INDEX_BASE;				\
	unsigned long long end = start + current_cpu_data.desc.waysize;	\
	unsigned long long ws_inc = 1UL << current_cpu_data.desc.waybit;	\
	unsigned long long ws_end = current_cpu_data.desc.ways <<		\
	                       current_cpu_data.desc.waybit;		\
	unsigned long long ws, addr;						\
									\
	__##pfx##flush_prologue						\
									\
	for (ws = 0; ws < ws_end; ws += ws_inc)				\
		for (addr = start; addr < end; addr += lsize * 32)	\
			cache##lsize##_unroll32(addr|ws,indexop);	\
									\
	__##pfx##flush_epilogue						\
}									\
									\
static inline void blast_##pfx##cache##lsize##_page(unsigned long long page)	\
{									\
	unsigned long long start = page;					\
	unsigned long long end = page + PAGE_SIZE;				\
									\
	__##pfx##flush_prologue						\
									\
	do {								\
		cache##lsize##_unroll32(start,hitop);			\
		start += lsize * 32;					\
	} while (start < end);						\
									\
	__##pfx##flush_epilogue						\
}									\
									\
static inline void blast_##pfx##cache##lsize##_page_indexed(unsigned long long page) \
{									\
	unsigned long long indexmask = current_cpu_data.desc.waysize - 1;	\
	unsigned long long start = INDEX_BASE + (page & indexmask);		\
	unsigned long long end = start + PAGE_SIZE;				\
	unsigned long long ws_inc = 1UL << current_cpu_data.desc.waybit;	\
	unsigned long long ws_end = current_cpu_data.desc.ways <<		\
	                       current_cpu_data.desc.waybit;		\
	unsigned long long ws, addr;						\
									\
	__##pfx##flush_prologue						\
									\
	for (ws = 0; ws < ws_end; ws += ws_inc)				\
		for (addr = start; addr < end; addr += lsize * 32)	\
			cache##lsize##_unroll32(addr|ws,indexop);	\
									\
	__##pfx##flush_epilogue						\
}

__BUILD_BLAST_CACHE(d, dcache, Index_Writeback_Inv_D, Hit_Writeback_Inv_D, 16)
__BUILD_BLAST_CACHE(i, icache, Index_Invalidate_I, Hit_Invalidate_I, 16)
__BUILD_BLAST_CACHE(s, scache, Index_Writeback_Inv_SD, Hit_Writeback_Inv_SD, 16)
__BUILD_BLAST_CACHE(d, dcache, Index_Writeback_Inv_D, Hit_Writeback_Inv_D, 32)
__BUILD_BLAST_CACHE(i, icache, Index_Invalidate_I, Hit_Invalidate_I, 32)
__BUILD_BLAST_CACHE(s, scache, Index_Writeback_Inv_SD, Hit_Writeback_Inv_SD, 32)
__BUILD_BLAST_CACHE(i, icache, Index_Invalidate_I, Hit_Invalidate_I, 64)
__BUILD_BLAST_CACHE(s, scache, Index_Writeback_Inv_SD, Hit_Writeback_Inv_SD, 64)
__BUILD_BLAST_CACHE(s, scache, Index_Writeback_Inv_SD, Hit_Writeback_Inv_SD, 128)

/* build blast_xxx_range, protected_blast_xxx_range */
#define __BUILD_BLAST_CACHE_RANGE(pfx, desc, hitop, prot) \
static inline void prot##blast_##pfx##cache##_range(unsigned long long start, \
						    unsigned long long end)	\
{									\
	unsigned long long lsize = cpu_##desc##_line_size();			\
	unsigned long long addr = start & ~(lsize - 1);			\
	unsigned long long aend = (end - 1) & ~(lsize - 1);			\
									\
	__##pfx##flush_prologue						\
									\
	while (1) {							\
		prot##cache_op(hitop, addr);				\
		if (addr == aend)					\
			break;						\
		addr += lsize;						\
	}								\
									\
	__##pfx##flush_epilogue						\
}

__BUILD_BLAST_CACHE_RANGE(d, dcache, Hit_Writeback_Inv_D, protected_)
__BUILD_BLAST_CACHE_RANGE(s, scache, Hit_Writeback_Inv_SD, protected_)
__BUILD_BLAST_CACHE_RANGE(i, icache, Hit_Invalidate_I, protected_)
__BUILD_BLAST_CACHE_RANGE(d, dcache, Hit_Writeback_Inv_D, )
__BUILD_BLAST_CACHE_RANGE(s, scache, Hit_Writeback_Inv_SD, )
/* blast_inv_dcache_range */
__BUILD_BLAST_CACHE_RANGE(inv_d, dcache, Hit_Invalidate_D, )
__BUILD_BLAST_CACHE_RANGE(inv_s, scache, Hit_Invalidate_SD, )

static inline  void r4k_dma_cache_wback_inv(unsigned long long addr, unsigned long long size)
{
	/* Catch bad driver code */
	int scache_size=512*1024;

		if (size >= scache_size)
			blast_scache32();
		else
			blast_scache_range(addr, addr + size);
			blast_dcache32();
		return;

}
#endif /* _ASM_R4KCACHE_H */
