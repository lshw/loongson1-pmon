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
#include "target/firewall.h"

#include <pmon.h>

#include "mod_x86emu_int10.h"
#include "mod_x86emu.h"
#include "mod_vgacon.h"
#include "mod_framebuffer.h"
#include "mod_smi712.h"
#include "mod_smi502.h"
#include "mod_sisfb.h"
#if PCI_IDSEL_CS5536 != 0
#include <include/cs5536.h>
#endif
#if (NMOD_X86EMU_INT10 > 0)||(NMOD_X86EMU >0)
extern int vga_bios_init(void);
#endif
extern int radeon_init(void);
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

#if (NMOD_X86EMU_INT10 == 0)&&(NMOD_X86EMU == 0)
int vga_available=0;
#elif defined(VGAROM_IN_BIOS)
#include "vgarom.c"
#endif

int tgt_i2cread(int type,unsigned char *addr,int addrlen,unsigned char* buf ,int count);
int tgt_i2cwrite(int type,unsigned char *addr,int addrlen,unsigned char* buf ,int count);
extern struct trapframe DBGREG;
extern void *memset(void *, int, size_t);

int kbd_available;
int usb_kbd_available;;
int vga_available;

static int md_pipefreq = 0;
static int md_cpufreq = 0;
int clk_invalid = 0;
static int nvram_invalid = 0;
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
void __attribute__((weak))  init_legacy_rtc(void);

#ifdef USE_GPIO_SERIAL
//modified by zhanghualiang 
//this function is for parallel port to send and received  data
//via GPIO
//GPIO bit 2 for Receive clk   clk hight for request
//GPIO bit 3 for receive data 
#define DBG     
#define WAITSTART 10
#define START 20
#define HIBEGCLK 30
#define HINEXTCLK 40
#define LOWCLK 50
#define RXERROR 60
//static int zhlflg = 0;
static int
ppns16550 (int op, struct DevEntry *dev, unsigned long param, int data)
{
	volatile ns16550dev *dp;  
	static int zhlflg = 0;
	int     crev,crevp,dat,datap,clk,clkp; //zhl begin
	int 	cnt = 0;
	int 	tick = 0;
	int	STAT = START;
	int	tmp;
	char    cget = 0;
	crev = 0;
	crevp = 0;
	dat = 0;
	datap = 0;	
	clk = 0;
	clkp = 0;	
	cget = 0;                                         //zhl end
	
	dp = (ns16550dev *) dev->sio;
	if(dp==-1)return 1;
	switch (op) {
		case OP_INIT:
			return 1;

		case OP_XBAUD:
		case OP_BAUD:
			return 0;

		case OP_TXRDY:
			return 1;

		case OP_TX:
		tgt_putchar(data);
			break;

		case OP_RXRDY:
		tmp = *(volatile int *)(0xbfe0011c);
	//	tgt_printf("tmp=(%x)H, %d  ",tmp,tmp);
		tmp = tmp >> 18;
		tmp = tmp & 1;
		if (tmp)
		{
	//	tgt_printf("pmon pmon pom pmon pomn pmon get signal ");
	//	tgt_putchar('O');
	//	tgt_putchar('K');
		tgt_putchar(0x80);
	//	tgt_printf("\n");
		}
		return (tmp);
                
		case OP_RX:
		cget = 0;
		cnt = 0;
		tick = 0;
		while (cnt < 8)
		{
			crevp = crev;
			tmp = *(volatile int *)(0xbfe0011c);
                 //     tgt_printf("pmon tmp=%x,\n",tmp);
			tmp = tmp >> 18;
	 		tmp = tmp & 3;
			crev = tmp;
		     
			clkp = clk;
			clk = crev & 1;
			datap = dat;
			dat = crev & 2;
			dat = dat >> 1;
			dat = dat & 1;
		//	if (clkp != clk)
		//	{
			tick++;
			if(tick>5000000)
			{	
		//	tgt_putchar(0x80);
			return 10;	
			}
	//		tgt_printf("%d,%d\n",clk,dat);
	//		}       
	//		continue;
			switch(STAT)
			{
			case START:
			if (clk == 0)
			{
				STAT = LOWCLK;		
			}
			break;

			case LOWCLK:
			if(clk == 1)
			{	
				STAT = HIBEGCLK;
			}
			break;
			
			case HIBEGCLK:
			dat = dat << cnt;
			cget = cget | dat;
			cnt++;
			STAT = HINEXTCLK;
	//		tgt_printf("p c=%d, d=%d,\n",cnt,dat);
			if (cnt == 8 )
			{
			do {
			tmp = *(volatile int *)(0xbfe0011c);
			tmp = tmp >> 18;
	 		tmp = tmp & 1;
		     	}
			while(tmp);
	/*	{	sl_tgt_putchar('p');
			sl_tgt_putchar(' ');
			sl_tgt_putchar('c');
			sl_tgt_putchar('=');
			sl_tgt_putchar(cget);
			sl_tgt_putchar('\n');
		}	*/
	//		tgt_printf(" ch = (%x)h,%d\n",cget,cget);
			return cget;
			}
			break;
			
			case HINEXTCLK:
			if (clk == 0)
			STAT = LOWCLK;
			break;
			
			case RXERROR:
			break;
			}
		   }
					
	

		case OP_RXSTOP:
		return (0);
			break;
	}
	return 0;
}
//end modification by zhanghualiang
#endif



ConfigEntry	ConfigTable[] =
{
#ifdef HAVE_NB_SERIAL
	#ifdef DEVBD2F_FIREWALL
	 { (char *)LS2F_COMA_ADDR, 0, ns16550, 256, CONS_BAUD, NS16550HZ/2 },
	#else
	 { (char *)COM3_BASE_ADDR, 0, ns16550, 256, CONS_BAUD, NS16550HZ },
	#endif
#elif defined(USE_SM502_UART0)
	{ (char *)0xb6030000, 0, ns16550, 256, CONS_BAUD, NS16550HZ/2 },
#elif defined(USE_GPIO_SERIAL)
	 { (char *)COM1_BASE_ADDR, 0,ppns16550, 256, CONS_BAUD, NS16550HZ }, 
#else
	 { (char *)COM1_BASE_ADDR, 0, ns16550, 256, CONS_BAUD, NS16550HZ/2 }, 
#endif
#if NMOD_VGACON >0
	#if NMOD_FRAMEBUFFER >0
	{ (char *)1, 0, fbterm, 256, CONS_BAUD, NS16550HZ },
	#else
	{ (char *)1, 0, vgaterm, 256, CONS_BAUD, NS16550HZ },
	#endif
#endif
#ifdef DEVBD2F_FIREWALL
	 { (char *)LS2F_COMB_ADDR, 0, ns16550, 256, CONS_BAUD, NS16550HZ/2 ,1},
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

pcireg_t _pci_allocate_io(struct pci_device *dev, vm_size_t size);
static void superio_reinit();

void
initmips(unsigned int memsz)
{
tgt_fpuenable();
#if defined(DEVBD2F_SM502) && (!defined(DEVBD2F_SM502_GOLDING) && !defined(DEVBD2F_SM502_GWI))
{
/*set lio bus to 16 bit*/
volatile int *p=0xbfe00108;
*p=((*p)&~(0x1f<<8))|(0x8<<8) |(1<<13);
}
#endif
tgt_printf("memsz %d\n",memsz);
/*enable float*/
tgt_fpuenable();
CPU_TLBClear();

#if PCI_IDSEL_CS5536 != 0
superio_reinit();
#endif
	/*
	 *	Set up memory address decoders to map entire memory.
	 *	But first move away bootrom map to high memory.
	 */
#if 0
	GT_WRITE(BOOTCS_LOW_DECODE_ADDRESS, BOOT_BASE >> 20);
	GT_WRITE(BOOTCS_HIGH_DECODE_ADDRESS, (BOOT_BASE - 1 + BOOT_SIZE) >> 20);
#endif
	memorysize = memsz > 256 ? 256 << 20 : memsz << 20;
	memorysize_high = memsz > 256 ? (memsz - 256) << 20 : 0;

#if 0
asm(\
"	 sd %1,0x18(%0);;\n" \
"	 sd %2,0x28(%0);;\n" \
"	 sd %3,0x20(%0);;\n" \
	 ::"r"(0xffffffffbff00000ULL),"r"(memorysize),"r"(memorysize_high),"r"(0x20000000)
	 :"$2"
   );
#endif

#if 0
	{
	  int start = 0x80000000;
	  int end = 0x80000000 + 16384;

	  while (start < end) {
	    __asm__ volatile (" cache 1,0(%0)\r\n"
		              " cache 1,1(%0)\r\n"
		              " cache 1,2(%0)\r\n"
		              " cache 1,3(%0)\r\n"
		              " cache 0,0(%0)\r\n"::"r"(start));
	    start += 32;
	  }

	  __asm__ volatile ( " mfc0 $2,$16\r\n"
	                   " and  $2, $2, 0xfffffff8\r\n"
			   " or   $2, $2, 2\r\n"
			   " mtc0 $2, $16\r\n" :::"$2");
	}
#endif

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

#ifndef ROM_EXCEPTION
	CPU_SetSR(0, SR_BOOT_EXC_VEC);
#endif
	SBD_DISPLAY("BEV0",0);
	
	printf("BEV in SR set to zero.\n");

	

#if PCI_IDSEL_VIA686B
if(getenv("powermg"))
{
pcitag_t mytag;
unsigned char data;
unsigned int addr;
	mytag=_pci_make_tag(0,PCI_IDSEL_VIA686B,4);
	data=_pci_conf_readn(mytag,0x41,1);
	_pci_conf_writen(mytag,0x41,data|0x80,1);
	addr=_pci_allocate_io(_pci_head,256);
	printf("power management addr=%x\n",addr);
	_pci_conf_writen(mytag,0x48,addr|1,4);
}
#endif

#ifdef DEVBD2F_SM502_GWI
gwi_nand_nand_init();
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
int psaux_init(void);
extern int video_hw_init (void);

extern int fb_init(unsigned long,unsigned long);
void
tgt_devconfig()
{
#if NMOD_VGACON > 0
	int rc=0;
#if NMOD_FRAMEBUFFER > 0 
	unsigned long fbaddress,ioaddress;
	extern struct pci_device *vga_dev;
#endif
#endif
	_pci_devinit(1);	/* PCI device initialization */
#if (NMOD_X86EMU_INT10 > 0)||(NMOD_X86EMU >0)
	SBD_DISPLAY("VGAI", 0);
	rc = vga_bios_init();
#endif
#if (NMOD_X86EMU_INT10 == 0 && defined(RADEON7000))
	SBD_DISPLAY("VGAI", 0);
	rc = radeon_init();
#endif
#if NMOD_FRAMEBUFFER > 0
	vga_available=0;
	if(!vga_dev) {
		printf("ERROR !!! VGA device is not found\n"); 
		rc = -1;
	}
	if (rc >= 0) {
		rc=1;
		fbaddress  =_pci_conf_read(vga_dev->pa.pa_tag,0x10);
		ioaddress  =_pci_conf_read(vga_dev->pa.pa_tag,0x18);

		fbaddress = fbaddress &0xffffff00; //laster 8 bit
		ioaddress = ioaddress &0xfffffff0; //laster 4 bit

#if NMOD_SISFB
		fbaddress=sisfb_init_module();
#endif
		printf("fbaddress 0x%x\tioaddress 0x%x\n",fbaddress, ioaddress);

#if NMOD_SMI712 > 0
		fbaddress |= 0xb0000000;
		ioaddress |= 0xbfd00000;
        smi712_init((unsigned char *)fbaddress,(unsigned char *)ioaddress);
#endif

#if NMOD_SMI502 > 0
        rc = video_hw_init ();
                fbaddress  =_pci_conf_read(vga_dev->pa.pa_tag,0x10);
                ioaddress  =_pci_conf_read(vga_dev->pa.pa_tag,0x14);
                fbaddress |= 0xb0000000;
                ioaddress |= 0xb0000000;
#endif
		printf("begin fb_init\n");
		fb_init(fbaddress, ioaddress);
		printf("after fb_init\n");

	} else {
		printf("vga bios init failed, rc=%d\n",rc);
	}
#endif

#if (NMOD_FRAMEBUFFER > 0) || (NMOD_VGACON > 0 )
    if (rc > 0)
	 if(!getenv("novga")) vga_available=1;
	 else vga_available=0;
#endif
    config_init();
    configure();
#if ((NMOD_VGACON >0) &&((PCI_IDSEL_VIA686B !=0)|| (PCI_IDSEL_CS5536 !=0)))
	if(getenv("nokbd")) rc=1;
	else rc=kbd_initialize();
	printf("%s\n",kbd_error_msgs[rc]);
	if(!rc){ 
		kbd_available=1;
	}
//	psaux_init();
#endif
   printf("devconfig done.\n");
}

extern int test_icache_1(short *addr);
extern int test_icache_2(int addr);
extern int test_icache_3(int addr);
extern void godson1_cache_flush(void);
#define tgt_putchar_uc(x) (*(void (*)(char)) (((long)tgt_putchar)|0x20000000)) (x)

extern void cs5536_gpio_init(void);
extern void test_gpio_function(void);
extern void cs5536_pci_fixup(void);

#if PCI_IDSEL_CS5536 != 0
static int w83627_read(int dev,int addr)
{
int data;
/*enter*/
outb(0xbfd0002e,0x87);
outb(0xbfd0002e,0x87);
/*select logic dev reg */
outb(0xbfd0002e,0x7);
outb(0xbfd0002f,dev);
/*access reg */
outb(0xbfd0002e,addr);
data=inb(0xbfd0002f);
/*exit*/
outb(0xbfd0002e,0xaa);
outb(0xbfd0002e,0xaa);
return data;
}

static void w83627_write(int dev,int addr,int data)
{
/*enter*/
outb(0xbfd0002e,0x87);
outb(0xbfd0002e,0x87);
/*select logic dev reg */
outb(0xbfd0002e,0x7);
outb(0xbfd0002f,dev);
/*access reg */
outb(0xbfd0002e,addr);
outb(0xbfd0002f,data);
/*exit*/
outb(0xbfd0002e,0xaa);
outb(0xbfd0002e,0xaa);
}
#endif

#if PCI_IDSEL_CS5536 != 0
static void superio_reinit()
{
w83627_write(0,0x24,0xc1);
w83627_write(5,0x30,1);
w83627_write(5,0x60,0);
w83627_write(5,0x61,0x60);
w83627_write(5,0x62,0);
w83627_write(5,0x63,0x64);
w83627_write(5,0x70,1);
w83627_write(5,0x72,0xc);
w83627_write(5,0xf0,0x80);
_wrmsr(GET_MSR_ADDR(0x5140001F), 0, 0);//no keyboard emulation

#ifdef USE_CS5536_UART
//w83627_UART1
w83627_write(2,0x30,0x01);
w83627_write(2,0x60,0x03);
w83627_write(2,0x61,0xe8);
w83627_write(2,0x70,0x04);
w83627_write(2,0xf0,0x00);

//w83627_UART2
w83627_write(3,0x30,0x01);
w83627_write(3,0x60,0x02);
w83627_write(3,0x61,0xe8);
w83627_write(3,0x70,0x03);
w83627_write(3,0xf0,0x00);
#else
//w83627_UART1
w83627_write(2,0x30,0x01);
w83627_write(2,0x60,0x03);
w83627_write(2,0x61,0xf8);
w83627_write(2,0x70,0x04);
w83627_write(2,0xf0,0x00);

//w83627_UART2
w83627_write(3,0x30,0x01);
w83627_write(3,0x60,0x02);
w83627_write(3,0x61,0xf8);
w83627_write(3,0x70,0x03);
w83627_write(3,0xf0,0x00);
#endif

////w83627_PALLPort
w83627_write(1,0x30,0x01);
w83627_write(1,0x60,0x03);
w83627_write(1,0x61,0x78);
w83627_write(1,0x70,0x07);
w83627_write(1,0x74,0x04);
w83627_write(1,0xf0,0xF0); 
ConfigTable[0].flag=1;//reinit serial
}
#endif

void
tgt_devinit()
{
#if  (PCI_IDSEL_VIA686B != 0)
	SBD_DISPLAY("686I",0);
	
	vt82c686_init();
#endif

#if  (PCI_IDSEL_CS5536 != 0)
	SBD_DISPLAY("5536",0);
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
#if defined(VIA686B_POWERFIXUP) && (PCI_IDSEL_VIA686B != 0)
if(getenv("noautopower"))	vt82c686_powerfixup();
#endif

#if  (PCI_IDSEL_CS5536 != 0)
	cs5536_pci_fixup();
#endif
}


#ifdef DEVBD2F_CS5536
void
tgt_reboot()
{
	unsigned long hi, lo;
	
	/* reset the cs5536 whole chip */
	_rdmsr(0xe0000014, &hi, &lo);
	lo |= 0x00000001;
	_wrmsr(0xe0000014, hi, lo);

	while(1);
}

void
tgt_poweroff()
{
	unsigned long val;
	unsigned long tag;
	unsigned long base;

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

	while(1);
}
#else
void
tgt_poweroff()
{
	volatile int *p=0xbfe0011c;
	p[1]&=~1;
	p[0]&=~1;
	p[0]|=1;
}
#endif


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

void __attribute__((weak))  init_legacy_rtc(void)
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
                
                tgt_printf("RTC time invalid, reset to epoch.\n");
#ifdef DEVBD2F_FIREWALL
	{

		struct tm tm;
		time_t t;
		clk_invalid = 0;
		tm.tm_sec = 0;
		tm.tm_min = 0;
		tm.tm_hour = 0;
		tm.tm_mday = 1;
		tm.tm_mon = 0;
		tm.tm_year = 108;
		t = mktime(&tm);
		tgt_settime(t);
		clk_invalid = 1;
	}
#else
                CMOS_WRITE(3, DS_REG_YEAR);
                CMOS_WRITE(1, DS_REG_MONTH);
                CMOS_WRITE(1, DS_REG_DATE);
                CMOS_WRITE(0, DS_REG_HOUR);
                CMOS_WRITE(0, DS_REG_MIN);
                CMOS_WRITE(0, DS_REG_SEC);
#endif
        }

        CMOS_WRITE(DS_CTLB_24 | DS_CTLB_DM, DS_REG_CTLB);
                                                                               
#if (PCI_IDSEL_CS5536 != 0)
	if(!(CMOS_READ(0xd) & 0x80)) tgt_printf("\nrtc battery voltage too low!!!\n");
#endif
	//printf("RTC: %02d-%02d-%02d %02d:%02d:%02d\n",
	//    year, month, date, hour, min, sec);
}

int word_addr;

static inline unsigned char CMOS_READ(unsigned char addr)
{
        unsigned char val;
        unsigned char tmp1,tmp2;
	volatile int tmp;
	
#if defined(DEVBD2F_VIA)||defined(DEVBD2F_CS5536)||defined(DEVBD2F_EVA)
	linux_outb_p(addr, 0x70);
        val = linux_inb_p(0x71);
#elif defined(DEVBD2F_SM502)

	pcitag_t tag;
	unsigned char value;
	char i2caddr[]={(unsigned char)0x64, 0};
	if(addr >= 0x0a)
		return 0;
	switch(addr)
	{
		case 0:
			break;
		case 2:
			addr = 1;
			break;
		case 4:
			addr = 2;
			break;
		case 6:
			addr = 3;
			break;
		case 7:
			addr = 4;
			break;
		case 8:
			addr = 5;
			break;
		case 9:
			addr = 6;
			break;
		
	}
		
	i2caddr[1] = 0xe<<4;
	tgt_i2cread(I2C_SINGLE,i2caddr,2,&value,1);
		value = value|0x20;

	i2caddr[1] = 0xe<<4;
	tgt_i2cwrite(I2C_SINGLE,i2caddr,2,&value,1);
	i2caddr[1] = addr<<4;
	tgt_i2cread(I2C_SINGLE,i2caddr,2,&val,1);
	tmp1 = ((val>>4)&0x0f)*10;
	tmp2  = val&0x0f;
	val = tmp1 + tmp2;
	
#elif defined(DEVBD2F_FIREWALL)
	char i2caddr[]={0xde,0, 0};
	if(addr >= 0x0a)
		return 0;
	switch(addr)
	{
		case 0:
			addr= 0x30;
			break;
		case 2:
			addr = 0x31;
			break;
		case 4:
			addr = 0x32;
			break;
		case 6:
			addr = 0x36;
			break;
		case 7:
			addr = 0x33;
			break;
		case 8:
			addr = 0x34;
			break;
		case 9:
			addr = 0x35;
			break;
		
	}
#ifndef GPIO_I2C
	//atp8620_i2c_read(0xde,addr,&tmp,1);
#else	
	i2caddr[2] = addr;
	tgt_i2cread(I2C_SINGLE,i2caddr,3,&tmp,1);
#endif
	if(addr == 0x32)
		tmp = tmp&0x7f;
	val = ((tmp>>4)&0x0f)*10;
	val = val + (tmp&0x0f);
#endif

        return val;
}
                                                                               
static inline void CMOS_WRITE(unsigned char val, unsigned char addr)
{

	char a;
#if defined(DEVBD2F_VIA)||defined(DEVBD2F_CS5536)||defined(DEVBD2F_EVA)
	linux_outb_p(addr, 0x70);
        linux_outb_p(val, 0x71);

#elif defined(DEVBD2F_SM502)
  
  	unsigned char tmp1,tmp2;
	volatile int tmp;
	char i2caddr[]={(unsigned char)0x64, 0};
	tmp1 = (val/10)<<4;
	tmp2  = (val%10);
	val = tmp1|tmp2;
	if(addr >=0x0a)
		return 0;
	switch(addr)
	{
		case 0:
			break;
		case 2:
			addr = 1;
			break;
		case 4:
			addr = 2;
			break;
		case 6:
			addr = 3;
			break;
		case 7:
			addr = 4;
			break;
		case 8:
			addr = 5;
			break;
		case 9:
			addr = 6;
			break;
		
	}
	{
		unsigned char value;
	
		i2caddr[1] = 0xe<<4;
		tgt_i2cread(I2C_SINGLE,i2caddr,2,&value,1);
		value = value|0x20;
		i2caddr[1] = 0xe<<4;
		tgt_i2cwrite(I2C_SINGLE,i2caddr,2,&value,1);
		i2caddr[1] = addr<<4;
		tgt_i2cwrite(I2C_SINGLE,i2caddr,2,&val,1);
	}

#elif defined(DEVBD2F_FIREWALL)
	unsigned char tmp;
	char i2caddr[]={0xde,0,0};
	if(addr >= 0x0a)
		return 0;
	switch(addr)
	{
		case 0:
			addr= 0x30;
			break;
		case 2:
			addr = 0x31;
			break;
		case 4:
			addr = 0x32;
			break;
		case 6:
			addr = 0x36;
			break;
		case 7:
			addr = 0x33;
			break;
		case 8:
			addr = 0x34;
			break;
		case 9:
			addr = 0x35;
			break;
		
	}
	
	tmp = (val/10)<<4;
	val = tmp|(val%10);
#ifndef GPIO_I2C
	atp8620_i2c_write(0xde,addr,&val,1);
#else	
	i2caddr[2] = 0x3f;
	a = 2;
	tgt_i2cwrite(I2C_SINGLE,i2caddr,3,&a,1);
	i2caddr[2] = 0x3f;
	a = 6;
	tgt_i2cwrite(I2C_SINGLE,i2caddr,3,&a,1);
	i2caddr[2] = addr;
	tgt_i2cwrite(I2C_SINGLE,i2caddr,3,&tmp,1);
#endif
#endif
}

int __attribute__((weak)) tgt_getsec()
{
	int sec;
	while(CMOS_READ(DS_REG_CTLA) & DS_CTLA_UIP);

	sec = CMOS_READ(DS_REG_SEC);
	return sec;
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
        md_pipefreq = 660000000;        /* NB FPGA*/
        md_cpufreq  =  60000000;
#endif

        clk_invalid = 1;
#ifdef HAVE_TOD
#ifdef DEVBD2F_FIREWALL 
{
	extern void __main();
	char tmp;
	char i2caddr[]={0xde, 0, 0};
	i2caddr[2] = 0x3f;
	tgt_i2cread(I2C_SINGLE,i2caddr,3,&tmp,1);
	/*
	 * bit0:Battery is run out ,please replace the rtc battery
	 * bit4:Rtc oscillator is no operating,please reset the machine
	 */
	tgt_printf("0x3f value  %x\n",tmp);
	init_legacy_rtc();
	i2caddr[2] = 0x14;
	tgt_i2cread(I2C_SINGLE,i2caddr,3,&tmp,1);
	tgt_printf("0x14 value  %x\n",tmp);
}
#else
        init_legacy_rtc();
#endif

        SBD_DISPLAY ("FREI", CHKPNT_FREQ);

        /*
         * Do the next twice for two reasons. First make sure we run from
         * cache. Second make sure synched on second update. (Pun intended!)
         */
aa:
        for(i = 2;  i != 0; i--) {
                cnt = CPU_GetCOUNT();
                timeout = 10000000;
		sec = tgt_getsec();
                                                                               
                do {
                        timeout--;
                        cur = tgt_getsec();
                } while(timeout != 0 && ((cur == sec)||(cur !=((sec+1)%60))) && (CPU_GetCOUNT() - cnt<0x30000000));
                cnt = CPU_GetCOUNT() - cnt;
                if(timeout == 0) {
			tgt_printf("time out!\n");
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
         tgt_printf("cpu fre %u\n",md_pipefreq);                                                                      
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
__attribute__((weak)) tgt_gettime()
{
        struct tm tm;
        int ctrlbsave;
        time_t t;

	/*gx 2005-01-17 */
	//return 0;
                                                                               
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

char gpio_i2c_settime(struct tm *tm);
                                                                               
/*
 *  Set the current time if a TOD clock is present
 */
void
 __attribute__((weak)) tgt_settime(time_t t)
{
        struct tm *tm;
        int ctrlbsave;

	//return ;
                                                                               
#ifdef HAVE_TOD
        if(!clk_invalid) {
                tm = gmtime(&t);
	#ifndef DEVBD2F_FIREWALL
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
		gpio_i2c_settime(tm);	
	#endif
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

	sprintf(env, "%d", memorysize_high / (1024 * 1024));
	(*func)("highmemsize", env);

	sprintf(env, "%d", md_pipefreq);
	(*func)("cpuclock", env);

	sprintf(env, "%d", md_cpufreq);
	(*func)("busclock", env);

	(*func)("systype", SYSTYPE);
	
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
#include "mycmd.c"

#ifdef DEVBD2F_FIREWALL
#include "i2c-sm502.c"
#elif defined(DEVBD2F_SM502)
#include "i2c-sm502.c"

#elif (PCI_IDSEL_CS5536 != 0)
#include "i2c-cs5536.c"
#else
#include "i2c-via.c"
#endif


