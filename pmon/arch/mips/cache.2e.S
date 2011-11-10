/*	$Id: cache.2e.S,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 1998, 1999, 2000, 2001 Opsycon AB
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
#ifndef _KERNEL
#define _KERNEL
#endif

#include <machine/asm.h>
#include <machine/cpu.h>
#include <machine/regnum.h>

/*
 *  Skip the .h file. Noone else need to know!
 */

#define	IndexInvalidate_I	0x00
#define	IndexWBInvalidate_D	0x01
#define	HitInvalidate_I		0x10
#define	HitInvalidate_D		0x11
#define	HitWBInvalidate_D	0x15

#define	IndexWBInvalidate_S	0x03
#define	HitInvalidate_S		0x13
#define	HitWBInvalidate_S	0x17

#define	InvalidateSecondaryPage	0x17	/* Only RM527[0-1] */
#define	InvalidateTertiaryPage	0x16	/* Only RM7K */

#define	IndexStoreTag_T		0x0a	/* Only RM7K */
#define	IndexLoadTag_T		0x06	/* Only RM7K */

#define Index_Store_Tag_S       0x0B        /* 2       3 */

/*
 *  RM52xx config register bits.
 */
#define	CF_5_SE		(1 << 12)	/* Secondary cache enable */
#define	CF_5_SC		(1 << 17)	/* Secondary cache not present */
#define	CF_5_SS		(3 << 20)	/* Secondary cache size */
#define	CF_5_SS_AL	20		/* Shift to align */

/*
 *  RM7000 config register bits.
 */
#define	CF_7_SE		(1 << 3)	/* Secondary cache enable */
#define	CF_7_SC		(1 << 31)	/* Secondary cache not present */
#define	CF_7_TE		(1 << 12)	/* Tertiary cache enable */
#define	CF_7_TC		(1 << 17)	/* Tertiary cache not present */
#define	CF_7_TS		(3 << 20)	/* Tertiary cache size */
#define	CF_7_TS_AL	20		/* Shift to align */

/*
 *  Define cache type definition bits. NOTE! the 3 lsb may NOT change!
 */
#define	CTYPE_DIR		0x0001	/* Cache is direct mapped */
#define	CTYPE_2WAY		0x0002	/* Cache is TWO way */
#define	CTYPE_4WAY		0x0004	/* Cache is FOUR way */
#define	CTYPE_WAYMASK		0x0007

#define	CTYPE_HAS_L2		0x0100	/* On chip L2 Cache present */
#define	CTYPE_HAS_XL2		0x0200	/* Off chip L2 Cache present */
#define	CTYPE_HAS_XL3		0x0400	/* Off chip L3 Cache present */

#define NOP10	nop;nop;nop;nop;nop;\
		nop;nop;nop;nop;nop	/* Two cycles for dual issue machine */

	.set	noreorder		# Noreorder is default style!

/*----------------------------------------------------------------------------
 *
 * md_cachestat --
 *
 *	Return status of caches (on or off)
 *
 *	md_cachestat()
 *
 * Results:
 *	Returns cache status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
LEAF(md_cachestat)
	jr	ra
	li	v0, 1		/* Always ON */
END(md_cachestat)

/*----------------------------------------------------------------------------
 *
 * md_cacheon --
 *
 *	Turn on caches.
 *
 *	md_cacheon()
 *
 * Results:
 *	Nothing, caches on mips are always ON.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
LEAF(md_cacheon)
	jr	ra
	nop
END(md_cacheon)

/*----------------------------------------------------------------------------
 *
 * wbflush --
 *
 *	Return when the write buffer is empty.
 *
 *	wbflush()
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
LEAF(wbflush)
	nop
	sync
	j	ra
	nop
END(wbflush)


/*----------------------------------------------------------------------------
 *
 * CPU_ConfigCache --
 *
 *	Size and configure the caches.
 *	NOTE: should only be called from mips_init().
 *
 * Results:
 *	Returns the value of the cpu configuration register.
 *
 * Side effects:
 *	The size of the data cache is stored into CpuPrimaryDataCacheSize.
 *	The size of instruction cache is stored into CpuPrimaryInstCacheSize.
 *	Alignment mask for cache aliasing test is stored in CpuCacheAliasMask.
 *	CpuSecondaryCacheSize is set to the size of the secondary cache.
 *	CpuTertiaryCacheSize is set to the size of the tertiary cache.
 *	CpuNWayCache is set to 0 for direct mapped caches, 2 for two way
 *	caches and 4 for four way caches. This primarily indicates the
 *	primary cache associativity.
 *	cpu_id is set for later decision testing.
 *
 * Allocation:
 *	t4, t5 t6 used to hold I and D set size and Alias mask.
 *
 *----------------------------------------------------------------------------
 */


LEAF(CPU_ConfigCache)
	.set	noreorder
        la      t0, 1f				# run uncached
        or      t0, UNCACHED_MEMORY_ADDR
        j       t0
	nop
1: 
	mfc0	v1, COP_0_PRID			# read processor ID register
	mfc0	v0, COP_0_CONFIG		# Get configuration register
	sw	v1, CpuProcessorId		# save PRID register

	and	v1, 0xff00			# Recognize CPUs with
	
	srl	t1, v0, 9			# Get I cache size.
	and	t1, 7
	li	t2, 4096
	sll	t4, t2, 4	#t1			# t4 = Initial I set size.

	and	t2, v0, 0x20
	srl	t2, t2, 1			# Get I cache line size.
	addu	t2, t2, 16
	sw	t2, CpuPrimaryInstCacheLSize	

	srl	t1, v0, 6			# Get D cache size.
	and	t1, 7
	li	t2, 4096			# Fixed page size.
	sll	t5, t2, 4	#t1

	and	t2, v0, 0x10
	addu	t2, t2, 16			# Get D cache line size.
	sw	t2, CpuPrimaryDataCacheLSize

	li	t2, CTYPE_2WAY			# Assume two way cache
	li	t6, 0				# Secondary size 0.
	li	t7, 0				# Tertiary size 0.

	li	t1, (MIPS_R4600 << 8)		# N way L1 caches only.
	beq	v1, t1, ConfResult		# R4K 2 way, no L2 control	
	nop
	li	t1, (MIPS_R4700 << 8)	
	beq	v1, t1, ConfResult		# R4K 2 way, No L2 control
	nop
	li	t1, (MIPS_GODSON2 << 8)	
	li	t2, CTYPE_4WAY
	beq	v1, t1, ConfGodson2
	nop
	li	t1, (MIPS_GODSON1 << 8)	
	beq	v1, t1, ConfResult
	nop
	li	t1, (MIPS_R5000 << 8)
	beq	v1, t1, Conf5K			# R5K 2 way, check L2
	nop
	li	t1, (MIPS_RM52XX << 8)
	beq	v1, t1, Conf5K			# R5K 2 way, check L2
	nop
	li	t1, (MIPS_RM7000 << 8)
	beq	v1, t1, Conf7K
	nop
	li	t1, (MIPS_E9000 << 8)
	beq	v1, t1, Conf7K
	nop
						# R4000PC/R4400PC or unknown.
	li	t2, CTYPE_DIR			# default direct mapped cache
	b	ConfResult
	nop

#---- R5K ------------------------------
Conf5K:					# R5000 type, check for L2 cache
	and	t1, v0, CF_5_SC
	bnez	t1, ConfResult			# not present
	li	t6, 0				# set size to 0.

	li	t3, CF_5_SS
	and	t1, t3, v0
	beq	t1, t3, ConfResult		# No L2 cache
	srl	t1, CF_5_SS_AL

	or	t2, CTYPE_HAS_XL2		# External L2 present.
	li	t6, 512*1024			# 512k per 'click'.
	lw	t3, CpuExternalCacheOn		# Check if disabled
	bnez	t3, ConfResult			# No use it.
	sll	t6, t1

	and	t2, ~CTYPE_HAS_XL2
	li	t1, ~CF_5_SE			# Clear SE in conf
	and	v0, t1				# Update config register
	b	ConfResult
	li	t6, 0				# L2 cache disabled 


#---- RM7K -----------------------------
Conf7K:					# RM7000, check for L2 and L3 cache

	/* disable L2/L3 */
	and     v0,~(CF_7_TE|CF_7_TC|CF_7_SE|CF_7_SC)
	mtc0    v0,COP_0_CONFIG

#if 0
	li	t2, CTYPE_4WAY			# 4-way cache
	and	t1, v0, CF_7_TC
	bnez	t1, Conf7KL2			# No L3 cache if set
	li	t7, 0				# Set size = 0

#if defined(L3_SIZE_IN_CONFIG_REG)
	li	t3, CF_7_TS			# Figure out L3 size
	and	t1, t3, v0
	beq	t1, t3, Conf7KL2		# No L3 cache present
	srl	t1, CF_7_TS_AL
	li	t7, 512*1024			# else size = 512K ** N
	sll	t7, t1
#else
	lw	t7, CpuTertiaryCacheSize	# use what start.S provide.
	beqz	t7, Conf7KL2
	nop
#endif

	or	t2, CTYPE_HAS_XL3
	lw	t3, CpuExternalCacheOn		# Check if disabled
	bnez	t3, Conf7KL2			# No, use it
	nop

	and	t2, ~CTYPE_HAS_XL3
	and	v0, ~CF_7_TE			# Clear TE in conf
	mtc0	v0, COP_0_CONFIG		# establish any new config
	NOP10
	li	t7, 0				# L3 cache disabled 

Conf7KL2:
	and	t1, v0, CF_7_SC			# check for L2 cache
	bnez	t1, ConfResult
	li	t6, 0				# No L2?

	or	t2, CTYPE_HAS_L2		# L2 is on chip
	lw	t3, CpuOnboardCacheOn		# Check if disabled
	bnez	t3, ConfResult			# No, use it
	li	t6, 256*1024			# size = 256k

/* Flush on chip L2 */

	li	a0, 0x80000000 
	li	a1, 0x80040000
10:
	cache	IndexWBInvalidate_S, 0(a0)
	addu	a0, 32
	bne	a0, a1, 10b
	nop

	and	t2, ~CTYPE_HAS_L2
	li	t1, ~CF_7_SE			# Clear SE in conf
	and	v0, t1
	mtc0	v0, COP_0_CONFIG		# establish any new config
	NOP10
	b	ConfResult
	li	t6, 0				# L2 cache disabled
#endif

	li	t2, CTYPE_4WAY			# 4-way cache
	and	t2, ~CTYPE_HAS_L2
	li	t6, 0				# L2 cache disabled
	li	t7, 0				# L3 cache disabled

/*-------------------------------------------------------------------*/
ConfGodson2:
        /* disable L2/L3 */
        and     v0,~(CF_7_TE|CF_7_SE)
        mtc0    v0,COP_0_CONFIG

        #and     t1, v0, CF_7_TC
        #bnez    t1, ConfGodsonL2                    # No L3 cache if set
        #li      t7, 0                           # Set size = 0

        #lw      t7, CpuTertiaryCacheSize        # use what start.S provide.
        #beqz    t7, ConfGodsonL2
        #nop

        #or      t2, CTYPE_HAS_XL3
        #lw      t3, CpuExternalCacheOn          # Check if disabled
        #bnez    t3, ConfGodsonL2                    # No, use it
        #nop

        #and     t2, ~CTYPE_HAS_XL3
        #and     v0, ~CF_7_TE                    # Clear TE in conf
        #mtc0    v0, COP_0_CONFIG                # establish any new config
        #NOP10
        #li      t7, 0 
ConfGodsonL2:
        and     t1, v0, CF_7_SC                 # check for L2 cache
        bnez    t1, ConfResult
        li      t6, 0                           # No L2?

        or      t2, CTYPE_HAS_L2                # L2 is on chip
        li	t6, 512*1024
	#lw      t3, CpuOnboardCacheOn           # Check if disabled
        #bnez    t3, ConfResult                  # No, use it
        #li      t6, 256*1024                    # size = 256k

/* Flush on chip L2 */

        li      a0, 0x80000000
        li      a1, 0x80080000			# size = 512k
10:
        cache   IndexWBInvalidate_S, 0(a0)
        cache   IndexWBInvalidate_S, 1(a0)
        cache   IndexWBInvalidate_S, 2(a0)
        cache   IndexWBInvalidate_S, 3(a0)
        addu    a0, 32
        bne     a0, a1, 10b
        nop
	
        or	v0,CF_7_SE
        mtc0    v0,COP_0_CONFIG

        #and     t2, ~CTYPE_HAS_L2
        #li      t1, ~CF_7_SE                    # Clear SE in conf
        #and     v0, t1
        #mtc0    v0, COP_0_CONFIG                # establish any new config
        #NOP10
        #b       ConfResult
        #li      t6, 0                           # L2 cache disabled

        #li      t2, CTYPE_4WAY                  # 4-way cache
        #and     t2, ~CTYPE_HAS_L2
        #li      t6, 0                           # L2 cache disabled
        #li      t7, 0                           # L3 cache disabled
/*
 * Get here with t2 = Cache type, t4 = L1 I size, t5 = L1 D size.
 * t6 = secondary size, t7 = tertiary size.
 */
ConfResult:
	sw	v0, CpuConfigRegister
	mfc0	t3, COP_0_STATUS_REG
	sw	t3, CpuStatusRegister
	sw	t2, CpuCacheType		# Save cache attributes
	and	t2, CTYPE_WAYMASK		# isolate number of sets.
	sw	t2, CpuNWayCache
	srl	t2, 1				# get div shift for set size.

	sw	t6, CpuSecondaryCacheSize
	sw	t7, CpuTertiaryCacheSize

	addu	t1, t4, -1			# Use icache for alias mask
	srl	t1, t2				# Some cpus have different
	and	t1, ~(4096 - 1)			# i and d cache sizes...
	sw	t1, CpuCacheAliasMask
#ifdef CONFIG_CHACHE_64K_4WAY
	sll	t4,2
#endif
	sw	t4, CpuPrimaryInstCacheSize	# store cache size.
	srl	t4, t2				# calculate set size.
	sw	t4, CpuPrimaryInstSetSize

#ifdef CONFIG_CHACHE_64K_4WAY
	sll	t5,2
#endif
	sw	t5, CpuPrimaryDataCacheSize	# store cache size.
	srl	t5, t2				# calculate set size.
	sw	t5, CpuPrimaryDataSetSize

#if 1
	and	v0, 0xfffffff8
        or	v0, 0x00000003			# set coherency mode WB KSEG0

	mtc0	v0, COP_0_CONFIG		# establish any new config
	NOP10
#endif

	j	ra
	nop
END(CPU_ConfigCache)

/*----------------------------------------------------------------------------
 *
 * CPU_FlushCache --
 *
 *	Flush ALL caches.
 *	No need to look at number of sets since we are cleaning out
 *	the entire cache and thus will address all sets anyway.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of ALL caches are Invalidated or Flushed.
 *
 *----------------------------------------------------------------------------
 */
.set push
.set mips3
LEAF(CPU_FlushCache)
	.set	noreorder
	mfc0	t1, COP_0_PRID			# read processor ID register
	li		t0, 0x6302				#godson2e prid
	bne	t0, t1, not_godson2e
	nop
# godson2e
	li	t3, CACHED_MEMORY_ADDR
	lw	t4, CpuSecondaryCacheSize
10:
	cache	IndexWBInvalidate_S, 0(t3)
	cache	IndexWBInvalidate_S, 1(t3)
	cache	IndexWBInvalidate_S, 2(t3)
	cache	IndexWBInvalidate_S, 3(t3)
	subu	t4, 32				# Fixed cache line size.
	bgtz	t4, 10b
	addu	t3, 32
	b		99f
	nop
not_godson2e:
	lw	t1, CpuPrimaryInstCacheSize
	lw	t2, CpuPrimaryDataCacheSize
	lw	t3, CpuPrimaryInstCacheLSize
	lw	t4, CpuPrimaryDataCacheLSize

/*
 * Flush the instruction cache.
 */

	li	t0, CACHED_MEMORY_ADDR
	addu	t1, t0, t1			# Compute end address
	subu	t1, 128
	subu	t3, 16				# t3 = 0 if line size = 16

1:
	bne	t3, zero, 2f			# 32 byte line size
	cache	IndexInvalidate_I, 0(t0)

	cache	IndexInvalidate_I, 16(t0)	# 16 byte line size
	cache	IndexInvalidate_I, 48(t0)
	cache	IndexInvalidate_I, 80(t0)
	cache	IndexInvalidate_I, 112(t0)

2:
	cache	IndexInvalidate_I, 32(t0)
	cache	IndexInvalidate_I, 64(t0)
	cache	IndexInvalidate_I, 96(t0)

	bne	t0, t1, 1b
	addu	t0, t0, 128

/*
 * Flush the data cache.
 */
	li	t0, CACHED_MEMORY_ADDR
	addu	t1, t0, t2			# End address
	subu	t1, t1, 128
	subu	t4, 16				# t4 = 0 if line size = 16
1:
	bne	t4, zero, 2f			# 32 byte line size
	cache	IndexWBInvalidate_D, 0(t0)

	cache	IndexWBInvalidate_D, 16(t0)	# 16 byte line size
	cache	IndexWBInvalidate_D, 48(t0)
	cache	IndexWBInvalidate_D, 80(t0)
	cache	IndexWBInvalidate_D, 112(t0)

2:
	cache	IndexWBInvalidate_D, 32(t0)
	cache	IndexWBInvalidate_D, 64(t0)
	cache	IndexWBInvalidate_D, 96(t0)

#if 0
        lw      t5, CpuProcessorId
	li	t6, (MIPS_GODSON1<<8)
	beq	t5,t6,3f
	nop
	li      t6, (MIPS_GODSON2<<8)
	bne     t5,t6,non_godson2_2
	nop
#endif
3:
	cache	IndexWBInvalidate_D, 1(t0)
	cache	IndexWBInvalidate_D, 33(t0)
	cache	IndexWBInvalidate_D, 65(t0)
	cache	IndexWBInvalidate_D, 97(t0)
	
	cache	IndexWBInvalidate_D, 2(t0)
	cache	IndexWBInvalidate_D, 34(t0)
	cache	IndexWBInvalidate_D, 66(t0)
	cache	IndexWBInvalidate_D, 98(t0)
	
	cache	IndexWBInvalidate_D, 3(t0)
	cache	IndexWBInvalidate_D, 35(t0)
	cache	IndexWBInvalidate_D, 67(t0)
	cache	IndexWBInvalidate_D, 99(t0)
non_godson2_2:

	bne	t0, t1, 1b
	addu	t0, t0, 128

/* Do on chip L2 if present */
	lw	t0, CpuCacheType
	and	t0, CTYPE_HAS_L2
	beqz	t0, 20f
	nop

	li	t3, CACHED_MEMORY_ADDR
	lw	t4, CpuSecondaryCacheSize
10:
	cache	IndexWBInvalidate_S, 0(t3)
	subu	t4, 32				# Fixed cache line size.
	bgtz	t4, 10b
	addu	t3, 32

/* Do off chip L2 if present */
20:
	lw	t0, CpuCacheType
	and	t0, CTYPE_HAS_XL2
	beqz	t0, 30f
	nop

	mtc0    zero, COP_0_TAG_LO
	li	t3, CACHED_MEMORY_ADDR
	lw	t4, CpuSecondaryCacheSize
21:
	cache	InvalidateSecondaryPage, 0(t3)
	subu	t4, 4096			# Fixed cache page size.
	bgtz	t4, 21b
	addu	t3, 4096

/* Do off chip L3 if present */
30:
	lw	t0, CpuCacheType
	and	t0, CTYPE_HAS_XL3
	beqz	t0, 99f
	nop

	mtc0    zero, COP_0_TAG_LO
	li	t3, CACHED_MEMORY_ADDR
	lw	t4, CpuTertiaryCacheSize
31:
	cache	InvalidateTertiaryPage, 0(t3)
	subu	t4, 4096			# Fixed cache page size.
	bgtz	t4, 31b
	addu	t3, 4096

99:
	j	ra
	nop
END(CPU_FlushCache)

/*----------------------------------------------------------------------------
 *
 * CPU_FlushICache --
 *
 *	void CPU_FlushICache(addr, len)
 *		vm_offset_t addr, len;
 *
 *	Invalidate the L1 instruction cache for at least range
 *	of addr to addr + len - 1.
 *	The address is reduced to a KSEG0 index to avoid TLB faults.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of the L1 Instruction cache is flushed.
 *	Must not touch v0.
 *
 * Assumtions:
 *      If a cache is not direct mapped, line size is 32.
 *
 *----------------------------------------------------------------------------
 */
LEAF(CPU_FlushICache)
	lw	t0, CpuPrimaryInstSetSize	# Set size
	addu	a1, 127				# Align for unrolling
	and	a0, 0xffff80			# Align start address
	addu	a0, CACHED_MEMORY_ADDR		# a0 now new KSEG0 address
	srl	a1, a1, 7			# Number of unrolled loops
1:
	lw	v0, CpuNWayCache		# Cache properties
	addiu	v0, -2				# <0 1way, 0 = two, >0 four
	bgez	v0, 2f
	addu	a1, -1

	cache	IndexInvalidate_I, 16(a0)	# direct mapped
	cache	IndexInvalidate_I, 48(a0)
	cache	IndexInvalidate_I, 80(a0)
	b	3f
	cache	IndexInvalidate_I, 112(a0)

2:
	addu	t1, t0, a0			# Nway cache, flush set B.
	cache	IndexInvalidate_I, 0(t1)
	cache	IndexInvalidate_I, 32(t1)
	cache	IndexInvalidate_I, 64(t1)
	cache	IndexInvalidate_I, 96(t1)
	beqz	v0, 3f				# Is two way do set A
	addu	t1, t0				# else step to set C.

	cache	IndexInvalidate_I, 0(t1)
	cache	IndexInvalidate_I, 32(t1)
	cache	IndexInvalidate_I, 64(t1)
	cache	IndexInvalidate_I, 96(t1)

	addu	t1, t0				# step to set D
	cache	IndexInvalidate_I, 0(t1)
	cache	IndexInvalidate_I, 32(t1)
	cache	IndexInvalidate_I, 64(t1)
	cache	IndexInvalidate_I, 96(t1)

	addiu	v0, -2				# 0 = 4-way, >0 = 8-way
	beqz	v0, 3f				# If just 4-way, to set A
	addu	t1, t0				# else step to set E.

	cache	IndexInvalidate_I, 0(t1)
	cache	IndexInvalidate_I, 32(t1)
	cache	IndexInvalidate_I, 64(t1)
	cache	IndexInvalidate_I, 96(t1)

	addu	t1, t0				# step to set F
	cache	IndexInvalidate_I, 0(t1)
	cache	IndexInvalidate_I, 32(t1)
	cache	IndexInvalidate_I, 64(t1)
	cache	IndexInvalidate_I, 96(t1)

	addu	t1, t0				# step to set G
	cache	IndexInvalidate_I, 0(t1)
	cache	IndexInvalidate_I, 32(t1)
	cache	IndexInvalidate_I, 64(t1)
	cache	IndexInvalidate_I, 96(t1)

	addu	t1, t0				# step to set H
	cache	IndexInvalidate_I, 0(t1)
	cache	IndexInvalidate_I, 32(t1)
	cache	IndexInvalidate_I, 64(t1)
	cache	IndexInvalidate_I, 96(t1)

3:
	cache	IndexInvalidate_I, 0(a0)	# do set (A if NWay)
	cache	IndexInvalidate_I, 32(a0)
	cache	IndexInvalidate_I, 64(a0)
	cache	IndexInvalidate_I, 96(a0)

	bne	a1, zero, 1b
	addu	a0, 128

	j	ra
	move	v0, zero		# suiword depends on this!!
END(CPU_FlushICache)

/*----------------------------------------------------------------------------
 *
 * CPU_FlushDCache --
 *
 *	void CPU_FlushDCache(addr, len)
 *		vm_offset_t addr, len;
 *
 *	Flush the L1 data cache for index range of at least
 *	addr to addr + len - 1.
 *	The address is reduced to a KSEG0 index to avoid TLB faults.
 *	
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of the cache is written back to primary memory.
 *	The cache line is invalidated.
 *
 *----------------------------------------------------------------------------
 */
LEAF(CPU_FlushDCache)
	lw	a2, CpuPrimaryDataSetSize
	addiu	a3, a2, -1
	and	a0, a3				# get cache index
	addu	a0, CACHED_MEMORY_ADDR		# a0 now new KSEG0 address

	andi	a3, a0, 127
	addu	a1, a3				# compute extra size from
	subu	a0, a3				# alignment of address
	addiu	a1, 127
	blt	a1, a2, 1f			# flushing more than cache?
	srl	a1, a1, 7			# Compute number of cache lines

	srl	a1, a2, 7			# no need for more than size!
1:
	lw	v0, CpuNWayCache
	addiu	v0, -2				# <0 1way, 0 = two, >0 four
	bgez	v0, 2f				# NWay.. go flush sets
	addu	a1, -1

	cache	IndexWBInvalidate_D, 16(a0)	# direct mapped, assume 16 byte 
	cache	IndexWBInvalidate_D, 48(a0)	# linesize.
	cache	IndexWBInvalidate_D, 80(a0)
	b	3f
	cache	IndexWBInvalidate_D, 112(a0)

2:
	addu	t1, a0, a2			# flush set B.
	cache	IndexWBInvalidate_D, 0(t1)
	cache	IndexWBInvalidate_D, 32(t1)
	cache	IndexWBInvalidate_D, 64(t1)
	cache	IndexWBInvalidate_D, 96(t1)

	lw      t5, CpuProcessorId
	and     t5, 0xff00
	li      t6, (MIPS_GODSON1<<8)
	beq     t5,t6,3f
	nop
	li      t6, (MIPS_GODSON2<<8)
	bne     t5,t6,non_godson2_1
	nop
3:
	cache	IndexWBInvalidate_D, 1(t1)
	cache	IndexWBInvalidate_D, 33(t1)
	cache	IndexWBInvalidate_D, 65(t1)
	cache	IndexWBInvalidate_D, 97(t1)

	cache	IndexWBInvalidate_D, 2(t1)
	cache	IndexWBInvalidate_D, 34(t1)
	cache	IndexWBInvalidate_D, 66(t1)
	cache	IndexWBInvalidate_D, 98(t1)

	cache	IndexWBInvalidate_D, 3(t1)
	cache	IndexWBInvalidate_D, 35(t1)
	cache	IndexWBInvalidate_D, 67(t1)
	cache	IndexWBInvalidate_D, 99(t1)
non_godson2_1:
	beqz	v0, 3f				# Two way, do set A,
	addu	t1, a2

	cache	IndexWBInvalidate_D, 0(t1)	# do set C
	cache	IndexWBInvalidate_D, 32(t1)
	cache	IndexWBInvalidate_D, 64(t1)
	cache	IndexWBInvalidate_D, 96(t1)

	addu	t1, a2				# do set D
	cache	IndexWBInvalidate_D, 0(t1)
	cache	IndexWBInvalidate_D, 32(t1)
	cache	IndexWBInvalidate_D, 64(t1)
	cache	IndexWBInvalidate_D, 96(t1)

	addiu	v0, -2				# 0 = 4-way, >0 8-way
	bgez	v0, 3f				# Only 4-way... flush set A
	addu	t1, a2

	cache	IndexWBInvalidate_D, 0(t1)	# do set E
	cache	IndexWBInvalidate_D, 32(t1)
	cache	IndexWBInvalidate_D, 64(t1)
	cache	IndexWBInvalidate_D, 96(t1)

	addu	t1, a2				# do set F
	cache	IndexWBInvalidate_D, 0(t1)
	cache	IndexWBInvalidate_D, 32(t1)
	cache	IndexWBInvalidate_D, 64(t1)
	cache	IndexWBInvalidate_D, 96(t1)

	addu	t1, a2				# do set G
	cache	IndexWBInvalidate_D, 0(t1)
	cache	IndexWBInvalidate_D, 32(t1)
	cache	IndexWBInvalidate_D, 64(t1)
	cache	IndexWBInvalidate_D, 96(t1)

	addu	t1, a2				# do set H
	cache	IndexWBInvalidate_D, 0(t1)
	cache	IndexWBInvalidate_D, 32(t1)
	cache	IndexWBInvalidate_D, 64(t1)
	cache	IndexWBInvalidate_D, 96(t1)

3:
	cache	IndexWBInvalidate_D, 0(a0)	# do set A
	cache	IndexWBInvalidate_D, 32(a0)
	cache	IndexWBInvalidate_D, 64(a0)
	cache	IndexWBInvalidate_D, 96(a0)

	bne	a1, zero, 1b
	addu	a0, 128

	j	ra
	nop
END(CPU_FlushDCache)
/*----------------------------------------------------------------------------
 *
 * CPU_HitFlushDCache --
 *
 *	void CPU_HitFlushDCache(addr, len)
 *		vm_offset_t addr, len;
 *
 *	Flush data cache for range of addr to addr + len - 1.
 *	The address can be any valid viritual address as long
 *	as no TLB invalid traps occur. Only lines with matching
 *	addr are flushed.
 *
 *	Note: Use the CpuNWayCache flag to select 16 or 32 byte linesize.
 *	      All Nway cpu's now available have a fixed 32byte linesize.
 *	
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of the L1 cache is written back to primary memory.
 *	The cache line is invalidated.
 *
 *----------------------------------------------------------------------------
 */
LEAF(CPU_HitFlushDCache)
	mfc0	v1, COP_0_STATUS_REG		# Save the status register.
	li	v0, SR_DIAG_DE
	mtc0	v0, COP_0_STATUS_REG		# Disable interrupts

	lw	v0, CpuNWayCache
	beq	a1, zero, 3f			# size is zero!
	addu	a1, 127				# Round up
	addu	a1, a1, a0			# Add extra from address
	and	a0, a0, -128			# align address
	subu	a1, a1, a0
	srl	a1, a1, 7			# Compute number of cache lines
	addu	v0, -2				# Make negative is direct map.

	lw	t0, CpuCacheType
	and	t0, CTYPE_HAS_L2
1:
	bgez	v0, 2f
	addu	a1, -1

	cache	HitWBInvalidate_D, 16(a0)	# direct mapped, do 16 byte
	cache	HitWBInvalidate_D, 48(a0)	# line size.
	cache	HitWBInvalidate_D, 80(a0)
	cache	HitWBInvalidate_D, 112(a0)

2:
	cache	HitWBInvalidate_D, 0(a0)
	cache	HitWBInvalidate_D, 32(a0)
	cache	HitWBInvalidate_D, 64(a0)
	cache	HitWBInvalidate_D, 96(a0)

	bne	a1, zero, 1b
	addu	a0, 128

3:
	mtc0	v1, COP_0_STATUS_REG	# Restore the status register.
	NOP10
	j	ra
	nop
END(CPU_HitFlushDCache)


/*----------------------------------------------------------------------------
 *
 * CPU_HitFlushSCache --
 *
 *	void CPU_HitFlushSCache(addr, len)
 *		vm_offset_t addr, len;
 *
 *	Flush secondary cache for range of addr to addr + len - 1.
 *	The address can be any valid viritual address as long
 *	as no TLB invalid traps occur. Only lines with matching
 *	addr are flushed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of the L2 cache is written back to primary memory.
 *	The cache line is invalidated.
 *
 *----------------------------------------------------------------------------
 */
LEAF(CPU_HitFlushSCache)
	mfc0	v1, COP_0_STATUS_REG		# Save the status register.
	li	v0, SR_DIAG_DE
	mtc0	v0, COP_0_STATUS_REG		# Disable interrupts

	beq	a1, zero, 3f			# size is zero!
	addu	a1, 127				# Round up.
	addu	a1, a1, a0			# Add in extra from align
	and	a0, a0, -128			# Align address
	subu	a1, a1, a0
	srl	a1, a1, 7			# Compute number of cache lines
1:
	addu	a1, -1

	cache	HitWBInvalidate_S, 0(a0)
	cache	HitWBInvalidate_S, 32(a0)
	cache	HitWBInvalidate_S, 64(a0)
	cache	HitWBInvalidate_S, 96(a0)

	bne	a1, zero, 1b
	addu	a0, 128

3:
	mtc0	v1, COP_0_STATUS_REG	# Restore the status register.
	NOP10
	j	ra
	nop
END(CPU_HitFlushSCache)

/*----------------------------------------------------------------------------
 *
 * CPU_HitInvalidateDCache --
 *
 *	void CPU_HitInvalidateDCache(addr, len)
 *		vm_offset_t addr, len;
 *
 *	Invalidate data cache for range of addr to addr + len - 1.
 *	The address can be any valid address as long as no TLB misses occur.
 *	(Be sure to use cached K0SEG kernel addresses or mapped addresses)
 *	Only lines with matching addresses are invalidated.
 *
 *	Note: Use the CpuNWayCache flag to select 16 or 32 byte linesize.
 *	      All Nway cpu's now available have a fixed 32byte linesize.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The L1 cache line is invalidated.
 *
 *----------------------------------------------------------------------------
 */
LEAF(CPU_HitInvalidateDCache)
	mfc0	v1, COP_0_STATUS_REG		# Save the status register.
	li	v0, SR_DIAG_DE
	mtc0	v0, COP_0_STATUS_REG		# Disable interrupts

	lw	v0, CpuNWayCache
	beq	a1, zero, 3f			# size is zero!
	addu	a1, 127				# Round up.
	addu	a1, a1, a0			# Add in extra from align
	and	a0, a0, -128			# Align address
	subu	a1, a1, a0
	srl	a1, a1, 7			# Compute number of cache lines
	addu	v0, -2				# Make negative is direct map.

	lw	t0, CpuCacheType
	and	t0, CTYPE_HAS_L2
1:
	bgez	v0, 2f
	addu	a1, -1

	cache	HitInvalidate_D, 16(a0)		# direct mapped, do 16 byte 
	cache	HitInvalidate_D, 48(a0)		# line size.
	cache	HitInvalidate_D, 80(a0)
	cache	HitInvalidate_D, 112(a0)

2:
	cache	HitInvalidate_D, 0(a0)
	cache	HitInvalidate_D, 32(a0)
	cache	HitInvalidate_D, 64(a0)
	cache	HitInvalidate_D, 96(a0)

	bne	a1, zero, 1b
	addu	a0, 128

3:
	mtc0	v1, COP_0_STATUS_REG		# Restore the status register.
	NOP10
	j	ra
	nop
END(CPU_HitInvalidateDCache)


/*----------------------------------------------------------------------------
 *
 * CPU_HitInvalidateSCache --
 *
 *	void CPU_HitInvalidateSCache(addr, len)
 *		vm_offset_t addr, len;
 *
 *	Invalidate secondary cache for range of addr to addr + len - 1.
 *	The address can be any valid address as long as no TLB misses occur.
 *	(Be sure to use cached K0SEG kernel addresses or mapped addresses)
 *	Only lines with matching addresses are invalidated.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The L2 cache line is invalidated.
 *
 *----------------------------------------------------------------------------
 */
LEAF(CPU_HitInvalidateSCache)
	mfc0	v1, COP_0_STATUS_REG		# Save the status register.
	li	v0, SR_DIAG_DE
	mtc0	v0, COP_0_STATUS_REG		# Disable interrupts

	beq	a1, zero, 3f			# size is zero!
	addu	a1, 127				# Round up
	addu	a1, a1, a0			# Add in extra from align
	and	a0, a0, -128			# Align address
	subu	a1, a1, a0
	srl	a1, a1, 7			# Compute number of cache lines
1:
	addu	a1, -1
	
	cache	HitInvalidate_S, 0(a0)
	cache	HitInvalidate_S, 32(a0)
	cache	HitInvalidate_S, 64(a0)
	cache	HitInvalidate_S, 96(a0)

	bne	a1, zero, 1b
	addu	a0, 128

3:
	mtc0	v1, COP_0_STATUS_REG	# Restore the status register.
	NOP10
	j	ra
	nop
END(CPU_HitInvalidateSCache)

/*----------------------------------------------------------------------------
 *
 * CPU_IOFlushDCache --
 *
 *	void CPU_IOFlushDCache(addr, len, rw)
 *		vm_offset_t addr;
 *		int  len, rw;
 *
 *	Invalidate or flush data cache for range of addr to addr + len - 1.
 *	The address can be any valid address as long as no TLB misses occur.
 *	(Be sure to use cached K0SEG kernel addresses or mapped addresses)
 *
 *	In case of the existence of an external cache we invalidate pages
 *	which are in the given range ONLY if transfer direction is READ. 
 *	The assumption here is a 'write through' external cache which is
 *	true for all now supported processors.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If rw == 0 (read), L1 cache is invalidated or flushed if the area
 *		does not match the alignment requirements. L2 and L3 cache
 *		is invalidated for the address range.
 *	If rw =! 0 (write), L1 cache is written back to memory. L2 cache
 *		is left alone if R4K or R5K otherwise written back. L3
 *		cache is left alone (write through).
 *
 *----------------------------------------------------------------------------
 */
NON_LEAF(CPU_IOFlushDCache, STAND_FRAME_SIZE, ra)

	subu	sp, STAND_FRAME_SIZE
	sw	ra, STAND_RA_OFFSET(sp)
	sw	a0, STAND_FRAME_SIZE(sp)	# save args
	beqz	a2, FlushRD			# read operation
	sw	a1, STAND_FRAME_SIZE+4(sp)

# --- Flush for I/O Write --------
	lw	t0, CpuCacheType
	and	t0, CTYPE_HAS_L2		# Have internal L2?
	beqzl	t0, CPU_HitFlushDCache		# No flush L1
	addu	sp, STAND_FRAME_SIZE

	jal	CPU_HitFlushSCache		# Do internal L2 cache
	nop					# L1 done in parallel

	lw	a0, STAND_FRAME_SIZE(sp)
	jal	CPU_HitFlushDCache		# Do any orphans in L1
	lw	a1, STAND_FRAME_SIZE+4(sp)

	b	FlushDone			# Any L3 is write through
	lw	ra, STAND_RA_OFFSET(sp)		# no need to flush


# --- Flush for I/O Read ---------
FlushRD:

	and	t0, a0, 127			# check if invalidate possible
	bnez	t0, FlushRDWB			# both address and size must
	nop
	and	t0, a1, 127			# be aligned at the cache loop
	bnez	t0, FlushRDWB			# unroll size
	nop

	lw	t0, CpuCacheType		# Aligned, do invalidate
	and	t0, CTYPE_HAS_L2		# Have internal L2?
	bnez	t0, FlushRDL2
	nop

	jal	CPU_HitInvalidateDCache		# External L2 or no L2. Do L1.
	nop

	b	FlushRDXL2
	lw	ra, STAND_RA_OFFSET(sp)		# External L2 if present

FlushRDL2:
	jal	CPU_HitInvalidateSCache		# Internal L2 cache
	nop					# L1 done in parallel

	lw	a0, STAND_FRAME_SIZE(sp)
	jal	CPU_HitInvalidateDCache		# Do any orphans in L1
	lw	a1, STAND_FRAME_SIZE+4(sp)

	b	FlushRDL3
	lw	ra, STAND_RA_OFFSET(sp)		# L3 invalidate if present

FlushRDWB:
	lw	t0, CpuCacheType
	and	t0, CTYPE_HAS_L2		# Have internal L2?
	bnez	t0, FlushRDWBL2			# Yes, do L2
	nop

	jal	CPU_HitFlushDCache
	nop

	b	FlushRDXL2
	lw	ra, STAND_RA_OFFSET(sp)		# External L2 if present

FlushRDWBL2:
	jal	CPU_HitFlushSCache		# Internal L2 cache
	nop					# L1 done in parallel

	lw	a0, STAND_FRAME_SIZE(sp)
	jal	CPU_HitFlushDCache		# Do any orphans in L1
	lw	a1, STAND_FRAME_SIZE+4(sp)

	b	FlushRDL3
	lw	ra, STAND_RA_OFFSET(sp)		# L3 invalidate if present

FlushRDXL2:
	lw	t0, CpuCacheType
	and	t0, CTYPE_HAS_XL2		# Have external L2?
	beqz	t0, FlushRDL3			# Nope.
	lw	a0, STAND_FRAME_SIZE(sp)
	lw	a1, STAND_FRAME_SIZE+4(sp)
	and	a2, a0, 4095			# align on page size
	subu	a0, a2
	addu 	a1, a2
50:
	blez	a1, FlushDone
	subu	a1, 4096			# Fixed cache page size.

	cache	InvalidateSecondaryPage, 0(a0)
	b	50b
	addu	a0, 4096

FlushRDL3:
	lw	t0, CpuCacheType
	and	t0, CTYPE_HAS_XL3		# Have L3?
	beqz	t0, FlushDone			# Nope.
	lw	a0, STAND_FRAME_SIZE(sp)
	lw	a1, STAND_FRAME_SIZE+4(sp)
	and	a2, a0, 4095			# align on page size
	subu	a0, a2
	addu 	a1, a2
40:
	blez	a1, FlushDone
	subu	a1, 4096			# Fixed cache page size.

	cache	InvalidateTertiaryPage, 0(a0)
	b	40b
	addu	a0, 4096

FlushDone:
	NOP10
	j	ra
	addu	sp, STAND_FRAME_SIZE
END(CPU_IOFlushDCache)
.set pop
