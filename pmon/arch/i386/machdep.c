/*	$Id: machdep.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2000-2001 Opsycon AB  (http://www.opsycon.se)
 * Copyright (c) 2002 Patrik Lindergren   (http://www.lindergren.com)
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
 *	This product includes software developed for Rtmx, Inc by
 *	Opsycon Open System Consulting AB, Sweden.
 *	This product includes software developed by Patrik Lindergren.
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

/*
 * Pmon architecture dependent (and possibly cpu type dependent) code.
 */

#include <ctype.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/systm.h> 
#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif

#include <machine/cpu.h>
#include <pmon.h>

#include "mainbus.h"

extern struct trapframe DBGREG;
extern u_int32_t FPREG[];
extern int memorysize;

extern int trapcode, trapsize;
extern int eintrcode, eintrsize;
extern int memorysize;

#if NMAINBUS > 0
static int clkenable;
static unsigned long clkpertick;
static unsigned long clkperusec;
static unsigned long _softcompare;
#else
static unsigned long clkperusec;
static void tgt_clksetup(void);
#endif

int copytoram __P((void *, void *));
void initi386 __P((int));
void extint_handler __P((void));

char hwethadr[6];

volatile struct	timeval time;

/*
 *  This code is called from start.S to move the PROM contents to RAM
 *  after the initial SDRAM controller setup work has been done.
 */
int
copytoram(void *rom, void *ram)
{
extern u_int8_t edata[];
extern u_int8_t end[];
	u_int32_t *rdata, *pdata;
	u_int count;

	/*
	 *  Copy ROM to RAM memory. 
	 */
	pdata = (u_int32_t *)rom;
	rdata = (u_int32_t *)ram;
	count = (int)end - (int)ram;
	while(count > 0) {
		*rdata++ = *pdata++;
		count -= 4;
	}

	/*
	 *  Verify copy ROM to RAM memory. 
	 */
	pdata = (u_int32_t *)rom;
	rdata = (u_int32_t *)ram;
	count = (int)end - (int)ram;
	while(count > 0) {
		if(*rdata++ != *pdata++) {
			break;
		}
		count -= 4;
	}
	if(count > 0) {  /* Copy failed */
		return(1);
	}

	/*
	 *  Clear BSS.
	 */
	rdata = (u_int32_t *)edata;
	count = (end - edata) / 4;
	while(count--) {
		*rdata++ = 0;
	}
#if 0
	syncicache(rdata, (int)end - (int)ram);  /* pram issue (bel) */
#endif
	return(0);
}



/*
 *  This code is called from start.S to launch PMON/2000 after having
 *  completed the final setup. Do all setup that does not really need
 *  assembly code here to keep down the size of .start.S'.
 */
void
initi386(msize)
	int msize;
{
	memorysize = msize;

        SBD_DISPLAY ("INIT", CHKPNT_FREQ);

#ifdef HAVE_TOD
	/*
	 * Init Real-Time Clock
	 */
	tgt_rtinit();
#endif

	/*
	 *  Probe clock frequencys so delays will work properly.
	 */
	tgt_cpufreq();

#if NMAINBUS == 0
	/*
	 * Setup clock variables
	 */
	tgt_clksetup();
#endif /* PMON_SYS */
	
#ifdef PMON_PFLASH
	/*
	 * Init dynamic sized flash memory bank
	 */
	tgt_flashinit();
#endif /* PMON_PFLASH */
	
	/*
	 *  Init PMON and debug
	 */
        SBD_DISPLAY ("DBG", CHKPNT_FREQ);
	dbginit(NULL);

	/*
	 * Launch!
	 */
	main();
}

/*
 *  Return a ascii version of the processor type.
 */
const char *
md_cpuname()
{
	int cputype;

	cputype = md_cputype();
#if 0
	switch((cputype >> 16) & 0xffff) {
	case CPU_TYPE_601:
		return("601");
	case CPU_TYPE_602:
		return("602");
	case CPU_TYPE_603:
		return("603");
	case CPU_TYPE_604:
		return("604");
	case CPU_TYPE_603E:
		return("603e");
	case CPU_TYPE_603EV:
		return("603ev");
	case CPU_TYPE_750:
		switch((cputype >> 8) & 0xf0) {
		case 0x00:
			return("750");
		case 0x20:
			return("750CX");
		case 0x30:
			return("755");
		case 0x80:
			return("750L");
		default:
			return("7??");	/* We don't really know... */
		}
	case CPU_TYPE_604E:
		return("604e");
	case CPU_TYPE_604EM:
		return("604em");
	case CPU_TYPE_7400:
		return("7400");
	case CPU_TYPE_7410:
		return("7410");
	default:
		return("unknown");
	}
#else
	return ("intel");
#endif
}

/*
 *  Return the pipe-line freq.
 */
int
   md_getpipefreq(int busfreq)
{
	int pipefreq = 660000000;
	return(pipefreq);
}

#if NMAINBUS > 0
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
	count = get_count();
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

	freq = tgt_cpufreq () / 4;	/* TB ticker frequency */

	/* get initial value of real time clock */
	time.tv_sec = tgt_gettime ();
	time.tv_usec = 0;

	clkpertick = freq / hz;
	clkperusec = freq / 1000000;
	_softcompare = get_count() + clkpertick;
	clkenable = 0;
}

void
enablertclock()
{
	clkenable = 1;
}

void
cpu_initclocks()
{
	printf("cpu_initclocks\n");
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
#if 0
	cframe.srr1 = 0;
	cframe.srr0 = 0;
	cframe.pri = 7;
	cframe.depth = 0;
#endif
	/* poll the free-running clock */
	count = get_count ();
	cycles = count - _softcompare;

	if (cycles > 0) {

		/* as we are polling, we could have missed a number of ticks */
		ticks = (cycles / clkpertick) + 1;
		_softcompare += ticks * clkpertick;

	/* There is a race between reading count and setting compare
	 * whereby we could set compare to a value "below" the new
	 * clock value.  Check again to avoid an 80 sec. wait
	 */
			cycles = get_count() - _softcompare;
			while (cycles > 0) {
			_softcompare += clkpertick; ticks++;
			cycles = get_count() - _softcompare;
		}
                while(ticks--) {
			hardclock(&cframe);
		}
	}
} 
#else
void
tgt_clksetup()
{
	unsigned long freq;

	freq = tgt_cpufreq () / 4;	/* TB ticker frequency */
	clkperusec = freq / 1000000;
}
#endif /* NMAINBUS */


void
delay(int microseconds)
{
	int total, start;

	start = get_count();
	total = microseconds * clkperusec;

        while(total > (get_count() - start));
}

/*
 *  Handle an external interrupt dispatching it to registred handler.
 */

void
extint_handler()
{
}

void
flush_cache (type, adr)
     int type;
     void *adr;
{
#if 0
    switch (type) {
	case ICACHE:
		flushicache((void *)0, memorysize);
		break;

	case DCACHE:
		flushdcache((void *)0, memorysize);
		break;

	case IADDR:
		syncicache((void *)((int)adr & ~3), 4);
		break;

	case ICACHE|DCACHE:
		flushicache((void *)0, memorysize);
		flushdcache((void *)0, memorysize);
		break;
    }
#endif
}

