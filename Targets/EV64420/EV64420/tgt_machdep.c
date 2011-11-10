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
#undef DEEP_DEBUG

#if 0
#ifdef USE_LEGACY_RTC
#	undef NVRAM_IN_FLASH 
#else
#	define NVRAM_IN_FLASH 1
#endif
#endif

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

#ifdef USE_LEGACY_RTC
#include <dev/ic/mc146818reg.h>
#include <linux/io.h>
#else
#include <dev/ic/ds15x1reg.h>
#endif

#include <autoconf.h>

#include <machine/cpu.h>
#include <machine/pio.h>
#include "pflash.h"
#include "dev/pflash_tgt.h"

#include "include/ev64420.h"
#include <pmon/dev/gt64420reg.h>
#include <pmon/dev/ns16550.h>

#include <pmon.h>
#include <sys/ioctl.h>

#include "mod_x86emu.h"
#include "mod_x86emu_int10.h"
#include "mod_vgacon.h"
extern void vga_bios_init(void);
extern int kbd_initialize(void);
extern int write_at_cursor(char val);
extern const char *kbd_error_msgs[];
#include "flash.h"
#if (NMOD_FLASH_AMD + NMOD_FLASH_INTEL + NMOD_FLASH_SST) == 0
#ifdef HAVE_FLASH
#undef HAVE_FLASH
#endif
#else
#define HAVE_FLASH
#endif

#if NMOD_X86EMU_INT10 == 0
int vga_available=0;
#else
#include "vgarom.c"
#endif

extern int boot_i2c_read(int addr);

extern struct trapframe DBGREG;

extern void *memset(void *, int, size_t);

int kbd_available;

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

int mpscdebug (int op, struct DevEntry *dev, unsigned long param, int data);
ConfigEntry	ConfigTable[] =
{
#ifndef USE_SUPERIO_UART
	{ (char *)1,0,mpscdebug,256, CONS_BAUD, NS16550HZ },
#else
	{ (char *)COM1_BASE_ADDR, 0, ns16550, 256, CONS_BAUD, NS16550HZ },
	//{ (char *)COM1_BASE_ADDR, 0, ns16550, 256, CONS_BAUD, NS16550HZ },
	//{ (char *)COM2_BASE_ADDR, 0, ns16550, 256, CONS_BAUD, NS16550HZ },
#endif
#if NMOD_VGACON > 0
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
void test_regs(void);

void
initmips(unsigned int memsz)
{
	memorysize=(memsz&0x0000ffff) << 20;
	memorysize_high=((memsz&0xffff0000)>>16) << 20;

	SBD_DISPLAY("STAR",0);
	/*
	 *  Probe clock frequencys so delays will work properly.
	 */
#ifdef USE_SUPERIO_UART
#define	ISAREFRESH (14318180/(100000/15))
	linux_outb(0x7c,0x43);
	linux_outb(ISAREFRESH&&0xff,0x41);
	linux_outb(ISAREFRESH>>8,0x41);
#endif

	tgt_cpufreq();

#if 0
	tgt_display("memtest with cache on\n",0);

	/* memtest */
	addr_tst1();
	addr_tst2();
	movinv1(2,0,~0);
	movinv1(2,0xaa5555aa,~0xaa5555aa);
	tgt_display("memtest done\n",0);

	/*
	__asm__ volatile (
			"mfc0    $2,$16\r\n"
			"and     $2,$2,~0x7\r\n"
			"or      $2,$2,0x2\r\n"
			"mtc0    $2,$16\r\n":::"$2");
	*/

#endif

	SBD_DISPLAY("DBGI",0);
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

extern void	vt82c686_init(void);
void
tgt_devconfig()
{

#if NMOD_VGACON >0
	int rc;
#endif

	
	/* Set the PHY addresses for the GT ethernet */
	GT_WRITE(ETH_PHY_ADDR_REG, 0x18a4);
	GT_WRITE(MAIN_ROUTING_REGISTER, 0x007ffe38);
#if defined(LINUX_PC)
	GT_WRITE(SERIAL_PORT_MULTIPLEX, 0x00001101);
#else
	GT_WRITE(SERIAL_PORT_MULTIPLEX, 0x00001102);
#endif
	
#if defined(LINUX_PC)
	/* Route Multi Purpose Pins */
	GT_WRITE(MPP_CONTROL0, 0x77777777);
	GT_WRITE(MPP_CONTROL1, 0x00000000);
	GT_WRITE(MPP_CONTROL2, 0x00888888);
	GT_WRITE(MPP_CONTROL3, 0x00000066);
#elif defined(GODSONEV2A) 
	/* Route Multi Purpose Pins */
	GT_WRITE(MPP_CONTROL0, 0x77777777);
	GT_WRITE(MPP_CONTROL1, 0x00000000);
	GT_WRITE(MPP_CONTROL2, 0x00888888);
	GT_WRITE(MPP_CONTROL3, 0x00000066);
#elif defined(LONGMENG)
/*
 * MPP8			MPP9		MPP18	MPP19	MPP20	MPP21	MPP22	MPP23	MPP24	MPP25	
 * DBG_UART_TX DBG_UART_RX  GNT0# 	REQ0# 	GNT1#	REQ1#	GNT#2	REQ#2	GNT#3	REQ#3
 * MPP28      MPP29     MPP30     MPP31
 * PCI_INTA#  PCI_INTB# PCI_INTC# PCI_INTD#		
 */
#define DBG_LED0	(1<<4)     /*MPP4 as led0*/
#define DBG_LED1	(1<<5)     /*MPP5 as led1*/
#define DBG_LED2	(1<<6)	   /*MPP6 as led2*/
#define MPP_UART_TX	(1<<0)
#define MP_INTA	(1<<24)
#define MP_INTB	(1<<25)
#define MP_INTC	(1<<26)
#define MP_INTD	(1<<27)

 	GT_WRITE(GPP_IO_CONTROL,MPP_UART_TX|DBG_LED0|DBG_LED1|DBG_LED2);
 	GT_WRITE(GPP_LEVEL_CONTROL,MP_INTA|MP_INTB|MP_INTC|MP_INTD);
 	GT_WRITE(GPP_INTERRUPT_MASK,MP_INTA|MP_INTB|MP_INTC|MP_INTD);

	/* Route Multi Purpose Pins */
	GT_WRITE(MPP_CONTROL0, 0x00000322);	//MPP0,MPP1 as S0_TXD,S0_RXD,MPP2 clk24M,others set to gpio pin
	GT_WRITE(MPP_CONTROL1, 0x00000000);	
	GT_WRITE(MPP_CONTROL2, 0x11111111);	//MPP16~23 as PCI REQ or GNT.
	GT_WRITE(MPP_CONTROL3, 0x00400000); //MPP24~27 as PCI INTA-INTD,MPP29 as clk14M if nessary,others gpio. 

#else
	/* Route Multi Purpose Pins */
	GT_WRITE(MPP_CONTROL0, 0x53547777);
	GT_WRITE(MPP_CONTROL1, 0x44009911);
	GT_WRITE(MPP_CONTROL2, 0x40098888);
	GT_WRITE(MPP_CONTROL3, 0x00090066);
#endif	
	_pci_devinit(1);	/* PCI device initialization */
	SBD_DISPLAY("VGAI", 0);
#if (NMOD_X86EMU_INT10 > 0) || (NMOD_X86EMU >0)
	vga_bios_init();
#endif
	config_init();
	SBD_DISPLAY("CONF", 0);
	configure();
#if  NMOD_VGACON >0
	rc=kbd_initialize();
	printf("%s\n",kbd_error_msgs[rc]);
	if(!rc){ 
		kbd_available=1;
	}
#endif
	SBD_DISPLAY("DEVD", 0);
}

extern void godson2_cache_flush(void);
#ifdef DEEP_DEBUG
void tgt_test_memory(void)
{
	register unsigned i,j;
        register volatile char * volatile cmem;
        register volatile unsigned * volatile imem;
        register volatile unsigned * volatile imem_uc;
  //      long long * volatile imem;

	printf("%s:%d executing...\n",__FUNCTION__,__LINE__);
	j = 0;
	for (i=0;i<100000;i++) {
		i++;
	}
	tgt_putchar('1');
	if (i!=j) { tgt_putchar('2'); } 
        else { tgt_putchar('('); }

        cmem = (char*)0x80400000L;
        imem = (unsigned *)0x80400000L;
        imem_uc = (unsigned *)0xa0400000L;

        for (i=0;i<64*1024/sizeof(*cmem);i++) {
          cmem[i] = 0x5a;
          if (cmem[i]!=0x5a) {
	   tgt_putchar('4');
          }else{
	   //tgt_putchar('3');
          }
        }
	puts("char access test passed");

        for (i=0;i<64*1024/sizeof(*imem);i++) {
          imem[i] = 0xaa55aa55;
          if (imem[i]!=0xaa55aa55) {
	   tgt_putchar('6');
          }else{
	   //tgt_putchar('5');
          }
        }

	puts("word access test passed");

	printf("executing...\n");
        for (i=0,j=0x87654321;i<64*1024/sizeof(*imem);i++,j+=0x01010101) {
	  imem[i] = j;
	  if (imem[i] != j) printf("error1(imem[i]=0x%x,j=0x%x.\n", imem[i],j);
	}
	CPU_FlushDCache((int)imem, 64*1024);
        for (i=0,j=0x87654321;i<64*1024/sizeof(*imem);i++,j+=0x01010101) {
	  if (imem_uc[i] != j) printf("%s:%d error(imem_uc[i]=0x%x,j=0x%x.\n", __FUNCTION__,__LINE__,imem_uc[i],j);
	}
        for (i=0,j=0x87654321;i<64*1024/sizeof(*imem);i++,j+=0x01010101) {
	  if (imem[i] != j) printf("%s:%d error(imem[i]=0x%x,j=0x%x.\n", __FUNCTION__,__LINE__,imem[i],j);
	}

	printf("%s:%d executing...\n",__FUNCTION__,__LINE__);
        for (i=0,j=0x12345678;i<64*1024/sizeof(*imem);i++,j+=0x01010101) {
	  imem_uc[i] = j;
	  if (imem_uc[i] != j) printf("%s:%d error(imem_uc[i]=0x%x,j=0x%x.\n", __FUNCTION__,__LINE__,imem_uc[i],j);
	}
	CPU_HitInvalidateDCache((int)imem, 64*1024);
        for (i=0,j=0x12345678;i<64*1024/sizeof(*imem);i++,j+=0x01010101) {
	  if (imem[i] != j) printf("%s:%d error(imem[i]=0x%x,j=0x%x.\n", __FUNCTION__,__LINE__,imem[i],j);
	}
        for (i=0,j=0x12345678;i<64*1024/sizeof(*imem);i++,j+=0x01010101) {
	  if (imem_uc[i] != j) printf("%s:%d error(imem_uc[i]=0x%x,j=0x%x.\n", __FUNCTION__,__LINE__,imem_uc[i],j);
	}

}
#endif
extern int novga;
void
tgt_devinit()
{
int cnt;
//ioctl (STDIN, FIONREAD, &cnt);
//if(cnt){novga=1;}

	SBD_DISPLAY("686I",0);
	
	vt82c686_init();
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
	
	SBD_DISPLAY("CACH",0);
	CPU_ConfigCache();
	SBD_DISPLAY("CAC2",0);
	SBD_DISPLAY("CAC3",0);

	//godson2_cache_flush();

	_pci_businit(1);	/* PCI bus initialization */
}


void
tgt_reboot()
{

	void (*longreach) __P((void));
	
	longreach = (void *)0xbfc00000;
	(*longreach)();
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
	printf("Board revision level: %c.\n", in8(PLD_BAR) + 'A');
	printf("PLD revision levels: %d.%d and %d.%d.\n",
					in8(PLD_ID1) >> 4, in8(PLD_ID1) & 15,
					in8(PLD_ID2) >> 4, in8(PLD_ID2) & 15);
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
    printf("[[[[[[[2000][[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[\n"); 
}
#ifdef USE_LEGACY_RTC
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

static void init_legacy_rtc(void)
{
	int year, month, date, hour, min, sec;
	char buf[128];
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

	sprintf(buf,"RTC: %02d-%02d-%02d %02d:%02d:%02d\n",
			year, month, date, hour, min, sec);
	tgt_display(buf,0);
}
#endif

static void
_probe_frequencies()
{
#ifdef HAVE_TOD
	int i, timeout, cur, sec, cnt;
#endif
	extern void tgt_setpar100mhz __P((void));

	SBD_DISPLAY ("FREQ", CHKPNT_FREQ);


	md_pipefreq = 300000000;	/* Defaults */
	md_cpufreq  = 100000000;
	clk_invalid = 1;
#ifdef HAVE_TOD
#ifdef USE_LEGACY_RTC
	init_legacy_rtc();
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
			tgt_display("RTC clock is not running!\r\n",0);
			break;		/* Get out if clock is not running */
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
		md_cpufreq = 100000000;
	}

	/*
	 * Hack to change timing of GT64420 if not 125Mhz FSB.
	 */

	if(md_cpufreq < 110000000) {
		//tgt_setpar100mhz();
	}

#endif /* USE_LEGACY_RTC */
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

#ifdef HAVE_TOD
/*
 *  Returns the current time if a TOD clock is present or 0
 *  This code is defunct after 2088... (out of bits)
 */
#define FROMBCD(x)      (((x) >> 4) * 10 + ((x) & 0xf))
#define TOBCD(x)        (((x) / 10 * 16) + ((x) % 10)) 
#define YEARDAYS(year)  ((((year) % 4) == 0 && \
                         ((year % 100) != 0 || (year % 400) == 0)) ? 366 : 365)
#define SECMIN  (60)            /* seconds per minute */
#define SECHOUR (60*SECMIN)     /* seconds per hour */
#define SECDAY  (24*SECHOUR)    /* seconds per day */
#define SECYR   (365*SECDAY)    /* seconds per common year */

static const short dayyr[12] =
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

time_t
tgt_gettime()
{
	struct tm tm;
	int ctrlbsave;
	time_t t;

	if(!clk_invalid) {
#ifdef USE_LEGACY_RTC
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
#else
		/* Freeze readout data regs */
		ctrlbsave =  inb(RTC_BASE + DS_REG_CTLB);
		outb(RTC_BASE + DS_REG_CTLB, ctrlbsave & ~DS_CTLB_TE);

		tm.tm_sec = FROMBCD(inb(RTC_BASE + DS_REG_SEC));
		tm.tm_min = FROMBCD(inb(RTC_BASE + DS_REG_MIN));
		tm.tm_hour = FROMBCD(inb(RTC_BASE + DS_REG_HOUR));
		tm.tm_wday = FROMBCD(inb(RTC_BASE + DS_REG_WDAY));
		tm.tm_mday = FROMBCD(inb(RTC_BASE + DS_REG_DATE));
		tm.tm_mon = FROMBCD(inb(RTC_BASE + DS_REG_MONTH) & 0x1f) - 1;
		tm.tm_year = FROMBCD(inb(RTC_BASE + DS_REG_YEAR));
		tm.tm_year += 100 * (FROMBCD(inb(RTC_BASE + DS_REG_CENT)) - 19);

		outb(RTC_BASE + DS_REG_CTLB, ctrlbsave | DS_CTLB_TE);
#endif /* USE_LEGACY_RTC */

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

	if(!clk_invalid) {
		tm = gmtime(&t);
#ifdef USE_LEGACY_RTC
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
#else
		/* Enable register writing */
		ctrlbsave =  inb(RTC_BASE + DS_REG_CTLB);
		outb(RTC_BASE + DS_REG_CTLB, ctrlbsave & ~DS_CTLB_TE);

		outb(RTC_BASE + DS_REG_CENT, TOBCD(tm->tm_year / 100 + 19));
		outb(RTC_BASE + DS_REG_YEAR, TOBCD(tm->tm_year % 100));
		outb(RTC_BASE + DS_REG_MONTH,TOBCD(tm->tm_mon + 1));
		outb(RTC_BASE + DS_REG_DATE, TOBCD(tm->tm_mday));
		outb(RTC_BASE + DS_REG_WDAY, TOBCD(tm->tm_wday));
		outb(RTC_BASE + DS_REG_HOUR, TOBCD(tm->tm_hour));
		outb(RTC_BASE + DS_REG_MIN,  TOBCD(tm->tm_min));
		outb(RTC_BASE + DS_REG_SEC,  TOBCD(tm->tm_sec));

		/* Transfer new time to counters */
		outb(RTC_BASE + DS_REG_CTLB, ctrlbsave | DS_CTLB_TE);
#endif /* USE_LEGACY_RTC */
	}
}

#else

time_t
tgt_gettime()
{
	return(957960000);  /* Wed May 10 14:00:00 2000 :-) */
}

#endif /* HAVE_TOD */


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
	printf("Copyright 2003, Michael and Pine, ICT CAS.\n");
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
struct fl_map tgt_fl_map_boot8[] = {
        TARGET_FLASH_DEVICES_8
};
 
struct fl_map tgt_fl_map_boot32[] = {
        TARGET_FLASH_DEVICES_32
};

struct fl_map tgt_fl_map_boot16[]={
	TARGET_FLASH_DEVICES_16
};


struct fl_map *
tgt_flashmap()
{
#ifdef LONGMENG
return tgt_fl_map_boot16;
#else
	if((GT_READ(GT_BOOT_PAR) & 0x00300000) == 0) {
		return tgt_fl_map_boot8;
	}
	else {
		return tgt_fl_map_boot32;
	}
#endif
}   
void
tgt_flashwrite_disable()
{
}

int
tgt_flashwrite_enable()
{
	if(in8(PLD_BSTAT) & 0x40) {
		return(1);
	}
	else {
		return(1);	/* can't enable. jumper selected! */
	}
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
#ifdef NVRAM_IN_FLASH
	nvram = (char *)(tgt_flashmap())->fl_map_base;
	if(fl_devident(nvram, NULL) == 0 ||
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
	bcopy(&nvram[ETHER_OFFS], hwethadr, 6);
	{int i;
	 for(i=0;i<6;i++)
		 if(hwethadr[i]!=0)break;
	 if(i==6){
		 for(i=0;i<6;i++)
			 hwethadr[i]=i;
	 }
	}
	sprintf(env, "%02x:%02x:%02x:%02x:%02x:%02x", hwethadr[0], hwethadr[1],
	    hwethadr[2], hwethadr[3], hwethadr[4], hwethadr[5]);
	(*func)("ethaddr", env);

#ifndef NVRAM_IN_FLASH
	free(nvram);
#endif

	sprintf(env, "%d", memorysize / (1024 * 1024));
	(*func)("memsize", env);

	sprintf(env, "%d", md_pipefreq);
	(*func)("cpuclock", env);

	sprintf(env, "%d", md_cpufreq);
	(*func)("busclock", env);

	(*func)("systype", SYSTYPE);

	sprintf(env, "%d", CpuTertiaryCacheSize / (1024*1024));
	(*func)("l3cache", env);

	sprintf(env, "%x", GT_BASE_ADDR);
	(*func)("gtbase", env);
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

	bcopy(hwethadr, &nvrambuf[ETHER_OFFS], 6);
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
	//printf("nvram_get:\n");
	for(i = 0; i < 114; i++) {
		linux_outb(i + RTC_NVRAM_BASE, RTC_INDEX_REG);	/* Address */
		buffer[i] = linux_inb(RTC_DATA_REG);
	}
	/*
	for(i = 0; i < 114; i++) {
		printf("%x ", buffer[i]);
	}
	printf("\n");
	*/
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

#if 0
void
mydelay(int microseconds)
{
	volatile int total;
	int start;
	start = CPU_GetCOUNT();
	//total = microseconds * 100;
	total = microseconds * clkperusec;

	while(total > (CPU_GetCOUNT() - start));
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
	char *p = msg;

	while (*p != 0) {
		tgt_putchar(*p++);
		delay(500);
	}
}

void
tgt_display1(char *msg, int x)
{
	char *p = msg;
	tgt_display(msg,x);
	tgt_display("\r\n",x);
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

#ifdef DEEP_DEBUG
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
   {(unsigned long*)0x88000000,(unsigned long*)(0x8f000000 - 0x100000)},
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
	char buf[256];
	sprintf(buf,"adr=%x,good=%x,bad=%x,xor=%x\n",adr,good,bad,xor);
	tgt_display(buf,0);
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
	tgt_display("Tst  Pass   Failing Address          Good       Bad     Err-Bits  Count Chan\n",0);
	tgt_display("---  ----  -----------------------  --------  --------  --------  ----- ----\n",0);
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
	tgt_display(".",0);
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
	char buf[256];

	tgt_display("address test 1..\r\n",0);

	/* Test the global address bits */
	for (p1=0, j=0; j<2; j++) {
        	//hprint(LINE_PAT, COL_PAT, p1);
		sprintf(buf,"\r\npat=%x\r\n",p1);
		tgt_display(buf,0);

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
		sprintf(buf,"\npat=%x\n",p1);
		tgt_display(buf,0);

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
	char buf[256];

        //cprint(LINE_PAT, COL_PAT, "        ");
	tgt_display("addr test 2...\n",0);

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
	char buf[256];

	/* Display the current pattern */
        //hprint(LINE_PAT, COL_PAT, p1);
	sprintf(buf,"movinv test with p1=%x,p2=%x...\n",p1,p2);
	tgt_display(buf,0);

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
	char buf[256];

	/* Display the current pattern */
	/*
        hprint(LINE_PAT, COL_PAT-2, p1);
	cprint(LINE_PAT, COL_PAT+6, "-");
        dprint(LINE_PAT, COL_PAT+7, offset, 2, 1);
	*/
	sprintf(buf,"modtst offset=%x,p1=%x,p2=%x\n",offset,p1,p2);
	tgt_display(buf,0);

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
#endif

#include "mycmd.c"
