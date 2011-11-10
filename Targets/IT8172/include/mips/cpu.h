/*
 * cpu.h: include appropriate CPU definitions for this target
 * Copyright (c) 1992-2000, Algorithmics Ltd.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the "Free MIPS" License Agreement, a copy of 
 * which is available at:
 *
 *  http://www.algor.co.uk/ftp/pub/doc/freemips-license.txt
 *
 * You may not, however, modify or remove any part of this copyright 
 * message if this program is redistributed or reused in whole or in
 * part.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * "Free MIPS" License for more details.  
 */

#ifndef _CPU_H_
#ifdef __cplusplus
extern "C" {
#endif
#define _CPU_H_

#ifndef SR_IMASK
#if #cpu(l64360)
#include <lsi/l64360.h>
#elif #cpu(lr333x0) || #cpu(lr33300) || #cpu(lr33310)
#include <lsi/lr33300.h>
#elif #cpu(lr33020)
#include <lsi/lr33020.h>
#elif #cpu(lr33k) || #cpu(lr33050) || #cpu(lr33000)
#include <lsi/lr33000.h>
#elif #cpu(r3041)
#include <idt/r3041.h>
#elif #cpu(r3081)
#include <idt/r3081.h>
#elif #cpu(r36100)
#include <idt/r36100.h>
#elif #cpu(r4640) || #cpu(r4650)
#include <mips/r4650.h>
#elif #cpu(rc3236x)
#include <idt/rc32364.h>
#elif #cpu(rc6457x)
#include <idt/rc6457x.h>
#elif #cpu(tr4102)
#include <lsi/tr4102.h>
#elif #cpu(tr4101)
#include <lsi/tr4101.h>
#elif #cpu(cw401x)
#include <lsi/cw401x.h>
#elif #cpu(cw400x)
#include <lsi/cw400x.h>
#elif #cpu(jade) && defined(_mips32_nda_)
#include <mips/jade.h>
#elif #cpu(r4000)
#include <mips/r4kc0.h>
#elif #cpu(rm7000)
#include <mips/rm7kc0.h>
#elif #cpu(r5000)
#include <mips/r5kc0.h>
#elif #cpu(r3000)
#include <mips/r3kc0.h>
#elif __mips == 32
# if defined(_mips32_nda_)
#  include <mips/m32c0.h>
# else
#  include <mips/r4kc0.h>
# endif
#elif __mips >= 4
#include <mips/r5kc0.h>
#elif __mips >= 2
#include <mips/r4kc0.h>
#elif __mips16 && __mips64
#include <mips/r4kc0.h>
#elif __mips
#include <mips/r3kc0.h>
#endif
#endif /* SR_IMASK */

#if defined(__STDC__) && !defined(__ASSEMBLER__)
/* 
 * Generic MIPS cache handling
 * 
 * R4000
 *   primary: virtual index, physical tag, write back;
 *   secondary: physical index, physical tag, write back;
 *   pass correct virtual address to primary cache routines.
 * R3000
 *   primary: physical index, physical tag, write through;
 *   secondary: none;
 *   pass kseg0/1 address to primary cache routines
 */

extern int	mips_icache_size, mips_icache_linesize, mips_icache_ways;
extern int	mips_dcache_size, mips_dcache_linesize, mips_dcache_ways;
extern int	mips_scache_size, mips_scache_linesize, mips_scache_ways;
extern int	mips_tcache_size, mips_tcache_linesize, mips_tcache_ways;
extern int	mips_scache_split, mips_scache_discontig;

/* these are now the only standard interfaces to the caches */
extern void	mips_init_cache (void);
extern void	mips_size_cache (void);
extern void	mips_flush_cache (void);
extern void	mips_flush_dcache (void);
extern void	mips_flush_icache (void);
extern void	mips_clean_cache (vaddr_t, size_t);
extern void	mips_clean_dcache (vaddr_t, size_t);
extern void	mips_clean_dcache_nowrite (vaddr_t, size_t);
extern void	mips_clean_icache (vaddr_t, size_t);
extern void	mips_lock_dcache (vaddr_t, size_t);
extern void	mips_lock_icache (vaddr_t, size_t);
extern void	mips_lock_scache (vaddr_t, size_t);

#if 0
/* these are now cpu-specific for use by diagnostics that know 
   what they are doing! */
extern void	mips_flush_cache_nowrite (void);
extern void	mips_clean_dcache_indexed (vaddr_t, size_t);
extern void	mips_clean_dcache_nowrite (vaddr_t, size_t);
extern void	mips_clean_icache_indexed (vaddr_t, size_t);
extern void	mips_clean_icache_nowrite (vaddr_t, size_t);
extern void	mips_clean_scache (vaddr_t, size_t);
extern void	mips_clean_scache_indexed (vaddr_t, size_t);
extern void	mips_clean_scache_nowrite (vaddr_t, size_t);
extern void	mips_clean_tcache (vaddr_t, size_t);
extern void	mips_clean_tcache_indexed (vaddr_t, size_t);
extern void	mips_clean_tcache_nowrite (vaddr_t, size_t);
#endif

/*
 * Other common utilities for all CPUs
 */
extern void	(mips_wbflush) (void);
extern void	mips_cycle (unsigned);
extern void	mips_init_tlb (void);
extern int	mips_tlb_size (void);

/*
 * Coprocessor 0 register manipulation
 * Warning: all non-atomic in face of interrupts.
 */
#if !__mips16 && defined(_mips_mfc0)

/* exchange (swap) VAL and cp0 register REG */
#define _mips_mxc0(reg, val) \
({ \
    register reg32_t __o; \
    __o = _mips_mfc0 (reg); \
    _mips_mtc0 (reg, (val)); \
    __o; \
})

/* bit clear non-zero bits from CLR in cp0 register REG */
#define _mips_bcc0(reg, clr) \
({ \
    register reg32_t __o; \
    __o = _mips_mfc0 (reg); \
    _mips_mtc0 (reg, __o & ~(clr)); \
    __o; \
})

/* bit set non-zero bits from SET in cp0 register REG */
#define _mips_bsc0(reg, set) \
({ \
    register reg32_t __o; \
    __o = _mips_mfc0 (reg); \
    _mips_mtc0 (reg, __o | (set)); \
    __o; \
})

/* bit clear nz bits in from CLR and set nz bits from SET in REG */
#define _mips_bcsc0(reg, clr, set) \
({ \
    register reg32_t __o; \
    __o = _mips_mfc0 (reg); \
    _mips_mtc0 (reg, (__o & ~(clr)) | (set)); \
    __o; \
})

#else

#undef _mips_mfc0
#undef _mips_mtc0

/* use a functional interface to access CP0 registers */
extern reg32_t _mips_xxc0 (reg32_t reg, reg32_t clear, reg32_t set);

#define _mips_mfc0(reg)		_mips_xxc0 (reg, 0, 0)
#define _mips_mtc0(reg,v)	(void)_mips_xxc0 (reg, ~0, v)
#define _mips_mxc0(reg,v)	_mips_xxc0 (reg, ~0, v)
#define _mips_bcc0(reg,clr)	_mips_xxc0 (reg, clr, 0)
#define _mips_bsc0(reg,set)	_mips_xxc0 (reg, 0, set)
#define _mips_bcsc0(reg,c,s)	_mips_xxc0 (reg, c, s)

#endif

/*
 * Standard R3000 & R4000 CP0 register access functions
 */

/* CP0 Status register (NOTE: not atomic operations) */
#define mips_getsr()		_mips_mfc0(C0_SR)
#define mips_setsr(v)		_mips_mtc0(C0_SR,v)
#define mips_xchsr(v)		_mips_mxc0(C0_SR,v)
#define mips_bicsr(clr)		_mips_bcc0(C0_SR,clr)
#define mips_bissr(set)		_mips_bsc0(C0_SR,set)
#define mips_bcssr(c,s)		_mips_bcsc0(C0_SR,c,s)

/* CP0 Cause register (NOTE: not atomic operations) */
#define mips_getcr()		_mips_mfc0(C0_CR)
#define mips_setcr(v)		_mips_mtc0(C0_CR,v)
#define mips_xchcr(v)		_mips_mxc0(C0_CR,v)
#define mips_biccr(clr)		_mips_bcc0(C0_CR,clr)
#define mips_biscr(set)		_mips_bsc0(C0_CR,set)
#define mips_bcscr(c,s)		_mips_bcsc0(C0_CR,c,s)

/* CP0 PrID register */
#define mips_getprid()		_mips_mfc0(C0_PRID)

#ifdef C0_COUNT
/* CP0 Count register */
#define mips_getcount()		_mips_mfc0(C0_COUNT)
#define mips_setcount(v)	_mips_mtc0(C0_COUNT,v)
#define mips_xchcount(v)	_mips_mxc0(C0_COUNT,v)
#endif

#ifdef C0_COMPARE
/* CP0 Compare register*/
#define mips_getcompare()	_mips_mfc0(C0_COMPARE)
#define mips_setcompare(v)	_mips_mtc0(C0_COMPARE,v)
#define mips_xchcompare(v)	_mips_mxc0(C0_COMPARE,v)
#endif

#ifdef C0_CONFIG
/* CP0 Config register */
#define mips_getconfig()	_mips_mfc0(C0_CONFIG)
#define mips_setconfig(v)	_mips_mtc0(C0_CONFIG,v)
#define mips_xchconfig(v)	_mips_mxc0(C0_CONFIG,v)
#define mips_bicconfig(c)	_mips_bcc0(C0_CONFIG,c)
#define mips_bisconfig(s)	_mips_bsc0(C0_CONFIG,s)
#define mips_bcsconfig(c,s)	_mips_bcsc0(C0_CONFIG,c,s)
#endif

#ifdef C0_ECC
/* CP0 ECC register */
#define mips_getecc()		_mips_mfc0(C0_ECC)
#define mips_setecc(x)		_mips_mtc0(C0_ECC, x)
#define mips_xchecc(x)		_mips_mxc0(C0_ECC, x)
#endif

#ifdef C0_TAGLO
/* CP0 TagLo register */
#define mips_gettaglo()		_mips_mfc0(C0_TAGLO)
#define mips_settaglo(x)	_mips_mtc0(C0_TAGLO, x)
#define mips_xchtaglo(x)	_mips_mxc0(C0_TAGLO, x)
#endif

#ifdef C0_TAGHI
/* CP0 TagHi register */
#define mips_gettaghi()		_mips_mfc0(C0_TAGHI)
#define mips_settaghi(x)	_mips_mtc0(C0_TAGHI, x)
#define mips_xchtaghi(x)	_mips_mxc0(C0_TAGHI, x)
#endif

#ifdef C0_WATCHLO
/* CP0 WatchLo register */
#define mips_getwatchlo()	_mips_mfc0(C0_WATCHLO)
#define mips_setwatchlo(x)	_mips_mtc0(C0_WATCHLO, x)
#define mips_xchwatchlo(x)	_mips_mxc0(C0_WATCHLO, x)
#endif

#ifdef C0_WATCHHI
/* CP0 WatchHi register */
#define mips_getwatchhi()	_mips_mfc0(C0_WATCHHI)
#define mips_setwatchhi(x)	_mips_mtc0(C0_WATCHHI, x)
#define mips_xchwatchhi(x)	_mips_mxc0(C0_WATCHHI, x)
#endif


/* 
 * Atomic memory updates.
 *
 * The atomic_update() primitive provides a range of atomic operations
 * on the 32-bit word pointed to by its first argument.  In all cases 
 * the previous value of the word is returned.
 * 
 * The primitive is used to implement the following functions: 
 * bis = bit set; bic = bit clear; flip = bit clear and set;
 * swap = swap; add = increment/decrement.
 */

#if (defined(C0_LLADDR) || defined(_MIPS_HAS_LL_SC)) && !__mips16
#define _mips_atomic_update(wp, clear, addend)		\
    ({unsigned int __old, __tmp;			\
      __asm__ volatile (				\
"1:	ll	%0,%1\n"				\
"	and	%2,%0,%3\n"				\
"	addu	%2,%4\n"				\
"	sc	%2,%1\n"				\
"	beqz	%2,1b"					\
	: "=&d" (__old), "+m" (*wp), "=&d" (__tmp)	\
	: "dJK" (~(clear)), "dJI" (addend)); 		\
      __old; })
#else
extern unsigned int	_mips_atomic_update (volatile unsigned int *,
					     unsigned int, unsigned int);
#endif

#define mips_atomic_bis(wp, val)	_mips_atomic_update(wp,val,val)
#define mips_atomic_bic(wp, val)	_mips_atomic_update(wp,val,0)
#define mips_atomic_bcs(wp, clr, set)	_mips_atomic_update(wp,(clr)|(set),set)
#define mips_atomic_flip(wp, clr, set)	_mips_atomic_update(wp,(clr)|(set),set)
#define mips_atomic_swap(wp, val)	_mips_atomic_update(wp,~0,val)
#define mips_atomic_add(wp, val)	_mips_atomic_update(wp,0,val)


/* 
 * Count-leading zeroes and ones.
 * Simulate with a function call if this CPU hasn't defined a 
 * macro with a suitable asm.
 */
#if !defined(mips_clz)
extern unsigned int _mips_clz(unsigned int);
#define	mips_clz(x)	_mips_clz(x)
#define	mips_clo(x)	_mips_clz(~(x))
#else
#define _mips_have_clz 1
#endif

#if !defined(mips_dclz)
extern unsigned int _mips_dclz(unsigned long long);
#define	mips_dclz(x)	_mips_dclz(x)
#define	mips_dclo(x)	_mips_dclz(~(x))
#else
#define _mips_have_dclz 1
#endif

/* 
 * Default versions of get/put for any MIPS CPU.
 * Some CPUs may have defined special versions for on-chip ram, etc. 
 */
#ifndef mips_get_byte
#define mips_get_byte(addr, errp)	(*(volatile unsigned char *)(addr))
#define mips_get_half(addr, errp)	(*(volatile unsigned short *)(addr))
#define mips_get_word(addr, errp)	(*(volatile unsigned int *)(addr))
#define mips_get_dword(addr, errp)	(*(volatile unsigned long long *)(addr))

#define mips_put_byte(addr, v)	(*(volatile unsigned char *)(addr)=(v), 0)
#define mips_put_half(addr, v)	(*(volatile unsigned short *)(addr)=(v), 0)
#define mips_put_word(addr, v)	(*(volatile unsigned int *)(addr)=(v), 0)
#define mips_put_dword(addr, v)	(*(volatile unsigned long long *)(addr)=(v), 0)
#endif /* mips_get_byte */

/* unoptimisable 2 instruction loop */
#if !__mips16
#define mips_cycle(count)				\
    do {						\
      unsigned int __count = (count);			\
      __asm__ volatile ("%(nop; nop; 1: bnez %0,1b; subu %0,1%)"	\
	: "+d" (__count)); 				\
    } while (0);
#else
#define mips_cycle(count)				\
    do {						\
      unsigned int __count = (count);			\
      if (__count != 0) 				\
        __asm__ volatile ("1: subu %0,1; bnez %0,1b"	\
	  : "+d" (__count)); 				\
    } while (0);
#endif

#ifndef _mips_intdisable
/* "safe" interrupt enable/disable */
extern int _mips_intdisable (void);
extern void _mips_intrestore (int s);
#endif

#ifndef _mips_wait
#define _mips_wait() (void)0
#endif

#endif /* !ASSEMBLER */

#ifdef __cplusplus
}
#endif
#endif /*_CPU_H_*/
