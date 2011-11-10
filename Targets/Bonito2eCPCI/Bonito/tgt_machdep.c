/*	$Id: tgt_machdep.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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
#if 1
#include <include/sbd.h>
#include <sys/param.h>
#include <sys/syslog.h>
#include <machine/endian.h>
#include <sys/device.h>
#include <machine/cpu.h>
#include <machine/pio.h>
#include <machine/intr.h>
#include <dev/pci/pcivar.h>
#endif
#include <sys/types.h>
#include <termio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include <dev/ic/mc146818reg.h>
#include <linux/io.h>

#include <autoconf.h>

#include <machine/cpu.h>
#include <machine/pio.h>
#include "pflash.h"
#include "dev/pflash_tgt.h"

#include "include/bonito.h"
#include <pmon/dev/gt64240reg.h>
#include <pmon/dev/ns16550.h>

#include <pmon.h>

#include "mod_x86emu_int10.h"
//#include "mod_x86emu.h"
#include "mod_vgacon.h"
extern void vga_bios_init(void);
extern int kbd_initialize(void);
extern int write_at_cursor(char val);
extern const char *kbd_error_msgs[];
#include "flash.h"
#if (NMOD_FLASH_AMD + NMOD_FLASH_INTEL) == 0
#ifdef HAVE_FLASH
#undef HAVE_FLASH
#endif
#else
#define HAVE_FLASH
#endif

extern struct trapframe DBGREG;

extern void *memset(void *, int, size_t);

int kbd_available;
int usb_kbd_available;
#if NMOD_X86EMU_INT10 == 0
int vga_available=0;
#else
#include "vgarom.c"
#endif


static int md_pipefreq = 0;
static int md_cpufreq = 0;
static int clk_invalid = 0;
static int nvram_invalid = 0;
static int cksum(void *p, size_t s, int set);
static void _probe_frequencies(void);

#ifndef NVRAM_IN_FLASH
void nvram_get(char *);
void nvram_put(char *);
#endif

extern int vgaterm(int op, struct DevEntry * dev, unsigned long param, int data);
void error(unsigned long *adr, unsigned long good, unsigned long bad);
void modtst(int offset, int iter, unsigned long p1, unsigned long p2);
void do_tick(void);
void print_hdr(void);
void ad_err2(unsigned long *adr, unsigned long bad);
void ad_err1(unsigned long *adr1, unsigned long *adr2, unsigned long good, unsigned long bad);
void mv_error(unsigned long *adr, unsigned long good, unsigned long bad);

void print_err( unsigned long *adr, unsigned long good, unsigned long bad, unsigned long xor);
static inline unsigned char CMOS_READ(unsigned char addr);
static inline void CMOS_WRITE(unsigned char val, unsigned char addr);
static void init_piix_rtc(void);

ConfigEntry	ConfigTable[] =
{
#ifdef HAVE_NB_SERIAL
	 { (char *)COM3_BASE_ADDR, 0, ns16550, 256, CONS_BAUD, NS16550HZ },
#else
	 { (char *)COM2_BASE_ADDR, 0, ns16550, 256, CONS_BAUD, NS16550HZ/2 },
#endif
#if NMOD_VGACON >0
	{ (char *)1, 0, vgaterm, 256, CONS_BAUD, NS16550HZ },
#endif
	{ 0 }
};

unsigned long _filebase;

extern int memorysize;
extern int memorysize_high;

extern char MipsException[], MipsExceptionEnd[];

unsigned char hwethadr[6];
unsigned int  gpiodata;
unsigned char activecom=2;

void initmips(unsigned int memsz);

void addr_tst1(void);
void addr_tst2(void);
void movinv1(int iter, ulong p1, ulong p2);

static int havepci=1;
static int havekbd=1;
int user_getenv(int);
void earlyenv()
{
#ifndef HAVE_NB_SERIAL
  int activecom;
  activecom=get_userenv(ACTIVECOM_OFFS) & 0x3;
  if(activecom&1)ConfigTable[0].devinfo=COM1_BASE_ADDR;
  else if(activecom&2) ConfigTable[0].devinfo=COM2_BASE_ADDR;
  else ConfigTable[0].devinfo=-1;
#endif
}


void
initmips(unsigned int memsz)
{
/*enable float*/
asm("
mfc0 $2,$12
li   $3,(1<<29)
or   $2,$3
mtc0 $2,$12
"
:::"$2","$3"
	);
#if 0
if(get_userenv(ACTIVECOM_OFFS)&0x80)
{
#define WATCHDOG_REG BONITO(0x0160)
int *p=(void *)0xa2000000;
	if(*p!=0x12345678)
	{
		*p=0x12345678;
	WATCHDOG_REG =0;
	WATCHDOG_REG=(3<<24)|0;
	while(1);
	}
*p=0;
}
#endif

earlyenv();
	/*
	 *	Set up memory address decoders to map entire memory.
	 *	But first move away bootrom map to high memory.
	 */
#if 1
if(tgt_testchar())
{
	if(tgt_getchar()=='p')havepci=0;
}
#endif
#if 0
	GT_WRITE(BOOTCS_LOW_DECODE_ADDRESS, BOOT_BASE >> 20);
	GT_WRITE(BOOTCS_HIGH_DECODE_ADDRESS, (BOOT_BASE - 1 + BOOT_SIZE) >> 20);
#endif
	SBD_DISPLAY("inms",0);
	memorysize = memsz > 256 ? 256 << 20 : memsz << 20;
	memorysize_high = memsz > 256 ? (memsz - 256) << 20 : 0;

asm("
	 sd %1,0x18(%0);
	 sd %2,0x28(%0);
	 sd %3,0x20(%0);
	 "
	 ::"r"(0x900000001ff00000ULL),"r"(memorysize),"r"(memorysize_high),"r"(0x20000000)
	 :"$2"
   );

	/*
	 *  Probe clock frequencys so delays will work properly.
	 */
	SBD_DISPLAY("0001",0);
	tgt_cpufreq();
	SBD_DISPLAY("DONE",0);
	/*
	 *  Init PMON and debug
	 */
	cpuinfotab[0] = &DBGREG;
	dbginit(NULL);

	/*
	 *  Set up exception vectors.
	 */
	SBD_DISPLAY("BEV1",0);
	bcopy(MipsException, (char *)TLB_MISS_EXC_VEC, MipsExceptionEnd - MipsException);
	bcopy(MipsException, (char *)GEN_EXC_VEC, MipsExceptionEnd - MipsException);

	CPU_FlushCache();

	CPU_SetSR(0, SR_BOOT_EXC_VEC);
	SBD_DISPLAY("BEV0",0);
	
	printf("BEV in SR set to zero.\n");

#if 0
	/* memtest */
	addr_tst1();
	addr_tst2();
	movinv1(2,0,~0);
	movinv1(2,0xaa5555aa,~0xaa5555aa);
	printf("memtest done\n");
#endif

	/*
	 * Launch!
	 */
	if(getenv("setup"))do_cmd("setup");
	main();
}

/*
 *  Put all machine dependent initialization here. This call
 *  is done after console has been initialized so it's safe
 *  to output configuration and debug information with printf.
 */

void
tgt_devconfig()
{
#if NMOD_VGACON > 0
	int rc;
#endif
#if 0
	/* Set the PHY addresses for the GT ethernet */
	GT_WRITE(ETH_PHY_ADDR_REG, 0x18a4);
	GT_WRITE(MAIN_ROUTING_REGISTER, 0x007ffe38);
	GT_WRITE(SERIAL_PORT_MULTIPLEX, 0x00000102);

	/* Route Multi Purpose Pins */
	GT_WRITE(MPP_CONTROL0, 0x77777777);
	GT_WRITE(MPP_CONTROL1, 0x00000000);
	GT_WRITE(MPP_CONTROL2, 0x00888888);
	GT_WRITE(MPP_CONTROL3, 0x00000066);
#endif
if(havepci)	_pci_devinit(1);	/* PCI device initialization */
#if NMOD_X86EMU_INT10 > 0
	SBD_DISPLAY("VGAI", 0);
if(havepci)	vga_bios_init();
#endif
        config_init();
if(havepci)     configure();
#if NMOD_VGACON >0
if(havekbd)	rc=kbd_initialize();
else rc=1;
	printf("%s\n",kbd_error_msgs[rc]);
	if(!rc){ 
		if(!getenv("nokbd"))kbd_available=1;
	}
#endif
	printf("devconfig done.\n");
}

extern int test_icache_1(short *addr);
extern int test_icache_2(int addr);
extern int test_icache_3(int addr);
extern void godson1_cache_flush(void);
#define tgt_putchar_uc(x) (*(void (*)(char)) (((long)tgt_putchar)|0x20000000)) (x)

void
tgt_devinit()
{
	/*
	 *  Gather info about and configure caches.
	 */
	if(getenv("ocache_off")) {
		CpuOnboardCacheOn = 0;
	}
	else {
		CpuOnboardCacheOn = 1;
	}
	if(getenv("ecache_off")) {
		CpuExternalCacheOn = 0;
	}
	else {
		CpuExternalCacheOn = 1;
	}
	printf("CPU_ConfigCache begin:\n");	
       CPU_ConfigCache();
	printf("CPU_ConfigCache done\n");
       //godson2_cache_flush();

       //tgt_putchar('&');
       
       //test_icache_2(0);
       //test_icache_3(0);
       //tgt_putchar(0x30+i);
       //tgt_putchar('Y');
      
       //tgt_test_memory();

       //tgt_putchar('Z');

if(havepci)_pci_businit(1);	/* PCI bus initialization */
}


void
tgt_reboot()
{
#if 1
    volatile unsigned int val;
    volatile unsigned int *ptr = (unsigned int *)0xbfe00104;
    val = *ptr;
    val &= ~(1<<2);
    *ptr = val;
    val |= (1<<2);
    *ptr = val;
#else
	void (*longreach) __P((void));
	
	longreach = (void *)0xbfc00000;
	(*longreach)();
	while(1);
#endif
}


/*
 *  This function makes inital HW setup for debugger and
 *  returns the apropriate setting for the status register.
 */
register_t
tgt_enable(int machtype)
{
	/* XXX Do any HW specific setup */
	return(SR_COP_1_BIT|SR_FR_32|SR_EXL);
}


/*
 *  Target dependent version printout.
 *  Printout available target version information.
 */
void
tgt_cmd_vers()
{
#if 0
	printf("Board revision level: %c.\n", in8(PLD_BAR) + 'A');
	printf("PLD revision levels: %d.%d and %d.%d.\n",
					in8(PLD_ID1) >> 4, in8(PLD_ID1) & 15,
#endif					in8(PLD_ID2) >> 4, in8(PLD_ID2) & 15);
}

/*
 *  Display any target specific logo.
 */
void
tgt_logo()
{
    printf("\n");
    printf("[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[\n");
    printf("[[[            [[[[   [[[[[[[[[[   [[[[            [[[[   [[[[[[[  [[\n");
    printf("[[   [[[[[[[[   [[[    [[[[[[[[    [[[   [[[[[[[[   [[[    [[[[[[  [[\n");
    printf("[[  [[[[[[[[[[  [[[  [  [[[[[[  [  [[[  [[[[[[[[[[  [[[  [  [[[[[  [[\n");
    printf("[[  [[[[[[[[[[  [[[  [[  [[[[  [[  [[[  [[[[[[[[[[  [[[  [[  [[[[  [[\n");
    printf("[[   [[[[[[[[   [[[  [[[  [[  [[[  [[[  [[[[[[[[[[  [[[  [[[  [[[  [[\n");
    printf("[[             [[[[  [[[[    [[[[  [[[  [[[[[[[[[[  [[[  [[[[  [[  [[\n");
    printf("[[  [[[[[[[[[[[[[[[  [[[[[  [[[[[  [[[  [[[[[[[[[[  [[[  [[[[[  [  [[\n");
    printf("[[  [[[[[[[[[[[[[[[  [[[[[[[[[[[[  [[[  [[[[[[[[[[  [[[  [[[[[[    [[\n");
    printf("[[  [[[[[[[[[[[[[[[  [[[[[[[[[[[[  [[[   [[[[[[[[   [[[  [[[[[[[   [[\n");
    printf("[[  [[[[[[[[[[[[[[[  [[[[[[[[[[[[  [[[[            [[[[  [[[[[[[[  [[\n");
    printf("[[[[[[[2005][[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[\n"); 
}

static void init_piix_rtc(void)
{
        int year, month, date, hour, min, sec;
        CMOS_WRITE(DS_CTLA_DV1, DS_REG_CTLA);
        CMOS_WRITE(DS_CTLB_24 | DS_CTLB_DM | DS_CTLB_SET, DS_REG_CTLB);
        CMOS_WRITE(0, DS_REG_CTLC);
        CMOS_WRITE(0, DS_REG_CTLD);
        year = CMOS_READ(DS_REG_YEAR);
        month = CMOS_READ(DS_REG_MONTH);
        date = CMOS_READ(DS_REG_DATE);
        hour = CMOS_READ(DS_REG_HOUR);
        min = CMOS_READ(DS_REG_MIN);
        sec = CMOS_READ(DS_REG_SEC);
        if( (year > 99) || (month < 1 || month > 12) ||
                (date < 1 || date > 31) || (hour > 23) || (min > 59) ||
                (sec > 59) ){
                /*
                printf("RTC time invalid, reset to epoch.\n");*/
                CMOS_WRITE(3, DS_REG_YEAR);
                CMOS_WRITE(1, DS_REG_MONTH);
                CMOS_WRITE(1, DS_REG_DATE);
                CMOS_WRITE(0, DS_REG_HOUR);
                CMOS_WRITE(0, DS_REG_MIN);
                CMOS_WRITE(0, DS_REG_SEC);
        }
        CMOS_WRITE(DS_CTLB_24 | DS_CTLB_DM, DS_REG_CTLB);
#ifdef	BEBUG_RTC
        for(;;){
                while(CMOS_READ(DS_REG_CTLA)&DS_CTLA_UIP);
                year = CMOS_READ(DS_REG_YEAR);
                month = CMOS_READ(DS_REG_MONTH);
                date = CMOS_READ(DS_REG_DATE);
                hour = CMOS_READ(DS_REG_HOUR);
                min = CMOS_READ(DS_REG_MIN);
                sec = CMOS_READ(DS_REG_SEC);
                                                                               
                printf("RTC: %02d-%02d-%02d %02d:%02d:%02d\n",
                        year, month, date, hour, min, sec);
        }
#endif
}

static inline unsigned char CMOS_READ(unsigned char addr)
{
        unsigned char val;
        linux_outb_p(addr, 0x70);
        val = linux_inb_p(0x71);
        return val;
}
                                                                               
static inline void CMOS_WRITE(unsigned char val, unsigned char addr)
{
        linux_outb_p(addr, 0x70);
        linux_outb_p(val, 0x71);
}

static void
_probe_frequencies()
{
#ifdef HAVE_TOD
        int i, timeout, cur, sec, cnt;
#endif
                                                                               
        SBD_DISPLAY ("FREQ", CHKPNT_FREQ);
                                                                               
                                                                               
#if 0
        md_pipefreq = 300000000;        /* Defaults */
        md_cpufreq  = 66000000;
#else
        md_pipefreq = 120000000;        /* NB FPGA*/
        md_cpufreq  =  40000000;
#endif

        clk_invalid = 1;
#ifdef HAVE_TOD
        init_piix_rtc();
        /*
         * Do the next twice for two reasons. First make sure we run from
         * cache. Second make sure synched on second update. (Pun intended!)
         */

	for(i = 2;  i != 0; i--) {
                cnt = CPU_GetCOUNT();
                timeout = 10000000;
                while(CMOS_READ(DS_REG_CTLA) & DS_CTLA_UIP);
                                                                               
                sec = CMOS_READ(DS_REG_SEC);
                                                                               
                do {
                        timeout--;
                        while(CMOS_READ(DS_REG_CTLA) & DS_CTLA_UIP);
                                                                               
                        cur = CMOS_READ(DS_REG_SEC);
                } while(timeout != 0 && cur == sec);
                                                                               
                cnt = CPU_GetCOUNT() - cnt;
                if(timeout == 0) {
                        break;          /* Get out if clock is not running */
                }
        }
        /*
         *  Get PLL programming info.
         */
        i =  2 - ((CPU_GetCONFIG() >> 16) & 1);         /* Halfclock bit */
        i = (((CPU_GetCONFIG() >> 28) & 7) + 2) * i;    /* Multiplier */
        /*
         *  Calculate the external bus clock frequency.
         */
        if (timeout != 0) {
                clk_invalid = 0;
                md_pipefreq = cnt / 10000;
                md_pipefreq *= 20000;
                md_cpufreq = (md_pipefreq * 2) / i;
        }
        else {
                md_pipefreq = (md_cpufreq * i) / 2;
        }
                                                                               
                                                                               
#endif /* HAVE_TOD */
}
                                                                               

/*
 *   Returns the CPU pipelie clock frequency
 */
int
tgt_pipefreq()
{
	if(md_pipefreq == 0) {
		_probe_frequencies();
	}
	return(md_pipefreq);
}

/*
 *   Returns the external clock frequency, usually the bus clock
 */
int
tgt_cpufreq()
{
	if(md_cpufreq == 0) {
		_probe_frequencies();
	}
	return(md_cpufreq);
}

time_t
tgt_gettime()
{
        struct tm tm;
        int ctrlbsave;
        time_t t;
#if 1 //fix me
	int clk_invalid=0;
#endif
#ifdef HAVE_TOD
                                                                               
        if(!clk_invalid) {
                ctrlbsave = CMOS_READ(DS_REG_CTLB);
                CMOS_WRITE(ctrlbsave | DS_CTLB_SET, DS_REG_CTLB);
                                                                               
                tm.tm_sec = CMOS_READ(DS_REG_SEC);
                tm.tm_min = CMOS_READ(DS_REG_MIN);
                tm.tm_hour = CMOS_READ(DS_REG_HOUR);
                tm.tm_wday = CMOS_READ(DS_REG_WDAY);
                tm.tm_mday = CMOS_READ(DS_REG_DATE);
                tm.tm_mon = CMOS_READ(DS_REG_MONTH) - 1;
                tm.tm_year = CMOS_READ(DS_REG_YEAR);
                if(tm.tm_year < 50)tm.tm_year += 100;
                                                                               
                CMOS_WRITE(ctrlbsave & ~DS_CTLB_SET, DS_REG_CTLB);
                                                                               
                tm.tm_isdst = tm.tm_gmtoff = 0;
                t = gmmktime(&tm);
        }
        else 
#endif
		{
                t = 957960000;  /* Wed May 10 14:00:00 2000 :-) */
        }
        return(t);
}
                                                                               
/*
 *  Set the current time if a TOD clock is present
 */
void
tgt_settime(time_t t)
{
#ifdef HAVE_TOD
        struct tm *tm;
        int ctrlbsave;

                                                                               
        if(!clk_invalid) {
                tm = gmtime(&t);
                ctrlbsave = CMOS_READ(DS_REG_CTLB);
                CMOS_WRITE(ctrlbsave | DS_CTLB_SET, DS_REG_CTLB);
                                                                               
                CMOS_WRITE(tm->tm_year % 100, DS_REG_YEAR);
                CMOS_WRITE(tm->tm_mon + 1, DS_REG_MONTH);
                CMOS_WRITE(tm->tm_mday, DS_REG_DATE);
                CMOS_WRITE(tm->tm_wday, DS_REG_WDAY);
                CMOS_WRITE(tm->tm_hour, DS_REG_HOUR);
                CMOS_WRITE(tm->tm_min, DS_REG_MIN);
                CMOS_WRITE(tm->tm_sec, DS_REG_SEC);
                                                                               
                CMOS_WRITE(ctrlbsave & ~DS_CTLB_SET, DS_REG_CTLB);
        }
#endif
}


/*
 *  Print out any target specific memory information
 */
void
tgt_memprint()
{
	printf("Primary Instruction cache size %dkb (%d line, %d way)\n",
		CpuPrimaryInstCacheSize / 1024, CpuPrimaryInstCacheLSize, CpuNWayCache);
	printf("Primary Data cache size %dkb (%d line, %d way)\n",
		CpuPrimaryDataCacheSize / 1024, CpuPrimaryDataCacheLSize, CpuNWayCache);
	if(CpuSecondaryCacheSize != 0) {
		printf("Secondary cache size %dkb\n", CpuSecondaryCacheSize / 1024);
	}
	if(CpuTertiaryCacheSize != 0) {
		printf("Tertiary cache size %dkb\n", CpuTertiaryCacheSize / 1024);
	}
}

void
tgt_machprint()
{
	printf("Copyright 2000-2002, Opsycon AB, Sweden.\n");
	printf("Copyright 2005, ICT CAS.\n");
	printf("CPU %s @", md_cpuname());
} 

/*
 *  Return a suitable address for the client stack.
 *  Usually top of RAM memory.
 */

register_t
tgt_clienttos()
{
	return((register_t)(int)PHYS_TO_UNCACHED(memorysize & ~7) - 64);
}

#ifdef HAVE_FLASH
/*
 *  Flash programming support code.
 */

/*
 *  Table of flash devices on target. See pflash_tgt.h.
 */

struct fl_map tgt_fl_map_boot16[]={
	TARGET_FLASH_DEVICES_16
};


struct fl_map *
tgt_flashmap()
{
	/*if((GT_READ(GT_BOOT_PAR) & 0x00300000) == 0) {
		return tgt_fl_map_boot8;
	}
	else {
		return tgt_fl_map_boot32;
	}*/
	return tgt_fl_map_boot16;
}   
void
tgt_flashwrite_disable()
{
}

int
tgt_flashwrite_enable()
{
	return(1);
}

void
tgt_flashinfo(void *p, size_t *t)
{
	struct fl_map *map;

	map = fl_find_map(p);
	if(map) {
		*t = map->fl_map_size;
	}
	else {
		*t = 0;
	}
}

void
tgt_flashprogram(void *p, int size, void *s, int endian)
{
	printf("Programming flash %x:%x into %x\n", s, size, p);
	if(fl_erase_device(p, size, TRUE)) {
		printf("Erase failed!\n");
		return;
	}
	if(fl_program_device(p, s, size, TRUE)) {
		printf("Programming failed!\n");
	}
	fl_verify_device(p, s, size, TRUE);
}
#endif /* PFLASH */

/*
 *  Network stuff.
 */
void
tgt_netinit()
{
}

int
tgt_ethaddr(char *p)
{
	bcopy((void *)&hwethadr, p, 6);
	return(0);
}

void
tgt_netreset()
{
}

/*************************************************************************/
/*
 *	Target dependent Non volatile memory support code
 *	=================================================
 *
 *
 *  On this target a part of the boot flash memory is used to store
 *  environment. See EV64260.h for mapping details. (offset and size).
 */

/*
 *  Read in environment from NV-ram and set.
 */
void
tgt_mapenv(int (*func) __P((char *, char *)))
{
        char *ep;
        char env[512];
        char *nvram;
	int i;

        /*
         *  Check integrity of the NVRAM env area. If not in order
         *  initialize it to empty.
         */
printf("in envinit\n");
#ifdef NVRAM_IN_FLASH
        nvram = (char *)(tgt_flashmap())->fl_map_base;
	printf("nvram=%08x\n",(unsigned int)nvram);
	if(fl_devident(nvram, NULL) == 0 ||
           cksum(nvram + NVRAM_OFFS, NVRAM_SIZE, 0) != 0) {
#else
        nvram = (char *)malloc(512);
	nvram_get(nvram);
	if(cksum(nvram, NVRAM_SIZE, 0) != 0) {
#endif
		printf("NVRAM is invalid!\n");
                nvram_invalid = 1;
        }
        else {
		nvram += NVRAM_OFFS;
                ep = nvram+2;;

                while(*ep != 0) {
                        char *val = 0, *p = env;
			i = 0;
                        while((*p++ = *ep++) && (ep <= nvram + NVRAM_SIZE - 1) && i++ < 255) {
                                if((*(p - 1) == '=') && (val == NULL)) {
                                        *(p - 1) = '\0';
                                        val = p;
                                }
                        }
                        if(ep <= nvram + NVRAM_SIZE - 1 && i < 255) {
                                (*func)(env, val);
                        }
                        else {
                                nvram_invalid = 2;
                                break;
                        }
                }
        }

	printf("NVRAM@%x\n",(u_int32_t)nvram);

	/*
	 *  Ethernet address for Galileo ethernet is stored in the last
	 *  six bytes of nvram storage. Set environment to it.
	 */
	
	gpiodata=*(volatile int *)(PHYS_TO_UNCACHED(PCI_IO_SPACE+GPO_REG));

	sprintf(env,"%08x",gpiodata);
	(*func)("gpiocfg",env);
    
    if(!nvram_invalid) bcopy(&nvram[ACTIVECOM_OFFS],&activecom, 1);
	else activecom=1;
	sprintf(env,"0x%02x",activecom);
	(*func)("activecom",env);
	
	bcopy(&nvram[ETHER_OFFS], hwethadr, 6);
	sprintf(env, "%02x:%02x:%02x:%02x:%02x:%02x", hwethadr[0], hwethadr[1],
	    hwethadr[2], hwethadr[3], hwethadr[4], hwethadr[5]);
	(*func)("ethaddr", env);

#ifndef NVRAM_IN_FLASH
	free(nvram);
#endif

#ifdef no_thank_you
        (*func)("vxWorks", env);
#endif


	sprintf(env, "%d", memorysize / (1024 * 1024));
	(*func)("memsize", env);

	sprintf(env, "%d", memorysize_high / (1024 * 1024));
	(*func)("highmemsize", env);

	sprintf(env, "%d", md_pipefreq);
	(*func)("cpuclock", env);

	sprintf(env, "%d", md_cpufreq);
	(*func)("busclock", env);

	(*func)("systype", SYSTYPE);
	
}

static int nvram_secsize=0;

int
tgt_unsetenv(char *name)
{
	char *ep, *np, *sp;
	char *nvram;
	char *nvrambuf;
	char *nvramsecbuf;
	int status;
	struct fl_device *dev;

	if(nvram_invalid) {
		return(0);
	}

	/* Use first defined flash device (we probably have only one) */
#ifdef NVRAM_IN_FLASH
	nvram = (char *)(tgt_flashmap())->fl_map_base;
	if(!nvram_secsize)
	{
	dev = fl_devident(nvram,0);
	nvram_secsize=dev?dev->fl_secsize:0x1000;
	}
	/* Map. Deal with an entire sector even if we only use part of it */
	nvram += NVRAM_OFFS & ~(nvram_secsize - 1);
	nvramsecbuf = (char *)malloc(nvram_secsize);
	if(nvramsecbuf == 0) {
		printf("Warning! Unable to malloc nvrambuffer!\n");
		return(-1);
	}
	memcpy(nvramsecbuf, nvram, nvram_secsize);
	nvrambuf = nvramsecbuf + (NVRAM_OFFS & (nvram_secsize - 1));
#else
	if(!nvram_secsize)nvram_secsize=NVRAM_SIZE;
	nvramsecbuf = nvrambuf = nvram = (char *)malloc(512);
	nvram_get(nvram);
#endif

	ep = nvrambuf + 2;

	status = 0;
	while((*ep != '\0') && (ep <= nvrambuf + NVRAM_SIZE)) {
		np = name;
		sp = ep;

		while((*ep == *np) && (*ep != '=') && (*np != '\0')) {
			ep++;
			np++;
		}
		if((*np == '\0') && ((*ep == '\0') || (*ep == '='))) {
			while(*ep++);
			while(ep <= nvrambuf + NVRAM_SIZE) {
				*sp++ = *ep++;
			}
			if(nvrambuf[2] == '\0') {
				nvrambuf[3] = '\0';
			}
			cksum(nvrambuf, NVRAM_SIZE, 1);
#ifdef NVRAM_IN_FLASH
			if(fl_erase_device(nvram, nvram_secsize, FALSE)) {
				status = -1;
				break;
			}

			if(fl_program_device(nvram, nvramsecbuf, nvram_secsize, FALSE)) {
				status = -1;
				break;
			}
#else
			nvram_put(nvram);
#endif
			status = 1;
			break;
		}
		else if(*ep != '\0') {
			while(*ep++ != '\0');
		}
	}

	free(nvramsecbuf);
	return(status);
}

int
tgt_setenv(char *name, char *value)
{
	char *ep;
	int envlen;
	char *nvrambuf;
	char *nvramsecbuf;
#ifdef NVRAM_IN_FLASH
	char *nvram;
	struct fl_device *dev;
#endif

	/* Non permanent vars. */
	if(strcmp(EXPERT, name) == 0) {
		return(1);
	}

	/* Calculate total env mem size requiered */
	envlen = strlen(name);
	if(envlen == 0) {
		return(0);
	}
	if(value != NULL) {
		envlen += strlen(value);
	}
	envlen += 2;    /* '=' + null byte */
	if(envlen > 255) {
		return(0);      /* Are you crazy!? */
	}

	/* Use first defined flash device (we probably have only one) */
#ifdef NVRAM_IN_FLASH
	nvram = (char *)(tgt_flashmap())->fl_map_base;
	if(!nvram_secsize)
	{
	dev = fl_devident(nvram,0);
	nvram_secsize=dev?dev->fl_secsize:0x1000;
	}

	/* Deal with an entire sector even if we only use part of it */
	nvram += NVRAM_OFFS & ~(nvram_secsize - 1);
#else
	if(!nvram_secsize)nvram_secsize=NVRAM_SIZE;
#endif

	/* If NVRAM is found to be uninitialized, reinit it. */
	if(nvram_invalid) {
		nvramsecbuf = (char *)malloc(nvram_secsize);
		if(nvramsecbuf == 0) {
			printf("Warning! Unable to malloc nvrambuffer!\n");
			return(-1);
		}
#ifdef NVRAM_IN_FLASH
		memcpy(nvramsecbuf, nvram, nvram_secsize);
#endif
		nvrambuf = nvramsecbuf + (NVRAM_OFFS & (nvram_secsize - 1));
		memset(nvrambuf, -1, NVRAM_SIZE);
		nvrambuf[2] = '\0';
		nvrambuf[3] = '\0';
		cksum((void *)nvrambuf, NVRAM_SIZE, 1);
		printf("Warning! NVRAM checksum fail. Reset!\n");
#ifdef NVRAM_IN_FLASH
		if(fl_erase_device(nvram, nvram_secsize, FALSE)) {
			printf("Error! Nvram erase failed!\n");
			free(nvramsecbuf);
			return(-1);
		}
		if(fl_program_device(nvram, nvramsecbuf, nvram_secsize, FALSE)) {
			printf("Error! Nvram init failed!\n");
			free(nvramsecbuf);
			return(-1);
		}
#else
		nvram_put(nvramsecbuf);
#endif
		nvram_invalid = 0;
		free(nvramsecbuf);
	}

	/* Remove any current setting */
	tgt_unsetenv(name);

	/* Find end of evironment strings */
	nvramsecbuf = (char *)malloc(nvram_secsize);
	if(nvramsecbuf == 0) {
		printf("Warning! Unable to malloc nvrambuffer!\n");
		return(-1);
	}
#ifndef NVRAM_IN_FLASH
	nvram_get(nvramsecbuf);
#else
	memcpy(nvramsecbuf, nvram, nvram_secsize);
#endif
	nvrambuf = nvramsecbuf + (NVRAM_OFFS & (nvram_secsize - 1));
	/* Etheraddr is special case to save space */
	if (strcmp("activecom",name)==0)
    {
        activecom=strtoul(value,0,0);
        printf("set activecom to com %d\n",activecom);
    }
	else if (strcmp("gpiocfg",name)==0)
	{
		gpiodata=strtoul(value,0,0);
		printf("set gpiocfg to 0x%08x\n",gpiodata);
	}
	else if (strcmp("ethaddr", name) == 0) {
		char *s = value;
		int i;
		int32_t v;
		for(i = 0; i < 6; i++) {
			gethex(&v, s, 2);
			hwethadr[i] = v;
			s += 3;         /* Don't get to fancy here :-) */
		} 
	} else {
		ep = nvrambuf+2;
		if(*ep != '\0') {
			do {
				while(*ep++ != '\0');
			} while(*ep++ != '\0');
			ep--;
		}
		if(((int)ep + NVRAM_SIZE - (int)ep) < (envlen + 1)) {
			free(nvramsecbuf);
			return(0);      /* Bummer! */
		}

		/*
		 *  Special case heaptop must always be first since it
		 *  can change how memory allocation works.
		 */
		if(strcmp("heaptop", name) == 0) {

			bcopy(nvrambuf+2, nvrambuf+2 + envlen,
					ep - nvrambuf+1);

			ep = nvrambuf+2;
			while(*name != '\0') {
				*ep++ = *name++;
			}
			if(value != NULL) {
				*ep++ = '=';
				while((*ep++ = *value++) != '\0');
			}
			else {
				*ep++ = '\0';
			}
		}
		else {
			while(*name != '\0') {
				*ep++ = *name++;
			}
			if(value != NULL) {
				*ep++ = '=';
				while((*ep++ = *value++) != '\0');
			}
			else {
				*ep++ = '\0';
			}
			*ep++ = '\0';   /* End of env strings */
		}
	}
	cksum(nvrambuf, NVRAM_SIZE, 1);

	bcopy(hwethadr, &nvrambuf[ETHER_OFFS], 6);
	bcopy(&gpiodata, &nvrambuf[GPIOCFG_OFFS], 4);
	bcopy(&activecom,&nvrambuf[ACTIVECOM_OFFS], 1);
#ifdef NVRAM_IN_FLASH
	if(fl_erase_device(nvram, nvram_secsize, FALSE)) {
		printf("Error! Nvram erase failed!\n");
		free(nvramsecbuf);
		return(0);
	}
	if(fl_program_device(nvram, nvramsecbuf, nvram_secsize, FALSE)) {
		printf("Error! Nvram program failed!\n");
		free(nvramsecbuf);
		return(0);
	}
#else
	nvram_put(nvramsecbuf);
#endif
	free(nvramsecbuf);
	return(1);
}


/*
 *  Calculate checksum. If 'set' checksum is calculated and set.
 */
static int
cksum(void *p, size_t s, int set)
{
	u_int16_t sum = 0;
	u_int8_t *sp = p;
	int sz = s / 2;

	if(set) {
		*sp = 0;	/* Clear checksum */
		*(sp+1) = 0;	/* Clear checksum */
	}
	while(sz--) {
		sum += (*sp++) << 8;
		sum += *sp++;
	}
	if(set) {
		sum = -sum;
		*(u_int8_t *)p = sum >> 8;
		*((u_int8_t *)p+1) = sum;
	}
	return(sum);
}

#ifndef NVRAM_IN_FLASH

/*
 *  Read and write data into non volatile memory in clock chip.
 */
void
nvram_get(char *buffer)
{
	int i;
	for(i = 0; i < 114; i++) {
		linux_outb(i + RTC_NVRAM_BASE, RTC_INDEX_REG);	/* Address */
		buffer[i] = linux_inb(RTC_DATA_REG);
	}
}

void
nvram_put(char *buffer)
{
	int i;
	for(i = 0; i < 114; i++) {
		linux_outb(i+RTC_NVRAM_BASE, RTC_INDEX_REG);	/* Address */
		linux_outb(buffer[i],RTC_DATA_REG);
	}
}

#endif

/*
 *  Simple display function to display a 4 char string or code.
 *  Called during startup to display progress on any feasible
 *  display before any serial port have been initialized.
 */
void
tgt_display(char *msg, int x)
{
	/* Have simple serial port driver */
	tgt_putchar(msg[0]);
	tgt_putchar(msg[1]);
	tgt_putchar(msg[2]);
	tgt_putchar(msg[3]);
	tgt_putchar('\r');
	tgt_putchar('\n');
}
#include <include/stdarg.h>
int
tgt_printf (const char *fmt, ...)
{
    int  n;
    char buf[1024];
	char *p=buf;
	char c;
	va_list     ap;
	va_start(ap, fmt);
    n = vsprintf (buf, fmt, ap);
    va_end(ap);
	while((c=*p++))
	{ 
	 if(c=='\n')tgt_putchar('\r');
	 tgt_putchar(c);
	}
    return (n);
}


void
clrhndlrs()
{
}

int
tgt_getmachtype()
{
	return(md_cputype());
}

/*
 *  Create stubs if network is not compiled in
 */
#ifdef INET
void
tgt_netpoll()
{
	splx(splhigh());
}

#else
extern void longjmp(label_t *, int);
void gsignal(label_t *jb, int sig);
void
gsignal(label_t *jb, int sig)
{
	if(jb != NULL) {
		longjmp(jb, 1);
	}
};

int	netopen (const char *, int);
int	netread (int, void *, int);
int	netwrite (int, const void *, int);
long	netlseek (int, long, int);
int	netioctl (int, int, void *);
int	netclose (int);
int netopen(const char *p, int i)	{ return -1;}
int netread(int i, void *p, int j)	{ return -1;}
int netwrite(int i, const void *p, int j)	{ return -1;}
int netclose(int i)	{ return -1;}
long int netlseek(int i, long j, int k)	{ return -1;}
int netioctl(int j, int i, void *p)	{ return -1;}
void tgt_netpoll()	{};

#endif /*INET*/
#include "mycmd.c"
