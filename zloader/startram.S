#ifndef _KERNEL
#define _KERNEL
#endif


#define zero	$0	/* always zero */
#define AT	$at	/* assembler temp */
#define v0	$2	/* return value */
#define v1	$3
#define a0	$4	/* argument registers */
#define a1	$5
#define a2	$6
#define a3	$7
#define t0	$8	/* temp registers (not saved across subroutine calls) */
#define t1	$9
#define t2	$10
#define t3	$11
#define t4	$12
#define t5	$13
#define t6	$14
#define t7	$15
#define s0	$16	/* saved across subroutine calls (callee saved) */
#define s1	$17
#define s2	$18
#define s3	$19
#define s4	$20
#define s5	$21
#define s6	$22
#define s7	$23
#define t8	$24	/* two more temp registers */
#define t9	$25
#define k0	$26	/* kernel temporary */
#define k1	$27
#define gp	$28	/* global pointer */
#define sp	$29	/* stack pointer */
#define s8	$30	/* one more callee saved */
#define ra	$31	/* return address */


/*
 * Coprocessor 0 registers:
 */
#define COP_0_TLB_INDEX		$0
#define COP_0_TLB_RANDOM	$1
#define COP_0_TLB_LO0		$2
#define COP_0_TLB_LO1		$3
#define COP_0_TLB_CONTEXT	$4
#define COP_0_TLB_PG_MASK	$5
#define COP_0_TLB_WIRED		$6
#define COP_0_BAD_VADDR		$8
#define COP_0_COUNT		$9
#define COP_0_TLB_HI		$10
#define COP_0_COMPARE		$11
#define COP_0_STATUS_REG	$12
#define COP_0_CAUSE_REG		$13
#define COP_0_EXC_PC		$14
#define COP_0_PRID		$15
#define COP_0_CONFIG		$16
#define COP_0_LLADDR		$17
#define COP_0_WATCH_LO		$18
#define COP_0_WATCH_HI		$19
#define COP_0_TLB_XCONTEXT	$20
#define COP_0_ECC		$26
#define COP_0_CACHE_ERR		$27
#define COP_0_TAG_LO		$28
#define COP_0_TAG_HI		$29
#define COP_0_ERROR_PC		$30
/* RM7000 specific */
#define COP_0_WATCH_1		$18
#define COP_0_WATCH_2		$19
#define COP_0_WATCH_M		$24
#define COP_0_PC_COUNT		$25
#define COP_0_PC_CTRL		$22

#define	COP_0_ICR		$20		/* CFC */
#define	COP_0_DERR_0		$26		/* CFC */
#define	COP_0_DERR_1		$27		/* CFC */
#define SR_BOOT_EXC_VEC		0x00400000
//---------------------------------------------------

#define CONFIG_CACHE_64K_4WAY 1

#define tmpsize		s1
#define msize		s2
#define sdShape		s3
#define bonito		s4
#define dbg		s5
#define sdCfg		s6

#define	CFG_IB 0x00000020
#define CFG_DB 0x00000010
#define CFG_C_WBACK 3
#define CFG_BE 0x00008000
#define CFG_EPMASK 0x0f000000
#define CFG_EPD 0x00000000
#define CFG_EM_R4K 0x00000000
#define CFG_EMMASK 0x00c00000
#define CFG_AD 0x00800000

#define CP0_CONFIG $16
#define CP0_TAGLO  $28
#define CP0_TAGHI  $29

#define DDR100 0x04041091
#define	DDR266 0x0410435e
#define DDR300 0x041453df


/*
 *   Register usage:
 *
 *	s0	link versus load offset, used to relocate absolute adresses.
 *	s1	free
 *	s2	memory size.
 *	s3	sdShape.
 *	s4	Bonito base address.
 *	s5	dbg.
 *	s6	sdCfg.
 *	s7	rasave.
 *	s8	L3 Cache size.
 */

	.set	noreorder
	.globl	_start
	.globl	start
	.globl	__main
_start:
start:
	.globl	stack
stack = start - 0x4000		/* Place PMON stack below PMON start in RAM */
	mtc0	zero, COP_0_STATUS_REG
	mtc0	zero, COP_0_CAUSE_REG
	li	t0, SR_BOOT_EXC_VEC	/* Exception to Boostrap Location */
	mtc0	t0, COP_0_STATUS_REG
	la	sp, stack
	la	gp, _gp
	move s1,a3 //struct callvectors *cv

	bal	locate			/* Get current execute address */
	nop

/*
 *  We get here from executing a bal to get the PC value of the current execute
 *  location into ra. Check to see if we run from ROM or if this is ramloaded.
 */
locate:
	la	s0,start
	subu	s0,ra,s0
	and	s0,0xffff0000

#if 1 //Ê¹ÄÜcache
        mfc0   a0,COP_0_CONFIG
        and    a0,a0,0xfffffff8
        or     a0,a0,0x3
        mtc0   a0,COP_0_CONFIG
#endif

	/* Clear BSS */
	la	a0, _edata
	la	a2, _end
2:	sw	zero, 0(a0)
	bne	a2, a0, 2b
	addu	a0, 4

	li	a0, 0
	sw	a0, CpuTertiaryCacheSize /* Set L3 cache size */
	li  a0,MEMSIZE
	move a1,s1
 
	la	v0, initmips
	jalr	v0
	nop
stuck:
	b	stuck
	nop


	.rdata
hexchar:
	.ascii	"0123456789abcdef"

	.text
	.align	2
