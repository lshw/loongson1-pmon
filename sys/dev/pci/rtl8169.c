/*
=========================================================================
 r8169.c: A RealTek RTL-8169 Gigabit Ethernet driver for Linux kernel 2.4.x.
 --------------------------------------------------------------------
 Taken from linux driver for loongson BIOS
 Copy right http://www.lemote.com, Yanhua

 History:
 Feb  4 2002	- created initially by ShuChen <shuchen@realtek.com.tw>.
 May 20 2002	- Add link status force-mode and TBI mode support.
        2004	- Massive updates. See kernel SCM system for details.
=========================================================================
  1. [DEPRECATED: use ethtool instead] The media can be forced in 5 modes.
	 Command: 'insmod r8169 media = SET_MEDIA'
	 Ex:	  'insmod r8169 media = 0x04' will force PHY to operate in 100Mpbs Half-duplex.
	
	 SET_MEDIA can be:
 		_10_Half	= 0x01
 		_10_Full	= 0x02
 		_100_Half	= 0x04
 		_100_Full	= 0x08
 		_1000_Full	= 0x10
  
  2. Support TBI mode.
=========================================================================
VERSION 1.1	<2002/10/4>

	The bit4:0 of MII register 4 is called "selector field", and have to be
	00001b to indicate support of IEEE std 802.3 during NWay process of
	exchanging Link Code Word (FLP). 

VERSION 1.2	<2002/11/30>

	- Large style cleanup
	- Use ether_crc in stock kernel (linux/crc32.h)
	- Copy mc_filter setup code from 8139cp
	  (includes an optimization, and avoids set_bit use)

VERSION 1.6LK	<2004/04/14>

	- Merge of Realtek's version 1.6
	- Conversion to DMA API
	- Suspend/resume
	- Endianness
	- Misc Rx/Tx bugs

VERSION 2.2LK	<2005/01/25>

	- RX csum, TX csum/SG, TSO
	- VLAN
	- baby (< 7200) Jumbo frames support
	- Merge of Realtek's version 2.2 (new phy)
 */
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

#define RTLDBG printf("TRACE: %s, %d\n", __FUNCTION__, __LINE__);


#include <sys/linux/types.h>

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

//#define	EPLC46	1

#ifdef CONFIG_R8169_NAPI
#define NAPI_SUFFIX	"-NAPI"
#else
#define NAPI_SUFFIX	""
#endif

#define RTL8169_VERSION "2.2LK" NAPI_SUFFIX
#define MODULENAME "r8169"
#define PFX MODULENAME ": "

#define RTL8169_DEBUG

#ifdef RTL8169_DEBUG
#define assert(expr) \
        if(!(expr)) {					\
	        printf( "Assertion failed! %s,%s,%s,line=%d\n",	\
        	#expr,__FILE__,__FUNCTION__,__LINE__);		\
        }
#define dprintk(fmt, args...)	do { printf(PFX fmt, ## args); } while (0)
#else
#define assert(expr) do {} while (0)
#define dprintk(fmt, args...)	do {} while (0)
#endif /* RTL8169_DEBUG */

#define R8169_MSG_DEFAULT \
	(NETIF_MSG_DRV | NETIF_MSG_PROBE | NETIF_MSG_IFUP | NETIF_MSG_IFDOWN)

#define TX_BUFFS_AVAIL(tp) \
	(tp->dirty_tx + NUM_TX_DESC - tp->cur_tx - 1)

#ifdef CONFIG_R8169_NAPI
#define rtl8169_rx_skb			netif_receive_skb
#define rtl8169_rx_hwaccel_skb		vlan_hwaccel_receive_skb
#define rtl8169_rx_quota(count, quota)	min(count, quota)
#else
#define rtl8169_rx_skb			netif_rx
#define rtl8169_rx_hwaccel_skb		vlan_hwaccel_rx
#define rtl8169_rx_quota(count, quota)	count
#endif

/* Maximum events (Rx packets, etc.) to handle at each interrupt. */
static int max_interrupt_work = 20;
//static const int max_interrupt_work = 200; //lihui.

/* Maximum number of multicast addresses to filter (vs. Rx-all-multicast).
   The RTL chips use a 64 element hash table based on the Ethernet CRC. */
static const int multicast_filter_limit = 32;

/* MAC address length */
#define MAC_ADDR_LEN	6

#define RX_FIFO_THRESH	7	/* 7 means NO threshold, Rx buffer level before first PCI xfer. */
#define RX_DMA_BURST	6	/* Maximum PCI burst, '6' is 1024 */
#define TX_DMA_BURST	6	/* Maximum PCI burst, '6' is 1024 */
#define EarlyTxThld 	0x3F	/* 0x3F means NO early transmit */
#define RxPacketMaxSize	0x3FE8	/* 16K - 1 - ETH_HLEN - VLAN - CRC... */
#define SafeMtu		0x1c20	/* ... actually life sucks beyond ~7k */
#define InterFrameGap	0x03	/* 3 means InterFrameGap = the shortest one */

#define R8169_REGS_SIZE		256
#define R8169_NAPI_WEIGHT	64
//#define NUM_TX_DESC	64	/* Number of Tx descriptor registers */
//#define NUM_RX_DESC	256	/* Number of Rx descriptor registers */
#define NUM_TX_DESC  64	//it seems likely that I could not allocate too many buffers in pmon. I have tested 
#define NUM_RX_DESC  128  //several times and these 2 values may be a good choice.lihui@mail.ustc.edu.cn
#define R8169_TX_RING_BYTES	(NUM_TX_DESC * sizeof(struct TxDesc))
#define R8169_RX_RING_BYTES	(NUM_RX_DESC * sizeof(struct RxDesc))

#define RTL8169_TX_TIMEOUT	(6*HZ)
#define RTL8169_PHY_TIMEOUT	(10*HZ)

/* write/read MMIO register */
#define writeb(val, addr) (*(volatile u8*)(addr) = (val))
#define writew(val, addr) (*(volatile u16*)(addr) = (val))
#define writel(val, addr) (*(volatile u32*)(addr) = (val))
#define readb(addr) (*(volatile u8*)(addr))
#define readw(addr) (*(volatile u16*)(addr))
#define readl(addr) (*(volatile u32*)(addr))

#define RTL_W8(tp, reg, val8)	writeb ((val8), tp->ioaddr + (reg))
#define RTL_W16(tp, reg, val16)	writew ((val16), tp->ioaddr + (reg))
#define RTL_W32(tp, reg, val32)	writel ((val32), tp->ioaddr + (reg))
#define RTL_R8(tp, reg)		readb (tp->ioaddr + (reg))
#define RTL_R16(tp, reg)		readw (tp->ioaddr + (reg))
#define RTL_R32(tp, reg)		((unsigned long) readl (tp->ioaddr + (reg)))

#define le32_to_cpu(x) (x)
#define le64_to_cpu(x) (x)
#define cpu_to_le32(x) (x)
#define cpu_to_le64(x) (x)

#define ETH_HLEN 14
#define ETH_ZLEN 60
#define 		NET_IP_ALIGN 2	

#define MAX_FRAGS  (65536/16*1024)
#define ETH_MAX_PACKET 1536
#define TX_BUF_SIZE     ETH_MAX_PACKET
#define RX_BUF_SIZE     ETH_MAX_PACKET  

static int if_in_attach;
static int if_frequency;

typedef unsigned long long u64;

struct TxDesc {
	u32 opts1;
	u32 opts2;
       u64 addr;
};

struct RxDesc {
	u32 opts1;
	u32 opts2;
       u64 addr;
};


struct ring_info {
	struct mbuf *mb;
	u32		len;
	u8		__pad[sizeof(void *) - sizeof(u32)];
};

struct ethtool_cmd {
	u32   cmd;
	u32   supported;  /* Features this interface supports */
	u32   advertising;    /* Features this interface advertises */
	u16   speed;      /* The forced speed, 10Mb, 100Mb, gigabit */
	u8    duplex;     /* Duplex, half or full */
	u8    port;       /* Which connector port */
	u8    phy_address;
	u8    transceiver;    /* Which transceiver to use */
	u8    autoneg;    /* Enable or disable autonegotiation */
	u32   maxtxpkt;   /* Tx pkts before generating tx int */
	u32   maxrxpkt;   /* Rx pkts before generating rx int */
	u32   reserved[4];
};


struct net_device_stats
{
	unsigned long	rx_packets;		/* total packets received	*/
	unsigned long	tx_packets;		/* total packets transmitted	*/
	unsigned long	rx_bytes;		/* total bytes received 	*/
	unsigned long	tx_bytes;		/* total bytes transmitted	*/
	unsigned long	rx_errors;		/* bad packets received		*/
	unsigned long	tx_errors;		/* packet transmit problems	*/
	unsigned long	rx_dropped;		/* no space in linux buffers	*/
	unsigned long	tx_dropped;		/* no space available in linux	*/
	unsigned long	multicast;		/* multicast packets received	*/
	unsigned long	collisions;

	/* detailed rx_errors: */
	unsigned long	rx_length_errors;
	unsigned long	rx_over_errors;		/* receiver ring buff overflow	*/
	unsigned long	rx_crc_errors;		/* recved pkt with crc error	*/
	unsigned long	rx_frame_errors;	/* recv'd frame alignment error */
	unsigned long	rx_fifo_errors;		/* recv'r fifo overrun		*/
	unsigned long	rx_missed_errors;	/* receiver missed packet	*/

	/* detailed tx_errors */
	unsigned long	tx_aborted_errors;
	unsigned long	tx_carrier_errors;
	unsigned long	tx_fifo_errors;
	unsigned long	tx_heartbeat_errors;
	unsigned long	tx_window_errors;
	
	/* for cslip etc */
	unsigned long	rx_compressed;
	unsigned long	tx_compressed;
};

struct rtl8169_private {
	struct device sc_dev;		/* generic device structures */
	void *sc_ih;			/* interrupt handler cookie */
	bus_space_tag_t sc_st;		/* bus space tag */
	bus_space_handle_t sc_sh;	/* bus space handle */
	pci_chipset_tag_t sc_pc;	/* chipset handle needed by mips */

	struct pci_attach_args *pa; 

	struct arpcom arpcom;		/* per-interface network data !!we use this*/
	struct mii_data sc_mii;		/* MII media information */
	unsigned char   dev_addr[6]; //the net interface's address
	unsigned long ioaddr;

	struct net_device_stats stats;
	int flags;
	int mc_count;
	void *mmio_addr;	/* memory map physical address */
	u32 msg_enable;
	int chipset;
	int mac_version;
	int phy_version;
	u32 cur_rx; /* Index into the Rx descriptor buffer of next Rx pkt. */
	u32 cur_tx; /* Index into the Tx descriptor buffer of next Rx pkt. */
	u32 dirty_rx;
	u32 dirty_tx;
	struct TxDesc *TxDescArray;	/* 256-aligned Tx descriptor ring */
	struct RxDesc *RxDescArray;	/* 256-aligned Rx descriptor ring */
	unsigned long TxPhyAddr;
	unsigned long RxPhyAddr;
	unsigned char *rx_buffer[NUM_RX_DESC];	/* Rx data buffers */
	unsigned char *tx_buffer[NUM_TX_DESC];	/* Tx data buffers */
	unsigned rx_buf_sz;
	unsigned tx_buf_sz;
	u16 cp_cmd;
	u16 intr_mask;
	int phy_auto_nego_reg;
	int phy_1000_ctrl_reg;
#ifdef CONFIG_R8169_VLAN
	struct vlan_group *vlgrp;
#endif
	int (*set_speed)(struct rtl8169_private *, u8 autoneg, u16 speed, u8 duplex);
	void (*get_settings)(struct rtl8169_private *, struct ethtool_cmd *);
	void (*phy_reset_enable)(struct rtl8169_private *);
	unsigned int (*phy_reset_pending)(struct rtl8169_private *);
	unsigned int (*link_ok)(struct rtl8169_private *);
	unsigned wol_enabled : 1;
};

typedef struct rtl8169_private r8169;

static struct mbuf * getmbuf(struct rtl8169_private *tp)
{
	struct mbuf *m;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if(m == NULL){
		printf("getmbuf failed, Out of memory!!!\n");
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
		pci_sync_cache(tp->sc_pc, (vm_offset_t)m->m_ext.ext_buf,
				MCLBYTES, SYNC_R);
	}
#define RFA_ALIGNMENT_FUDGE 2
	m->m_data += RFA_ALIGNMENT_FUDGE;
#else
	m->m_data += RFA_ALIGNMENT_FUDGE;
#endif
	return m;
}


static int r8169_match( struct device *, void   *match, void * aux);
static void r8169_attach(struct device * , struct device * , void *);

struct cfattach rtk_ca = {
	sizeof(r8169), r8169_match, r8169_attach, 
};

struct cfdriver rtk_cd = {
	NULL, "rtk", DV_IFNET,
};



enum mac_version {
	RTL_GIGA_MAC_VER_B = 0x00,
	/* RTL_GIGA_MAC_VER_C = 0x03, */
	RTL_GIGA_MAC_VER_D = 0x01,
	RTL_GIGA_MAC_VER_E = 0x02,
	RTL_GIGA_MAC_VER_X = 0x04	/* Greater than RTL_GIGA_MAC_VER_E */
};

enum phy_version {
	RTL_GIGA_PHY_VER_C = 0x03, /* PHY Reg 0x03 bit0-3 == 0x0000 */
	RTL_GIGA_PHY_VER_D = 0x04, /* PHY Reg 0x03 bit0-3 == 0x0000 */
	RTL_GIGA_PHY_VER_E = 0x05, /* PHY Reg 0x03 bit0-3 == 0x0000 */
	RTL_GIGA_PHY_VER_F = 0x06, /* PHY Reg 0x03 bit0-3 == 0x0001 */
	RTL_GIGA_PHY_VER_G = 0x07, /* PHY Reg 0x03 bit0-3 == 0x0002 */
	RTL_GIGA_PHY_VER_H = 0x08, /* PHY Reg 0x03 bit0-3 == 0x0003 */
};


#define _R(NAME,MAC,MASK) \
	{ .name = NAME, .mac_version = MAC, .RxConfigMask = MASK }

static const struct {
	const char *name;
	u8 mac_version;
	u32 RxConfigMask;	/* Clears the bits supported by this chip */
} rtl_chip_info[] = {
	_R("RTL8169",		RTL_GIGA_MAC_VER_B, 0xff7e1880),
	_R("RTL8169s/8110s",	RTL_GIGA_MAC_VER_D, 0xff7e1880),
	_R("RTL8169s/8110s",	RTL_GIGA_MAC_VER_E, 0xff7e1880),
	_R("RTL8169s/8110s",	RTL_GIGA_MAC_VER_X, 0xff7e1880),
};
#undef _R


enum RTL8169_registers {
	MAC0 = 0,		/* Ethernet hardware address. */
	MAR0 = 8,		/* Multicast filter. */
	CounterAddrLow = 0x10,
	CounterAddrHigh = 0x14,
	TxDescStartAddrLow = 0x20,
	TxDescStartAddrHigh = 0x24,
	TxHDescStartAddrLow = 0x28,
	TxHDescStartAddrHigh = 0x2c,
	FLASH = 0x30,
	ERSR = 0x36,
	ChipCmd = 0x37,
	TxPoll = 0x38,
	IntrMask = 0x3C,
	IntrStatus = 0x3E,
	TxConfig = 0x40,
	RxConfig = 0x44,
	RxMissed = 0x4C,
	Cfg9346 = 0x50,
	Config0 = 0x51,
	Config1 = 0x52,
	Config2 = 0x53,
	Config3 = 0x54,
	Config4 = 0x55,
	Config5 = 0x56,
	MultiIntr = 0x5C,
	PHYAR = 0x60,
	TBICSR = 0x64,
	TBI_ANAR = 0x68,
	TBI_LPAR = 0x6A,
	PHYstatus = 0x6C,
	RxMaxSize = 0xDA,
	CPlusCmd = 0xE0,
	IntrMitigate = 0xE2,
	RxDescAddrLow = 0xE4,
	RxDescAddrHigh = 0xE8,
	EarlyTxThres = 0xEC,
	FuncEvent = 0xF0,
	FuncEventMask = 0xF4,
	FuncPresetState = 0xF8,
	FuncForceEvent = 0xFC,
};

enum RTL8169_register_content {
	/* InterruptStatusBits */
	SYSErr = 0x8000,
	PCSTimeout = 0x4000,
	SWInt = 0x0100,
	TxDescUnavail = 0x80,
	RxFIFOOver = 0x40,
	LinkChg = 0x20,
	RxOverflow = 0x10,
	TxErr = 0x08,
	TxOK = 0x04,
	RxErr = 0x02,
	RxOK = 0x01,

	/* RxStatusDesc */
	RxRES = 0x00200000,
	RxCRC = 0x00080000,
	RxRUNT = 0x00100000,
	RxRWT = 0x00400000,

	/* ChipCmdBits */
	CmdReset = 0x10,
	CmdRxEnb = 0x08,
	CmdTxEnb = 0x04,
	RxBufEmpty = 0x01,

	/* Cfg9346Bits */
	Cfg9346_Lock = 0x00,
	Cfg9346_Unlock = 0xC0,
	Cfg9346_Autoload = 0x40,

	/* rx_mode_bits */
	AcceptErr = 0x20,
	AcceptRunt = 0x10,
	AcceptBroadcast = 0x08,
	AcceptMulticast = 0x04,
	AcceptMyPhys = 0x02,
	AcceptAllPhys = 0x01,

	/* RxConfigBits */
	RxCfgFIFOShift = 13,
	RxCfgDMAShift = 8,

	/* TxConfigBits */
	TxInterFrameGapShift = 24,
	TxDMAShift = 8,	/* DMA burst value (0-7) is shift this many bits */

	/* Config1 register p.24 */
	PMEnable	= (1 << 0),	/* Power Management Enable */

	/* Config3 register p.25 */
	MagicPacket	= (1 << 5),	/* Wake up when receives a Magic Packet */
	LinkUp		= (1 << 4),	/* Wake up when the cable connection is re-established */

	/* Config5 register p.27 */
	BWF		= (1 << 6),	/* Accept Broadcast wakeup frame */
	MWF		= (1 << 5),	/* Accept Multicast wakeup frame */
	UWF		= (1 << 4),	/* Accept Unicast wakeup frame */
	LanWake		= (1 << 1),	/* LanWake enable/disable */
	PMEStatus	= (1 << 0),	/* PME status can be reset by PCI RST# */

	/* TBICSR p.28 */
	TBIReset	= 0x80000000,
	TBILoopback	= 0x40000000,
	TBINwEnable	= 0x20000000,
	TBINwRestart	= 0x10000000,
	TBILinkOk	= 0x02000000,
	TBINwComplete	= 0x01000000,

	/* CPlusCmd p.31 */
	RxVlan		= (1 << 6),
	RxChkSum	= (1 << 5),
	PCIDAC		= (1 << 4),
	PCIMulRW	= (1 << 3),

	/* rtl8169_PHYstatus */
	TBI_Enable = 0x80,
	TxFlowCtrl = 0x40,
	RxFlowCtrl = 0x20,
	_1000bpsF = 0x10,
	_100bps = 0x08,
	_10bps = 0x04,
	LinkStatus = 0x02,
	FullDup = 0x01,

	/* GIGABIT_PHY_registers */
	PHY_CTRL_REG = 0,
	PHY_STAT_REG = 1,
	PHY_AUTO_NEGO_REG = 4,
	PHY_1000_CTRL_REG = 9,

	/* GIGABIT_PHY_REG_BIT */
	PHY_Restart_Auto_Nego = 0x0200,
	PHY_Enable_Auto_Nego = 0x1000,

	/* PHY_STAT_REG = 1 */
	PHY_Auto_Neco_Comp = 0x0020,

	/* PHY_AUTO_NEGO_REG = 4 */
	PHY_Cap_10_Half = 0x0020,
	PHY_Cap_10_Full = 0x0040,
	PHY_Cap_100_Half = 0x0080,
	PHY_Cap_100_Full = 0x0100,

	/* PHY_1000_CTRL_REG = 9 */
	PHY_Cap_1000_Full = 0x0200,

	PHY_Cap_Null = 0x0,

	/* _MediaType */
	_10_Half = 0x01,
	_10_Full = 0x02,
	_100_Half = 0x04,
	_100_Full = 0x08,
	_1000_Full = 0x10,

	/* _TBICSRBit */
	TBILinkOK = 0x02000000,

	/* DumpCounterCommand */
	CounterDump = 0x8,
};

enum _DescStatusBit {
	DescOwn		= (1 << 31), /* Descriptor is owned by NIC */
	RingEnd		= (1 << 30), /* End of descriptor ring */
	FirstFrag	= (1 << 29), /* First segment of a packet */
	LastFrag	= (1 << 28), /* Final segment of a packet */

	/* Tx private */
	LargeSend	= (1 << 27), /* TCP Large Send Offload (TSO) */
	MSSShift	= 16,        /* MSS value position */
	MSSMask		= 0xfff,     /* MSS value + LargeSend bit: 12 bits */
	IPCS		= (1 << 18), /* Calculate IP checksum */
	UDPCS		= (1 << 17), /* Calculate UDP/IP checksum */
	TCPCS		= (1 << 16), /* Calculate TCP/IP checksum */
	TxVlanTag	= (1 << 17), /* Add VLAN tag */

	/* Rx private */
	PID1		= (1 << 18), /* Protocol ID bit 1/2 */
	PID0		= (1 << 17), /* Protocol ID bit 2/2 */

#define RxProtoUDP	(PID1)
#define RxProtoTCP	(PID0)
#define RxProtoIP	(PID1 | PID0)
#define RxProtoMask	RxProtoIP

	IPFail		= (1 << 16), /* IP checksum failed */
	UDPFail		= (1 << 15), /* UDP/IP checksum failed */
	TCPFail		= (1 << 14), /* TCP/IP checksum failed */
	RxVlanTag	= (1 << 16), /* VLAN tag available */
};

/* media options */
#define MAX_UNITS 8
static int media[MAX_UNITS] = { _100_Full, -1, -1, -1, -1, -1, -1, -1 };

#define RsvdMask	0x3fffc000

static void rtl8169_start_xmit(struct ifnet *ifp);
static int rtl8169_interrupt(void *arg);
static int rtl8169_init_ring(struct rtl8169_private *tp);
static void rtl8169_hw_start(struct rtl8169_private *tp);
static int rtl8169_close(struct rtl8169_private *tp);
static void rtl8169_set_rx_mode(struct rtl8169_private *tp);
static int rtl8169_rx_interrupt(struct rtl8169_private *);
static void rtl8169_down(struct rtl8169_private *tp);

#ifdef CONFIG_R8169_NAPI
static int rtl8169_poll(struct rtl8169_private *tp, int *budget);
#endif

static const u16 rtl8169_intr_mask =
/*	SYSErr | */LinkChg | RxOverflow | RxFIFOOver | TxErr | TxOK | RxErr | RxOK;
static const u16 rtl8169_napi_event =
	RxOK | RxOverflow | RxFIFOOver | TxOK | TxErr;
static const unsigned int rtl8169_rx_config =
    (RX_FIFO_THRESH << RxCfgFIFOShift) | (RX_DMA_BURST << RxCfgDMAShift);

#define PHY_Cap_10_Half_Or_Less PHY_Cap_10_Half
#define PHY_Cap_10_Full_Or_Less PHY_Cap_10_Full | PHY_Cap_10_Half_Or_Less
#define PHY_Cap_100_Half_Or_Less PHY_Cap_100_Half | PHY_Cap_10_Full_Or_Less
#define PHY_Cap_100_Full_Or_Less PHY_Cap_100_Full | PHY_Cap_100_Half_Or_Less

#define PCI_VENDOR_ID_REALTEK       0x10ec
#define PCI_VENDOR_ID_DLINK         0x1186
#define R8110_DEV_ID1               0x8169
#define R8110_DEV_ID2 				0x8129
#define DLINK_DEV_ID  			    0x4300

static int r8169_match(
		struct device *parent, 
		void   *match,
		void * aux
		)
{
	struct pci_attach_args *pa = aux;

	switch(PCI_VENDOR(pa->pa_id)) {
	case PCI_VENDOR_ID_REALTEK:
		if(PCI_PRODUCT(pa->pa_id) == 0x8169 ||PCI_PRODUCT(pa->pa_id) == 0x8110
			|| PCI_PRODUCT(pa->pa_id) == 0x8129)
			return 1;
		else
			return 0;
	case PCI_VENDOR_ID_DLINK:
		if(PCI_PRODUCT(pa->pa_id) == 0x4300)
			return 1;
		else 
			return 0;
	default:
		return 0;
	}

	return 0;
}

static void mdio_write(struct rtl8169_private *tp, int RegAddr, int value)
{
	int i;

	RTL_W32(tp, PHYAR, 0x80000000 | (RegAddr & 0xFF) << 16 | value);

	for (i = 2000; i > 0; i--) {
		/* Check if the RTL8169 has completed writing to the specified MII register */
		if (!(RTL_R32(tp, PHYAR) & 0x80000000)) 
			break;
		delay(200);
	}
}

static int mdio_read(struct rtl8169_private *tp, int RegAddr)
{
	int i, value = -1;

	RTL_W32(tp, PHYAR, 0x0 | (RegAddr & 0xFF) << 16);
	
	for (i = 2000; i > 0; i--) {
		/* Check if the RTL8169 has completed retrieving data from the specified MII register */
		if (RTL_R32(tp, PHYAR) & 0x80000000) {
			value = (int) (RTL_R32(tp, PHYAR) & 0xFFFF);
			break;
		}
		delay(200);
	}
	return value;
}

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



/*  EEPROM_Ctrl bits. */
#define EE_CS           0x08    /* EEPROM chip select. */
#define EE_SHIFT_CLK    0x04    /* EEPROM shift clock. */
#define EE_DATA_WRITE   0x02    /* EEPROM chip data in. */
#define EE_WRITE_0      0x00
#define EE_WRITE_1      0x02
#define EE_DATA_READ    0x01    /* EEPROM chip data out. */
#define EE_ENB          (0x80 | EE_CS)

#define myoutb(v, a)  (*(volatile unsigned char *)(a) = (v))
#define myoutw(v, a)  (*(volatile unsigned short*)(a) = (v))
#define myoutl(v, a)  (*(volatile unsigned long *)(a) = (v))
#define eeprom_delay()  inl(ee_addr)
static void write_eeprom_enable(long ioaddr){
	int i;
	long ee_addr = ioaddr + Cfg9346;
	int  cmd = EE_WEN_CMD;

	myoutb(EE_ENB & ~EE_CS, ee_addr);
	myoutb(EE_ENB, ee_addr);

	/* Shift the read command bits out. */
#ifndef EPLC46
	for (i = 10; i >= 0; i--) {
		int dataval = (cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		myoutb(EE_ENB | dataval, ee_addr);
		eeprom_delay();
		myoutb(EE_ENB | dataval | EE_SHIFT_CLK, ee_addr);
		eeprom_delay();
	}
#else
	for (i = 11; i >= 0; i--) {
		int dataval = (cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		myoutb(EE_ENB | dataval, ee_addr);
		eeprom_delay();
		myoutb(EE_ENB | dataval | EE_SHIFT_CLK, ee_addr);
		eeprom_delay();
	}
#endif
	myoutb(~EE_CS, ee_addr);

}

static void write_eeprom_disable(unsigned long ioaddr)
{
	int i;
	long ee_addr = ioaddr + Cfg9346;
	int  cmd = EE_WDS_CMD;

	myoutb(EE_ENB & ~EE_CS, ee_addr);
	myoutb(EE_ENB, ee_addr);

	/* Shift the read command bits out. */

#ifndef EPLC46
	i=10;
#else
	i=11;
#endif
	for (; i >= 0; i--) {
		int dataval = (cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		myoutb(EE_ENB | dataval, ee_addr);
		eeprom_delay();
		myoutb(EE_ENB | dataval | EE_SHIFT_CLK, ee_addr);
		eeprom_delay();
	}
	myoutb(~EE_CS, ee_addr);
}


int rtl8169_write_eeprom(long ioaddr, int location,unsigned short data)
{
	int i;
	long ee_addr = ioaddr + Cfg9346;
	int  cmd = location | EE_WRITE_CMD;

	write_eeprom_enable(ioaddr);

	cmd <<=16;
	cmd |= data;
	myoutb(EE_ENB & ~EE_CS, ee_addr);
	myoutb(EE_ENB, ee_addr);

	/* Shift the read command bits out. */
	for (i = 26; i >= 0; i--) {
		int dataval = (cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		myoutb(EE_ENB | dataval, ee_addr);
		eeprom_delay();
		myoutb(EE_ENB | dataval | EE_SHIFT_CLK, ee_addr);
		eeprom_delay();
	}
	/* Terminate the EEPROM access. */

	myoutb(~EE_CS, ee_addr);
	eeprom_delay();

	myoutb(EE_ENB, ee_addr);
	 
	 {int timeout=1000;
    while( ! (inb(ee_addr) & EE_DATA_READ) ){
		myoutb(EE_ENB, ee_addr);
		eeprom_delay();
		delay(100); if(!timeout--)break;
	}
	}
	return 0;
}

int rtl8169_write_eeprom8(long ioaddr, int location,unsigned short data)
{
	int i;
	long ee_addr = ioaddr + Cfg9346;
	int  cmd = location | EE_WRITE_CMD;

	write_eeprom_enable(ioaddr);

	cmd <<= 8;
	cmd |= data;
	myoutb(EE_ENB & ~EE_CS, ee_addr);
	myoutb(EE_ENB, ee_addr);

	/* Shift the read command bits out. */
	for (i = 18; i >= 0; i--) {
		int dataval = (cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		myoutb(EE_ENB | dataval, ee_addr);
		eeprom_delay();
		myoutb(EE_ENB | dataval | EE_SHIFT_CLK, ee_addr);
		eeprom_delay();
	}
	/* Terminate the EEPROM access. */

//	myoutb(~EE_CS, ee_addr);
	eeprom_delay();

	myoutb(EE_ENB, ee_addr);
	 
	{int timeout=1000;
       while( ! (inb(ee_addr) & EE_DATA_READ) ){
		myoutb(EE_ENB, ee_addr);
		eeprom_delay();
		delay(100); if(!timeout--)break;
	}
	}
	return 0;
}

static int read_eeprom(struct rtl8169_private *tp, int RegAddr)
{
	int i;
	long ee_addr = tp->ioaddr + Cfg9346;
	int read_cmd = RegAddr | EE_READ_CMD;
	int retval = 0;	

	RTL_W8(tp, Cfg9346, EE_ENB & ~EE_CS);
    RTL_W8(tp, Cfg9346, EE_ENB);
	
#ifndef EPLC46
	/* Shift the read command bits out. */
	for (i = 10; i >= 0; i--) {
		int dataval = (read_cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		RTL_W8(tp, Cfg9346, EE_ENB | dataval);
		eeprom_delay();
		RTL_W8(tp, Cfg9346, EE_ENB | dataval | EE_SHIFT_CLK);
		eeprom_delay();
	}  
#else
	for (i = 11; i >= 0; i--) {
		int dataval = (read_cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		RTL_W8(tp, Cfg9346, EE_ENB | dataval);
		eeprom_delay();
		RTL_W8(tp, Cfg9346, EE_ENB | dataval | EE_SHIFT_CLK);
		eeprom_delay();
	}  
#endif
	RTL_W8(tp, Cfg9346, EE_ENB);
    eeprom_delay();

#ifndef EPLC46
	for( i = 16; i > 0; i--) {
		RTL_W8(tp, Cfg9346, EE_ENB | EE_SHIFT_CLK);
		delay(1);
		retval = (retval << 1) | ((RTL_R8(tp, Cfg9346) & EE_DATA_READ) ? 1 : 0);
		RTL_W8(tp, Cfg9346, EE_ENB);
		delay(1);
	}	
#else 
		for( i = 8; i > 0; i--) {
		RTL_W8(tp, Cfg9346, EE_ENB | EE_SHIFT_CLK);
		delay(1);
		retval = (retval << 1) | ((RTL_R8(tp, Cfg9346) & EE_DATA_READ) ? 1 : 0);
		RTL_W8(tp, Cfg9346, EE_ENB);
		delay(1);
    }
#endif 
	return retval;
}

static void rtl8169_irq_mask_and_ack(struct rtl8169_private *tp)
{
	RTL_W16(tp, IntrMask, 0x0000);

	RTL_W16(tp, IntrStatus, 0xffff);
}

static void rtl8169_asic_down(struct rtl8169_private *tp)
{
	RTL_W8(tp, ChipCmd, 0x00);
	rtl8169_irq_mask_and_ack(tp);
	RTL_R16(tp, CPlusCmd);
}

static unsigned int rtl8169_tbi_reset_pending(struct rtl8169_private *tp)
{
	return RTL_R32(tp, TBICSR) & TBIReset;
}

static unsigned int rtl8169_xmii_reset_pending(struct rtl8169_private *tp)
{
	return mdio_read(tp, 0) & 0x8000;
}

static unsigned int rtl8169_tbi_link_ok(struct rtl8169_private *tp)
{
	return RTL_R32(tp, TBICSR) & TBILinkOk;
}

static unsigned int rtl8169_xmii_link_ok(struct rtl8169_private *tp)
{
	return RTL_R8(tp, PHYstatus) & LinkStatus;
}

static void rtl8169_tbi_reset_enable(struct rtl8169_private *tp)
{
	RTL_W32(tp, TBICSR, RTL_R32(tp, TBICSR) | TBIReset);
}

static void rtl8169_xmii_reset_enable(struct rtl8169_private *tp)
{
	unsigned int val;

	val = (mdio_read(tp, PHY_CTRL_REG) | 0x8000) & 0xffff;
	mdio_write(tp, PHY_CTRL_REG, val);
}

static void rtl8169_check_link_status(struct rtl8169_private *tp)
{
    int i; 
    int old_value;
	if (tp->link_ok(tp)) {
		printf("r8110: link up :  ");
        printf("PHY status:0x%x\n",RTL_R8(tp,0x6c));
        if(RTL_R8(tp,0x6c) == if_frequency){
            return;
        }
	} else {
		printf("r8110: link down\n");
        //printf("PHY status:0x%x\n",RTL_R8(tp,0x6c));
	}
    //by liuqi we try to get the 8110 up
    if (if_in_attach != 1){
    for (i = 0; i<10; i++){
        printf("Trying the %d time--->\n", i);

	    old_value = mdio_read(tp, PHY_CTRL_REG);
        old_value |= 0x0200;       //set to re auto negotiation
        mdio_write(tp, PHY_CTRL_REG, old_value);
        delay(10000000);
        if(RTL_R8(tp,0x6c) == if_frequency){
            printf("Connect done!!\n");
            break; 
        }
    }
    }
}

#define SPEED_10    10
#define SPEED_100   100
#define SPEED_1000  1000
#define HALF_DUPLEX 1 
#define FULL_DUPLEX 2

/* Which connector port. */
#define PORT_TP         0x00
#define PORT_AUI        0x01
#define PORT_MII        0x02 
#define PORT_FIBRE      0x03
#define PORT_BNC        0x04
    
/* Which transceiver to use. */
#define XCVR_INTERNAL       0x00
#define XCVR_EXTERNAL       0x01
#define XCVR_DUMMY1     0x02 
#define XCVR_DUMMY2     0x03 
#define XCVR_DUMMY3     0x04

/* Indicates what features are supported by the interface. */
#define SUPPORTED_10baseT_Half      (1 << 0)
#define SUPPORTED_10baseT_Full      (1 << 1)
#define SUPPORTED_100baseT_Half     (1 << 2)
#define SUPPORTED_100baseT_Full     (1 << 3)
#define SUPPORTED_1000baseT_Half    (1 << 4)
#define SUPPORTED_1000baseT_Full    (1 << 5)
#define SUPPORTED_Autoneg       (1 << 6)
#define SUPPORTED_TP            (1 << 7)
#define SUPPORTED_AUI           (1 << 8) 
#define SUPPORTED_MII           (1 << 9)
#define SUPPORTED_FIBRE         (1 << 10)
#define SUPPORTED_BNC           (1 << 11)
#define SUPPORTED_10000baseT_Full   (1 << 12)
#define SUPPORTED_Pause         (1 << 13)
#define SUPPORTED_Asym_Pause        (1 << 14)

/* Indicates what features are advertised by the interface. */
#define ADVERTISED_10baseT_Half     (1 << 0)
#define ADVERTISED_10baseT_Full     (1 << 1)
#define ADVERTISED_100baseT_Half    (1 << 2)
#define ADVERTISED_100baseT_Full    (1 << 3)
#define ADVERTISED_1000baseT_Half   (1 << 4)
#define ADVERTISED_1000baseT_Full   (1 << 5)
#define ADVERTISED_Autoneg      (1 << 6)
#define ADVERTISED_TP           (1 << 7)
#define ADVERTISED_AUI          (1 << 8)
#define ADVERTISED_MII          (1 << 9) 
#define ADVERTISED_FIBRE        (1 << 10)
#define ADVERTISED_BNC          (1 << 11)
#define ADVERTISED_10000baseT_Full  (1 << 12)
#define ADVERTISED_Pause        (1 << 13)
#define ADVERTISED_Asym_Pause       (1 << 14)
/* Duplex, half or full. */
#define DUPLEX_HALF     0x00
#define DUPLEX_FULL     0x01

#define AUTONEG_DISABLE     0x00 
#define AUTONEG_ENABLE      0x01 

static void rtl8169_link_option(int idx, u8 *autoneg, u16 *speed, u8 *duplex)
{
      struct {
		u16 speed;
		u8 duplex;
		u8 autoneg;
		u8 media;
	} link_settings[] = {
		{ SPEED_10,	DUPLEX_HALF, AUTONEG_ENABLE,	_10_Half },
		{ SPEED_10,	DUPLEX_FULL, AUTONEG_ENABLE,	_10_Full },
		{ SPEED_100,	DUPLEX_HALF, AUTONEG_ENABLE,	_100_Half },
		{ SPEED_100,	DUPLEX_FULL, AUTONEG_ENABLE,	_100_Full },
		{ SPEED_1000,	DUPLEX_FULL, AUTONEG_ENABLE,	_1000_Full },
		// Make TBI happy 
		{ SPEED_1000,	DUPLEX_FULL, AUTONEG_ENABLE,	0xff }
	}, *p;

	unsigned char option;
	
	option = ((idx < MAX_UNITS) && (idx >= 0)) ? media[idx] : 0xff;

	if ((option != 0xff) && !idx)
		printf("media option is deprecated.\n");

	for (p = link_settings; p->media != 0xff; p++) {
		if (p->media == option)
			break;
	}
	*autoneg = p->autoneg;
	*speed = p->speed;
	*duplex = p->duplex;

}

static int rtl8169_set_speed_tbi(struct rtl8169_private *tp,
				 u8 autoneg, u16 speed, u8 duplex)
{
	int ret = 0;
	u32 reg;

    printf("tbi mode\n");

	reg = RTL_R32(tp, TBICSR);
	if ((autoneg == AUTONEG_DISABLE) && (speed == SPEED_1000) &&
	    (duplex == DUPLEX_FULL)) {
		RTL_W32(tp, TBICSR, reg & ~(TBINwEnable | TBINwRestart));
	} else if (autoneg == AUTONEG_ENABLE)
		RTL_W32(tp, TBICSR, reg | TBINwEnable | TBINwRestart);
	else {
		printf( "incorrect speed setting refused in TBI mode\n");
		ret = -EOPNOTSUPP;
	}

	return ret;
}

static int rtl8169_set_speed_xmii(struct rtl8169_private *tp,
				  u8 autoneg, u16 speed, u8 duplex)
{
	int auto_nego, giga_ctrl;

	auto_nego = mdio_read(tp, PHY_AUTO_NEGO_REG);
	auto_nego &= ~(PHY_Cap_10_Half | PHY_Cap_10_Full |
		       PHY_Cap_100_Half | PHY_Cap_100_Full);
	giga_ctrl = mdio_read(tp, PHY_1000_CTRL_REG);
	giga_ctrl &= ~(PHY_Cap_1000_Full | PHY_Cap_Null);

    printf("rtl8169_set_speed_xmii\n");
	if (autoneg == AUTONEG_ENABLE) {
		auto_nego |= (PHY_Cap_10_Half | PHY_Cap_10_Full |
			      PHY_Cap_100_Half | PHY_Cap_100_Full);
		giga_ctrl |= PHY_Cap_1000_Full;
	} else {
		if (speed == SPEED_10)
			auto_nego |= PHY_Cap_10_Half | PHY_Cap_10_Full;
		else if (speed == SPEED_100)
			auto_nego |= PHY_Cap_100_Half | PHY_Cap_100_Full;
		else if (speed == SPEED_1000)
			giga_ctrl |= PHY_Cap_1000_Full;

		if (duplex == DUPLEX_HALF)
			auto_nego &= ~(PHY_Cap_10_Full | PHY_Cap_100_Full);

		if (duplex == DUPLEX_FULL)
			auto_nego &= ~(PHY_Cap_10_Half | PHY_Cap_100_Half);
	}

	tp->phy_auto_nego_reg = auto_nego;
	tp->phy_1000_ctrl_reg = giga_ctrl;

    mdio_write(tp, PHY_AUTO_NEGO_REG, auto_nego);
	mdio_write(tp, PHY_1000_CTRL_REG, giga_ctrl);
	mdio_write(tp, PHY_CTRL_REG, PHY_Enable_Auto_Nego |
					 PHY_Restart_Auto_Nego);
    

	return 0;
}

static int rtl8169_set_speed(struct rtl8169_private *tp,
			     u8 autoneg, u16 speed, u8 duplex)
{
	int ret;

	ret = tp->set_speed(tp, autoneg, speed, duplex);
	return ret;
}

#ifdef CONFIG_R8169_VLAN

static inline u32 rtl8169_tx_vlan_tag(struct rtl8169_private *tp,
				      struct mbuf *m)
{
	return (tp->vlgrp && vlan_tx_tag_present(skb)) ?
		TxVlanTag | swab16(vlan_tx_tag_get(skb)) : 0x00;
}

static void rtl8169_vlan_rx_register(struct net_device *dev,
				     struct vlan_group *grp)
{
	struct rtl8169_private *tp = netdev_priv(dev);
	void *ioaddr = tp->mmio_addr;
	unsigned long flags;

	tp->vlgrp = grp;
	if (tp->vlgrp)
		tp->cp_cmd |= RxVlan;
	else
		tp->cp_cmd &= ~RxVlan;
	RTL_W16(tp, CPlusCmd, tp->cp_cmd);
	RTL_R16(tp, CPlusCmd);
}

static void rtl8169_vlan_rx_kill_vid(struct net_device *dev, unsigned short vid)
{
	struct rtl8169_private *tp = netdev_priv(dev);
	unsigned long flags;

	if (tp->vlgrp)
		tp->vlgrp->vlan_devices[vid] = NULL;
}


#else /* !CONFIG_R8169_VLAN */

static inline 
u32 rtl8169_tx_vlan_tag(struct rtl8169_private *tp,
				      struct mbuf *m)
{
	return 0;
}

#endif

static void rtl8169_gset_tbi(struct rtl8169_private *tp, struct ethtool_cmd *cmd)
{
	u32 status;

	cmd->supported =
		SUPPORTED_1000baseT_Full | SUPPORTED_Autoneg | SUPPORTED_FIBRE;
	cmd->port = PORT_FIBRE;
	cmd->transceiver = XCVR_INTERNAL;

	status = RTL_R32(tp, TBICSR);
	cmd->advertising = (status & TBINwEnable) ?  ADVERTISED_Autoneg : 0;
	cmd->autoneg = !!(status & TBINwEnable);

	cmd->speed = SPEED_1000;
	cmd->duplex = DUPLEX_FULL; /* Always set */
}

static void rtl8169_gset_xmii(struct rtl8169_private *tp, struct ethtool_cmd *cmd)
{
	u8 status;

	cmd->supported = SUPPORTED_10baseT_Half |
			 SUPPORTED_10baseT_Full |
			 SUPPORTED_100baseT_Half |
			 SUPPORTED_100baseT_Full |
			 SUPPORTED_1000baseT_Full |
			 SUPPORTED_Autoneg |
		         SUPPORTED_TP;

	cmd->autoneg = 1;
	cmd->advertising = ADVERTISED_TP | ADVERTISED_Autoneg;

	if (tp->phy_auto_nego_reg & PHY_Cap_10_Half)
		cmd->advertising |= ADVERTISED_10baseT_Half;
	if (tp->phy_auto_nego_reg & PHY_Cap_10_Full)
		cmd->advertising |= ADVERTISED_10baseT_Full;
	if (tp->phy_auto_nego_reg & PHY_Cap_100_Half)
		cmd->advertising |= ADVERTISED_100baseT_Half;
	if (tp->phy_auto_nego_reg & PHY_Cap_100_Full)
		cmd->advertising |= ADVERTISED_100baseT_Full;
	if (tp->phy_1000_ctrl_reg & PHY_Cap_1000_Full)
		cmd->advertising |= ADVERTISED_1000baseT_Full;

	status = RTL_R8(tp, PHYstatus);

	if (status & _1000bpsF)
		cmd->speed = SPEED_1000;
	else if (status & _100bps)
		cmd->speed = SPEED_100;
	else if (status & _10bps)
		cmd->speed = SPEED_10;

	cmd->duplex = ((status & _1000bpsF) || (status & FullDup)) ?
		      DUPLEX_FULL : DUPLEX_HALF;
}

#define ETH_GSTRING_LEN     32

static const char rtl8169_gstrings[][ETH_GSTRING_LEN] = {
	"tx_packets",
	"rx_packets",
	"tx_errors",
	"rx_errors",
	"rx_missed",
	"align_errors",
	"tx_single_collisions",
	"tx_multi_collisions",
	"unicast",
	"broadcast",
	"multicast",
	"tx_aborted",
	"tx_underrun",
};

struct rtl8169_counters {
	u64	tx_packets;
	u64	rx_packets;
	u64	tx_errors;
	u32	rx_errors;
	u16	rx_missed;
	u16	align_errors;
	u32	tx_one_collision;
	u32	tx_multi_collision;
	u64	rx_unicast;
	u64	rx_broadcast;
	u32	rx_multicast;
	u16	tx_aborted;
	u16	tx_underun;
};

static void rtl8169_write_gmii_reg_bit(struct rtl8169_private *tp, int reg, int bitnum,
				       int bitval)
{
	int val;

	val = mdio_read(tp, reg);
	val = (bitval == 1) ?
		val | (bitval << bitnum) :  val & ~(0x0001 << bitnum);
	mdio_write(tp, reg, val & 0xffff); 
}

static void rtl8169_get_mac_version(struct rtl8169_private *tp, void *ioaddr)
{
	const struct {
		u32 mask;
		int mac_version;
	} mac_info[] = {
		{ 0x1 << 28,	RTL_GIGA_MAC_VER_X },
		{ 0x1 << 26,	RTL_GIGA_MAC_VER_E },
		{ 0x1 << 23,	RTL_GIGA_MAC_VER_D }, 
		{ 0x00000000,	RTL_GIGA_MAC_VER_B } /* Catch-all */
	}, *p = mac_info;
	u32 reg;

	reg = RTL_R32(tp, TxConfig) & 0x7c800000;
	while ((reg & p->mask) != p->mask)
		p++;
	tp->mac_version = p->mac_version;
}

static void rtl8169_print_mac_version(struct rtl8169_private *tp)
{
	struct {
		int version;
		char *msg;
	} mac_print[] = {
		{ RTL_GIGA_MAC_VER_X, "RTL_GIGA_MAC_VER_X" },
		{ RTL_GIGA_MAC_VER_E, "RTL_GIGA_MAC_VER_E" },
		{ RTL_GIGA_MAC_VER_D, "RTL_GIGA_MAC_VER_D" },
		{ RTL_GIGA_MAC_VER_B, "RTL_GIGA_MAC_VER_B" },
		{ 0, NULL }
	}, *p;

	for (p = mac_print; p->msg; p++) {
		if (tp->mac_version == p->version) {
			dprintk("mac_version == %s (%04d)\n", p->msg,
				  p->version);
			return;
		}
	}
	dprintk("mac_version == Unknown\n");
}

static void rtl8169_get_phy_version(struct rtl8169_private *tp, void *ioaddr)
{
	const struct {
		u16 mask;
		u16 set;
		int phy_version;
	} phy_info[] = {
		{ 0x000f, 0x0002, RTL_GIGA_PHY_VER_G },
		{ 0x000f, 0x0001, RTL_GIGA_PHY_VER_F },
		{ 0x000f, 0x0000, RTL_GIGA_PHY_VER_E },
		{ 0x0000, 0x0000, RTL_GIGA_PHY_VER_D } /* Catch-all */
	}, *p = phy_info;
	u16 reg;

	reg = mdio_read(tp, 3) & 0xffff;
	while ((reg & p->mask) != p->set)
		p++;
	tp->phy_version = p->phy_version;
}

static void rtl8169_print_phy_version(struct rtl8169_private *tp)
{
	struct {
		int version;
		char *msg;
		u32 reg;
	} phy_print[] = {
		{ RTL_GIGA_PHY_VER_G, "RTL_GIGA_PHY_VER_G", 0x0002 },
		{ RTL_GIGA_PHY_VER_F, "RTL_GIGA_PHY_VER_F", 0x0001 },
		{ RTL_GIGA_PHY_VER_E, "RTL_GIGA_PHY_VER_E", 0x0000 },
		{ RTL_GIGA_PHY_VER_D, "RTL_GIGA_PHY_VER_D", 0x0000 },
		{ 0, NULL, 0x0000 }
	}, *p;

	for (p = phy_print; p->msg; p++) {
		if (tp->phy_version == p->version) {
			dprintk("phy_version == %s (%04x)\n", p->msg, p->reg);
			return;
		}
	}
	dprintk("phy_version == Unknown\n");
}

static void rtl8169_hw_phy_config(struct rtl8169_private *tp)
{
	struct {
		u16 regs[5]; /* Beware of bit-sign propagation */
	} phy_magic[5] = { {
		{ 0x0000,	//w 4 15 12 0
		  0x00a1,	//w 3 15 0 00a1
		  0x0008,	//w 2 15 0 0008
		  0x1020,	//w 1 15 0 1020
		  0x1000 } },{	//w 0 15 0 1000
		{ 0x7000,	//w 4 15 12 7
		  0xff41,	//w 3 15 0 ff41
		  0xde60,	//w 2 15 0 de60
		  0x0140,	//w 1 15 0 0140
		  0x0077 } },{	//w 0 15 0 0077
		{ 0xa000,	//w 4 15 12 a
		  0xdf01,	//w 3 15 0 df01
		  0xdf20,	//w 2 15 0 df20
		  0xff95,	//w 1 15 0 ff95
		  0xfa00 } },{	//w 0 15 0 fa00
		{ 0xb000,	//w 4 15 12 b
		  0xff41,	//w 3 15 0 ff41
		  0xde20,	//w 2 15 0 de20
		  0x0140,	//w 1 15 0 0140
		  0x00bb } },{	//w 0 15 0 00bb
		{ 0xf000,	//w 4 15 12 f
		  0xdf01,	//w 3 15 0 df01
		  0xdf20,	//w 2 15 0 df20
		  0xff95,	//w 1 15 0 ff95
		  0xbf00 }	//w 0 15 0 bf00
		}
	}, *p = phy_magic;
	int i;

	rtl8169_print_mac_version(tp);
	rtl8169_print_phy_version(tp);

	if (tp->mac_version <= RTL_GIGA_MAC_VER_B)
		return;
	if (tp->phy_version >= RTL_GIGA_PHY_VER_H)
		return;

	dprintk("MAC version != 0 && PHY version == 0 or 1\n");
	dprintk("Do final_reg2.cfg\n");

	/* Shazam ! */
#if 1
	if (tp->mac_version == RTL_GIGA_MAC_VER_X) {
		mdio_write(tp, 31, 0x0001);
		mdio_write(tp,  9, 0x273a);
		mdio_write(tp, 14, 0x7bfb);
		mdio_write(tp, 27, 0x841e);

		mdio_write(tp, 31, 0x0002);
		mdio_write(tp,  1, 0x90d0);
		mdio_write(tp, 31, 0x0000);
		return;
	}
#endif
	/* phy config for RTL8169s mac_version C chip */
	mdio_write(tp, 31, 0x0001);			//w 31 2 0 1
	mdio_write(tp, 21, 0x1000);			//w 21 15 0 1000
	mdio_write(tp, 24, 0x65c7);			//w 24 15 0 65c7
	rtl8169_write_gmii_reg_bit(tp, 4, 11, 0);	//w 4 11 11 0

	for (i = 0; i < sizeof(phy_magic)/sizeof(phy_magic[0]); i++, p++) {
		int val, pos = 4;

		val = (mdio_read(tp, pos) & 0x0fff) | (p->regs[0] & 0xffff);
		mdio_write(tp, pos, val);
		while (--pos >= 0)
			mdio_write(tp, pos, p->regs[4 - pos] & 0xffff);
		rtl8169_write_gmii_reg_bit(tp, 4, 11, 1); //w 4 11 11 1
		rtl8169_write_gmii_reg_bit(tp, 4, 11, 0); //w 4 11 11 0
	}
	mdio_write(tp, 31, 0x0000); //w 31 2 0 0
}



#define PCI_PM_CTRL 0x04
#define  PCI_PM_CTRL_STATE_MASK 0x0003  /* Current power state (D0 to D3) */
#define  PCI_PM_CTRL_NO_SOFT_RESET  0x0004  /* No reset for D3hot->D0 */
#define  PCI_PM_CTRL_PME_ENABLE 0x0100  /* PME pin enable */
#define  PCI_PM_CTRL_DATA_SEL_MASK  0x1e00  /* Data select (??) */
#define  PCI_PM_CTRL_DATA_SCALE_MASK    0x6000  /* Data scale (??) */
#define  PCI_PM_CTRL_PME_STATUS 0x8000  /* PME pin status */
#define PCI_PM_PPB_EXTENSIONS   6   /* PPB support extensions (??) */
#define  PCI_PM_PPB_B2_B3   0x40    /* Stop clock when in D3hot (??) */
#define  PCI_PM_BPCC_ENABLE 0x80    /* Bus power/clock control enable (??) */
#define PCI_PM_DATA_REGISTER    7   /* (??) */
#define PCI_PM_SIZEOF       8

static int 
rtl8169_init_board(struct rtl8169_private *tp, struct pci_attach_args *pa)
{
	void *ioaddr = NULL;
	int offset, value;
	int rc = 0, i, acpi_idle_state = 0, pm_cap;
	pci_chipset_tag_t pc = pa->pa_pc;
	int tmp;
       int iobase, iosize;
    u32 status;

	/* save power state before pci_enable_device overwrites it */
	pm_cap = pci_get_capability(pc, pa->pa_tag, PCI_CAP_PWRMGMT, &offset, &value);
	if (pm_cap) {
		u16 pwr_command;

		pwr_command = _pci_conf_read(pa->pa_tag, offset + PCI_PM_CTRL);
		acpi_idle_state = pwr_command & PCI_PM_CTRL_STATE_MASK;
	} else {
		printf("PowerManagement capability not found.\n");
	}

    tp->cp_cmd = PCIMulRW | RxChkSum;

	//pci_set_master(pdev);
	tmp = _pci_conf_read(pa->pa_tag, PCI_COMMAND_STATUS_REG);
	if(!(tmp & PCI_COMMAND_MASTER_ENABLE)) {
		tmp |= PCI_COMMAND_MASTER_ENABLE;
		_pci_conf_write(pa->pa_tag, PCI_COMMAND_STATUS_REG, tmp);
	}
	
    //disable SERREN
	tmp = _pci_conf_read(pa->pa_tag, PCI_COMMAND_STATUS_REG);
	tmp &= ~PCI_COMMAND_SERR_ENABLE;
	_pci_conf_write(pa->pa_tag, PCI_COMMAND_STATUS_REG, tmp);

    if (pci_io_find(pc, pa->pa_tag, 0x10, &iobase, &iosize)) {
		printf(": can't find i/o space\n");
		return -1;
	}

	if (bus_space_map(pa->pa_iot, iobase, iosize, 0, &tp->sc_sh)) {
		printf(": can't map i/o space\n");
		return -1;
	}
	printf("8169 iobase =%8x\n", tp->sc_sh);	
	tp->sc_st = pa->pa_iot;
	tp->sc_pc = pc;
	tp->cp_cmd = PCIMulRW | RxChkSum;
	
	tp->ioaddr = (unsigned long)tp->sc_sh;

    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("before softreset -> %x ", status);
	/* Soft reset the chip. */
	RTL_W8(tp, ChipCmd, CmdReset);

    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("After softreset -> %x ", status);

	/* Check that the chip has finished the reset. */
	for (i = 1000; i > 0; i--) {
		if ((RTL_R8(tp, ChipCmd) & CmdReset) == 0)
			break;
		delay(10);
	}

	printf("r8169: Reset device %s \n", i ? "success": "timeout");

	/* Identify chip attached to board */
	rtl8169_get_mac_version(tp, ioaddr);
	rtl8169_get_phy_version(tp, ioaddr);

	rtl8169_print_mac_version(tp);
	rtl8169_print_phy_version(tp);

	for (i = sizeof(rtl_chip_info)/sizeof(rtl_chip_info[0]) - 1; i >= 0; i--) {
		if (tp->mac_version == rtl_chip_info[i].mac_version)
			break;
	}
	if (i < 0) {
		/* Unknown chip: assume array element #0, original RTL-8169 */
		printf( "unknown chip version, assuming %s\n",
			       rtl_chip_info[0].name);
		i++;
	}
	tp->chipset = i;

	RTL_W8(tp, Cfg9346, Cfg9346_Unlock);
       RTL_W8(tp, Config0, RTL_R8(tp, Config0) | 0x15);
       RTL_W8(tp, Config1, RTL_R8(tp, Config1) | PMEnable | 0xCD);
       RTL_W8(tp, Config3, RTL_R8(tp, Config3) | 0xA1);
       RTL_W8(tp, Config4, RTL_R8(tp, Config4) | 0x80);
	RTL_W8(tp, Config5, RTL_R8(tp, Config5) & PMEStatus | 0x13);   //lihui.
	RTL_W8(tp, Cfg9346, Cfg9346_Lock);

    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("After set-> %x ", status);

	return rc;

}

static struct rtl8169_private* myRTL = NULL; 
static int rtl8169_open(struct rtl8169_private *tp);

static int
rtl8169_ether_ioctl(struct ifnet *ifp, unsigned long cmd, caddr_t data)
{
	struct ifaddr *ifa = (struct ifaddr *) data;
	struct rtl8169_private *sc = ifp->if_softc;
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
			error = rtl8169_open(sc);
			if(error == -1){
				return(error);
			}	
			ifp->if_flags |= IFF_UP;
			
#ifdef __OpenBSD__
			arp_ifinit(&sc->arpcom, ifa);
			printf("\n");
#else
			arp_ifinit(ifp, ifa);
#endif
			
			break;
#endif

		default:
		       rtl8169_open(sc);
		       RTLDBG;
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
		if(ifp->if_flags & IFF_UP){
			RTLDBG;
                    rtl8169_open(sc);
		}
		break;

        case SIOCETHTOOL:
        {
        long *p=data;
        myRTL = sc;
        cmd_setmac(p[0],p[1]);
        }
        break;
       case SIOCRDEEPROM:
                {
                long *p=data;
                myRTL = sc;
                cmd_reprom(p[0],p[1]);
                }
                break;
       case SIOCWREEPROM:
                {
                long *p=data;
                myRTL = sc;
                cmd_wrprom(p[0],p[1]);
                }
                break;
	default:
		RTLDBG; 
		error = EINVAL;
	}

	splx(s);

	return (error);
}

static int rtl8169_set_hard_speed(struct rtl8169_private *tp,
				  u8 autoneg, u16 speed, u8 duplex)
{
	int auto_nego, giga_ctrl;
    int old_value;
    int i;

	old_value = mdio_read(tp, PHY_CTRL_REG);
    printf("the old PHY_CTRL_REG value is %x\n", old_value);
    old_value &= ~0x1000;       //disable auto-negotiation
    old_value |= 0x0040;       //set to 1000M
    old_value &= ~0x2000;       
    mdio_write(tp, PHY_CTRL_REG, old_value);
   
	for (i = 200000000; i > 0; i--) {
		/* Check if the RTL8169 has completed */
		if (mdio_read(tp, PHY_CTRL_REG) & 0x8000 == 0) {
			break;
		}
		delay(2000);
	}
    printf("----------------> %d\n", i);
#if 0
	old_value = mdio_read(tp, PHY_CTRL_REG);
    old_value |= 0x0040;       //set to 1000M
    old_value &= ~0x2000;       //disable auto-negotiation
    mdio_write(tp, PHY_CTRL_REG, old_value);
   
	for (i = 2000; i > 0; i--) {
		/* Check if the RTL8169 has completed */
		if (mdio_read(tp, PHY_CTRL_REG) & 0x8000 == 0) {
			break;
		}
		delay(200);
	}
    printf("----------------> %d\n", i);
#endif
	return 0;
}



static void 
r8169_attach(struct device * parent, struct device * self, void *aux)
{
	struct rtl8169_private *tp = (struct rtl8169_private *)self;
	static int board_idx = -1;
	u8 autoneg, duplex;
	u16 speed;
	int i, rc;
	struct  ifnet *ifp;
	pci_intr_handle_t ih;
	const char *intrstr = NULL;
    u32 int_mask;
    u32 status;

       myRTL = tp;

	board_idx++;
    if_in_attach = 1;
    if_frequency = 0x13;
	rc = rtl8169_init_board(tp, aux);
	if (rc < 0)
		return;

	if (RTL_R8(tp, PHYstatus) & TBI_Enable) {
		tp->set_speed = rtl8169_set_speed_tbi;
		tp->get_settings = rtl8169_gset_tbi;
		tp->phy_reset_enable = rtl8169_tbi_reset_enable;
		tp->phy_reset_pending = rtl8169_tbi_reset_pending;
		tp->link_ok = rtl8169_tbi_link_ok;

		tp->phy_1000_ctrl_reg = PHY_Cap_1000_Full; /* Implied by TBI */
	} else {
		tp->set_speed = rtl8169_set_speed_xmii;
		tp->get_settings = rtl8169_gset_xmii;
		tp->phy_reset_enable = rtl8169_xmii_reset_enable;
		tp->phy_reset_pending = rtl8169_xmii_reset_pending;
		tp->link_ok = rtl8169_xmii_link_ok;
	}

    /*
    int_mask = RTL_R16(tp, IntrMask) & 0x7fff;
    RTL_W16(tp,IntrMask,int_mask);
	*/
    /* Get MAC address.  FIXME: read EEPROM */
#if 0
	RTL_W8(tp, Cfg9346, Cfg9346_Unlock);
	RTL_W32(tp, MAC0 + 4, 0x01000002);
	RTL_W32(tp, MAC0, 0x44444400);
	RTL_W8(tp, Cfg9346, Cfg9346_Lock);
#endif
//去掉接在93c46 sk脚上的上拉电阻,setmac后要关电

	printf("MAC addr is ");
	for (i = 0; i < MAC_ADDR_LEN; i++){
		
		tp->dev_addr[i] = RTL_R8(tp, MAC0 + i);
		
		printf("%02x", tp->dev_addr[i]);
        
		if ( i == 5 )
				printf("\n");
		else
				printf(":");
	}		

	for(i=0; i<32/2; i++){
		printf("%04x", read_eeprom(tp, i));
		if((i+1)%8 ==0)
			printf("\n");
		else
			printf(" ");
	}


#ifdef CONFIG_R8169_VLAN
	dev->features |= NETIF_F_HW_VLAN_TX | NETIF_F_HW_VLAN_RX;
	dev->vlan_rx_register = rtl8169_vlan_rx_register;
	dev->vlan_rx_kill_vid = rtl8169_vlan_rx_kill_vid;
#endif


	tp->intr_mask = 0xffff;
	tp->mmio_addr = (void *)tp->ioaddr;

    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("before identified-> %x\n ", status);
	
    printf("Identified chip type is '%s'.\n",
		       rtl_chip_info[tp->chipset].name);

	printf("r8110 at 0x%lx, "
			"%02x:%02x:%02x:%02x:%02x:%02x\n",
			tp->ioaddr,
			tp->dev_addr[0], tp->dev_addr[1],
			tp->dev_addr[2], tp->dev_addr[3],
			tp->dev_addr[4], tp->dev_addr[5]);

	rtl8169_hw_phy_config(tp);

	printf("Set MAC Reg C+CR Offset 0x82h = 0x01h\n");
	RTL_W8(tp, 0x82, 0x01);

	if (tp->mac_version < RTL_GIGA_MAC_VER_E) {
		printf("Set PCI Latency=0x40\n");
#define PCI_LATENCY_TIMER 0x0d
		_pci_conf_writen(tp->pa->pa_tag, PCI_LATENCY_TIMER, 0x40, 1);
	}

	if (tp->mac_version == RTL_GIGA_MAC_VER_D) {
		dprintk("Set MAC Reg C+CR Offset 0x82h = 0x01h\n");
		RTL_W8(tp, 0x82, 0x01);
		dprintk("Set PHY Reg 0x0bh = 0x00h\n");
		mdio_write(tp, 0x0b, 0x0000); //w 0x0b 15 0 0
	}

    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("before link option-> %x\n ", status);
	
    rtl8169_link_option(board_idx, &autoneg, &speed, &duplex);

    printf("by whd: autoneg = %x, speed = %x, duplex = %x\n");
    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("before set speed-> %x\n ", status);
	//rtl8169_set_hard_speed(tp, autoneg, speed, duplex);
	rtl8169_set_speed(tp, AUTONEG_ENABLE, SPEED_1000, DUPLEX_FULL);
	
	if ((RTL_R8(tp, PHYstatus) & TBI_Enable))
		printf("TBI auto-negotiating\n");


	ifp = &tp->arpcom.ac_if;
	bcopy(tp->dev_addr, tp->arpcom.ac_enaddr, sizeof(tp->arpcom.ac_enaddr));

	bcopy(tp->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
	
	ifp->if_softc = tp;
	ifp->if_ioctl = rtl8169_ether_ioctl;
	ifp->if_start = rtl8169_start_xmit;


	ifp->if_snd.ifq_maxlen = NUM_TX_DESC - 1;

	if_attach(ifp);
	ether_ifattach(ifp);
	
    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("before open-> %x\n ", status);
	
    rc = rtl8169_open(tp);
	if(rc) {
		printf("rtl8169_open: error code %d \n", rc);
		return;
	}

    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("before map interrupt-> %x\n ", status);
	if (pci_intr_map(pc, pa->pa_intrtag, pa->pa_intrpin,
	    pa->pa_intrline, &ih)) { //#define	pci_intr_map(a, b, c, d, e)		(*e = -1, 0)
		printf(": couldn't map interrupt\n");
		return;
	}
	
	intrstr = pci_intr_string(pc, ih);

    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("before establish interrupt-> %x\n ", status);
	
    tp->sc_ih = pci_intr_establish(pc, ih, IPL_NET, rtl8169_interrupt, tp, 
			self->dv_xname);
	if(tp->sc_ih == NULL){
             printf("error: could not establish interrupt !");
	      RTLDBG;
	      return;
	}else{
             printf("interrupt established!");
	}
    if_in_attach = 1;
	return ;
}


static void rtl8169_hw_reset(struct rtl8169_private *tp);
static int rtl8169_open_times = 0; 
static int rtl8169_open(struct rtl8169_private *tp)
{
	int retval = 0;
    u32 status;

	/*
	 * Rx and Tx desscriptors needs 256 bytes alignment.
	 * pci_alloc_consistent provides more.
	 */
    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("1 -> %x\n ", status);
	
    if(!tp->TxDescArray){
		tp->TxDescArray = (struct TxDesc*)malloc(R8169_TX_RING_BYTES + 255, M_DEVBUF, M_DONTWAIT);
		if (!tp->TxDescArray) {
	       	RTLDBG;
		       retval = -ENOMEM;
		       goto err_free_tx;
        	}
	       memset((caddr_t)tp->TxDescArray, 0, R8169_TX_RING_BYTES + 255);
       	pci_sync_cache(tp->sc_pc, (caddr_t)(tp->TxDescArray),  R8169_TX_RING_BYTES + 255, SYNC_W);
       	tp->TxDescArray = ((unsigned long)(tp->TxDescArray) + 255) & ~255;	
	}
	tp->TxDescArray = (struct TxDesc*)CACHED_TO_UNCACHED((unsigned long)(tp->TxDescArray));
	tp->TxPhyAddr = (unsigned long)vtophys((unsigned long)(tp->TxDescArray));

    
    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("2 -> %x\n ", status);
	


	if(!tp->RxDescArray){
		tp->RxDescArray = (struct RxDesc*)malloc(R8169_RX_RING_BYTES + 255, M_DEVBUF, M_DONTWAIT);
	       if (!tp->RxDescArray) {
		      RTLDBG;
		      retval = -ENOMEM;
		      goto err_free_rx;
       	}		
	       memset((caddr_t)(tp->RxDescArray), 0, R8169_RX_RING_BYTES + 255);
       	pci_sync_cache(tp->sc_pc, (caddr_t)(tp->RxDescArray),  R8169_RX_RING_BYTES + 255, SYNC_W);
       	tp->RxDescArray = ((unsigned long)(tp->RxDescArray) + 255) & ~255;		
	}	
	tp->RxDescArray = (struct RxDesc*)CACHED_TO_UNCACHED((unsigned long)(tp->RxDescArray));
	tp->RxPhyAddr = (unsigned long)vtophys((unsigned long)(tp->RxDescArray));

    
    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("3 -> %x\n ", status);
	

       tp->tx_buf_sz = TX_BUF_SIZE;
	tp->rx_buf_sz = RX_BUF_SIZE;
	retval = rtl8169_init_ring(tp);
    
    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("4 -> %x\n ", status);
	
	if (retval < 0){
		RTLDBG;
		goto err_free_rx;
	}
    printf("hw_reset_first\n");
    
    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("5 -> %x\n ", status);
	
	rtl8169_hw_reset(tp);//by AdonWang
    
    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("6 -> %x\n ", status);
	
	//rtl8169_hw_start(tp);
	//rtl8169_hw_start(tp);
	rtl8169_hw_start(tp);
    
    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("7 -> %x\n ", status);
	
	rtl8169_check_link_status(tp);
       tp->arpcom.ac_if.if_flags |=  IFF_RUNNING;

    
    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("8 -> %x\n ", status);
	CPU_FlushCache();//fix me
	

out:
	return retval;

err_free_rx:
	free(tp->RxDescArray, M_DEVBUF);
err_free_tx:
	free(tp->TxDescArray, M_DEVBUF);

	goto out;
}


static void rtl8169_hw_reset(struct rtl8169_private *tp)
{
	/* Disable interrupts */
    u32 i;
	rtl8169_irq_mask_and_ack(tp);

    delay(1000);

	/* Reset the chipset */
	RTL_W8(tp, ChipCmd, CmdReset);

	for (i = 1000; i > 0; i--) {
		if ((RTL_R8(tp, ChipCmd) & CmdReset) == 0)
        {
            printf("reset_done!!! i= %d\n",i);
			break;
        }
		delay(10);
	}
	/* PCI commit */
	RTL_R8(tp, ChipCmd);
}

static void
rtl8169_hw_start(struct rtl8169_private *tp)
{
	u32 i;
    u32 status;
    u32 RegAddr, value;
#if 0 
    RegAddr = 0x00;
    //value = 0x2100;         //disable auto-nogotaitation and set to 100M full-duplex
    //value = 0x0140;         //disable auto-nogotaitation and set to 1000M full-duplex
    value = 0x8200;         //restart auto-nogotaitaiton
	RTL_W32(tp, PHYAR, 0x80000000 | (RegAddr & 0xFF) << 16 | value);

	for (i = 20; i > 0; i--) {
		/* Check if the RTL8169 has completed writing to the specified MII register */
		if (!(RTL_R32(tp, PHYAR) & 0x80000000)) 
			break;
		delay(25);
	}
#endif
    
    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("11 -> %x ", status);
	/* Soft reset the chip. */
	RTL_W8(tp, ChipCmd, CmdReset);

    status = RTL_R16(tp, IntrStatus);
    printf("12 -> %x ", status);
	/* Check that the chip has finished the reset. */
	for (i = 1000; i > 0; i--) {
		if ((RTL_R8(tp, ChipCmd) & CmdReset) == 0)
			break;
		delay(10);
	}

    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("13 -> %x ", status);

	RTL_W8(tp, Cfg9346, Cfg9346_Unlock);
	//RTL_W8(tp, Cfg9346, Cfg9346_Autoload);
    //delay(10000);
#if 0
	RTL_W8(tp, ChipCmd, CmdReset);
	for (i = 1000; i > 0; i--) {
		if ((RTL_R8(tp, ChipCmd) & CmdReset) == 0)
			break;
		delay(10);
	}
#endif
//	RTL_W8(tp, ChipCmd, CmdTxEnb | CmdRxEnb);
    printf("mtps : %x \n",RTL_R16(tp,EarlyTxThres));
	//RTL_W8(tp, EarlyTxThres, EarlyTxThld);
	RTL_W8(tp, EarlyTxThres, 0x5ff);

    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("14 -> %x ", status);

	/* Low hurts. Let's disable the filtering. */
	RTL_W16(tp, RxMaxSize, 16383);

    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("15 -> %x \n", status);

    printf("rtl_chip_info: %s, %x\n",rtl_chip_info[tp->chipset].name,rtl_chip_info[tp->chipset].mac_version);
	/* Set Rx Config register */
	i = rtl8169_rx_config |
		(RTL_R32(tp, RxConfig) & rtl_chip_info[tp->chipset].RxConfigMask);
	RTL_W32(tp, RxConfig, i);
    printf("Rxconfig = %x",RTL_R32(tp,RxConfig));

    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("16 -> %x ", status);

	/* Set DMA burst size and Interframe Gap Time */
	RTL_W32(tp, TxConfig,
		(TX_DMA_BURST << TxDMAShift) | (InterFrameGap <<
						TxInterFrameGapShift));
	tp->cp_cmd |= RTL_R16(tp, CPlusCmd);
	RTL_W16(tp, CPlusCmd, tp->cp_cmd);

    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("17 -> %x ", status);

	if ((tp->mac_version == RTL_GIGA_MAC_VER_D) ||
	    (tp->mac_version == RTL_GIGA_MAC_VER_E)) 
    {
		printf("Set MAC Reg C+CR Offset 0xE0. "
			"Bit-3 and bit-14 MUST be 1\n");
		tp->cp_cmd |= (1 << 14) | PCIMulRW;
		RTL_W16(tp, CPlusCmd, tp->cp_cmd);
	}

    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("18 -> %x ", status);

	/*
	 * Undocumented corner. Supposedly:
	 * (TxTimer << 12) | (TxPackets << 8) | (RxTimer << 4) | RxPackets
	 */
	//RTL_W16(tp, IntrMitigate, 0x0000);

    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("19 -> %x ", status);

#define DMA_32BIT_MASK 0xffffffffUL


	RTL_W32(tp, TxDescStartAddrLow, ((u64) tp->TxPhyAddr & DMA_32BIT_MASK));
	RTL_W32(tp, TxDescStartAddrHigh, ((u64) tp->TxPhyAddr >> 32));
	RTL_W32(tp, RxDescAddrLow, ((u64) tp->RxPhyAddr & DMA_32BIT_MASK));
	RTL_W32(tp, RxDescAddrHigh, ((u64) tp->RxPhyAddr >> 32));
	RTL_W8(tp, Cfg9346, Cfg9346_Lock);

    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("20 -> %x ", status);

	delay(1000);

    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("21 -> %x ", status);

	RTL_W32(tp, RxMissed, 0);


	RTL_W8(tp, ChipCmd, CmdTxEnb | CmdRxEnb);
    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("22 -> %x ", status);

	tp->flags=IFF_PROMISC;
	rtl8169_set_rx_mode(tp);

    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("23 -> %x \n", status);

	/* no early-rx interrupts */
	RTL_W16(tp, MultiIntr, RTL_R16(tp, MultiIntr) & 0xF000);

    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("24 -> %x ", status);

    
	RTL_W16(tp, IntrMask, 0x0);
    RTL_W16(tp,IntrStatus,status);
    /*
	RTL_W8(tp, ChipCmd, CmdReset);
	for (i = 1000; i > 0; i--) {
		if ((RTL_R8(tp, ChipCmd) & CmdReset) == 0)
			break;
		delay(10);
	}
    delay(1000);
    RTL_W16(tp,IntrStatus,status);
    delay(1000);
	RTL_W8(tp, ChipCmd, CmdReset);
	for (i = 1000; i > 0; i--) {
		if ((RTL_R8(tp, ChipCmd) & CmdReset) == 0)
			break;
		delay(10);
	}
    delay(1000);
	RTL_W8(tp, ChipCmd, CmdTxEnb | CmdRxEnb);
    status = RTL_R16(tp, IntrStatus);
    printf("24 -> %x ", status);
	*/
    /* Enable all known interrupts by setting the interrupt mask. */
	RTL_W16(tp, IntrMask, rtl8169_intr_mask);

    //by liuqi
    status = RTL_R16(tp, IntrStatus);
    printf("24 -> %x ", status);

}

static inline void rtl8169_make_unusable_by_asic(struct RxDesc *desc)
{
        desc->addr = 0x0badbadbadbadbadull;;

	desc->opts1 &=  ~cpu_to_le32(DescOwn | RsvdMask);
} 

static void rtl8169_free_rx(struct rtl8169_private *tp,
				unsigned char *buf, struct RxDesc *desc)
{
	free(buf, M_DEVBUF);
	buf = NULL;
	rtl8169_make_unusable_by_asic(desc);
}

static inline void rtl8169_mark_to_asic(struct RxDesc *desc, u32 rx_buf_sz, int caller)
{
        unsigned int eor = 0;
	if(caller == 1)
            eor = le32_to_cpu(desc->opts1) & RingEnd;

      	desc->opts1 = cpu_to_le32(DescOwn | eor | (u16)rx_buf_sz);
}

static inline void rtl8169_map_to_asic(struct RxDesc *desc, unsigned long mapping,
				       u32 rx_buf_sz, int caller)
{  
       desc->addr = cpu_to_le64(mapping);

	rtl8169_mark_to_asic(desc, rx_buf_sz, caller);
}

static int rtl8169_alloc_rx(unsigned char **rx_buffer,
				struct RxDesc *desc, int rx_buf_sz, int caller)
{
	unsigned long mapping;
	int ret = 0;


	*rx_buffer = (char*)malloc(rx_buf_sz + 7, M_DEVBUF, M_DONTWAIT);
	if(!(*rx_buffer)){
              RTLDBG;
		goto err_out;
	}
	*rx_buffer = ((unsigned long)(*rx_buffer) + 7) & ~7;
	*rx_buffer = CACHED_TO_UNCACHED(*rx_buffer);	

	mapping = (unsigned long)vtophys(*rx_buffer);

	rtl8169_map_to_asic(desc, mapping, rx_buf_sz, caller);

out:
	return ret;

err_out:
	ret = -ENOMEM;
	rtl8169_make_unusable_by_asic(desc);
	goto out;
}

static int rtl8169_alloc_tx(unsigned char **tx_buffer,
				struct TxDesc *desc, int tx_buf_sz)
{
	unsigned long mapping;
	int ret = 0;
	unsigned long tmp;


  	tmp = (unsigned long)malloc(tx_buf_sz, M_DEVBUF, M_DONTWAIT);
	if(!tmp){
              RTLDBG;
		goto err_out;
	}

	*tx_buffer = CACHED_TO_UNCACHED(tmp);

	mapping = (unsigned long)vtophys(*tx_buffer);

       desc->addr = cpu_to_le64(mapping);

	return ret;
err_out:
	ret = -ENOMEM;
	return ret;
}

static void rtl8169_rx_clear(struct rtl8169_private *tp)
{
	int i;

	for (i = 0; i < NUM_RX_DESC; i++) {
		if (tp->rx_buffer[i]) {
			rtl8169_free_rx(tp, tp->rx_buffer[i],
					    tp->RxDescArray + i);
		}
	}
}

static u32 rtl8169_rx_fill(struct rtl8169_private *tp,
			   u32 start, u32 end, int caller)
{
	u32 cur = 0, tmprecord = 0;
       int ret;
 
       if(caller == 0){
             for(cur = 0; cur < NUM_RX_DESC; cur++){
                    	if (tp->rx_buffer[cur])
				continue;
			

       		ret = rtl8169_alloc_rx(&(tp->rx_buffer[cur]), tp->RxDescArray + cur, tp->rx_buf_sz, caller);
			if(ret < 0){
                            printf("ERROR: %s, %d\n", __FUNCTION__, __LINE__);
				break;
			}
             }
	      tp->RxDescArray[cur - 1].opts1 |= RingEnd;
	      return cur;
	}

	//caller == 1
	for (cur = start; cur < end; cur++) {
		int  entry = cur % NUM_RX_DESC;

              if(tp->RxDescArray[entry].opts1& DescOwn)
		       break;

              tmprecord = tp->RxDescArray[entry].opts1 & RingEnd;
		tp->RxDescArray[entry].opts1 = 0;
		tp->RxDescArray[entry].opts1 = tmprecord | DescOwn | tp->rx_buf_sz;
	}
	
	return cur - start;
}

static u32 rtl8169_tx_fill(struct rtl8169_private *tp,
			   u32 start, u32 end, u32 caller)
{
	u32 cur, ret, entry;
	int counter = 0;


     	
       for (cur = start;  cur < end; cur++) {
                 entry = cur % NUM_TX_DESC;
                 if(caller == 0){
			   if(tp->tx_buffer[entry])
			   	   continue;
                        tp->TxDescArray[entry].opts1 = 0;
                        tp->TxDescArray[entry].opts2 = 0;
		          tp->TxDescArray[entry].addr = 0;
		          tp->tx_buffer[entry] = NULL;

                        ret = rtl8169_alloc_tx(&(tp->tx_buffer[entry]),
				     tp->TxDescArray + entry, tp->tx_buf_sz);
	                 if (ret < 0){
                                 printf("ret = %d. %s, %d\n", ret, __FILE__, __LINE__);
                                 break;
	                 }
       	   }			  
	          counter ++;
		   if(caller == 0)
	                 tp->TxDescArray[cur - 1].opts1 |= RingEnd;
	          if(caller == 1){
		            if(tp->TxDescArray[entry].opts1 & DescOwn)
		                  break; 
                          tp->TxDescArray[entry].opts1 &= RingEnd;
                 }		   
       }

	return cur - start;
}


static inline void rtl8169_mark_as_last_descriptor(struct RxDesc *desc)
{
	desc->opts1 |= cpu_to_le32(RingEnd);
}

static void rtl8169_init_ring_indexes(struct rtl8169_private *tp)
{
	tp->dirty_tx = tp->dirty_rx = tp->cur_tx = tp->cur_rx = 0;
}

static void rtl8169_tx_clear(struct rtl8169_private *tp);
static int rtl8169_init_ring(struct rtl8169_private *tp)
{
       int tmpres;

	rtl8169_init_ring_indexes(tp);

	/*add two lines*/		
//		memset(tp->tx_buffer, 0x0, sizeof(char*) * NUM_TX_DESC);
//		memset(tp->rx_buffer, 0x0, sizeof(char*) * NUM_RX_DESC);

	if(rtl8169_open_times== 1){
	      memset(tp->tx_buffer, 0x0, sizeof(char*) * NUM_TX_DESC);
	      memset(tp->rx_buffer, 0x0, sizeof(char*) * NUM_RX_DESC);
	}

	if ((tmpres = rtl8169_rx_fill(tp, 0, NUM_RX_DESC, 0)) != NUM_RX_DESC){
              RTLDBG;
		goto err_out1;
	}		
	
	
	if (rtl8169_tx_fill(tp, 0, NUM_TX_DESC, 0) != NUM_TX_DESC){
		RTLDBG;
		goto err_out2;
	}
	rtl8169_mark_as_last_descriptor(tp->RxDescArray + NUM_RX_DESC - 1);
	return 0;

err_out2:
	rtl8169_rx_clear(tp);
err_out1:
	rtl8169_tx_clear(tp);
	return -ENOMEM;
}


static void rtl8169_free_tx(struct rtl8169_private * tp, unsigned char * buf, struct TxDesc * desc){
       free(buf, M_DEVBUF);
       buf = NULL;
	rtl8169_make_unusable_by_asic(desc);
       return;
}
static void rtl8169_tx_clear(struct rtl8169_private *tp)
{
	unsigned int i;

	for (i = tp->dirty_tx; i < tp->dirty_tx + NUM_TX_DESC; i++) {
		unsigned int entry = i % NUM_TX_DESC;
              if(tp->tx_buffer[entry]){
                     rtl8169_free_tx( tp, tp->tx_buffer[entry], tp->TxDescArray+entry);
		}
		tp->stats.tx_dropped++;
	}
	tp->cur_tx = tp->dirty_tx = 0;
}

static void rtl8169_wait_for_quiescence(struct rtl8169_private *tp)
{

	/* Wait for any pending NAPI task to complete */
	rtl8169_irq_mask_and_ack(tp);
}

static void rtl8169_reinit_task(void *_data)
{
	struct rtl8169_private *tp= _data;
	struct ifnet * ifp = &tp->arpcom.ac_if;
	int ret;

	if (ifp->if_flags & IFF_UP) {
		rtl8169_wait_for_quiescence(tp);
		rtl8169_close(tp);
	}

	ret = rtl8169_open(tp);
	if ((ret < 0)) 
		printf( "reinit failure (status = %d)." " Rescheduling.\n", ret);
}


static int rtl8169_xmit_frags(struct rtl8169_private *tp,  struct ifnet *ifp)
{
       struct TxDesc* td;
	struct mbuf *mb_head;
	unsigned int entry = 0;

	while(ifp->if_snd.ifq_head != NULL){
		u32 status, len;
              u32 IfRingEnd = 0;
		  
              entry = tp->cur_tx % NUM_TX_DESC;
              if(tp->TxDescArray[entry].opts1 & DescOwn){
                    printf("NO available TX buffer ! %d\n", __LINE__);
                    break;
	       }
		
		td = tp->TxDescArray + entry;


		IF_DEQUEUE(&ifp->if_snd, mb_head);
		m_copydata(mb_head, 0, mb_head->m_pkthdr.len, tp->tx_buffer[entry]);
	             
          	
/*** 
*** rtl8169 will do padding work automatically. lihui.		
***/
              len = mb_head->m_pkthdr.len;
		IfRingEnd = ((entry + 1) % NUM_TX_DESC) ? 0 : RingEnd;
	       status = DescOwn |FirstFrag | LastFrag |len | IfRingEnd;//no fragement.
  	      	td->opts1 = cpu_to_le32(status);

#ifdef __mips__
		pci_sync_cache(tp->sc_pc, (vm_offset_t)tp->tx_buffer[entry], 
				len, SYNC_W);
#endif
		m_freem(mb_head);
		wbflush();  
		tp->cur_tx ++;
		
	}

	return 0;
}


static void rtl8169_TxDesc_Print(struct rtl8169_private *tp){
        u32 entry;

	 for(entry= 0; entry < NUM_TX_DESC; entry ++){
	 	printf("Cur Desc is: %d.    The add of cur Desc is: %x\n", entry, &(tp->TxDescArray[entry]));
	 	printf("The content of cur Desc is:\n");
	       printf("opts1: %x   ---   opts2: %x  ---  addr: %x\n\n\n", 
	 	         tp->TxDescArray[entry].opts1, tp->TxDescArray[entry].opts2, (tp->TxDescArray[entry].addr));
	 }

        return;
}

static void rtl8169_start_xmit(struct ifnet *ifp)
{
	struct rtl8169_private *tp = ifp->if_softc;
	unsigned int entry = tp->cur_tx % NUM_TX_DESC;
	struct TxDesc *txd = tp->TxDescArray + entry;


       if(tp->cur_tx - tp->dirty_tx == NUM_TX_DESC){
	      RTLDBG;
             return;
	}
	
	if (le32_to_cpu(txd->opts1) & DescOwn){
              printf("DescOwn Error!Desc NUM = %d", entry); RTLDBG;
	       tp->stats.tx_dropped++;
		return;
	}

       if(rtl8169_xmit_frags(tp, ifp) < 0){
              printf("TX Error!  %s, %d\n", __FUNCTION__, __LINE__);
	       return;
	}//transmit all

	RTL_W8(tp, TxPoll, 0x40);	// set polling bit 


	if (TX_BUFFS_AVAIL(tp) < MAX_FRAGS) { //FIXME
		if (TX_BUFFS_AVAIL(tp) >= MAX_FRAGS){
		    ;//FIXME netif_wake_queue(dev);
		}
	
	}

	return ;
}

static void rtl8169_pcierr_interrupt(struct rtl8169_private *tp)
{
	u16 pci_status, pci_cmd;
	struct pci_attach_args *pa = tp->pa;
	u32 tmp;

#if	1
	printf("jlliu : tp %p, pa %p\n", tp, pa);
#endif


    printf("8169_interrupt_0,mask = %x\n", RTL_R16(tp,IntrMask));
	tmp = _pci_conf_read(pa->pa_tag, PCI_COMMAND_STATUS_REG);
    printf("8169_interrupt_1\n");

	pci_status = tmp & 0xffff0000;
	pci_cmd = tmp & 0xffff;


	/*
	 * The recovery sequence below admits a very elaborated explanation:
	 * - it seems to work;
	 * - I did not see what else could be done.
	 *
	 * Feel free to adjust to your needs.
	 */
	_pci_conf_writen(pa->pa_tag, PCI_COMMAND_STATUS_REG,
			      pci_cmd | PCI_COMMAND_SERR_ENABLE | PCI_COMMAND_PARITY_ENABLE, 2);

    printf("8169_interrupt_2\n");
	pci_status &= (PCI_STATUS_PARITY_ERROR |
		PCI_STATUS_SPECIAL_ERROR | PCI_STATUS_MASTER_ABORT |
		PCI_STATUS_TARGET_TARGET_ABORT| PCI_STATUS_MASTER_TARGET_ABORT);
	pci_status >>= 16;

	_pci_conf_writen(pa->pa_tag, PCI_COMMAND_STATUS_REG + 2, pci_status, 2);
    printf("8169_interrupt_3\n");

	/* The infamous DAC f*ckup only happens at boot time */
	if ((tp->cp_cmd & PCIDAC) && !tp->dirty_rx && !tp->cur_rx) {
               RTLDBG;
			printf("disabling PCI DAC.\n");
		tp->cp_cmd &= ~PCIDAC;
		RTL_W16(tp, CPlusCmd, tp->cp_cmd);
		rtl8169_reinit_task(tp);
	}
    printf("8169_interrupt_4\n");

	rtl8169_hw_reset(tp);
    printf("8169_interrupt_5\n");
    rtl8169_hw_start(tp);
    printf("8169_interrupt_6\n");
	
}

static void
rtl8169_tx_interrupt(struct rtl8169_private *tp)
{
	unsigned int dirty_tx, tx_left;

	assert(tp != NULL);
       
	dirty_tx = tp->dirty_tx;
	tx_left = tp->cur_tx - dirty_tx;
 
	while (tx_left > 0) {
		unsigned int entry = dirty_tx % NUM_TX_DESC;
		u32 status = 0;

		status = le32_to_cpu(tp->TxDescArray[entry].opts1);
		if (status & DescOwn){
            printf("TX Interrupt error!  %s, %d\n", __FUNCTION__, __LINE__);
			break;
		}

		tp->stats.tx_packets++;

              if(rtl8169_tx_fill(tp, entry, entry + 1, 1) != 1){//should I do it here ? lihui
                     printf("TX fill eror!  %s, %d\n", __FUNCTION__, __LINE__);
		       return;
	       }           
		dirty_tx++;
		tx_left--;
	}  
	tp->dirty_tx = dirty_tx;

	return;
}



#define CHECKSUM_UNNECESSARY 0x01
#define CHECKSUM_NONE        0x02

static inline int rtl8169_rx_csum(struct RxDesc *desc)
{
	u32 opts1 = le32_to_cpu(desc->opts1);
	u32 status = opts1 & RxProtoMask;

	if (((status == RxProtoTCP) && !(opts1 & TCPFail)) ||
	    ((status == RxProtoUDP) && !(opts1 & UDPFail)) ||
	    ((status == RxProtoIP) && !(opts1 & IPFail)))
		return CHECKSUM_UNNECESSARY;
	else
		return CHECKSUM_NONE;
}

static int
rtl8169_rx_interrupt(struct rtl8169_private *tp)
{
	unsigned int cur_rx, rx_left;
	unsigned int delta, count;
	struct ether_header *eh;
	struct  ifnet *ifp = &tp->arpcom.ac_if;
	u32 status = 0;
	struct RxDesc *desc;

	assert(tp != NULL);

	cur_rx = tp->cur_rx;
	rx_left = NUM_RX_DESC + tp->dirty_rx - cur_rx;     

	for (; rx_left > 0; rx_left--, cur_rx++) {
		unsigned int entry = cur_rx % NUM_RX_DESC;
		desc = &(tp->RxDescArray[entry]);
		status = 0;

		status = le32_to_cpu(desc->opts1);
	  
		if (status & DescOwn)
			break;
		
  		if ((status & RxRES)) {
			printf( "Rx ERROR. status = %08x\n", status);
			tp->stats.rx_errors++;
			if (status & (RxRWT | RxRUNT))
				tp->stats.rx_length_errors++;
			if (status & RxCRC)
				tp->stats.rx_crc_errors++;
			rtl8169_mark_to_asic(desc, tp->rx_buf_sz, 1);
		} else {
			struct mbuf *m;
			int pkt_size = (status & 0x00001FFF) - 4;
			
			/*
			 * The driver does not support incoming fragmented
			 * frames. They are seen as a symptom of over-mtu
			 * sized frames.
			 */
			if ((status & (FirstFrag|LastFrag)) != (FirstFrag|LastFrag)) {
    				tp->stats.rx_dropped++;			
				tp->stats.rx_length_errors++;
				rtl8169_mark_to_asic(desc, tp->rx_buf_sz, 1);
				RTLDBG;
				continue;
			}

			rtl8169_rx_csum(desc); //FIXME
			
			m = getmbuf(tp);
			if(m == NULL) {
				RTLDBG;//lihui.
				printf("rx_interrupt getmbuf failed\n");
				return -1;
			}

			bcopy(tp->rx_buffer[entry], mtod(m, caddr_t), pkt_size); 

				
			m->m_pkthdr.rcvif = ifp;
			m->m_pkthdr.len = m->m_len = pkt_size - sizeof(struct ether_header);

			eh = mtod(m, struct ether_header *);
	
			m->m_data += sizeof(struct ether_header);

			ether_input(ifp, eh, m);
				 
			tp->stats.rx_bytes += pkt_size;
			tp->stats.rx_packets++;

                     delta = rtl8169_rx_fill(tp,  entry, entry + 1, 1);//should this work be done here ? lihui.
                     tp->dirty_rx += delta; //FIXME			
		}
	}

	count = cur_rx - tp->cur_rx;
	tp->cur_rx = cur_rx;

	/*
	 * FIXME: until there is periodic timer to try and refill the ring,
	 * a temporary shortage may definitely kill the Rx process.
	 * - disable the asic to try and avoid an overflow and kick it again
	 *   after refill ?
	 * - how do others driver handle this condition (Uh oh...).
	 */
	if ((tp->dirty_rx + NUM_RX_DESC) == tp->cur_rx)
		printf("Rx buffers exhausted\n");

	return count;
}

/* The interrupt handler does all of the Rx thread work and cleans up after the Tx thread. */
static 
int rtl8169_interrupt(void *arg)
{
	struct rtl8169_private *tp = (struct rtl8169_private *)arg; 
	int boguscnt = max_interrupt_work;
	u32 status = 0;
	int handled = 0;

	struct ifnet * ifp = &tp->arpcom.ac_if;
	

	do {
		status = RTL_R16(tp, IntrStatus);



		/* hotplug/major error/no more work/shared irq */
		if ((status == 0xFFFF) || !status){
                     break;
		}	

		handled = 1;

		status &= tp->intr_mask;
		RTL_W16(tp, IntrStatus,
			(status & RxFIFOOver) ? (status | RxOverflow) : status);

		if (!(status & rtl8169_intr_mask)){
			break;
		}

		if ((status & SYSErr)) { 
			RTLDBG;
			rtl8169_pcierr_interrupt(tp);
			break;
		}

		/* Rx interrupt */
		if (status & (RxOK | RxOverflow | RxFIFOOver) && (ifp->if_flags  & IFF_RUNNING)) {
			rtl8169_rx_interrupt(tp);
		}

		/* Tx interrupt */		
		if (status & (TxOK | TxErr) && (ifp->if_flags  & IFF_RUNNING)){
			rtl8169_tx_interrupt(tp);
		}

		boguscnt--;
	} while (boguscnt > 0);

	if (boguscnt <= 0) {
		/* Clear all interrupt sources. */
		RTL_W16(tp, IntrStatus, 0xffff);
	}

	return (handled);
}

#ifdef CONFIG_R8169_NAPI
static int rtl8169_poll(struct rtl8169_private *tp, int *budget)
{
	unsigned int work_done, work_to_do = min(*budget, dev->quota);
	void *ioaddr = tp->mmio_addr;

       printf(".......... IN RTL8169POLL ..........");

	work_done = rtl8169_rx_interrupt(tp);
	rtl8169_tx_interrupt(tp);

	*budget -= work_done;

	if (work_done < work_to_do) {
		netif_rx_complete(dev);
		tp->intr_mask = 0xffff;
		/*
		 * 20040426: the barrier is not strictly required but the
		 * behavior of the irq handler could be less predictable
		 * without it. Btw, the lack of flush for the posted pci
		 * write is safe - FR
		 */
		RTL_W16(tp, IntrMask, rtl8169_intr_mask);
	}

	return (work_done >= work_to_do);
}
#endif

static void rtl8169_down(struct rtl8169_private *tp)
{
	unsigned int poll_locked = 0;


core_down:

	rtl8169_asic_down(tp);

	/* Update the error counts. */
	tp->stats.rx_missed_errors += RTL_R32(tp, RxMissed);	RTL_W32(tp, RxMissed, 0);

	if (!poll_locked) {
		poll_locked++;
	}

	/*
	 * And now for the 50k$ question: are IRQ disabled or not ?
	 *
	 * Two paths lead here:
	 * 1) dev->close
	 *    -> netif_running() is available to sync the current code and the
	 *       IRQ handler. See rtl8169_interrupt for details.
	 * 2) dev->change_mtu
	 *    -> rtl8169_poll can not be issued again and re-enable the
	 *       interruptions. Let's simply issue the IRQ down sequence again.
	 */
	if (RTL_R16(tp, IntrMask))
		goto core_down;

	rtl8169_tx_clear(tp);
	rtl8169_rx_clear(tp);

}
static int rtl8169_close_times = 0;
static int rtl8169_close(struct rtl8169_private *tp)
{

	rtl8169_down(tp);

	free(tp->RxDescArray, M_DEVBUF);
	free(tp->TxDescArray, M_DEVBUF);

	tp->TxDescArray = NULL;
	tp->RxDescArray = NULL;

       printf("!!!!!!rtl8169_close_times: %d, %s, %d", ++rtl8169_close_times, __FUNCTION__, __LINE__);
	   
	return 0;
}

static void
rtl8169_set_rx_mode(struct rtl8169_private *tp)
{
	u32 mc_filter[2];	/* Multicast hash filter */
	int rx_mode;
	u32 tmp = 0;

	if (tp->flags & IFF_PROMISC) {
		/* Unconditionally log net taps. */
		printf("Promiscuous mode enabled.\n");
		rx_mode =
		    AcceptBroadcast | AcceptMulticast | AcceptMyPhys |
		    AcceptAllPhys;
		mc_filter[1] = mc_filter[0] = 0xffffffff;
	} else if ((tp->mc_count > multicast_filter_limit)
		   || (tp->flags & IFF_ALLMULTI)) {
		/* Too many to filter perfectly -- accept all multicasts. */
		rx_mode = AcceptBroadcast | AcceptMulticast | AcceptMyPhys;
		mc_filter[1] = mc_filter[0] = 0xffffffff;
	} else {
		//struct dev_mc_list *mclist;
		rx_mode = AcceptBroadcast | AcceptMyPhys;
		mc_filter[1] = mc_filter[0] = 0;
	}

	tmp = rtl8169_rx_config | rx_mode |
	      (RTL_R32(tp, RxConfig) & rtl_chip_info[tp->chipset].RxConfigMask);

	RTL_W32(tp, RxConfig, tmp);
	RTL_W32(tp, MAR0 + 0, mc_filter[0]);
	RTL_W32(tp, MAR0 + 4, mc_filter[1]);

}


#if	1 
#include <pmon.h>
static int cmd_ifm(int ac, char *av[])
{
	unsigned short fullduplex = 0;
    unsigned short speed1000 = 0;
    unsigned short speed100 = 0;
	int i;
    char* par[] = {"auto", "1000", "100", "10", "FULL-DUPLEX", "HALF-DUPLEX",};
       
	if(!myRTL){
		printf("8169 interface not initialized\n");
		return 0;
	}

	if(ac !=1 && ac!=2){
REMIND:
	       printf("usage: ifm [1000|100|10|auto|FULL-DUPLEX|HALF-DUPLEX]\n");		
		return 0;
	}

	speed1000 |= (1 << 6 | 1 << 12); //0x1040
	speed100 |= (1 << 13 | 1 << 12); //0x3000
	fullduplex |= 1 << 8; //0x100
	if(ac == 1){
        speed1000 &= mdio_read(myRTL, PHY_CTRL_REG);
	    speed100 &= mdio_read(myRTL, PHY_CTRL_REG);
		fullduplex &= mdio_read(myRTL, PHY_CTRL_REG);
		if(speed1000 == 1 << 6){
		       printf("1000Mbps %s-DUPLEX.\n", fullduplex? "FULL":"HALF");
		}else{
               printf("%sMbps %s-DUPLEX.\n", (speed100 == 1<<13)? "100":"10", fullduplex? "FULL":"HALF");
		         }
		return 0;
	}
  
       for(i = 0; i < 6; i ++)
              if(!strcmp(par[i], av[1]))
                    break;

		   
       switch(i){
              case 0:  
			  	myRTL->set_speed(myRTL, AUTONEG_ENABLE, 0, 0);
				break;
		case 1:
			       myRTL->set_speed(myRTL, 0, SPEED_1000, 0);
				break;
	       case 2:
		   	       myRTL->set_speed(myRTL, 0, SPEED_100, 0);
				break;
	       case 3:
		   	       myRTL->set_speed(myRTL, 0, SPEED_10, 0);
				break;
	       case 4:
		   	       myRTL->set_speed(myRTL, 0, 0, DUPLEX_FULL);
				break;
	       case 5:
		   	       myRTL->set_speed(myRTL, 0, 0, DUPLEX_HALF);
				break;
		default: 
			       goto REMIND;				 
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

static int cmd_wrprom(int ac, char *av[])
{
        int i;
        unsigned long clocks_num=0;
unsigned short data;
static unsigned char rom[]={
/*00000000:*/0x29,0x81,0xec,0x10,0x69,0x81,0xec,0x10,0x69,0x81,0x20,0x40,0x01,0xa1,0x00,0xe0,
/*00000010:*/0x4c,0x67,0x10,0x51,0x15,0xcd,0xc2,0xf7,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x13,
/*00000020:*/0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*00000030:*/0x00,0x00,0xd7,0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,
/*00000040:*/0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*00000050:*/0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*00000060:*/0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*00000070:*/0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	
        if(!myRTL){
                printf("8169 interface not initialized\n");
                return 0;
        }

#if 1
                clocks_num =CPU_GetCOUNT();
                mysrand(clocks_num);
                for( i = 0; i < 4;i++ )
                {
                        rom[0x8*2 + i]=myrand()%256;
                        printf( " rom[%d]=02x%x\n", (0x8*2+i),rom[0x8*2+i]);
                }
#endif

	for(i=0;i<0x40;i++)
	{
	while(1){
#ifndef EPLC46
		data = read_eeprom(myRTL,  i);
#else
		data = read_eeprom(myRTL, 2*i);
		data = data | (read_eeprom(myRTL,2*i+1)) << 8;
#endif
if(data==((rom[2*i+1]<<8)|rom[2*i]))break;
printf("program %02x:%04x\n",2*i,(rom[2*i+1]<<8)|rom[2*i]);
#ifndef EPLC46
		rtl8169_write_eeprom(myRTL->ioaddr, i, (rom[2*i+1]<<8)|rom[2*i]);
#else 
		rtl8169_write_eeprom8(myRTL->ioaddr, i*2, rom[2*i] );
		rtl8169_write_eeprom8(myRTL->ioaddr, i*2+1, rom[2*i+1]);
#endif
};
	}
	printf("The whole eeprom have been programmed!\n");
}

static int cmd_setmac(int ac, char *av[])
{
	int i;
	unsigned short val = 0, v;
	
	if(!myRTL){
		printf("8169 interface not initialized\n");
		return 0;
	}

	if(ac != 2){
		printf("MAC ADDRESS ");
		for(i=0; i<6; i++){
			printf("%02x", myRTL->arpcom.ac_enaddr[i]);
			if(i==5)
				printf("\n");
			else
				printf(":");
		}
		printf("Use \"setmac <mac> \" to set mac address\n");
		return 0;
	}

{
unsigned short data;
static unsigned char rom[]={
/*00000000:*/0x29,0x81,0xec,0x10,0x69,0x81,0xec,0x10,0x69,0x81,0x20,0x40,0x01,0xa1,0x00,0xe0,
/*00000010:*/0x4c,0x67,0x10,0x51,0x15,0xcd,0xc2,0xf7,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x13,
/*00000020:*/0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*00000030:*/0x00,0x00,0xd7,0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,
/*00000040:*/0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*00000050:*/0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*00000060:*/0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*00000070:*/0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	for (i = 0; i < 6; i++) {
		val = 0;
		gethex(&v, av[1], 2);
		val = v ;
		rom[0x7*2+i]=val;
		av[1]+=3;
	}

	for (i = 0; i < 0x40; i=i++) {
	while(1){
#ifndef EPLC46
		data = read_eeprom(myRTL,  i);
#else
		data = read_eeprom(myRTL, 2*i);
		data = data | (read_eeprom(myRTL,2*i+1)) << 8;
#endif
if(data==((rom[2*i+1]<<8)|rom[2*i]))break;
printf("program %02x:%04x\n",2*i,(rom[2*i+1]<<8)|rom[2*i]);
#ifndef EPLC46
		rtl8169_write_eeprom(myRTL->ioaddr, i, (rom[2*i+1]<<8)|rom[2*i]);
#else 
		rtl8169_write_eeprom8(myRTL->ioaddr, i*2, rom[2*i] );
		rtl8169_write_eeprom8(myRTL->ioaddr, i*2+1, rom[2*i+1]);
#endif
};
	}
}

	printf("The machine should be restarted to make the mac change to take effect!!\n");

	return 0;
}

static int cmd_reprom(int ac, char *av)
{
	int i;
	unsigned short data;	

	printf("dump eprom:\n");

	for(i=0; i< 64; i++){
#ifndef EPLC46
		data = read_eeprom(myRTL,  i);
#else
		data = read_eeprom(myRTL, 2*i);
		data = data | (read_eeprom(myRTL,2*i+1)) << 8;
#endif
		printf("%04x ", data);
		if((i+1)%8 == 0)
			printf("\n");
	}
	return 0;
}
static void cmd_set_frequency(int ac, char *av[])
{
    char* par[] = {"1000", "100","1","0",};
    
    if (ac == 1) {
        printf(" 8110_speed : now status configuration at %x, force_speed_set = %x \n",if_frequency, !if_in_attach);
        printf(" usage : r8110_speed [1000|100] [1|0]\n");
        return;
    } else if (ac != 3) {
        printf(" usage : r8110_speed [1000|100] [1|0]\n");
        return;
    }
    if (!strcmp(av[1],par[0]))
        if_frequency = 0x13;
    else if (!strcmp(av[1],par[1]))
        if_frequency = 0xb;
    else {
        printf("error option 1, %s\n",av[1]);
        return;
    }
    if (!strcmp(av[2],par[2]))
        if_in_attach = 0;
    else if (!strcmp(av[2],par[3]))
        if_in_attach = 1;
    else printf("error option 2, %s\n",av[2]);
    return;
}

static int cmd_set_8110()
				  
{
	int auto_nego, giga_ctrl;
    int old_value;
    int i;
    struct rtl8169_private *tp;

    tp = myRTL;

	old_value = mdio_read(tp, PHY_CTRL_REG);
    printf("the old PHY_CTRL_REG value is %x\n", old_value);
    //old_value &= ~0x1000;       //disable auto-negotiation
    old_value |= 0x0200;       //set to re auto negotiation
    //old_value &= ~0x2000;       
    mdio_write(tp, PHY_CTRL_REG, old_value);
#if 0 
	for (i = 200000000; i > 0; i--) {
		/* Check if the RTL8169 has completed */
		if (mdio_read(tp, PHY_CTRL_REG) & 0x8000 == 0) {
			break;
		}
		delay(200);
	}
#endif
    printf("----------------> %d\n", i);
#if 0
	old_value = mdio_read(tp, PHY_CTRL_REG);
    old_value |= 0x0040;       //set to 1000M
    old_value &= ~0x2000;       //disable auto-negotiation
    mdio_write(tp, PHY_CTRL_REG, old_value);
   
	for (i = 2000; i > 0; i--) {
		/* Check if the RTL8169 has completed */
		if (mdio_read(tp, PHY_CTRL_REG) & 0x8000 == 0) {
			break;
		}
		delay(200);
	}
    printf("----------------> %d\n", i);
#endif
	return 0;
}

int db8169 = 0;
static int netdmp_cmd (int ac, char *av[])
{
	struct ifnet *ifp;
	int i;
       if(myRTL == NULL){
            printf("error: device is not working !\n");
	     return -1;
	}
	
	ifp = &myRTL->arpcom.ac_if;
	printf("if_snd.mb_head: %x\n", ifp->if_snd.ifq_head);	
	printf("if_snd.ifq_snd.ifqlen =%d\n", ifp->if_snd.ifq_len);
	printf("ChipCmd= %x\n", RTL_R8(myRTL, ChipCmd));
	printf("ifnet address=%8x\n", ifp);
	printf("if_flags = %x\n", ifp->if_flags);
	printf("Intr =%x\n", RTL_R16(myRTL, IntrStatus));
	printf("TxConfig =%x\n", RTL_R32(myRTL, TxConfig));
	printf("RxConfig =%x\n", RTL_R32(myRTL, RxConfig));
	printf("cur_rx =%x\n", myRTL->cur_rx);
	printf("cur_tx =%d, dirty_tx=%d\n", myRTL->cur_tx, myRTL->dirty_tx);
	printf("RX Descriptor ring addr: %x \n", myRTL->RxDescArray);
	printf("TX Descriptor ring addr: %x \n", myRTL->TxDescArray);
	for(i = 0; i < NUM_RX_DESC; i ++){
             printf("RX buffer i addr: %x \n", myRTL->rx_buffer[i]);
	}
	for(i = 0; i < NUM_TX_DESC; i ++){
             printf("TX buffer i addr: %x \n", myRTL->tx_buffer[i]);
	}

	if(ac==2){
		if(strcmp(av[1], "on")==0){
			db8169=1;
		}
		else if(strcmp(av[1], "off")==0){
			db8169=0;
		}else {
			int x=atoi(av[1]);
			max_interrupt_work=x;
		}
		
	}
	printf("db8139=%d\n",db8169);
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
	{"8169"},

	{"ifm", "", NULL,
		    "Set 8169 interface mode", cmd_ifm, 1, 2, 0},
	{"setmac", "", NULL,
		    "Set mac address into 8169 eeprom", cmd_setmac, 1, 5, 0},
	{"readrom", "", NULL,
			"dump rtl8169 eprom content", cmd_reprom, 1, 1,0},
	{"writerom", "", NULL,
			"dump rtl8169 eprom content", cmd_wrprom, 1, 1,0},
    {"r8169dump", "", NULL, "dump rtl8169 parameters", netdmp_cmd, 1, 2, 0},
    {"r8110_reneo","",NULL, "set re-auto xx of rtl 8110 phy", cmd_set_8110,1,2,0},
    {"r8110_speed","",NULL, "disable/enable 8110 ifaddr config retry for 1000M/100M", cmd_set_frequency,1,3,0},
	{0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

#endif

