/*	$Id: tgt_machdep.c,v 1.6 2006/07/20 09:37:06 cpu Exp $ */

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
//#define USE_LEGACY_RTC
#ifdef USE_LEGACY_RTC
#      undef NVRAM_IN_FLASH
#else
#      define NVRAM_IN_FLASH 1
#      define FLASH_OFFS (tgt_flashmap()->fl_map_size - 0x1000)
#endif

#include <include/stdarg.h>
void		tgt_putchar (int);
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
#include <stdarg.h>

#include <dev/ic/mc146818reg.h>
#include <linux/io.h>

#include <autoconf.h>

#include <machine/cpu.h>
#include <machine/pio.h>
#include "pflash.h"
#include "dev/pflash_tgt.h"

#include "../pci/cs5536_io.h"
#include "include/bonito.h"
#ifdef LOONGSON2F_7INCH
#include "kb3310.h"
#endif
#include <pmon/dev/gt64240reg.h>
#include <pmon/dev/ns16550.h>

#include <pmon.h>

#include "mod_x86emu_int10.h"
#include "mod_x86emu.h"
#include "mod_vgacon.h"
#include "mod_framebuffer.h"
extern int vga_bios_init(void);
extern int radeon_init(void);
extern int smi712_init(unsigned char *, unsigned char *);
extern int kbd_initialize(void);
extern int write_at_cursor(char val);
extern const char *kbd_error_msgs[];
#include "flash.h"

extern int fl_program(void *fl_base, void *data_base, int data_size, int verbose);

//#include "vt82c686.h"
#include "cs5536.h"
#include "target/cs5536.h"

#if (NCS5536 + NVT82C686) > 0
#define HAVE_RTC 1
#else
#define HAVE_RTC 0
#endif

#if (NMOD_FLASH_AMD + NMOD_FLASH_INTEL + NMOD_FLASH_SST + NMOD_FLASH_WINBOND) == 0

#ifdef HAVE_FLASH
#undef HAVE_FLASH
#endif

#else

#ifndef HAVE_FLASH
#define HAVE_FLASH
#endif

#endif

#if NMOD_X86EMU_INT10 != 0 || NMOD_X86EMU != 0
#ifdef VGA_NO_ROM
#include "vgarom.c"
#endif
#endif

extern struct trapframe DBGREG;

extern void *memset(void *, int, size_t);

int kbd_available = 0;
int usb_kbd_available;
int vga_available = 0;
static int  vga_ok = 0;

static int md_pipefreq = 0;
static int md_cpufreq = 0;
static int clk_invalid = 0;
static int nvram_invalid = 0;
static int *ec_version;
static int cksum(void *p, size_t s, int set);
static void _probe_frequencies(void);

#ifndef NVRAM_IN_FLASH
void nvram_get(char *);
void nvram_put(char *);
#endif

extern int vgaterm(int op, struct DevEntry * dev, unsigned long param, int data);
extern int fbterm(int op, struct DevEntry * dev, unsigned long param, int data);
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
#if HAVE_RTC > 0
static void init_legacy_rtc(void);
#endif

/*
 * NOTE : we use COMMON_COM_BASE_ADDR and NS16550HZ instead the former. please see
 * the Targets/Bonito/include/bonito.h for detail.
 */
ConfigEntry	ConfigTable[] =
{
	{ (char *)COMMON_COM_BASE_ADDR, 0, ns16550, 256, CONS_BAUD, NS16550HZ }, 
	 /*{ (char *)COM2_BASE_ADDR, 0, ns16550, 256, CONS_BAUD, NS16550HZ }, */
#if NMOD_VGACON >0
#if NMOD_FRAMEBUFFER >0
	{ (char *)1, 0, fbterm, 256, CONS_BAUD, NS16550HZ },
#else
	{ (char *)1, 0, vgaterm, 256, CONS_BAUD, NS16550HZ },
#endif
#endif
	{ 0 }
};

unsigned long _filebase;

extern unsigned int memorysize_high;

extern char MipsException[], MipsExceptionEnd[];

unsigned char hwethadr[6];

void initmips(unsigned int memsz);

void addr_tst1(void);
void addr_tst2(void);
void movinv1(int iter, ulong p1, ulong p2);

void
initmips(unsigned int memsz)
{
	/*
	 *	Set up memory address decoders to map entire memory.
	 *	But first move away bootrom map to high memory.
	 */
#if 0
	GT_WRITE(BOOTCS_LOW_DECODE_ADDRESS, BOOT_BASE >> 20);
	GT_WRITE(BOOTCS_HIGH_DECODE_ADDRESS, (BOOT_BASE - 1 + BOOT_SIZE) >> 20);
#endif
	tgt_fpuenable();
	//memsz = 512;
	memorysize = memsz > 256 ? 256 << 20 : memsz << 20;
	memorysize_high = memsz > 256 ? (memsz - 256) << 20 : 0;

	
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
extern void vt82c686_init(void);
extern void cs5536_init(void);
extern int fb_init(unsigned long,unsigned long);
void
tgt_devconfig()
{
#if NMOD_VGACON > 0
	int rc;
#if NMOD_FRAMEBUFFER > 0 
	unsigned long fbaddress,ioaddress;
	extern struct pci_device *vga_dev;
#endif
#endif
	_pci_devinit(1);	/* PCI device initialization */

#if NMOD_X86EMU_INT10 > 0 || NMOD_X86EMU > 0
	SBD_DISPLAY("VGAI", 0);
	rc = vga_bios_init();

#elif (NMOD_X86EMU_INT10 == 0 && defined(RADEON7000))
	SBD_DISPLAY("VGAI", 0);
	rc = radeon_init();
#endif

#ifdef SM712_GRAPHIC_CARD
	rc = 1; 
#endif

#if NMOD_FRAMEBUFFER > 0
	if (rc > 0) {
		SBD_DISPLAY("FRBI", 0);
		fbaddress  =_pci_conf_read(vga_dev->pa.pa_tag,0x10);
		ioaddress  =_pci_conf_read(vga_dev->pa.pa_tag,0x18);

		fbaddress = fbaddress &0xffffff00; //laster 8 bit
		ioaddress = ioaddress &0xfffffff0; //laster 4 bit

		printf("fbaddress 0x%x\tioaddress 0x%x\n",fbaddress, ioaddress);

#ifdef SM712_GRAPHIC_CARD
		fbaddress |= 0xb0000000;
		ioaddress |= 0xbfd00000;
		smi712_init((unsigned char *)fbaddress,
			(unsigned char *)ioaddress);
#endif

		fb_init(fbaddress, ioaddress);
	} else {
		printf("vga bios init failed, rc=%d\n",rc);
	}
#endif

	/* light the lcd */	
	*((volatile unsigned char *)(0xbfd00000 | HIGH_PORT)) = 0xfe;
	*((volatile unsigned char *)(0xbfd00000 | LOW_PORT)) = 0x01;
	*((volatile unsigned char *)(0xbfd00000 | DATA_PORT)) = 0x80;

	if (rc > 0) {
		if(!getenv("novga")) { 
			vga_available=1;
			vga_ok = 2;
		} else
			vga_ok = 1;
	}
	
	vga_available = 1; /*Suppress the output*/

	config_init();
	configure();
    
#if NMOD_VGACON >0
#if !(defined(VGA_NOTEBOOK_V1) || defined(VGA_NOTEBOOK_V2)) && NCS5536 > 0
	rc = kbd_initialize();
#else
	rc = -1;
#endif
	printf("%s\n",kbd_error_msgs[rc]);
	if(!rc){ 
		if(!getenv("nokbd")) kbd_available = 1;
	}
#endif
	if (vga_ok > 1)
		vga_available = 1;
}

extern int test_icache_1(short *addr);
extern int test_icache_2(int addr);
extern int test_icache_3(int addr);
extern void godson1_cache_flush(void);
#define tgt_putchar_uc(x) (*(void (*)(char)) (((long)tgt_putchar)|0x20000000)) (x)

extern void cs5536_pci_fixup(void);
extern void ec_fixup(void);
extern void ec_update_rom(void *src, int size);

/* disable AC_BEEP for cs5536 gpio1,         *
 * only used EC_BEEP to control U13 gate     *
 * set cs5536 gpio1 output low level voltage *
 * huangw 2008-10-16                         */
void cs5536_gpio1_fixup(void)
{
	unsigned long val;
	pcitag_t tag;
	unsigned long base;

	tag = _pci_make_tag(0, 14, 0);
	base = _pci_conf_read(tag, 0x14);
	base |= 0xbfd00000;
	base &= ~3;

	/* make cs5536 gpio1 output enable */
	val = *(volatile unsigned long *)(base + 0x04);
	val = ( val & ~(1 << (16 + 1)) ) | (1 << 1) ;
	*(volatile unsigned long *)(base + 0x04) = val;
	
	/* make cs5536 gpio1 output low level voltage. */
	val = *(volatile unsigned long *)(base + 0x00);
	val = (val | (1 << (16 + 1))) & ~(1 << 1);
	*(volatile unsigned long *)(base + 0x00) = val;
}

void
tgt_devinit()
{

	SBD_DISPLAY("5536",0);
	
#if NVT82C686 > 0
	vt82c686_init();
#endif

#if	NCS5536 > 0
	cs5536_init();
#endif

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

	_pci_businit(1);	/* PCI bus initialization */

#if	NCS5536 > 0
	cs5536_pci_fixup();
#endif
	cs5536_gpio1_fixup();
#ifdef HAS_EC
	ec_fixup();
#endif

	return;
}


void
tgt_reboot()
{
	/* reset the cs5536 whole chip */
#if NCS5536 > 0 && !defined(LOONGSON2F_7INCH) 
	unsigned long hi, lo;
	_rdmsr(0xe0000014, &hi, &lo);
	lo |= 0x00000001;
	_wrmsr(0xe0000014, hi, lo);
#endif

#ifdef LOONGSON2F_7INCH

	/* dark the lcd */
	*((volatile unsigned char *)(0xbfd00000 | HIGH_PORT)) = 0xfe;
	*((volatile unsigned char *)(0xbfd00000 | LOW_PORT)) = 0x01;
	*((volatile unsigned char *)(0xbfd00000 | DATA_PORT)) = 0x00;

#if 0
	*((volatile unsigned char *)(0xbfd00000 | HIGH_PORT)) = 0xfc;
	*((volatile unsigned char *)(0xbfd00000 | LOW_PORT)) = 0x20;
	val = *((volatile unsigned char *)(0xbfd00000 | DATA_PORT));
	/* output the low level for reset sequence */
	*((volatile unsigned char *)(0xbfd00000 | DATA_PORT)) = val & (~(1 << 5));
	/* delay for 100~200ms */
	for(i = 0; i < 200; i++);
		delay(1000);
	/* output the high level for reset sequence */
	*((volatile unsigned char *)(0xbfd00000 | DATA_PORT)) = val | (1 << 5);
#else
	*((volatile unsigned char *)(0xbfd00000 | HIGH_PORT)) = 0xf4;
	*((volatile unsigned char *)(0xbfd00000 | LOW_PORT)) = 0xec;
	*((volatile unsigned char *)(0xbfd00000 | DATA_PORT)) = 0x01;
#endif

#endif
	/* we should not exec until here. */
	//__asm__ ("jr %0\n"::"r"(0xbfc00000));

	while(1);
}

void
tgt_poweroff()
{
	unsigned long val;
	unsigned long tag;
	unsigned long base;

#ifdef	LOONGSON2F_7INCH
#if 0
	*((volatile unsigned char *)(0xbfd00000 | HIGH_PORT)) = 0xfc;
	*((volatile unsigned char *)(0xbfd00000 | LOW_PORT)) = 0x29;
	val = *((volatile unsigned char *)(0xbfd00000 | DATA_PORT));
	*((volatile unsigned char *)(0xbfd00000 | DATA_PORT)) = val & ~(1 << 1);
	for(i = 0; i < 0x10000; i++)
		for(j = 0; j < 0x10000; i++);
	*((volatile unsigned char *)(0xbfd00000 | DATA_PORT)) = val | (1 << 1);
#else
	/* cpu-gpio0 output low */
	*((volatile unsigned long *)(0xbfe0011c)) &= ~0x00000001;
	/* cpu-gpio0 as output */
	*((volatile unsigned long *)(0xbfe00120)) &= ~0x00000001;
#endif
#else
	tag = _pci_make_tag(0, 14, 0);
	base = _pci_conf_read(tag, 0x14);
	base |= 0xbfd00000;
	base &= ~3;

	/* make cs5536 gpio13 output enable */
	val = *(volatile unsigned long *)(base + 0x04);
	val = ( val & ~(1 << (16 + 13)) ) | (1 << 13) ;
	*(volatile unsigned long *)(base + 0x04) = val;
	
	/* make cs5536 gpio13 output low level voltage. */
	val = *(volatile unsigned long *)(base + 0x00);
	val = (val | (1 << (16 + 13))) & ~(1 << 13);
	*(volatile unsigned long *)(base + 0x00) = val;
#endif
	while(1);
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
}

/*
 *  Display any target specific logo.
 */
void
tgt_logo()
{
#if 0
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
#endif
}

#if HAVE_RTC > 0
static void init_legacy_rtc(void)
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

	//printf("RTC: %02d-%02d-%02d %02d:%02d:%02d\n",
	//    year, month, date, hour, min, sec);
}
#endif

void suppress_auto_start(void)
{
	/* suppress auto start when power pluged in */
	CMOS_WRITE(0x80, 0xd);
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
#if defined(HAVE_TOD) && HAVE_RTC 
        int i, timeout, cur, sec, cnt;
#endif
                                                                               
        SBD_DISPLAY ("FREQ", CHKPNT_FREQ);
                                                                               
                                                                               
#if 1
        md_pipefreq = 300000000;        /* Defaults */
        md_cpufreq  = 66000000;
#else
        md_pipefreq = 120000000;        /* NB FPGA*/
        md_cpufreq  =  40000000;
#endif

        clk_invalid = 1;
	
#if defined(HAVE_TOD) && (NCS5536 >0 || NVT82C686 > 0)
        init_legacy_rtc();

        SBD_DISPLAY ("FREI", CHKPNT_FREQ);

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
	 *  Calculate the external bus clock frequency.
	 */
	if (timeout != 0) {
		clk_invalid = 0;
		md_pipefreq = cnt / 10000;
		md_pipefreq *= 20000;
		/* we have no simple way to read multiplier value
		 */
		md_cpufreq = 66000000;
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
	//return 0;
                                                                               
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
	} else {
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
	printf("Copyright 2006, Lemote Corp. Ltd., ICT CAS.\n");
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

struct fl_map tgt_fl_map_boot8[]={
	TARGET_FLASH_DEVICES_8
};


struct fl_map *
tgt_flashmap()
{
	return tgt_fl_map_boot8;
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
#if 0
	if(fl_erase_device(p, size, TRUE)) {
		printf("Erase failed!\n");
		return;
	}
	if(fl_program_device(p, s, size, TRUE)) {
		printf("Programming failed!\n");
	}
#endif
	if( fl_program(p, s, size, TRUE) ){
		printf("Programming failed!\n");
	}
	
	fl_verify_device(p, s, size, TRUE);
}
#endif /* PFLASH */

/* update the ec_firmware */
void tgt_ecprogram(void *s, int size)
{
	ec_update_rom(s, size);
 	return;
}

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

extern unsigned char *get_ecver(void);

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
	nvram = (char *)(tgt_flashmap()->fl_map_base + FLASH_OFFS);
	printf("nvram %x\n", nvram);
	if(fl_devident((void *)(tgt_flashmap()->fl_map_base), NULL) == 0 ||
           cksum(nvram + NVRAM_OFFS, NVRAM_SIZE, 0) != 0) {
#else
	nvram = (char *)malloc(512);
	nvram_get(nvram);
	if(cksum(nvram, NVRAM_SIZE, 0) != 0) {
#endif
			printf("NVRAM is invalid!\n");
			nvram_invalid = 1;
	} else {
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
	/*bcopy(&nvram[ETHER_OFFS], hwethadr, 6);
	sprintf(env, "%02x:%02x:%02x:%02x:%02x:%02x", hwethadr[0], hwethadr[1],
	    hwethadr[2], hwethadr[3], hwethadr[4], hwethadr[5]);
	(*func)("ethaddr", env);*/

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

	/* get ec version */
	ec_version = get_ecver();
	sprintf(env, "%s", ec_version);
	(*func)("ECVersion", env);
}

int
tgt_unsetenv(char *name)
{
	char *ep, *np, *sp;
	char *nvram;
	char *nvrambuf;
	char *nvramsecbuf;
	int status;

	if(nvram_invalid) {
		return(0);
	}

	/* Use first defined flash device (we probably have only one) */
#ifdef NVRAM_IN_FLASH
	nvram = (char *)((tgt_flashmap())->fl_map_base + FLASH_OFFS);

	/* Map. Deal with an entire sector even if we only use part of it */
	nvram += NVRAM_OFFS & ~(NVRAM_SECSIZE - 1);
	nvramsecbuf = (char *)malloc(NVRAM_SECSIZE);
	if(nvramsecbuf == 0) {
		printf("Warning! Unable to malloc nvrambuffer!\n");
		return(-1);
	}
	memcpy(nvramsecbuf, nvram, NVRAM_SECSIZE);
	nvrambuf = nvramsecbuf + (NVRAM_OFFS & (NVRAM_SECSIZE - 1));
#else
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
#if 0
			if(fl_erase_device(nvram, NVRAM_SECSIZE, TRUE)) {
				status = -1;
				break;
			}

			if(fl_program_device(nvram, nvramsecbuf, NVRAM_SECSIZE, TRUE)) {
				status = -1;
				break;
			}
#endif
			fl_program(nvram, nvramsecbuf, NVRAM_SECSIZE, TRUE);
#else
			nvram_put(nvram);
#endif
			status = 1;
			break;
		} else if(*ep != '\0') {
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
	nvram = (char *)((tgt_flashmap())->fl_map_base + FLASH_OFFS);
	/* Deal with an entire sector even if we only use part of it */
	nvram += NVRAM_OFFS & ~(NVRAM_SECSIZE - 1);
#endif

        /* If NVRAM is found to be uninitialized, reinit it. */
	if(nvram_invalid) {
		nvramsecbuf = (char *)malloc(NVRAM_SECSIZE);
		if(nvramsecbuf == 0) {
			printf("Warning! Unable to malloc nvrambuffer!\n");
			return(-1);
		}
#ifdef NVRAM_IN_FLASH
		memcpy(nvramsecbuf, nvram, NVRAM_SECSIZE);
#endif
		nvrambuf = nvramsecbuf + (NVRAM_OFFS & (NVRAM_SECSIZE - 1));
		memset(nvrambuf, -1, NVRAM_SIZE);
		nvrambuf[2] = '\0';
		nvrambuf[3] = '\0';
		cksum((void *)nvrambuf, NVRAM_SIZE, 1);
		printf("Warning! NVRAM checksum fail. Reset!\n");
#ifdef NVRAM_IN_FLASH
#if 0
		if(fl_erase_device(nvram, NVRAM_SECSIZE, TRUE)) {
			printf("Error! Nvram erase failed!\n");
			free(nvramsecbuf);
			return(-1);
		}
		if(fl_program_device(nvram, nvramsecbuf, NVRAM_SECSIZE, TRUE)) {
			printf("Error! Nvram init failed!\n");
			free(nvramsecbuf);
			return(-1);
		}
#endif
		fl_program(nvram, nvramsecbuf, NVRAM_SECSIZE, TRUE);
#else
		nvram_put(nvramsecbuf);
#endif
		nvram_invalid = 0;
		free(nvramsecbuf);
	}

	/* Remove any current setting */
	tgt_unsetenv(name);

	/* Find end of evironment strings */
	nvramsecbuf = (char *)malloc(NVRAM_SECSIZE);
	if(nvramsecbuf == 0) {
		printf("Warning! Unable to malloc nvrambuffer!\n");
		return(-1);
	}
#ifndef NVRAM_IN_FLASH
	nvram_get(nvramsecbuf);
#else
	memcpy(nvramsecbuf, nvram, NVRAM_SECSIZE);
#endif
	nvrambuf = nvramsecbuf + (NVRAM_OFFS & (NVRAM_SECSIZE - 1));
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

	bcopy(hwethadr, &nvramsecbuf[ETHER_OFFS], 6);
	cksum(nvrambuf, NVRAM_SIZE, 1);
#ifdef NVRAM_IN_FLASH
#if 0
	if(fl_erase_device(nvram, NVRAM_SECSIZE, TRUE)) {
		printf("Error! Nvram erase failed!\n");
		free(nvramsecbuf);
		return(0);
	}
	if(fl_program_device(nvram, nvramsecbuf, NVRAM_SECSIZE, TRUE)) {
		printf("Error! Nvram program failed!\n");
		free(nvramsecbuf);
		return(0);
	}
#endif
	fl_program(nvram, nvramsecbuf, NVRAM_SECSIZE, TRUE);
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

static int putDebugChar(unsigned char byte)
{
	while ((linux_inb(0x3fd) & 0x20) == 0);
	linux_outb(byte,0x3f8);
	return 1;
}

static char buf[1024];
void prom_printf(char *fmt, ...)
{
	va_list args;
	int l;
	char *p, *buf_end;

	int putDebugChar(unsigned char);

	va_start(args, fmt);
	l = vsprintf(buf, fmt, args); /* hopefully i < sizeof(buf) */
	va_end(args);

	buf_end = buf + l;

	for (p = buf; p < buf_end; p++) {
		/* Crude cr/nl handling is better than none */
		if(*p == '\n')putDebugChar('\r');
		putDebugChar(*p);
	}
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

int netopen (const char *, int);
int netread (int, void *, int);
int netwrite (int, const void *, int);
long netlseek (int, long, int);
int netioctl (int, int, void *);
int netclose (int);
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
