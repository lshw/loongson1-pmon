/* rtl8139.c - etherboot driver for the Realtek 8139 chipset

  ported from the linux driver written by Donald Becker
  by Rainer Bawidamann (Rainer.Bawidamann@informatik.uni-ulm.de) 1999

  This software may be used and distributed according to the terms
  of the GNU Public License, incorporated herein by reference.

  changes to the original driver:
  - removed support for interrupts, switching to polling mode (yuck!)
  - removed support for the 8129 chip (external MII)

*/

/*********************************************************************/
/* Revision History                                                  */
/*********************************************************************/

/*

  4 Feb 2000	espenlaub@informatik.uni-ulm.de (Klaus Espenlaub)
     Shuffled things around, removed the leftovers from the 8129 support
     that was in the Linux driver and added a bit more 8139 definitions.
     Moved the 8K receive buffer to a fixed, available address outside the
     0x98000-0x9ffff range.  This is a bit of a hack, but currently the only
     way to make room for the Etherboot features that need substantial amounts
     of code like the ANSI console support.  Currently the buffer is just below
     0x10000, so this even conforms to the tagged boot image specification,
     which reserves the ranges 0x00000-0x10000 and 0x98000-0xA0000.  My
     interpretation of this "reserved" is that Etherboot may do whatever it
     likes, as long as its environment is kept intact (like the BIOS
     variables).  Hopefully fixed rtl8139_poll() once and for all.  The symptoms
     were that if Etherboot was left at the boot menu for several minutes, the
     first eth_poll failed.  Seems like I am the only person who does this.
     First of all I fixed the debugging code and then set out for a long bug
     hunting session.  It took me about a week full time work - poking around
     various places in the driver, reading Don Becker's and Jeff Garzik's Linux
     driver and even the FreeBSD driver (what a piece of crap!) - and
     eventually spotted the nasty thing: the transmit routine was acknowledging
     each and every interrupt pending, including the RxOverrun and RxFIFIOver
     interrupts.  This confused the RTL8139 thoroughly.  It destroyed the
     Rx ring contents by dumping the 2K FIFO contents right where we wanted to
     get the next packet.  Oh well, what fun.

  18 Jan 2000   mdc@thinguin.org (Marty Connor)
     Drastically simplified error handling.  Basically, if any error
     in transmission or reception occurs, the card is reset.
     Also, pointed all transmit descriptors to the same buffer to
     save buffer space.  This should decrease driver size and avoid
     corruption because of exceeding 32K during runtime.

  28 Jul 1999   (Matthias Meixner - meixner@rbg.informatik.tu-darmstadt.de)
     rtl8139_poll was quite broken: it used the RxOK interrupt flag instead
     of the RxBufferEmpty flag which often resulted in very bad
     transmission performace - below 1kBytes/s.

*/
//yh
#if 1  

#ifdef DEBUG_8139
#  define DPRINTF(fmt, args...) printf("%s:" fmt, __FUNCTION__, ##args)
#else
#  define DPRINTF(fmt, args...) 
#endif

#define DEBUG_RX
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/syslog.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#endif

#ifdef IPX
#include <netipx/ipx.h>
#include <netipx/ipx_if.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__)

#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>

#if defined(__NetBSD__)
#include <net/if_ether.h>
#include <netinet/if_inarp.h>
#endif

#if defined(__OpenBSD__)
#include <netinet/if_ether.h>
#endif

#include <vm/vm.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/mii/miivar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/if_fxpreg.h>
#include <dev/pci/if_fxpvar.h>

#ifdef __alpha__		/* XXX */
/* XXX XXX NEED REAL DMA MAPPING SUPPORT XXX XXX */
#undef vtophys
#define	vtophys(va)	alpha_XXX_dmamap((vm_offset_t)(va))
#endif /* __alpha__ */

#else /* __FreeBSD__ */

#include <sys/sockio.h>

#include <netinet/if_ether.h>

#include <vm/vm.h>		/* for vtophys */
#include <vm/vm_param.h>	/* for vtophys */
#include <vm/pmap.h>		/* for vtophys */
#include <machine/clock.h>	/* for DELAY */

#include <pci/pcivar.h>
#include <pci/if_fxpreg.h>
#include <pci/if_fxpvar.h>

#endif /* __NetBSD__ || __OpenBSD__ */

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/*
 * NOTE!  On the Alpha, we have an alignment constraint.  The
 * card DMAs the packet immediately following the RFA.  However,
 * the first thing in the packet is a 14-byte Ethernet header.
 * This means that the packet is misaligned.  To compensate,
 * we actually offset the RFA 2 bytes into the cluster.  This
 * aligns the packet after the Ethernet header at a 32-bit
 * boundary.  HOWEVER!  This means that the RFA is misaligned!
 */

#ifdef BADPCIBRIDGE
#define BADPCIBRIDGE
#define	RFA_ALIGNMENT_FUDGE	4
#else
#define	RFA_ALIGNMENT_FUDGE	2
#endif
#endif //yh


/* to get some global routines like printf */
#include "etherboot.h"
/* to get the interface to the body of the program */
#include "nic.h"
/* we have a PIC card */

#define PHYSADDR(a)	(((unsigned long)(a)) & 0x1fffffff)
#define virt_to_bus(x)  (PHYSADDR((x)))

#define	eth_probe	rtl8139_probe

#define RTL_TIMEOUT (1*TICKS_PER_SEC) 

/* PCI Tuning Parameters
   Threshold is bytes transferred to chip before transmission starts. */
#define TX_FIFO_THRESH 256      /* In bytes, rounded down to 32 byte units. */
#define RX_FIFO_THRESH  7   //4       /* Rx buffer level before first PCI xfer.  */
#define RX_DMA_BURST    6       /* Maximum PCI burst, '4' is 256 bytes '6' is 1024*/
#define TX_DMA_BURST    4       /* Calculate as 16<<val. */
#define NUM_TX_DESC     4       /* Number of Tx descriptor registers. */
#define TX_BUF_SIZE (ETH_MAX_PACKET-4)	/* FCS is added by the chip */
#define RX_BUF_LEN_IDX 2	/* 0, 1, 2 is allowed - 8,16,32K rx buffer */
#define RX_BUF_LEN (8192 << RX_BUF_LEN_IDX)

//#undef DEBUG_TX
//#undef DEBUG_RX

#define DEBUG_TX
#define DEBUG_RX

// the followind defined by 8139dv111.pdf
#define EEPROM_SIZE          64
#define EEPROM_CRC_POSITION  0x32
#define CRC0                 0x55aa

#define RTL8139D_HWID       0x75
#define RTL8100B_HWID       0x75

#define RTL8101_HWID        0x77
#define RTL8139C_PLUS_HWID  0x76

#define RTL8100_HWID        0x7a
#define RTL8139C_HWID       0x74

#define RTL8130_HWID        0x78
#define RTL8139B_HWID       0x78

#define RTL8139A_G_HWID    0x74
#define RTL8139A_HWID      0x70
#define RTL8139_HWID       0x60

/* EEPROM content bufffer */
unsigned char rtl8139c_eeprom_content[2 * EEPROM_SIZE] = {
	0x29, 0x81, 0xec, 0x10, 0x39, 0x81, 0xec, 0x10, 0x39, 0x81,
	0x20, 0x40, 0x12, 0xe1, 0x00, 0xe0,
	0x4c, 0x01, 0x02, 0x03, 0x15, 0x4d, 0xc2, 0xf7, 0x01, 0x88,
	0xb9, 0x43, 0xf2, 0xb0, 0x1a, 0x03,
	0x43, 0xdf, 0x36, 0x8a, 0x43, 0xdf, 0x36, 0x8a, 0xb9, 0x43,
	0xf2, 0xb0, 0x11, 0x11, 0x11, 0x11,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};


unsigned char rtl8100b_eeprom_content[2 * EEPROM_SIZE] = {
	0x29, 0x81, 0xec, 0x10, 0x39, 0x81, 0xec, 0x10, 0x39, 0x81,
	0x20, 0x40, 0x33, 0xe1, 0x00, 0xe0,
	0x4c, 0x77, 0x11, 0x9a, 0x10, 0xcc, 0xc2, 0xf7, 0x01, 0x88,
	0xb9, 0x03, 0xf4, 0x60, 0x1a, 0x04,
	0xa3, 0xdf, 0x36, 0x98, 0xa3, 0xdf, 0x36, 0x98, 0xb9, 0x03,
	0xf4, 0x60, 0x1a, 0x1a, 0x1a, 0x1a,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

/* Symbolic offsets to registers. */
enum RTL8139_registers {
	MAC0=0,			/* Ethernet hardware address. */
	MAR0=8,			/* Multicast filter. */
	TxStatus0=0x10,		/* Transmit status (four 32bit registers). */
	TxAddr0=0x20,		/* Tx descriptors (also four 32bit). */
	RxBuf=0x30, RxEarlyCnt=0x34, RxEarlyStatus=0x36,
	ChipCmd=0x37, RxBufPtr=0x38, RxBufAddr=0x3A,
	IntrMask=0x3C, IntrStatus=0x3E,
	TxConfig=0x40, RxConfig=0x44,
	Timer=0x48,		/* general-purpose counter. */
	RxMissed=0x4C,		/* 24 bits valid, write clears. */
	Cfg9346=0x50, Config0=0x51, Config1=0x52,
	TimerIntrReg=0x54,	/* intr if gp counter reaches this value */
	MediaStatus=0x58,
	Config3=0x59, Config4=0x5A, HltClk=0x5B,
	MultiIntr=0x5C,
	RevisionID=0x5E,	/* revision of the RTL8139 chip */
	TxSummary=0x60,
	MII_BMCR=0x62, MII_BMSR=0x64, NWayAdvert=0x66, NWayLPAR=0x68,
	NWayExpansion=0x6A,
	DisconnectCnt=0x6C, FalseCarrierCnt=0x6E,
	NWayTestReg=0x70,
	RxCnt=0x72,		/* packet received counter */
	CSCR=0x74,		/* chip status and configuration register */
	PhyParm1=0x78,TwisterParm=0x7c,PhyParm2=0x80,	/* undocumented */
	/* from 0x84 onwards are a number of power management/wakeup frame
	 * definitions we will probably never need to know about.  */
};

enum ChipCmdBits {
	CmdReset=0x10, CmdRxEnb=0x08, CmdTxEnb=0x04, RxBufEmpty=0x01, };

/* Interrupt register bits, using my own meaningful names. */
enum IntrStatusBits {
	PCIErr=0x8000, PCSTimeout=0x4000, CableLenChange= 0x2000,
	RxFIFOOver=0x40, RxUnderrun=0x20, RxOverflow=0x10,
	TxErr=0x08, TxOK=0x04, RxErr=0x02, RxOK=0x01,
	
	RxAckBits = RxOK | RxOverflow | RxFIFOOver
};
enum TxStatusBits {
	TxHostOwns=0x2000, TxUnderrun=0x4000, TxStatOK=0x8000,
	TxOutOfWindow=0x20000000, TxAborted=0x40000000,
	TxCarrierLost=0x80000000,
};
enum RxStatusBits {
	RxMulticast=0x8000, RxPhysical=0x4000, RxBroadcast=0x2000,
	RxBadSymbol=0x0020, RxRunt=0x0010, RxTooLong=0x0008, RxCRCErr=0x0004,
	RxBadAlign=0x0002, RxStatusOK=0x0001,
};

enum MediaStatusBits {
	MSRTxFlowEnable=0x80, MSRRxFlowEnable=0x40, MSRSpeed10=0x08,
	MSRLinkFail=0x04, MSRRxPauseFlag=0x02, MSRTxPauseFlag=0x01,
};

enum MIIBMCRBits {
	BMCRReset=0x8000, BMCRSpeed100=0x2000, BMCRNWayEnable=0x1000,
	BMCRRestartNWay=0x0200, BMCRDuplex=0x0100,
};

enum CSCRBits {
	CSCR_LinkOKBit=0x0400, CSCR_LinkChangeBit=0x0800,
	CSCR_LinkStatusBits=0x0f000, CSCR_LinkDownOffCmd=0x003c0,
	CSCR_LinkDownCmd=0x0f3c0,
};

/* Bits in RxConfig. */
enum rx_mode_bits {
	RxCfgWrap=0x80,
	AcceptErr=0x20, AcceptRunt=0x10, AcceptBroadcast=0x08,
	AcceptMulticast=0x04, AcceptMyPhys=0x02, AcceptAllPhys=0x01,

};


//yh from linux 8139 driver
enum RxConfigBits {
	/* rx fifo threshold */
	RxCfgFIFOShift = 13,
	RxCfgFIFONone = (7 << RxCfgFIFOShift),

	/* Max DMA burst */
	RxCfgDMAShift = 8,
	RxCfgDMAUnlimited = (7 << RxCfgDMAShift),

	/* rx ring buffer length */
	RxCfgRcv8K = 0,
	RxCfgRcv16K = (1 << 11),
	RxCfgRcv32K = (1 << 12),
	RxCfgRcv64K = (1 << 11) | (1 << 12),

	/* Disable packet wrap at end of Rx buffer */
	RxNoWrap = (1 << 7),
};

enum Config1Bits {
	Cfg1_PM_Enable = 0x01,
	Cfg1_VPD_Enable = 0x02,
	Cfg1_PIO = 0x04,
	Cfg1_MMIO = 0x08,
	LWAKE = 0x10,		/* not on 8139, 8139A */
	Cfg1_Driver_Load = 0x20,
	Cfg1_LED0 = 0x40,
	Cfg1_LED1 = 0x80,
	SLEEP = (1 << 1),	/* only on 8139, 8139A */
	PWRDN = (1 << 0),	/* only on 8139, 8139A */
};

enum Cfg9346Bits {
	Cfg9346_Lock = 0x00,
	Cfg9346_Unlock = 0xC0,
};


enum Config4Bits {
	LWPTN = (1 << 2),	/* not on 8139, 8139A */
};
static const unsigned int rtl8139_rx_config =
	RxCfgRcv32K | 
	(RX_FIFO_THRESH << RxCfgFIFOShift) |
	(RX_DMA_BURST << RxCfgDMAShift);
// above is from linux driver
	
enum tx_config_bits {
	TxIFG1 = (1 << 25),	/* Interframe Gap Time */
	TxIFG0 = (1 << 24),	/* Enabling these bits violates IEEE 802.3 */
	TxLoopBack = (1 << 18) | (1 << 17), /* enable loopback test mode */
	TxCRC = (1 << 16),	/* DISABLE appending CRC to end of Tx packets */
	TxClearAbt = (1 << 0),	/* Clear abort (WO) */
	TxDMAShift = 8,		/* DMA burst value (0-7) is shifted this many bits */
	TxRetryShift = 4,	/* TXRR value (0-15) is shifted this many bits */

	TxVersionMask = 0x7C800000, /* mask out version bits 30-26, 23 */
};

static int ioaddr;
// current receive address & transmit address
unsigned int cur_rx;
volatile unsigned cur_tx, dirty_tx;

/* The RTL8139 can only transmit from a contiguous, aligned memory block.  */
// transmit buffer address

//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
unsigned char my_mac_address[6];
//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
unsigned int rtlioaddress = 0;
//function proto types
static int rtl_ether_ioctl(struct ifnet *ifp, FXP_IOCTLCMD_TYPE cmd, caddr_t data);

struct nic *rtl8139_probe(struct nic *nic);
int rtl8139_intr(void *arg);
static int read_eeprom(struct nic *nic, long ioaddr, int location);
static int rtl8139_init(struct nic *nic);
static void rtl8139_transmit(struct ifnet *ifp);
	
static int rtl8139_poll(struct nic *nic);
static long long rtl_read_mac(struct nic *nic);
int write_eeprom(long idaddr,int,unsigned short);
int write_eeprom8(long idaddr,int,unsigned short);

#define RTL8139_PCI_MEMA  0x14
#define RTL8139_PCI_IOBA  0x10
int db8139=0;
#if 0
static void dump_buf(unsigned char* data, int len)
{
	int i;
	printf("\nThe buffer %p contents is\n", data);
	for (i=0; i<len; i++) {
		if ((i&15) == 0)
		    printf("Off %04x: ", i);
		printf("%02x ", data[i]);
		if (i%16 == 15)
		    printf("\n");
		else if ((i&3) == 3)
		    printf(" - ");
	}
	printf("\n");
}
#endif

/* Serial EEPROM section. */

/*  EEPROM_Ctrl bits. */
#define EE_SHIFT_CLK    0x04    /* EEPROM shift clock. */
#define EE_CS           0x08    /* EEPROM chip select. */
#define EE_DATA_WRITE   0x02    /* EEPROM chip data in. */
#define EE_WRITE_0      0x00
#define EE_WRITE_1      0x02
#define EE_DATA_READ    0x01    /* EEPROM chip data out. */
#define EE_ENB          (0x80 | EE_CS)

/*
	Delay between EEPROM clock transitions.
	No extra delay is needed with 33Mhz PCI, but 66Mhz may change this.
*/

#define eeprom_delay()  inl(ee_addr)

/* The EEPROM commands include the alway-set leading bit. */
#ifndef EPLC46
#define EE_WEN_CMD      ( (4 << 6) | ( 3 << 4 ) )
#define EE_WRITE_CMD    (5 << 6)
#define EE_WDS_CMD      (4 << 6)
#define EE_READ_CMD     (6 << 6)
#define EE_ERASE_CMD    (7 << 6)
#else
#define EE_WEN_CMD      ( (4 << 7) | ( 3 << 5 ) )
#define EE_WRITE_CMD    (5 << 7)
#define EE_WDS_CMD      (4 << 7)
#define EE_READ_CMD     (6 << 7)
#define EE_ERASE_CMD    (7 << 7)
#endif

#define outb(v, a)  (*(volatile unsigned char *)(a) = (v))
#define outw(v, a)  (*(volatile unsigned short*)(a) = (v))
#define outl(v, a)  (*(volatile unsigned long *)(a) = (v))

static int read_eeprom(struct nic * nic, long ioaddr, int location)
{
	int i;
	unsigned int retval = 0;
	long ee_addr = ioaddr + Cfg9346;
	int read_cmd = location | EE_READ_CMD;

	RTL_WRITE_1(nic, Cfg9346, EE_ENB & ~EE_CS);
	RTL_WRITE_1(nic, Cfg9346, EE_ENB );

#ifndef EPLC46
	/* Shift the read command bits out. */
	for (i = 10; i >= 0; i--) {
		int dataval = (read_cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		RTL_WRITE_1(nic, Cfg9346, EE_ENB | dataval);
		eeprom_delay();
		RTL_WRITE_1(nic, Cfg9346, EE_ENB | dataval | EE_SHIFT_CLK);
		eeprom_delay();
	}  
#else
	for (i = 11; i >= 0; i--) {
		int dataval = (read_cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		RTL_WRITE_1(nic, Cfg9346, EE_ENB | dataval);
		eeprom_delay();
		RTL_WRITE_1(nic, Cfg9346, EE_ENB | dataval | EE_SHIFT_CLK);
		eeprom_delay();
	}  
#endif
	RTL_WRITE_1(nic, Cfg9346, EE_ENB);
	eeprom_delay();

#ifndef	 EPLC46
	for (i = 16; i > 0; i--) {
		RTL_WRITE_1(nic, Cfg9346, EE_ENB | EE_SHIFT_CLK);
		eeprom_delay();
		retval = (retval << 1) | ((inb(ee_addr) & EE_DATA_READ) ? 1 : 0);
		RTL_WRITE_1(nic, Cfg9346, EE_ENB);
		eeprom_delay();
	}

	RTL_WRITE_1(nic,~EE_CS, ee_addr);
#else
	for (i = 8; i > 0; i--) {
		RTL_WRITE_1(nic, Cfg9346, EE_ENB | EE_SHIFT_CLK);
		eeprom_delay();
		retval = (retval << 1) | ((inb(ee_addr) & EE_DATA_READ) ? 1 : 0);
		RTL_WRITE_1(nic, Cfg9346, EE_ENB);
		eeprom_delay();
	}
	
	RTL_WRITE_1(nic,~EE_CS, ee_addr);   //END
#endif

	/* Terminate the EEPROM access. */
	return retval;
}

static void write_eeprom_enable(long ioaddr){
	int i;
	long ee_addr = ioaddr + Cfg9346;
	int  cmd = EE_WEN_CMD;

	outb(EE_ENB & ~EE_CS, ee_addr);
	outb(EE_ENB, ee_addr);

	/* Shift the read command bits out. */
#ifndef EPLC46
	for (i = 10; i >= 0; i--) {
		int dataval = (cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		outb(EE_ENB | dataval, ee_addr);
		eeprom_delay();
		outb(EE_ENB | dataval | EE_SHIFT_CLK, ee_addr);
		eeprom_delay();
	}
#else
	for (i = 11; i >= 0; i--) {
		int dataval = (cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		outb(EE_ENB | dataval, ee_addr);
		eeprom_delay();
		outb(EE_ENB | dataval | EE_SHIFT_CLK, ee_addr);
		eeprom_delay();
	}
#endif
	outb(~EE_CS, ee_addr);

}

static void write_eeprom_disable(long idaddr)
{
	int i;
	long ee_addr = ioaddr + Cfg9346;
	int  cmd = EE_WDS_CMD;

	outb(EE_ENB & ~EE_CS, ee_addr);
	outb(EE_ENB, ee_addr);

	/* Shift the read command bits out. */

#ifndef EPLC46
	i=10;
#else
	i=11;
#endif
	for (; i >= 0; i--) {
		int dataval = (cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		outb(EE_ENB | dataval, ee_addr);
		eeprom_delay();
		outb(EE_ENB | dataval | EE_SHIFT_CLK, ee_addr);
		eeprom_delay();
	}
	outb(~EE_CS, ee_addr);
}

int write_eeprom(long ioaddr, int location,unsigned short data)
{
	int i;
	long ee_addr = ioaddr + Cfg9346;
	int  cmd = location | EE_WRITE_CMD;

	write_eeprom_enable(ioaddr);

	cmd <<=16;
	cmd |= data;
	outb(EE_ENB & ~EE_CS, ee_addr);
	outb(EE_ENB, ee_addr);

	/* Shift the read command bits out. */
	for (i = 26; i >= 0; i--) {
		int dataval = (cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		outb(EE_ENB | dataval, ee_addr);
		eeprom_delay();
		outb(EE_ENB | dataval | EE_SHIFT_CLK, ee_addr);
		eeprom_delay();
	}
	/* Terminate the EEPROM access. */

	outb(~EE_CS, ee_addr);
	eeprom_delay();

	outb(EE_ENB, ee_addr);
	 
    while( ! (inb(ee_addr) & EE_DATA_READ) ){
		outb(EE_ENB, ee_addr);
		eeprom_delay();
	}
	return 0;
}

int write_eeprom8(long ioaddr, int location,unsigned short data)
{
	int i;
	long ee_addr = ioaddr + Cfg9346;
	int  cmd = location | EE_WRITE_CMD;

	write_eeprom_enable(ioaddr);

	cmd <<= 8;
	cmd |= data;
	outb(EE_ENB & ~EE_CS, ee_addr);
	outb(EE_ENB, ee_addr);

	/* Shift the read command bits out. */
	for (i = 18; i >= 0; i--) {
		int dataval = (cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		outb(EE_ENB | dataval, ee_addr);
		eeprom_delay();
		outb(EE_ENB | dataval | EE_SHIFT_CLK, ee_addr);
		eeprom_delay();
	}
	/* Terminate the EEPROM access. */

	outb(~EE_CS, ee_addr);
	eeprom_delay();

	outb(EE_ENB, ee_addr);
	 
    while( ! (inb(ee_addr) & EE_DATA_READ) ){
		outb(EE_ENB, ee_addr);
		eeprom_delay();
	}
	return 0;
}


static char mii_2_8139_map[8] = {
	MII_BMCR,
	MII_BMSR,
	0,
	0,
	NWayAdvert,
	NWayLPAR,
	NWayExpansion,
	0
};

static void mdio_write (struct nic *nic, int phy_id, int location,
			int value)
{
	if (phy_id > 31) {	/* Really a 8139.  Use internal registers. */
		if (location == 0) {
			RTL_WRITE_1 (nic, Cfg9346, Cfg9346_Unlock);
			RTL_WRITE_2 (nic, MII_BMCR, value);
			RTL_WRITE_1 (nic, Cfg9346, Cfg9346_Lock);
		} else if (location < 8 && mii_2_8139_map[location])
			RTL_WRITE_2 (nic, mii_2_8139_map[location], value);
		return;
	}
}

static void rtl_delay(int n)
{
    volatile unsigned int delay = 0xFF, num = n;

    if (!num)
	num = 1;
    while (num--) {
	while (delay--);
	delay = 0xFF;
    }
}
/* Reset the 8139 chip and start it
 * */
static int rtl8139_init(struct nic* nic)
{
	struct ifnet *ifp;
	int speed10, fullduplex;
	int i;
	unsigned char   tmp8, new_tmp8;
		
	ifp = &nic->arpcom.ac_if;

	/*
	 * Bring the interface out of low power mode
	 * */		
	RTL_WRITE_1(nic, HltClk, 'R');
	
	tmp8 = new_tmp8 = RTL_READ_1(nic, Config1);
	if(tmp8 & LWAKE)
		new_tmp8 &= ~ LWAKE;
	new_tmp8 |= Cfg1_PM_Enable;
	if(new_tmp8 != tmp8){
		printf("Config1\n");
		RTL_WRITE_1(nic, Cfg9346, Cfg9346_Unlock );
		RTL_WRITE_1(nic, Config1, new_tmp8);  //?
		RTL_WRITE_1(nic, Cfg9346, Cfg9346_Lock);	
	}
	
	tmp8 = RTL_READ_1 (nic, Config4);
	if (tmp8 & LWPTN) {
		printf("Config4\n");
		RTL_WRITE_1 (nic, Cfg9346, Cfg9346_Unlock);
		RTL_WRITE_1 (nic, Config4, tmp8 & ~LWPTN);
		RTL_WRITE_1 (nic, Cfg9346, Cfg9346_Lock);
	}
	

	
	RTL_WRITE_1(nic, ChipCmd, CmdReset);
	rtl_delay(24);
	
	cur_rx = 0;
	cur_tx = 0;         // currently write to this        
	dirty_tx =0;        // has to check whether this transfered successfully
	
	/* Check that the chip has finished the reset. */
	for (i = 1000; i > 0; i--)
		if ((RTL_READ_1(nic,  ChipCmd) & CmdReset) == 0)
			break;
	if (!i){
		printf("Reset timeout!\n");
		return -1;
	}
	
//	RTL_WRITE_1 (nic, Cfg9346, Cfg9346_Unlock);
//	RTL_WRITE_2 (nic, MII_BMCR, 0x8000);
//	RTL_WRITE_1 (nic, Cfg9346, Cfg9346_Lock);
	
	speed10 = RTL_READ_1(nic,  MediaStatus) & MSRSpeed10;
	fullduplex = RTL_READ_2(nic,  MII_BMCR) & BMCRDuplex;
	printf(" %sMbps %s-DUPLEX.\n", speed10 ? "10" : "100",
		fullduplex ? "FULL" : "HALF");

	
	/* Must enable Tx/Rx before setting transfer thresholds! */
	RTL_WRITE_1(nic, ChipCmd, CmdRxEnb | CmdTxEnb);
	/*RX_FIFO_THREASH=256 Bytes RX_Buffer=8k+16bytes    MAX dma burst size 256bytes*/

	RTL_WRITE_4(nic, RxConfig, 
			rtl8139_rx_config | AcceptBroadcast  |AcceptMyPhys
			);
				/* XXX accept no frames yet!  */
	RTL_WRITE_4(nic, TxConfig, (TX_DMA_BURST<<8)|0x03000000);


	//set receive buffer address and 
	//transmit buffer address
	RTL_WRITE_4(nic, RxBuf, (unsigned long)nic->rx_dma);
	for(i=0; i<4; i++){
		RTL_WRITE_4(nic, TxAddr0+i*4, 
				(unsigned long)nic->tx_ad[i]);
	}
	
	/* Start the chip's Tx and Rx process. */
	RTL_WRITE_4(nic, RxMissed, 0);
	/* set_rx_mode */
	
	/* If we add multicast support, the MAR0 register would have to be
	 * initialized to 0xffffffffffffffff (two 32 bit accesses).  Etherboot
	 * only needs broadcast (for ARP/RARP/BOOTP/DHCP) and unicast.  */
	RTL_WRITE_1(nic, ChipCmd, CmdRxEnb | CmdTxEnb);
	
	if((RTL_READ_1(nic, ChipCmd) &(CmdRxEnb|CmdTxEnb)) != (CmdRxEnb|CmdTxEnb))
		printf("Chip Rx or Tx not enabled\n");
	/* Disable all known interrupts by setting the interrupt mask. */
	RTL_WRITE_2(nic, IntrMask, 0);
	// ifp->if_flags |= IFF_BROADCAST | IFF_SIMPLEX ; //| IFF_MULTICAST;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX ; //| IFF_MULTICAST;
	ifp->if_flags |=IFF_RUNNING;	
	//for(i = 0; i <4; i++){
	//	printf("TxStatus[%d]=%x\n", i, RTL_READ_4(nic, TxStatus0+i*4));
	//}
	
	return 0;
}

static void rtl8139_transmit(struct ifnet *ifp)
{
	unsigned entry;
	struct nic *nic = ifp->if_softc;
	struct mbuf *mb_head;		

	/* Note: RTL8139 doesn't auto-pad, send minimum payload (another 4
	 * bytes are sent automatically for the FCS, totalling to 64 bytes). */

	if(cur_tx - dirty_tx ==4){
		return;
	}		
	while(ifp->if_snd.ifq_head != NULL ){
		entry = cur_tx % 4;	
		
		IF_DEQUEUE(&ifp->if_snd, mb_head);
		m_copydata(mb_head, 0, mb_head->m_pkthdr.len, nic->tx_buf[entry]);
		
		if(mb_head->m_pkthdr.len<60)
		     memset(nic->tx_buf[entry]+mb_head->m_pkthdr.len, 0, 60-mb_head->m_pkthdr.len);
		
#ifdef __mips__
		pci_sync_cache(nic->sc_pc, (vm_offset_t)nic->tx_buf[entry], 
				max(60, mb_head->m_pkthdr.len), SYNC_W);
#endif				   


		/*This will clear the TxStatus(OWN), thus start DMA
		 * then we poll to find whether the dma finished
		 * */
		RTL_WRITE_4(nic, TxStatus0 +entry*4, ((TX_FIFO_THRESH<<11) & 0x003f0000) 
				| max(60, mb_head->m_pkthdr.len));
		cur_tx ++;
		m_freem(mb_head);
		wbflush();
		if(cur_tx - dirty_tx == 4)
			break;	
	} // endwhile
}
static int rtl8139_tx_intr(struct nic *nic)	
{
	int dowork=0;
	volatile int free_tx ;
	int entry ;	
	
	free_tx = cur_tx - dirty_tx;
	while(free_tx > 0){
		
		int txstatus;
		
		entry = dirty_tx %4;
		txstatus = RTL_READ_4 (nic, TxStatus0 + (entry *4 ));
		
		if (!(txstatus & (TxStatOK | TxUnderrun | TxAborted)))
			break;	/* It still hasn't been Txed */

		/* Note: TxCarrierLost is always asserted at 100mbps. */
		if (txstatus & (TxOutOfWindow | TxAborted)) {
			/* There was an major error, log it. */
			DPRINTF ("Transmit error, Tx status %8.8x.\n", txstatus);
			if (txstatus & TxAborted) {
				RTL_WRITE_4 (nic, TxConfig, TxClearAbt);
				RTL_WRITE_2 (nic, IntrStatus, TxErr);
				wbflush();
			}
		} 
		
		dirty_tx++;
		free_tx--;
		dowork  = 1;
	}
	
	return dowork;
}
// receive the data
static struct mbuf * getmbuf(struct nic *nic);
static struct mbuf * getmbuf(struct nic *nic)
{
	struct mbuf *m;


	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if(m == NULL){
		printf("getmbuf for reception failed\n");
		return  NULL;
	} else {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			return NULL;
		}
		if(m->m_data != m->m_ext.ext_buf){
			printf("m_data not equal to ext_buf!!!\n");
		}
	}
	
#if defined(__mips__)
	/*
	 * Sync the buffer so we can access it uncached.
	 */
	if (m->m_ext.ext_buf!=NULL) {
		pci_sync_cache(nic->sc_pc, (vm_offset_t)m->m_ext.ext_buf,
				MCLBYTES, SYNC_R);
	}
	m->m_data += RFA_ALIGNMENT_FUDGE;
#else
	m->m_data += RFA_ALIGNMENT_FUDGE;
#endif
	return m;
}
static void rtl8139_rx_err(struct nic *nic, int status)
{
	
	unsigned char  tmp8;
	int tmp_work;
	
	/* disable receive */
	RTL_WRITE_1 (nic, ChipCmd, CmdTxEnb);
	tmp_work = 200;
	while (--tmp_work > 0) {
		tmp_work = RTL_READ_1 (nic, ChipCmd);
		if (!(tmp_work & CmdRxEnb))
			break;
	}
	if (tmp_work <= 0)
		DPRINTF ("rx stop wait too long\n");
	/* restart receive */
	tmp_work = 200;
	while (--tmp_work > 0) {
		RTL_WRITE_1 (nic, ChipCmd, CmdRxEnb | CmdTxEnb);
		tmp8 = RTL_READ_1 (nic, ChipCmd);
		if ((tmp8 & CmdRxEnb) && (tmp8 & CmdTxEnb))
			break;
	}
	if (tmp_work <= 0)
		DPRINTF ("tx/rx enable wait too long\n");

	/* and reinitialize all rx related registers */
	RTL_WRITE_1 (nic, Cfg9346, 0xc0); // allow write the config register ?not need so
	/* Must enable Tx/Rx before setting transfer thresholds! */
	RTL_WRITE_1 (nic, ChipCmd, CmdRxEnb | CmdTxEnb);

	RTL_WRITE_4(nic, RxConfig,
			rtl8139_rx_config| AcceptMyPhys |AcceptBroadcast);
	cur_rx = 0;

	DPRINTF("init buffer addresses\n");

	/* Lock Config[01234] and BMCR register writes */
	RTL_WRITE_1 (nic, Cfg9346, 0x00); //disable writing to config registers

	/* init Rx ring buffer DMA address */
	RTL_WRITE_4 (nic, RxBuf, (unsigned long )nic->rx_dma);

	/* A.C.: Reset the multicast list. */
	//__set_rx_mode (dev);
}
static int rtl8139_poll(struct nic *nic)
{
	unsigned int ring_offs;
	unsigned int rx_size, rx_status;
	
	struct ifnet *ifp=&nic->arpcom.ac_if;
	struct mbuf *m;
	struct ether_header *eh;


	while( (RTL_READ_1(nic, ChipCmd) & RxBufEmpty) ==0){
		wbflush();
		ring_offs = cur_rx % RX_BUF_LEN;
		rx_status = *(unsigned int*)(nic->rx_buffer+ ring_offs);
		rx_size = rx_status >> 16;
		rx_status &= 0xffff;
		
		DPRINTF("rtl8139_poll: rx_status=0x%x rx_size =%d\n", rx_status, rx_size);
		/* Packet copy from FIFO still in progress.
		 * Theoretically, this should never happen
		 * since EarlyRx is disabled.
		 */
		if (rx_size == 0xfff0) {
			//tp->xstats.early_rx++;
			break;
		}
	  	if( (rx_size < 64 || (rx_size > ETH_MAX_PACKET) || 
				!(rx_status &RxStatusOK))) {

			DPRINTF("rx error status=%x size=%d\n", rx_status, rx_size);
			rtl8139_rx_err(nic, rx_status);
			return 0;   
		}

	/* Received a good packet */
	nic->packetlen = rx_size - 4;	/* no one cares about the FCS */

	m =getmbuf(nic);
	if (m == NULL){
		printf("getmbuf failed in  rtl8139_poll\n");
		return 0; // no successful
	}
	
	if (ring_offs+4+rx_size-4 > RX_BUF_LEN) {
		int semi_count = RX_BUF_LEN - ring_offs - 4;
		
		bcopy(nic->rx_buffer+ ring_offs+4, mtod(m, caddr_t), semi_count);
		bcopy(nic->rx_buffer, mtod(m, caddr_t)+semi_count , rx_size-4-semi_count);
	} else {
		bcopy(nic->rx_buffer+ ring_offs +4, mtod(m, caddr_t), nic->packetlen);
	}

	//hand up  the received package to upper protocol for further dealt
	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = m->m_len = (rx_size -4)  -sizeof(struct ether_header);

	eh=mtod(m, struct ether_header *); 	

	m->m_data += sizeof(struct ether_header);
	//printf("%s, etype %x:\n", __FUNCTION__, eh->ether_type);
	ether_input(ifp, eh, m);
	
	//cur_rx = (cur_rx + rx_size + 4 + 3) & ~3;
	cur_rx = (cur_rx + rx_size + 4 + 3)  & (RX_BUF_LEN-1)& ~3 ;

	RTL_WRITE_2(nic, RxBufPtr, cur_rx -16);
	/* See RTL8139 Programming Guide V0.1 for the official handling of
	 * Rx overflow situations.  The document itself contains basically no
	 * usable information, except for a few exception handling rules.  */
	if(RTL_READ_2(nic, IntrStatus) & RxAckBits )
		RTL_WRITE_2(nic, IntrStatus, RxAckBits);
     	} //end while(RxBufEmpty)	
	return 1;
}

static long long rtl_read_mac(struct nic *nic)
{
	int i;
	long long mac_tmp = 0;
	unsigned short u16tmp;
	
	for (i = 0; i < 3; i++) {
#ifndef EPLC46
		u16tmp = read_eeprom(nic, ioaddr, 7 + i);
#else
		u16tmp = read_eeprom(nic, ioaddr, (7 +i ) * i);
		u16tmp = u16tmp | (read_eeprom(nic, ioaddr, (7 + i) *2 +1) << 8);
#endif
		mac_tmp <<= 16;
		mac_tmp |= ((u16tmp & 0xff) << 8) | ((u16tmp >> 8) & 0xff);
	}
	return mac_tmp;
}

#if 1 //yanhua

#if defined(__BROKEN_INDIRECT_CONFIG) || defined(__OpenBSD__)
static int rtl8139_match (struct device *, void *, void *);
#else
static int rtl8139 (struct device *, struct cfdata *, void *);
#endif
static void rtl8139_attach (struct device *, struct device *, void *);

struct cfattach rtl_ca = {
	sizeof(struct nic), rtl8139_match, rtl8139_attach, 
};


//DV_IFNET means network interface
//defined in sys/dev/device.h
struct cfdriver rtl_cd = {
	NULL, "rtl", DV_IFNET,
};

#define PCI_VENDOR_RTL  0x10EC
#define PCI_PRODUCT_RTL 0x8139
static int rtl8139_match(
		struct device *parent, 
		void   *match,
		void * aux
		)
{
	struct pci_attach_args *pa = aux;
	if(PCI_VENDOR(pa->pa_id) == PCI_VENDOR_RTL && 
		PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_RTL)
		return 1;
	return 0;
}

static int
rtl8139_attach_common(struct nic *nic, u_int8_t *enaddr)
{
	unsigned long long macaddr;
	u_int8_t *paddr;
	int i;

	/* 
	 * Reset the chip
	 */
	RTL_WRITE_1(nic, ChipCmd, CmdReset);
	for(i=100; i>=0; i--){
		if((RTL_READ_1(nic, ChipCmd) & CmdReset) ==0)
			break;
	}
	if(i<0)
		printf("Reset failed in %s", __FUNCTION__);	
	/*
	 * Read MAC address.
	 */
	macaddr=rtl_read_mac(nic);
	paddr=(uint8_t*)&macaddr;
#ifdef USE_ENVMAC 
	{
		int i;
		int32_t v;
	char *s=getenv("ethaddr");
	if(s){
		for(i = 0; i < 6; i++) {
			gethex(&v, s, 2);
			enaddr[i] = v;
			s += 3;         /* Don't get to fancy here :-) */
		} 
	 }
	} 
#else

#ifndef EPLC46
	if (read_eeprom (nic, ioaddr, 0) != 0x8129) 
#else
	if (read_eeprom (nic, ioaddr, 0) != 0x29 &&
			read_eeprom(nic, ioaddr, 1) != 0x81) 
#endif
	{
		unsigned short rom[] = {
			0x8129 ,0x10ec ,0x8139 ,0x10ec ,0x8139 ,0x4020 ,0xe512 ,0x0a00,
			0x56eb ,0x135b ,0x4d15 ,0xf7c2 ,0x8801 ,0x03b9 ,0x60f4 ,0x071a,
			0xdfa3 ,0x9836 ,0xdfa3 ,0x9836 ,0x03b9 ,0x60f4 ,0x1a1a ,0x1a1a,
			0x0000 ,0xb6e3 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x2000,
			0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000,
			0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000,
			0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000,
			0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000};
		printf("Invalid eeprom! word 0 = %x, Updated to default\n",read_eeprom(nic, ioaddr, 0));
		for (i = 0; i < 64; i++)
		{
#ifndef EPLC46			
			write_eeprom(ioaddr, i, rom[i]);
#else
			write_eeprom8(ioaddr, 2*i, ((unsigned char *)rom)[2*i]);
			write_eeprom8(ioaddr, 2*i+1, ((unsigned char *)rom)[2*i+1]);
#endif
		}

		for (i = 0; i < 64; i++)
		{
#ifndef EPLC46
			printf("%2.2x ",read_eeprom(nic, ioaddr, i));
#else
			printf("%02x",read_eeprom(nic, ioaddr, 2*i+1));
			printf("%02x ",read_eeprom(nic, ioaddr, 2*i));
#endif
		}
	}
/*
	for(i =0; i <6; i++)
	{	
		enaddr[i]=read_eeprom(nic, ioaddr, MAC0+i);
		printf("%02x%c",enaddr,(i!=5)?':':'\n');
	}
*/
	enaddr[0] = paddr[5- 0];
	enaddr[1] = paddr[5- 1];
	enaddr[2] = paddr[5- 2];
	enaddr[3] = paddr[5- 3];
	enaddr[4] = paddr[5- 4];
	enaddr[5] = paddr[5- 5];
#endif

	
	return (0);
}

struct nic * mynic;
static void rtl8139_init_ring(struct nic *nic)
{
	int i;
	unsigned long buf;
	
	/*
	 * allocate Tx buffer and init it; 
	 */
	buf = (unsigned long)malloc(TX_BUF_SIZE*4+15, M_DEVBUF, M_DONTWAIT );
	memset((caddr_t)buf, 0, TX_BUF_SIZE*14+15);
	pci_sync_cache(nic->sc_pc, buf, TX_BUF_SIZE*4+15, SYNC_W);
	
	buf = (buf+15) & ~15;
	nic->tx_buffer = (unsigned char *)CACHED_TO_UNCACHED(buf);
	nic->tx_dma =(unsigned char *)vtophys(buf);

	/*
	 * allocate Rx buffer and init it
	 */
	buf = (unsigned long)malloc(RX_BUF_LEN+15, M_DEVBUF, M_DONTWAIT );
	memset((caddr_t)buf, 0, RX_BUF_LEN+15);
	pci_sync_cache(nic->sc_pc, buf, RX_BUF_LEN, SYNC_W);

	buf = (buf+15) & ~15;
	nic->rx_buffer = (unsigned char *)CACHED_TO_UNCACHED(buf);
	nic->rx_dma =(unsigned char *)vtophys(buf);

	
	for(i = 0; i< 4; i++){
		nic->tx_ad[i] = &nic->tx_dma[i*TX_BUF_SIZE];
		nic->tx_buf[i] =nic->tx_buffer + i*TX_BUF_SIZE;
	}
}

static void rtl8139_attach(struct device * parent, struct device * self, void *aux)
{
	/*struct device is the first element of struct nic*/
	struct nic *nic = (struct nic *)self;

	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	const char *intrstr = NULL;
	u_int8_t enaddr[6];
	struct ifnet *ifp;
#ifdef __OpenBSD__
	bus_space_tag_t iot = pa->pa_iot;
	bus_addr_t iobase;
	bus_size_t iosize;
#endif

	// for debug purpose	
	mynic =nic;

#ifndef __OpenBSD__
	/*
	 * Map control/status registers.
	 */
	if (pci_mapreg_map(pa, RTL8139_PCI_IOBA, PCI_MAPREG_TYPE_MEM, 0,
	    &nic->sc_st, &nic->sc_sh, NULL, NULL)) {
		printf(": can't map registers\n");
		return;
	}
#else
	if (pci_io_find(pc, pa->pa_tag, RTL8139_PCI_IOBA, &iobase, &iosize)) {
		printf(": can't find i/o space\n");
		return;
	}
	
	if (bus_space_map(iot, iobase, iosize, 0, &nic->sc_sh)) {
		printf(": can't map i/o space\n");
		return;
	}
	ioaddr = nic->sc_sh;      //iobase 0x1fd0ff00

	printf("8139 iobase =%8x\n", ioaddr);
	nic->sc_st = iot;
	nic->sc_pc = pc;
	
#endif
	/*
	 * Allocate our interrupt.
	 */
	if (pci_intr_map(pc, pa->pa_intrtag, pa->pa_intrpin,
	    pa->pa_intrline, &ih)) {
		printf(": couldn't map interrupt\n");
		return;
	}
	
	intrstr = pci_intr_string(pc, ih);
#ifdef __OpenBSD__
	nic->sc_ih = pci_intr_establish(pc, ih, IPL_NET, rtl8139_intr, nic,
	    self->dv_xname);
#else
	nic->sc_ih = pci_intr_establish(pc, ih, IPL_NET, fxp_intr, nic);
#endif
	
	if (nic->sc_ih == NULL) {
		printf(": couldn't establish interrupt");
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	
	
	rtl8139_init_ring(nic);
	/* Do generic parts of attach. */
	if (rtl8139_attach_common(nic, enaddr)) {
		/* Failed! */
		return;
	}

#ifdef USE_ENVMAC 
	/* set the mac address */
	{
		int i;
		unsigned int *pint=(unsigned int*)&enaddr;
		RTL_WRITE_1(nic,0x50,0xc0);

		for(i =0; i <2; i++)
			RTL_WRITE_4(nic, MAC0+i*4,pint[i]);
	}
#else
	{
		int i;
		unsigned int *pint = (unsigned int *)&enaddr;
		RTL_WRITE_1 (nic, Cfg9346, Cfg9346_Unlock);
		for(i =0; i <2; i++)
			RTL_WRITE_4(nic, MAC0+i*4,pint[i]);
	}
#endif


#ifdef __OpenBSD__
	ifp = &nic->arpcom.ac_if;
	bcopy(enaddr, nic->arpcom.ac_enaddr, sizeof(enaddr));
#else
	ifp = &nic->sc_ethercom.ec_if;
#endif
	bcopy(nic->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
	
	ifp->if_softc = nic;
	ifp->if_ioctl = rtl_ether_ioctl; 
	ifp->if_start = rtl8139_transmit;
	//ifp->if_watchdog = fxp_watchdog;
	ifp->if_watchdog = NULL; 
	
	printf(": %s, address %s\n", intrstr,
	    ether_sprintf(nic->arpcom.ac_enaddr));
#if 0
	/*
	 * Initialize our media structures and probe the MII.
	 */
	nic->sc_mii.mii_ifp = ifp;
	nic->sc_mii.mii_readreg = fxp_mdi_read;
	nic->sc_mii.mii_writereg = fxp_mdi_write;
	nic->sc_mii.mii_statchg = fxp_statchg;
	ifmedia_init(&sc->sc_mii.mii_media, 0, fxp_mediachange,
	    fxp_mediastatus);
	mii_phy_probe(self, &sc->sc_mii, 0xffffffff);
	/* If no phy found, just use auto mode */
	if (LIST_FIRST(&sc->sc_mii.mii_phys) == NULL) {
		ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|IFM_MANUAL,
		    0, NULL);
		printf(FXP_FORMAT ": no phy found, using manual mode\n",
		    FXP_ARGS(sc));
	}

	if (ifmedia_match(&nic->sc_mii.mii_media, IFM_ETHER|IFM_MANUAL, 0))
		ifmedia_set(&nic->sc_mii.mii_media, IFM_ETHER|IFM_MANUAL);
	else if (ifmedia_match(&ni->sc_mii.mii_media, IFM_ETHER|IFM_AUTO, 0))
		ifmedia_set(&nic->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);
	else
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_10_T);

	/*
	 * Attach the interface.
	 */
#endif
	rtl8139_init(nic);
	if_attach(ifp);
	/*
	 * Let the system queue as many packets as we have available
	 * TX descriptors.
	 */
	//ifp->if_snd.ifq_maxlen = FXP_NTXCB - 1;
	ifp->if_snd.ifq_maxlen = 4;
#ifdef __NetBSD__
	ether_ifattach(ifp, enaddr);
#else
	ether_ifattach(ifp);
#endif
#if NBPFILTER > 0
#ifdef __OpenBSD__
	bpfattach(&sc->arpcom.ac_if.if_bpf, ifp, DLT_EN10MB,
	    sizeof(struct ether_header));
#else
	bpfattach(&sc->sc_ethercom.ec_if.if_bpf, ifp, DLT_EN10MB,
	    sizeof(struct ether_header));
#endif
#endif


}


static int
rtl_ether_ioctl(ifp, cmd, data)
	struct ifnet *ifp;
	FXP_IOCTLCMD_TYPE cmd;
	caddr_t data;
{
	struct ifaddr *ifa = (struct ifaddr *) data;
	struct nic *sc = ifp->if_softc;
	int error = 0;
	
	int s;
	s = splimp();
		
	switch (cmd) {
#ifdef PMON
	case SIOCPOLL:
		break;
#endif
	case SIOCSIFADDR:

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			error = rtl8139_init(sc);
			if(error == -1)
				return(error);
			//printf("set ipaddr\n");
			ifp->if_flags |= IFF_UP;

#ifdef __OpenBSD__
			arp_ifinit(&sc->arpcom, ifa);
#else
			arp_ifinit(ifp, ifa);
#endif
			
			break;
#endif

		default:
			rtl8139_init(sc);
			ifp->if_flags |= IFF_UP;
			break;
		}
		break;
	case SIOCSIFFLAGS:

		/*
		 * If interface is marked up and not running, then start it.
		 * If it is marked down and running, stop it.
		 * XXX If it's up then re-initialize it. This is so flags
		 * such as IFF_PROMISC are handled.
		 */
		if (ifp->if_flags & IFF_UP) {
			rtl8139_init(sc);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				;//fxp_stop(sc, 1);
		}
		break;
	case SIOCETHTOOL:
	{
	long *p=data;
	mynic = sc;
	cmd_setmac(p[0],p[1]);
	}
	break;
	case SIOCGETHERADDR:
	{
		long long val;
		char *p=data;
		mynic = sc;
		val =rtl_read_mac(mynic);
		p[5] = val>>40&0xff; 
		p[4] = val>>32&0xff; 
		p[3] = val>>24&0xff; 
		p[2] = val>>16&0xff; 
		p[1] = val>>8&0xff; 
		p[0] = val&0xff; 

	}
	break;
       case SIOCRDEEPROM:
                {
                long *p=data;
                mynic = sc;
                cmd_reprom(p[0],p[1]);
                }
                break;
       case SIOCWREEPROM:
                {
                long *p=data;
                mynic = sc;
                cmd_wrprom(p[0],p[1]);
                }
                break;
	default:
		error = EINVAL;
	}

	splx(s);
	return (error);
}
static void rtl8139_weird(struct nic *nic , int status, int link_changed)
{
	return ;
}
static int max_interrupt_work = 5; //if this too big, the network speed will be too slow,
				  //otherwise, No buffer available will happen	
int 
rtl8139_intr(void *arg)
{
	
	struct nic *nic = (struct nic *)arg;
	struct ifnet *ifp = &nic->arpcom.ac_if;
	int boguscnt = max_interrupt_work;
	int ackstat, status;
	int link_changed = 0; 
	int dowork;
	

	do {
		status = RTL_READ_2 (nic, IntrStatus);
		
		if (status == 0xFFFF)
			break;

		if ((status &
		     (PCIErr | PCSTimeout | RxUnderrun | RxOverflow |
		      RxFIFOOver | TxErr | TxOK | RxErr | RxOK)) == 0)
			break;

		/* Acknowledge all of the current interrupt sources ASAP, but
		   an first get an additional status bit from CSCR. */
		if (status & RxUnderrun)
			link_changed = RTL_READ_2 (nic, CSCR) & CSCR_LinkChangeBit;

		/* The chip takes special action when we clear RxAckBits,
		 * so we clear them later in rtl8139_rx_interrupt
		 */
		ackstat = status & ~(RxAckBits | TxErr);
		RTL_WRITE_2 (nic, IntrStatus, ackstat);
		DPRINTF ("interrupt  status=%#4.4x ackstat=%#4.4x new intstat=%#4.4x.\n",
			  ackstat, status, RTL_READ_2 (nic, IntrStatus));

		if (ifp->if_flags & IFF_RUNNING) 
			rtl8139_poll ( nic);

		/* Check uncommon events with one test. */
		if (status & (PCIErr | PCSTimeout | RxUnderrun | RxOverflow |
			  	      RxFIFOOver | RxErr))
			rtl8139_weird(nic,  status, link_changed);

		
		if ((ifp->if_flags  & IFF_RUNNING) && (status &(TxOK | TxErr))) {
			dowork = rtl8139_tx_intr(nic);
			if (status & TxErr)
				RTL_WRITE_2 (nic, IntrStatus, TxErr);
			if(dowork ==1)
				rtl8139_transmit(ifp);
		}

		boguscnt--;
	} while (boguscnt > 0);

	if (boguscnt <= 0) {
		DPRINTF ("Too much work at interrupt, "
			"IntrStatus=0x%4.4x.\n",  status);

		/* Clear all interrupt sources. */
		RTL_WRITE_2 (nic, IntrStatus, 0xffff);
	}

	return 1; 
}

void rtl8139_stop(void)
{
	if (mynic) {
		RTL_WRITE_1 (mynic, ChipCmd, 0);
		RTL_WRITE_1 (mynic, ChipCmd, CmdReset);
	}
}


#if 1
#include <pmon.h>
int cmd_ifm(int ac, char *av[])
{
	unsigned  int speed10, fullduplex;
	struct nic *nic = mynic;
	
	if(nic == NULL){
		printf("8139 interface not initialized\n");
		return 0;
	}
	
	if(ac !=1 && ac!=2){
		printf("usage: ifm [100|10|auto]\n");		
		return 0;
	}
	if(ac == 1){
		speed10 = RTL_READ_1(nic,  MediaStatus) & MSRSpeed10;
		fullduplex = RTL_READ_2(nic,  MII_BMCR) & BMCRDuplex;
		printf(" %sMbps %s-DUPLEX.\n", speed10 ? "10" : "100",
						fullduplex ? "FULL" : "HALF");
		return 0;
	}
			
	if(strcmp("100", av[1]) == 0){
		mdio_write(nic, 40, 0, 0x2100);	
	} else if(strcmp("10", av[1]) ==0){
		mdio_write(nic, 40, 0, 0x0100);
	} else if(strcmp("auto", av[1])==0){
		mdio_write(nic, 40, 0, 0x1000);		
	}
	return 0;		
}

int cmd_setmac(int ac, char *av[])
{
	int i;
	unsigned short val = 0, v;
	struct nic *nic = mynic;
	
	if(nic == NULL){
		printf("8139 interface not initialized\n");
		return 0;
	}
#if 0
	if (ac != 4) {
		printf("MAC ADDRESS ");
		for(i=0; i<6; i++){
			printf("%02x",nic->arpcom.ac_enaddr[i]);
			if(i==5)
				printf("\n");
			else
				printf(":");
		}
		printf("Use \"setmac word1(16bit) word2 word3\"\n");
		return 0;
	}
	printf("set mac to ");
	for (i = 0; i < 3; i++) {
		val = strtoul(av[i+1],0,0);
		printf("%04x ", val);
		write_eeprom(ioaddr, 0x7 + i, val);
	}
	printf("\n");
	printf("The machine should be restarted to make the mac change to take effect!!\n");
#else
	if(ac != 2){
	long long macaddr;
	u_int8_t *paddr;
	u_int8_t enaddr[6];
	macaddr=rtl_read_mac(nic);
	paddr=(uint8_t*)&macaddr;
	enaddr[0] = paddr[5- 0];
	enaddr[1] = paddr[5- 1];
	enaddr[2] = paddr[5- 2];
	enaddr[3] = paddr[5- 3];
	enaddr[4] = paddr[5- 4];
	enaddr[5] = paddr[5- 5];
		printf("MAC ADDRESS ");
		for(i=0; i<6; i++){
			printf("%02x",enaddr[i]);
			if(i==5)
				printf("\n");
			else
				printf(":");
		}
		printf("Use \"setmac <mac> \" to set mac address\n");
		return 0;
	}
	for (i = 0; i < 3; i++) {
		val = 0;
		gethex(&v, av[1], 2);
		val = v ;
		av[1]+=3;

		gethex(&v, av[1], 2);
		val = val | (v << 8);
		av[1] += 3;

#ifndef EPLC46
		write_eeprom(ioaddr, 0x7 + i, val);
#else 
		write_eeprom8(ioaddr, (0x7 +i) *2, val & 0xff);
		write_eeprom8(ioaddr, (0x7 +i) *2+1, (val >> 8) & 0xff);
#endif

	}
#endif
	return 0;
}

int cmd_reprom(int ac, char *av)
{
	int i;
	unsigned short data;	

	printf("dump eprom:\n");

	for(i=0; i< 64; i++){
#ifndef EPLC46
		data = read_eeprom(mynic, ioaddr, i);
#else
		data = read_eeprom(mynic, ioaddr, 2*i);
		data = data | (read_eeprom(mynic, ioaddr, 2*i+1)) << 8;
#endif
		printf("%04x ", data);
		if((i+1)%8 == 0)
			printf("\n");
	}
	return 0;
}

#if 1
static unsigned long next = 1;

           /* RAND_MAX assumed to be 32767 */
static int myrand(void) {
               next = next * 1103515245 + 12345;
               return((unsigned)(next/65536) % 32768);
           }

static void mysrand(unsigned int seed) {
               next = seed;
           }
#endif

int cmd_wrprom(int ac,char **av)
{
        int i=0;
        unsigned long clocks_num=0;
        unsigned char tmp[4];
        unsigned short eeprom_data;
        unsigned short rom[64]={
				0x8129, 0x10ec, 0x8139, 0x10ec, 0x8139, 0x4020, 0xe512, 0x0a00,
        			0x56eb, 0x135b, 0x4d15, 0xf7c2, 0x8801, 0x03b9, 0x60f4, 0x071a,
				0xdfa3, 0x9836, 0xdfa3, 0x9836, 0x03b9, 0x60f4, 0x1a1a, 0x1a1a,
				0x0000, 0xb6e3, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2000,
				0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
				0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
				0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
				0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000                        
                        };

        printf("Now beginningwrite whole eprom\n");

#if 1
                clocks_num =CPU_GetCOUNT(); // clock();
                mysrand(clocks_num);
                for( i = 0; i < 4;i++ )
                {
                        tmp[i]=myrand()%256;
                        printf( " tmp[%d]=02x%x\n", i,tmp[i]);
                }
                eeprom_data =tmp[1] |( tmp[0]<<8);
		rom[8] = eeprom_data;
                printf("eeprom_data [8] = 0x%4x\n",eeprom_data);
                eeprom_data =tmp[3] |( tmp[2]<<8);
		rom[9] = eeprom_data;
                printf("eeprom_data [9] = 0x%4x\n",eeprom_data);
#endif
	if(ac>1)
	{
	 //offset:data,data
	 int i;
	 int offset;
	 int data;
	 for(i=1;i<ac;i++)
	 {
	 	char *p=av[i];
		char *nextp;
	 	int offset=strtoul(p,&nextp,0);
		while(nextp!=p)
		{
		p=++nextp;
		data=strtoul(p,&nextp,0);
		if(nextp==p)break;
		rom[offset++]=data;
		}
	 }
	}
        for(i=0; i< 64; i++)
        {
                eeprom_data = rom[i];
                write_eeprom(ioaddr, i, eeprom_data);
        }

        printf("Write the whole eeprom OK!\n");
        return 0;
}

int netdmp_cmd (int ac, char *av[])
{
	struct ifnet *ifp;
	int i;
	
	ifp = &mynic->arpcom.ac_if;
	printf("if_snd.mb_head: %x\n", ifp->if_snd.ifq_head);	
	printf("if_snd.ifq_snd.ifqlen =%d\n", ifp->if_snd.ifq_len);
	printf("ChipCmd= %x\n", RTL_READ_1(mynic, ChipCmd));
	printf("ifnet address=%8x\n", ifp);
	printf("if_flags = %x\n", ifp->if_flags);
	printf("Intr =%x\n", RTL_READ_2(mynic, IntrStatus));
	printf("TxConfig =%x\n", RTL_READ_4(mynic, TxConfig));
	printf("RxConfig =%x\n", RTL_READ_4(mynic, RxConfig));
	printf("RxBufPtr= %x\n", RTL_READ_2(mynic, RxBufPtr));
	printf("RxBufAddr =%x\n", RTL_READ_2(mynic, RxBufAddr));
	printf("cur_rx =%x\n", cur_rx);
	printf("rx_ring: %x\n",mynic->rx_dma);
	printf("tx_dma: %x\n",mynic->tx_dma);
	printf("cur_tx =%d, dirty_tx=%d\n", cur_tx, dirty_tx);
	for (i =0; i<4; i++){
		printf("Txstatus[%d]=%x\n", i, RTL_READ_4(mynic, TxStatus0+i*4));
	}
	if(ac==2){
		
		if(strcmp(av[1], "on")==0){
			db8139=1;
		}
		else if(strcmp(av[1], "off")==0){
			db8139=0;
		}else {
			int x=atoi(av[1]);
			max_interrupt_work=x;
		}
		
	}
	printf("db8139=%d\n",db8139);
	return 0;
}
		       
static const Optdesc netdmp_opts[] =
{
    {"<interface>", "Interface name"},
    {"<netdmp>", "IP Address"},
    {0}
};

static const Cmd Cmds[] =
{
	{"8139"},
	{"netdmp",	"",
			0,
			"8139 helper", netdmp_cmd, 1, 3, 0},
	{"ifm", "", NULL,
		    "Set 8139 interface mode", cmd_ifm, 1, 2, 0},
	{"setmac", "", NULL,
		    "Set mac address into 8139 eeprom", cmd_setmac, 1, 5, 0},
	{"readrom", "", NULL,
			"dump rtl8139 eprom content", cmd_reprom, 1, 1,0},
	{"writerom", "", NULL,
			"write the whole rtl8139 eprom content", cmd_wrprom, 1, 1,0},
	{0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

#endif 

#endif
