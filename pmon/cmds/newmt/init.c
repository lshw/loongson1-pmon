/* init.c - MemTest-86  Version 3.2
 *
 * Released under version 2 of the Gnu Public License.
 * By Chris Brady
 * ----------------------------------------------------
 * MemTest86+ V1.11 Specific code (GPL V2.0)
 * By Samuel DEMEULEMEESTER, sdemeule@memtest.org
 * http://www.x86-secret.com - http://www.memtest.org
 */

#include "fb.h"

extern short memsz_mode;
extern short firmware;

struct cpu_ident cpu_id;
ulong st_low, st_high;
ulong end_low, end_high;
ulong cal_low, cal_high;
ulong extclock;

static ulong memspeed(ulong src, ulong len, int iter);
static void cpu_type(void);
static void cacheable(void);
static int cpuspeed(void);

static void display_init(void)
{
	int i;
	volatile char *pp;

	serial_echo_init();
        serial_echo_print("[LINE_SCROLL;24r"); /* Set scroll area row 7-23 */
        serial_echo_print("[H[2J");   /* Clear Screen */
        serial_echo_print("[37m[44m");
        serial_echo_print("[0m");
        serial_echo_print("[37m[44m");
#if (NMOD_X86EMU_INT10 > 0)||(NMOD_X86EMU > 0)
	/* Clear screen & set background to blue */
	for(i=0, pp=(char *)(SCREEN_ADR); i<80*24; i++) {
		*pp++ = ' ';
		*pp++ = 0x17;
	}

	/* Make the name background red */
	for(i=0, pp=(char *)(SCREEN_ADR+1); i<TITLE_WIDTH; i++, pp+=2) {
		*pp = 0x47;
	}
	cprint(0, 0, "      Memtest-86 v3.2       ");

	/* Do reverse video for the bottom display line */
	for(i=0, pp=(char *)(SCREEN_ADR+1+(24 * 160)); i<80; i++, pp+=2) {
		*pp = 0x71;
	}
#endif
#if NMOD_FRAMEBUFFER >0
	/* Clear screen & set background to blue */
	video_set_background(0, 0, 128);
	/* Make the name background red */
	//TODO
	video_set_bg(128, 0, 0);
	cprint(0, 0, "      Memtest-86 v3.2       ");
	video_set_bg(0, 0, 128);
	/* Do reverse video for the bottom display line */
	//TODO
#endif
        serial_echo_print("[0m");
}

static void mem_size(void)
{
	int i, n;
	/* Build the memory map for testing */

		v->pmap[0].start = (LOW_TEST_ADR + 4095) >> 12;
		v->pmap[0].end = HIGH_TEST_ADR >> 12;
		v->test_pages = v->pmap[0].end - v->pmap[0].start;

	v->msegs = 1;
	cprint(LINE_INFO, COL_MMAP, "LinuxBIOS");
}

/*
 * Initialize test, setup screen and find out how much memory there is.
 */
void init(void)
{
	int i;

	/* Turn on cache */
	set_cache(1);

	/* Setup the display */
	display_init();

	mem_size();
	autotest=0;
    v->plim_lower=LOW_TEST_ADR>>12;
	v->plim_upper=HIGH_TEST_ADR>>12;
	v->msegs=1;
	v->test = 0;
	v->testsel = -1;
	v->rdtsc =1;
	v->msg_line = LINE_SCROLL-1;
	v->scroll_start = v->msg_line * 160;

	cprint(LINE_CPU+1, 0, "L1 ICache: Unknown ");
	cprint(LINE_CPU+2, 0, "L1 DCache: Unknown ");
	cprint(LINE_CPU+3, 0, "Memory  : ");
	aprint(LINE_CPU+3, 10, v->test_pages);
	cprint(LINE_CPU+4, 0, "Chipset : ");
	cprint(LINE_CPU+14, 0, SYSTYPE);



	/* Find the memory controller */
	cacheable();

	cprint(0, COL_MID,"Pass   %");
	cprint(1, COL_MID,"Test   %");
	cprint(2, COL_MID,"Test #");
	cprint(3, COL_MID,"Testing: ");
	cprint(4, COL_MID,"Pattern: ");
	cprint(LINE_INFO-2, 0, " WallTime   Cached  RsvdMem   MemMap   Cache  ECC  Test  Pass  Errors ECC Errs");
	cprint(LINE_INFO-1, 0, " ---------  ------  -------  --------  -----  ---  ----  ----  ------ --------");
	cprint(LINE_INFO, COL_TST, "Std");
	cprint(LINE_INFO, COL_PASS, "    0");
	cprint(LINE_INFO, COL_ERR, "     0");
	cprint(LINE_INFO+1, 0, " -----------------------------------------------------------------------------");

	for(i=0; i < 5; i++) {
		cprint(i, COL_MID-2, "| ");
	}
	footer();
	v->printmode=PRINTMODE_ADDRESSES;
	v->numpatn=0;
	find_ticks();
	cprint(LINE_SCROLL+1, 0, "press key 0-9 to run test 0-9,key a to run all test.");
}

#define FLAT 0

static unsigned long mapped_window = 1;
void paging_off(void)
{
	if (!v->pae)
		return;
	mapped_window = 1;
}

static void paging_on(void *pdp)
{
	if (!v->pae)
		return;

}

int map_page(unsigned long page)
{

	return 0;
}

void *mapping(unsigned long page_addr)
{
	void *result;
	if (FLAT || (page_addr < 0x80000)) {
		/* If the address is less that 1GB directly use the address */
		result = (void *)(page_addr << 12);
	}
	else {
		unsigned long alias;
		alias = page_addr & 0x7FFFF;
		alias += 0x80000;
		result = (void *)(alias << 12);
	}
	return result;
}

void *emapping(unsigned long page_addr)
{
	void *result;
	result = mapping(page_addr -1);
	/* The result needs to be 256 byte alinged... */
	result = ((unsigned char *)result) + 0xf00;
	return result;
}

unsigned long page_of(void *addr)
{
	unsigned long page;
	page = ((unsigned long)addr) >> 12;
	if (!FLAT && (page >= 0x80000)) {
		page &= 0x7FFFF;
		page += mapped_window << 19;
	}
#if 0
	cprint(LINE_SCROLL -2, 0, "page_of(        )->            ");
	hprint(LINE_SCROLL -2, 8, ((unsigned long)addr));
	hprint(LINE_SCROLL -2, 20, page);
#endif	
	return page;
}

/* Find cache-able memory size */
static void cacheable(void)
{
ulong cached;
cached=(HIGH_TEST_ADR-LOW_TEST_ADR)>>12;
aprint(LINE_INFO, COL_CACHE_TOP, cached);
 dprint(LINE_CPU+1, 12, CpuPrimaryInstCacheSize, 3, 0);
 dprint(LINE_CPU+2, 12, CpuPrimaryDataCacheSize, 3, 0);
}

