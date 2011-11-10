/*
 * (C) Copyright 2001
 * John Clemens <clemens@mclx.com>, Mission Critical Linux, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

 
/*
 * mpsc.c - driver for console over the MPSC.
 */
//mpsc_init2 is the initialization function
//
#ifndef USE_SUPERIO_UART
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

#ifdef USE_PIIX_RTC
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

#include "mod_x86emu.h"
#include "mod_x86emu_int10.h"
#include "mod_vgacon.h"
#define CFG_TCLK 100000000

#define MVREGREAD(offset)                                                  \
            (*((volatile unsigned int *)                          \
            (PHYS_TO_UNCACHED(GT_BASE_ADDR) | (offset))))
 
#define MV_REG_WRITE(offset, data)                                         \
            ((*((volatile unsigned int *)                                      \
            (PHYS_TO_UNCACHED(GT_BASE_ADDR) |                           \
            (offset)))) = (data))

#include "mpsc.h"

 
/* Define this if you wish to use the MPSC as a register based UART. 
 * This will force the serial port to not use the SDMA engine at all.
 */
#define CONFIG_MPSC_DEBUG_PORT


int (*mpsc_putchar)(char ch) = mpsc_putchar_early;
char (*mpsc_getchar)(void) = mpsc_getchar_debug;
int  (*mpsc_test_char)(void) = mpsc_test_char_debug;

static unsigned long	gd_baudrate;

static volatile unsigned int *rx_desc_base=NULL;
static unsigned int rx_desc_index=0;
static volatile unsigned int *tx_desc_base=NULL;
static unsigned int tx_desc_index=0;

/* local function declarations */
static int galmpsc_connect(int channel, int connect);
static int galmpsc_route_rx_clock(int channel, int brg);
static int galmpsc_route_tx_clock(int channel, int brg);
static int galmpsc_write_config_regs(int mpsc, int mode);
static int galmpsc_config_channel_regs(int mpsc);
static int galmpsc_set_char_length(int mpsc, int value);
static int galmpsc_set_stop_bit_length(int mpsc, int value);
static int galmpsc_set_parity(int mpsc, int value);
static int galmpsc_enter_hunt(int mpsc);
static int galmpsc_set_brkcnt(int mpsc, int value);
static int galmpsc_set_tcschar(int mpsc, int value);
static int galmpsc_shutdown(int mpsc);


static int galbrg_set_CDV(int channel, int value);
static int galbrg_enable(int channel);
static int galbrg_disable(int channel);
static int galbrg_set_clksrc(int channel, int value);
static int galbrg_set_CUV(int channel, int value);



#ifdef CONFIG_MPSC_DEBUG_PORT

#define udelay delay
#define NO_BIT          0x00000000
#define BIT0            0x00000001
#define BIT1            0x00000002
#define BIT2            0x00000004
#define BIT3            0x00000008
#define BIT4            0x00000010
#define BIT5            0x00000020
#define BIT6            0x00000040
#define BIT7            0x00000080
#define BIT8            0x00000100
#define BIT9            0x00000200
#define BIT10           0x00000400
#define BIT11           0x00000800
#define BIT12           0x00001000
#define BIT13           0x00002000
#define BIT14           0x00004000
#define BIT15           0x00008000
#define BIT16           0x00010000
#define BIT17           0x00020000
#define BIT18           0x00040000
#define BIT19           0x00080000
#define BIT20           0x00100000
#define BIT21           0x00200000
#define BIT22           0x00400000
#define BIT23           0x00800000
#define BIT24           0x01000000
#define BIT25           0x02000000
#define BIT26           0x04000000
#define BIT27           0x08000000
#define BIT28           0x10000000
#define BIT29           0x20000000
#define BIT30           0x40000000
#define BIT31           0x80000000


static void
mpsc_debug_init(void) {

	volatile unsigned int temp;
	
	/* Clear the CFR  (CHR4) */
	/* Write random 'Z' bit (bit 29) of CHR4 to enable debug uart *UNDOCUMENTED FEATURE* */ 
	temp = MVREGREAD(GALMPSC_CHANNELREG_4+(CHANNEL*GALMPSC_REG_GAP));
	temp &= 0xffffff00;
	temp |= BIT29;
	MV_REG_WRITE(GALMPSC_CHANNELREG_4+(CHANNEL*GALMPSC_REG_GAP), temp);

	/* Set the Valid bit 'V' (bit 12) and int generation bit 'INT' (bit 15) */
	temp = MVREGREAD(GALMPSC_CHANNELREG_5+(CHANNEL*GALMPSC_REG_GAP));
	temp |= (BIT12 | BIT15);
	MV_REG_WRITE(GALMPSC_CHANNELREG_5+(CHANNEL*GALMPSC_REG_GAP), temp);

	/* Set int mask */
	temp = MVREGREAD(GALMPSC_0_INT_MASK);
	temp |= BIT6;
	MV_REG_WRITE(GALMPSC_0_INT_MASK, temp);
}
#endif

char
mpsc_getchar_debug(void)
{
	volatile int temp;
	volatile unsigned int cause;

	cause = MVREGREAD(GALMPSC_0_INT_CAUSE);
	while ((cause & BIT6) == 0) {		
		cause = MVREGREAD(GALMPSC_0_INT_CAUSE);
	}

	temp = MVREGREAD(GALMPSC_CHANNELREG_10+(CHANNEL*GALMPSC_REG_GAP));
	/* By writing 1's to the set bits, the register is cleared */
	MV_REG_WRITE(GALMPSC_CHANNELREG_10+(CHANNEL*GALMPSC_REG_GAP), temp );
	MV_REG_WRITE(GALMPSC_0_INT_CAUSE, cause & ~BIT6);
	return (temp >> 16) & 0xff;
}

/* special function for running out of flash.  doesn't modify any
 * global variables [josh] */
int 
mpsc_putchar_early(char ch)
{
	int mpsc=CHANNEL;
	int temp=MVREGREAD(GALMPSC_CHANNELREG_2+(mpsc*GALMPSC_REG_GAP));
	galmpsc_set_tcschar(mpsc,ch);
	MV_REG_WRITE(GALMPSC_CHANNELREG_2+(mpsc*GALMPSC_REG_GAP), temp|0x200);

#define MAGIC_FACTOR	(10*1000000)

	udelay(MAGIC_FACTOR / gd_baudrate);
	return 0;
}

int
mpsc_test_char_debug(void)
{
	if  ((MVREGREAD(GALMPSC_0_INT_CAUSE) & BIT6) == 0 ) 
		return 0;
	else{
		return 1;
	}
}

int
mpsc_init(int baud)
{
	/* BRG CONFIG */
	galbrg_set_baudrate(CHANNEL, baud);
	galbrg_set_clksrc(CHANNEL,0);	/* set source=bclk_in */
	galbrg_set_CUV(CHANNEL, 0);	/* set up CountUpValue */
	galbrg_enable(CHANNEL);		/* Enable BRG */

	/* Set up clock routing */
	galmpsc_connect(CHANNEL, GALMPSC_CONNECT);	/* connect it */

	galmpsc_route_rx_clock(CHANNEL, CHANNEL);		/* chosse BRG0 for Rx */
	galmpsc_route_tx_clock(CHANNEL, CHANNEL);		/* chose BRG0 for Tx */

	/* reset MPSC state */
	//galmpsc_shutdown(CHANNEL);
	

	/* MPSC CONFIG */
	galmpsc_write_config_regs(CHANNEL, GALMPSC_UART);
	galmpsc_config_channel_regs(CHANNEL);
	galmpsc_set_char_length(CHANNEL, GALMPSC_CHAR_LENGTH_8);       /* 8 */
	galmpsc_set_parity(CHANNEL, GALMPSC_PARITY_NONE);              /* N */
	galmpsc_set_stop_bit_length(CHANNEL, GALMPSC_STOP_BITS_1);     /* 1 */

#ifdef CONFIG_MPSC_DEBUG_PORT
	mpsc_debug_init();
#endif
	return 0;
}


int
galbrg_set_baudrate(int channel, int rate) 
{
	int clock;

	galbrg_disable(channel);	/*ok*/

#ifdef ZUMA_NTL
	/* from tclk */
	clock = (CFG_TCLK/(16*rate)) - 1;
#else
	clock = (CFG_TCLK/(16*rate)) - 1;
#endif

	galbrg_set_CDV(channel, clock);	/* set timer Reg. for BRG*/

	galbrg_enable(channel);

	gd_baudrate = rate;

	return 0;
}

/* ------------------------------------------------------------------ */

/* Below are all the private functions that no one else needs */

static int
galbrg_set_CDV(int channel, int value)
{
	unsigned int temp;

	temp = MVREGREAD(GALBRG_0_CONFREG+(channel*GALBRG_REG_GAP));
	temp &= 0xFFFF0000;
	temp |= (value & 0x0000FFFF);
	MV_REG_WRITE(GALBRG_0_CONFREG+(channel*GALBRG_REG_GAP), temp);

	return 0;
}

static int
galbrg_enable(int channel)
{
	unsigned int temp;

	temp = MVREGREAD(GALBRG_0_CONFREG+(channel*GALBRG_REG_GAP));
	temp |= 0x00010000;
	MV_REG_WRITE(GALBRG_0_CONFREG+(channel*GALBRG_REG_GAP), temp);

	return 0;
}

static int
galbrg_disable(int channel)
{
	unsigned int temp;

	temp = MVREGREAD(GALBRG_0_CONFREG+(channel*GALBRG_REG_GAP));
	temp &= 0xFFFEFFFF;
	MV_REG_WRITE(GALBRG_0_CONFREG+(channel*GALBRG_REG_GAP), temp);

	return 0;
}

static int
galbrg_set_clksrc(int channel, int value)
{
	unsigned int temp;

	temp = MVREGREAD(GALBRG_0_CONFREG+(channel*GALBRG_REG_GAP));
	temp &= 0xFFC3FFFF;   /* Bit 18 - 21 (MV 64260 18-22)*/
	temp |= (value << 18); 
	MV_REG_WRITE(GALBRG_0_CONFREG+(channel*GALBRG_REG_GAP), temp);
	return 0;
}

static int
galbrg_set_CUV(int channel, int value)
{
	/* set CountUpValue*/
	/* MV_REG_WRITE(GALBRG_0_BTREG + (channel * GALBRG_REG_GAP), value); read-only register. */

	return 0;
}

#if 0
static int
galbrg_reset(int channel)
{
	unsigned int temp;

	temp = MVREGREAD(GALBRG_0_CONFREG + (channel * GALBRG_REG_GAP));
	temp |= 0x20000;
	MV_REG_WRITE(GALBRG_0_CONFREG + (channel * GALBRG_REG_GAP), temp);

	return 0;
}
#endif

static int
galmpsc_connect(int channel, int connect)
{
	unsigned int temp;

	temp = MVREGREAD(GALMPSC_ROUTING_REGISTER);

	if ((channel == 0) && connect) 
		temp &= ~0x00000007;
	else if ((channel == 1) && connect)
		temp &= ~(0x00000007 << 6);
	else if ((channel == 0) && !connect)
		temp |= 0x00000007;
	else
		temp |= (0x00000007 << 6);

	/* Just in case... */
	temp &= 0x3fffffff;

	MV_REG_WRITE(GALMPSC_ROUTING_REGISTER, temp);

	return 0;
}

static int
galmpsc_route_rx_clock(int channel, int brg)
{
	unsigned int temp;

	temp = MVREGREAD(GALMPSC_RxC_ROUTE);

	if (channel == 0) {
		temp &= ~0x0000000F;
		temp |= brg; }
	else	{
		temp &= ~0x00000F00;
		temp |= (brg << 8); }

	MV_REG_WRITE(GALMPSC_RxC_ROUTE,temp);

	return 0;
}

static int
galmpsc_route_tx_clock(int channel, int brg)
{
	unsigned int temp;

	temp = MVREGREAD(GALMPSC_TxC_ROUTE);

	if (channel == 0) {
		temp &= ~0x0000000F;
		temp |= brg; }
	else	{
		temp &= ~0x00000F00;
		temp |= (brg << 8); }

	MV_REG_WRITE(GALMPSC_TxC_ROUTE,temp);

	return 0;
}

static int
galmpsc_write_config_regs(int mpsc, int mode)
{
	if (mode == GALMPSC_UART) {
		/* Main config reg Low (Null modem, Enable Tx/Rx, UART mode) */
		MV_REG_WRITE(GALMPSC_MCONF_LOW + (mpsc*GALMPSC_REG_GAP),
			     0x000004c4);
		
		/* Main config reg High (32x Rx/Tx clock mode, width=8bits */
		MV_REG_WRITE(GALMPSC_MCONF_HIGH +(mpsc*GALMPSC_REG_GAP),
			     0x02400200 /*0x024003f8*/);
		/*        22 2222 1111*/
		/*        54 3210 9876*/
		/* 0000 0010 0000 0000*/
		/*       1*/
		/*       098 7654 3210*/
		/* 0000 0011 1111 1000*/
	} else 
		return -1;

	return 0;
}

static int
galmpsc_config_channel_regs(int mpsc)
{
	MV_REG_WRITE(GALMPSC_CHANNELREG_1+(mpsc*GALMPSC_REG_GAP), 0);
	MV_REG_WRITE(GALMPSC_CHANNELREG_2+(mpsc*GALMPSC_REG_GAP), 0);
	MV_REG_WRITE(GALMPSC_CHANNELREG_3+(mpsc*GALMPSC_REG_GAP), 1);
	MV_REG_WRITE(GALMPSC_CHANNELREG_4+(mpsc*GALMPSC_REG_GAP), 0);
	MV_REG_WRITE(GALMPSC_CHANNELREG_5+(mpsc*GALMPSC_REG_GAP), 0);
	MV_REG_WRITE(GALMPSC_CHANNELREG_6+(mpsc*GALMPSC_REG_GAP), 0);
	MV_REG_WRITE(GALMPSC_CHANNELREG_7+(mpsc*GALMPSC_REG_GAP), 0);
	MV_REG_WRITE(GALMPSC_CHANNELREG_8+(mpsc*GALMPSC_REG_GAP), 0);
	MV_REG_WRITE(GALMPSC_CHANNELREG_9+(mpsc*GALMPSC_REG_GAP), 0);
	MV_REG_WRITE(GALMPSC_CHANNELREG_10+(mpsc*GALMPSC_REG_GAP), 0);

	galmpsc_set_brkcnt(mpsc, 0x3);
	galmpsc_set_tcschar(mpsc, 0xab);

	return 0;
}

static int
galmpsc_set_brkcnt(int mpsc, int value)
{
	unsigned int temp;

	temp = MVREGREAD(GALMPSC_CHANNELREG_1+(mpsc*GALMPSC_REG_GAP));
	temp &= 0x0000FFFF;
	temp |= (value << 16);
	MV_REG_WRITE(GALMPSC_CHANNELREG_1+(mpsc*GALMPSC_REG_GAP), temp);

	return 0;
}

static int
galmpsc_set_tcschar(int mpsc, int value)
{
	unsigned int temp;

	temp = MVREGREAD(GALMPSC_CHANNELREG_1+(mpsc*GALMPSC_REG_GAP));
	temp &= 0xFFFF0000;
	temp |= value;
	MV_REG_WRITE(GALMPSC_CHANNELREG_1+(mpsc*GALMPSC_REG_GAP), temp);

	return 0;
}

static int
galmpsc_set_char_length(int mpsc, int value)
{
	unsigned int temp;

	temp = MVREGREAD(GALMPSC_PROTOCONF_REG+(mpsc*GALMPSC_REG_GAP));
	temp &= 0xFFFFCFFF;
	temp |= (value << 12);
	MV_REG_WRITE(GALMPSC_PROTOCONF_REG+(mpsc*GALMPSC_REG_GAP), temp);

	return 0;
}

static int
galmpsc_set_stop_bit_length(int mpsc, int value)
{
	unsigned int temp;

	temp = MVREGREAD(GALMPSC_PROTOCONF_REG+(mpsc*GALMPSC_REG_GAP));
	temp &= 0xFFFFBFFF;
	temp |= (value << 14);
	MV_REG_WRITE(GALMPSC_PROTOCONF_REG+(mpsc*GALMPSC_REG_GAP),temp);

	return 0;
}

static int
galmpsc_set_parity(int mpsc, int value)
{
	unsigned int temp;

	temp = MVREGREAD(GALMPSC_CHANNELREG_2+(mpsc*GALMPSC_REG_GAP));
	if (value != -1) {
		temp &= 0xFFF3FFF3;
		temp |= ((value << 18) | (value << 2));
		temp |= ((value << 17) | (value << 1));
	} else {
		temp &= 0xFFF1FFF1;
	}
		
	MV_REG_WRITE(GALMPSC_CHANNELREG_2+(mpsc*GALMPSC_REG_GAP), temp);

	return 0;
}

static int
galmpsc_enter_hunt(int mpsc)
{
	int temp;

	temp = MVREGREAD(GALMPSC_CHANNELREG_2+(mpsc*GALMPSC_REG_GAP));
	temp |= 0x80000000;
	MV_REG_WRITE(GALMPSC_CHANNELREG_2+(mpsc*GALMPSC_REG_GAP), temp);

	while(MVREGREAD(GALMPSC_CHANNELREG_2+(mpsc*GALMPSC_REG_GAP)) & MPSC_ENTER_HUNT) {
		udelay(1);
	}
	return 0;
}


static int
galmpsc_shutdown(int mpsc)
{
	unsigned int temp;

	/* cause RX abort (clears RX) */
	temp = MVREGREAD(GALMPSC_CHANNELREG_2+(mpsc*GALMPSC_REG_GAP));
	temp |= MPSC_RX_ABORT | MPSC_TX_ABORT;
	temp &= ~MPSC_ENTER_HUNT;
	MV_REG_WRITE(GALMPSC_CHANNELREG_2+(mpsc*GALMPSC_REG_GAP),temp);
	
	MV_REG_WRITE(GALSDMA_0_COM_REG, 0);
	MV_REG_WRITE(GALSDMA_0_COM_REG, SDMA_TX_ABORT | SDMA_RX_ABORT);

	/* shut down the MPSC */
	MV_REG_WRITE(GALMPSC_MCONF_LOW, 0);
	MV_REG_WRITE(GALMPSC_MCONF_HIGH, 0);
	MV_REG_WRITE(GALMPSC_PROTOCONF_REG+(mpsc*GALMPSC_REG_GAP),0);
	
	udelay(100);
	
	/* shut down the sdma engines. */
	/* reset config to default */	
	MV_REG_WRITE(GALSDMA_0_CONF_REG, 0x000000fc); 
	
	udelay(100);
	
	/* clear the SDMA current and first TX and RX pointers */
	MV_REG_WRITE(GALSDMA_0_CUR_RX_PTR, 0);
	MV_REG_WRITE(GALSDMA_0_CUR_TX_PTR, 0);
	MV_REG_WRITE(GALSDMA_0_FIR_TX_PTR, 0);

	udelay(100);

	return 0;
}




int
mpscdebug (int op, struct DevEntry *dev, unsigned long param, int data)
{unsigned char c;

	switch (op) {
		case OP_INIT:
			return 0;

		case OP_XBAUD:
		case OP_BAUD:
			return 0;//mpsc_init (dev->freq);

		case OP_TXRDY:
			return 1;

		case OP_TX:
			tgt_putchar(data&0xff);
			//mpsc_putchar(data);
			delay(2000);
			break;

		case OP_RXRDY:
			//printf("space:%d\n",Qspace(dev->rxq));
			return mpsc_test_char();

		case OP_RX:
			c=tgt_getchar()&0xff;
			return c&0xff;//mpsc_getchar() & 0xff;

		case OP_RXSTOP:
			/*
			if (data)
				outb(&dp->mcr, inb(&dp->mcr) & ~MCR_RTS);
			else
				outb(&dp->mcr, inb(&dp->mcr) | MCR_RTS);
				*/
			break;
	}
	return 0;
}
#endif
