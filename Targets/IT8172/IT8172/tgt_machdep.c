/*	$Id: tgt_machdep.c,v 1.1.1.1 2006/09/14 01:59:09 root Exp $ */

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
#include <sys/syslog.h>
#include <machine/endian.h>
#include <sys/device.h>
#include <machine/cpu.h>
#include <machine/pio.h>
#include <machine/intr.h>
#include <dev/pci/pcivar.h>
#include <sys/types.h>
#include <termio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>

#include <autoconf.h>

#include <machine/cpu.h>
#include <machine/pio.h>
#include "pflash.h"
#include "dev/pflash_tgt.h"


#include <include/it8172.h>
#include <include/rtc.h>
#include <pmon/dev/ns16550.h>
#include <pmon.h>
#include <linux/io.h>

#include "mod_x86emu.h"
#include "mod_x86emu_int10.h"
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

#if NMOD_X86EMU_INT10 == 0
int vga_available=0;
#endif

extern int boot_i2c_read(int addr);
extern int sis6326_init(int fbaddress, int ioaddress);
extern int fb_init(void);


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
extern int fbterm(int op, struct DevEntry * dev, unsigned long param, int data);
extern struct pci_device *vga_dev;

ConfigEntry	ConfigTable[] =
{
	{ (char *)(KSEG1 + IT8172_PCI_IO_BASE + IT_UART_BASE), 0, ns16550, 256, CONS_BAUD, NS16550HZ },
/*	{ (char *)(KSEG1 + IT8172_PCI_IO_BASE + IT_UART_BASE), 0, ns16550, 256, CONS_BAUD, NS16550HZ },*/
#if NMOD_VGACON > 0
#ifndef CONFIG_VGA_CARD_SIS6326
	{ (char *)1, 0, vgaterm, 256, CONS_BAUD, NS16550HZ },
#else
	{ (char *)1, 0, fbterm, 256, CONS_BAUD, NS16550HZ },
#endif
#endif

	{ 0 }
};

unsigned long _filebase;

extern int memorysize;
extern int memorysize_high;

extern char MipsException[], MipsExceptionEnd[];

unsigned char hwethadr[6];
int fb_available = 0;
unsigned long fbaddress = 0;
unsigned long ioaddress = 0;

void initmips(unsigned int memsz);

void addr_tst1(void);
void addr_tst2(void);
void movinv1(int iter, ulong p1, ulong p2);
void test_regs(void);


void
flash_gpio (int code)
{
        *((volatile unsigned short *)0xb4013802) = 0x0550;
        *((volatile unsigned char *)0xb4013800) = (code & 0xf) << 2;
}


void
initmips(unsigned int memsz)
{
	memorysize = memsz;

	/*
	 *  Init PMON and debug
	 */
	cpuinfotab[0] = &DBGREG;
	dbginit(NULL);

	tgt_cpufreq();

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
 * serial printf
 */
void
tgt_puts(char *buf)
{
	char *p;
	for (p = buf; *p != '\0'; p++)
		tgt_putchar(*p);
}

int
tgt_printf(const char *fmt, ...)
{
	int len;
	va_list	ap;
	char buf[200];
	
	va_start(ap,fmt);
	len = vsprintf(buf, fmt, ap);
	tgt_puts(buf);
	va_end(ap);

	return (len);
}

/*
 *  Put all machine dependent initialization here. This call
 *  is done after console has been initialized so it's safe
 *  to output configuration and debug information with printf.
 */

/*** LPC ***/
// MB PnP configuration register
#define LPC_KEY_ADDR    0x2E
#define LPC_DATA_ADDR   0x2F
// Device LDN
#define LDN_SERIAL1     0x01 
#define LDN_SERIAL2     0x02
#define LDN_PARALLEL    0x03 
#define LDN_KEYBOARD    0x05
#define LDN_MOUSE       0x06

static void LPCEnterMBPnP(void)
{
	int i;
	unsigned char key[4] = {0x87, 0x01, 0x55, 0x55};

	for (i = 0; i<4; i++)
		linux_outb(key[i], LPC_KEY_ADDR);
}

static void LPCExitMBPnP(void)
{
	linux_outb(0x02, LPC_KEY_ADDR);
	linux_outb(0x02, LPC_DATA_ADDR);
}

static void LPCSetConfig(char LdnNumber, char Index, char data)
{
	LPCEnterMBPnP();				// Enter IT8712 MB PnP mode
	linux_outb(0x07, LPC_KEY_ADDR);
	linux_outb(LdnNumber, LPC_DATA_ADDR);
	linux_outb(Index, LPC_KEY_ADDR);
	linux_outb(data, LPC_DATA_ADDR);
	LPCExitMBPnP();
}

static char LPCGetConfig(char LdnNumber, char Index)
{
	char rtn;

	LPCEnterMBPnP();				// Enter IT8712 MB PnP mode
	linux_outb(0x07, LPC_KEY_ADDR);
	linux_outb(LdnNumber, LPC_DATA_ADDR);
	linux_outb(Index, LPC_KEY_ADDR);
	rtn = linux_inb(LPC_DATA_ADDR);
	LPCExitMBPnP();
	return rtn;
}

static int SearchIT8712(void)
{
	unsigned char Id1, Id2;
	unsigned short Id;

	LPCEnterMBPnP();
	linux_outb(0x20, LPC_KEY_ADDR); /* chip id byte 1 */
	Id1 = linux_inb(LPC_DATA_ADDR);
	linux_outb(0x21, LPC_KEY_ADDR); /* chip id byte 2 */
	Id2 = linux_inb(LPC_DATA_ADDR);
	Id = (Id1 << 8) | Id2;
	LPCExitMBPnP();
	if (Id == 0x8712)
		return 1;
	else
		return 0;
}

static void InitLPCInterface(void)
{
	unsigned char bus, dev_fn;
	unsigned long data;

	bus = 0;
	dev_fn = 1<<3 | 4;


	/* pci cmd, SERR# Enable */
	IT_WRITE(IT_CONFADDR,
		 (bus         << IT_BUSNUM_SHF)   |
		 (dev_fn      << IT_FUNCNUM_SHF) |
		 ((0x4 / 4) << IT_REGNUM_SHF));
	IT_READ(IT_CONFDATA, data);
	data |= 0x0100;
	IT_WRITE(IT_CONFADDR,
		 (bus         << IT_BUSNUM_SHF)   |
		 (dev_fn      << IT_FUNCNUM_SHF) |
		 ((0x4 / 4) << IT_REGNUM_SHF));
	IT_WRITE(IT_CONFDATA, data);

	/* setup serial irq control register */
	IT_WRITE(IT_CONFADDR,
		 (bus         << IT_BUSNUM_SHF)   |
		 (dev_fn      << IT_FUNCNUM_SHF) |
		 ((0x48 / 4) << IT_REGNUM_SHF));
	IT_READ(IT_CONFDATA, data);
	data  = (data & 0xffff00ff) | 0xc400;
	IT_WRITE(IT_CONFADDR,
		 (bus         << IT_BUSNUM_SHF)   |
		 (dev_fn      << IT_FUNCNUM_SHF) |
		 ((0x48 / 4) << IT_REGNUM_SHF));
	IT_WRITE(IT_CONFDATA, data);


	/* Enable I/O Space Subtractive Decode */
	/* default 0x4C is 0x3f220000 */
	IT_WRITE(IT_CONFADDR,
		 (bus         << IT_BUSNUM_SHF)   |
		 (dev_fn      << IT_FUNCNUM_SHF) |
		 ((0x4C / 4) << IT_REGNUM_SHF));
	IT_WRITE(IT_CONFDATA, 0x3f2200f3);
}

/*
 * According to the ITE Special BIOS Note for waking up the
 * keyboard controller...
 */
static int init_8712_keyboard(void)
{
	unsigned int cmd_port = 0x64;
	unsigned int data_port = 0x60;
	unsigned char data;
	int i;

	linux_outb(0xaa, cmd_port); /* send self-test cmd */
	i = 0;
	while (!(linux_inb(cmd_port) & 0x1)) { /* wait output buffer full */
		i++;
		if (i > 0xffffff)
			return 0;
	}

	data = linux_inb(data_port);
	linux_outb(0xcb, cmd_port); /* set ps2 mode */
	while (linux_inb(cmd_port) & 0x2) { /* wait while input buffer full */
		i++;
		if (i > 0xffffff)
			return 0;
	}
	linux_outb(0x01, data_port);
	while (linux_inb(cmd_port) & 0x2) { /* wait while input buffer full */
		i++;
		if (i > 0xffffff)
			return 0;
	}

	linux_outb(0x60, cmd_port); /* write 8042 command byte */
	while (linux_inb(cmd_port) & 0x2) { /* wait while input buffer full */
		i++;
		if (i > 0xffffff)
			return 0;
	}
	linux_outb(0x45, data_port); /* at interface, keyboard enabled, system flag */
	while (linux_inb(cmd_port) & 0x2) { /* wait while input buffer full */
		i++;
		if (i > 0xffffff)
			return 0;
	}

	linux_outb(0xae, cmd_port); /* enable interface */
	return 1;
}

static int init_8712(void)
{
  	printf("8712 initializing..\n");
	InitLPCInterface();

  	printf("LPC initialized\n");

	if (!SearchIT8712()) {
	  printf("8712 not found!\n");
	  return 0;
	}

	// enable IT8712 serial port
	LPCSetConfig(LDN_SERIAL1, 0x30, 0x01); /* enable */
	LPCSetConfig(LDN_SERIAL1, 0x23, 0x01); /* clock selection */
	if (!init_8712_keyboard()) {
	  printf("Unable to initialize keyboard!\n");
	  return 0;
	}
	LPCSetConfig(LDN_KEYBOARD, 0x30, 0x1);
	LPCSetConfig(LDN_KEYBOARD, 0xf0, 0x2);
	LPCSetConfig(LDN_KEYBOARD, 0x71, 0x3);
	LPCSetConfig(LDN_MOUSE, 0x30, 0x1); /* enable mouse */
	if ((LPCGetConfig(LDN_KEYBOARD, 0x30) == 0) ||
	    (LPCGetConfig(LDN_MOUSE, 0x30) == 0))
	  printf("Error: keyboard or mouse not enabled\n");

	return 1;
}

void
tgt_devconfig()
{
#if NMOD_VGACON >0
	int rc;
#endif
	_pci_devinit(1);	/* PCI device initialization */
	SBD_DISPLAY("VGAI", 0);
#if (NMOD_X86EMU_INT10 > 0) || (NMOD_X86EMU >0)
	vga_bios_init();
#endif

#ifdef CONFIG_VGA_CARD_SIS6326
	if (vga_dev != NULL) {
	  SBD_DISPLAY("FRBI", 0);
	  fbaddress  =_pci_conf_read(vga_dev->pa.pa_tag,0x10);
	  ioaddress  =_pci_conf_read(vga_dev->pa.pa_tag,0x18);

	  fbaddress = fbaddress &0xffffff00; //laster 8 bit
	  ioaddress = ioaddress &0xfffffff0; //laster 4 bit

	  sis6326_init(fbaddress, ioaddress);
	  fb_init();
	  fb_available=1;
	}
#endif	
        config_init();
        configure();
#if NMOD_VGACON >0
	init_8712();
	rc=kbd_initialize();
	printf("%s\n",kbd_error_msgs[rc]);
	if(!rc){ 
		kbd_available=1;
	}
#endif
}
#define tgt_putchar_uc(x) (*(void (*)(char)) (((long)tgt_putchar)|0x20000000)) (x)

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
	
       SBD_DISPLAY("CACH",0);
       CPU_ConfigCache();
       SBD_DISPLAY("CAC2",0);
       SBD_DISPLAY("CAC3",0);

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
#if 0
	printf("Board revision level: %c.\n", in8(PLD_BAR) + 'A');
	printf("PLD revision levels: %d.%d and %d.%d.\n",
					in8(PLD_ID1) >> 4, in8(PLD_ID1) & 15,
					in8(PLD_ID2) >> 4, in8(PLD_ID2) & 15);
#endif
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

#define IT8172_RTC_ADR_REG  (IT8172_PCI_IO_BASE + IT_RTC_BASE)
#define IT8172_RTC_DAT_REG  (IT8172_RTC_ADR_REG + 1)

/* Local Variables */
static volatile char *rtc_adr_reg = (volatile char*)
        KSEG1ADDR((volatile char *)IT8172_RTC_ADR_REG);
static volatile char *rtc_dat_reg = (volatile char*)
        KSEG1ADDR((volatile char *)IT8172_RTC_DAT_REG);
 
static inline unsigned char CMOS_READ (unsigned char addr)
{
	unsigned char retval;

 	*rtc_adr_reg = addr;
	 retval =  *rtc_dat_reg;
	 return retval;
}

void
CMOS_WRITE (unsigned char data, unsigned char addr)
{
	*rtc_adr_reg = addr;
	*rtc_dat_reg = data;
}

/* Pending */
#define BCD_TO_BIN(val) ((val)=((val)&15) + ((val)>>4)*10)
static void init_it8172_rtc(void)
{
	int year, month, date, hour, min, sec;
	CMOS_WRITE(0x20, RTC_REG_A);/* Normal operation */
	CMOS_WRITE(RTCSB_24HR /*| RTC_DM_BINARY*/ | RTC_SET, RTC_REG_B);
	CMOS_WRITE(0, RTC_REG_C);
	CMOS_WRITE(0, RTC_REG_D);
	year = CMOS_READ(RTC_YEAR);
	month = CMOS_READ(RTC_MONTH);
	date = CMOS_READ(RTC_DAY_OF_MONTH);
	hour = CMOS_READ(RTC_HOURS);
	min = CMOS_READ(RTC_MINUTES);
	sec = CMOS_READ(RTC_SECONDS);
	if( (year > 99) || (month < 1 || month > 12) || 
		(date < 1 || date > 31) || (hour > 23) || (min > 59) ||
		(sec > 59) ){
		/*
		printf("RTC time invalid, reset to epoch.\n");*/
		CMOS_WRITE(3, RTC_YEAR);
		CMOS_WRITE(1, RTC_MONTH);
		CMOS_WRITE(1, RTC_DAY_OF_MONTH);
		CMOS_WRITE(0, RTC_HOURS);
		CMOS_WRITE(0, RTC_MINUTES);
		CMOS_WRITE(0, RTC_SECONDS);
	}
	CMOS_WRITE(RTCSB_24HR /*| RTC_DM_BINARY*/, RTC_REG_B);
}

/* Pending */
static unsigned int cal_r4koff(void)
{
	unsigned long count;

	CMOS_WRITE(0x20, RTC_REG_A);/* Normal operation */
	
	/* Start counter exactly on falling edge of update flag */
	while (CMOS_READ(RTC_REG_A) & RTC_UIP);
	while (!(CMOS_READ(RTC_REG_A) & RTC_UIP));
	
	/* Start r4k counter. */
	CPU_SetCOUNT(0);
	
 	/* Read counter exactly on falling edge of update flag */
	while (CMOS_READ(RTC_REG_A) & RTC_UIP);
	while (!(CMOS_READ(RTC_REG_A) & RTC_UIP));
	
	count = CPU_GetCOUNT();
	
	return (count / 100);
}


/* Pending */
static void
_probe_frequencies()
{
	 unsigned long r4k_offset;
	 unsigned long est_freq;
	 clk_invalid = 1;
	 init_it8172_rtc();
	 r4k_offset = cal_r4koff();
	 est_freq = 2*r4k_offset*100;
	 est_freq += 5000;    /* round */
         est_freq -= est_freq%10000;
	 md_pipefreq = est_freq*3/2;
	 md_cpufreq = est_freq;
	 printf("CPU frequency %d.%02d MHz\n", est_freq/1000000,
	          (est_freq%1000000)*100/1000000);
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


static inline time_t
_mktime (unsigned int year, unsigned int mon,
        unsigned int day, unsigned int hour,
        unsigned int min, unsigned int sec)
{
	if (0 >= (int) (mon -= 2)) {        /* 1..12 -> 11,12,1..10 */
		mon += 12;              /* Puts Feb last since it has leap day */
		year -= 1;
	}

	return ((((unsigned long) (year/4 - year/100 + year/400 + 367*mon/12 + day) +
	         year*365 - 719499)*24 + hour /* now have hours */
	         )*60 + min /* now have minutes */
		 )*60 + sec; /* finally seconds */
}

time_t
tgt_gettime()
{
	unsigned int year, mon, day, hour, min, sec;
	unsigned char save_control;
        //CMOS_WRITE(CMOS_READ(RTC_CONTROL) | RTC_DM_BINARY, RTC_CONTROL);
	save_control = CMOS_READ(RTC_CONTROL);
	
        /* Freeze it. */
        CMOS_WRITE(save_control | RTC_SET, RTC_CONTROL);

	/* Read regs. */
	sec = CMOS_READ(RTC_SECONDS);
	min = CMOS_READ(RTC_MINUTES);
	hour = CMOS_READ(RTC_HOURS);
	
        if (!(save_control & RTC_24H))
	{
		if ((hour & 0xf) == 0xc)
			hour &= 0x80;
		if (hour & 0x80)
			hour = (hour & 0xf) + 12;
	}
	day = CMOS_READ(RTC_DAY_OF_MONTH);
	mon = CMOS_READ(RTC_MONTH);
	year = CMOS_READ(RTC_YEAR);

	/* Unfreeze clock. */
	CMOS_WRITE(save_control, RTC_CONTROL);
	if ((year += 1900) < 1970)
	year += 100;

	return _mktime(year, mon, day, hour, min, sec);
								
}

/*
 *  Set the current time if a TOD clock is present
 */
void
tgt_settime(time_t t)
{
#if 0
	struct tm *tm;
	int ctrlbsave;

	if(!clk_invalid) {
		tm = gmtime(&t);
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
	printf("Copyright 2004-2005, Opsycon AB, Sweden.\n");
	printf("Copyright 2005, Liangjin Peng, ICT CAS.\n");
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
#if 0
	if(in8(PLD_BSTAT) & 0x40) {
		return(1);
	}
	else {
		return(1);	/* can't enable. jumper selected! */
	}
#endif
	return 1;
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
#ifdef NVRAM_IN_FLASH
        nvram = (char *)(tgt_flashmap())->fl_map_base;
	if(fl_devident(nvram, NULL) == 0 ||
           cksum(nvram + NVRAM_OFFS, NVRAM_SIZE, 0) != 0) {
#else
        nvram = (char *)malloc(NVRAM_SIZE);
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

//	printf("NVRAM@%x\n",(u_int32_t)nvram);

	/*
	 *  Ethernet address for Galileo ethernet is stored in the last
	 *  six bytes of nvram storage. Set environment to it.
	 */
	bcopy(&nvram[ETHER_OFFS], hwethadr, 6);
	sprintf(env, "%02x:%02x:%02x:%02x:%02x:%02x", hwethadr[0], hwethadr[1],
	    hwethadr[2], hwethadr[3], hwethadr[4], hwethadr[5]);
	tgt_printf("ethaddr=%s\n", env);
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

	sprintf(env, "%d", CpuTertiaryCacheSize / (1024*1024));
	(*func)("l3cache", env);

	sprintf(env, "%x", IT8172_BASE);
	(*func)("gtbase", env);
	
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
        nvram = (char *)(tgt_flashmap())->fl_map_base;

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
                        if(fl_erase_device(nvram, NVRAM_SECSIZE, FALSE)) {
                                status = -1;
				break;
                        }

                        if(fl_program_device(nvram, nvramsecbuf, NVRAM_SECSIZE, FALSE)) {
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
                if(fl_erase_device(nvram, NVRAM_SECSIZE, FALSE)) {
			printf("Error! Nvram erase failed!\n");
			free(nvramsecbuf);
                        return(-1);
                }
                if(fl_program_device(nvram, nvramsecbuf, NVRAM_SECSIZE, FALSE)) {
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
        cksum(nvrambuf, NVRAM_SIZE, 1);

	bcopy(hwethadr, &nvramsecbuf[ETHER_OFFS], 6);
#ifdef NVRAM_IN_FLASH
        if(fl_erase_device(nvram, NVRAM_SECSIZE, FALSE)) {
		printf("Error! Nvram erase failed!\n");
		free(nvramsecbuf);
                return(0);
        }
        if(fl_program_device(nvram, nvramsecbuf, NVRAM_SECSIZE, FALSE)) {
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
//	return(sum);
	return 0;
}

#ifndef NVRAM_IN_FLASH

void write_rtc_ram(unsigned int data, unsigned short idx)
{
	*(volatile unsigned char *)(idx + (RTC_BASE)) = data;
}

char read_rtc_ram(unsigned short idx)
{
	return *(volatile char *)(idx + (RTC_BASE));
}
/*
 *  Read and write data into non volatile memory in clock chip.
 */
void
nvram_get(char *buffer)
{
	int i;
//	printf("nvram_get:\n");
	for(i = 0; i < 114; i++) {
		write_rtc_ram(i + RTC_NVRAM_BASE, RTC_INDEX_REG_BANK1);	/* Address */
		buffer[i] = read_rtc_ram(RTC_DATA_REG_BANK1);
	}
/*	for(i = 0; i < 114; i++) {
		printf("%x ", buffer[i]);
	}
	printf("\n");*/
}

void
nvram_put(char *buffer)
{
	int i;
	printf("nvram_put:\n");
	for(i = 0; i < 114; i++) {
		write_rtc_ram(i+RTC_NVRAM_BASE, RTC_INDEX_REG_BANK1);	/* Address */
		write_rtc_ram(buffer[i],RTC_DATA_REG_BANK1);
	}
	for(i = 0; i < 114; i++) {
		printf("%x ", buffer[i]);
	}
	printf("\n");
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
#endif
