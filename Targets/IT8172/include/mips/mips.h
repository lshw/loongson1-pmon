/* $Id: mips.h,v 1.1.1.1 2006/09/14 01:59:09 root Exp $ */
/* mips.h - machine dependent definitions for the MIPS architecture
 *	The contents of this file are not copyrighted in any
 *	way, and may therefore be used without restriction.
 */

/*
 * This file supports both the LR3000 and the LR33000. By default
 * the LR3000 is assumed. For the LR33000 invoke the compiler with
 * the option -DLR33000.
 *
 * This file also supports development under SABLE, (the simulator
 * supplied with SPP). To compile for the SABLE environment, invoke
 * the compiler with the option -DSABLE.
 */

#ifndef _MIPS_
#define _MIPS_

#if #system(sde)
# if defined(__ASSEMBLER__) 
#  ifndef LANGUAGE_ASSEMBLY 
#   define LANGUAGE_ASSEMBLY 
#  endif 
# else 
#  ifndef LANGUAGE_C 
#   define LANGUAGE_C 
#  endif 
# endif  
#else 
# ifndef LANGUAGE_C
#  define LANGUAGE_C
# endif
#endif

#define K0BASE 		0x80000000
#define K0SIZE 		0x20000000
#define K1BASE 		0xa0000000
#define K1SIZE 		0x20000000
#define K2BASE 		0xc0000000

#define GEN_VECT 	0x80000080
#define UTLB_VECT 	0x80000000

#ifdef LANGUAGE_ASSEMBLY

#define PHYS_TO_K0(pa)	((pa)|K0BASE)
#define PHYS_TO_K1(pa)	((pa)|K1BASE)
#define K0_TO_PHYS(va)	((va)&(K0SIZE-1))
#define K1_TO_PHYS(va)	((va)&(K1SIZE-1))
#define K0_TO_K1(va)	((va)|K1SIZE)
#define K1_TO_K0(va)	((va)&~K1SIZE)

/* SDE compatibility */
#ifndef KSEG0_BASE
#define PA_TO_KVA0(pa)	PHYS_TO_K0(pa)
#define PA_TO_KVA1(pa)	PHYS_TO_K1(pa)
#define KVA_TO_PA(pa)	K1_TO_PHYS(pa)
#define KSEG0_BASE	K0BASE
#define KSEG1_BASE	K1BASE
#endif

#define jr j
#define jalr jal

/* aliases for general registers */
#define zero		$0
#define	AT		$1		/* assembler temporaries */
#define	v0		$2		/* value holders */
#define	v1		$3
#define	a0		$4		/* arguments */
#define	a1		$5
#define	a2		$6
#define	a3		$7
#define	t0		$8		/* temporaries */
#define	t1		$9
#define	t2		$10
#define	t3		$11
#define	t4		$12
#define	t5		$13
#define	t6		$14
#define	t7		$15
#define ta0		$12
#define ta1		$13
#define ta2		$14
#define ta3		$15
#define	s0		$16		/* saved registers */
#define	s1		$17
#define	s2		$18
#define	s3		$19
#define	s4		$20
#define	s5		$21
#define	s6		$22
#define	s7		$23
#define	t8		$24		/* temporaries */
#define	t9		$25
#define	k0		$26		/* kernel registers */
#define	k1		$27
#define	gp		$28		/* global pointer */
#define	sp		$29		/* stack pointer */
#define	s8		$30		/* saved register */
#define	fp		$30		/* frame pointer (old usage) */
#define	ra		$31		/* return address */

/* System Control Coprocessor (CP0) registers */
#define C0_SR		$12		/* Processor Status */
#define C0_STATUS	$12		/* Processor Status */
#define C0_CAUSE	$13		/* Exception Cause */
#define C0_EPC		$14		/* Exception PC */
#define C0_BADADDR	$8		/* Bad Address */
#define C0_BADVADDR	$8		/* Bad Virtual Address */
#define C0_PRID		$15		/* Processor Revision Indentifier */

#ifdef LR33000
#include "lr33000.h"
#else 
#define C0_CTEXT	$4		/* Context */
#define C0_TLBHI	$10		/* TLB EntryHi */
#define C0_TLBLO	$2		/* TLB EntryLo */
#define C0_INX		$0		/* TLB Index */
#define C0_RAND		$1		/* TLB Random */
#ifdef R4000
#define C0_TLBLO0	$2		/* TLB EntryLo0 */
#define C0_TLBLO1	$3		/* TLB EntryLo1 */
#define C0_PGMASK	$5		/* TLB PageMask */
#define C0_WIRED	$6		/* TLB Wired */
#define C0_COUNT 	$9		/* Count */
#define C0_COMPARE	$11		/* Compare */
#define C0_CONFIG	$16		/* Config */
#define C0_LLADDR	$17		/* LLAddr */
#define C0_WATCHLO	$18		/* WatchpointLo */
#define C0_WATCHHI	$19		/* WatchpointHi */
#define C0_XCTEXT	$20		/* XContext */
#define C0_WATCHMASK	$24		/* RM7000 Watchmask */
#define C0_ECC		$26		/* ECC */
#define C0_CACHEERR	$27		/* CacheErr */
#define C0_TAGLO	$28		/* TagLo */
#define C0_TAGHI	$29		/* TagHi */
#define C0_ERREPC	$30		/* ErrorEPC */

/* RM7000 control registers, access via cfc0/ctc0 */
#define C0C_IPLLO	$18
#define C0C_IPLHI	$19
#define C0C_ICR		$20
#define C0C_DERRADDR1	$26
#define C0C_DERRADDR2	$27
#endif /* R4000 */
#endif /* !LR33000 */

/* Floating-Point Control registers */
#define FPA_CSR		$31		/* Control/Status register */
#define FPA_IRR		$0		/* Implementation/Revision register */

#define LEAF(name) \
  	.text; \
  	.globl	name; \
  	.ent	name; \
name:

#define XLEAF(name) \
  	.text; \
  	.globl	name; \
  	.aent	name; \
name:

#define SLEAF(name) \
  	.text; \
  	.ent	name; \
name:

#define END(name) \
  	.size name,.-name; \
  	.end	name

#define SEND(name) END(name)

#define EXPORT(name) \
  	.globl name; \
name:

#define EXPORTS(name,sz) \
  	.globl name; \
name:

#define	IMPORT(name, size) \
	.extern	name,size

#define BSS(name,size) \
	.comm	name,size

#define LBSS(name,size) \
  	.lcomm	name,size

#else /* LANGUAGE_C */

#define PHYS_TO_K0(pa)	((unsigned)(pa)|K0BASE)
#define PHYS_TO_K1(pa)	((unsigned)(pa)|K1BASE)
#define K0_TO_PHYS(va)	((unsigned)(va)&(K0SIZE-1))
#define K1_TO_PHYS(va)	((unsigned)(va)&(K1SIZE-1))
#define K0_TO_K1(va)	((unsigned)(va)|K1SIZE)
#define K1_TO_K0(va)	((unsigned)(va)&~K1SIZE)

#define	IS_K0SEG(x)	((unsigned)(x) >= K0BASE && (unsigned)(x) < K1BASE)
#define	IS_K1SEG(x)	((unsigned)(x) >= K1BASE && (unsigned)(x) < K2BASE)

/* SDE compatibility */
#ifndef KSEG0_BASE
#define KSEG0_BASE	(void *)K0BASE
#define KSEG1_BASE	(void *)K1BASE
#define PA_TO_KVA0(pa)	(void *)PHYS_TO_K0(pa)
#define PA_TO_KVA1(pa)	(void *)PHYS_TO_K1(pa)
#define KVA_TO_PA(pa)	K1_TO_PHYS(pa)
#define KVA0_TO_KVA1(v)	(void *)K0_TO_K1(v)
#define KVA1_TO_KVA0(v)	(void *)K1_TO_K0(v)
#define IS_KVA(v)	((int)(v) < 0)
#define IS_KVA0(v)	(((unsigned)(v) >> 29) == 0x4)
#define IS_KVA1(v)	(((unsigned)(v) >> 29) == 0x5)
#define IS_KVA01(v)	(((unsigned)(v) >> 30) == 0x2)
#endif /* KSEG0_BASE */

#define C0_SR		12		/* Processor Status */
#define C0_STATUS	12		/* Processor Status */
#define C0_CAUSE	13		/* Exception Cause */
#define C0_EPC		14		/* Exception PC */
#define C0_BADADDR	8		/* Bad Address */
#define C0_BADVADDR	8		/* Bad Virtual Address */
#define C0_PRID		15		/* Processor Revision Indentifier */

#ifdef LR33000
#include "lr33000.h"
#else
#define C0_CTEXT	4		/* Context */
#define C0_TLBHI	10		/* TLB EntryHi */
#define C0_TLBLO	2		/* TLB EntryLo */
#define C0_INX		0		/* TLB Index */
#define C0_RAND		1		/* TLB Random */
#ifdef R4000
#define C0_TLBLO0	2		/* TLB EntryLo0 */
#define C0_TLBLO1	3		/* TLB EntryLo1 */
#define C0_PGMASK	5		/* TLB PageMask */
#define C0_WIRED	6		/* TLB Wired */
#define C0_COUNT 	9		/* Count */
#define C0_COMPARE	11		/* Compare */
#define C0_CONFIG	16		/* Config */
#define C0_LLADDR	17		/* LLAddr */
#define C0_WATCHLO	18		/* WatchpointLo */
#define C0_WATCHHI	19		/* WatchpointHi */
#define C0_ECC		26		/* ECC */
#define C0_CACHEERR	27		/* CacheErr */
#define C0_TAGLO	28		/* TagLo */
#define C0_TAGHI	29		/* TagHi */
#define C0_ERREPC	30		/* ErrorEPC */
#endif /* R4000 */
#endif /* !LR33000 */

/* Floating-Point Control registers */
#define FPA_CSR		31		/* Control/Status register */
#define FPA_IRR		0		/* Implementation/Revision register */
#endif /* LANGUAGE_C */

/* Floating-Point Control register bits */
#define CSR_C		0x00800000
#define CSR_EXC		0x0003f000
#define CSR_EE		0x00020000
#define CSR_EV		0x00010000
#define CSR_EZ		0x00008000
#define CSR_EO		0x00004000
#define CSR_EU		0x00002000
#define CSR_EI		0x00001000
#define CSR_TV		0x00000800
#define CSR_TZ		0x00000400
#define CSR_TO		0x00000200
#define CSR_TU		0x00000100
#define CSR_TI		0x00000080
#define CSR_SV		0x00000040
#define CSR_SZ		0x00000020
#define CSR_SO		0x00000010
#define CSR_SU		0x00000008
#define CSR_SI		0x00000004
#define CSR_RM		0x00000003

/* Status Register */
#define SR_CUMASK	0xf0000000	/* Coprocessor usable bits */
#define	SR_CU3		0x80000000	/* Coprocessor 3 usable */
#define SR_CU2		0x40000000	/* coprocessor 2 usable */
#define SR_CU1		0x20000000	/* Coprocessor 1 usable */
#define SR_CU0		0x10000000	/* Coprocessor 0 usable */

#define SR_RE		0x02000000	/* Reverse Endian in user mode */
#define SR_BEV		0x00400000	/* Bootstrap Exception Vector */
#define SR_TS		0x00200000	/* TLB shutdown */

#ifndef R4000
/* R3000-specific bits */
#define SR_PE		0x00100000	/* Parity Error */
#define SR_CM		0x00080000	/* Cache Miss */
#define SR_PZ		0x00040000	/* Parity Zero */
#define SR_SWC		0x00020000	/* Swap Caches */
#define SR_ISC		0x00010000	/* Isolate Cache */

#define SR_KUO		0x00000020	/* Kernel/User mode, old */
#define SR_IEO		0x00000010	/* Interrupt Enable, old */
#define SR_KUP		0x00000008	/* Kernel/User mode, previous */
#define SR_IEP		0x00000004	/* Interrupt Enable, previous */
#define SR_KUC		0x00000002	/* Kernel/User mode, current */
#define SR_IEC		0x00000001	/* Interrupt Enable, current */
#else
/* R4000-specific bits */
#define SR_RP		0x08000000	/* Reduce Power */
#define SR_FR		0x04000000	/* Enable extra floating-point registers */
#define SR_SR		0x00100000	/* Soft Reset */
#define SR_CH		0x00040000	/* Cache Hit */
#define SR_CE		0x00020000	/* Cache ECC register modifies check bits */
#define SR_DE		0x00010000	/* Disable cache errors */

#define SR_KX		0x00000080	/* xtlb in kernel mode */
#define SR_SX		0x00000040	/* mips3 & xtlb in supervisor mode */
#define SR_UX		0x00000020	/* mips3 & xtlb in user mode */

#define SR_KSU_MASK	0x00000018	/* ksu mode mask */
#define SR_KSU_USER	0x00000010	/* user mode */
#define SR_KSU_SUPV	0x00000008	/* supervisor mode */
#define SR_KSU_KERN	0x00000000	/* kernel mode */

#define SR_ERL		0x00000004	/* error level */
#define SR_EXL		0x00000002	/* exception level */
#define SR_IE		0x00000001 	/* interrupt enable */
#endif /* R4000 */

#define SR_IMASK	0x0000ff00	/* Interrupt Mask */
#define SR_IMASK8	0x00000000	/* Interrupt Mask level=8 */
#define SR_IMASK7	0x00008000	/* Interrupt Mask level=7 */
#define SR_IMASK6	0x0000c000	/* Interrupt Mask level=6 */
#define SR_IMASK5	0x0000e000	/* Interrupt Mask level=5 */
#define SR_IMASK4	0x0000f000	/* Interrupt Mask level=4 */
#define SR_IMASK3	0x0000f800	/* Interrupt Mask level=3 */
#define SR_IMASK2	0x0000fc00	/* Interrupt Mask level=2 */
#define SR_IMASK1	0x0000fe00	/* Interrupt Mask level=1 */
#define SR_IMASK0	0x0000ff00	/* Interrupt Mask level=0 */

#define SR_IBIT8	0x00008000	/*  (Intr5) */
#define SR_IBIT7	0x00004000	/*  (Intr4) */
#define SR_IBIT6	0x00002000	/*  (Intr3) */
#define SR_IBIT5	0x00001000	/*  (Intr2) */
#define SR_IBIT4	0x00000800	/*  (Intr1) */
#define SR_IBIT3	0x00000400	/*  (Intr0) */
#define SR_IBIT2	0x00000200	/*  (Software Interrupt 1) */
#define SR_IBIT1	0x00000100	/*  (Software Interrupt 0) */

/* Cause Register */
#define CAUSE_BD		0x80000000	/* Branch Delay */
#define CAUSE_CEMASK		0x30000000	/* Coprocessor Error */
#define CAUSE_CESHIFT		28		/* Right justify CE  */
#define CAUSE_IPMASK		0x0000ff00	/* Interrupt Pending */
#define CAUSE_IPSHIFT		8		/* Right justify IP  */
#define CAUSE_IP8		0x00008000	/*  (Intr5) */
#define CAUSE_IP7		0x00004000	/*  (Intr4) */
#define CAUSE_IP6		0x00002000	/*  (Intr3) */
#define CAUSE_IP5		0x00001000	/*  (Intr2) */
#define CAUSE_IP4		0x00000800	/*  (Intr1) */
#define CAUSE_IP3		0x00000400	/*  (Intr0) */
#define CAUSE_SW2		0x00000200	/*  (Software Interrupt 1) */
#define CAUSE_SW1		0x00000100	/*  (Software Interrupt 0) */
#define CAUSE_EXCMASK		0x0000007c	/* Exception Code */
#define CAUSE_EXCSHIFT		2		/* Right justify EXC */

/* Exception Code */
/* XXX now defined in mips/cpu.h et al */
#define CEXC_INT	(0 << 2)	/* External interrupt */
#define CEXC_MOD	(1 << 2)	/* TLB modification */
#define CEXC_TLBL	(2 << 2)    	/* TLB miss (Load or Ifetch) */
#define CEXC_TLBS	(3 << 2)	/* TLB miss (Save) */
#define CEXC_ADEL	(4 << 2)    	/* Address error (Load or Ifetch) */
#define CEXC_ADES	(5 << 2)	/* Address error (Save) */
#define CEXC_IBE	(6 << 2)	/* Bus error (Ifetch) */
#define CEXC_DBE	(7 << 2)	/* Bus error (data load or store) */
#define CEXC_SYS	(8 << 2)	/* System call */
#define CEXC_BP		(9 << 2)	/* Break point */
#define CEXC_RI		(10 << 2)	/* Reserved instruction */
#define CEXC_CPU	(11 << 2)	/* Coprocessor unusable */
#define CEXC_OVF	(12 << 2)	/* Arithmetic overflow */
#ifdef R4000
#define CEXC_TRAP	(13 << 2)	/* Trap exception */
#define CEXC_VCEI	(14 << 2)	/* Virtual Coherency Exception (I) */
#define CEXC_FPE	(15 << 2)	/* Floating Point Exception */
#define CEXC_CP2	(16 << 2)	/* Cp2 Exception */
/*#define CEXC_C2E	(16 << 2)*/	/* Cp2 Exception */
#define CEXC_WATCH	(23 << 2)	/* Watchpoint exception */
#define CEXC_CACHE	(30 << 2)	/* Fake cache exception */
#define CEXC_VCED	(31 << 2)	/* Virtual Coherency Exception (D) */
#endif /* R4000 */

#ifdef R4000
#define	NTLBENTRIES	48
#else
#define	NTLBENTRIES	64
#endif

#define HI_HALF(x)	((x) >> 16)
#define LO_HALF(x)	((x) & 0xffff)

/* FPU stuff */
#define C1_CSR		$31
#define CSR_EMASK	(0x3f<<12)
#define CSR_TMASK	(0x1f<<7)
#define CSR_SMASK	(0x1f<<2)
#define C1_FRID		$0

#ifdef LR33020
#include "lr33020.h"
#endif

#ifdef R4000
/*
 * R4000 Config Register 
 */
#ifndef CFG_ECMASK
#define CFG_CM		0x80000000	/* Master-Checker mode */
#define CFG_ECMASK	0x70000000	/* System Clock Ratio */
#define CFG_ECSHIFT	28
#define CFG_ECBY2	0x00000000 	/* divide by 2 */
#define CFG_ECBY3	0x00000000 	/* divide by 3 */
#define CFG_ECBY4	0x00000000 	/* divide by 4 */
#define CFG_EPMASK	0x0f000000	/* Transmit data pattern */
#define CFG_EPD		0x00000000	/* D */
#define CFG_EPDDX	0x01000000	/* DDX */
#define CFG_EPDDXX	0x02000000	/* DDXX */
#define CFG_EPDXDX	0x03000000	/* DXDX */
#define CFG_EPDDXXX	0x04000000	/* DDXXX */
#define CFG_EPDDXXXX	0x05000000	/* DDXXXX */
#define CFG_EPDXXDXX	0x06000000	/* DXXDXX */
#define CFG_EPDDXXXXX	0x07000000	/* DDXXXXX */
#define CFG_EPDXXXDXXX	0x08000000	/* DXXXDXXX */
#define CFG_SBMASK	0x00c00000	/* Secondary cache block size */
#define CFG_SBSHIFT	22
#define CFG_SB4		0x00000000	/* 4 words */
#define CFG_SB8		0x00400000	/* 8 words */
#define CFG_SB16	0x00800000	/* 16 words */
#define CFG_SB32	0x00c00000	/* 32 words */
#define CFG_EMMASK	0x00c00000	/* Vr54xx: SysAD mode */
#define CFG_EMSHIFT	22
#define CFG_EM_R4K	0x00000000	/* Vr54xx: R4x000 compatible */
#define CFG_EM_SPLITRD	0x00400000	/* Vr54xx: Multiple split reads */
#define CFG_EM_PIPEWR	0x00800000	/* Vr54xx: Pipeline writes */
#define CFG_EM_WRREISSU	0x00c00000	/* Vr54xx: Write-reissue */
#define CFG_AD		0x00800000	/* Accelerated data (R4100) */
#define CFG_SS		0x00200000	/* Split secondary cache */
#define CFG_SW		0x00100000	/* Secondary cache port width */
#define CFG_EWMASK	0x000c0000	/* System port width */
#define CFG_EWSHIFT	18
#define CFG_EW64	0x00000000	/* 64 bit */
#define CFG_EW32	0x00040000	/* 32 bit */
#define CFG_SC		0x00020000	/* Secondary cache absent */
#define CFG_SM		0x00010000	/* Dirty Shared mode disabled */
#define CFG_BE		0x00008000	/* Big Endian */
#define CFG_EM		0x00004000	/* ECC mode enable */
#define CFG_EB		0x00002000	/* Block ordering */
#define CFG_ICMASK	0x00000e00	/* Instruction cache size */
#define CFG_ICSHIFT	9
#define CFG_DCMASK	0x000001c0	/* Data cache size */
#define CFG_DCSHIFT	6
#define CFG_IB		0x00000020	/* Instruction cache block size */
#define CFG_DB		0x00000010	/* Data cache block size */
#define CFG_CU		0x00000008	/* Update on Store Conditional */
#define CFG_K0MASK	0x00000007	/* KSEG0 coherency algorithm */
#endif CFG_ECMASK

/*
 * Primary cache mode
 */
#define CFG_C_WTHRU_NOALLOC	0	/* r4600 only */
#define CFG_C_WTHRU_ALLOC	1	/* r4600 only */
#define CFG_C_UNCACHED		2
#define CFG_C_NONCOHERENT	3
#define CFG_C_WBACK		3
#define CFG_C_COHERENTXCL	4
#define CFG_C_COHERENTXCLW	5 	
#define CFG_C_COHERENTUPD	6	/* r4000/r4400 only */
#define CFG_C_UNCACHED_ACCEL	7	/* t5 only */

/* 
 * Primary Cache TagLo 
 */
#define TAG_PTAG_MASK           0xffffff00      /* Primary Tag */
#define TAG_PTAG_SHIFT          8
#define TAG_PSTATE_MASK         0x000000c0      /* Primary Cache State */
#define TAG_PSTATE_SHIFT        6
#define TAG_PARITY_MASK         0x00000001      /* Primary Tag Parity */
#define TAG_PARITY_SHIFT        0

#define PSTATE_INVAL		0
#define PSTATE_SHARED		1
#define PSTATE_CLEAN_EXCL	2
#define PSTATE_DIRTY_EXCL	3

/* 
 * Secondary Cache TagLo 
 */
#ifndef TAG_STAG_MASK
#define TAG_STAG_MASK           0xffffe000      /* Secondary Tag */
#define TAG_STAG_SHIFT          13
#define TAG_SSTATE_MASK         0x00001c00      /* Secondary Cache State */
#define TAG_SSTATE_SHIFT        10
#define TAG_VINDEX_MASK         0x00000380      /* Secondary Virtual Index */
#define TAG_VINDEX_SHIFT        7
#define TAG_ECC_MASK            0x0000007f      /* Secondary Tag ECC */
#define TAG_ECC_SHIFT           0
#define TAG_STAG_SIZE		19		/* Secondary Tag Width */
#endif

#define SSTATE_INVAL		0
#define SSTATE_CLEAN_EXCL	4
#define SSTATE_DIRTY_EXCL	5
#define SSTATE_CLEAN_SHARED	6
#define SSTATE_DIRTY_SHARED	7

/*
 * R4000 CacheErr register
 */
#define CACHEERR_TYPE		0x80000000	/* reference type: 
						   0=Instr, 1=Data */
#define CACHEERR_LEVEL		0x40000000	/* cache level:
						   0=Primary, 1=Secondary */
#define CACHEERR_DATA		0x20000000	/* data field:
						   0=No error, 1=Error */
#define CACHEERR_TAG		0x10000000	/* tag field:
						   0=No error, 1=Error */
#define CACHEERR_REQ		0x08000000	/* request type:
						   0=Internal, 1=External */
#define CACHEERR_BUS		0x04000000	/* error on bus:
						   0=No, 1=Yes */
#define CACHEERR_BOTH		0x02000000	/* Data & Instruction error:
						   0=No, 1=Yes */
#define CACHEERR_REFILL		0x01000000	/* Error on Refill:
						   0=No, 1=Yes */
#define CACHEERR_SIDX_MASK	0x003ffff8	/* PADDR(21..3) */
#define CACHEERR_SIDX_SHIFT		 3
#define CACHEERR_PIDX_MASK	0x00000007	/* VADDR(14..12) */
#define CACHEERR_PIDX_SHIFT		12


/*
 * R4000 Cache operations
 */
#ifndef Index_Invalidate_I
#define Index_Invalidate_I               0x0         /* 0       0 */
#define Index_Writeback_Inv_D            0x1         /* 0       1 */
#define Index_Invalidate_SI              0x2         /* 0       2 */
#define Index_Writeback_Inv_SD           0x3         /* 0       3 */
#define Index_Load_Tag_I                 0x4         /* 1       0 */
#define Index_Load_Tag_D                 0x5         /* 1       1 */
#define Index_Load_Tag_SI                0x6         /* 1       2 */
#define Index_Load_Tag_SD                0x7         /* 1       3 */
#define Index_Store_Tag_I                0x8         /* 2       0 */
#define Index_Store_Tag_D                0x9         /* 2       1 */
#define Index_Store_Tag_SI               0xA         /* 2       2 */
#define Index_Store_Tag_SD               0xB         /* 2       3 */
#define Create_Dirty_Exc_D               0xD         /* 3       1 */
#define Create_Dirty_Exc_SD              0xF         /* 3       3 */
#define Hit_Invalidate_I                 0x10        /* 4       0 */
#define Hit_Invalidate_D                 0x11        /* 4       1 */
#define Hit_Invalidate_SI                0x12        /* 4       2 */
#define Hit_Invalidate_SD                0x13        /* 4       3 */
#define Fill_I                           0x14        /* 5       0 */
#define Hit_Writeback_Inv_D              0x15        /* 5       1 */
#define Hit_Writeback_Inv_SD             0x17        /* 5       3 */
#define Hit_Writeback_I                  0x18        /* 6       0 */
#define Hit_Writeback_D                  0x19        /* 6       1 */
#define Hit_Writeback_SD                 0x1B        /* 6       3 */
#define Hit_Set_Virtual_SI               0x1E        /* 7       2 */
#define Hit_Set_Virtual_SD               0x1F        /* 7       3 */
#endif

/* Watchpoint Register */
#ifndef WATCH_PA
#define WATCH_PA	0xfffffff8
#define WATCH_R		0x00000002
#define WATCH_W		0x00000001
#endif

#define TLBMASK_MASKMASK	0x01ffe000
#define TLBMASK_MASK4Kb		0x00000000
#define TLBMASK_MASK16kb	0x00006000
#define TLBMASK_MASK64kb	0x0001e000
#define TLBMASK_MASK256kb	0x0007e000
#define TLBMASK_MASK1Mb		0x001fe000
#define TLBMASK_MASK4Mb		0x007fe000
#define TLBMASK_MASK16Mb	0x01ffe000

#define TLBMASK_4100_MASKMASK	0x0007f800
#define TLBMASK_4100_MASK1Kb	0x00000000
#define TLBMASK_4100_MASK4Kb	0x00001800
#define TLBMASK_4100_MASK16kb	0x00007800
#define TLBMASK_4100_MASK64kb	0x0001f800
#define TLBMASK_4100_MASK256kb	0x0007f800

/* FIXME 64bit TLB entries */
#define TLBHI_ASIDMASK		0x000000ff
#define TLBHI_VPN2MASK		0xffffe000
#define TLBHI_4100_VPN2MASK	0xfffff800

#define TLBLO_G			0x00000001
#define TLBLO_V			0x00000002
#define TLBLO_D			0x00000004
#define TLBLO_CALGMASK		0x00000038
#define TLBLO_PFNMASK		0x3fffffc0

#endif /* R4000 */

#define	PRID_PMC_RM5231A	(0x2831)
#define PRID_BLX_GODSONI	(0x4200)
#define PRID_BLX_GODSON2B	(0x6300)
#define PRID_BLX_GODSON2C	(0x6301)

#endif /* _MIPS_ */

