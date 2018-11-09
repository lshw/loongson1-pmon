/*************************************************************
 * File: lib/r4300cfl.s
 * Purpose: Part of C runtime library
 * Author: Phil Bunce (pjb@carmel.com)
 * Revision History:
 *	980304	Created
 */

#include <mips.h>

#ifdef flush_icache
#undef flush_icache
#endif
#ifdef wb_dcache
#undef wb_dcache
#endif

#ifdef ALGOR
#define flush_icache(o,r)	.set nowarn; \
			.word	(0xbc000000|((r)<<21)|((o)<<18)); \
			.set warn; 42:
#define hitinv_dcache(o,r)	.set nowarn; \
			.word	(0xbc110000|((r)<<21)|((o)<<18)); \
			.set warn; 42:
#define isttag_dcache(o,r)	.set nowarn; \
			.word	(0xbc090000|((r)<<21)|((o)<<18)); \
			.set warn; 42:
#define wb_dcache(o,r)	.set nowarn; \
			.word	(0xbc010000|((r)<<21)|((o)<<18)); \
			.set warn; 42:
#define read_dtag(o,r)	.set nowarn; \
			.word	(0xbc050000|((r)<<21)|((o)<<18)); \
			.set warn; 42:
#else
#define flush_icache(o,r)	.word	(0xbc000000|((r)<<21)|((o)<<18))
#define hitinv_dcache(o,r)	.word	(0xbc110000|((r)<<21)|((o)<<18))
#define isttag_dcache(o,r)	.word	(0xbc090000|((r)<<21)|((o)<<18))
#define wb_dcache(o,r)	.word	(0xbc010000|((r)<<21)|((o)<<18))
#define	read_dtag(o,r)	.word	(0xbc050000|((r)<<21)|((o)<<18))
#endif

#define Index_Invalidate 0	/* I */
#define Index_Writeback_Invalidate 0	/* D */
#define Index_Store_Tag	2	/* I & D */
#define Hit_Invalidate	4	/* I & D */

#define C0_TAGLO	$28
#define C0_TAGHI	$29
#define SR_DE		(1<<16)

/*************************************************************
*  r4300_flush(type,adr)
*	Flush the designated VR4300 cache.
*	Note that this isn't a real subroutine, it just transfers
*	control to the appropriate flush routine.
*
*	The VR4300 has a 16KB direct-mapped icache that has 8 words 
*	per line. It has a 8KB direct-mapped write-back dcache that 
*	has 4 words per line.
*
*	To flush the Icache, issue the instruction 0xbc010000.
*	To flush the Dcache, issue the instruction 0xbc020000.
*	To flush both caches, issue the instruction 0xbc030000.
*	To write-back a Dcache line, issue the instruction 0xbc040000
*		specifying offset and base.
*/
	.globl r4300_flush
	.ent r4300_flush
r4300_flush:
	bne	a0,ICACHEI,1f
	j	r4300_iflush

1:	bne	a0,ICACHE,1f
	j	r4300_iflush

1:	bne	a0,DCACHE,1f
	j	r4300_dflush

1:	bne	a0,DCACHEI,1f
	j	r4300_dflushi

1:	move	a0,a1
	j	r4300_iflush
	.end r4300_flush


/*************************************************************
*  r4300_iflush()
*	Flush the VR4300 Instruction cache.
*/
	.globl r4300_iflush
	.ent r4300_iflush
r4300_iflush:
	# disable ints
	.set noreorder
	mfc0	t7,C0_SR
	nop
	and	t0,t7,~SR_IEC
	or	t0,SR_DE
	mtc0	t0,C0_SR
	.set reorder

	# switch to Kseg1
	la	t0,1f
	li	t1,K1BASE
	or	t0,t1
	j	t0

1:	
	# save taglo
	.set noreorder
	mfc0	t6,C0_TAGLO

	li	t1,0x80000000
	addu	t0,t1,(16*1024)	# cache size

1: 	
	# set taglo
	mtc0	zero,C0_TAGLO
	nop
	flush_icache(0,r_t1)
	nop
	bne	t0,t1,1b
	addu	t1,16		# line size

	# restore taglo
	mtc0	t6,C0_TAGLO
	nop

	nop
	mtc0	t7,C0_SR	# restore SR
	nop
	nop
	.set reorder
	j	ra
	.end r4300_iflush

/*************************************************************
*  r4300_dflushi()
*	Initial flush of the VR4300 Data cache.
*/
	.globl r4300_dflushi
	.ent r4300_dflushi
r4300_dflushi:
	# disable ints
	.set noreorder
	mfc0	t7,C0_SR
	nop
	and	t0,t7,~SR_IEC
	or	t0,SR_DE
	mtc0	t0,C0_SR
	.set reorder

	# switch to Kseg1
	la	t0,1f
	li	t1,K1BASE
	or	t0,t1
	j	t0

1:	
	# save taglo & taghi
	.set noreorder
	mfc0	t6,C0_TAGHI
	mfc0	t5,C0_TAGLO

	# load valid virtual addresses into the cache tags
	li	t1,0x80000000
	addu	t0,t1,(8*1024)	# cache size
	mtc0	zero,C0_TAGHI
	mtc0	zero,C0_TAGLO
	nop
	nop

1:
	isttag_dcache(0,r_t1)
	nop
	bne	t0,t1,1b
	addu	t1,16		# line size
	.set reorder

	# now Hit_Invalidate all entries
	li	t1,0x80000000
	addu	t0,t1,(8*1024)	# cache size

	.set noreorder
1: 	
	mtc0	zero,C0_TAGLO
	nop
	nop
	hitinv_dcache(0,r_t1)
	nop
	bne	t0,t1,1b
	addu	t1,16		# line size

	# restore taglo & taghi
	mtc0	t5,C0_TAGLO
	mtc0	t6,C0_TAGHI
	nop
	mtc0	t7,C0_SR	# restore SR
	nop
	nop
	.set reorder
	j	ra
	.end r4300_dflushi

/*************************************************************
*  r4300_dflush()
*	Flush the VR4300 Data cache.
*	o disable ints
*	o switch to kseg1
*	o write-back the cache
*	o invalidate the cache
*/
	.globl r4300_dflush
	.ent r4300_dflush
r4300_dflush:
	# disable ints
	.set noreorder
	mfc0	t7,C0_SR
	nop
	and	t0,t7,~SR_IEC
	or	t0,SR_DE
	mtc0	t0,C0_SR
	.set reorder

	# switch to Kseg1
	la	t0,1f
	li	t1,K1BASE
	or	t0,t1
	j	t0
1:
	# save taglo
	.set noreorder
	mfc0	t6,C0_TAGLO

	# write-back and invalidate the Dcache
	li	t0,0x80000000
	addu	t1,t0,(8*1024)		# 8KB cache

1:	
	# set taglo
	mtc0	zero,C0_TAGLO
	nop
	wb_dcache(0,r_t0)
	nop
	bne	t0,t1,1b
	addu	t0,16		# line size

	mtc0	t6,C0_TAGLO	# restore taglo
	nop
	mtc0	t7,C0_SR	# restore SR
	nop
	.set reorder

	j	ra
	.end r4300_dflush


/*************************************************************
*  r4300_readdtag()
*	Read the VR4300 Dcache tag.
*/
	.globl r4300_readdtag
	.ent r4300_readdtag
r4300_readdtag:
	# disable ints
	.set noreorder
	mfc0	t7,C0_SR
	nop
	and	t0,t7,~SR_IEC
	or	t0,SR_DE
	mtc0	t0,C0_SR
	.set reorder

	# switch to Kseg1
	la	t0,1f
	li	t1,K1BASE
	or	t0,t1
	j	t0

1:	
	# save taglo & taghi
	.set noreorder
	mfc0	t5,C0_TAGLO
	mfc0	t6,C0_TAGHI
	move	t1,a0

	read_dtag(0,r_t1)
	nop
	nop

	# store result
	mfc0	t0,C0_TAGLO
	nop
	sw	t0,(a1)
	mfc0	t0,C0_TAGHI
	nop
	sw	t0,(a2)

	# restore taglo & taghi
	mtc0	t5,C0_TAGLO
	mtc0	t6,C0_TAGHI
	nop

	nop
	mtc0	t7,C0_SR	# restore SR
	nop
	nop
	.set reorder
	j	ra
	.end r4300_readdtag
