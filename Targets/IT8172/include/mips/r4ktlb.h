/*
 * r4ktlb.h : Generic R4000-style MMU/TLB definitions
 *
 * Copyright (c) 1999, Algorithmics Ltd.  All rights reserved.
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

#ifndef _R4KTLB_H_
#define _R4KTLB_H_

#ifdef __cplusplus
extern "C" {
#endif

/* R4000 EntryHi bits */
#if #cpu(r4100)
/* vr4100 has minimum 1K page */
#define TLBHI_VPN2MASK	0xfffff800
#define TLBHI_VPN2SHIFT	11
#define TLBHI_VPNMASK	0xfffffc00
#define TLBHI_VPNSHIFT	10
#else
/* everything else minimum 4K page */
#define TLBHI_VPN2MASK	0xffffe000
#define TLBHI_VPN2SHIFT	13
#define TLBHI_VPNMASK	0xfffff000
#define TLBHI_VPNSHIFT	12
#endif
#define TLBHI_PIDMASK	0x000000ff
#define TLBHI_PIDSHIFT	0x00000000


/* R4000 EntryLo bits */
#define TLB_PFNMASK	0x3fffffc0
#define TLB_PFNSHIFT	6
#define TLB_FLAGS	0x0000003f
#define TLB_CMASK	0x00000038
#define TLB_CSHIFT	3
#define TLB_D		0x00000004
#define TLB_V		0x00000002
#define TLB_G		0x00000001

#define TLB_WTHRU_NOALLOC	(CFG_C_WTHRU_NOALLOC	<< TLB_CSHIFT)
#define TLB_WTHRU_ALLOC		(CFG_C_WTHRU_ALLOC	<< TLB_CSHIFT)
#define TLB_UNCACHED		(CFG_C_UNCACHED		<< TLB_CSHIFT)
#define TLB_NONCOHERENT		(CFG_C_NONCOHERENT	<< TLB_CSHIFT)
#define TLB_WBACK		(CFG_C_WBACK		<< TLB_CSHIFT)
#ifdef CFG_C_COHERENTXCL
#define TLB_COHERENTXCL		(CFG_C_COHERENTXCL	<< TLB_CSHIFT)
#define TLB_COHERENTXCLW	(CFG_C_COHERENTXCLW	<< TLB_CSHIFT)
#define TLB_COHERENTUPD		(CFG_C_COHERENTUPD	<< TLB_CSHIFT)
#endif
#ifdef CFG_C_UNCACHED_NOBLOCK
#define TLB_UNCACHED_NOBLOCK	(CFG_C_UNCACHED_NOBLOCK	<< TLB_CSHIFT)
#endif
#ifdef CFG_C_BYPASS
#define TLB_BYPASS		(CFG_C_BYPASS		<< TLB_CSHIFT)
#endif
#ifdef CFG_C_UNCACHED_ACCEL
#define TLB_UNCACHED_ACCEL	(CFG_C_UNCACHED_ACCEL	<< TLB_CSHIFT)/*t5*/
#endif

/* R4000 Index bits */
#define TLBIDX_MASK	0x3f
#define TLBIDX_SHIFT	0

/* R4000 Random bits */
#define TLBRAND_MASK	0x3f
#define TLBRAND_SHIFT	0

#ifndef NTLBID
#define NTLBID		64	/* max number of tlb entry pairs */
#endif

/* macros to constuct tlbhi and tlblo */
#define mktlbhi(vpn,id)   ((((tlbhi_t)(vpn)>>1) << TLBHI_VPN2SHIFT) | \
			   ((id) << TLBHI_PIDSHIFT))
#define mktlblo(pn,flags) (((tlblo_t)(pn) << TLB_PFNSHIFT) | (flags))

/* and destruct them */
#define tlbhiVpn(hi)	((hi) >> TLBHI_VPNSHIFT)
#define tlbhiId(hi)	((hi) & TLBHI_PIDMASK)
#define tlbloPn(lo)	((lo) >> TLB_PFNSHIFT)
#define tlbloFlags(lo)	((lo) & TLB_FLAGS)

#ifdef __ASSEMBLER__
/* 
 * R4000 virtual memory regions (SDE-MIPS only supports 32-bit addressing)
 */
#define	KSEG0_BASE	0x80000000
#define	KSEG1_BASE	0xa0000000
#define	KSEG2_BASE	0xc0000000
#define	KSEGS_BASE	0xc0000000
#define	KSEG3_BASE	0xe0000000
#define RVEC_BASE	0xbfc00000	/* reset vector base */

#define KUSEG_SIZE	0x80000000
#define KSEG0_SIZE	0x20000000
#define KSEG1_SIZE	0x20000000
#define KSEG2_SIZE	0x40000000
#define KSEGS_SIZE	0x20000000
#define KSEG3_SIZE	0x20000000

/* 
 * Translate a kernel virtual address in KSEG0 or KSEG1 to a real
 * physical address and back.
 */
#define KVA_TO_PA(v) 	((v) & 0x1fffffff)
#define PA_TO_KVA0(pa)	((pa) | 0x80000000)
#define PA_TO_KVA1(pa)	((pa) | 0xa0000000)

/* translate betwwen KSEG0 and KSEG1 virtual addresses */
#define KVA0_TO_KVA1(v)	((v) | 0x20000000)
#define KVA1_TO_KVA0(v)	((v) & ~0x20000000)

#else /* __ASSEMBLER__ */

/*
 * Standard address types
 */
typedef unsigned long		paddr_t;	/* a physical address */
typedef unsigned long		vaddr_t;	/* a virtual address */
#if __mips64
typedef unsigned long long	tlbhi_t;	/* the tlbhi field */
#else
typedef unsigned long		tlbhi_t;	/* the tlbhi field */
#endif
typedef unsigned long		tlblo_t;	/* the tlblo field */

/* 
 * R4000 virtual memory regions (SDE-MIPS only supports 32-bit addressing)
 */
#define KUSEG_BASE 	((void *)0x00000000)
#define KSEG0_BASE	((void *)0x80000000)
#define KSEG1_BASE	((void *)0xa0000000)
#define KSEG2_BASE	((void *)0xc0000000)
#define KSEGS_BASE	((void *)0xc0000000)
#define KSEG3_BASE	((void *)0xe0000000)
#define RVEC_BASE	((void *)0xbfc00000)	/* reset vector base */

#define KUSEG_SIZE	0x80000000u
#define KSEG0_SIZE	0x20000000u
#define KSEG1_SIZE	0x20000000u
#define KSEG2_SIZE	0x40000000u
#define KSEGS_SIZE	0x20000000u
#define KSEG3_SIZE	0x20000000u

/* 
 * Translate a kernel virtual address in KSEG0 or KSEG1 to a real
 * physical address and back.
 */
#define KVA_TO_PA(v) 	((paddr_t)(v) & 0x1fffffff)
#define PA_TO_KVA0(pa)	((void *) ((pa) | 0x80000000))
#define PA_TO_KVA1(pa)	((void *) ((pa) | 0xa0000000))

/* translate betwwen KSEG0 and KSEG1 virtual addresses */
#define KVA0_TO_KVA1(v)	((void *) ((unsigned)(v) | 0x20000000))
#define KVA1_TO_KVA0(v)	((void *) ((unsigned)(v) & ~0x20000000))

/* Test for KSEGS */
#define IS_KVA(v)	((int)(v) < 0)
#define IS_KVA0(v)	(((unsigned)(v) >> 29) == 0x4)
#define IS_KVA1(v)	(((unsigned)(v) >> 29) == 0x5)
#define IS_KVA01(v)	(((unsigned)(v) >> 30) == 0x2)
#define IS_KVAS(v)	(((unsigned)(v) >> 29) == 0x6)
#define IS_KVA2(v)	(((unsigned)(v) >> 29) == 0x7)
#define IS_UVA(v)	((int)(v) >= 0)

/* convert register type to address and back */
#define VA_TO_REG(v)	((long)(v))		/* sign-extend 32->64 */
#define REG_TO_VA(v)	((void *)(long)(v))	/* truncate 64->32 */

/*
 * R4000 can set the page size on each TLB entry,
 * we present the common case here.
 */
#if #cpu(r4100)
#define VMPGSIZE 	1024
#define VMPGMASK 	(VMPGSIZE-1)
#define VMPGSHIFT 	10
#else
#define VMPGSIZE 	4096
#define VMPGMASK 	(VMPGSIZE-1)
#define VMPGSHIFT 	12
#endif

/* virtual address to virtual page number and back */
#define vaToVpn(va)	((reg_t)(va) >> VMPGSHIFT)
#define vpnToVa(vpn)	(void *)((vpn) << VMPGSHIFT)

/* physical address to physical page number and back */
#define paToPn(pa)	((pa) >> VMPGSHIFT)
#define pnToPa(pn)	((paddr_t)((pn) << VMPGSHIFT))

/* 
 * R4000 TLB acccess functions
 */
void	r4k_tlbri (tlbhi_t *, tlblo_t *, tlblo_t *, unsigned *, unsigned);
void	r4k_tlbwi (tlbhi_t, tlblo_t, tlblo_t, unsigned, unsigned);
void	r4k_tlbwr (tlbhi_t, tlblo_t, tlblo_t, unsigned);
int	r4k_tlbrwr (tlbhi_t, tlblo_t, tlblo_t, unsigned);
int	r4k_tlbprobe (tlbhi_t, tlblo_t *, tlblo_t *, unsigned *);
void	r4k_tlbinval (tlbhi_t);
void	r4k_tlbinvalall (void);

/* R4000 CP0 Context register */
#define r4k_getcontext()	_mips_mfc0(C0_CONTEXT)
#define r4k_setcontext(v)	_mips_mtc0(C0_CONTEXT,v)
#define r4k_xchcontext(v)	_mips_mxc0(C0_CONTEXT,v)

/* R4000 CP0 EntryHi register */
#define r4k_getentryhi()	_mips_mfc0(C0_ENTRYHI)
#define r4k_setentryhi(v)	_mips_mtc0(C0_ENTRYHI,v)
#define r4k_xchentryhi(v)	_mips_mxc0(C0_ENTRYHI,v)

/* R4000 CP0 EntryLo0 register */
#define r4k_getentrylo0()	_mips_mfc0(C0_ENTRYLO0)
#define r4k_setentrylo0(v)	_mips_mtc0(C0_ENTRYLO0,v)
#define r4k_xchentrylo0(v)	_mips_mxc0(C0_ENTRYLO0,v)

/* R4000 CP0 EntryLo1 register */
#define r4k_getentrylo1()	_mips_mfc0(C0_ENTRYLO1)
#define r4k_setentrylo1(v)	_mips_mtc0(C0_ENTRYLO1,v)
#define r4k_xchentrylo1(v)	_mips_mxc0(C0_ENTRYLO1,v)

/* R4000 CP0 PageMask register */
#define r4k_getpagemask()	_mips_mfc0(C0_PAGEMASK)
#define r4k_setpagemask(v)	_mips_mtc0(C0_PAGEMASK,v)
#define r4k_xchpagemask(v)	_mips_mxc0(C0_PAGEMASK,v)

/* R4000 CP0 Wired register */
#define r4k_getwired()		_mips_mfc0(C0_WIRED)
#define r4k_setwired(v)		_mips_mtc0(C0_WIRED,v)
#define r4k_xchwired(v)		_mips_mxc0(C0_WIRED,v)

#endif /* __ASSEMBLER__ */

#ifdef __cplusplus
}
#endif

#endif /* _R4KTLB_H_ */
