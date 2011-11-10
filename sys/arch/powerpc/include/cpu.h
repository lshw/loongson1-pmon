/*	$OpenBSD: cpu.h,v 1.3 1997/10/13 10:53:42 pefo Exp $	*/
/*	$NetBSD: cpu.h,v 1.1 1996/09/30 16:34:21 ws Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
 * All rights reserved.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef	_MACHINE_CPU_H_
#define	_MACHINE_CPU_H_

/* BAT stuff */
#define BATU(vadr, size) (((vadr)&0xf0000000)|(size))
#define BATL(radr, wimg) (((radr)&0xf0000000)|(wimg)|0x2)

#define	BAT_256M	0x1ffe
#define	BAT_128M	0x0ffe
#define	BAT_64M		0x07fe
#define	BAT_32M		0x03fe
#define	BAT_16M		0x01fe
#define	BAT_8M		0x00fe
#define	BAT_4M		0x007e

#define BAT_W   0x40
#define BAT_I   0x20
#define BAT_M   0x10
#define BAT_G   0x08

/* Cpu types and their definitions */
#define CPU_TYPE_601	0x0001	/* PPC 601 CPU */
#define CPU_TYPE_602	0x0002	/* PPC 602 CPU */
#define CPU_TYPE_603	0x0003	/* PPC 603 CPU */
#define CPU_TYPE_604	0x0004	/* PPC 604 CPU */
#define CPU_TYPE_603E	0x0006	/* PPC 603e CPU */
#define CPU_TYPE_603EV	0x0007	/* PPC 603ev CPU */
#define CPU_TYPE_740	0x0008	/* PPC 740 CPU */
#define CPU_TYPE_750	0x0008	/* PPC 750 CPU (yep they goofed!) */
#define CPU_TYPE_750FX	0x7000	/* PPC 750FX CPU */
#define CPU_TYPE_604E	0x0009	/* PPC 604e CPU */
#define CPU_TYPE_604EM	0x000a	/* PPC 604e MACH5 CPU */
#define CPU_TYPE_7400	0x000c	/* PPC 7400 CPU */
#define CPU_TYPE_7410	0x800c	/* ??? PPC 7410 CPU */

#define	CACHELINESIZE	32			/* For now		XXX */


#define	CPU_HID0_BTIC	0x20	/* Branch target instruction cache enable */
#define	CPU_HID0_BHT	0x04	/* Branch history enable */

#ifndef _LOCORE

struct bat {
	u_int32_t batu;
	u_int32_t batl;
};
void savebat __P((struct bat *));
void loadbat __P((struct bat *));

extern struct bat trapbat;

#include <machine/frame.h>
#include <machine/psl.h>


/* Frame */

#define	CLKF_USERMODE(frame)	(((frame)->srr1 & PSL_PR) != 0)
#define	CLKF_BASEPRI(frame)	((frame)->pri == 0)
#define	CLKF_PC(frame)		((frame)->srr0)
#define	CLKF_INTR(frame)	((frame)->depth != 0)

#define	cpu_swapout(p)
#define cpu_wait(p)

extern void delay __P((int));
#define	DELAY(n)	delay(n)

extern volatile int want_resched;
extern volatile int astpending;

#define	need_resched()		(want_resched = 1, astpending = 1)
#define	need_proftick(p)	((p)->p_flag |= P_OWEUPC, astpending = 1)
#define	signotify(p)		(astpending = 1)

static __inline int get_count __P((void));
static __inline int
get_count()
{
	int _decr_;

	__asm__  volatile("mfspr %0, 22\n" : "=r"(_decr_));
	return( 0 - _decr_);
}

/*
 *  This inlineable function can be used to synchronize the data and
 *  instruction cache. I does not invalidate the data cache contents,
 *  just pushes back updated lines to sdram so they can be cached in
 *  with the correct contents next time the i cache misses.
 */
__inline void syncicache __P((void *, size_t));
extern __inline void
syncicache(from, len)
	void *from;
	size_t len;
{
	int l;		/* loop eval needs an unsigned.... */
	void *p = from;
	
	l = len + ((int)from & (CACHELINESIZE - 1));
	do {
		asm volatile ("dcbst 0,%0" :: "r"(p));
		p += CACHELINESIZE;
	} while ((l -= CACHELINESIZE) > 0);
	asm volatile ("sync");
	l = len + ((int)from & 31);
	do {
		asm volatile ("icbi 0,%0" :: "r"(from));
		from += CACHELINESIZE;
	} while ((l -= CACHELINESIZE) > 0);
	asm volatile ("isync");
}

/*
 *  This inlineable function invalidates the i cache so sdram data
 *  is cached in again bringing in a fresh copy of sdram data.
 */
__inline void flushicache __P((void *, size_t));
extern __inline void
flushicache(from, len)
	void *from;
	size_t len;
{
	int l;

	l = len + ((int)from & (CACHELINESIZE - 1));
	do {
		asm volatile ("icbi 0,%0" :: "r"(from));
		from += CACHELINESIZE;
	} while ((l -= CACHELINESIZE) > 0);
	asm volatile ("isync");
}

/*
 *  This inlineable function writes back a d cache line and *invalidates*
 *  the line in the d cache. This means that a fresh copy will be brought
 *  in from sdram when the d cache misses the next time. Using this function
 *  allows non snooping clients doing dma to be synchronized.
 */
__inline void flushdcache __P((void *, size_t));
extern __inline void
flushdcache(from, len)
	void *from;
	size_t len;
{
	int l;

	l = len + ((int)from & (CACHELINESIZE - 1));

	do {
		asm volatile ("dcbf 0,%0" :: "r"(from));
		from += CACHELINESIZE;
	} while ((l -= CACHELINESIZE) > 0);
	asm volatile ("sync");
}

#if defined(NOSNOOP) || defined(HORIZON)
#define	CACHESYNC(a,s, d)	flushdcache(a, s)
#else
#define	CACHESYNC(a,s, d)
#endif
#define SYNC_W 0
#define SYNC_R 1

extern char *bootpath;
#endif /* _LOCORE */

#endif	/* _MACHINE_CPU_H_ */
