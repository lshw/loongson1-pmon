/*************************************************************
 * File: lib/r3kcfl.s
 * Purpose: Part of C runtime library
 * Author: Phil Bunce (pjb@carmel.com)
 * Revision History:
 *	970304	Start of revision history
 */

#include "mips.h"

.comm	icache_size,4
.comm	dcache_size,4

/*************************************************************
*  r3k_flush(type,adr)
*	Flush the designated LR3000 cache
*/
	.globl r3k_flush
	.ent r3k_flush
r3k_flush:
	bne	a0,ICACHE,1f
	j	r3k_iflush

   1:	bne	a0,ICACHE,1f
	j	r3k_iflush

   1:	bne	a0,DCACHE,1f
	j	r3k_dflush

   1:	bne	a0,DCACHEI,1f
	j	r3k_dflush

   1:	move	a0,a1
	j	r3k_iaflush
	.end r3k_flush

/*************************************************************
*  r3k_iflush()
*	Flush the LR3000 Icache
*	Note that this routine saves it's return address in t9, in
*	order to avoid the need for memory accesses.
*/
	.globl	r3k_iflush
	.ent	r3k_iflush
r3k_iflush:
	move	t9,ra
	
	# make me uncacheable
	la	t0,1f
	li	t1,K1BASE
	or	t0,t1
	j	t0

1: 	# get size
	la	t0,icache_size
	li	t1,K1BASE
	or	t0,t1
	lw	v0,(t0)

	# if size == 0, call size_cache
	bne	v0,zero,1f
	li	a0,(SR_ISC|SR_SWC)	# sr bits
	jal	size_cache

	# if size == 0, return
	beq	v0,zero,10f

	# update size
	la	t0,icache_size
	li	t1,K1BASE
	or	t0,t1
	sw	v0,(t0)

1:	li	a0,(SR_ISC|SR_SWC)	# sr bits
	move	a1,v0			# size

	jal	flush_common

10:	j	t9
	.end	r3k_iflush

/*************************************************************
*  r3k_dflush()
*	Flush the LR3000 Dcache
*	Note that this routine saves it's return address in t9, in
*	order to avoid the need for memory accesses.
*/
	.globl	r3k_dflush
	.ent	r3k_dflush
r3k_dflush:
	move	t9,ra

	# get size
	la	t0,dcache_size
	li	t1,K1BASE
	or	t0,t1
	lw	v0,(t0)

	# if size == 0, call size_cache
	bne	v0,zero,1f
	li	a0,SR_ISC	# sr bits
	jal	size_cache

	# if size == 0, return
	beq	v0,zero,10f

	# update size
	la	t0,dcache_size
	li	t1,K1BASE
	or	t0,t1
	sw	v0,(t0)

1:	li	a0,SR_ISC	# sr bits
	move	a1,v0		# size

	jal	flush_common

10:	j	t9
	.end	r3k_dflush

/*************************************************************
*  flush_common()
*	common cache flushing code
*/
	.globl flush_common
	.ent flush_common
flush_common:
	# a0=sr bits a1=size

	.set noreorder
	mfc0	t8,C0_SR
	nop
	or	t0,a0,t8
	and	t0,~SR_IEC	# disable ints
	mtc0	t0,C0_SR

	li	v0,K0BASE
	addu	v1,v0,a1
1:
	sb	zero,0x0(v0)	/* See - "LR3000 and LR3000A MIPS RISC	*/
	sb	zero,0x4(v0)	/*        Microprocessor User's Manual"	*/
	sb	zero,0x8(v0)	/*	  P8-9 'Cache Isolation'	*/
	sb	zero,0xc(v0)
	addiu	v0,v0,0x10
	bltu	v0,v1,1b
	nop

	mtc0	t8,C0_SR	# restore sr
	.set reorder
	j	ra
	.end flush_common

/*************************************************************
*  size_cache()
*	cache sizing code
*/
	.globl size_cache
	.ent size_cache
size_cache:
	# a0=sr bits rtn=size
	.set noreorder
	mfc0	t8,C0_SR
	nop
	or	t0,a0,t8
	and	t0,~SR_IEC	# disable ints
	mtc0	t0,C0_SR

	/* clear possible cache boundaries */
	lui	v0,0x8000
	sw	zero,0x1000(v0)		/* clear KSEG0	(+  4K) */
	sw	zero,0x2000(v0)		/*		(+  8K) */
	sw	zero,0x4000(v0)		/*		(+ 16K) */
	ori	v0,0x8000
	sw	zero,0(v0)		/*		(+ 32K) */
	lui	v0,0x8001
	sw	zero,0(v0)		/*		(+ 64K) */
	lui	v0,0x8002
	sw	zero,0(v0)		/*		(+128K) */

	lui	a0,0x8000		/* set marker */
	li	a1,0x6d61726b		/* "mark" */
	sw	a1,0(a0)

	li	t0,SR_CM		/* cache miss bit */

	li	v0,0		/* no cache if we fail the next tests */
	lw	a2,0(a0)
	mfc0	a3,C0_SR
	nop
	and	a3,t0
	bne	a3,zero,2f		# bra if cache miss
	nop
	bne	a1,a2,2f
	nop

	li	v0,0x1000		/* search marker */

1:	addu	t1,a0,v0
	lw	a2,0(t1)
	mfc0	a3,C0_SR
	nop
	and	a3,t0
	bne	a3,zero,2f		# bra if cache miss
	nop
	beq	a1,a2,2f		/* check data */
	sll	v0,1
	j	1b
	nop
	/* If your system has over than 256MB cache, Fix here! :-) */

2: 	mtc0	t8,C0_SR
	.set reorder
	j	ra
	.end size_cache

/*************************************************************
*  r3k_iaflush(adr)
*	Flush a single line of the LR3000 Icache
*/
	.globl r3k_iaflush
	.ent r3k_iaflush
r3k_iaflush:
	# a0=addr
	# word align the address
	li	t0,~3
	and	a0,t0

	.set noreorder
	mfc0	t8,C0_SR
	li	t0,(SR_ISC|SR_SWC)
	or	t0,t8
	mtc0	t0,C0_SR
	.set reorder

	sb	zero,(a0)

	.set noreorder
	mtc0	t8,C0_SR
	.set reorder
	j	ra
	.end r3k_iaflush

