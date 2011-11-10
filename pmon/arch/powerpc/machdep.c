/*	$Id: machdep.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2000-2001 Opsycon AB  (http://www.opsycon.se)
 * Copyright (c) 2000-2001 RTMX, Inc   (http://www.rtmx.com)
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
#include <powerpc.h>
#include <pmon.h>

#include "mainbus.h"

extern struct trapframe DBGREG;
extern u_int32_t FPREG[];
extern int memorysize;

extern int trapcode, trapsize;
extern int eintrcode, eintrsize;
extern int isitrap, isisize;
extern int dsitrap, dsisize;
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

int copytoram (void *, void *);
void initppc (int);
void initppcsmp (void);
void extint_handler __P((void));

struct bat battable[16];	/* Translation table, PMON */
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
	syncicache(rdata, (int)end - (int)ram);  /* pram issue (bel) */
	return(0);
}



/*
 *  This code is called from start.S to launch PMON2000 after having
 *  completed the final setup. Do all setup that does not really need
 *  assembly code here to keep down the size of .start.S'.
 *  NOTE that until translation is enabled no I/O must be done so
 *  targets that enable caches in start.S will work.
 */
void
initppc(msize)
	int msize;
{
	int i, exc, scratch;

	memorysize = msize;

	/*
	 *  Set up an initial mapping using BAT registers.
	 *  First clear all to avoid any old setting overlap.
	 */
        __asm__ volatile ("mtibatu 0,%0" :: "r"(0));
        __asm__ volatile ("mtibatu 1,%0" :: "r"(0));
        __asm__ volatile ("mtibatu 2,%0" :: "r"(0));
        __asm__ volatile ("mtibatu 3,%0" :: "r"(0));
        __asm__ volatile ("mtdbatu 0,%0" :: "r"(0));
        __asm__ volatile ("mtdbatu 1,%0" :: "r"(0));
        __asm__ volatile ("mtdbatu 2,%0" :: "r"(0));
        __asm__ volatile ("mtdbatu 3,%0" :: "r"(0)); 
	/*
	 *  IBAT0 and DBAT0 points at first 256Mb of RAM.
	 */
	__asm__ volatile ("mtibatl 0,%0; mtibatu 0,%1"
                      :: "r"(BATL(0x00000000, BAT_M)),
			 "r"(BATU(0x00000000, BAT_256M)));
	__asm__ volatile ("mtdbatl 0,%0; mtdbatu 0,%1"
                      :: "r"(BATL(0x00000000, BAT_M)),
			 "r"(BATU(0x00000000, BAT_256M)));

#if defined(PPC604MMU)
	/*
         *  Then map ISA/PCI I/O space and PCI Mem space.
         */
	__asm__ volatile ("mtdbatl 1,%0; mtdbatu 1,%1"
			:: "r"(BATL(PCI_IO_BASE, BAT_I)),
			"r"(BATU(PCI_IO_BASE, BAT_256M)));
	__asm__ volatile ("mtdbatl 2,%0; mtdbatu 2,%1"
			:: "r"(BATL(PCI_MEM_BASE, BAT_I)),
			 "r"(BATU(PCI_MEM_BASE, BAT_256M)));
/*XXX*/
	__asm__ volatile ("mtdbatl 3,%0; mtdbatu 3,%1"
			:: "r"(BATL(0xf0000000, BAT_I)),
			"r"(BATU(0xf0000000, BAT_256M)));
#endif

	savebat(&trapbat);
	cpuinfotab[0] = md_getcpuinfoptr();

	/*
	 * Set up bat table for linear mapping of entire space.
	 * This has to be uncached since some I/O registers will
	 * be handled by the BAT spill handler when accessed.
	 */
	for(i = 0; i < 16; i++) {
		battable[i].batu = BATU(i << 28, BAT_256M);
		battable[i].batl = BATL(i << 28, BAT_I);
	}

	/*
	 *  Set up trap vectors
	 */
	for (exc = EX_BEGIN; exc <= EX_END; exc += 0x100) {
		switch(exc) {
		case EX_DSI:
			bcopy((char *)&dsitrap, (char *)exc, (size_t)&dsisize);
			break;

		case EX_ISI:
			bcopy((char *)&isitrap, (char *)exc, (size_t)&isisize);
			break;

		case EX_EINT:
			bcopy((char *)&eintrcode, (char *)exc, (size_t)&eintrsize);
			break;

		default:
			bcopy((char *)&trapcode, (char *)exc, (size_t)&trapsize);
			break;
		}
	}
	syncicache((void *)EX_BEGIN, EX_END + 0x100 - EX_BEGIN);

	/*
	 *  Now turn on translation.
	 */

	__asm__ volatile ("eieio; mfmsr %0; ori %0,%0,%1; mtmsr %0; sync;isync"
                      : "=r"(scratch) : "K"(PSL_IR|PSL_DR|PSL_ME|PSL_RI));

        SBD_DISPLAY ("TRAN", CHKPNT_FREQ);

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

#if defined(SMP)
/*
 *  Main code entry for second+ CPUs.
 */
void
initppcsmp()
{
	int scratch;
extern void _pmon_break(void);

	/*
	 *  IBAT0 and DBAT0 points at first 256Mb of RAM.
	 */
	__asm__ volatile ("mtibatl 0,%0; mtibatu 0,%1"
                      :: "r"(BATL(0x00000000, BAT_M)),
			 "r"(BATU(0x00000000, BAT_256M)));
	__asm__ volatile ("mtdbatl 0,%0; mtdbatu 0,%1"
                      :: "r"(BATL(0x00000000, BAT_M)),
			 "r"(BATU(0x00000000, BAT_256M)));
	/*
	 *  Now turn on translation.
	 */

	__asm__ volatile ("eieio; mfmsr %0; ori %0,%0,%1; mtmsr %0; sync;isync"
                      : "=r"(scratch) : "K"(PSL_IR|PSL_DR|PSL_ME|PSL_RI));

	if (!getenv("nocache"))
		md_cacheon();
	cpuinfotab[tgt_smpwhoami()] = md_getcpuinfoptr();
	tgt_smpready();		/* Synchronize with CPU 0 */
	_pmon_break();		/* Go idle */

}

/*
 *  Do a processor fork, eg save state, copy and setup new stack
 *  and schedule next processor to run with the new setup.
 */
int
tgt_smpfork(size_t copysize, char *newstack)
{
extern int _pmon_snap(void);
	if (_pmon_snap() == 0) {
		char *oldstack;
		struct trapframe *myinfo = cpuinfotab[tgt_smpwhoami()];
		struct trapframe *soninfo = cpuinfotab[tgt_smpwhoami()+1];
		oldstack = (char *)myinfo->fixreg[1];
		newstack = newstack - copysize;
		memcpy(newstack, oldstack, copysize);
		*soninfo = *myinfo;
		soninfo->fixreg[1] = (register_t)newstack;
		soninfo->fixreg[3] = 1;
		tgt_smpschedule(tgt_smpwhoami()+1);
		return(tgt_smpwhoami()+1);
	}
	return(0);
}

#endif

/*
 *  Return a ascii version of the processor type.
 */
const char *
md_cpuname()
{
	int cputype;

	cputype = md_cputype();

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
		switch((cputype >> 8) & 0xff) {
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
			return("750");
		case 0x22:
			return("750CX");
		case 0x33:
			return("750CXe");
		case 0x31:
		case 0x32:
			return("755");
		case 0x83:
			return("750L");
		default:
			return("750??");	/* We don't really know... */
		}
	case CPU_TYPE_750FX:
		return("750FX");
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
}

/*
 *  Return the pipe-line freq.
 */
int
md_getpipefreq(int busfreq)
{
	int cputype;
	int pipefreq = -1;
	int i;
static char ppc7400_pllratio[16] =
	/* 0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15 */
	{ 90, 75, 70,  1, 20, 65, 25, 45, 30, 55, 40, 50, 80, 60, 35, 0 };
static char ppc750_pllratio[16] =
	{  0, 75, 70,  1, 20, 65,100, 45, 30, 55, 40, 50, 80, 60, 35, 0 };	
static char ppc750fx_pllratio[32] =
	{  0,  0,  0,  0, 20, 25,  30, 35, 40, 45, 50, 55, 60, 65, 70, 75,
	  80, 85, 90, 95,100,110,120,130,140, 150,160,170,180,190,200, 0 };	

	cputype = md_cputype();

	GETHID1(i);		/* Get PLL info */ 
	i = (i >> 28) & 0x0f;

	switch((cputype >> 16) & 0xffff) {
		case CPU_TYPE_601:
		case CPU_TYPE_602:
		case CPU_TYPE_603:
		case CPU_TYPE_604:
		case CPU_TYPE_603E:
		case CPU_TYPE_603EV:
		case CPU_TYPE_604E:
		case CPU_TYPE_604EM:
			break;
		case CPU_TYPE_750:
			pipefreq = (busfreq / 10) * ppc750_pllratio[i];
			break;
		case CPU_TYPE_750FX:
			GETHID1(i);
			i = (i >> 27) & 0x1f;
			pipefreq = (busfreq / 10) * ppc750fx_pllratio[i];
			break;
		case CPU_TYPE_7400:
		case CPU_TYPE_7410:
			pipefreq = (busfreq / 10) * ppc7400_pllratio[i];
			break;
		default:
			break;
	}
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
	cframe.srr1 = 0;
	cframe.srr0 = 0;
	cframe.pri = 7;
	cframe.depth = 0;

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

u_int __res_randomid(void);
u_int
__res_randomid()
{
extern int ticks;
	return(ticks * get_count());	/* Just something... */
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
}

