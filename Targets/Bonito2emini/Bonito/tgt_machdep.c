/*	$Id: tgt_machdep.c,v 1.4 2004/05/17 10:39:22 wlin Exp $ */

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
int vga_available;

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
	 { (char *)COM3_BASE_ADDR, 0, ns16550, 256, CONS_BAUD, NS16550HZ },
/*	 { (char *)COM1_BASE_ADDR, 0, ns16550, 256, CONS_BAUD, NS16550HZ },
	 { (char *)COM2_BASE_ADDR, 0, ns16550, 256, CONS_BAUD, NS16550HZ }, */
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

void initmips(unsigned int memsz);

void addr_tst1(void);
void addr_tst2(void);
void movinv1(int iter, ulong p1, ulong p2);

static int havepci=1;
static int havekbd=0;

void
initmips(unsigned int memsz)
{
	/*
	 *	Set up memory address decoders to map entire memory.
	 *	But first move away bootrom map to high memory.
	 */
BONITO(0x100)=0xe58;
if(tgt_testchar())
{
	if(tgt_getchar()=='p')havepci=0;
}

#if 0
	GT_WRITE(BOOTCS_LOW_DECODE_ADDRESS, BOOT_BASE >> 20);
	GT_WRITE(BOOTCS_HIGH_DECODE_ADDRESS, (BOOT_BASE - 1 + BOOT_SIZE) >> 20);
#endif
	memorysize=(memsz&0x0000ffff) << 20;//recover to original size:256M
	memorysize_high=((memsz&0xffff0000)>>16) << 20;//0

	/*
	 *  Probe clock frequencys so delays will work properly.
	 */
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
	
       CPU_ConfigCache();

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
        md_pipefreq = 528000000;        /* NB FPGA*/
        md_cpufreq  =  66000000;
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

	/*gx 2005-01-17 */
	return 0;
                                                                               
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
        else {
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
        struct tm *tm;
        int ctrlbsave;

	return ;
                                                                               
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
	if (strcmp("ethaddr", name) == 0) {
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

	bcopy(hwethadr, &nvramsecbuf[ETHER_OFFS], 6);
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

#define SPINSZ		0x800000
#define DEFTESTS	7
#define MOD_SZ		20
#define BAILOUT		if (bail) goto skip_test;
#define BAILR		if (bail) return;

/* memspeed operations */
#define MS_BCOPY	1
#define MS_COPY		2
#define MS_WRITE	3
#define MS_READ		4

struct map_struct {
	unsigned long *start;
	unsigned long *end;
};

struct map_struct map[] = { 
   {(unsigned long*)0x80100000,(unsigned long*)(0x84000000 - 0x100000)},
   {(unsigned long*)0x0,(unsigned long*)0x0}
};

int segs=1;

int bail;
volatile unsigned long *p;
unsigned long p1, p2;
int test_ticks, nticks;
unsigned long bad;
int ecount = 0, errors = 0;

void poll_errors(void);
static void update_err_counts(void);

static unsigned long my_roundup(unsigned long value, unsigned long mask)
{
	return (value + mask) & ~mask;
}

/*
 * Print an individual error
 */
void print_err( unsigned long *adr, unsigned long good, unsigned long bad, unsigned long xor) 
{
	printf("adr=%x,good=%x,bad=%x,xor=%x\n",adr,good,bad,xor);
}

/*
 * Display data error message. Don't display duplicate errors.
 */
void error(unsigned long *adr, unsigned long good, unsigned long bad)
{
	unsigned long xor;

	xor = good ^ bad;

	update_err_counts();
	print_err(adr, good, bad, xor);
}

/*
 * Display data error message from the block move test.  The actual failing
 * address is unknown so don't use this failure information to create
 * BadRAM patterns.
 */
void mv_error(unsigned long *adr, unsigned long good, unsigned long bad)
{
	unsigned long xor;

	update_err_counts();
	xor = good ^ bad;
	print_err(adr, good, bad, xor);
}

/*
 * Display address error message.
 * Since this is strictly an address test, trying to create BadRAM
 * patterns does not make sense.  Just report the error.
 */
void ad_err1(unsigned long *adr1, unsigned long *adr2, unsigned long good, unsigned long bad)
{
	unsigned long xor;
	update_err_counts();
	xor = ((unsigned long)adr1) ^ ((unsigned long)adr2);
	print_err(adr1, good, bad, xor);
}

/*
 * Display address error message.
 * Since this type of address error can also report data errors go
 * ahead and generate BadRAM patterns.
 */
void ad_err2(unsigned long *adr, unsigned long bad)
{
#if 0
	int patnchg;

	/* Process the address in the pattern administration */
	patnchg=insertaddress ((unsigned long) adr);
#endif

	update_err_counts();
	print_err(adr, (unsigned long)adr, bad, ((unsigned long)adr) ^ bad);
}

void print_hdr(void)
{
	printf("Tst  Pass   Failing Address          Good       Bad     Err-Bits  Count Chan\n");
	printf("---  ----  -----------------------  --------  --------  --------  ----- ----\n");
}

static void update_err_counts(void)
{
	++(ecount);
	errors++;
}


/*
 * Show progress by displaying elapsed time and update bar graphs
 */
void do_tick(void)
{
	printf(".");
}
/*
 * Memory address test, walking ones
 */
void addr_tst1(void)
{
	int i, j, k;
	volatile unsigned long *pt;
	volatile unsigned long *end;
	unsigned long bad, mask, bank;

	printf("address test 1..\n");

	/* Test the global address bits */
	for (p1=0, j=0; j<2; j++) {
        	//hprint(LINE_PAT, COL_PAT, p1);
		printf("\npat=%x\n",p1);

		/* Set pattern in our lowest multiple of 0x20000 */
		p = (unsigned long *)my_roundup((unsigned long)map[0].start, 0x1ffff);
		*p = p1;
	
		/* Now write pattern compliment */
		p1 = ~p1;
		end = map[segs-1].end;
		for (i=0; i<1000; i++) {
			mask = 4;
			do {
				pt = (unsigned long *)((unsigned long)p | mask);
				if (pt == p) {
					mask = mask << 1;
					continue;
				}
				if (pt >= end) {
					break;
				}
				*pt = p1;
				if ((bad = *p) != ~p1) {
					ad_err1((unsigned long *)p, (unsigned long *)mask,
						bad, ~p1);
					i = 1000;
				}
				mask = mask << 1;
			} while(mask);
		do_tick();
		}
		BAILR
	}

	/* Now check the address bits in each bank */
	/* If we have more than 8mb of memory then the bank size must be */
	/* bigger than 256k.  If so use 1mb for the bank size. */
	if (map[segs-1].end - map[0].start > (0x800000 >> 12)) {
		bank = 0x100000;
	} else {
		bank = 0x40000;
	}
	for (p1=0, k=0; k<2; k++) {
        	//hprint(LINE_PAT, COL_PAT, p1);
		printf("\npat=%x\n",p1);

		for (j=0; j<segs; j++) {
			p = map[j].start;
			/* Force start address to be a multiple of 256k */
			p = (unsigned long *)my_roundup((unsigned long)p, bank - 1);
			end = map[j].end;
			while (p < end) {
				*p = p1;

				p1 = ~p1;
				for (i=0; i<200; i++) {
					mask = 4;
					do {
						pt = (unsigned long *)
						    ((unsigned long)p | mask);
						if (pt == p) {
							mask = mask << 1;
							continue;
						}
						if (pt >= end) {
							break;
						}
						*pt = p1;
						if ((bad = *p) != ~p1) {
							ad_err1((unsigned long *)p,
							    (unsigned long *)mask,
							    bad,~p1);
							i = 200;
						}
						mask = mask << 1;
					} while(mask);
				}
				if (p + bank > p) {
					p += bank;
				} else {
					p = end;
				}
				p1 = ~p1;
			}
			do_tick();
		}
		BAILR
		p1 = ~p1;
	}
}

/*
 * Memory address test, own address
 */
void addr_tst2(void)
{
	int j, done;
	volatile unsigned long *pe;
	volatile unsigned long *end, *start;

        //cprint(LINE_PAT, COL_PAT, "        ");
	printf("addr test 2...\n");

	/* Write each address with it's own address */
	for (j=0; j<segs; j++) {
		start = map[j].start;
		end = map[j].end;
		pe = (unsigned long *)start;
		p = start;
		done = 0;
		do {
			/* Check for overflow */
			if (pe + SPINSZ > pe) {
				pe += SPINSZ;
			} else {
				pe = end;
			}
			if (pe >= end) {
				pe = end;
				done++;
			}
			if (p == pe ) {
				break;
			}

 			for (; p < pe; p++) {
				*p = (unsigned long)p;
			}
			do_tick();
			BAILR
		} while (!done);
	}

	/* Each address should have its own address */
	for (j=0; j<segs; j++) {
		start = map[j].start;
		end = map[j].end;
		pe = (unsigned long *)start;
		p = start;
		done = 0;
		do {
			/* Check for overflow */
			if (pe + SPINSZ > pe) {
				pe += SPINSZ;
			} else {
				pe = end;
			}
			if (pe >= end) {
				pe = end;
				done++;
			}
			if (p == pe ) {
				break;
			}
 			for (; p < pe; p++) {
 				if((bad = *p) != (unsigned long)p) {
 					ad_err2((unsigned long*)p, bad);
 				}
 			}
			do_tick();
			BAILR
		} while (!done);
	}
}

/*
 * Test all of memory using a "moving inversions" algorithm using the
 * pattern in p1 and it's complement in p2.
 */
void movinv1(int iter, unsigned long p1, unsigned long p2)
{
	int i, j, done;
	volatile unsigned long *pe;
	volatile unsigned long len;
	volatile unsigned long *start,*end;

	/* Display the current pattern */
        //hprint(LINE_PAT, COL_PAT, p1);
	printf("movinv test with p1=%x,p2=%x...\n",p1,p2);

	/* Initialize memory with the initial pattern.  */
	for (j=0; j<segs; j++) {
		start = map[j].start;
		end = map[j].end;
		pe = start;
		p = start;
		done = 0;
		do {
			/* Check for overflow */
			if (pe + SPINSZ > pe) {
				pe += SPINSZ;
			} else {
				pe = end;
			}
			if (pe >= end) {
				pe = end;
				done++;
			}
			len = pe - p;
			if (p == pe ) {
				break;
			}

			for (; p < pe; p++) {
				*p = p1;
			}
			do_tick();
			BAILR
		} while (!done);
	}

	/* Do moving inversions test. Check for initial pattern and then
	 * write the complement for each memory location. Test from bottom
	 * up and then from the top down.  */
	for (i=0; i<iter; i++) {
		for (j=0; j<segs; j++) {
			start = map[j].start;
			end = map[j].end;
			pe = start;
			p = start;
			done = 0;
			do {
				/* Check for overflow */
				if (pe + SPINSZ > pe) {
					pe += SPINSZ;
				} else {
					pe = end;
				}
				if (pe >= end) {
					pe = end;
					done++;
				}
				if (p == pe ) {
					break;
				}
				for (; p < pe; p++) {
					if ((bad=*p) != p1) {
						error((unsigned long*)p, p1, bad);
					}
					*p = p2;
				}
				do_tick();
				BAILR
			} while (!done);
		}
		for (j=segs-1; j>=0; j--) {
			start = map[j].start;
			end = map[j].end;
			pe = end -1;
			p = end -1;
			done = 0;
			do {
				/* Check for underflow */
				if (pe - SPINSZ < pe) {
					pe -= SPINSZ;
				} else {
					pe = start;
				}
				if (pe <= start) {
					pe = start;
					done++;
				}
				if (p == pe ) {
					break;
				}
 				do {
					if ((bad=*p) != p2) {
						error((unsigned long*)p, p2, bad);
				}
					*p = p1;
				} while (p-- > pe);
				do_tick();
				BAILR
			} while (!done);
		}
	}
}

#if 0
void movinv32(int iter, unsigned long p1, unsigned long lb, unsigned long hb, int sval, int off)
{
	int i, j, k=0, n = 0, done;
	volatile unsigned long *pe;
	volatile unsigned long *start, *end;
	unsigned long pat, p3;

	p3 = sval << 31;

	/* Display the current pattern */
        hprint(LINE_PAT, COL_PAT, p1);

	/* Initialize memory with the initial pattern.  */
	for (j=0; j<segs; j++) {
		start = v->map[j].start;
		end = v->map[j].end;
		pe = start;
		p = start;
		done = 0;
		k = off;
		pat = p1;
		do {
			/* Check for overflow */
			if (pe + SPINSZ > pe) {
				pe += SPINSZ;
			} else {
				pe = end;
			}
			if (pe >= end) {
				pe = end;
				done++;
			}
			if (p == pe ) {
				break;
			}
			/* Do a SPINSZ section of memory */
/* Original C code replaced with hand tuned assembly code
 *			while (p < pe) {
 *				*p = pat;
 *				if (++k >= 32) {
 *					pat = lb;
 *					k = 0;
 *				} else {
 *					pat = pat << 1;
 *					pat |= sval;
 *				}
 *				p++;
 *			}
 */
			asm __volatile__ (
				"jmp L20\n\t"
				".p2align 4,,7\n\t"

				"L20:\n\t"
				"movl %%ecx,(%%edi)\n\t"
				"addl $1,%%ebx\n\t"
				"cmpl $32,%%ebx\n\t"
				"jne L21\n\t"
				"movl %%esi,%%ecx\n\t"
				"xorl %%ebx,%%ebx\n\t"
				"jmp L22\n"
				"L21:\n\t"
				"shll $1,%%ecx\n\t"
				"orl %%eax,%%ecx\n\t"
				"L22:\n\t"
				"addl $4,%%edi\n\t"
				"cmpl %%edx,%%edi\n\t"
				"jb L20\n\t"
				: "=b" (k), "=D" (p)
				: "D" (p),"d" (pe),"b" (k),"c" (pat),
					"a" (sval), "S" (lb)
			);
			do_tick();
			BAILR
		} while (!done);
	}

	/* Do moving inversions test. Check for initial pattern and then
	 * write the complement for each memory location. Test from bottom
	 * up and then from the top down.  */
	for (i=0; i<iter; i++) {
		for (j=0; j<segs; j++) {
			start = v->map[j].start;
			end = v->map[j].end;
			pe = start;
			p = start;
			done = 0;
			k = off;
			pat = p1;
			do {
				/* Check for overflow */
				if (pe + SPINSZ > pe) {
					pe += SPINSZ;
				} else {
					pe = end;
				}
				if (pe >= end) {
					pe = end;
					done++;
				}
				if (p == pe ) {
					break;
				}
/* Original C code replaced with hand tuned assembly code
 *				while (p < pe) {
 *					if ((bad=*p) != pat) {
 *						error((unsigned long*)p, pat, bad);
 *					}
 *					*p = ~pat;
 *					if (++k >= 32) {
 *						pat = lb;
 *						k = 0;
 *					} else {
 *						pat = pat << 1;
 *						pat |= sval;
 *					}
 *					p++;
 *				}
 */
				asm __volatile__ (
					"pushl %%ebp\n\t"
					"jmp L30\n\t"

					".p2align 4,,7\n\t"
					"L30:\n\t"
					"movl (%%edi),%%ebp\n\t"
					"cmpl %%ecx,%%ebp\n\t"
					"jne L34\n\t"

					"L35:\n\t"
					"notl %%ecx\n\t"
					"movl %%ecx,(%%edi)\n\t"
					"notl %%ecx\n\t"
					"incl %%ebx\n\t"
					"cmpl $32,%%ebx\n\t"
					"jne L31\n\t"
					"movl %%esi,%%ecx\n\t"
					"xorl %%ebx,%%ebx\n\t"
					"jmp L32\n"
					"L31:\n\t"
					"shll $1,%%ecx\n\t"
					"orl %%eax,%%ecx\n\t"
					"L32:\n\t"
					"addl $4,%%edi\n\t"
					"cmpl %%edx,%%edi\n\t"
					"jb L30\n\t"
					"jmp L33\n\t"

					"L34:\n\t" \
					"pushl %%esi\n\t"
					"pushl %%eax\n\t"
					"pushl %%ebx\n\t"
					"pushl %%edx\n\t"
					"pushl %%ebp\n\t"
					"pushl %%ecx\n\t"
					"pushl %%edi\n\t"
					"call error\n\t"
					"popl %%edi\n\t"
					"popl %%ecx\n\t"
					"popl %%ebp\n\t"
					"popl %%edx\n\t"
					"popl %%ebx\n\t"
					"popl %%eax\n\t"
					"popl %%esi\n\t"
					"jmp L35\n"

					"L33:\n\t"
					"popl %%ebp\n\t"
					: "=b" (k), "=D" (p)
					: "D" (p),"d" (pe),"b" (k),"c" (pat),
						"a" (sval), "S" (lb)
				);
				do_tick();
				BAILR
			} while (!done);
		}

		/* Since we already adjusted k and the pattern this
		 * code backs both up one step
		 */
		if (--k < 0) {
			k = 31;
		}
		for (pat = lb, n = 0; n < k; n++) {
			pat = pat << 1;
			pat |= sval;
		}
		k++;
		for (j=segs-1; j>=0; j--) {
			start = v->map[j].start;
			end = v->map[j].end;
			p = end -1;
			pe = end -1;
			done = 0;
			do {
				/* Check for underflow */
				if (pe - SPINSZ < pe) {
					pe -= SPINSZ;
				} else {
					pe = start;
				}
				if (pe <= start) {
					pe = start;
					done++;
				}
				if (p == pe ) {
					break;
				}
/* Original C code replaced with hand tuned assembly code
 *				do {
 *					if ((bad=*p) != ~pat) {
 *						error((unsigned long*)p, ~pat, bad);
 *					}
 *					*p = pat;
 *					if (--k <= 0) {
 *						pat = hb;
 *						k = 32;
 *					} else {
 *						pat = pat >> 1;
 *						pat |= p3;
 *					}
 *				} while (p-- > pe);
 */
				asm __volatile__ (
					"pushl %%ebp\n\t"
					"addl $4,%%edi\n\t"
					"jmp L40\n\t"
	
					".p2align 4,,7\n\t"
					"L40:\n\t"
					"subl $4,%%edi\n\t"
					"movl (%%edi),%%ebp\n\t"
					"notl %%ecx\n\t"
					"cmpl %%ecx,%%ebp\n\t"
					"jne L44\n\t"

					"L45:\n\t"
					"notl %%ecx\n\t"
					"movl %%ecx,(%%edi)\n\t"
					"decl %%ebx\n\t"
					"cmpl $0,%%ebx\n\t"
					"jg L41\n\t"
					"movl %%esi,%%ecx\n\t"
					"movl $32,%%ebx\n\t"
					"jmp L42\n"
					"L41:\n\t"
					"shrl $1,%%ecx\n\t"
					"orl %%eax,%%ecx\n\t"
					"L42:\n\t"
					"cmpl %%edx,%%edi\n\t"
					"ja L40\n\t"
					"jmp L43\n\t"

					"L44:\n\t" \
					"pushl %%esi\n\t"
					"pushl %%eax\n\t"
					"pushl %%ebx\n\t"
					"pushl %%edx\n\t"
					"pushl %%ebp\n\t"
					"pushl %%ecx\n\t"
					"pushl %%edi\n\t"
					"call error\n\t"
					"popl %%edi\n\t"
					"popl %%ecx\n\t"
					"popl %%ebp\n\t"
					"popl %%edx\n\t"
					"popl %%ebx\n\t"
					"popl %%eax\n\t"
					"popl %%esi\n\t"
					"jmp L45\n"

					"L43:\n\t"
					"subl $4,%%edi\n\t"
					"popl %%ebp\n\t"
					: "=b" (k), "=D" (p), "=c" (pat)
					: "D" (p),"d" (pe),"b" (k),"c" (pat),
						"a" (p3), "S" (hb)
				);
				do_tick();
				BAILR
			} while (!done);
		}
	}
}
#endif

/*
 * Test all of memory using modulo X access pattern.
 */
void modtst(int offset, int iter, unsigned long p1, unsigned long p2)
{
	int j, k, l, done;
	volatile unsigned long *pe;
	volatile unsigned long *start, *end;

	/* Display the current pattern */
	/*
        hprint(LINE_PAT, COL_PAT-2, p1);
	cprint(LINE_PAT, COL_PAT+6, "-");
        dprint(LINE_PAT, COL_PAT+7, offset, 2, 1);
	*/
	printf("modtst offset=%x,p1=%x,p2=%x\n",offset,p1,p2);

	/* Write every nth location with pattern */
	for (j=0; j<segs; j++) {
		start = map[j].start;
		end = map[j].end;
		pe = (unsigned long *)start;
		p = start+offset;
		done = 0;
		do {
			/* Check for overflow */
			if (pe + SPINSZ > pe) {
				pe += SPINSZ;
			} else {
				pe = end;
			}
			if (pe >= end) {
				pe = end;
				done++;
			}
			if (p == pe ) {
				break;
			}
 			for (; p < pe; p += MOD_SZ) {
 				*p = p1;
 			}
			do_tick();
			BAILR
		} while (!done);
	}

	/* Write the rest of memory "iter" times with the pattern complement */
	for (l=0; l<iter; l++) {
		for (j=0; j<segs; j++) {
			start = map[j].start;
			end = map[j].end;
			pe = (unsigned long *)start;
			p = start;
			done = 0;
			k = 0;
			do {
				/* Check for overflow */
				if (pe + SPINSZ > pe) {
					pe += SPINSZ;
				} else {
					pe = end;
				}
				if (pe >= end) {
					pe = end;
					done++;
				}
				if (p == pe ) {
					break;
				}
 				for (; p < pe; p++) {
 					if (k != offset) {
 						*p = p2;
 					}
 					if (++k > MOD_SZ-1) {
 						k = 0;
 					}
 				}
				do_tick();
				BAILR
			} while (!done);
		}
	}

	/* Now check every nth location */
	for (j=0; j<segs; j++) {
		start = map[j].start;
		end = map[j].end;
		pe = (unsigned long *)start;
		p = start+offset;
		done = 0;
		do {
			/* Check for overflow */
			if (pe + SPINSZ > pe) {
				pe += SPINSZ;
			} else {
				pe = end;
			}
			if (pe >= end) {
				pe = end;
				done++;
			}
			if (p == pe ) {
				break;
			}
 			for (; p < pe; p += MOD_SZ) {
 				if ((bad=*p) != p1) {
 					error((unsigned long*)p, p1, bad);
 				}
			}
			do_tick();
			BAILR
		} while (!done);
	}
}

#if 0
/*
 * Test memory using block moves 
 * Adapted from Robert Redelmeier's burnBX test
 */
void block_move(int iter)
{
	int i, j, done;
	unsigned long len;
	volatile unsigned long p, pe, pp;
	volatile unsigned long start, end;

        //cprint(LINE_PAT, COL_PAT-2, "          ");
	printf("block move test...\n");

	/* Initialize memory with the initial pattern.  */
	for (j=0; j<segs; j++) {
		start = (unsigned long)map[j].start;
		end = (unsigned long)map[j].end;
		pe = start;
		p = start;
		done = 0;
		do {
			/* Check for overflow */
			if (pe + SPINSZ*4 > pe) {
				pe += SPINSZ*4;
			} else {
				pe = end;
			}
			if (pe >= end) {
				pe = end;
				done++;
			}
			if (p == pe ) {
				break;
			}
			len  = ((unsigned long)pe - (unsigned long)p) / 64;
			asm __volatile__ (
				".set noreorder\n\t"
                                "b   1f\n\n"
				"nop\n\t"
				".algin 4\n\t"
				"1f:\n\t"
				"move" 

				"jmp L100\n\t"

				".p2align 4,,7\n\t"
				"L100:\n\t"
				"movl %%eax, %%edx\n\t"
				"notl %%edx\n\t"
				"movl %%eax,0(%%edi)\n\t"
				"movl %%eax,4(%%edi)\n\t"
				"movl %%eax,8(%%edi)\n\t"
				"movl %%eax,12(%%edi)\n\t"
				"movl %%edx,16(%%edi)\n\t"
				"movl %%edx,20(%%edi)\n\t"
				"movl %%eax,24(%%edi)\n\t"
				"movl %%eax,28(%%edi)\n\t"
				"movl %%eax,32(%%edi)\n\t"
				"movl %%eax,36(%%edi)\n\t"
				"movl %%edx,40(%%edi)\n\t"
				"movl %%edx,44(%%edi)\n\t"
				"movl %%eax,48(%%edi)\n\t"
				"movl %%eax,52(%%edi)\n\t"
				"movl %%edx,56(%%edi)\n\t"
				"movl %%edx,60(%%edi)\n\t"
				"rcll $1, %%eax\n\t"
				"leal 64(%%edi), %%edi\n\t"
				"decl %%ecx\n\t"
				"jnz  L100\n\t"
				: "=D" (p)
				: "D" (p), "c" (len), "a" (1)
				: "edx"
			);
			do_tick();
			BAILR
		} while (!done);
	}

	/* Now move the data around 
	 * First move the data up half of the segment size we are testing
	 * Then move the data to the original location + 32 bytes
	 */
	for (j=0; j<segs; j++) {
		start = (unsigned long)map[j].start;
		end = (unsigned long)map[j].end;
		pe = start;
		p = start;
		done = 0;
		do {
			/* Check for overflow */
			if (pe + SPINSZ*4 > pe) {
				pe += SPINSZ*4;
			} else {
				pe = end;
			}
			if (pe >= end) {
				pe = end;
				done++;
			}
			if (p == pe ) {
				break;
			}
			pp = p + ((pe - p) / 2);
			len  = ((unsigned long)pe - (unsigned long)p) / 8;
			for(i=0; i<iter; i++) {
				asm __volatile__ (
					"cld\n"
					"jmp L110\n\t"

					".p2align 4,,7\n\t"
					"L110:\n\t"
					"movl %1,%%edi\n\t"
					"movl %0,%%esi\n\t"
					"movl %2,%%ecx\n\t"
					"rep\n\t"
					"movsl\n\t"
					"movl %0,%%edi\n\t"
					"addl $32,%%edi\n\t"
					"movl %1,%%esi\n\t"
					"movl %2,%%ecx\n\t"
					"subl $8,%%ecx\n\t"
					"rep\n\t"
					"movsl\n\t"
					"movl %0,%%edi\n\t"
					"movl $8,%%ecx\n\t"
					"rep\n\t"
					"movsl\n\t"
					:: "g" (p), "g" (pp), "g" (len)
					: "edi", "esi", "ecx"
				);
				do_tick();
				BAILR
			}
			p = pe;
		} while (!done);
	}

	/* Now check the data 
	 * The error checking is rather crude.  We just check that the
	 * adjacent words are the same.
	 */
	for (j=0; j<segs; j++) {
		start = (unsigned long)map[j].start;
		end = (unsigned long)map[j].end;
		pe = start;
		p = start;
		done = 0;
		do {
			/* Check for overflow */
			if (pe + SPINSZ*4 > pe) {
				pe += SPINSZ*4;
			} else {
				pe = end;
			}
			if (pe >= end) {
				pe = end;
				done++;
			}
			if (p == pe ) {
				break;
			}
			asm __volatile__ (
				"jmp L120\n\t"

				".p2align 4,,7\n\t"
				"L120:\n\t"
				"movl (%%edi),%%ecx\n\t"
				"cmpl 4(%%edi),%%ecx\n\t"
				"jnz L121\n\t"

				"L122:\n\t"
				"addl $8,%%edi\n\t"
				"cmpl %%edx,%%edi\n\t"
				"jb L120\n"
				"jmp L123\n\t"

				"L121:\n\t"
				"pushl %%edx\n\t"
				"pushl 4(%%edi)\n\t"
				"pushl %%ecx\n\t"
				"pushl %%edi\n\t"
				"call mv_error\n\t"
				"popl %%edi\n\t"
				"addl $8,%%esp\n\t"
				"popl %%edx\n\t"
				"jmp L122\n"
				"L123:\n\t"
				: "=D" (p)
				: "D" (p), "d" (pe)
				: "ecx"
			);
			do_tick();
			BAILR
		} while (!done);
	}
}
#endif


#if 0
int main(int argc,char **argv)
{
	char *buf;

	buf = (char*)malloc(0x10000000);
	map[0].start = (unsigned long*)buf;
	map[0].end = (unsigned long*)(buf + 0x10000000);
	addr_tst1();
	addr_tst2();
	movinv1(2,0,~0);
}
#endif
#include "mycmd.c"
