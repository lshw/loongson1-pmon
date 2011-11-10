/* test.c - MemTest-86  Version 3.2
 *
 * Released under version 2 of the Gnu Public License.
 * By Chris Brady
 */
#include <stdio.h>
#include <termio.h>
#include <string.h>
#include <setjmp.h>
#include <sys/endian.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif

#include <machine/cpu.h>

#include <pmon.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#define rm9000_tlb_hazard(...)
#define CONFIG_PAGE_SIZE_64KB
#define SERIAL_CONSOLE_DEFAULT 0

extern char *heaptop;
int returncode=0;
struct termio sav;
extern int memorysize;
#define LOW_TEST_ADR    ((unsigned int)heaptop)      /* Final adrs for test code */
#define HIGH_TEST_ADR   (0x80000000+memorysize)      /* Relocation base address */
#include "test.h"
#define DEFTESTS 9
jmp_buf         jmpb_mt;	

volatile ulong *p = 0;
ulong p1 = 0, p2 = 0, p0 = 0;
int segs = 0, bail = 0;
int test_ticks;
int nticks;
int ecount=0;

char firsttime = 0;
static autotest=0;

struct vars variables = {};
struct vars * const v = &variables;

void mv_error(ulong *adr, ulong good, ulong bad);
void poll_errors();
short memsz_mode = SZ_MODE_BIOS;
struct cpu_ident cpu_id;
static void compute_segments(int win);

#include "fb.h"
#include "config.c"
#include "patn.c"
#include "screen_buffer.c"
#include "extra.c"
#include "lib.c"
#include "random.c"
#include "test.c"
#include "init.c"






struct tseq tseq[] = {
	{0, 5, 3, 0, 0,    "[Address test, walking ones, no cache]"},
	{1, 6, 3, 2, 0,    "[Address test, own address]           "},
	{1, 0, 3, 14, 0,   "[Moving inversions, ones & zeros]     "},
	{1, 1, 2, 80, 0,   "[Moving inversions, 8 bit pattern]    "},
	{1, 10, 60, 300, 0,"[Moving inversions, random pattern]   "},
	{1, 7, 64, 66, 0,  "[Block move, 64 moves]                "},
	{1, 2, 2, 320, 0,  "[Moving inversions, 32 bit pattern]   "},
	{1, 9, 40, 120, 0, "[Random number sequence]              "},
	{1, 3, 4, 240, 0,  "[Modulo 20, ones & zeros]             "},
	{1, 8, 1, 2, 0,    "[Bit fade test, 90 min, 2 patterns]   "},
	{0, 0, 0, 0, 0, NULL}
};

void restart()
{
    int i;
    volatile char *pp;

    /* clear variables */
    firsttime = 0;
    v->test = 0;
    v->pass = 0;
    v->msg_line = 0;
    v->ecount = 0;
    v->ecc_ecount = 0;
#if (NMOD_X86EMU_INT10 > 0)||(NMOD_X86EMU > 0)
        /* Clear the screen */
        for(i=0, pp=(char *)(SCREEN_ADR+0); i<80*24; i++, pp+=2) {
                *pp = ' ';
        }
#elif NMOD_FRAMEBUFFER >0
	video_cls();
#endif
		returncode=1;
		longjmp(jmpb_mt,1);
}

static int window = 0;
static struct pmap windows[] = 
{
{ 0, 0}
};

/* Compute the total number of ticks per pass */
void find_ticks(void)
{
	int i, j, chunks;

	v->pptr = 0;

	/* Compute the number of SPINSZ memory segments in one pass */
	chunks = 0;
	for(j = 0; j < sizeof(windows)/sizeof(windows[0]); j++) {
		compute_segments(j);
		for(i = 0; i < segs; i++) {
			unsigned long len;
			len = v->map[i].end - v->map[i].start;
			chunks += (len + SPINSZ -1)/SPINSZ;
		}
	}
	compute_segments(window);
	window = 0;
	for (v->pass_ticks=0, i=0; i<DEFTESTS != NULL; i++) {

		/* Test to see if this test is selected for execution */
		if (v->testsel >= 0) {
			if (i != v->testsel) {
				continue;
			}
                }
		v->pass_ticks += find_ticks_for_test(chunks, i);
	}
}

#if 1
static int find_ticks_for_test(unsigned long chunks, int test)
{
	int ticks;
	ticks = chunks * tseq[test].ticks;
	if (tseq[test].pat == 5) {
		/* Address test, walking ones */
		ticks = 4;
	}
	return ticks;
}

static void compute_segments(int win)
{
	unsigned long wstart, wend;
	int i;

	/* Compute the window I am testing memory in */
	wstart = windows[win].start;
	wend = windows[win].end;
	segs = 0;

	/* Now reduce my window to the area of memory I want to test */
	if (wstart < v->plim_lower) {
		wstart = v->plim_lower;
	}
	if (wend > v->plim_upper) {
		wend = v->plim_upper;
	}
	if (wstart >= wend) {
		return;
	}
	/* List the segments being tested */
	for (i=0; i< v->msegs; i++) {
		unsigned long start, end;
		start = v->pmap[i].start;
		end = v->pmap[i].end;
		if (start <= wstart) {
			start = wstart;
		}
		if (end >= wend) {
			end = wend;
		}
#if 0
		cprint(LINE_SCROLL+(2*i), 0, " (");
		hprint(LINE_SCROLL+(2*i), 2, start);
		cprint(LINE_SCROLL+(2*i), 10, ", ");
		hprint(LINE_SCROLL+(2*i), 12, end);
		cprint(LINE_SCROLL+(2*i), 20, ") ");

		cprint(LINE_SCROLL+(2*i), 22, "r(");
		hprint(LINE_SCROLL+(2*i), 24, wstart);
		cprint(LINE_SCROLL+(2*i), 32, ", ");
		hprint(LINE_SCROLL+(2*i), 34, wend);
		cprint(LINE_SCROLL+(2*i), 42, ") ");

		cprint(LINE_SCROLL+(2*i), 44, "p(");
		hprint(LINE_SCROLL+(2*i), 46, v->plim_lower);
		cprint(LINE_SCROLL+(2*i), 54, ", ");
		hprint(LINE_SCROLL+(2*i), 56, v->plim_upper);
		cprint(LINE_SCROLL+(2*i), 64, ") ");

		cprint(LINE_SCROLL+(2*i+1),  0, "w(");
		hprint(LINE_SCROLL+(2*i+1),  2, windows[win].start);
		cprint(LINE_SCROLL+(2*i+1), 10, ", ");
		hprint(LINE_SCROLL+(2*i+1), 12, windows[win].end);
		cprint(LINE_SCROLL+(2*i+1), 20, ") ");

		cprint(LINE_SCROLL+(2*i+1), 22, "m(");
		hprint(LINE_SCROLL+(2*i+1), 24, v->pmap[i].start);
		cprint(LINE_SCROLL+(2*i+1), 32, ", ");
		hprint(LINE_SCROLL+(2*i+1), 34, v->pmap[i].end);
		cprint(LINE_SCROLL+(2*i+1), 42, ") ");

		cprint(LINE_SCROLL+(2*i+1), 44, "i=");
		hprint(LINE_SCROLL+(2*i+1), 46, i);
		
		cprint(LINE_SCROLL+(2*i+2), 0, 
			"                                        "
			"                                        ");
		cprint(LINE_SCROLL+(2*i+3), 0, 
			"                                        "
			"                                        ");
#endif
		if ((start < end) && (start < wend) && (end > wstart)) {
			v->map[segs].pbase_addr = start;
			v->map[segs].start = mapping(start);
			v->map[segs].end = emapping(end);
#if 0
			cprint(LINE_SCROLL+(2*i+1), 54, " segs: ");
			hprint(LINE_SCROLL+(2*i+1), 61, segs);
#endif
			segs++;
		}
	}
}
#endif
void set_cache(int val);
static int newmt()
{
	int i = 0, j = 0;
	unsigned long chunks;
	unsigned long lo, hi;
	int cnt;

#if NMOD_FRAMEBUFFER > 0 
	video_cls();
#endif
 	if(setjmp(jmpb_mt)&&(returncode==2)){
    	volatile char *pp;
		ioctl (STDIN, TCSETAF, &sav);
		    firsttime = 0;
    		v->test = 0;
    		v->pass = 0;
    		v->msg_line = 0;
   		    v->ecount = 0;
    		v->ecc_ecount = 0;
			autotest=0;
			firsttime=0;
        /* Clear the screen */
#if NMOD_FRAMEBUFFER > 0
		video_cls();
#else
        for(i=0, pp=(char *)(SCREEN_ADR+0); i<80*25; i++, pp+=2) {
                *pp = ' ';pp[1]=7;
        }
#endif
        serial_echo_print("[H[2J");   /* Clear Screen */
		return 0;
	    }

		ioctl (STDIN, CBREAK, &sav);
while(1)
{
		window=0;
	/* If first time, initialize test */
		windows[0].start =LOW_TEST_ADR>>12;
		windows[0].end= HIGH_TEST_ADR>>12;
		if(!firsttime){init();firsttime++;}

	/* Find the memory areas I am going to test */
	compute_segments(window);
	if (segs == 0) {
		goto skip_window;
	}
	/* Now map in the window... */
	if (map_page(v->map[0].pbase_addr) < 0) {
		goto skip_window;
	}

	/* Update display of memory segments being tested */
	lo = page_of(v->map[0].start);
	hi = page_of(v->map[segs -1].end);
	aprint(LINE_RANGE, COL_MID+9, lo-0x80000);
	cprint(LINE_RANGE, COL_MID+14, " - ");
	aprint(LINE_RANGE, COL_MID+17, hi-0x80000);
	aprint(LINE_RANGE, COL_MID+23, v->selected_pages);
	cprint(LINE_RANGE, COL_MID+28, 
		((ulong)&_start == LOW_TEST_ADR)?"          ":" Relocated");

		while(!autotest) {
			cnt=check_input();
			if(cnt>='0' && cnt<='9'){v->testsel=(cnt-'0')%9;break;}
			if(cnt=='a'){autotest=1;v->test=0;v->pass=0;}
		}


	/* Now setup the test parameters based on the current test number */
	/* Figure out the next test to run */
	if (v->testsel >= 0) {
		v->test = v->testsel;
	}
	dprint(LINE_TST, COL_MID+6, v->test, 2, 1);
	cprint(LINE_TST, COL_MID+9, tseq[v->test].msg);
	set_cache(tseq[v->test].cache);

	/* Compute the number of SPINSZ memory segments */
	chunks = 0;
	for(i = 0; i < segs; i++) {
		unsigned long len;
		len = v->map[i].end - v->map[i].start;
		chunks += (len + SPINSZ -1)/SPINSZ;
	}

	test_ticks = find_ticks_for_test(chunks, v->test);
	nticks = 0;
	v->tptr = 0;
	cprint(1, COL_MID+8, "                                         ");
	switch(tseq[v->test].pat) {

	/* Now do the testing according to the selected pattern */
	case 0:	/* Moving inversions, all ones and zeros */
		p1 = 0;
		p2 = ~p1;
		movinv1(tseq[v->test].iter,p1,p2);
		BAILOUT;
	
		/* Switch patterns */
		p2 = p1;
		p1 = ~p2;
		movinv1(tseq[v->test].iter,p1,p2);
		BAILOUT;
		break;
		
	case 1: /* Moving inversions, 8 bit wide walking ones and zeros. */
		p0 = 0x80;
		for (i=0; i<8; i++, p0=p0>>1) {
			p1 = p0 | (p0<<8) | (p0<<16) | (p0<<24);
			p2 = ~p1;
			movinv1(tseq[v->test].iter,p1,p2);
			BAILOUT;
	
			/* Switch patterns */
			p2 = p1;
			p1 = ~p2;
			movinv1(tseq[v->test].iter,p1,p2);
			BAILOUT
		}
		break;

	case 2: /* Moving inversions, 32 bit shifting pattern, very long */
		for (i=0, p1=1; p1; p1=p1<<1, i++) {
			movinv32(tseq[v->test].iter,p1, 1, 0x80000000, 0, i);
			BAILOUT
			movinv32(tseq[v->test].iter,~p1, 0xfffffffe,
				0x7fffffff, 1, i);
			BAILOUT
		}
		break;

	case 3: /* Modulo X check, all ones and zeros */
		p1=0;
		for (i=0; i<MOD_SZ; i++) {
			p2 = ~p1;
			modtst(i, tseq[v->test].iter, p1, p2);
			BAILOUT

			/* Switch patterns */
			p2 = p1;
			p1 = ~p2;
			modtst(i, tseq[v->test].iter, p1,p2);
			BAILOUT
		}
		break;

	case 4: /* Modulo X check, 8 bit pattern */
		p0 = 0x80;
		for (j=0; j<8; j++, p0=p0>>1) {
			p1 = p0 | (p0<<8) | (p0<<16) | (p0<<24);
			for (i=0; i<MOD_SZ; i++) {
				p2 = ~p1;
				modtst(i, tseq[v->test].iter, p1, p2);
				BAILOUT

				/* Switch patterns */
				p2 = p1;
				p1 = ~p2;
				modtst(i, tseq[v->test].iter, p1, p2);
				BAILOUT
			}
		}
		break;
	case 5: /* Address test, walking ones */
		addr_tst1();
		BAILOUT;
		break;

	case 6: /* Address test, own address */
		addr_tst2();
		BAILOUT;
		break;

	case 7: /* Block move test */
		block_move(tseq[v->test].iter);
		BAILOUT;
		break;
	case 8: /* Bit fade test */
		if (window == 0 ) {
			bit_fade();
		}
		BAILOUT;
		break;
	case 9: /* Random Data Sequence */
		for (i=0; i < tseq[v->test].iter; i++) {
			movinvr();
			BAILOUT;
		}
		break;
	case 10: /* Random Data */
		for (i=0; i < tseq[v->test].iter; i++) {
			p1 = Rand();
			p2 = ~p1;
			movinv1(2,p1,p2);
			BAILOUT;
		}
		break;
	}
 skip_window:
	if (bail) {
		goto bail_test;
	}
	/* Rever to the default mapping and enable the cache */
	paging_off();
	set_cache(1);
	window++;
	if (window >= sizeof(windows)/sizeof(windows[0])) {
		window = 0;
	}
	/* We finished the test so clear the pattern */
	cprint(LINE_PAT, COL_PAT, "            ");
	skip_test:
		v->test++;
	bail_test:

		paging_off();
		set_cache(1);
		check_input();
		window = 0;
		cprint(LINE_PAT, COL_PAT-3, "   ");
		/* If this was the last test then we finished a pass */
		if (v->test >= DEFTESTS || v->testsel >= 0) {
			v->pass++;
			dprint(LINE_INFO, COL_PASS, v->pass, 5, 0);
			v->test = 0;
			v->total_ticks = 0;
			v->pptr = 0;
			cprint(0, COL_MID+8,
				"                                         ");
		}
	
}
	return 0;
}

//-------------------------------------------------------------------------------------------
static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"newmt","",0,"new memory test",newmt,0,99,CMD_REPEAT},
	{0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}


