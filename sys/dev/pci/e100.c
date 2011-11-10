#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/syslog.h>

#include <sys/systm.h>
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
typedef struct FILE {
	int fd;
	int valid;
	int ungetcflag;
	int ungetchar;
} FILE;
extern FILE _iob[];
#define serialout (&_iob[1])

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

#endif /* __NetBSD__ || __OpenBSD__ */

#include <fb/sis/pci_ids.h>
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

#include <linux/types.h>
#include <include/ri.h>
//#include <linux/pci.h>
//-----------------------mii.h---------
/* Generic MII registers. */

#define MII_BMCR            0x00        /* Basic mode control register */
#define MII_BMSR            0x01        /* Basic mode status register  */
#define MII_PHYSID1         0x02        /* PHYS ID 1                   */
#define MII_PHYSID2         0x03        /* PHYS ID 2                   */
#define MII_ADVERTISE       0x04        /* Advertisement control reg   */
#define MII_LPA             0x05        /* Link partner ability reg    */
#define MII_EXPANSION       0x06        /* Expansion register          */
#define MII_CTRL1000        0x09        /* 1000BASE-T control          */
#define MII_STAT1000        0x0a        /* 1000BASE-T status           */
#define MII_ESTATUS	    0x0f	/* Extended Status */
#define MII_DCOUNTER        0x12        /* Disconnect counter          */
#define MII_FCSCOUNTER      0x13        /* False carrier counter       */
#define MII_NWAYTEST        0x14        /* N-way auto-neg test reg     */
#define MII_RERRCOUNTER     0x15        /* Receive error counter       */
#define MII_SREVISION       0x16        /* Silicon revision            */
#define MII_RESV1           0x17        /* Reserved...                 */
#define MII_LBRERROR        0x18        /* Lpback, rx, bypass error    */
#define MII_PHYADDR         0x19        /* PHY address                 */
#define MII_RESV2           0x1a        /* Reserved...                 */
#define MII_TPISTATUS       0x1b        /* TPI status for 10mbps       */
#define MII_NCONFIG         0x1c        /* Network interface config    */

/* Basic mode control register. */
#define BMCR_RESV               0x003f  /* Unused...                   */
#define BMCR_SPEED1000		0x0040  /* MSB of Speed (1000)         */
#define BMCR_CTST               0x0080  /* Collision test              */
#define BMCR_FULLDPLX           0x0100  /* Full duplex                 */
#define BMCR_ANRESTART          0x0200  /* Auto negotiation restart    */
#define BMCR_ISOLATE            0x0400  /* Disconnect DP83840 from MII */
#define BMCR_PDOWN              0x0800  /* Powerdown the DP83840       */
#define BMCR_ANENABLE           0x1000  /* Enable auto negotiation     */
#define BMCR_SPEED100           0x2000  /* Select 100Mbps              */
#define BMCR_LOOPBACK           0x4000  /* TXD loopback bits           */
#define BMCR_RESET              0x8000  /* Reset the DP83840           */

/* Basic mode status register. */
#define BMSR_ERCAP              0x0001  /* Ext-reg capability          */
#define BMSR_JCD                0x0002  /* Jabber detected             */
#define BMSR_LSTATUS            0x0004  /* Link status                 */
#define BMSR_ANEGCAPABLE        0x0008  /* Able to do auto-negotiation */
#define BMSR_RFAULT             0x0010  /* Remote fault detected       */
#define BMSR_ANEGCOMPLETE       0x0020  /* Auto-negotiation complete   */
#define BMSR_RESV               0x00c0  /* Unused...                   */
#define BMSR_ESTATEN		0x0100	/* Extended Status in R15 */
#define BMSR_100FULL2		0x0200	/* Can do 100BASE-T2 HDX */
#define BMSR_100HALF2		0x0400	/* Can do 100BASE-T2 FDX */
#define BMSR_10HALF             0x0800  /* Can do 10mbps, half-duplex  */
#define BMSR_10FULL             0x1000  /* Can do 10mbps, full-duplex  */
#define BMSR_100HALF            0x2000  /* Can do 100mbps, half-duplex */
#define BMSR_100FULL            0x4000  /* Can do 100mbps, full-duplex */
#define BMSR_100BASE4           0x8000  /* Can do 100mbps, 4k packets  */

/* Advertisement control register. */
#define ADVERTISE_SLCT          0x001f  /* Selector bits               */
#define ADVERTISE_CSMA          0x0001  /* Only selector supported     */
#define ADVERTISE_10HALF        0x0020  /* Try for 10mbps half-duplex  */
#define ADVERTISE_1000XFULL     0x0020  /* Try for 1000BASE-X full-duplex */
#define ADVERTISE_10FULL        0x0040  /* Try for 10mbps full-duplex  */
#define ADVERTISE_1000XHALF     0x0040  /* Try for 1000BASE-X half-duplex */
#define ADVERTISE_100HALF       0x0080  /* Try for 100mbps half-duplex */
#define ADVERTISE_1000XPAUSE    0x0080  /* Try for 1000BASE-X pause    */
#define ADVERTISE_100FULL       0x0100  /* Try for 100mbps full-duplex */
#define ADVERTISE_1000XPSE_ASYM 0x0100  /* Try for 1000BASE-X asym pause */
#define ADVERTISE_100BASE4      0x0200  /* Try for 100mbps 4k packets  */
#define ADVERTISE_PAUSE_CAP     0x0400  /* Try for pause               */
#define ADVERTISE_PAUSE_ASYM    0x0800  /* Try for asymetric pause     */
#define ADVERTISE_RESV          0x1000  /* Unused...                   */
#define ADVERTISE_RFAULT        0x2000  /* Say we can detect faults    */
#define ADVERTISE_LPACK         0x4000  /* Ack link partners response  */
#define ADVERTISE_NPAGE         0x8000  /* Next page bit               */

#define ADVERTISE_FULL (ADVERTISE_100FULL | ADVERTISE_10FULL | \
			ADVERTISE_CSMA)
#define ADVERTISE_ALL (ADVERTISE_10HALF | ADVERTISE_10FULL | \
                       ADVERTISE_100HALF | ADVERTISE_100FULL)

/* Link partner ability register. */
#define LPA_SLCT                0x001f  /* Same as advertise selector  */
#define LPA_10HALF              0x0020  /* Can do 10mbps half-duplex   */
#define LPA_1000XFULL           0x0020  /* Can do 1000BASE-X full-duplex */
#define LPA_10FULL              0x0040  /* Can do 10mbps full-duplex   */
#define LPA_1000XHALF           0x0040  /* Can do 1000BASE-X half-duplex */
#define LPA_100HALF             0x0080  /* Can do 100mbps half-duplex  */
#define LPA_1000XPAUSE          0x0080  /* Can do 1000BASE-X pause     */
#define LPA_100FULL             0x0100  /* Can do 100mbps full-duplex  */
#define LPA_1000XPAUSE_ASYM     0x0100  /* Can do 1000BASE-X pause asym*/
#define LPA_100BASE4            0x0200  /* Can do 100mbps 4k packets   */
#define LPA_PAUSE_CAP           0x0400  /* Can pause                   */
#define LPA_PAUSE_ASYM          0x0800  /* Can pause asymetrically     */
#define LPA_RESV                0x1000  /* Unused...                   */
#define LPA_RFAULT              0x2000  /* Link partner faulted        */
#define LPA_LPACK               0x4000  /* Link partner acked us       */
#define LPA_NPAGE               0x8000  /* Next page bit               */

#define LPA_DUPLEX		(LPA_10FULL | LPA_100FULL)
#define LPA_100			(LPA_100FULL | LPA_100HALF | LPA_100BASE4)

/* Expansion register for auto-negotiation. */
#define EXPANSION_NWAY          0x0001  /* Can do N-way auto-nego      */
#define EXPANSION_LCWP          0x0002  /* Got new RX page code word   */
#define EXPANSION_ENABLENPAGE   0x0004  /* This enables npage words    */
#define EXPANSION_NPCAPABLE     0x0008  /* Link partner supports npage */
#define EXPANSION_MFAULTS       0x0010  /* Multiple faults detected    */
#define EXPANSION_RESV          0xffe0  /* Unused...                   */

#define ESTATUS_1000_TFULL	0x2000	/* Can do 1000BT Full */
#define ESTATUS_1000_THALF	0x1000	/* Can do 1000BT Half */

/* N-way test register. */
#define NWAYTEST_RESV1          0x00ff  /* Unused...                   */
#define NWAYTEST_LOOPBACK       0x0100  /* Enable loopback for N-way   */
#define NWAYTEST_RESV2          0xfe00  /* Unused...                   */

/* 1000BASE-T Control register */
#define ADVERTISE_1000FULL      0x0200  /* Advertise 1000BASE-T full duplex */
#define ADVERTISE_1000HALF      0x0100  /* Advertise 1000BASE-T half duplex */

/* 1000BASE-T Status register */
#define LPA_1000LOCALRXOK       0x2000  /* Link partner local receiver status */
#define LPA_1000REMRXOK         0x1000  /* Link partner remote receiver status */
#define LPA_1000FULL            0x0800  /* Link partner 1000BASE-T full duplex */
//-----------------------
#define AUTONEG_DISABLE		0x00
#define AUTONEG_ENABLE		0x01
//------------------------
#define IRQ_NONE 0
#define IRQ_HANDLED 1
#define 		NET_IP_ALIGN 2	
#define ETH_ALEN	6		/* Octets in one ethernet addr	 */
#define ETH_HLEN	14		/* Total octets in header.	 */
#define ETH_ZLEN	60		/* Min. octets in frame sans FCS */
#define ETH_DATA_LEN	1500		/* Max. octets in payload	 */
#define ETH_FRAME_LEN	1514		/* Max. octets in frame sans FCS */
#define VLAN_ETH_ALEN	6		/* Octets in one ethernet addr	 */
#define VLAN_ETH_HLEN	18		/* Total octets in header.	 */
#define VLAN_ETH_ZLEN	64		/* Min. octets in frame sans FCS */
#define irqreturn_t

#define	ENODATA		61	/* No data available */
typedef unsigned int dma_addr_t;
#  define memcpy(d, s, n)   bcopy((s), (d), (n)) 
#  define memcmp(s1, s2, n) bcmp((s1), (s2), (n)) 
#  define memzero(s, n)     bzero((s), (n))
#define netif_receive_skb(x) netif_rx(x)
#define PCI_SUBSYSTEM_VENDOR_ID 0x2c
#define PCI_ANY_ID (~0)
#define KERN_DEBUG
#define kmalloc(size,...)  malloc(size,M_DEVBUF, M_DONTWAIT)
#define kfree(addr,...) free(addr,M_DEVBUF);
#define in_interrupt(...) (0)
#define pci_dma_mapping_error(x) (0)
#define ioremap(a,s) (((long)a)|0xb0000000)
#define iounmap(x) 
#define netdev_priv(dev) dev->priv
#define readb(addr)     (*(volatile unsigned char *)(addr))
#define readw(addr)     (*(volatile unsigned short *)(addr))
#define readl(addr)     (*(volatile unsigned int *)(addr))
#define writeb(b,addr) ((*(volatile unsigned char *)(addr)) = (b))
#define writew(w,addr) ((*(volatile unsigned short *)(addr)) = (w))
#define writel(l,addr) ((*(volatile unsigned int *)(addr)) = (l))
#define iowrite8(b,addr) ((*(volatile unsigned char *)(addr)) = (b))
#define iowrite16(w,addr) ((*(volatile unsigned short *)(addr)) = (w))
#define iowrite32(l,addr) ((*(volatile unsigned int *)(addr)) = (l))

#define ioread8(addr)     (*(volatile unsigned char *)(addr))
#define ioread16(addr)     (*(volatile unsigned short *)(addr))
#define ioread32(addr)     (*(volatile unsigned int *)(addr))
#define KERN_WARNING
#define printk printf
#define le16_to_cpu(x) (x)
#define KERN_INFO
#define PCI_REVISION_ID 8
#define spin_lock_init(...)
#define spin_lock(...)
#define spin_unlock(...)
#define likely(x) (x)
#define unlikely(x) (x)
extern int ticks;
#define jiffies ticks
#define KERN_ERR
#define KERN_NOTICE
#define ETH_ALEN	6		/* Octets in one ethernet addr	 */
#define MODULE_DESCRIPTION(...)
#define MODULE_AUTHOR(...)
#define MODULE_LICENSE(...)
#define MODULE_VERSION(...)
#define module_param(...)
#define MODULE_PARM_DESC(...)
#define MODULE_DEVICE_TABLE(...)
#define udelay delay
#define __iomem
#define ____cacheline_aligned __attribute__((aligned(32)))
#define VLAN_ETH_DATA_LEN	1500	/* Max. octets in payload	 */
#define VLAN_ETH_FRAME_LEN	1518	/* Max. octets in frame sans FCS */
#define DUPLEX_HALF		0x00
#define DUPLEX_FULL		0x01
#define MII_LPA             0x05        /* Link partner ability reg    */
#define MII_BMCR            0x00        /* Basic mode control register */
#define MII_STAT1000        0x0a        /* 1000BASE-T status           */
#define SPEED_10		10
#define SPEED_100		100
#define SPEED_1000		1000
#define SPEED_2500		2500
#define SPEED_10000		10000
#define MII_BMSR            0x01        /* Basic mode status register  */
#define PORT_MII		0x02
#define XCVR_INTERNAL		0x00
#define MII_ADVERTISE       0x04        /* Advertisement control reg   */
#define MII_CTRL1000        0x09        /* 1000BASE-T control          */
#define ADVERTISE_1000FULL      0x0200  /* Advertise 1000BASE-T full duplex */
#define ADVERTISE_1000HALF      0x0100  /* Advertise 1000BASE-T half duplex */
/* Advertisement control register. */
#define ADVERTISE_SLCT          0x001f  /* Selector bits               */
#define ADVERTISE_CSMA          0x0001  /* Only selector supported     */
#define ADVERTISE_10HALF        0x0020  /* Try for 10mbps half-duplex  */
#define ADVERTISE_1000XFULL     0x0020  /* Try for 1000BASE-X full-duplex */
#define ADVERTISE_10FULL        0x0040  /* Try for 10mbps full-duplex  */
#define ADVERTISE_1000XHALF     0x0040  /* Try for 1000BASE-X half-duplex */
#define ADVERTISE_100HALF       0x0080  /* Try for 100mbps half-duplex */
#define ADVERTISE_1000XPAUSE    0x0080  /* Try for 1000BASE-X pause    */
#define ADVERTISE_100FULL       0x0100  /* Try for 100mbps full-duplex */
#define ADVERTISE_1000XPSE_ASYM 0x0100  /* Try for 1000BASE-X asym pause */
#define ADVERTISE_100BASE4      0x0200  /* Try for 100mbps 4k packets  */
#define ADVERTISE_PAUSE_CAP     0x0400  /* Try for pause               */
#define ADVERTISE_PAUSE_ASYM    0x0800  /* Try for asymetric pause     */
#define ADVERTISE_RESV          0x1000  /* Unused...                   */
#define ADVERTISE_RFAULT        0x2000  /* Say we can detect faults    */
#define ADVERTISE_LPACK         0x4000  /* Ack link partners response  */
#define ADVERTISE_NPAGE         0x8000  /* Next page bit               */
#define SUPPORTED_10baseT_Half		(1 << 0)
#define SUPPORTED_10baseT_Full		(1 << 1)
#define SUPPORTED_100baseT_Half		(1 << 2)
#define SUPPORTED_100baseT_Full		(1 << 3)
#define SUPPORTED_1000baseT_Half	(1 << 4)
#define SUPPORTED_1000baseT_Full	(1 << 5)
#define SUPPORTED_Autoneg		(1 << 6)
#define SUPPORTED_TP			(1 << 7)
#define SUPPORTED_AUI			(1 << 8)
#define SUPPORTED_MII			(1 << 9)
#define SUPPORTED_FIBRE			(1 << 10)
#define SUPPORTED_BNC			(1 << 11)
#define SUPPORTED_10000baseT_Full	(1 << 12)
#define SUPPORTED_Pause			(1 << 13)
#define SUPPORTED_Asym_Pause		(1 << 14)

/* Indicates what features are advertised by the interface. */
#define ADVERTISED_10baseT_Half		(1 << 0)
#define ADVERTISED_10baseT_Full		(1 << 1)
#define ADVERTISED_100baseT_Half	(1 << 2)
#define ADVERTISED_100baseT_Full	(1 << 3)
#define ADVERTISED_1000baseT_Half	(1 << 4)
#define ADVERTISED_1000baseT_Full	(1 << 5)
#define ADVERTISED_Autoneg		(1 << 6)
#define ADVERTISED_TP			(1 << 7)
#define ADVERTISED_AUI			(1 << 8)
#define ADVERTISED_MII			(1 << 9)
#define ADVERTISED_FIBRE		(1 << 10)
#define ADVERTISED_BNC			(1 << 11)
#define ADVERTISED_10000baseT_Full	(1 << 12)
#define ADVERTISED_Pause		(1 << 13)
#define ADVERTISED_Asym_Pause		(1 << 14)

#define HZ 100
static inline void msleep(int microseconds){
int i;
for(i=0;i<microseconds;i++)delay(1000);
}
#define le32_to_cpu(x) (x)
#define cpu_to_le32(x) (x)
#define cpu_to_le16(x) (x)
#define spin_lock_irq(...)
#define spin_unlock_irq(...)
#define spin_lock_irqsave(...)
#define spin_unlock_irqrestore(...)
#define netif_start_queue(...)
#define netif_stop_queue(...)
#define netif_queue_stopped(...) 0
#define pci_enable_wake(...) (0)
static int irqstate=0;
static void wmb(void){}

#define  request_irq(irq,b,c,d,e) (irqstate|=(1<<irq),0)

#define free_irq(irq,b) (irqstate &=~(1<<irq),0)


/* This should work for both 32 and 64 bit userland. */
struct ethtool_cmd {
	u32	cmd;
	u32	supported;	/* Features this interface supports */
	u32	advertising;	/* Features this interface advertises */
	u16	speed;		/* The forced speed, 10Mb, 100Mb, gigabit */
	u8	duplex;		/* Duplex, half or full */
	u8	port;		/* Which connector port */
	u8	phy_address;
	u8	transceiver;	/* Which transceiver to use */
	u8	autoneg;	/* Enable or disable autonegotiation */
	u32	maxtxpkt;	/* Tx pkts before generating tx int */
	u32	maxrxpkt;	/* Rx pkts before generating rx int */
	u32	reserved[4];
};
struct mii_if_info {
	int phy_id;
	int advertising;
	int phy_id_mask;
	int reg_num_mask;

	unsigned int full_duplex : 1;	/* is full duplex? */
	unsigned int force_media : 1;	/* is autoneg. disabled? */
	unsigned int supports_gmii : 1; /* are GMII registers supported? */

	struct net_device *dev;
	int (*mdio_read) (struct net_device *dev, int phy_id, int location);
	void (*mdio_write) (struct net_device *dev, int phy_id, int location, int val);
};

static inline void netif_carrier_on(struct net_device *dev);
static inline void netif_carrier_off(struct net_device *dev);
void mii_check_link (struct mii_if_info *mii)
{
	int cur_link = mii_link_ok(mii);
	int prev_link = netif_carrier_ok(mii->dev);

	if (cur_link && !prev_link)
		netif_carrier_on(mii->dev);
	else if (prev_link && !cur_link)
		netif_carrier_off(mii->dev);
}

static inline unsigned int mii_nway_result (unsigned int negotiated)
{
	unsigned int ret;

	if (negotiated & LPA_100FULL)
		ret = LPA_100FULL;
	else if (negotiated & LPA_100BASE4)
		ret = LPA_100BASE4;
	else if (negotiated & LPA_100HALF)
		ret = LPA_100HALF;
	else if (negotiated & LPA_10FULL)
		ret = LPA_10FULL;
	else
		ret = LPA_10HALF;

	return ret;
}

#if 1
int mii_ethtool_gset(struct mii_if_info *mii, struct ethtool_cmd *ecmd)
{
	struct net_device *dev = mii->dev;
	u32 advert, bmcr, lpa, nego;
	u32 advert2 = 0, bmcr2 = 0, lpa2 = 0;

	ecmd->supported =
	    (SUPPORTED_10baseT_Half | SUPPORTED_10baseT_Full |
	     SUPPORTED_100baseT_Half | SUPPORTED_100baseT_Full |
	     SUPPORTED_Autoneg | SUPPORTED_TP | SUPPORTED_MII);
	if (mii->supports_gmii)
		ecmd->supported |= SUPPORTED_1000baseT_Half |
			SUPPORTED_1000baseT_Full;

	/* only supports twisted-pair */
	ecmd->port = PORT_MII;

	/* only supports internal transceiver */
	ecmd->transceiver = XCVR_INTERNAL;

	/* this isn't fully supported at higher layers */
	ecmd->phy_address = mii->phy_id;

	ecmd->advertising = ADVERTISED_TP | ADVERTISED_MII;
	advert = mii->mdio_read(dev, mii->phy_id, MII_ADVERTISE);
	if (mii->supports_gmii)
		advert2 = mii->mdio_read(dev, mii->phy_id, MII_CTRL1000);

	if (advert & ADVERTISE_10HALF)
		ecmd->advertising |= ADVERTISED_10baseT_Half;
	if (advert & ADVERTISE_10FULL)
		ecmd->advertising |= ADVERTISED_10baseT_Full;
	if (advert & ADVERTISE_100HALF)
		ecmd->advertising |= ADVERTISED_100baseT_Half;
	if (advert & ADVERTISE_100FULL)
		ecmd->advertising |= ADVERTISED_100baseT_Full;
	if (advert2 & ADVERTISE_1000HALF)
		ecmd->advertising |= ADVERTISED_1000baseT_Half;
	if (advert2 & ADVERTISE_1000FULL)
		ecmd->advertising |= ADVERTISED_1000baseT_Full;

	bmcr = mii->mdio_read(dev, mii->phy_id, MII_BMCR);
	lpa = mii->mdio_read(dev, mii->phy_id, MII_LPA);
	if (mii->supports_gmii) {
		bmcr2 = mii->mdio_read(dev, mii->phy_id, MII_CTRL1000);
		lpa2 = mii->mdio_read(dev, mii->phy_id, MII_STAT1000);
	}
	if (bmcr & BMCR_ANENABLE) {
		ecmd->advertising |= ADVERTISED_Autoneg;
		ecmd->autoneg = AUTONEG_ENABLE;
		
		nego = mii_nway_result(advert & lpa);
		if ((bmcr2 & (ADVERTISE_1000HALF | ADVERTISE_1000FULL)) & 
		    (lpa2 >> 2))
			ecmd->speed = SPEED_1000;
		else if (nego == LPA_100FULL || nego == LPA_100HALF)
			ecmd->speed = SPEED_100;
		else
			ecmd->speed = SPEED_10;
		if ((lpa2 & LPA_1000FULL) || nego == LPA_100FULL ||
		    nego == LPA_10FULL) {
			ecmd->duplex = DUPLEX_FULL;
			mii->full_duplex = 1;
		} else {
			ecmd->duplex = DUPLEX_HALF;
			mii->full_duplex = 0;
		}
	} else {
		ecmd->autoneg = AUTONEG_DISABLE;

		ecmd->speed = ((bmcr & BMCR_SPEED1000 && 
				(bmcr & BMCR_SPEED100) == 0) ? SPEED_1000 :
			       (bmcr & BMCR_SPEED100) ? SPEED_100 : SPEED_10);
		ecmd->duplex = (bmcr & BMCR_FULLDPLX) ? DUPLEX_FULL : DUPLEX_HALF;
	}

	/* ignore maxtxpkt, maxrxpkt for now */

	return 0;
}
int mii_link_ok (struct mii_if_info *mii)
{
	/* first, a dummy read, needed to latch some MII phys */
	mii->mdio_read(mii->dev, mii->phy_id, MII_BMSR);
	if (mii->mdio_read(mii->dev, mii->phy_id, MII_BMSR) & BMSR_LSTATUS)
		return 1;
	return 0;
}
#endif

#define RTL_W8(tp, reg, val8)   iowrite8 ((val8), tp->base_addr + (reg))
#define RTL_W16(tp, reg, val16) iowrite16 ((val16), tp->base_addr + (reg))
#define RTL_W32(tp, reg, val32) iowrite32 ((val32), tp->base_addr + (reg))
#define RTL_R8(tp, reg)         ioread8 (tp->base_addr + (reg))
#define RTL_R16(tp, reg)                ioread16 (tp->base_addr + (reg))
#define RTL_R32(tp, reg)                ((unsigned long) ioread32 (tp->base_addr + (reg)))

#define EEPROMDATA 0x34
#define EEPROMCTRL 0x36
#define EEP_BUSY   0x8000
#define EEP_WRITE  0x0100
#define EEP_READ   0x0200


#define MII_BMCR            0x00        /* Basic mode control register */
#define MII_BMSR            0x01        /* Basic mode status register  */
#define MII_PHYSID1         0x02        /* PHYS ID 1                   */
#define MII_PHYSID2         0x03        /* PHYS ID 2                   */
#define MII_ADVERTISE       0x04        /* Advertisement control reg   */
#define MII_LPA             0x05        /* Link partner ability reg    */
#define MII_EXPANSION       0x06        /* Expansion register          */
#define MII_CTRL1000        0x09        /* 1000BASE-T control          */
#define MII_STAT1000        0x0a        /* 1000BASE-T status           */
#define MII_ESTATUS	    0x0f	/* Extended Status */
#define MII_DCOUNTER        0x12        /* Disconnect counter          */
#define MII_FCSCOUNTER      0x13        /* False carrier counter       */
#define MII_NWAYTEST        0x14        /* N-way auto-neg test reg     */
#define MII_RERRCOUNTER     0x15        /* Receive error counter       */
#define MII_SREVISION       0x16        /* Silicon revision            */
#define MII_RESV1           0x17        /* Reserved...                 */
#define MII_LBRERROR        0x18        /* Lpback, rx, bypass error    */
#define MII_PHYADDR         0x19        /* PHY address                 */
#define MII_RESV2           0x1a        /* Reserved...                 */
#define MII_TPISTATUS       0x1b        /* TPI status for 10mbps       */
#define MII_NCONFIG         0x1c        /* Network interface config    */

/* Basic mode control register. */
#define BMCR_RESV               0x003f  /* Unused...                   */
#define BMCR_SPEED1000		0x0040  /* MSB of Speed (1000)         */
#define BMCR_CTST               0x0080  /* Collision test              */
#define BMCR_FULLDPLX           0x0100  /* Full duplex                 */
#define BMCR_ANRESTART          0x0200  /* Auto negotiation restart    */
#define BMCR_ISOLATE            0x0400  /* Disconnect DP83840 from MII */
#define BMCR_PDOWN              0x0800  /* Powerdown the DP83840       */
#define BMCR_ANENABLE           0x1000  /* Enable auto negotiation     */
#define BMCR_SPEED100           0x2000  /* Select 100Mbps              */
#define BMCR_LOOPBACK           0x4000  /* TXD loopback bits           */
#define BMCR_RESET              0x8000  /* Reset the DP83840           */

/* Basic mode status register. */
#define BMSR_ERCAP              0x0001  /* Ext-reg capability          */
#define BMSR_JCD                0x0002  /* Jabber detected             */
#define BMSR_LSTATUS            0x0004  /* Link status                 */
#define BMSR_ANEGCAPABLE        0x0008  /* Able to do auto-negotiation */
#define BMSR_RFAULT             0x0010  /* Remote fault detected       */
#define BMSR_ANEGCOMPLETE       0x0020  /* Auto-negotiation complete   */
#define BMSR_RESV               0x00c0  /* Unused...                   */
#define BMSR_ESTATEN		0x0100	/* Extended Status in R15 */
#define BMSR_100FULL2		0x0200	/* Can do 100BASE-T2 HDX */
#define BMSR_100HALF2		0x0400	/* Can do 100BASE-T2 FDX */
#define BMSR_10HALF             0x0800  /* Can do 10mbps, half-duplex  */
#define BMSR_10FULL             0x1000  /* Can do 10mbps, full-duplex  */
#define BMSR_100HALF            0x2000  /* Can do 100mbps, half-duplex */
#define BMSR_100FULL            0x4000  /* Can do 100mbps, full-duplex */
#define BMSR_100BASE4           0x8000  /* Can do 100mbps, 4k packets  */
/* Advertisement control register. */
#define ADVERTISE_SLCT          0x001f  /* Selector bits               */
#define ADVERTISE_CSMA          0x0001  /* Only selector supported     */
#define ADVERTISE_10HALF        0x0020  /* Try for 10mbps half-duplex  */
#define ADVERTISE_1000XFULL     0x0020  /* Try for 1000BASE-X full-duplex */
#define ADVERTISE_10FULL        0x0040  /* Try for 10mbps full-duplex  */
#define ADVERTISE_1000XHALF     0x0040  /* Try for 1000BASE-X half-duplex */
#define ADVERTISE_100HALF       0x0080  /* Try for 100mbps half-duplex */
#define ADVERTISE_1000XPAUSE    0x0080  /* Try for 1000BASE-X pause    */
#define ADVERTISE_100FULL       0x0100  /* Try for 100mbps full-duplex */
#define ADVERTISE_1000XPSE_ASYM 0x0100  /* Try for 1000BASE-X asym pause */
#define ADVERTISE_100BASE4      0x0200  /* Try for 100mbps 4k packets  */
#define ADVERTISE_PAUSE_CAP     0x0400  /* Try for pause               */
#define ADVERTISE_PAUSE_ASYM    0x0800  /* Try for asymetric pause     */
#define ADVERTISE_RESV          0x1000  /* Unused...                   */
#define ADVERTISE_RFAULT        0x2000  /* Say we can detect faults    */
#define ADVERTISE_LPACK         0x4000  /* Ack link partners response  */
#define ADVERTISE_NPAGE         0x8000  /* Next page bit               */

#define ADVERTISE_FULL (ADVERTISE_100FULL | ADVERTISE_10FULL | \
			ADVERTISE_CSMA)
#define ADVERTISE_ALL (ADVERTISE_10HALF | ADVERTISE_10FULL | \
                       ADVERTISE_100HALF | ADVERTISE_100FULL)
typedef unsigned long spinlock_t;
struct net_device_stats
{
    unsigned long   rx_packets;     /* total packets received   */
    unsigned long   tx_packets;     /* total packets transmitted    */
    unsigned long   rx_bytes;       /* total bytes received     */
    unsigned long   tx_bytes;       /* total bytes transmitted  */
    unsigned long   rx_errors;      /* bad packets received     */
    unsigned long   tx_errors;      /* packet transmit problems */
    unsigned long   rx_dropped;     /* no space in linux buffers    */
    unsigned long   tx_dropped;     /* no space available in linux  */
    unsigned long   multicast;      /* multicast packets received   */
    unsigned long   collisions;

    /* detailed rx_errors: */
    unsigned long   rx_length_errors;
    unsigned long   rx_over_errors;     /* receiver ring buff overflow  */
    unsigned long   rx_crc_errors;      /* recved pkt with crc error    */
    unsigned long   rx_frame_errors;    /* recv'd frame alignment error */
    unsigned long   rx_fifo_errors;     /* recv'r fifo overrun      */
    unsigned long   rx_missed_errors;   /* receiver missed packet   */

    /* detailed tx_errors */
    unsigned long   tx_aborted_errors;
    unsigned long   tx_carrier_errors;
    unsigned long   tx_fifo_errors;
    unsigned long   tx_heartbeat_errors;
    unsigned long   tx_window_errors;

    /* for cslip etc */
    unsigned long   rx_compressed;
    unsigned long   tx_compressed;
};

struct pci_device_id {
    unsigned int vendor, device;        /* Vendor and device ID or PCI_ANY_ID */
    unsigned int subvendor, subdevice;  /* Subsystem ID's or PCI_ANY_ID */
    unsigned int class, class_mask;     /* (class,subclass,prog-if) triplet */
    unsigned long driver_data;      /* Data private to the driver */
};
struct pci_dev {
		struct pci_attach_args pa;
		struct net_device *dev;
		int irq;
    unsigned int    devfn;      /* encoded device & function index */
    unsigned short  vendor;
    unsigned short  device;
    unsigned short  subsystem_vendor;
    unsigned short  subsystem_device;
    unsigned int    class;      /* 3 bytes: (base,sub,prog-if) */
};
struct net_device 
{
//most of the fields come from struct fxp_softc
#if defined(__NetBSD__) || defined(__OpenBSD__)
	struct device sc_dev;		/* generic device structures */
	void *sc_ih;			/* interrupt handler cookie */
	bus_space_tag_t sc_st;		/* bus space tag */
	bus_space_handle_t sc_sh;	/* bus space handle */
	pci_chipset_tag_t sc_pc;	/* chipset handle needed by mips */
#else
	struct caddr_t csr;		/* control/status registers */
#endif /* __NetBSD__ || __OpenBSD__ */
#if defined(__OpenBSD__) || defined(__FreeBSD__)
	struct arpcom arpcom;		/* per-interface network data !!we use this*/
#endif
#if defined(__NetBSD__)
	struct ethercom sc_ethercom;	/* ethernet common part */
#endif
	struct mii_data sc_mii;		/* MII media information */
	char		node_addr[6]; //the net interface's address

	unsigned char *tx_ad[4];   // Transmit buffer
	unsigned char *tx_dma;     // used by dma
	unsigned char *tx_buffer;  // Transmit buffer
	unsigned char *tx_buf[4];  // used by driver coping data

	unsigned char *rx_dma;     // Receive buffer, used by dma	
	unsigned char *rx_buffer;     // used by driver 
	//We use mbuf here
	int		packetlen;
	struct pci_dev pcidev;
	void		*priv;	/* driver can hang private data here */
   	unsigned long       mem_end;    /* shared mem end   */
    unsigned long       mem_start;  /* shared mem start */
    unsigned long       base_addr;  /* device I/O address   */
    unsigned int        irq;        /* device IRQ number    */
	int         features;

int flags;
int mtu;
    struct dev_mc_list  *mc_list;   /* Multicast mac addresses  */
char            name[IFNAMSIZ];
	unsigned char dev_addr[6];
    unsigned char       addr_len;   /* hardware address length  */
	    unsigned long       state;
	unsigned long trans_start,last_rx;
	unsigned int opencount;
	unsigned char		perm_addr[6]; /* permanent hw address */
	int mc_count;
};
const static struct pci_device_id *
pci_match_device(const struct pci_device_id *ids, 	struct pci_attach_args *pa )
{
	unsigned short	vendor;
	unsigned short	device,class;
	unsigned short	subsystem_vendor;
	unsigned short	subsystem_device;
	unsigned int i;

		vendor = pa->pa_id & 0xffff;
	    device = (pa->pa_id >> 16) & 0xffff;
		class=(pa->pa_class >>8);
		i=pci_conf_read(0,pa->pa_tag,PCI_SUBSYSTEM_VENDOR_ID);
		subsystem_vendor=i&0xffff;
		subsystem_device=i>>16;

    while (ids->vendor || ids->subvendor || ids->class_mask) {
        if ((ids->vendor == PCI_ANY_ID || ids->vendor == vendor) &&
            (ids->device == PCI_ANY_ID || ids->device == device) &&
            (ids->subvendor == PCI_ANY_ID || ids->subvendor == subsystem_vendor) &&
            (ids->subdevice == PCI_ANY_ID || ids->subdevice == subsystem_device) /*&&
            !((ids->class ^ class) & ids->class_mask)*/)
            return ids;
        ids++;
    }
    return NULL;
}

void pci_set_drvdata(struct pci_dev *dev,void *data)
{
dev->dev=data;
}

static int netif_running(struct net_device *netdev)
{
struct ifnet *ifp = &netdev->arpcom.ac_if;
	return (ifp->if_flags & IFF_RUNNING)?1:0;
}

#define UNCACHED_TO_CACHED(x) PHYS_TO_CACHED(UNCACHED_TO_PHYS(x))
static void pci_free_consistent(struct pci_dev *pdev, size_t size, void *cpu_addr,
            dma_addr_t dma_addr)
{
	kfree(UNCACHED_TO_CACHED(cpu_addr));
}

//pci_alloc_consistent 最后一个参数是DMA地址，返回的是非cache的cpu地址。
static void *pci_alloc_consistent(void *hwdev, size_t size,
		               dma_addr_t * dma_handle)
{
void *buf;
    buf = kmalloc(size,M_DEVBUF, M_DONTWAIT );
    pci_sync_cache(hwdev, buf,size, SYNC_R);

    buf = (unsigned char *)CACHED_TO_UNCACHED(buf);
    *dma_handle =vtophys(buf);

	return (void *)buf;
}

static unsigned short  read_eeprom(unsigned long ioaddr,unsigned int eep_addr);
static  void  write_eeprom(unsigned long ioaddr, unsigned int eep_addr, unsigned short writedata);

struct sk_buff {
    unsigned int    len;            /* Length of actual data            */
    unsigned char   *data;          /* Data head pointer                */
    unsigned char   *head;          /* Data head pointer                */
	unsigned char protocol;
	struct net_device *dev;
};
#define dev_kfree_skb dev_kfree_skb_any
#define dev_kfree_skb_irq dev_kfree_skb_any

static inline void dev_kfree_skb_any(struct sk_buff *skb)
{
	kfree(skb->head);
	kfree(skb);
}

static void skb_put(struct sk_buff *skb, unsigned int len)
{
    skb->len+=len;
}

static struct sk_buff *dev_alloc_skb(unsigned int length){
    struct sk_buff *skb=kmalloc(sizeof(struct sk_buff),GFP_KERNEL);
	if(skb)
	{
    skb->len=0;
    skb->data=skb->head=(void *) kmalloc(length,GFP_KERNEL);
	}
	if(!skb||!skb->data){printf("not enough memory!enlarge NKMEMCLUSTERS in sys/arch/mips/include/param.h");}
	return skb&&skb->data?skb:0;
}

static inline void skb_reserve(struct sk_buff *skb, unsigned int len)
{
    skb->data+=len;
}

static inline dma_addr_t pci_map_single(struct pci_dev *hwdev, void *ptr,
		                    size_t size, int direction)
{
	    unsigned long addr = (unsigned long) ptr;
pci_sync_cache(hwdev,addr,size, SYNC_W);
return _pci_dmamap(addr,size);
}

static inline void pci_unmap_single(struct pci_dev *hwdev, dma_addr_t dma_addr,
                    size_t size, int direction)
{
pci_sync_cache(hwdev, _pci_cpumap(dma_addr,size), size, SYNC_R);
}



#define PCI_DMA_BIDIRECTIONAL	0
#define PCI_DMA_TODEVICE	1
#define PCI_DMA_FROMDEVICE	2
#define PCI_DMA_NONE		3

#define pci_dma_sync_single_for_device pci_dma_sync_single_for_cpu
static inline void
pci_dma_sync_single_for_cpu(struct pci_dev *hwdev, dma_addr_t dma_handle,
		    size_t size, int direction)
{
pci_sync_cache(hwdev, _pci_cpumap(dma_handle,size), size, (direction==PCI_DMA_TODEVICE)?SYNC_W:SYNC_R);
}

static struct mbuf * getmbuf()
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
		pci_sync_cache(0, (vm_offset_t)m->m_ext.ext_buf,
				MCLBYTES, SYNC_R);
	}
	m->m_data += RFA_ALIGNMENT_FUDGE;
#else
	m->m_data += RFA_ALIGNMENT_FUDGE;
#endif
	return m;
}

static inline int netif_rx(struct sk_buff *skb)
{
	struct mbuf *m;
	struct ether_header *eh;
	struct net_device *netdev=skb->dev;
	struct ifnet *ifp = &netdev->arpcom.ac_if;
    m =getmbuf();
    if (m == NULL){
        printf("getmbuf failed in  netif_rx\n");
        return 0; // no successful
    }
skb->len=skb->len+4;//bug
#if 0
	{
	char str[80];
	sprintf(str,"pcs -1;d1 0x%x %d",skb->data,skb->len);
	printf("m=%x,m->m_data=%x\n%s\n",m,m->m_data,str);
	do_cmd(str);
	}
#endif
        bcopy(skb->data, mtod(m, caddr_t), skb->len);

    //hand up  the received package to upper protocol for further dealt
    m->m_pkthdr.rcvif = ifp;
    m->m_pkthdr.len = m->m_len = skb->len -sizeof(struct ether_header);

    eh=mtod(m, struct ether_header *);

    m->m_data += sizeof(struct ether_header);
    //printf("%s, etype %x:\n", __FUNCTION__, eh->ether_type);
    ether_input(ifp, eh, m);
	dev_kfree_skb_any(skb);
	return 0;
}

static inline void *pci_get_drvdata (struct pci_dev *pdev)
{
    return pdev->dev;
}
static int pci_read_config_dword(struct pci_dev *linuxpd, int reg, u32 *val)
{
	if ((reg & 3) || reg < 0 || reg >= 0x100) {
        	printf ("pci_read_config_dword: bad reg %x\n", reg);
        	return -1;
    	}
	*val=_pci_conf_read(linuxpd->pa.pa_tag, reg);       
	return 0;
}

static int pci_write_config_dword(struct pci_dev *linuxpd, int reg, u32 val)
{
    if ((reg & 3) || reg < 0 || reg >= 0x100) {
	    printf ("pci_write_config_dword: bad reg %x\n", reg);
	return -1;
    }
   _pci_conf_write(linuxpd->pa.pa_tag,reg,val); 
   return 0;
}

static int pci_read_config_word(struct pci_dev *linuxpd, int reg, u16 *val)
{
	if ((reg & 1) || reg < 0 || reg >= 0x100) {
        	printf ("pci_read_config_word: bad reg %x\n", reg);
        	return -1;
    	}
	*val=_pci_conf_readn(linuxpd->pa.pa_tag,reg,2);       
	return 0;
}

static int pci_write_config_word(struct pci_dev *linuxpd, int reg, u16 val)
{
    if ((reg & 1) || reg < 0 || reg >= 0x100) {
	    printf ("pci_write_config_word: bad reg %x\n", reg);
	return -1;
    }
   _pci_conf_writen(linuxpd->pa.pa_tag,reg,val,2); 
   return 0;
}

static int pci_read_config_byte(struct pci_dev *linuxpd, int reg, u8 *val)
{
    if (reg < 0 || reg >= 0x100) {
	    printf ("pci_write_config_word: bad reg %x\n", reg);
	return -1;
    }
	*val=_pci_conf_readn(linuxpd->pa.pa_tag,reg,1);       
	return 0;
}

static int pci_write_config_byte(struct pci_dev *linuxpd, int reg, u8 val)
{
    if (reg < 0 || reg >= 0x100) {
	    printf ("pci_write_config_word: bad reg %x\n", reg);
	return -1;
    }
   _pci_conf_writen(linuxpd->pa.pa_tag,reg,val,1); 
   return 0;
}
#define PCI_MAP_REG_START 0x10
static int pci_resource_start(struct pci_dev *pdev,int bar)
{
	int reg=PCI_MAP_REG_START+bar*4;
	int rv,start,size,flags,type;
		type=pci_mapreg_type(0,pdev->pa.pa_tag,reg);
		rv=pci_mapreg_info(0,pdev->pa.pa_tag,reg,type,&start,&size,&flags);
		if (PCI_MAPREG_TYPE(type) == PCI_MAPREG_TYPE_IO)
			start=/*pdev->pa.pa_iot->bus_base+*/start;
		else	
			start= pdev->pa.pa_memt->bus_base + start;
	return rv?0:start;	
}

static int pci_resource_end(struct pci_dev *pdev,int bar)
{
	int reg=PCI_MAP_REG_START+bar*4;
	int rv,start,size,flags,type;
		type=pci_mapreg_type(0,pdev->pa.pa_tag,reg);
		rv=pci_mapreg_info(0,pdev->pa.pa_tag,reg,type,&start,&size,&flags);
		if (PCI_MAPREG_TYPE(type) == PCI_MAPREG_TYPE_IO)
			start=/*pdev->pa.pa_iot->bus_base+*/start;
		else	
			start=pdev->pa.pa_memt->bus_base+ start;
	return rv?0:start+size;	
}

static int pci_resource_len(struct pci_dev *pdev,int bar)
{
    int reg=PCI_MAP_REG_START+bar*4;
    int rv,start,size,flags;
        rv=pci_mapreg_type(0,pdev->pa.pa_tag,reg);
        rv=pci_mapreg_info(0,pdev->pa.pa_tag,reg,rv,&start,&size,&flags);
    return rv?0:size;
}

static inline int netif_carrier_ok(struct net_device *dev)
{
    return dev->state&1;
}

static inline void netif_carrier_on(struct net_device *dev)
{
	dev->state |=1;
}

static inline void netif_carrier_off(struct net_device *dev)
{
	dev->state &=~1;
}
//--------------------------------------------------------------------
/*******************************************************************************


  Copyright(c) 1999 - 2005 Intel Corporation. All rights reserved.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 59
  Temple Place - Suite 330, Boston, MA  02111-1307, USA.

  The full GNU General Public License is included in this distribution in the
  file called LICENSE.

  Contact Information:
  Linux NICS <linux.nics@intel.com>
  Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497

*******************************************************************************/

/*
 *	e100.c: Intel(R) PRO/100 ethernet driver
 *
 *	(Re)written 2003 by scott.feldman@intel.com.  Based loosely on
 *	original e100 driver, but better described as a munging of
 *	e100, e1000, eepro100, tg3, 8139cp, and other drivers.
 *
 *	References:
 *		Intel 8255x 10/100 Mbps Ethernet Controller Family,
 *		Open Source Software Developers Manual,
 *		http://sourceforge.net/projects/e1000
 *
 *
 *	                      Theory of Operation
 *
 *	I.   General
 *
 *	The driver supports Intel(R) 10/100 Mbps PCI Fast Ethernet
 *	controller family, which includes the 82557, 82558, 82559, 82550,
 *	82551, and 82562 devices.  82558 and greater controllers
 *	integrate the Intel 82555 PHY.  The controllers are used in
 *	server and client network interface cards, as well as in
 *	LAN-On-Motherboard (LOM), CardBus, MiniPCI, and ICHx
 *	configurations.  8255x supports a 32-bit linear addressing
 *	mode and operates at 33Mhz PCI clock rate.
 *
 *	II.  Driver Operation
 *
 *	Memory-mapped mode is used exclusively to access the device's
 *	shared-memory structure, the Control/Status Registers (CSR). All
 *	setup, configuration, and control of the device, including queuing
 *	of Tx, Rx, and configuration commands is through the CSR.
 *	cmd_lock serializes accesses to the CSR command register.  cb_lock
 *	protects the shared Command Block List (CBL).
 *
 *	8255x is highly MII-compliant and all access to the PHY go
 *	through the Management Data Interface (MDI).  Consequently, the
 *	driver leverages the mii.c library shared with other MII-compliant
 *	devices.
 *
 *	Big- and Little-Endian byte order as well as 32- and 64-bit
 *	archs are supported.  Weak-ordered memory and non-cache-coherent
 *	archs are supported.
 *
 *	III. Transmit
 *
 *	A Tx skb is mapped and hangs off of a TCB.  TCBs are linked
 *	together in a fixed-size ring (CBL) thus forming the flexible mode
 *	memory structure.  A TCB marked with the suspend-bit indicates
 *	the end of the ring.  The last TCB processed suspends the
 *	controller, and the controller can be restarted by issue a CU
 *	resume command to continue from the suspend point, or a CU start
 *	command to start at a given position in the ring.
 *
 *	Non-Tx commands (config, multicast setup, etc) are linked
 *	into the CBL ring along with Tx commands.  The common structure
 *	used for both Tx and non-Tx commands is the Command Block (CB).
 *
 *	cb_to_use is the next CB to use for queuing a command; cb_to_clean
 *	is the next CB to check for completion; cb_to_send is the first
 *	CB to start on in case of a previous failure to resume.  CB clean
 *	up happens in interrupt context in response to a CU interrupt.
 *	cbs_avail keeps track of number of free CB resources available.
 *
 * 	Hardware padding of short packets to minimum packet size is
 * 	enabled.  82557 pads with 7Eh, while the later controllers pad
 * 	with 00h.
 *
 *	IV.  Recieve
 *
 *	The Receive Frame Area (RFA) comprises a ring of Receive Frame
 *	Descriptors (RFD) + data buffer, thus forming the simplified mode
 *	memory structure.  Rx skbs are allocated to contain both the RFD
 *	and the data buffer, but the RFD is pulled off before the skb is
 *	indicated.  The data buffer is aligned such that encapsulated
 *	protocol headers are u32-aligned.  Since the RFD is part of the
 *	mapped shared memory, and completion status is contained within
 *	the RFD, the RFD must be dma_sync'ed to maintain a consistent
 *	view from software and hardware.
 *
 *	Under typical operation, the  receive unit (RU) is start once,
 *	and the controller happily fills RFDs as frames arrive.  If
 *	replacement RFDs cannot be allocated, or the RU goes non-active,
 *	the RU must be restarted.  Frame arrival generates an interrupt,
 *	and Rx indication and re-allocation happen in the same context,
 *	therefore no locking is required.  A software-generated interrupt
 *	is generated from the watchdog to recover from a failed allocation
 *	senario where all Rx resources have been indicated and none re-
 *	placed.
 *
 *	V.   Miscellaneous
 *
 * 	VLAN offloading of tagging, stripping and filtering is not
 * 	supported, but driver will accommodate the extra 4-byte VLAN tag
 * 	for processing by upper layers.  Tx/Rx Checksum offloading is not
 * 	supported.  Tx Scatter/Gather is not supported.  Jumbo Frames is
 * 	not supported (hardware limitation).
 *
 * 	MagicPacket(tm) WoL support is enabled/disabled via ethtool.
 *
 * 	Thanks to JC (jchapman@katalix.com) for helping with
 * 	testing/troubleshooting the development driver.
 *
 * 	TODO:
 * 	o several entry points race with dev->close
 * 	o check for tx-no-resources/stop Q races with tx clean/wake Q
 *
 *	FIXES:
 * 2005/12/02 - Michael O'Donnell <Michael.ODonnell at stratus dot com>
 *	- Stratus87247: protect MDI control register manipulations
 */

#define DRV_NAME		"e100"
#define DRV_EXT		"-NAPI"
#define DRV_VERSION		"3.5.10-k2"DRV_EXT
#define DRV_DESCRIPTION		"Intel(R) PRO/100 Network Driver"
#define DRV_COPYRIGHT		"Copyright(c) 1999-2005 Intel Corporation"
#define PFX			DRV_NAME ": "

#define E100_WATCHDOG_PERIOD	(2 * HZ)
#define E100_NAPI_WEIGHT	16



MODULE_DESCRIPTION(DRV_DESCRIPTION);
MODULE_AUTHOR(DRV_COPYRIGHT);
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);

static int debug = 3;
static int eeprom_bad_csum_allow = 0;
module_param(debug, int, 0);
module_param(eeprom_bad_csum_allow, int, 0);
MODULE_PARM_DESC(debug, "Debug level (0=none,...,16=all)");
MODULE_PARM_DESC(eeprom_bad_csum_allow, "Allow bad eeprom checksums");
#if 1
#define DPRINTK(...)
#define DPRINTK1(nlevel, klevel, fmt, args...) \
	(void)( printk("%s: %s: " fmt, nic->netdev->name, \
		__FUNCTION__ , ## args))
#else
#define DPRINTK(nlevel, klevel, fmt, args...) \
	(void)( printk("%s: %s: " fmt, nic->netdev->name, \
		__FUNCTION__ , ## args))
#endif

#define INTEL_8255X_ETHERNET_DEVICE(device_id, ich) {\
	PCI_VENDOR_ID_INTEL, device_id, PCI_ANY_ID, PCI_ANY_ID, \
	PCI_CLASS_NETWORK_ETHERNET << 8, 0xFFFF00, ich }
static struct pci_device_id e100_id_table[] = {
	INTEL_8255X_ETHERNET_DEVICE(0x1029, 0),
	INTEL_8255X_ETHERNET_DEVICE(0x1030, 0),
	INTEL_8255X_ETHERNET_DEVICE(0x1031, 3),
	INTEL_8255X_ETHERNET_DEVICE(0x1032, 3),
	INTEL_8255X_ETHERNET_DEVICE(0x1033, 3),
	INTEL_8255X_ETHERNET_DEVICE(0x1034, 3),
	INTEL_8255X_ETHERNET_DEVICE(0x1038, 3),
	INTEL_8255X_ETHERNET_DEVICE(0x1039, 4),
	INTEL_8255X_ETHERNET_DEVICE(0x103A, 4),
	INTEL_8255X_ETHERNET_DEVICE(0x103B, 4),
	INTEL_8255X_ETHERNET_DEVICE(0x103C, 4),
	INTEL_8255X_ETHERNET_DEVICE(0x103D, 4),
	INTEL_8255X_ETHERNET_DEVICE(0x103E, 4),
	INTEL_8255X_ETHERNET_DEVICE(0x1050, 5),
	INTEL_8255X_ETHERNET_DEVICE(0x1051, 5),
	INTEL_8255X_ETHERNET_DEVICE(0x1052, 5),
	INTEL_8255X_ETHERNET_DEVICE(0x1053, 5),
	INTEL_8255X_ETHERNET_DEVICE(0x1054, 5),
	INTEL_8255X_ETHERNET_DEVICE(0x1055, 5),
	INTEL_8255X_ETHERNET_DEVICE(0x1056, 5),
	INTEL_8255X_ETHERNET_DEVICE(0x1057, 5),
	INTEL_8255X_ETHERNET_DEVICE(0x1059, 0),
	INTEL_8255X_ETHERNET_DEVICE(0x1064, 6),
	INTEL_8255X_ETHERNET_DEVICE(0x1065, 6),
	INTEL_8255X_ETHERNET_DEVICE(0x1066, 6),
	INTEL_8255X_ETHERNET_DEVICE(0x1067, 6),
	INTEL_8255X_ETHERNET_DEVICE(0x1068, 6),
	INTEL_8255X_ETHERNET_DEVICE(0x1069, 6),
	INTEL_8255X_ETHERNET_DEVICE(0x106A, 6),
	INTEL_8255X_ETHERNET_DEVICE(0x106B, 6),
	INTEL_8255X_ETHERNET_DEVICE(0x1091, 7),
	INTEL_8255X_ETHERNET_DEVICE(0x1092, 7),
	INTEL_8255X_ETHERNET_DEVICE(0x1093, 7),
	INTEL_8255X_ETHERNET_DEVICE(0x1094, 7),
	INTEL_8255X_ETHERNET_DEVICE(0x1095, 7),
	INTEL_8255X_ETHERNET_DEVICE(0x1209, 0),
	INTEL_8255X_ETHERNET_DEVICE(0x1229, 0),
	INTEL_8255X_ETHERNET_DEVICE(0x2449, 2),
	INTEL_8255X_ETHERNET_DEVICE(0x2459, 2),
	INTEL_8255X_ETHERNET_DEVICE(0x245D, 2),
	INTEL_8255X_ETHERNET_DEVICE(0x27DC, 7),
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, e100_id_table);

enum mac {
	mac_82557_D100_A  = 0,
	mac_82557_D100_B  = 1,
	mac_82557_D100_C  = 2,
	mac_82558_D101_A4 = 4,
	mac_82558_D101_B0 = 5,
	mac_82559_D101M   = 8,
	mac_82559_D101S   = 9,
	mac_82550_D102    = 12,
	mac_82550_D102_C  = 13,
	mac_82551_E       = 14,
	mac_82551_F       = 15,
	mac_82551_10      = 16,
	mac_unknown       = 0xFF,
};

enum phy {
	phy_100a     = 0x000003E0,
	phy_100c     = 0x035002A8,
	phy_82555_tx = 0x015002A8,
	phy_nsc_tx   = 0x5C002000,
	phy_82562_et = 0x033002A8,
	phy_82562_em = 0x032002A8,
	phy_82562_ek = 0x031002A8,
	phy_82562_eh = 0x017002A8,
	phy_unknown  = 0xFFFFFFFF,
};

/* CSR (Control/Status Registers) */
struct csr {
	struct {
		u8 status;
		u8 stat_ack;
		u8 cmd_lo;
		u8 cmd_hi;
		u32 gen_ptr;
	} scb;
	u32 port;
	u16 flash_ctrl;
	u8 eeprom_ctrl_lo;
	u8 eeprom_ctrl_hi;
	u32 mdi_ctrl;
	u32 rx_dma_count;
};

enum scb_status {
	rus_ready        = 0x10,
	rus_mask         = 0x3C,
};

enum ru_state  {
	RU_SUSPENDED = 0,
	RU_RUNNING	 = 1,
	RU_UNINITIALIZED = -1,
};

enum scb_stat_ack {
	stat_ack_not_ours    = 0x00,
	stat_ack_sw_gen      = 0x04,
	stat_ack_rnr         = 0x10,
	stat_ack_cu_idle     = 0x20,
	stat_ack_frame_rx    = 0x40,
	stat_ack_cu_cmd_done = 0x80,
	stat_ack_not_present = 0xFF,
	stat_ack_rx = (stat_ack_sw_gen | stat_ack_rnr | stat_ack_frame_rx),
	stat_ack_tx = (stat_ack_cu_idle | stat_ack_cu_cmd_done),
};

enum scb_cmd_hi {
	irq_mask_none = 0x00,
	irq_mask_all  = 0x01,
	irq_sw_gen    = 0x02,
};

enum scb_cmd_lo {
	cuc_nop        = 0x00,
	ruc_start      = 0x01,
	ruc_load_base  = 0x06,
	cuc_start      = 0x10,
	cuc_resume     = 0x20,
	cuc_dump_addr  = 0x40,
	cuc_dump_stats = 0x50,
	cuc_load_base  = 0x60,
	cuc_dump_reset = 0x70,
};

enum cuc_dump {
	cuc_dump_complete       = 0x0000A005,
	cuc_dump_reset_complete = 0x0000A007,
};

enum port {
	software_reset  = 0x0000,
	selftest        = 0x0001,
	selective_reset = 0x0002,
};

enum eeprom_ctrl_lo {
	eesk = 0x01,
	eecs = 0x02,
	eedi = 0x04,
	eedo = 0x08,
};

enum mdi_ctrl {
	mdi_write = 0x04000000,
	mdi_read  = 0x08000000,
	mdi_ready = 0x10000000,
};

enum eeprom_op {
	op_write = 0x05,
	op_read  = 0x06,
	op_ewds  = 0x10,
	op_ewen  = 0x13,
};

enum eeprom_offsets {
	eeprom_cnfg_mdix  = 0x03,
	eeprom_id         = 0x0A,
	eeprom_config_asf = 0x0D,
	eeprom_smbus_addr = 0x90,
};

enum eeprom_cnfg_mdix {
	eeprom_mdix_enabled = 0x0080,
};

enum eeprom_id {
	eeprom_id_wol = 0x0020,
};

enum eeprom_config_asf {
	eeprom_asf = 0x8000,
	eeprom_gcl = 0x4000,
};

enum cb_status {
	cb_complete = 0x8000,
	cb_ok       = 0x2000,
};

enum cb_command {
	cb_nop    = 0x0000,
	cb_iaaddr = 0x0001,
	cb_config = 0x0002,
	cb_multi  = 0x0003,
	cb_tx     = 0x0004,
	cb_ucode  = 0x0005,
	cb_dump   = 0x0006,
	cb_tx_sf  = 0x0008,
	cb_cid    = 0x1f00,
	cb_i      = 0x2000,
	cb_s      = 0x4000,
	cb_el     = 0x8000,
};

struct rfd {
	u16 status;
	u16 command;
	u32 link;
	u32 rbd;
	u16 actual_size;
	u16 size;
};

struct rx {
	struct rx *next, *prev;
	struct sk_buff *skb;
	dma_addr_t dma_addr;
};

#if defined(__BIG_ENDIAN_BITFIELD)
#define X(a,b)	b,a
#else
#define X(a,b)	a,b
#endif
struct config {
/*0*/	u8 X(byte_count:6, pad0:2);
/*1*/	u8 X(X(rx_fifo_limit:4, tx_fifo_limit:3), pad1:1);
/*2*/	u8 adaptive_ifs;
/*3*/	u8 X(X(X(X(mwi_enable:1, type_enable:1), read_align_enable:1),
	   term_write_cache_line:1), pad3:4);
/*4*/	u8 X(rx_dma_max_count:7, pad4:1);
/*5*/	u8 X(tx_dma_max_count:7, dma_max_count_enable:1);
/*6*/	u8 X(X(X(X(X(X(X(late_scb_update:1, direct_rx_dma:1),
	   tno_intr:1), cna_intr:1), standard_tcb:1), standard_stat_counter:1),
	   rx_discard_overruns:1), rx_save_bad_frames:1);
/*7*/	u8 X(X(X(X(X(rx_discard_short_frames:1, tx_underrun_retry:2),
	   pad7:2), rx_extended_rfd:1), tx_two_frames_in_fifo:1),
	   tx_dynamic_tbd:1);
/*8*/	u8 X(X(mii_mode:1, pad8:6), csma_disabled:1);
/*9*/	u8 X(X(X(X(X(rx_tcpudp_checksum:1, pad9:3), vlan_arp_tco:1),
	   link_status_wake:1), arp_wake:1), mcmatch_wake:1);
/*10*/	u8 X(X(X(pad10:3, no_source_addr_insertion:1), preamble_length:2),
	   loopback:2);
/*11*/	u8 X(linear_priority:3, pad11:5);
/*12*/	u8 X(X(linear_priority_mode:1, pad12:3), ifs:4);
/*13*/	u8 ip_addr_lo;
/*14*/	u8 ip_addr_hi;
/*15*/	u8 X(X(X(X(X(X(X(promiscuous_mode:1, broadcast_disabled:1),
	   wait_after_win:1), pad15_1:1), ignore_ul_bit:1), crc_16_bit:1),
	   pad15_2:1), crs_or_cdt:1);
/*16*/	u8 fc_delay_lo;
/*17*/	u8 fc_delay_hi;
/*18*/	u8 X(X(X(X(X(rx_stripping:1, tx_padding:1), rx_crc_transfer:1),
	   rx_long_ok:1), fc_priority_threshold:3), pad18:1);
/*19*/	u8 X(X(X(X(X(X(X(addr_wake:1, magic_packet_disable:1),
	   fc_disable:1), fc_restop:1), fc_restart:1), fc_reject:1),
	   full_duplex_force:1), full_duplex_pin:1);
/*20*/	u8 X(X(X(pad20_1:5, fc_priority_location:1), multi_ia:1), pad20_2:1);
/*21*/	u8 X(X(pad21_1:3, multicast_all:1), pad21_2:4);
/*22*/	u8 X(X(rx_d102_mode:1, rx_vlan_drop:1), pad22:6);
	u8 pad_d102[9];
};

#define E100_MAX_MULTICAST_ADDRS	64
struct multi {
	u16 count;
	u8 addr[E100_MAX_MULTICAST_ADDRS * ETH_ALEN + 2/*pad*/];
};

/* Important: keep total struct u32-aligned */
#define UCODE_SIZE			134
struct cb {
	u16 status;
	u16 command;
	u32 link;
	union {
		u8 iaaddr[ETH_ALEN];
		u32 ucode[UCODE_SIZE];
		struct config config;
		struct multi multi;
		struct {
			u32 tbd_array;
			u16 tcb_byte_count;
			u8 threshold;
			u8 tbd_count;
			struct {
				u32 buf_addr;
				u16 size;
				u16 eol;
			} tbd;
		} tcb;
		u32 dump_buffer_addr;
	} u;
	struct cb *next, *prev;
	dma_addr_t dma_addr;
	struct sk_buff *skb;
};

enum loopback {
	lb_none = 0, lb_mac = 1, lb_phy = 3,
};

struct stats {
	u32 tx_good_frames, tx_max_collisions, tx_late_collisions,
		tx_underruns, tx_lost_crs, tx_deferred, tx_single_collisions,
		tx_multiple_collisions, tx_total_collisions;
	u32 rx_good_frames, rx_crc_errors, rx_alignment_errors,
		rx_resource_errors, rx_overrun_errors, rx_cdt_errors,
		rx_short_frame_errors;
	u32 fc_xmt_pause, fc_rcv_pause, fc_rcv_unsupported;
	u16 xmt_tco_frames, rcv_tco_frames;
	u32 complete;
};

struct mem {
	struct {
		u32 signature;
		u32 result;
	} selftest;
	struct stats stats;
	u8 dump_buf[596];
};

struct param_range {
	u32 min;
	u32 max;
	u32 count;
};

struct params {
	struct param_range rfds;
	struct param_range cbs;
};


struct nic {
	/* Begin: frequently used values: keep adjacent for cache effect */
	u32 msg_enable				____cacheline_aligned;
	struct net_device *netdev;
	struct pci_dev *pdev;

	struct rx *rxs				____cacheline_aligned;
	struct rx *rx_to_use;
	struct rx *rx_to_clean;
	struct rfd blank_rfd;
	enum ru_state ru_running;

	spinlock_t cb_lock			____cacheline_aligned;
	spinlock_t cmd_lock;
	struct csr __iomem *csr;
	enum scb_cmd_lo cuc_cmd;
	unsigned int cbs_avail;
	struct cb *cbs;
	struct cb *cb_to_use;
	struct cb *cb_to_send;
	struct cb *cb_to_clean;
	u16 tx_command;
	/* End: frequently used values: keep adjacent for cache effect */

	enum {
		ich                = (1 << 0),
		promiscuous        = (1 << 1),
		multicast_all      = (1 << 2),
		wol_magic          = (1 << 3),
		ich_10h_workaround = (1 << 4),
	} flags					____cacheline_aligned;

	enum mac mac;
	enum phy phy;
	struct params params;
	struct net_device_stats net_stats;
//	struct timer_list watchdog;
//	struct timer_list blink_timer;
//	struct mii_if_info mii;
//	struct work_struct tx_timeout_task;
	enum loopback loopback;

	struct mem *mem;
	dma_addr_t dma_addr;

	dma_addr_t cbs_dma_addr;
	u8 adaptive_ifs;
	u8 tx_threshold;
	u32 tx_frames;
	u32 tx_collisions;
	u32 tx_deferred;
	u32 tx_single_collisions;
	u32 tx_multiple_collisions;
	u32 tx_fc_pause;
	u32 tx_tco_frames;

	u32 rx_fc_pause;
	u32 rx_fc_unsupported;
	u32 rx_tco_frames;
	u32 rx_over_length_errors;

	u8 rev_id;
	u16 leds;
	u16 eeprom_wc;
	u16 eeprom[256];
	spinlock_t mdio_lock;
	struct mii_if_info mii;
	int mc_count;
};

static inline void e100_write_flush(struct nic *nic)
{
	/* Flush previous PCI writes through intermediate bridges
	 * by doing a benign read */
	(void)readb(&nic->csr->scb.status);
}

static void e100_enable_irq(struct nic *nic)
{
	unsigned long flags;

	spin_lock_irqsave(&nic->cmd_lock, flags);
	writeb(irq_mask_none, &nic->csr->scb.cmd_hi);
	e100_write_flush(nic);
	spin_unlock_irqrestore(&nic->cmd_lock, flags);
}

static void e100_disable_irq(struct nic *nic)
{
	unsigned long flags;

	spin_lock_irqsave(&nic->cmd_lock, flags);
	writeb(irq_mask_all, &nic->csr->scb.cmd_hi);
	e100_write_flush(nic);
	spin_unlock_irqrestore(&nic->cmd_lock, flags);
}

static void e100_hw_reset(struct nic *nic)
{
	/* Put CU and RU into idle with a selective reset to get
	 * device off of PCI bus */
	writel(selective_reset, &nic->csr->port);
	e100_write_flush(nic); udelay(20);

	/* Now fully reset device */
	writel(software_reset, &nic->csr->port);
	e100_write_flush(nic); udelay(20);

	/* Mask off our interrupt line - it's unmasked after reset */
	e100_disable_irq(nic);
}

static int e100_self_test(struct nic *nic)
{
	u32 dma_addr = nic->dma_addr + offsetof(struct mem, selftest);

	/* Passing the self-test is a pretty good indication
	 * that the device can DMA to/from host memory */

	nic->mem->selftest.signature = 0;
	nic->mem->selftest.result = 0xFFFFFFFF;

	writel(selftest | dma_addr, &nic->csr->port);
	e100_write_flush(nic);
	/* Wait 10 msec for self-test to complete */
	msleep(10);

	/* Interrupts are enabled after self-test */
	e100_disable_irq(nic);

	/* Check results of self-test */
	if(nic->mem->selftest.result != 0) {
		DPRINTK(HW, ERR, "Self-test failed: result=0x%08X\n",
			nic->mem->selftest.result);
		return -ETIMEDOUT;
	}
	if(nic->mem->selftest.signature == 0) {
		DPRINTK(HW, ERR, "Self-test failed: timed out\n");
		return -ETIMEDOUT;
	}

	return 0;
}

static void e100_eeprom_write(struct nic *nic, u16 addr_len, u16 addr, u16 data)
{
	u32 cmd_addr_data[3];
	u8 ctrl;
	int i, j;

	/* Three cmds: write/erase enable, write data, write/erase disable */
	cmd_addr_data[0] = op_ewen << (addr_len - 2);
	cmd_addr_data[1] = (((op_write << addr_len) | addr) << 16) |
		cpu_to_le16(data);
	cmd_addr_data[2] = op_ewds << (addr_len - 2);

	/* Bit-bang cmds to write word to eeprom */
	for(j = 0; j < 3; j++) {

		/* Chip select */
		writeb(eecs | eesk, &nic->csr->eeprom_ctrl_lo);
		e100_write_flush(nic); udelay(4);

		for(i = 31; i >= 0; i--) {
			ctrl = (cmd_addr_data[j] & (1 << i)) ?
				eecs | eedi : eecs;
			writeb(ctrl, &nic->csr->eeprom_ctrl_lo);
			e100_write_flush(nic); udelay(4);

			writeb(ctrl | eesk, &nic->csr->eeprom_ctrl_lo);
			e100_write_flush(nic); udelay(4);
		}
		/* Wait 10 msec for cmd to complete */
		msleep(10);

		/* Chip deselect */
		writeb(0, &nic->csr->eeprom_ctrl_lo);
		e100_write_flush(nic); udelay(4);
	}
};

/* General technique stolen from the eepro100 driver - very clever */
static u16 e100_eeprom_read(struct nic *nic, u16 *addr_len, u16 addr)
{
	u32 cmd_addr_data;
	u16 data = 0;
	u8 ctrl;
	int i;

	cmd_addr_data = ((op_read << *addr_len) | addr) << 16;

	/* Chip select */
	writeb(eecs | eesk, &nic->csr->eeprom_ctrl_lo);
	e100_write_flush(nic); udelay(4);

	/* Bit-bang to read word from eeprom */
	for(i = 31; i >= 0; i--) {
		ctrl = (cmd_addr_data & (1 << i)) ? eecs | eedi : eecs;
		writeb(ctrl, &nic->csr->eeprom_ctrl_lo);
		e100_write_flush(nic); udelay(4);

		writeb(ctrl | eesk, &nic->csr->eeprom_ctrl_lo);
		e100_write_flush(nic); udelay(4);

		/* Eeprom drives a dummy zero to EEDO after receiving
		 * complete address.  Use this to adjust addr_len. */
		ctrl = readb(&nic->csr->eeprom_ctrl_lo);
		if(!(ctrl & eedo) && i > 16) {
			*addr_len -= (i - 16);
			i = 17;
		}

		data = (data << 1) | (ctrl & eedo ? 1 : 0);
	}

	/* Chip deselect */
	writeb(0, &nic->csr->eeprom_ctrl_lo);
	e100_write_flush(nic); udelay(4);

	return le16_to_cpu(data);
};

/* Load entire EEPROM image into driver cache and validate checksum */
static int e100_eeprom_load(struct nic *nic)
{
	u16 addr, addr_len = 8, checksum = 0;

	/* Try reading with an 8-bit addr len to discover actual addr len */
	e100_eeprom_read(nic, &addr_len, 0);
	nic->eeprom_wc = 1 << addr_len;

	for(addr = 0; addr < nic->eeprom_wc; addr++) {
		nic->eeprom[addr] = e100_eeprom_read(nic, &addr_len, addr);
		if(addr < nic->eeprom_wc - 1)
			checksum += cpu_to_le16(nic->eeprom[addr]);
	}

	/* The checksum, stored in the last word, is calculated such that
	 * the sum of words should be 0xBABA */
	checksum = le16_to_cpu(0xBABA - checksum);
	if(checksum != nic->eeprom[nic->eeprom_wc - 1]) {
		DPRINTK1(PROBE, ERR, "EEPROM corrupted\n");
		if (!eeprom_bad_csum_allow)
			return -EAGAIN;
	}

	return 0;
}

/* Save (portion of) driver EEPROM cache to device and update checksum */
static int e100_eeprom_save(struct nic *nic, u16 start, u16 count)
{
	u16 addr, addr_len = 8, checksum = 0;

	/* Try reading with an 8-bit addr len to discover actual addr len */
	e100_eeprom_read(nic, &addr_len, 0);
	nic->eeprom_wc = 1 << addr_len;

	if(start + count >= nic->eeprom_wc)
		return -EINVAL;

	for(addr = start; addr < start + count; addr++)
		e100_eeprom_write(nic, addr_len, addr, nic->eeprom[addr]);

	/* The checksum, stored in the last word, is calculated such that
	 * the sum of words should be 0xBABA */
	for(addr = 0; addr < nic->eeprom_wc - 1; addr++)
		checksum += cpu_to_le16(nic->eeprom[addr]);
	nic->eeprom[nic->eeprom_wc - 1] = le16_to_cpu(0xBABA - checksum);
	printf("checksum %x\n",(unsigned)le16_to_cpu(0xBABA - checksum));
	e100_eeprom_write(nic, addr_len, nic->eeprom_wc - 1,
		nic->eeprom[nic->eeprom_wc - 1]);

	return 0;
}

#define E100_WAIT_SCB_TIMEOUT 20000 /* we might have to wait 100ms!!! */
#define E100_WAIT_SCB_FAST 20       /* delay like the old code */
static int e100_exec_cmd(struct nic *nic, u8 cmd, dma_addr_t dma_addr)
{
	unsigned long flags;
	unsigned int i;
	int err = 0;

	spin_lock_irqsave(&nic->cmd_lock, flags);

	/* Previous command is accepted when SCB clears */
	for(i = 0; i < E100_WAIT_SCB_TIMEOUT; i++) {
		if(likely(!readb(&nic->csr->scb.cmd_lo)))
			break;
		//cpu_relax();
		if(unlikely(i > E100_WAIT_SCB_FAST))
			udelay(5);
	}
	if(unlikely(i == E100_WAIT_SCB_TIMEOUT)) {
		err = -EAGAIN;
		goto err_unlock;
	}

	if(unlikely(cmd != cuc_resume))
		writel(dma_addr, &nic->csr->scb.gen_ptr);
	writeb(cmd, &nic->csr->scb.cmd_lo);

err_unlock:
	spin_unlock_irqrestore(&nic->cmd_lock, flags);

	return err;
}

static void e100_tx_timeout_task(struct net_device *netdev);
static int e100_exec_cb(struct nic *nic, struct sk_buff *skb,
	void (*cb_prepare)(struct nic *, struct cb *, struct sk_buff *))
{
	struct cb *cb;
	unsigned long flags;
	int err = 0;

	spin_lock_irqsave(&nic->cb_lock, flags);

	if(unlikely(!nic->cbs_avail)) {
		err = -ENOMEM;
		goto err_unlock;
	}

	cb = nic->cb_to_use;
	nic->cb_to_use = cb->next;
	nic->cbs_avail--;
	cb->skb = skb;

	if(unlikely(!nic->cbs_avail))
		err = -ENOSPC;

	cb_prepare(nic, cb, skb);

	/* Order is important otherwise we'll be in a race with h/w:
	 * set S-bit in current first, then clear S-bit in previous. */
	cb->command |= cpu_to_le16(cb_s);
	wmb();
	cb->prev->command &= cpu_to_le16(~cb_s);

	while(nic->cb_to_send != nic->cb_to_use) {
		if(unlikely(e100_exec_cmd(nic, nic->cuc_cmd,
			nic->cb_to_send->dma_addr))) {
			/* Ok, here's where things get sticky.  It's
			 * possible that we can't schedule the command
			 * because the controller is too busy, so
			 * let's just queue the command and try again
			 * when another command is scheduled. */
			if(err == -ENOSPC) {
				//request a reset
				//schedule_work(&nic->tx_timeout_task);
			  struct net_device *netdev = nic->netdev;
				e100_tx_timeout_task(netdev);
			}
			break;
		} else {
			nic->cuc_cmd = cuc_resume;
			nic->cb_to_send = nic->cb_to_send->next;
		}
	}

err_unlock:
	spin_unlock_irqrestore(&nic->cb_lock, flags);

	return err;
}

static u16 mdio_ctrl(struct nic *nic, u32 addr, u32 dir, u32 reg, u16 data)
{
	u32 data_out = 0;
	unsigned int i;
	unsigned long flags;


	/*
	 * Stratus87247: we shouldn't be writing the MDI control
	 * register until the Ready bit shows True.  Also, since
	 * manipulation of the MDI control registers is a multi-step
	 * procedure it should be done under lock.
	 */
	spin_lock_irqsave(&nic->mdio_lock, flags);
	for (i = 100; i; --i) {
		if (readl(&nic->csr->mdi_ctrl) & mdi_ready)
			break;
		udelay(20);
	}
	if (unlikely(!i)) {
		printk("e100.mdio_ctrl(%s) won't go Ready\n",
			nic->netdev->name );
		spin_unlock_irqrestore(&nic->mdio_lock, flags);
		return 0;		/* No way to indicate timeout error */
	}
	writel((reg << 16) | (addr << 21) | dir | data, &nic->csr->mdi_ctrl);

	for (i = 0; i < 100; i++) {
		udelay(20);
		if ((data_out = readl(&nic->csr->mdi_ctrl)) & mdi_ready)
			break;
	}
	spin_unlock_irqrestore(&nic->mdio_lock, flags);
	DPRINTK(HW, DEBUG,
		"%s:addr=%d, reg=%d, data_in=0x%04X, data_out=0x%04X\n",
		dir == mdi_read ? "READ" : "WRITE", addr, reg, data, data_out);
	return (u16)data_out;
}

static int mdio_read(struct net_device *netdev, int addr, int reg)
{
	return mdio_ctrl(netdev_priv(netdev), addr, mdi_read, reg, 0);
}

static void mdio_write(struct net_device *netdev, int addr, int reg, int data)
{
	mdio_ctrl(netdev_priv(netdev), addr, mdi_write, reg, data);
}

static void e100_get_defaults(struct nic *nic)
{
	struct param_range rfds = { .min = 16, .max = 256, .count = 256 };
	struct param_range cbs  = { .min = 64, .max = 256, .count = 128 };

	pci_read_config_byte(nic->pdev, PCI_REVISION_ID, &nic->rev_id);
	/* MAC type is encoded as rev ID; exception: ICH is treated as 82559 */
	nic->mac = (nic->flags & ich) ? mac_82559_D101M : nic->rev_id;
	if(nic->mac == mac_unknown)
		nic->mac = mac_82557_D100_A;

	nic->params.rfds = rfds;
	nic->params.cbs = cbs;

	/* Quadwords to DMA into FIFO before starting frame transmit */
	nic->tx_threshold = 0xE0;

	/* no interrupt for every tx completion, delay = 256us if not 557*/
	nic->tx_command = cpu_to_le16(cb_tx | cb_tx_sf |
		((nic->mac >= mac_82558_D101_A4) ? cb_cid : cb_i));

	/* Template for a freshly allocated RFD */
	nic->blank_rfd.command = cpu_to_le16(cb_el);
	nic->blank_rfd.rbd = 0xFFFFFFFF;
	nic->blank_rfd.size = cpu_to_le16(VLAN_ETH_FRAME_LEN);

	/* MII setup */
	nic->mii.phy_id_mask = 0x1F;
	nic->mii.reg_num_mask = 0x1F;
	nic->mii.dev = nic->netdev;
	nic->mii.mdio_read = mdio_read;
	nic->mii.mdio_write = mdio_write;
}

static void e100_configure(struct nic *nic, struct cb *cb, struct sk_buff *skb)
{
	struct config *config = &cb->u.config;
	u8 *c = (u8 *)config;

	cb->command = cpu_to_le16(cb_config);

	memset(config, 0, sizeof(struct config));

	config->byte_count = 0x16;		/* bytes in this struct */
	config->rx_fifo_limit = 0x8;		/* bytes in FIFO before DMA */
	config->direct_rx_dma = 0x1;		/* reserved */
	config->standard_tcb = 0x1;		/* 1=standard, 0=extended */
	config->standard_stat_counter = 0x1;	/* 1=standard, 0=extended */
	config->rx_discard_short_frames = 0x1;	/* 1=discard, 0=pass */
	config->tx_underrun_retry = 0x3;	/* # of underrun retries */
	config->mii_mode = 0x1;			/* 1=MII mode, 0=503 mode */
	config->pad10 = 0x6;
	config->no_source_addr_insertion = 0x1;	/* 1=no, 0=yes */
	config->preamble_length = 0x2;		/* 0=1, 1=3, 2=7, 3=15 bytes */
	config->ifs = 0x6;			/* x16 = inter frame spacing */
	config->ip_addr_hi = 0xF2;		/* ARP IP filter - not used */
	config->pad15_1 = 0x1;
	config->pad15_2 = 0x1;
	config->crs_or_cdt = 0x0;		/* 0=CRS only, 1=CRS or CDT */
	config->fc_delay_hi = 0x40;		/* time delay for fc frame */
	config->tx_padding = 0x1;		/* 1=pad short frames */
	config->fc_priority_threshold = 0x7;	/* 7=priority fc disabled */
	config->pad18 = 0x1;
	config->full_duplex_pin = 0x1;		/* 1=examine FDX# pin */
	config->pad20_1 = 0x1F;
	config->fc_priority_location = 0x1;	/* 1=byte#31, 0=byte#19 */
	config->pad21_1 = 0x5;

	config->adaptive_ifs = nic->adaptive_ifs;
	config->loopback = nic->loopback;

	if(nic->mii.force_media && nic->mii.full_duplex)
		config->full_duplex_force = 0x1;	/* 1=force, 0=auto */

	if(nic->flags & promiscuous || nic->loopback) {
		config->rx_save_bad_frames = 0x1;	/* 1=save, 0=discard */
		config->rx_discard_short_frames = 0x0;	/* 1=discard, 0=save */
		config->promiscuous_mode = 0x1;		/* 1=on, 0=off */
	}

	if(nic->flags & multicast_all)
		config->multicast_all = 0x1;		/* 1=accept, 0=no */

	/* disable WoL when up */
	if(netif_running(nic->netdev) || !(nic->flags & wol_magic))
		config->magic_packet_disable = 0x1;	/* 1=off, 0=on */

	if(nic->mac >= mac_82558_D101_A4) {
		config->fc_disable = 0x1;	/* 1=Tx fc off, 0=Tx fc on */
		config->mwi_enable = 0x1;	/* 1=enable, 0=disable */
		config->standard_tcb = 0x0;	/* 1=standard, 0=extended */
		config->rx_long_ok = 0x1;	/* 1=VLANs ok, 0=standard */
		if(nic->mac >= mac_82559_D101M)
			config->tno_intr = 0x1;		/* TCO stats enable */
		else
			config->standard_stat_counter = 0x0;
	}

	DPRINTK(HW, DEBUG, "[00-07]=%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
		c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7]);
	DPRINTK(HW, DEBUG, "[08-15]=%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
		c[8], c[9], c[10], c[11], c[12], c[13], c[14], c[15]);
	DPRINTK(HW, DEBUG, "[16-23]=%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
		c[16], c[17], c[18], c[19], c[20], c[21], c[22], c[23]);
}

/********************************************************/
/*  Micro code for 8086:1229 Rev 8                      */
/********************************************************/

/*  Parameter values for the D101M B-step  */
#define D101M_CPUSAVER_TIMER_DWORD		78
#define D101M_CPUSAVER_BUNDLE_DWORD		65
#define D101M_CPUSAVER_MIN_SIZE_DWORD		126

#define D101M_B_RCVBUNDLE_UCODE \
{\
0x00550215, 0xFFFF0437, 0xFFFFFFFF, 0x06A70789, 0xFFFFFFFF, 0x0558FFFF, \
0x000C0001, 0x00101312, 0x000C0008, 0x00380216, \
0x0010009C, 0x00204056, 0x002380CC, 0x00380056, \
0x0010009C, 0x00244C0B, 0x00000800, 0x00124818, \
0x00380438, 0x00000000, 0x00140000, 0x00380555, \
0x00308000, 0x00100662, 0x00100561, 0x000E0408, \
0x00134861, 0x000C0002, 0x00103093, 0x00308000, \
0x00100624, 0x00100561, 0x000E0408, 0x00100861, \
0x000C007E, 0x00222C21, 0x000C0002, 0x00103093, \
0x00380C7A, 0x00080000, 0x00103090, 0x00380C7A, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x0010009C, 0x00244C2D, 0x00010004, 0x00041000, \
0x003A0437, 0x00044010, 0x0038078A, 0x00000000, \
0x00100099, 0x00206C7A, 0x0010009C, 0x00244C48, \
0x00130824, 0x000C0001, 0x00101213, 0x00260C75, \
0x00041000, 0x00010004, 0x00130826, 0x000C0006, \
0x002206A8, 0x0013C926, 0x00101313, 0x003806A8, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00080600, 0x00101B10, 0x00050004, 0x00100826, \
0x00101210, 0x00380C34, 0x00000000, 0x00000000, \
0x0021155B, 0x00100099, 0x00206559, 0x0010009C, \
0x00244559, 0x00130836, 0x000C0000, 0x00220C62, \
0x000C0001, 0x00101B13, 0x00229C0E, 0x00210C0E, \
0x00226C0E, 0x00216C0E, 0x0022FC0E, 0x00215C0E, \
0x00214C0E, 0x00380555, 0x00010004, 0x00041000, \
0x00278C67, 0x00040800, 0x00018100, 0x003A0437, \
0x00130826, 0x000C0001, 0x00220559, 0x00101313, \
0x00380559, 0x00000000, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00000000, 0x00130831, 0x0010090B, 0x00124813, \
0x000CFF80, 0x002606AB, 0x00041000, 0x00010004, \
0x003806A8, 0x00000000, 0x00000000, 0x00000000, \
}

/********************************************************/
/*  Micro code for 8086:1229 Rev 9                      */
/********************************************************/

/*  Parameter values for the D101S  */
#define D101S_CPUSAVER_TIMER_DWORD		78
#define D101S_CPUSAVER_BUNDLE_DWORD		67
#define D101S_CPUSAVER_MIN_SIZE_DWORD		128

#define D101S_RCVBUNDLE_UCODE \
{\
0x00550242, 0xFFFF047E, 0xFFFFFFFF, 0x06FF0818, 0xFFFFFFFF, 0x05A6FFFF, \
0x000C0001, 0x00101312, 0x000C0008, 0x00380243, \
0x0010009C, 0x00204056, 0x002380D0, 0x00380056, \
0x0010009C, 0x00244F8B, 0x00000800, 0x00124818, \
0x0038047F, 0x00000000, 0x00140000, 0x003805A3, \
0x00308000, 0x00100610, 0x00100561, 0x000E0408, \
0x00134861, 0x000C0002, 0x00103093, 0x00308000, \
0x00100624, 0x00100561, 0x000E0408, 0x00100861, \
0x000C007E, 0x00222FA1, 0x000C0002, 0x00103093, \
0x00380F90, 0x00080000, 0x00103090, 0x00380F90, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x0010009C, 0x00244FAD, 0x00010004, 0x00041000, \
0x003A047E, 0x00044010, 0x00380819, 0x00000000, \
0x00100099, 0x00206FFD, 0x0010009A, 0x0020AFFD, \
0x0010009C, 0x00244FC8, 0x00130824, 0x000C0001, \
0x00101213, 0x00260FF7, 0x00041000, 0x00010004, \
0x00130826, 0x000C0006, 0x00220700, 0x0013C926, \
0x00101313, 0x00380700, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00080600, 0x00101B10, 0x00050004, 0x00100826, \
0x00101210, 0x00380FB6, 0x00000000, 0x00000000, \
0x002115A9, 0x00100099, 0x002065A7, 0x0010009A, \
0x0020A5A7, 0x0010009C, 0x002445A7, 0x00130836, \
0x000C0000, 0x00220FE4, 0x000C0001, 0x00101B13, \
0x00229F8E, 0x00210F8E, 0x00226F8E, 0x00216F8E, \
0x0022FF8E, 0x00215F8E, 0x00214F8E, 0x003805A3, \
0x00010004, 0x00041000, 0x00278FE9, 0x00040800, \
0x00018100, 0x003A047E, 0x00130826, 0x000C0001, \
0x002205A7, 0x00101313, 0x003805A7, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00130831, \
0x0010090B, 0x00124813, 0x000CFF80, 0x00260703, \
0x00041000, 0x00010004, 0x00380700  \
}

/********************************************************/
/*  Micro code for the 8086:1229 Rev F/10               */
/********************************************************/

/*  Parameter values for the D102 E-step  */
#define D102_E_CPUSAVER_TIMER_DWORD		42
#define D102_E_CPUSAVER_BUNDLE_DWORD		54
#define D102_E_CPUSAVER_MIN_SIZE_DWORD		46

#define     D102_E_RCVBUNDLE_UCODE \
{\
0x007D028F, 0x0E4204F9, 0x14ED0C85, 0x14FA14E9, 0x0EF70E36, 0x1FFF1FFF, \
0x00E014B9, 0x00000000, 0x00000000, 0x00000000, \
0x00E014BD, 0x00000000, 0x00000000, 0x00000000, \
0x00E014D5, 0x00000000, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00E014C1, 0x00000000, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00E014C8, 0x00000000, 0x00000000, 0x00000000, \
0x00200600, 0x00E014EE, 0x00000000, 0x00000000, \
0x0030FF80, 0x00940E46, 0x00038200, 0x00102000, \
0x00E00E43, 0x00000000, 0x00000000, 0x00000000, \
0x00300006, 0x00E014FB, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00906E41, 0x00800E3C, 0x00E00E39, 0x00000000, \
0x00906EFD, 0x00900EFD, 0x00E00EF8, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
0x00000000, 0x00000000, 0x00000000, 0x00000000, \
}

static void e100_setup_ucode(struct nic *nic, struct cb *cb, struct sk_buff *skb)
{
/* *INDENT-OFF* */
	static struct {
		u32 ucode[UCODE_SIZE + 1];
		u8 mac;
		u8 timer_dword;
		u8 bundle_dword;
		u8 min_size_dword;
	} ucode_opts[] = {
		{ D101M_B_RCVBUNDLE_UCODE,
		  mac_82559_D101M,
		  D101M_CPUSAVER_TIMER_DWORD,
		  D101M_CPUSAVER_BUNDLE_DWORD,
		  D101M_CPUSAVER_MIN_SIZE_DWORD },
		{ D101S_RCVBUNDLE_UCODE,
		  mac_82559_D101S,
		  D101S_CPUSAVER_TIMER_DWORD,
		  D101S_CPUSAVER_BUNDLE_DWORD,
		  D101S_CPUSAVER_MIN_SIZE_DWORD },
		{ D102_E_RCVBUNDLE_UCODE,
		  mac_82551_F,
		  D102_E_CPUSAVER_TIMER_DWORD,
		  D102_E_CPUSAVER_BUNDLE_DWORD,
		  D102_E_CPUSAVER_MIN_SIZE_DWORD },
		{ D102_E_RCVBUNDLE_UCODE,
		  mac_82551_10,
		  D102_E_CPUSAVER_TIMER_DWORD,
		  D102_E_CPUSAVER_BUNDLE_DWORD,
		  D102_E_CPUSAVER_MIN_SIZE_DWORD },
		{ {0}, 0, 0, 0, 0}
	}, *opts;
/* *INDENT-ON* */

/*************************************************************************
*  CPUSaver parameters
*
*  All CPUSaver parameters are 16-bit literals that are part of a
*  "move immediate value" instruction.  By changing the value of
*  the literal in the instruction before the code is loaded, the
*  driver can change the algorithm.
*
*  INTDELAY - This loads the dead-man timer with its inital value.
*    When this timer expires the interrupt is asserted, and the
*    timer is reset each time a new packet is received.  (see
*    BUNDLEMAX below to set the limit on number of chained packets)
*    The current default is 0x600 or 1536.  Experiments show that
*    the value should probably stay within the 0x200 - 0x1000.
*
*  BUNDLEMAX -
*    This sets the maximum number of frames that will be bundled.  In
*    some situations, such as the TCP windowing algorithm, it may be
*    better to limit the growth of the bundle size than let it go as
*    high as it can, because that could cause too much added latency.
*    The default is six, because this is the number of packets in the
*    default TCP window size.  A value of 1 would make CPUSaver indicate
*    an interrupt for every frame received.  If you do not want to put
*    a limit on the bundle size, set this value to xFFFF.
*
*  BUNDLESMALL -
*    This contains a bit-mask describing the minimum size frame that
*    will be bundled.  The default masks the lower 7 bits, which means
*    that any frame less than 128 bytes in length will not be bundled,
*    but will instead immediately generate an interrupt.  This does
*    not affect the current bundle in any way.  Any frame that is 128
*    bytes or large will be bundled normally.  This feature is meant
*    to provide immediate indication of ACK frames in a TCP environment.
*    Customers were seeing poor performance when a machine with CPUSaver
*    enabled was sending but not receiving.  The delay introduced when
*    the ACKs were received was enough to reduce total throughput, because
*    the sender would sit idle until the ACK was finally seen.
*
*    The current default is 0xFF80, which masks out the lower 7 bits.
*    This means that any frame which is x7F (127) bytes or smaller
*    will cause an immediate interrupt.  Because this value must be a
*    bit mask, there are only a few valid values that can be used.  To
*    turn this feature off, the driver can write the value xFFFF to the
*    lower word of this instruction (in the same way that the other
*    parameters are used).  Likewise, a value of 0xF800 (2047) would
*    cause an interrupt to be generated for every frame, because all
*    standard Ethernet frames are <= 2047 bytes in length.
*************************************************************************/

/* if you wish to disable the ucode functionality, while maintaining the
 * workarounds it provides, set the following defines to:
 * BUNDLESMALL 0
 * BUNDLEMAX 1
 * INTDELAY 1
 */
#define BUNDLESMALL 1
#define BUNDLEMAX (u16)6
#define INTDELAY (u16)1536 /* 0x600 */

	/* do not load u-code for ICH devices */
	if (nic->flags & ich)
		goto noloaducode;

	/* Search for ucode match against h/w rev_id */
	for (opts = ucode_opts; opts->mac; opts++) {
		int i;
		u32 *ucode = opts->ucode;
		if (nic->mac != opts->mac)
			continue;

		/* Insert user-tunable settings */
		ucode[opts->timer_dword] &= 0xFFFF0000;
		ucode[opts->timer_dword] |= INTDELAY;
		ucode[opts->bundle_dword] &= 0xFFFF0000;
		ucode[opts->bundle_dword] |= BUNDLEMAX;
		ucode[opts->min_size_dword] &= 0xFFFF0000;
		ucode[opts->min_size_dword] |= (BUNDLESMALL) ? 0xFFFF : 0xFF80;

		for (i = 0; i < UCODE_SIZE; i++)
			cb->u.ucode[i] = cpu_to_le32(ucode[i]);
		cb->command = cpu_to_le16(cb_ucode | cb_el);
		return;
	}

noloaducode:
	cb->command = cpu_to_le16(cb_nop | cb_el);
}

static inline int e100_exec_cb_wait(struct nic *nic, struct sk_buff *skb,
	void (*cb_prepare)(struct nic *, struct cb *, struct sk_buff *))
{
	int err = 0, counter = 50;
	struct cb *cb = nic->cb_to_clean;

	if ((err = e100_exec_cb(nic, NULL, e100_setup_ucode)))
		DPRINTK(PROBE,ERR, "ucode cmd failed with error %d\n", err);

	/* must restart cuc */
	nic->cuc_cmd = cuc_start;

	/* wait for completion */
	e100_write_flush(nic);
	udelay(10);

	/* wait for possibly (ouch) 500ms */
	while (!(cb->status & cpu_to_le16(cb_complete))) {
		msleep(10);
		if (!--counter) break;
	}

	/* ack any interupts, something could have been set */
	writeb(~0, &nic->csr->scb.stat_ack);

	/* if the command failed, or is not OK, notify and return */
	if (!counter || !(cb->status & cpu_to_le16(cb_ok))) {
		DPRINTK(PROBE,ERR, "ucode load failed\n");
		err = -EPERM;
	}

	return err;
}

static void e100_setup_iaaddr(struct nic *nic, struct cb *cb,
	struct sk_buff *skb)
{
	cb->command = cpu_to_le16(cb_iaaddr);
	memcpy(cb->u.iaaddr, nic->netdev->dev_addr, ETH_ALEN);
}

static void e100_dump(struct nic *nic, struct cb *cb, struct sk_buff *skb)
{
	cb->command = cpu_to_le16(cb_dump);
	cb->u.dump_buffer_addr = cpu_to_le32(nic->dma_addr +
		offsetof(struct mem, dump_buf));
}

#define NCONFIG_AUTO_SWITCH	0x0080
#define MII_NSC_CONG		MII_RESV1
#define NSC_CONG_ENABLE		0x0100
#define NSC_CONG_TXREADY	0x0400
#define ADVERTISE_FC_SUPPORTED	0x0400
static int e100_phy_init(struct nic *nic)
{
	struct net_device *netdev = nic->netdev;
	u32 addr;
	u16 bmcr, stat, id_lo, id_hi, cong;

	/* Discover phy addr by searching addrs in order {1,0,2,..., 31} */
	for(addr = 0; addr < 32; addr++) {
		nic->mii.phy_id = (addr == 0) ? 1 : (addr == 1) ? 0 : addr;
		bmcr = mdio_read(netdev, nic->mii.phy_id, MII_BMCR);
		stat = mdio_read(netdev, nic->mii.phy_id, MII_BMSR);
		stat = mdio_read(netdev, nic->mii.phy_id, MII_BMSR);
		if(!((bmcr == 0xFFFF) || ((stat == 0) && (bmcr == 0))))
			break;
	}
	DPRINTK(HW, DEBUG, "phy_addr = %d\n", nic->mii.phy_id);
	if(addr == 32)
		return -EAGAIN;

	/* Selected the phy and isolate the rest */
	for(addr = 0; addr < 32; addr++) {
		if(addr != nic->mii.phy_id) {
			mdio_write(netdev, addr, MII_BMCR, BMCR_ISOLATE);
		} else {
			bmcr = mdio_read(netdev, addr, MII_BMCR);
			mdio_write(netdev, addr, MII_BMCR,
				bmcr & ~BMCR_ISOLATE);
		}
	}

	/* Get phy ID */
	id_lo = mdio_read(netdev, nic->mii.phy_id, MII_PHYSID1);
	id_hi = mdio_read(netdev, nic->mii.phy_id, MII_PHYSID2);
	nic->phy = (u32)id_hi << 16 | (u32)id_lo;
	DPRINTK(HW, DEBUG, "phy ID = 0x%08X\n", nic->phy);

	/* Handle National tx phys */
#define NCS_PHY_MODEL_MASK	0xFFF0FFFF
	if((nic->phy & NCS_PHY_MODEL_MASK) == phy_nsc_tx) {
		/* Disable congestion control */
		cong = mdio_read(netdev, nic->mii.phy_id, MII_NSC_CONG);
		cong |= NSC_CONG_TXREADY;
		cong &= ~NSC_CONG_ENABLE;
		mdio_write(netdev, nic->mii.phy_id, MII_NSC_CONG, cong);
	}

	if((nic->mac >= mac_82550_D102) || ((nic->flags & ich) &&
	   (mdio_read(netdev, nic->mii.phy_id, MII_TPISTATUS) & 0x8000))) {
		/* enable/disable MDI/MDI-X auto-switching.
		   MDI/MDI-X auto-switching is disabled for 82551ER/QM chips */
		if((nic->mac == mac_82551_E) || (nic->mac == mac_82551_F) ||
		   (nic->mac == mac_82551_10) || (nic->mii.force_media) ||
		   !(nic->eeprom[eeprom_cnfg_mdix] & eeprom_mdix_enabled))
			mdio_write(netdev, nic->mii.phy_id, MII_NCONFIG, 0);
		else
			mdio_write(netdev, nic->mii.phy_id, MII_NCONFIG, NCONFIG_AUTO_SWITCH);
	}

	return 0;
}

static int e100_hw_init(struct nic *nic)
{
	int err;

	e100_hw_reset(nic);

	DPRINTK(HW, ERR, "e100_hw_init\n");
	if(!in_interrupt() && (err = e100_self_test(nic)))
		return err;

	if((err = e100_phy_init(nic)))
		return err;
	if((err = e100_exec_cmd(nic, cuc_load_base, 0)))
		return err;
	if((err = e100_exec_cmd(nic, ruc_load_base, 0)))
		return err;
	if ((err = e100_exec_cb_wait(nic, NULL, e100_setup_ucode)))
		return err;
	if((err = e100_exec_cb(nic, NULL, e100_configure)))
		return err;
	if((err = e100_exec_cb(nic, NULL, e100_setup_iaaddr)))
		return err;
	if((err = e100_exec_cmd(nic, cuc_dump_addr,
		nic->dma_addr + offsetof(struct mem, stats))))
		return err;
	if((err = e100_exec_cmd(nic, cuc_dump_reset, 0)))
		return err;

	e100_disable_irq(nic);

	return 0;
}
#if 0
static void e100_multi(struct nic *nic, struct cb *cb, struct sk_buff *skb)
{
	struct net_device *netdev = nic->netdev;
	struct dev_mc_list *list = netdev->mc_list;
	u16 i, count = min(netdev->mc_count, E100_MAX_MULTICAST_ADDRS);

	cb->command = cpu_to_le16(cb_multi);
	cb->u.multi.count = cpu_to_le16(count * ETH_ALEN);
	for(i = 0; list && i < count; i++, list = list->next)
		memcpy(&cb->u.multi.addr[i*ETH_ALEN], &list->dmi_addr,
			ETH_ALEN);
}

#endif
static void e100_set_multicast_list(struct net_device *netdev)
{
	struct nic *nic = netdev_priv(netdev);

	DPRINTK(HW, DEBUG, "mc_count=%d, flags=0x%04X\n",
		netdev->mc_count, netdev->flags);

		nic->flags |= promiscuous;

	if(netdev->flags & IFF_ALLMULTI ||
		netdev->mc_count > E100_MAX_MULTICAST_ADDRS)
		nic->flags |= multicast_all;
	else
		nic->flags &= ~multicast_all;

	e100_exec_cb(nic, NULL, e100_configure);
//	e100_exec_cb(nic, NULL, e100_multi);
}
static void e100_update_stats(struct nic *nic)
{
	struct net_device_stats *ns = &nic->net_stats;
	struct stats *s = &nic->mem->stats;
	u32 *complete = (nic->mac < mac_82558_D101_A4) ? &s->fc_xmt_pause :
		(nic->mac < mac_82559_D101M) ? (u32 *)&s->xmt_tco_frames :
		&s->complete;

	/* Device's stats reporting may take several microseconds to
	 * complete, so where always waiting for results of the
	 * previous command. */

	if(*complete == le32_to_cpu(cuc_dump_reset_complete)) {
		*complete = 0;
		nic->tx_frames = le32_to_cpu(s->tx_good_frames);
		nic->tx_collisions = le32_to_cpu(s->tx_total_collisions);
		ns->tx_aborted_errors += le32_to_cpu(s->tx_max_collisions);
		ns->tx_window_errors += le32_to_cpu(s->tx_late_collisions);
		ns->tx_carrier_errors += le32_to_cpu(s->tx_lost_crs);
		ns->tx_fifo_errors += le32_to_cpu(s->tx_underruns);
		ns->collisions += nic->tx_collisions;
		ns->tx_errors += le32_to_cpu(s->tx_max_collisions) +
			le32_to_cpu(s->tx_lost_crs);
		ns->rx_length_errors += le32_to_cpu(s->rx_short_frame_errors) +
			nic->rx_over_length_errors;
		ns->rx_crc_errors += le32_to_cpu(s->rx_crc_errors);
		ns->rx_frame_errors += le32_to_cpu(s->rx_alignment_errors);
		ns->rx_over_errors += le32_to_cpu(s->rx_overrun_errors);
		ns->rx_fifo_errors += le32_to_cpu(s->rx_overrun_errors);
		ns->rx_missed_errors += le32_to_cpu(s->rx_resource_errors);
		ns->rx_errors += le32_to_cpu(s->rx_crc_errors) +
			le32_to_cpu(s->rx_alignment_errors) +
			le32_to_cpu(s->rx_short_frame_errors) +
			le32_to_cpu(s->rx_cdt_errors);
		nic->tx_deferred += le32_to_cpu(s->tx_deferred);
		nic->tx_single_collisions +=
			le32_to_cpu(s->tx_single_collisions);
		nic->tx_multiple_collisions +=
			le32_to_cpu(s->tx_multiple_collisions);
		if(nic->mac >= mac_82558_D101_A4) {
			nic->tx_fc_pause += le32_to_cpu(s->fc_xmt_pause);
			nic->rx_fc_pause += le32_to_cpu(s->fc_rcv_pause);
			nic->rx_fc_unsupported +=
				le32_to_cpu(s->fc_rcv_unsupported);
			if(nic->mac >= mac_82559_D101M) {
				nic->tx_tco_frames +=
					le16_to_cpu(s->xmt_tco_frames);
				nic->rx_tco_frames +=
					le16_to_cpu(s->rcv_tco_frames);
			}
		}
	}


	if(e100_exec_cmd(nic, cuc_dump_reset, 0))
		DPRINTK(TX_ERR, DEBUG, "exec cuc_dump_reset failed\n");
}

static void e100_adjust_adaptive_ifs(struct nic *nic, int speed, int duplex)
{
	/* Adjust inter-frame-spacing (IFS) between two transmits if
	 * we're getting collisions on a half-duplex connection. */

	if(duplex == DUPLEX_HALF) {
		u32 prev = nic->adaptive_ifs;
		u32 min_frames = (speed == SPEED_100) ? 1000 : 100;

		if((nic->tx_frames / 32 < nic->tx_collisions) &&
		   (nic->tx_frames > min_frames)) {
			if(nic->adaptive_ifs < 60)
				nic->adaptive_ifs += 5;
		} else if (nic->tx_frames < min_frames) {
			if(nic->adaptive_ifs >= 5)
				nic->adaptive_ifs -= 5;
		}
		if(nic->adaptive_ifs != prev)
			e100_exec_cb(nic, NULL, e100_configure);
	}
}
#if 1
static void e100_watchdog(unsigned long data)
{
	struct nic *nic = (struct nic *)data;
	struct ethtool_cmd cmd;

	DPRINTK(TIMER, DEBUG, "right now = %ld\n", jiffies);

	/* mii library handles link maintenance tasks */

	mii_ethtool_gset(&nic->mii, &cmd);

	if(mii_link_ok(&nic->mii) && !netif_carrier_ok(nic->netdev)) {
		printf("link up, %sMbps, %s-duplex\n",
			cmd.speed == SPEED_100 ? "100" : "10",
			cmd.duplex == DUPLEX_FULL ? "full" : "half");
	} else if(!mii_link_ok(&nic->mii) && netif_carrier_ok(nic->netdev)) {
		printf("link down\n");
	}

	mii_check_link(&nic->mii);

	/* Software generated interrupt to recover from (rare) Rx
	 * allocation failure.
	 * Unfortunately have to use a spinlock to not re-enable interrupts
	 * accidentally, due to hardware that shares a register between the
	 * interrupt mask bit and the SW Interrupt generation bit */
	spin_lock_irq(&nic->cmd_lock);
	writeb(readb(&nic->csr->scb.cmd_hi) | irq_sw_gen,&nic->csr->scb.cmd_hi);
	e100_write_flush(nic);
	spin_unlock_irq(&nic->cmd_lock);

	e100_update_stats(nic);
	e100_adjust_adaptive_ifs(nic, cmd.speed, cmd.duplex);

	if(nic->mac <= mac_82557_D100_C)
		/* Issue a multicast command to workaround a 557 lock up */
		e100_set_multicast_list(nic->netdev);

	if(nic->flags & ich && cmd.speed==SPEED_10 && cmd.duplex==DUPLEX_HALF)
		/* Need SW workaround for ICH[x] 10Mbps/half duplex Tx hang. */
		nic->flags |= ich_10h_workaround;
	else
		nic->flags &= ~ich_10h_workaround;

//	mod_timer(&nic->watchdog, jiffies + E100_WATCHDOG_PERIOD);
}
#endif

static void e100_xmit_prepare(struct nic *nic, struct cb *cb,
	struct sk_buff *skb)
{
	cb->command = nic->tx_command;
	/* interrupt every 16 packets regardless of delay */
	if((nic->cbs_avail & ~15) == nic->cbs_avail)
		cb->command |= cpu_to_le16(cb_i);
	cb->u.tcb.tbd_array = cb->dma_addr + offsetof(struct cb, u.tcb.tbd);
	cb->u.tcb.tcb_byte_count = 0;
	cb->u.tcb.threshold = nic->tx_threshold;
	cb->u.tcb.tbd_count = 1;
	cb->u.tcb.tbd.buf_addr = cpu_to_le32(pci_map_single(nic->pdev,
		skb->data, skb->len, PCI_DMA_TODEVICE));
	/* check for mapping failure? */
	cb->u.tcb.tbd.size = cpu_to_le16(skb->len);
}

static int e100_xmit_frame(struct sk_buff *skb, struct net_device *netdev)
{
	struct nic *nic = netdev_priv(netdev);
	int err;

	if(nic->flags & ich_10h_workaround) {
		/* SW workaround for ICH[x] 10Mbps/half duplex Tx hang.
		   Issue a NOP command followed by a 1us delay before
		   issuing the Tx command. */
		if(e100_exec_cmd(nic, cuc_nop, 0))
			DPRINTK(TX_ERR, DEBUG, "exec cuc_nop failed\n");
		udelay(1);
	}

	err = e100_exec_cb(nic, skb, e100_xmit_prepare);

	switch(err) {
	case -ENOSPC:
		/* We queued the skb, but now we're out of space. */
		DPRINTK(TX_ERR, DEBUG, "No space for CB\n");
		netif_stop_queue(netdev);
		break;
	case -ENOMEM:
		/* This is a hard error - log it. */
		DPRINTK(TX_ERR, DEBUG, "Out of Tx resources, returning skb\n");
		netif_stop_queue(netdev);
		return 1;
	}

	netdev->trans_start = jiffies;
	return 0;
}

static int e100_tx_clean(struct nic *nic)
{
	struct cb *cb;
	int tx_cleaned = 0;

	spin_lock(&nic->cb_lock);

	DPRINTK(TX_DONE, DEBUG, "cb->status = 0x%04X\n",
		nic->cb_to_clean->status);

	/* Clean CBs marked complete */
	for(cb = nic->cb_to_clean;
	    cb->status & cpu_to_le16(cb_complete);
	    cb = nic->cb_to_clean = cb->next) {
		if(likely(cb->skb != NULL)) {
			nic->net_stats.tx_packets++;
			nic->net_stats.tx_bytes += cb->skb->len;

			pci_unmap_single(nic->pdev,
				le32_to_cpu(cb->u.tcb.tbd.buf_addr),
				le16_to_cpu(cb->u.tcb.tbd.size),
				PCI_DMA_TODEVICE);
			dev_kfree_skb_any(cb->skb);
			cb->skb = NULL;
			tx_cleaned = 1;
		}
		cb->status = 0;
	if(nic->cbs_avail<nic->params.cbs.count)nic->cbs_avail++;
	}

	spin_unlock(&nic->cb_lock);

	/* Recover from running out of Tx resources in xmit_frame */
	if(unlikely(tx_cleaned && netif_queue_stopped(nic->netdev)))
		netif_wake_queue(nic->netdev);

	return tx_cleaned;
}

static void e100_clean_cbs(struct nic *nic)
{
	if(nic->cbs) {
		while(nic->cbs_avail != nic->params.cbs.count) {
			struct cb *cb = nic->cb_to_clean;
			if(cb->skb) {
				pci_unmap_single(nic->pdev,
					le32_to_cpu(cb->u.tcb.tbd.buf_addr),
					le16_to_cpu(cb->u.tcb.tbd.size),
					PCI_DMA_TODEVICE);
				dev_kfree_skb(cb->skb);
			}
			nic->cb_to_clean = nic->cb_to_clean->next;
			nic->cbs_avail++;
		}
		pci_free_consistent(nic->pdev,
			sizeof(struct cb) * nic->params.cbs.count,
			nic->cbs, nic->cbs_dma_addr);
		nic->cbs = NULL;
		nic->cbs_avail = 0;
	}
	nic->cuc_cmd = cuc_start;
	nic->cb_to_use = nic->cb_to_send = nic->cb_to_clean =
		nic->cbs;
}

static int e100_alloc_cbs(struct nic *nic)
{
	struct cb *cb;
	unsigned int i, count = nic->params.cbs.count;

	nic->cuc_cmd = cuc_start;
	nic->cb_to_use = nic->cb_to_send = nic->cb_to_clean = NULL;
	nic->cbs_avail = 0;

	nic->cbs = pci_alloc_consistent(nic->pdev,
		sizeof(struct cb) * count, &nic->cbs_dma_addr);
	if(!nic->cbs)
		return -ENOMEM;

	for(cb = nic->cbs, i = 0; i < count; cb++, i++) {
		cb->next = (i + 1 < count) ? cb + 1 : nic->cbs;
		cb->prev = (i == 0) ? nic->cbs + count - 1 : cb - 1;

		cb->dma_addr = nic->cbs_dma_addr + i * sizeof(struct cb);
		cb->link = cpu_to_le32(nic->cbs_dma_addr +
			((i+1) % count) * sizeof(struct cb));
		cb->skb = NULL;
	}

	nic->cb_to_use = nic->cb_to_send = nic->cb_to_clean = nic->cbs;
	nic->cbs_avail = count;

	return 0;
}

static inline void e100_start_receiver(struct nic *nic, struct rx *rx)
{
	if(!nic->rxs) return;
	if(RU_SUSPENDED != nic->ru_running) return;

	/* handle init time starts */
	if(!rx) rx = nic->rxs;

	/* (Re)start RU if suspended or idle and RFA is non-NULL */
	if(rx->skb) {
		e100_exec_cmd(nic, ruc_start, rx->dma_addr);
		nic->ru_running = RU_RUNNING;
	}
}

#define RFD_BUF_LEN (sizeof(struct rfd) + VLAN_ETH_FRAME_LEN)
static int e100_rx_alloc_skb(struct nic *nic, struct rx *rx)
{
	if(!(rx->skb = dev_alloc_skb(RFD_BUF_LEN + NET_IP_ALIGN)))
		return -ENOMEM;

	/* Align, init, and map the RFD. */
	rx->skb->dev = nic->netdev;
	skb_reserve(rx->skb, NET_IP_ALIGN);
	memcpy(rx->skb->data, &nic->blank_rfd, sizeof(struct rfd));
	rx->dma_addr = pci_map_single(nic->pdev, rx->skb->data,
		RFD_BUF_LEN, PCI_DMA_BIDIRECTIONAL);

	if(pci_dma_mapping_error(rx->dma_addr)) {
		dev_kfree_skb_any(rx->skb);
		rx->skb = NULL;
		rx->dma_addr = 0;
		return -ENOMEM;
	}

	/* Link the RFD to end of RFA by linking previous RFD to
	 * this one, and clearing EL bit of previous.  */
	if(rx->prev->skb) {
		struct rfd *prev_rfd = (struct rfd *)rx->prev->skb->data;
		//put_unaligned(cpu_to_le32(rx->dma_addr), (u32 *)&prev_rfd->link);
		memcpy((u32 *)&prev_rfd->link,&rx->dma_addr,4);
		wmb();
		prev_rfd->command &= ~cpu_to_le16(cb_el);
		pci_dma_sync_single_for_device(nic->pdev, rx->prev->dma_addr,
			sizeof(struct rfd), PCI_DMA_TODEVICE);
	}

	return 0;
}

static int e100_rx_indicate(struct nic *nic, struct rx *rx,
	unsigned int *work_done, unsigned int work_to_do)
{
	struct sk_buff *skb = rx->skb;
	struct rfd *rfd = (struct rfd *)skb->data;
	u16 rfd_status, actual_size;

	if(unlikely(work_done && *work_done >= work_to_do))
		return -EAGAIN;

	/* Need to sync before taking a peek at cb_complete bit */
	pci_dma_sync_single_for_cpu(nic->pdev, rx->dma_addr,
		sizeof(struct rfd), PCI_DMA_FROMDEVICE);
	rfd_status = le16_to_cpu(rfd->status);

	DPRINTK(RX_STATUS, DEBUG, "status=0x%04X\n", rfd_status);

	/* If data isn't ready, nothing to indicate */
	if(unlikely(!(rfd_status & cb_complete)))
		return -ENODATA;

	/* Get actual data size */
	actual_size = le16_to_cpu(rfd->actual_size) & 0x3FFF;
	if(unlikely(actual_size > RFD_BUF_LEN - sizeof(struct rfd)))
		actual_size = RFD_BUF_LEN - sizeof(struct rfd);

	/* Get data */
	pci_unmap_single(nic->pdev, rx->dma_addr,
		RFD_BUF_LEN, PCI_DMA_FROMDEVICE);

	/* this allows for a fast restart without re-enabling interrupts */
	if(le16_to_cpu(rfd->command) & cb_el)
		nic->ru_running = RU_SUSPENDED;

	/* Pull off the RFD and put the actual data (minus eth hdr) */
	skb_reserve(skb, sizeof(struct rfd));
	skb_put(skb, actual_size);
//	skb->protocol = eth_type_trans(skb, nic->netdev);

	if(unlikely(!(rfd_status & cb_ok))) {
		/* Don't indicate if hardware indicates errors */
		dev_kfree_skb_any(skb);
	} else if(actual_size > ETH_DATA_LEN + VLAN_ETH_HLEN) {
		/* Don't indicate oversized frames */
		nic->rx_over_length_errors++;
		dev_kfree_skb_any(skb);
	} else {
		nic->net_stats.rx_packets++;
		nic->net_stats.rx_bytes += actual_size;
		nic->netdev->last_rx = jiffies;
		netif_receive_skb(skb);
		if(work_done)
			(*work_done)++;
	}

	rx->skb = NULL;

	return 0;
}

static void e100_rx_clean(struct nic *nic, unsigned int *work_done,
	unsigned int work_to_do)
{
	struct rx *rx;
	int restart_required = 0;
	struct rx *rx_to_start = NULL;

	/* are we already rnr? then pay attention!!! this ensures that
	 * the state machine progression never allows a start with a
	 * partially cleaned list, avoiding a race between hardware
	 * and rx_to_clean when in NAPI mode */
	if(RU_SUSPENDED == nic->ru_running)
		restart_required = 1;

	/* Indicate newly arrived packets */
	for(rx = nic->rx_to_clean; rx->skb; rx = nic->rx_to_clean = rx->next) {
		int err = e100_rx_indicate(nic, rx, work_done, work_to_do);
		if(-EAGAIN == err) {
			/* hit quota so have more work to do, restart once
			 * cleanup is complete */
			restart_required = 0;
			break;
		} else if(-ENODATA == err)
			break; /* No more to clean */
	}

	/* save our starting point as the place we'll restart the receiver */
	if(restart_required)
		rx_to_start = nic->rx_to_clean;

	/* Alloc new skbs to refill list */
	for(rx = nic->rx_to_use; !rx->skb; rx = nic->rx_to_use = rx->next) {
		if(unlikely(e100_rx_alloc_skb(nic, rx)))
			break; /* Better luck next time (see watchdog) */
	}

	if(restart_required) {
		// ack the rnr?
		writeb(stat_ack_rnr, &nic->csr->scb.stat_ack);
		e100_start_receiver(nic, rx_to_start);
		if(work_done)
			(*work_done)++;
	}
}

static void e100_rx_clean_list(struct nic *nic)
{
	struct rx *rx;
	unsigned int i, count = nic->params.rfds.count;

	nic->ru_running = RU_UNINITIALIZED;

	if(nic->rxs) {
		for(rx = nic->rxs, i = 0; i < count; rx++, i++) {
			if(rx->skb) {
				pci_unmap_single(nic->pdev, rx->dma_addr,
					RFD_BUF_LEN, PCI_DMA_FROMDEVICE);
				dev_kfree_skb(rx->skb);
			}
		}
		if(nic->rxs)kfree(nic->rxs);
		nic->rxs = NULL;
	}

	nic->rx_to_use = nic->rx_to_clean = NULL;
}

static int e100_rx_alloc_list(struct nic *nic)
{
	struct rx *rx;
	unsigned int i, count = nic->params.rfds.count;

	nic->rx_to_use = nic->rx_to_clean = NULL;
	nic->ru_running = RU_UNINITIALIZED;

	if(!(nic->rxs = kmalloc(sizeof(struct rx) * count, GFP_ATOMIC)))
		return -ENOMEM;
	memset(nic->rxs, 0, sizeof(struct rx) * count);

	for(rx = nic->rxs, i = 0; i < count; rx++, i++) {
		rx->next = (i + 1 < count) ? rx + 1 : nic->rxs;
		rx->prev = (i == 0) ? nic->rxs + count - 1 : rx - 1;
		if(e100_rx_alloc_skb(nic, rx)) {
			e100_rx_clean_list(nic);
			return -ENOMEM;
		}
	}

	nic->rx_to_use = nic->rx_to_clean = nic->rxs;
	nic->ru_running = RU_SUSPENDED;

	return 0;
}

static int e100_poll(struct net_device *netdev, int *budget);
static irqreturn_t e100_intr(void *dev_id)
{
	struct net_device *netdev = dev_id;
	struct nic *nic = netdev_priv(netdev);
	u8 stat_ack = readb(&nic->csr->scb.stat_ack);

//	DPRINTK(INTR, DEBUG, "stat_ack = 0x%02X\n", stat_ack);

	if(stat_ack == stat_ack_not_ours ||	/* Not our interrupt */
	   stat_ack == stat_ack_not_present)	/* Hardware is ejected */
		return IRQ_NONE;

	/* Ack interrupt(s) */
	writeb(stat_ack, &nic->csr->scb.stat_ack);

	/* We hit Receive No Resource (RNR); restart RU after cleaning */
	if(stat_ack & stat_ack_rnr)
		nic->ru_running = RU_SUSPENDED;

#if 0
	if(likely(netif_rx_schedule_prep(netdev))) {
		e100_disable_irq(nic);
		__netif_rx_schedule(netdev);
	}
#else
{
int budget=16;
	e100_poll(netdev,&budget);

}
#endif

	return IRQ_HANDLED;
}
static int e100_poll(struct net_device *netdev, int *budget)
{
	struct nic *nic = netdev_priv(netdev);
	unsigned int work_to_do = *budget;
	unsigned int work_done = 0;
	int tx_cleaned;

	e100_rx_clean(nic, &work_done, work_to_do);
	tx_cleaned = e100_tx_clean(nic);

	/* If no Rx and Tx cleanup work was done, exit polling mode. */
	if((!tx_cleaned && (work_done == 0)) || !netif_running(netdev)) {
		//netif_rx_complete(netdev);
		e100_enable_irq(nic);
		return 0;
	}

	*budget -= work_done;

	return 1;
}
#ifdef CONFIG_NET_POLL_CONTROLLER
static void e100_netpoll(struct net_device *netdev)
{
	struct nic *nic = netdev_priv(netdev);

	e100_disable_irq(nic);
	e100_intr(nic->pdev->irq, netdev, NULL);
	e100_tx_clean(nic);
	e100_enable_irq(nic);
}
#endif
#if 0
static struct net_device_stats *e100_get_stats(struct net_device *netdev)
{
	struct nic *nic = netdev_priv(netdev);
	return &nic->net_stats;
}

static int e100_set_mac_address(struct net_device *netdev, void *p)
{
	struct nic *nic = netdev_priv(netdev);
	struct sockaddr *addr = p;

	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;

	memcpy(netdev->dev_addr, addr->sa_data, netdev->addr_len);
	e100_exec_cb(nic, NULL, e100_setup_iaaddr);

	return 0;
}

static int e100_change_mtu(struct net_device *netdev, int new_mtu)
{
	if(new_mtu < ETH_ZLEN || new_mtu > ETH_DATA_LEN)
		return -EINVAL;
	netdev->mtu = new_mtu;
	return 0;
}
#endif

#ifdef CONFIG_PM
static int e100_asf(struct nic *nic)
{
	/* ASF can be enabled from eeprom */
	return((nic->pdev->device >= 0x1050) && (nic->pdev->device <= 0x1057) &&
	   (nic->eeprom[eeprom_config_asf] & eeprom_asf) &&
	   !(nic->eeprom[eeprom_config_asf] & eeprom_gcl) &&
	   ((nic->eeprom[eeprom_smbus_addr] & 0xFF) != 0xFE));
}
#endif

static int e100_up(struct nic *nic)
{
	int err;

	if((err = e100_rx_alloc_list(nic)))
		return err;
	if((err = e100_alloc_cbs(nic)))
		goto err_rx_clean_list;
	if((err = e100_hw_init(nic)))
	;//	goto err_clean_cbs;
	e100_set_multicast_list(nic->netdev);
	e100_start_receiver(nic, NULL);
//	mod_timer(&nic->watchdog, jiffies);
	if((err = request_irq(nic->pdev->irq, e100_intr, IRQF_SHARED,
		nic->netdev->name, nic->netdev)))
		goto err_no_irq;
//	netif_wake_queue(nic->netdev);
//	netif_poll_enable(nic->netdev);
	/* enable ints _after_ enabling poll, preventing a race between
	 * disable ints+schedule */
	e100_enable_irq(nic);
	return 0;

err_no_irq:
//	del_timer_sync(&nic->watchdog);
err_clean_cbs:
	e100_clean_cbs(nic);
err_rx_clean_list:
	e100_rx_clean_list(nic);
	return err;
}

static void e100_down(struct nic *nic)
{
	/* wait here for poll to complete */
//	netif_poll_disable(nic->netdev);
	netif_stop_queue(nic->netdev);
	e100_hw_reset(nic);
	free_irq(nic->pdev->irq, nic->netdev);
//	del_timer_sync(&nic->watchdog);
//	netif_carrier_off(nic->netdev);
	e100_clean_cbs(nic);
	e100_rx_clean_list(nic);
}

static void e100_tx_timeout(struct net_device *netdev)
{
	struct nic *nic = netdev_priv(netdev);

	/* Reset outside of interrupt context, to avoid request_irq
	 * in interrupt context */
//	schedule_work(&nic->tx_timeout_task);
	e100_tx_timeout_task(netdev);
}

static void e100_tx_timeout_task(struct net_device *netdev)
{
	struct nic *nic = netdev_priv(netdev);

	DPRINTK(TX_ERR, DEBUG, "scb.status=0x%02X\n",
		readb(&nic->csr->scb.status));
	e100_down(netdev_priv(netdev));
	e100_up(netdev_priv(netdev));
}

static int e100_loopback_test(struct nic *nic, enum loopback loopback_mode)
{
	int err;
	struct sk_buff *skb;

	/* Use driver resources to perform internal MAC or PHY
	 * loopback test.  A single packet is prepared and transmitted
	 * in loopback mode, and the test passes if the received
	 * packet compares byte-for-byte to the transmitted packet. */

	if((err = e100_rx_alloc_list(nic)))
		return err;
	if((err = e100_alloc_cbs(nic)))
		goto err_clean_rx;

	/* ICH PHY loopback is broken so do MAC loopback instead */
	if(nic->flags & ich && loopback_mode == lb_phy)
		loopback_mode = lb_mac;

	nic->loopback = loopback_mode;
	if((err = e100_hw_init(nic)))
		goto err_loopback_none;

	if(loopback_mode == lb_phy)
		mdio_write(nic->netdev, nic->mii.phy_id, MII_BMCR,
			BMCR_LOOPBACK);

	e100_start_receiver(nic, NULL);

	if(!(skb = dev_alloc_skb(ETH_DATA_LEN))) {
		err = -ENOMEM;
		goto err_loopback_none;
	}
	skb_put(skb, ETH_DATA_LEN);
	memset(skb->data, 0xFF, ETH_DATA_LEN);
	e100_xmit_frame(skb, nic->netdev);

	msleep(10);

	pci_dma_sync_single_for_cpu(nic->pdev, nic->rx_to_clean->dma_addr,
			RFD_BUF_LEN, PCI_DMA_FROMDEVICE);

	if(memcmp(nic->rx_to_clean->skb->data + sizeof(struct rfd),
	   skb->data, ETH_DATA_LEN))
		err = -EAGAIN;

err_loopback_none:
	mdio_write(nic->netdev, nic->mii.phy_id, MII_BMCR, 0);
	nic->loopback = lb_none;
	e100_clean_cbs(nic);
	e100_hw_reset(nic);
err_clean_rx:
	e100_rx_clean_list(nic);
	return err;
}

#define MII_LED_CONTROL	0x1B
static void e100_blink_led(unsigned long data)
{
	struct nic *nic = (struct nic *)data;
	enum led_state {
		led_on     = 0x01,
		led_off    = 0x04,
		led_on_559 = 0x05,
		led_on_557 = 0x07,
	};

	nic->leds = (nic->leds & led_on) ? led_off :
		(nic->mac < mac_82559_D101M) ? led_on_557 : led_on_559;
	mdio_write(nic->netdev, nic->mii.phy_id, MII_LED_CONTROL, nic->leds);
//	mod_timer(&nic->blink_timer, jiffies + HZ / 4);
}

#if 0
static int e100_do_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd)
{
	struct nic *nic = netdev_priv(netdev);

	return generic_mii_ioctl(&nic->mii, if_mii(ifr), cmd, NULL);
}
#endif

static int e100_alloc(struct nic *nic)
{
	nic->mem = pci_alloc_consistent(nic->pdev, sizeof(struct mem),
		&nic->dma_addr);
	return nic->mem ? 0 : -ENOMEM;
}

static void e100_free(struct nic *nic)
{
	if(nic->mem) {
		pci_free_consistent(nic->pdev, sizeof(struct mem),
			nic->mem, nic->dma_addr);
		nic->mem = NULL;
	}
}

static int e100_open(struct net_device *netdev)
{
	struct nic *nic = netdev_priv(netdev);
	int err = 0;

//	netif_carrier_off(netdev);
	if((err = e100_up(nic)))
		DPRINTK(IFUP, ERR, "Cannot open interface, aborting.\n");
	return err;
}

static int e100_close(struct net_device *netdev)
{
	e100_down(netdev_priv(netdev));
	return 0;
}

static int e100_probe(struct net_device *netdev,struct pci_dev *pdev,
	const struct pci_device_id *ent)
{
	struct nic *nic;
	int err;

#if 0
	netdev->open = e100_open;
	netdev->stop = e100_close;
	netdev->hard_start_xmit = e100_xmit_frame;
	netdev->get_stats = e100_get_stats;
	netdev->set_multicast_list = e100_set_multicast_list;
	netdev->set_mac_address = e100_set_mac_address;
	netdev->change_mtu = e100_change_mtu;
//	netdev->do_ioctl = e100_do_ioctl;
//	SET_ETHTOOL_OPS(netdev, &e100_ethtool_ops);
	netdev->tx_timeout = e100_tx_timeout;
	netdev->watchdog_timeo = E100_WATCHDOG_PERIOD;
	netdev->poll = e100_poll;
	netdev->weight = E100_NAPI_WEIGHT;
#ifdef CONFIG_NET_POLL_CONTROLLER
	netdev->poll_controller = e100_netpoll;
#endif
	strcpy(netdev->name, pci_name(pdev));
#endif
	strcpy(netdev->name,netdev->sc_dev.dv_xname);

	nic = netdev_priv(netdev);
	nic->netdev = netdev;
	nic->pdev = pdev;
	nic->msg_enable = (1 << debug) - 1;
	pci_set_drvdata(pdev, netdev);


	nic->csr = ioremap(pci_resource_start(pdev, 0), sizeof(struct csr));
	if(!nic->csr) {
		DPRINTK(PROBE, ERR, "Cannot map device registers, aborting.\n");
		err = -ENOMEM;
		goto err_out_free_res;
	}

	if(ent->driver_data)
		nic->flags |= ich;
	else
		nic->flags &= ~ich;

	e100_get_defaults(nic);

	/* locks must be initialized before calling hw_reset */
	spin_lock_init(&nic->cb_lock);
	spin_lock_init(&nic->cmd_lock);
	spin_lock_init(&nic->mdio_lock);

	/* Reset the device before pci_set_master() in case device is in some
	 * funky state and has an interrupt pending - hint: we don't have the
	 * interrupt handler registered yet. */
	e100_hw_reset(nic);

//	pci_set_master(pdev);

//	init_timer(&nic->watchdog);
//	nic->watchdog.function = e100_watchdog;
//	nic->watchdog.data = (unsigned long)nic;
//	init_timer(&nic->blink_timer);
//	nic->blink_timer.function = e100_blink_led;
//	nic->blink_timer.data = (unsigned long)nic;

//	INIT_WORK(&nic->tx_timeout_task, (void (*)(void *))e100_tx_timeout_task, netdev);

	if((err = e100_alloc(nic))) {
		DPRINTK(PROBE, ERR, "Cannot alloc driver memory, aborting.\n");
		goto err_out_iounmap;
	}

	if((err = e100_eeprom_load(nic)))
	{
		cmd_wrprom_fxp0(0,0);
//		goto err_out_free;
	}

	e100_phy_init(nic);

	memcpy(netdev->dev_addr, nic->eeprom, ETH_ALEN);
	memcpy(netdev->perm_addr, nic->eeprom, ETH_ALEN);


#if 0
	if(!is_valid_ether_addr(netdev->perm_addr)) {
		DPRINTK(PROBE, ERR, "Invalid MAC address from "
			"EEPROM, aborting.\n");
		err = -EAGAIN;
		goto err_out_free;
	}
#endif

	/* Wol magic packet can be enabled from eeprom */
	if((nic->mac >= mac_82558_D101_A4) &&
	   (nic->eeprom[eeprom_id] & eeprom_id_wol))
		nic->flags |= wol_magic;

	/* ack any pending wake events, disable PME */
	err = pci_enable_wake(pdev, 0, 0);
	if (err)
		DPRINTK(PROBE, ERR, "Error clearing wake event\n");

#if 0
	if((err = register_netdev(netdev))) {
		DPRINTK(PROBE, ERR, "Cannot register net device, aborting.\n");
		goto err_out_free;
	}
#endif

	DPRINTK(PROBE, INFO, "addr 0x%llx, irq %d, "
		"MAC addr %02X:%02X:%02X:%02X:%02X:%02X\n",
		(unsigned long long)pci_resource_start(pdev, 0), pdev->irq,
		netdev->dev_addr[0], netdev->dev_addr[1], netdev->dev_addr[2],
		netdev->dev_addr[3], netdev->dev_addr[4], netdev->dev_addr[5]);

	return 0;

err_out_free:
	e100_free(nic);
err_out_iounmap:
	iounmap(nic->csr);
err_out_free_res:
//	pci_release_regions(pdev);
err_out_disable_pdev:
//	pci_disable_device(pdev);
err_out_free_dev:
	pci_set_drvdata(pdev, NULL);
//	free_netdev(netdev);
	return err;
}

static void  e100_remove(struct pci_dev *pdev)
{
	struct net_device *netdev = pci_get_drvdata(pdev);

	if(netdev) {
		struct nic *nic = netdev_priv(netdev);
//		unregister_netdev(netdev);
		e100_free(nic);
		iounmap(nic->csr);
//		free_netdev(netdev);
//		pci_release_regions(pdev);
//		pci_disable_device(pdev);
		pci_set_drvdata(pdev, NULL);
	}
}

#ifdef CONFIG_PM
static int e100_suspend(struct pci_dev *pdev, pm_message_t state)
{
	struct net_device *netdev = pci_get_drvdata(pdev);
	struct nic *nic = netdev_priv(netdev);
	int retval;

	if(netif_running(netdev))
		e100_down(nic);
	e100_hw_reset(nic);
	netif_device_detach(netdev);

	pci_save_state(pdev);
	retval = pci_enable_wake(pdev, pci_choose_state(pdev, state), nic->flags & (wol_magic | e100_asf(nic)));
	if (retval)
		DPRINTK(PROBE,ERR, "Error enabling wake\n");
	pci_disable_device(pdev);
	retval = pci_set_power_state(pdev, pci_choose_state(pdev, state));
	if (retval)
		DPRINTK(PROBE,ERR, "Error %d setting power state\n", retval);

	return 0;
}

static int e100_resume(struct pci_dev *pdev)
{
	struct net_device *netdev = pci_get_drvdata(pdev);
	struct nic *nic = netdev_priv(netdev);
	int retval;

	retval = pci_set_power_state(pdev, PCI_D0);
	if (retval)
		DPRINTK(PROBE,ERR, "Error waking adapter\n");
	pci_restore_state(pdev);
	/* ack any pending wake events, disable PME */
	retval = pci_enable_wake(pdev, 0, 0);
	if (retval)
		DPRINTK(PROBE,ERR, "Error clearing wake events\n");

	netif_device_attach(netdev);
	if(netif_running(netdev))
		e100_up(nic);

	return 0;
}
#endif


static void e100_shutdown(struct pci_dev *pdev)
{
	struct net_device *netdev = pci_get_drvdata(pdev);
	struct nic *nic = netdev_priv(netdev);
	int retval;

#ifdef CONFIG_PM
	retval = pci_enable_wake(pdev, 0, nic->flags & (wol_magic | e100_asf(nic)));
#else
	retval = pci_enable_wake(pdev, 0, nic->flags & (wol_magic));
#endif
	if (retval)
		DPRINTK(PROBE,ERR, "Error enabling wake\n");
}

#if 0
/* ------------------ PCI Error Recovery infrastructure  -------------- */
/**
 * e100_io_error_detected - called when PCI error is detected.
 * @pdev: Pointer to PCI device
 * @state: The current pci conneection state
 */
static int e100_io_error_detected(struct pci_dev *pdev, pci_channel_state_t state)
{
	struct net_device *netdev = pci_get_drvdata(pdev);

	/* Similar to calling e100_down(), but avoids adpater I/O. */
	netdev->stop(netdev);

	/* Detach; put netif into state similar to hotplug unplug. */
	netif_poll_enable(netdev);
	netif_device_detach(netdev);

	/* Request a slot reset. */
	return PCI_ERS_RESULT_NEED_RESET;
}

/**
 * e100_io_slot_reset - called after the pci bus has been reset.
 * @pdev: Pointer to PCI device
 *
 * Restart the card from scratch.
 */
static int e100_io_slot_reset(struct pci_dev *pdev)
{
	struct net_device *netdev = pci_get_drvdata(pdev);
	struct nic *nic = netdev_priv(netdev);

	if (pci_enable_device(pdev)) {
		printk(KERN_ERR "e100: Cannot re-enable PCI device after reset.\n");
		return PCI_ERS_RESULT_DISCONNECT;
	}
	pci_set_master(pdev);

	/* Only one device per card can do a reset */
	if (0 != PCI_FUNC(pdev->devfn))
		return PCI_ERS_RESULT_RECOVERED;
	e100_hw_reset(nic);
	e100_phy_init(nic);

	return PCI_ERS_RESULT_RECOVERED;
}

/**
 * e100_io_resume - resume normal operations
 * @pdev: Pointer to PCI device
 *
 * Resume normal operations after an error recovery
 * sequence has been completed.
 */
static void e100_io_resume(struct pci_dev *pdev)
{
	struct net_device *netdev = pci_get_drvdata(pdev);
	struct nic *nic = netdev_priv(netdev);

	/* ack any pending wake events, disable PME */
	pci_enable_wake(pdev, 0, 0);

	netif_device_attach(netdev);
	if (netif_running(netdev)) {
		e100_open(netdev);
		mod_timer(&nic->watchdog, jiffies);
	}
}

static struct pci_error_handlers e100_err_handler = {
	.error_detected = e100_io_error_detected,
	.slot_reset = e100_io_slot_reset,
	.resume = e100_io_resume,
};

static struct pci_driver e100_driver = {
	.name =         DRV_NAME,
	.id_table =     e100_id_table,
	.probe =        e100_probe,
	.remove =       __devexit_p(e100_remove),
#ifdef CONFIG_PM
	.suspend =      e100_suspend,
	.resume =       e100_resume,
#endif
	.shutdown =     e100_shutdown,
	.err_handler = &e100_err_handler,
};

static int __init e100_init_module(void)
{
	if(((1 << debug) - 1) & NETIF_MSG_DRV) {
		printk(KERN_INFO PFX "%s, %s\n", DRV_DESCRIPTION, DRV_VERSION);
		printk(KERN_INFO PFX "%s\n", DRV_COPYRIGHT);
	}
	return pci_module_init(&e100_driver);
}

static void __exit e100_cleanup_module(void)
{
	pci_unregister_driver(&e100_driver);
}

module_init(e100_init_module);
module_exit(e100_cleanup_module);
#endif
//-------------------------
#if 0
#endif

//----------------------------------------------------------

static void netdev_init(struct net_device *netdev,struct pci_attach_args *pa)
{
    unsigned short  vendor;
    unsigned short  device,class;
    unsigned short  subsystem_vendor;
    unsigned short  subsystem_device;
    unsigned int i;
	static int irq=0;

        vendor = pa->pa_id & 0xffff;
        device = (pa->pa_id >> 16) & 0xffff;
        class=(pa->pa_class >>8);
        i=pci_conf_read(0,pa->pa_tag,PCI_SUBSYSTEM_VENDOR_ID);
        subsystem_vendor=i&0xffff;
        subsystem_device=i>>16;
	netdev->pcidev.vendor=vendor;
	netdev->pcidev.device=device;
	netdev->pcidev.subsystem_vendor=subsystem_vendor;
    netdev->pcidev.subsystem_device=subsystem_device;
	netdev->pcidev.pa=*pa;
	netdev->priv=kmalloc(sizeof(struct nic));//&netdev->em;
	memset(netdev->priv,0,sizeof(struct nic));
	netdev->addr_len=6;
	netdev->pcidev.irq=irq++;
}

static int fxp_ether_ioctl(struct ifnet *ifp,FXP_IOCTLCMD_TYPE cmd,caddr_t data);

static struct pci_device_id *fxp_pci_id=0;
/*
 * Check if a device is an 82557.
 */
static void fxp_start(struct ifnet *ifp);
static int
fxp_match(parent, match, aux)
	struct device *parent;
#if defined(__BROKEN_INDIRECT_CONFIG) || defined(__OpenBSD__)
	void *match;
#else
	struct cfdata *match;
#endif
	void *aux;
{
	struct pci_attach_args *pa = aux;
fxp_pci_id=pci_match_device(e100_id_table,pa);
return fxp_pci_id?1:0;
}

static void
fxp_shutdown(sc)
        void *sc;
{
}


extern char activeif_name[];
static int fxp_intr(void *data)
{
struct net_device *netdev = data;
int irq=netdev->irq;
struct ifnet *ifp = &netdev->arpcom.ac_if;
	if(ifp->if_flags & IFF_RUNNING)
	{
			e100_intr(data);
		   if (ifp->if_snd.ifq_head != NULL)
		   fxp_start(ifp);
	return 1;
	}
	return 0;
}

static void fxp_watchdog( struct ifnet *ifp )
{
	struct net_device *dev = ifp->if_softc;
	struct nic *np = netdev_priv(dev);
	e100_watchdog(np);
	ifp->if_timer = 5;
}

static struct net_device *mynic_fxp;

static void
fxp_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct net_device  *sc = (struct net_device *)self;
	struct pci_attach_args *pa = aux;
	//pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	const char *intrstr = NULL;
	struct ifnet *ifp;
#ifdef __OpenBSD__
	//bus_space_tag_t iot = pa->pa_iot;
	//bus_addr_t iobase;
	//bus_size_t iosize;
#endif
	mynic_fxp = sc ;

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
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, fxp_intr, sc, self->dv_xname);
#else
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, fxp_intr, sc);
#endif
	
	if (sc->sc_ih == NULL) {
		printf(": couldn't establish interrupt");
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	
	netdev_init(sc,pa);
	/* Do generic parts of attach. */
	if (e100_probe(sc,&sc->pcidev,fxp_pci_id)) {
		/* Failed! */
		return;
	}

#ifdef __OpenBSD__
	ifp = &sc->arpcom.ac_if;
	bcopy(sc->dev_addr, sc->arpcom.ac_enaddr, sc->addr_len);
#else
	ifp = &sc->sc_ethercom.ec_if;
#endif
	bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
	
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = fxp_ether_ioctl;
	ifp->if_start = fxp_start;
	ifp->if_watchdog = fxp_watchdog;
	ifp->if_timer = 5;

	printf(": %s, address %s\n", intrstr,
	    ether_sprintf(sc->arpcom.ac_enaddr));

	/*
	 * Attach the interface.
	 */
	if_attach(ifp);
	/*
	 * Let the system queue as many packets as we have available
	 * TX descriptors.
	 */
	ifp->if_snd.ifq_maxlen = 4;
#ifdef __NetBSD__
	ether_ifattach(ifp, sc->dev_addr);
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

	/*
	 * Add shutdown hook so that DMA is disabled prior to reboot. Not
	 * doing do could allow DMA to corrupt kernel memory during the
	 * reboot before the driver initializes.
	 */
	shutdownhook_establish(fxp_shutdown, sc);

#ifndef PMON
	/*
	 * Add suspend hook, for similiar reasons..
	 */
	powerhook_establish(fxp_power, sc);
#endif
}


/*
 * Start packet transmission on the interface.
 */



static void fxp_start(struct ifnet *ifp)
{
	struct net_device *sc = ifp->if_softc;
	struct mbuf *mb_head;		
	struct sk_buff *skb;

	while(ifp->if_snd.ifq_head != NULL ){
		
		IF_DEQUEUE(&ifp->if_snd, mb_head);
		
		skb=dev_alloc_skb(mb_head->m_pkthdr.len);
		m_copydata(mb_head, 0, mb_head->m_pkthdr.len, skb->data);
		skb->len=mb_head->m_pkthdr.len;
		e100_xmit_frame(skb,sc);

		m_freem(mb_head);
		wbflush();
	} 
}

static int
fxp_init(struct net_device *netdev)
{
    struct ifnet *ifp = &netdev->arpcom.ac_if;
	int stat=0;
	if(!netdev->opencount){ stat=e100_open(netdev);netdev->opencount++;}
    ifp->if_flags |= IFF_RUNNING;
	return stat;
}

static int
fxp_stop(struct net_device *netdev)
{
    struct ifnet *ifp = &netdev->arpcom.ac_if;
	ifp->if_timer = 0;
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	if(netdev->opencount){e100_close(netdev);netdev->opencount--;}
	return 0;
}


static long long e100_read_mac(struct fxp_softc *nic);
static int
fxp_ether_ioctl(ifp, cmd, data)
	struct ifnet *ifp;
	FXP_IOCTLCMD_TYPE cmd;
	caddr_t data;
{
	struct ifaddr *ifa = (struct ifaddr *) data;
	struct net_device *sc = ifp->if_softc;
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
			error = fxp_init(sc);
			if(error <0 )
				return(error);
			ifp->if_flags |= IFF_UP;

#ifdef __OpenBSD__
			arp_ifinit(&sc->arpcom, ifa);
#else
			arp_ifinit(ifp, ifa);
#endif
			
			break;
#endif

		default:
			error = fxp_init(sc);
			if(error <0 )
				return(error);
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
			error = fxp_init(sc);
			if(error <0 )
				return(error);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				fxp_stop(sc);
		}
		break;
       case SIOCETHTOOL:
                {
                long *p=data;
                mynic_fxp = sc;
                cmd_setmac_fxp0(p[0],p[1]);
                }
                break;
	case SIOCGETHERADDR:
	{
		long long val;
		char *p=data;
		mynic_fxp = sc;
		val =e100_read_mac(netdev_priv(mynic_fxp));
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
                mynic_fxp = sc;
                cmd_reprom_fxp0(p[0],p[1]);
                }
                break;
       case SIOCWREEPROM:
                {
                long *p=data;
                mynic_fxp = sc;
                cmd_wrprom_fxp0(p[0],p[1]);
                }
                break;

	default:
		error = EINVAL;
	}

	splx(s);
	return (error);
}

struct cfattach fxp_ca = {
	sizeof(struct net_device), fxp_match, fxp_attach
};

struct cfdriver fxp_cd = {
	NULL, "fxp", DV_IFNET
};
#define EEPROM_CHECKSUM_REG        0x003F

void fxp_read_eeprom(nic, data, offset, words)
	struct nic *nic;
	u_short *data;
	int offset;
	int words;
{
	u16 addr, addr_len = 8, i;
		e100_eeprom_read(nic, &addr_len, addr);
		for(i=0;i<words;i++)
		data[i] = e100_eeprom_read(nic, &addr_len, offset+i);
}

static long long e100_read_mac(struct fxp_softc *nic)
{

        int i;
        long long mac_tmp = 0;
	u_int16_t enaddr[3];
	unsigned short tmp=0;

        fxp_read_eeprom(nic, enaddr, 0, 3);

        for (i = 0; i < 3; i++) {
		tmp=0;
		tmp = ((enaddr[i] & 0xff) <<8)|((enaddr[i] & 0xff00)>>8); 
                mac_tmp <<= 16;
                mac_tmp |= tmp;
	}
        return mac_tmp;
}

int cmd_setmac_fxp0(int ac, char *av[])
{
        int i;
        unsigned short v;
		struct nic *nic ;
        unsigned short val_fxp = 0;

        if(mynic_fxp == NULL){
               printf("epro100 interface not initialized\n");
                return 0;
        }
		nic = netdev_priv(mynic_fxp);
        
	if(ac != 2){
        long long macaddr;
        u_int8_t *paddr;
        u_int8_t enaddr[6];
        macaddr=e100_read_mac(nic);
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
                val_fxp = 0;
                gethex(&v, av[1], 2);
                val_fxp = v ;
                av[1]+=3;
                gethex(&v, av[1], 2);
                val_fxp = val_fxp | (v << 8);
                av[1] += 3;
			 nic->eeprom[i]=val_fxp;
        }
		e100_eeprom_save(nic,0,3);
	printf("Write MAC address successfully!\n");
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

int cmd_wrprom_fxp0(int ac,char **av)
{
        int i=0;
        unsigned long clocks_num=0;
        unsigned char tmp[4];
	unsigned short eeprom_data;
        unsigned short rom[EEPROM_CHECKSUM_REG+1]={
#if 0			
				0x0a00, 0x5dc4, 0x9b78, 0x0203, 0x0000, 0x0201, 0x4701, 0x0000,
				0xa276, 0x9501, 0x5022, 0x5022, 0x5022, 0x007f, 0x0000, 0x0000,
				0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
				0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
				0x0000, 0x0000, 0x0000, 0x1229, 0x0000, 0x0000, 0x0000, 0x0000,
				0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
				0x0128, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
				0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x30cc
#else
                                0x0200, 0x00B3, 0x0000, 0x0203, 0xFFFF, 0x0201, 0x4701, 0xFFFF,
                                0xA795, 0x7401, 0x5FA2, 0x0070, 0x8086, 0x007F, 0xFFFF, 0xFFFF,
                                0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
                                0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
                                0xFFFF, 0xFFFF, 0xFFFF, 0x1209, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
                                0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
                                0x00EC, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
                                0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
#endif
      	             };

	struct nic *nic = netdev_priv(mynic_fxp);
	
        printf("Now beginningwrite whole eprom\n");

#if 1
                clocks_num =CPU_GetCOUNT(); // clock();
                mysrand(clocks_num);
                for( i = 0; i < 4;i++ )
                {
                        tmp[i]=myrand()%256;
                        printf( " tmp[%d]=0x%2x\n", i,tmp[i]);
                }
                eeprom_data =tmp[1] |( tmp[0]<<8);
                rom[1] = eeprom_data ;
                printf("eeprom_data [1] = 0x%4x\n",eeprom_data);
                eeprom_data =tmp[3] |( tmp[2]<<8);
                rom[2] = eeprom_data;
                printf("eeprom_data [2] = 0x%4x\n",eeprom_data);
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

	   memcpy(nic->eeprom,rom,EEPROM_CHECKSUM_REG*2);
	   e100_eeprom_save(nic,0,EEPROM_CHECKSUM_REG);
	
	printf("Write the whole eeprom OK!\n");
	
        
	return 0;
}

int cmd_reprom_fxp0(int ac, char *av)
{
        int i;
        unsigned short eeprom_data;
	struct nic *nic = netdev_priv(mynic_fxp);
		e100_eeprom_load(nic);
        printf("dump eprom:\n");

        for(i=0; i <= EEPROM_CHECKSUM_REG;)
        {
                printf("%04x ", nic->eeprom[i]);
                ++i;
                if( i%8 == 0 )
                        printf("\n");
        }
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
        {"fxp"},
        {"setmac_fxp", "", NULL,
                    "Set mac address into E100 eeprom", cmd_setmac_fxp0, 1, 5, 0},
        {"readrom_fxp", "", NULL,
                        "dump E100 eprom content", cmd_reprom_fxp0, 1, 2, 0},
        {"writerom_fxp", "", NULL,
                        "write E100 eprom content", cmd_wrprom_fxp0, 1, 2, 0},
        {0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
        cmdlist_expand(Cmds, 1);
}

