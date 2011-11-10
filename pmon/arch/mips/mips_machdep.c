/*	$Id: mips_machdep.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2001 Opsycon AB  (www.opsycon.se)
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

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/endian.h>
#include <sys/device.h>
#include <machine/cpu.h>
#include <machine/pio.h>
#include <machine/intr.h>
#include <dev/pci/pcivar.h>

#include <stdlib.h>

#include <autoconf.h>

#include <pmon.h>

extern char end[];
extern char edata[];
extern int memorysize;

extern char MipsException[], MipsExceptionEnd[];

char hwethadr[6];

static int clkenable;
static unsigned long clkpertick;
static unsigned long clkperusec = 500;
static unsigned long _softcompare;

int copytoram __P((void *, void *));
void clearbss __P((void));


void clearbss(void)
{
	u_int count;
	u_int64_t *rdata;
	/*
	 *  Clear BSS.
	 */
	rdata = (u_int64_t *)CACHED_TO_UNCACHED(edata);
	rdata = (u_int64_t *)edata;
	while((int)rdata & (sizeof(*rdata) - 1)) {
		*((char *)rdata)++ = 0;
	}
	count = (end - edata) / sizeof(*rdata);
	while(count--) {
		*rdata++ = 0;
	}
}
	
int
copytoram(void *rom, void *ram)
{
	u_int count;
	u_int64_t *rdata, *pdata;

	/*
	 *  Copy ROM to RAM memory. 
	 */
	pdata = (u_int64_t *)rom;
	rdata = (u_int64_t *)CACHED_TO_UNCACHED(ram);
	count = CACHED_TO_PHYS(end) - CACHED_TO_PHYS(ram);
	while(count > 0) {
		*rdata++ = *pdata++;
		count -= sizeof(*rdata);
	}

	/*
	 *  Verify copy ROM to RAM memory. 
	 */
	pdata = (u_int64_t *)rom;
	rdata = (u_int64_t *)CACHED_TO_UNCACHED(ram);
	count = CACHED_TO_PHYS(end) - CACHED_TO_PHYS(ram);
	while(count > 0) {
		if(*rdata++ != *pdata++) {
			return((int)rdata - sizeof(*rdata));	/* non zero */
		}
		count -= sizeof(*rdata);
	}
	clearbss();
	return(0);
}

/*
 *  Microtime returns the time that have elapsed since last 'tick'.
 *  To be able to do that we use the decrementer to figure out
 *  how much time have elapsed.
 */

void
microtime (tvp)
	struct timeval *tvp;
{
static struct timeval lasttime;
	unsigned long count;
	long cycles;

	*tvp = time;

	/* work out how far we've progressed since the last "tick" */
	count = CPU_GetCOUNT();
	cycles = count - (_softcompare - clkpertick);

	if (cycles >= 0)
		tvp->tv_usec += cycles / clkperusec;
	else
		log(LOG_INFO, "microtime: cnt=%u cmp=%u\n", count, _softcompare);

	if (tvp->tv_usec >= 1000000) {
		tvp->tv_sec += tvp->tv_usec / 1000000;
		tvp->tv_usec %= 1000000;
	}

	if (tvp->tv_sec == lasttime.tv_sec &&
	    tvp->tv_usec <= lasttime.tv_usec &&
	    (tvp->tv_usec = lasttime.tv_usec + 1) > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	lasttime = *tvp;
}


void
startrtclock(hz)
	int hz;
{
	unsigned long freq;

	freq = tgt_pipefreq() / 2;	/* TB ticker frequency */

	/* get initial value of real time clock */
	time.tv_sec = tgt_gettime ();
	time.tv_usec = 0;
	if(time.tv_sec < 0) { /* Bad clock ? */
		time.tv_sec = 0;
	}

	clkpertick = freq / hz;
	clkperusec = freq / 1000000;
	_softcompare = CPU_GetCOUNT() + clkpertick;
	clkenable = 0;

	SBD_DISPLAY("RTCL",0);
}

void
enablertclock()
{
	clkenable = 1;
}

void
tgt_clkpoll ()
{
	struct clockframe cframe;
	unsigned long count;
	long cycles, ticks;

	if (!clkenable) {
		return;
	}
	cframe.pc = 0;
	cframe.sr = 0;
	cframe.cr = 0;

	/* poll the free-running clock */
	count = CPU_GetCOUNT();
	cycles = count - _softcompare;

	if (cycles > 0) {

		/* as we are polling, we could have missed a number of ticks */
		ticks = (cycles / clkpertick) + 1;
		_softcompare += ticks * clkpertick;

        /* There is a race between reading count and setting compare
         * whereby we could set compare to a value "below" the new
         * clock value.  Check again to avoid an 80 sec. wait
         */
		cycles = CPU_GetCOUNT() - _softcompare;
		while (cycles > 0) {
			_softcompare += clkpertick; ticks++;
			cycles = CPU_GetCOUNT() - _softcompare;
		}
		while(ticks--) {
			hardclock(&cframe);
		}

	}
}

void
cpu_initclocks()
{
	printf("cpu_initclocks\n");
}

void
tgt_machreset()
{
	tgt_netreset();
}

void
delay(microseconds)
	int microseconds;
{
	int total, start ,start1;
#if 1
	start = CPU_GetCOUNT();
	start1 = CPU_GetCOUNT();
	if(start!=start1)
	{
	total = microseconds * clkperusec;
	while(total > (CPU_GetCOUNT() - start));
	}
	else
	 for(start1=0;start1<microseconds;start1++)
	 *(volatile char *)0xbfc00000;
#else
	total = microseconds * 200;
	loopNinstr(total);
#endif
}

extern void idle();
void delay1(int microseconds){
	int total, start;
	start = CPU_GetCOUNT();
	total = microseconds * clkperusec*1000;

	while(total > (CPU_GetCOUNT() - start))
	{
		idle();
	};
}

u_int __res_randomid(void);
u_int
__res_randomid()
{
extern int ticks;
	return(ticks * CPU_GetCOUNT());    /* Just something... */
}

/*for toolchain mips-elf-gcc mips3 use 32 fpu regs*/
void tgt_fpuenable()
{
#if __mips < 3
asm(\
"mfc0 $2,$12;\n" \
"li   $3,0x30000000 #ST0_CU1;\n" \
"or   $2,$3;\n" \
"mtc0 $2,$12;\n" \
"li $2,0x00000000 #FPU_DEFAULT;\n" \
"ctc1 $2,$31;\n" \
:::"$2","$3"
	);
#else
asm(\
"mfc0 $2,$12;\n" \
"li   $3,0x34000000 #ST0_CU1;\n" \
"or   $2,$3;\n" \
"mtc0 $2,$12;\n" \
"li $2,0x00000000 #FPU_DEFAULT;\n" \
"ctc1 $2,$31;\n" \
:::"$2","$3"
	);
#endif
}

