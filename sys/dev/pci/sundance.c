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
//#include <linux/pci.h>
typedef unsigned int dma_addr_t;
#define PCI_SUBSYSTEM_VENDOR_ID 0x2c
#define PCI_ANY_ID (~0)
#define KERN_DEBUG
#define kmalloc(size,...)  malloc(size,M_DEVBUF, M_DONTWAIT)
#define kfree(addr,...) free(addr,M_DEVBUF);
#define netdev_priv(dev) dev->priv
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
extern int ticks;
#define jiffies ticks
#define KERN_ERR
#define KERN_NOTICE
#define ETH_ALEN	6		/* Octets in one ethernet addr	 */
#define udelay delay
static inline void mdelay(int microseconds){
int i;
for(i=0;i<microseconds;i++)delay(microseconds);
}
#define le32_to_cpu(x) (x)
#define cpu_to_le32(x) (x)
#define spin_lock_irqsave(...)
#define spin_unlock_irqrestore(...)
#define netif_start_queue(...)
#define netif_stop_queue(...)
#define netif_queue_stopped(...) 0
void delay1(int microseconds);
static int mysleep(int ms)
{
	int i;

	for(i=0;i<ms;i++)
	{
	delay1(1000);
	printf("%d\n",i);
	}
	return 0;
}

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
            (ids->subdevice == PCI_ANY_ID || ids->subdevice == subsystem_device) &&
            !((ids->class ^ class) & ids->class_mask))
            return ids;
        ids++;
    }
    return NULL;
}


static void pci_free_consistent(struct pci_dev *pdev, size_t size, void *cpu_addr,
            dma_addr_t dma_addr)
{
	kfree(cpu_addr);
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
    skb->len=0;
    skb->data=skb->head=(void *) kmalloc(length,GFP_KERNEL);
return skb;
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
//--------------------------------------------------------------------
/* sundance.c: A Linux device driver for the Sundance ST201 "Alta". */
/*
	Written 1999-2000 by Donald Becker.

	This software may be used and distributed according to the terms of
	the GNU General Public License (GPL), incorporated herein by reference.
	Drivers based on or derived from this code fall under the GPL and must
	retain the authorship, copyright and license notice.  This file is not
	a complete program and may only be used when the entire operating
	system is licensed under the GPL.

	The author may be reached as becker@scyld.com, or C/O
	Scyld Computing Corporation
	410 Severn Ave., Suite 210
	Annapolis MD 21403

	Support and updates available at
	http://www.scyld.com/network/sundance.html
	[link no longer provides useful info -jgarzik]

*/

#define DRV_NAME	"sundance"
#define DRV_VERSION	"1.1"
#define DRV_RELDATE	"27-Jun-2006"


/* The user-configurable values.
   These may be modified when a driver module is loaded.*/
static int debug = 1;			/* 1 normal messages, 0 quiet .. 7 verbose. */
/* Maximum number of multicast addresses to filter (vs. rx-all-multicast).
   Typical is a 64 element hash table based on the Ethernet CRC.  */
static const int multicast_filter_limit = 32;

/* Set the copy breakpoint for the copy-only-tiny-frames scheme.
   Setting to > 1518 effectively disables this feature.
   This chip can receive into offset buffers, so the Alpha does not
   need a copy-align. */
static int rx_copybreak;
static int flowctrl=1;

/* media[] specifies the media type the NIC operates at.
		 autosense	Autosensing active media.
		 10mbps_hd 	10Mbps half duplex.
		 10mbps_fd 	10Mbps full duplex.
		 100mbps_hd 	100Mbps half duplex.
		 100mbps_fd 	100Mbps full duplex.
		 0		Autosensing active media.
		 1	 	10Mbps half duplex.
		 2	 	10Mbps full duplex.
		 3	 	100Mbps half duplex.
		 4	 	100Mbps full duplex.
*/
#define MAX_UNITS 8
static char *media[MAX_UNITS];


/* Operational parameters that are set at compile time. */

/* Keep the ring sizes a power of two for compile efficiency.
   The compiler will convert <unsigned>'%'<2^N> into a bit mask.
   Making the Tx ring too large decreases the effectiveness of channel
   bonding and packet priority, and more than 128 requires modifying the
   Tx error recovery.
   Large receive rings merely waste memory. */
#define TX_RING_SIZE	32
#define TX_QUEUE_LEN	(TX_RING_SIZE - 1) /* Limit ring entries actually used.  */
#define RX_RING_SIZE	64
#define RX_BUDGET	32
#define TX_TOTAL_SIZE	TX_RING_SIZE*sizeof(struct netdev_desc)
#define RX_TOTAL_SIZE	RX_RING_SIZE*sizeof(struct netdev_desc)

/* Operational parameters that usually are not changed. */
/* Time in jiffies before concluding the transmitter is hung. */
#define TX_TIMEOUT  (4*HZ)
#define PKT_BUF_SZ		1536	/* Size of each temporary Rx buffer.*/

/* Include files, designed to support most kernel versions 2.0.0 and later. */

/* Work-around for Kendin chip bugs. */
#ifndef CONFIG_SUNDANCE_MMIO
#define USE_IO_OPS 1
#endif

static const struct pci_device_id sundance_pci_tbl[] = {
	{ 0x1186, 0x1002, 0x1186, 0x1002, 0, 0, 0 },
	{ 0x1186, 0x1002, 0x1186, 0x1003, 0, 0, 1 },
	{ 0x1186, 0x1002, 0x1186, 0x1012, 0, 0, 2 },
	{ 0x1186, 0x1002, 0x1186, 0x1040, 0, 0, 3 },
	{ 0x1186, 0x1002, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 4 },
	{ 0x13F0, 0x0201, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 5 },
	{ 0x13F0, 0x0200, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 6 },
	{ }
};

enum {
	netdev_io_size = 128
};

struct pci_id_info {
        const char *name;
};
static const struct pci_id_info pci_id_tbl[] = {
	{"D-Link DFE-550TX FAST Ethernet Adapter"},
	{"D-Link DFE-550FX 100Mbps Fiber-optics Adapter"},
	{"D-Link DFE-580TX 4 port Server Adapter"},
	{"D-Link DFE-530TXS FAST Ethernet Adapter"},
	{"D-Link DL10050-based FAST Ethernet Adapter"},
	{"Sundance Technology Alta"},
	{"IC Plus Corporation IP100A FAST Ethernet Adapter"},
	{ }	/* terminate list. */
};

/* This driver was written to use PCI memory space, however x86-oriented
   hardware often uses I/O space accesses. */

/* Offsets to the device registers.
   Unlike software-only systems, device drivers interact with complex hardware.
   It's not useful to define symbolic names for every register bit in the
   device.  The name can only partially document the semantics and make
   the driver longer and more difficult to read.
   In general, only the important configuration values or bits changed
   multiple times should be defined symbolically.
*/
enum alta_offsets {
	DMACtrl = 0x00,
	TxListPtr = 0x04,
	TxDMABurstThresh = 0x08,
	TxDMAUrgentThresh = 0x09,
	TxDMAPollPeriod = 0x0a,
	RxDMAStatus = 0x0c,
	RxListPtr = 0x10,
	DebugCtrl0 = 0x1a,
	DebugCtrl1 = 0x1c,
	RxDMABurstThresh = 0x14,
	RxDMAUrgentThresh = 0x15,
	RxDMAPollPeriod = 0x16,
	LEDCtrl = 0x1a,
	ASICCtrl = 0x30,
	EEData = 0x34,
	EECtrl = 0x36,
	TxStartThresh = 0x3c,
	RxEarlyThresh = 0x3e,
	FlashAddr = 0x40,
	FlashData = 0x44,
	TxStatus = 0x46,
	TxFrameId = 0x47,
	DownCounter = 0x18,
	IntrClear = 0x4a,
	IntrEnable = 0x4c,
	IntrStatus = 0x4e,
	MACCtrl0 = 0x50,
	MACCtrl1 = 0x52,
	StationAddr = 0x54,
	MaxFrameSize = 0x5A,
	RxMode = 0x5c,
	MIICtrl = 0x5e,
	MulticastFilter0 = 0x60,
	MulticastFilter1 = 0x64,
	RxOctetsLow = 0x68,
	RxOctetsHigh = 0x6a,
	TxOctetsLow = 0x6c,
	TxOctetsHigh = 0x6e,
	TxFramesOK = 0x70,
	RxFramesOK = 0x72,
	StatsCarrierError = 0x74,
	StatsLateColl = 0x75,
	StatsMultiColl = 0x76,
	StatsOneColl = 0x77,
	StatsTxDefer = 0x78,
	RxMissed = 0x79,
	StatsTxXSDefer = 0x7a,
	StatsTxAbort = 0x7b,
	StatsBcastTx = 0x7c,
	StatsBcastRx = 0x7d,
	StatsMcastTx = 0x7e,
	StatsMcastRx = 0x7f,
	/* Aliased and bogus values! */
	RxStatus = 0x0c,
};
enum ASICCtrl_HiWord_bit {
	GlobalReset = 0x0001,
	RxReset = 0x0002,
	TxReset = 0x0004,
	DMAReset = 0x0008,
	FIFOReset = 0x0010,
	NetworkReset = 0x0020,
	HostReset = 0x0040,
	ResetBusy = 0x0400,
};

/* Bits in the interrupt status/mask registers. */
enum intr_status_bits {
	IntrSummary=0x0001, IntrPCIErr=0x0002, IntrMACCtrl=0x0008,
	IntrTxDone=0x0004, IntrRxDone=0x0010, IntrRxStart=0x0020,
	IntrDrvRqst=0x0040,
	StatsMax=0x0080, LinkChange=0x0100,
	IntrTxDMADone=0x0200, IntrRxDMADone=0x0400,
};

/* Bits in the RxMode register. */
enum rx_mode_bits {
	AcceptAllIPMulti=0x20, AcceptMultiHash=0x10, AcceptAll=0x08,
	AcceptBroadcast=0x04, AcceptMulticast=0x02, AcceptMyPhys=0x01,
};
/* Bits in MACCtrl. */
enum mac_ctrl0_bits {
	EnbFullDuplex=0x20, EnbRcvLargeFrame=0x40,
	EnbFlowCtrl=0x100, EnbPassRxCRC=0x200,
};
/*Bits in the PHY-CTRL registers*/
enum phy_ctrl_bits{
        PhyDuplexStatus=0x20,PhySpeedStatus=0x40,PhyLinkStatus=0x80,
};
enum mac_ctrl1_bits {
	StatsEnable=0x0020,	StatsDisable=0x0040, StatsEnabled=0x0080,
	TxEnable=0x0100, TxDisable=0x0200, TxEnabled=0x0400,
	RxEnable=0x0800, RxDisable=0x1000, RxEnabled=0x2000,
};

/* The Rx and Tx buffer descriptors. */
/* Note that using only 32 bit fields simplifies conversion to big-endian
   architectures. */
struct netdev_desc {
	u32 next_desc;
	u32 status;
	struct desc_frag { u32 addr, length; } frag[1];
};

/* Bits in netdev_desc.status */
enum desc_status_bits {
	DescOwn=0x8000,
	DescEndPacket=0x4000,
	DescEndRing=0x2000,
	LastFrag=0x80000000,
	DescIntrOnTx=0x8000,
	DescIntrOnDMADone=0x80000000,
	DisableAlign = 0x00000001,
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

#define PRIV_ALIGN	15 	/* Required alignment mask */
/* Use  __attribute__((aligned (L1_CACHE_BYTES)))  to maintain alignment
   within the structure. */
#define MII_CNT		4
struct netdev_private {
	/* Descriptor rings first for alignment. */
	struct netdev_desc *rx_ring;
	struct netdev_desc *tx_ring;
	struct sk_buff* rx_skbuff[RX_RING_SIZE];
	struct sk_buff* tx_skbuff[TX_RING_SIZE];
        dma_addr_t tx_ring_dma;
        dma_addr_t rx_ring_dma;
	struct net_device_stats stats;
	/* Frequently used values: keep some adjacent for cache effect. */
	spinlock_t lock;
	spinlock_t rx_lock;			/* Group with Tx control cache line. */
	int msg_enable;
	int chip_id;
	unsigned int cur_rx, dirty_rx;		/* Producer/consumer ring indices */
	unsigned int rx_buf_sz;			/* Based on MTU+slack. */
	struct netdev_desc *last_tx;		/* Last Tx descriptor used. */
	unsigned int cur_tx, dirty_tx;
	/* These values are keep track of the transceiver/media in use. */
	unsigned int flowctrl:1;
	unsigned int default_port:4;		/* Last dev->if_port value. */
	unsigned int an_enable:1;
	unsigned int speed;
	int budget;
	int cur_task;
	/* Multicast and receive mode. */
	spinlock_t mcastlock;			/* SMP lock multicast updates. */
	u16 mcast_filter[4];
	/* MII transceiver section. */
	struct mii_if_info mii_if;
	int mii_preamble_required;
	unsigned char phys[MII_CNT];		/* MII device addresses, only first one used. */
	struct pci_dev *pci_dev;
	void  *base;
	unsigned char pci_rev_id;
};

/* The station address location in the EEPROM. */
#define EEPROM_SA_OFFSET	0x10
#define DEFAULT_INTR (IntrRxDMADone | IntrPCIErr | \
			IntrDrvRqst | IntrTxDone | StatsMax | \
			LinkChange)

static int  change_mtu(struct net_device *dev, int new_mtu);
static int  eeprom_read(void  *ioaddr, int location);
static int  mdio_read(struct net_device *dev, int phy_id, int location);
static void mdio_write(struct net_device *dev, int phy_id, int location, int value);
static int  netdev_open(struct net_device *dev);
static void check_duplex(struct net_device *dev);
static void netdev_timer(unsigned long data);
static void init_ring(struct net_device *dev);
static int  start_tx(struct sk_buff *skb, struct net_device *dev);
static int reset_tx (struct net_device *dev);
static int ste_intr(void *);
static void rx_poll(unsigned long data);
static void tx_poll(unsigned long data);
static void refill_rx (struct net_device *dev);
static void netdev_error(struct net_device *dev, int intr_status);
static void netdev_error(struct net_device *dev, int intr_status);
static void set_rx_mode(struct net_device *dev);
static int __set_mac_addr(struct net_device *dev);
static struct net_device_stats *get_stats(struct net_device *dev);
static int netdev_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);
static int  netdev_close(struct net_device *dev);

void delay10us(void)
{
	 delay(10);
}

unsigned short outpw(unsigned long WR_ADDR,unsigned short WR_VAL)
{

        *( (volatile unsigned short *) WR_ADDR ) = WR_VAL ;
        return WR_VAL;
}

unsigned short inpw(unsigned long RD_ADDR)
{
        unsigned short RD_VAL;
        RD_VAL = *( (volatile unsigned short *) RD_ADDR ) ;
        return RD_VAL;
}

static void write_eeprom(unsigned long ioaddr_1, unsigned int eep_addr, unsigned short writedata)
{
   unsigned short v = 0;
   unsigned long uIOBase=ioaddr_1;
   int i = 0;

        v=outpw (uIOBase + EEPROMDATA, writedata );
        while ( (inpw(uIOBase + EEPROMCTRL) & EEP_BUSY ) &&(i<20000) )
        {
                i++;
        }

        v=outpw (uIOBase + EEPROMCTRL, 0xC0 );
        while ( (inpw(uIOBase + EEPROMCTRL) & EEP_BUSY ) &&(i<20000) )
        {
                i++;
        }

        v=outpw (uIOBase + EEPROMCTRL, EEP_WRITE | (eep_addr & 0xff) );
        while ( (inpw(uIOBase + EEPROMCTRL) & EEP_BUSY ) &&(i<20000) )
        {
                i++;
        }

        for( i = 0; i < 400; i++ )
        {
            delay10us();
        }

}

static unsigned short read_eeprom(unsigned long ioaddr_1, unsigned int eep_addr)
{

        unsigned int v = 0;
        int i = 10000;
        unsigned long uIOBase=ioaddr_1;

        v=outpw (uIOBase + EEPROMCTRL, EEP_READ | (eep_addr & 0xff));
        while (i-- > 0) {
                delay10us();
                if (!(inpw(uIOBase + EEPROMCTRL) & EEP_BUSY)) {
                        return inpw(uIOBase + EEPROMDATA);
                }
        }
        return 0;
}

static void sundance_reset(struct net_device *dev, unsigned long reset_cmd)
{
	struct netdev_private *np = netdev_priv(dev);
	void  *ioaddr = np->base + ASICCtrl;
	int countdown;

	/* ST201 documentation states ASICCtrl is a 32bit register */
	iowrite32 (reset_cmd | ioread32 (ioaddr), ioaddr);
	/* ST201 documentation states reset can take up to 1 ms */
	countdown = 10 + 1;
	while (ioread32 (ioaddr) & (ResetBusy << 16)) {
		if (--countdown == 0) {
			printk(KERN_WARNING "%s : reset not completed !!\n", dev->name);
			break;
		}
		udelay(100);
	}
}
static int  sundance_probe1 (struct net_device *dev,struct pci_dev *pdev,
				      const struct pci_device_id *ent)
{
	struct netdev_private *np;
	static int card_idx;
	int chip_idx = ent->driver_data;
	int irq;
	int i;
	void  *ioaddr;
	u16 mii_ctl;
	void *ring_space;
	dma_addr_t ring_dma;
#ifdef USE_IO_OPS
	int bar = 0;
#else
	int bar = 1;
#endif
	int phy, phy_idx = 0;


	_pci_conf_write(pdev->pa.pa_tag,4,7);
	ioaddr=0xbfd00000|(_pci_conf_read(pdev->pa.pa_tag,bar*4+0x10)&~3);
	printf("ioaddr=%x\n",ioaddr);

	if (!ioaddr)
		goto err_out_res;

	for (i = 0; i < 3; i++)
		((u16 *)dev->dev_addr)[i] =
			le16_to_cpu(eeprom_read(ioaddr, i + EEPROM_SA_OFFSET));
	memcpy(dev->perm_addr, dev->dev_addr, dev->addr_len);

	dev->base_addr = (unsigned long)ioaddr;
	dev->irq = irq;

	np = netdev_priv(dev);
	np->base = ioaddr;
	np->pci_dev = pdev;
	np->chip_id = chip_idx;
	np->msg_enable = (1 << debug) - 1;
	spin_lock_init(&np->lock);

	ring_space = pci_alloc_consistent(pdev, TX_TOTAL_SIZE, &ring_dma);
	if (!ring_space)
		goto err_out_cleardev;
	np->tx_ring = (struct netdev_desc *)ring_space;
	np->tx_ring_dma = ring_dma;

	ring_space = pci_alloc_consistent(pdev, RX_TOTAL_SIZE, &ring_dma);
	if (!ring_space)
		goto err_out_unmap_tx;
	np->rx_ring = (struct netdev_desc *)ring_space;
	np->rx_ring_dma = ring_dma;

	np->mii_if.dev = dev;
	np->mii_if.mdio_read = mdio_read;
	np->mii_if.mdio_write = mdio_write;
	np->mii_if.phy_id_mask = 0x1f;
	np->mii_if.reg_num_mask = 0x1f;

	/* The chip-specific entries in the device structure. */

	np->pci_rev_id=	_pci_conf_readn(pdev->pa.pa_tag,PCI_REVISION_ID,1);


	printk(KERN_INFO "%s: %s at %p, ",
		   dev->name, pci_id_tbl[chip_idx].name, ioaddr);
	for (i = 0; i < 5; i++)
			printk("%2.2x:", dev->dev_addr[i]);
	printk("%2.2x, IRQ %d.\n", dev->dev_addr[i], irq);

	np->phys[0] = 1;		/* Default setting */
	np->mii_preamble_required++;
	/*
	 * It seems some phys doesn't deal well with address 0 being accessed
	 * first, so leave address zero to the end of the loop (32 & 31).
	 */
	for (phy = 1; phy <= 32 && phy_idx < MII_CNT; phy++) {
		int phyx = phy & 0x1f;
		int mii_status = mdio_read(dev, phyx, MII_BMSR);
		if (mii_status != 0xffff  &&  mii_status != 0x0000) {
			np->phys[phy_idx++] = phyx;
			np->mii_if.advertising = mdio_read(dev, phyx, MII_ADVERTISE);
			if ((mii_status & 0x0040) == 0)
				np->mii_preamble_required++;
			printk(KERN_INFO "%s: MII PHY found at address %d, status "
				   "0x%4.4x advertising %4.4x.\n",
				   dev->name, phyx, mii_status, np->mii_if.advertising);
		}
	}
	np->mii_preamble_required--;

	if (phy_idx == 0) {
		printk(KERN_INFO "%s: No MII transceiver found, aborting.  ASIC status %x\n",
			   dev->name, ioread32(ioaddr + ASICCtrl));
		goto err_out_unregister;
	}

	np->mii_if.phy_id = np->phys[0];

	/* Parse override configuration */
	np->an_enable = 1;
	if (card_idx < MAX_UNITS) {
		if (media[card_idx] != NULL) {
			np->an_enable = 0;
			if (strcmp (media[card_idx], "100mbps_fd") == 0 ||
			    strcmp (media[card_idx], "4") == 0) {
				np->speed = 100;
				np->mii_if.full_duplex = 1;
			} else if (strcmp (media[card_idx], "100mbps_hd") == 0
				   || strcmp (media[card_idx], "3") == 0) {
				np->speed = 100;
				np->mii_if.full_duplex = 0;
			} else if (strcmp (media[card_idx], "10mbps_fd") == 0 ||
				   strcmp (media[card_idx], "2") == 0) {
				np->speed = 10;
				np->mii_if.full_duplex = 1;
			} else if (strcmp (media[card_idx], "10mbps_hd") == 0 ||
				   strcmp (media[card_idx], "1") == 0) {
				np->speed = 10;
				np->mii_if.full_duplex = 0;
			} else {
				np->an_enable = 1;
			}
		}
		if (flowctrl == 1)
			np->flowctrl = 1;
	}

	/* Fibre PHY? */
	if (ioread32 (ioaddr + ASICCtrl) & 0x80) {
		/* Default 100Mbps Full */
		if (np->an_enable) {
			np->speed = 100;
			np->mii_if.full_duplex = 1;
			np->an_enable = 0;
		}
	}
	/* Reset PHY */
	mdio_write (dev, np->phys[0], MII_BMCR, BMCR_RESET);
	mdelay (300);
	/* If flow control enabled, we need to advertise it.*/
	if (np->flowctrl)
		mdio_write (dev, np->phys[0], MII_ADVERTISE, np->mii_if.advertising | 0x0400);
	mdio_write (dev, np->phys[0], MII_BMCR, BMCR_ANENABLE|BMCR_ANRESTART);
	/* Force media type */
	if (!np->an_enable) {
		mii_ctl = 0;
		mii_ctl |= (np->speed == 100) ? BMCR_SPEED100 : 0;
		mii_ctl |= (np->mii_if.full_duplex) ? BMCR_FULLDPLX : 0;
		mdio_write (dev, np->phys[0], MII_BMCR, mii_ctl);
		printk (KERN_INFO "Override speed=%d, %s duplex\n",
			np->speed, np->mii_if.full_duplex ? "Full" : "Half");

	}

	/* Perhaps move the reset here? */
	/* Reset the chip to erase previous misconfiguration. */
		printk("ASIC Control is %x.\n", ioread32(ioaddr + ASICCtrl));
	iowrite16(0x00ff, ioaddr + ASICCtrl + 2);
		printk("ASIC Control is now %x.\n", ioread32(ioaddr + ASICCtrl));

	card_idx++;
	return 0;

err_out_unregister:
err_out_unmap_rx:
        pci_free_consistent(pdev, RX_TOTAL_SIZE, np->rx_ring, np->rx_ring_dma);
err_out_unmap_tx:
        pci_free_consistent(pdev, TX_TOTAL_SIZE, np->tx_ring, np->tx_ring_dma);
err_out_cleardev:
err_out_res:
err_out_netdev:
	return -ENODEV;
}

static int change_mtu(struct net_device *dev, int new_mtu)
{
	if ((new_mtu < 68) || (new_mtu > 8191)) /* Set by RxDMAFrameLen */
		return -EINVAL;
	dev->mtu = new_mtu;
	return 0;
}

#define eeprom_delay(ee_addr)	ioread16(ee_addr)
/* Read the EEPROM and MII Management Data I/O (MDIO) interfaces. */
static int  eeprom_read(void  *ioaddr, int location)
{
	int boguscnt = 10000;		/* Typical 1900 ticks. */
	iowrite16(0x0200 | (location & 0xff), ioaddr + EECtrl);
	printf("ioaddr=%x,ioaddr + EECtrl=%x\n",ioaddr,ioaddr + EECtrl);
	do {
	printf("%d\n",boguscnt);
		eeprom_delay(ioaddr + EECtrl);
	printf("%d\n",boguscnt);
		if (! (ioread16(ioaddr + EECtrl) & 0x8000)) {
			return ioread16(ioaddr + EEData);
		}
	} while (--boguscnt > 0);
	return 0;
}

/*  MII transceiver control section.
	Read and write the MII registers using software-generated serial
	MDIO protocol.  See the MII specifications or DP83840A data sheet
	for details.

	The maximum data clock rate is 2.5 Mhz.  The minimum timing is usually
	met by back-to-back 33Mhz PCI cycles. */
#define mdio_delay() ioread8(mdio_addr)

enum mii_reg_bits {
	MDIO_ShiftClk=0x0001, MDIO_Data=0x0002, MDIO_EnbOutput=0x0004,
};
#define MDIO_EnbIn  (0)
#define MDIO_WRITE0 (MDIO_EnbOutput)
#define MDIO_WRITE1 (MDIO_Data | MDIO_EnbOutput)

/* Generate the preamble required for initial synchronization and
   a few older transceivers. */
static void mdio_sync(void  *mdio_addr)
{
	int bits = 32;

	/* Establish sync by sending at least 32 logic ones. */
	while (--bits >= 0) {
		iowrite8(MDIO_WRITE1, mdio_addr);
		mdio_delay();
		iowrite8(MDIO_WRITE1 | MDIO_ShiftClk, mdio_addr);
		mdio_delay();
	}
}

static int mdio_read(struct net_device *dev, int phy_id, int location)
{
	struct netdev_private *np = netdev_priv(dev);
	void  *mdio_addr = np->base + MIICtrl;
	int mii_cmd = (0xf6 << 10) | (phy_id << 5) | location;
	int i, retval = 0;

	if (np->mii_preamble_required)
		mdio_sync(mdio_addr);

	/* Shift the read command bits out. */
	for (i = 15; i >= 0; i--) {
		int dataval = (mii_cmd & (1 << i)) ? MDIO_WRITE1 : MDIO_WRITE0;

		iowrite8(dataval, mdio_addr);
		mdio_delay();
		iowrite8(dataval | MDIO_ShiftClk, mdio_addr);
		mdio_delay();
	}
	/* Read the two transition, 16 data, and wire-idle bits. */
	for (i = 19; i > 0; i--) {
		iowrite8(MDIO_EnbIn, mdio_addr);
		mdio_delay();
		retval = (retval << 1) | ((ioread8(mdio_addr) & MDIO_Data) ? 1 : 0);
		iowrite8(MDIO_EnbIn | MDIO_ShiftClk, mdio_addr);
		mdio_delay();
	}
	return (retval>>1) & 0xffff;
}

static void mdio_write(struct net_device *dev, int phy_id, int location, int value)
{
	struct netdev_private *np = netdev_priv(dev);
	void  *mdio_addr = np->base + MIICtrl;
	int mii_cmd = (0x5002 << 16) | (phy_id << 23) | (location<<18) | value;
	int i;

	if (np->mii_preamble_required)
		mdio_sync(mdio_addr);

	/* Shift the command bits out. */
	for (i = 31; i >= 0; i--) {
		int dataval = (mii_cmd & (1 << i)) ? MDIO_WRITE1 : MDIO_WRITE0;

		iowrite8(dataval, mdio_addr);
		mdio_delay();
		iowrite8(dataval | MDIO_ShiftClk, mdio_addr);
		mdio_delay();
	}
	/* Clear out extra bits. */
	for (i = 2; i > 0; i--) {
		iowrite8(MDIO_EnbIn, mdio_addr);
		mdio_delay();
		iowrite8(MDIO_EnbIn | MDIO_ShiftClk, mdio_addr);
		mdio_delay();
	}
	return;
}

static int netdev_open(struct net_device *dev)
{
	struct netdev_private *np = netdev_priv(dev);
	void  *ioaddr = np->base;
	int i;

	/* Do we need to reset the chip??? */

	init_ring(dev);

	iowrite32(np->rx_ring_dma, ioaddr + RxListPtr);
	/* The Tx list pointer is written as packets are queued. */

	/* Initialize other registers. */
	__set_mac_addr(dev);
#if defined(CONFIG_VLAN_8021Q) || defined(CONFIG_VLAN_8021Q_MODULE)
	iowrite16(dev->mtu + 18, ioaddr + MaxFrameSize);
#else
	iowrite16(dev->mtu + 14, ioaddr + MaxFrameSize);
#endif
	if (dev->mtu > 2047)
		iowrite32(ioread32(ioaddr + ASICCtrl) | 0x0C, ioaddr + ASICCtrl);

	/* Configure the PCI bus bursts and FIFO thresholds. */

	spin_lock_init(&np->mcastlock);

	set_rx_mode(dev);
	iowrite16(0, ioaddr + IntrEnable);
	iowrite16(0, ioaddr + DownCounter);
	/* Set the chip to poll every N*320nsec. */
	iowrite8(100, ioaddr + RxDMAPollPeriod);
	iowrite8(127, ioaddr + TxDMAPollPeriod);
	/* Fix DFE-580TX packet drop issue */
	if (np->pci_rev_id >= 0x14)
		iowrite8(0x01, ioaddr + DebugCtrl1);
	netif_start_queue(dev);

	iowrite16 (StatsEnable | RxEnable | TxEnable, ioaddr + MACCtrl1);

		printk(KERN_DEBUG "%s: Done netdev_open(), status: Rx %x Tx %x "
			   "MAC Control %x, %4.4x %4.4x.\n",
			   dev->name, ioread32(ioaddr + RxStatus), ioread8(ioaddr + TxStatus),
			   ioread32(ioaddr + MACCtrl0),
			   ioread16(ioaddr + MACCtrl1), ioread16(ioaddr + MACCtrl0));

	timeout(netdev_timer,dev,3*hz);

	/* Enable interrupts by setting the interrupt mask. */
	iowrite16(DEFAULT_INTR, ioaddr + IntrEnable);

	return 0;
}

static void check_duplex(struct net_device *dev)
{
	struct netdev_private *np = netdev_priv(dev);
	void  *ioaddr = np->base;
	int mii_lpa = mdio_read(dev, np->phys[0], MII_LPA);
	int negotiated = mii_lpa & np->mii_if.advertising;
	int duplex;

	/* Force media */
	if (!np->an_enable || mii_lpa == 0xffff) {
		if (np->mii_if.full_duplex)
			iowrite16 (ioread16 (ioaddr + MACCtrl0) | EnbFullDuplex,
				ioaddr + MACCtrl0);
		return;
	}

	/* Autonegotiation */
	duplex = (negotiated & 0x0100) || (negotiated & 0x01C0) == 0x0040;
	if (np->mii_if.full_duplex != duplex) {
		np->mii_if.full_duplex = duplex;
			printk(KERN_INFO "%s: Setting %s-duplex based on MII #%d "
				   "negotiated capability %4.4x.\n", dev->name,
				   duplex ? "full" : "half", np->phys[0], negotiated);
		iowrite16(ioread16(ioaddr + MACCtrl0) | duplex ? 0x20 : 0, ioaddr + MACCtrl0);
	}
}

static void netdev_timer(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	struct netdev_private *np = netdev_priv(dev);
	void  *ioaddr = np->base;
	int next_tick = 10*hz;

	check_duplex(dev);
	timeout(netdev_timer,data,next_tick);
}

static void ste_watchdog( struct ifnet *ifp )
{
	struct net_device *dev = ifp->if_softc;
	struct netdev_private *np = netdev_priv(dev);
	void  *ioaddr = np->base;
	unsigned long flag;
	
	netif_stop_queue(dev);
	iowrite16(0, ioaddr + IntrEnable);
	printk(KERN_WARNING "%s: Transmit timed out, TxStatus %2.2x "
		   "TxFrameId %2.2x,"
		   " resetting...\n", dev->name, ioread8(ioaddr + TxStatus),
		   ioread8(ioaddr + TxFrameId));

	{
		int i;
		for (i=0; i<TX_RING_SIZE; i++) {
			printk(KERN_DEBUG "%02x %08llx %08x %08x(%02x) %08x %08x\n", i,
				(unsigned long long)(np->tx_ring_dma + i*sizeof(*np->tx_ring)),
				le32_to_cpu(np->tx_ring[i].next_desc),
				le32_to_cpu(np->tx_ring[i].status),
				(le32_to_cpu(np->tx_ring[i].status) >> 2) & 0xff,
				le32_to_cpu(np->tx_ring[i].frag[0].addr), 
				le32_to_cpu(np->tx_ring[i].frag[0].length));
		}
		printk(KERN_DEBUG "TxListPtr=%08x netif_queue_stopped=%d\n", 
			ioread32(np->base + TxListPtr), 
			netif_queue_stopped(dev));
		printk(KERN_DEBUG "cur_tx=%d(%02x) dirty_tx=%d(%02x)\n", 
			np->cur_tx, np->cur_tx % TX_RING_SIZE,
			np->dirty_tx, np->dirty_tx % TX_RING_SIZE);
		printk(KERN_DEBUG "cur_rx=%d dirty_rx=%d\n", np->cur_rx, np->dirty_rx);
		printk(KERN_DEBUG "cur_task=%d\n", np->cur_task);
	}
	spin_lock_irqsave(&np->lock, flag);

	/* Stop and restart the chip's Tx processes . */
	reset_tx(dev);
	spin_unlock_irqrestore(&np->lock, flag);


	dev->trans_start = jiffies;
	np->stats.tx_errors++;
	if (np->cur_tx - np->dirty_tx < TX_QUEUE_LEN - 4) {
		//netif_wake_queue(dev);
		ste_start(ifp);
	}
	iowrite16(DEFAULT_INTR, ioaddr + IntrEnable);
}


/* Initialize the Rx and Tx rings, along with various 'dev' bits. */
static void init_ring(struct net_device *dev)
{
	struct netdev_private *np = netdev_priv(dev);
	int i;

	np->cur_rx = np->cur_tx = 0;
	np->dirty_rx = np->dirty_tx = 0;
	np->cur_task = 0;

	np->rx_buf_sz = (dev->mtu <= 1520 ? PKT_BUF_SZ : dev->mtu + 16);

	/* Initialize all Rx descriptors. */
	for (i = 0; i < RX_RING_SIZE; i++) {
		np->rx_ring[i].next_desc = cpu_to_le32(np->rx_ring_dma +
			((i+1)%RX_RING_SIZE)*sizeof(*np->rx_ring));
		np->rx_ring[i].status = 0;
		np->rx_ring[i].frag[0].length = 0;
		np->rx_skbuff[i] = NULL;
	}

	/* Fill in the Rx buffers.  Handle allocation failure gracefully. */
	for (i = 0; i < RX_RING_SIZE; i++) {
		struct sk_buff *skb = dev_alloc_skb(np->rx_buf_sz);
		np->rx_skbuff[i] = skb;
		if (skb == NULL)
			break;
		skb->dev = dev;		/* Mark as being used by this device. */
		skb_reserve(skb, 2);	/* 16 byte align the IP header. */
		np->rx_ring[i].frag[0].addr = cpu_to_le32(
			pci_map_single(np->pci_dev, skb->data, np->rx_buf_sz,
				PCI_DMA_FROMDEVICE));
		np->rx_ring[i].frag[0].length = cpu_to_le32(np->rx_buf_sz | LastFrag);
	}
	np->dirty_rx = (unsigned int)(i - RX_RING_SIZE);

	for (i = 0; i < TX_RING_SIZE; i++) {
		np->tx_skbuff[i] = NULL;
		np->tx_ring[i].status = 0;
	}
	return;
}

static void tx_poll (unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	struct netdev_private *np = netdev_priv(dev);
	unsigned head = np->cur_task % TX_RING_SIZE;
	struct netdev_desc *txdesc = 
		&np->tx_ring[(np->cur_tx - 1) % TX_RING_SIZE];
	
	/* Chain the next pointer */
	for (; np->cur_tx - np->cur_task > 0; np->cur_task++) {
		int entry = np->cur_task % TX_RING_SIZE;
		txdesc = &np->tx_ring[entry];
		if (np->last_tx) {
			np->last_tx->next_desc = cpu_to_le32(np->tx_ring_dma +
				entry*sizeof(struct netdev_desc));
		}
		np->last_tx = txdesc;
	}
	/* Indicate the latest descriptor of tx ring */
	txdesc->status |= cpu_to_le32(DescIntrOnTx);

	if (ioread32 (np->base + TxListPtr) == 0)
		iowrite32 (np->tx_ring_dma + head * sizeof(struct netdev_desc),
			np->base + TxListPtr);
	return;
}

static int
start_tx (struct sk_buff *skb, struct net_device *dev)
{
	struct netdev_private *np = netdev_priv(dev);
	struct netdev_desc *txdesc;
	unsigned entry;

	/* Calculate the next Tx descriptor entry. */
	entry = np->cur_tx % TX_RING_SIZE;
	np->tx_skbuff[entry] = skb;
	txdesc = &np->tx_ring[entry];

	txdesc->next_desc = 0;
	txdesc->status = cpu_to_le32 ((entry << 2) | DisableAlign);
	txdesc->frag[0].addr = cpu_to_le32 (pci_map_single (np->pci_dev, skb->data,
							skb->len,
							PCI_DMA_TODEVICE));
	txdesc->frag[0].length = cpu_to_le32 (skb->len | LastFrag);

	/* Increment cur_tx before tasklet_schedule() */
	np->cur_tx++;
	/* Schedule a tx_poll() task */
	tx_poll(dev);

	/* On some architectures: explicitly flush cache lines here. */
	if (np->cur_tx - np->dirty_tx < TX_QUEUE_LEN - 1
			&& !netif_queue_stopped(dev)) {
		/* do nothing */
	} else {
		netif_stop_queue (dev);
	}
	dev->trans_start = jiffies;
	return 0;
}

/* Reset hardware tx and free all of tx buffers */
static int
reset_tx (struct net_device *dev)
{
	struct netdev_private *np = netdev_priv(dev);
	void  *ioaddr = np->base;
	struct sk_buff *skb;
	int i;
	
	/* Reset tx logic, TxListPtr will be cleaned */
	iowrite16 (TxDisable, ioaddr + MACCtrl1);
	iowrite16 (TxReset | DMAReset | FIFOReset | NetworkReset,
			ioaddr + ASICCtrl + 2);
	for (i=50; i > 0; i--) {
		if ((ioread16(ioaddr + ASICCtrl + 2) & ResetBusy) == 0)
			break;
		mdelay(1);
	}
	/* free all tx skbuff */
	for (i = 0; i < TX_RING_SIZE; i++) {
		skb = np->tx_skbuff[i];
		if (skb) {
			pci_unmap_single(np->pci_dev, 
				np->tx_ring[i].frag[0].addr, skb->len,
				PCI_DMA_TODEVICE);
				dev_kfree_skb (skb);
			np->tx_skbuff[i] = NULL;
			np->stats.tx_dropped++;
		}
	}
	np->cur_tx = np->dirty_tx = 0;
	np->cur_task = 0;
	iowrite16 (StatsEnable | RxEnable | TxEnable, ioaddr + MACCtrl1);
	return 0;
}

/* The interrupt handler cleans up after the Tx thread, 
   and schedule a Rx thread work */
static int sundance_intr(void *dev_instance)
{
	struct net_device *dev = (struct net_device *)dev_instance;
	struct netdev_private *np = netdev_priv(dev);
	void  *ioaddr = np->base;
	int hw_frame_id;
	int tx_cnt;
	int tx_status;
	int handled = 0;


	do {
		int intr_status = ioread16(ioaddr + IntrStatus);
		iowrite16(intr_status, ioaddr + IntrStatus);


		if (!(intr_status & DEFAULT_INTR))
			break;

		handled = 1;

		if (intr_status & (IntrRxDMADone)) {
			iowrite16(DEFAULT_INTR & ~(IntrRxDone|IntrRxDMADone),
					ioaddr + IntrEnable);
			if (np->budget < 0)
				np->budget = RX_BUDGET;
			//tasklet_schedule(&np->rx_tasklet);
			rx_poll(dev);
		}
		if (intr_status & (IntrTxDone | IntrDrvRqst)) {
			tx_status = ioread16 (ioaddr + TxStatus);
			for (tx_cnt=32; tx_status & 0x80; --tx_cnt) {
				if (tx_status & 0x1e) {
					np->stats.tx_errors++;
					if (tx_status & 0x10)
						np->stats.tx_fifo_errors++;
					if (tx_status & 0x08)
						np->stats.collisions++;
					if (tx_status & 0x04)
						np->stats.tx_fifo_errors++;
					if (tx_status & 0x02)
						np->stats.tx_window_errors++;
					/*
					** This reset has been verified on
					** DFE-580TX boards ! phdm@macqel.be.
					*/
					if (tx_status & 0x10) {	/* TxUnderrun */
						unsigned short txthreshold;

						txthreshold = ioread16 (ioaddr + TxStartThresh);
						/* Restart Tx FIFO and transmitter */
						sundance_reset(dev, (NetworkReset|FIFOReset|TxReset) << 16);
						iowrite16 (txthreshold, ioaddr + TxStartThresh);
						/* No need to reset the Tx pointer here */
					}
					/* Restart the Tx. */
					iowrite16 (TxEnable, ioaddr + MACCtrl1);
				}
				/* Yup, this is a documentation bug.  It cost me *hours*. */
				iowrite16 (0, ioaddr + TxStatus);
				if (tx_cnt < 0) {
					iowrite32(5000, ioaddr + DownCounter);
					break;
				}
				tx_status = ioread16 (ioaddr + TxStatus);
			}
			hw_frame_id = (tx_status >> 8) & 0xff;
		} else 	{
			hw_frame_id = ioread8(ioaddr + TxFrameId);
		}
			
		if (np->pci_rev_id >= 0x14) {	
			spin_lock(&np->lock);
			for (; np->cur_tx - np->dirty_tx > 0; np->dirty_tx++) {
				int entry = np->dirty_tx % TX_RING_SIZE;
				struct sk_buff *skb;
				int sw_frame_id;
				sw_frame_id = (le32_to_cpu(
					np->tx_ring[entry].status) >> 2) & 0xff;
				if (sw_frame_id == hw_frame_id &&
					!(le32_to_cpu(np->tx_ring[entry].status)
					& 0x00010000))
						break;
				if (sw_frame_id == (hw_frame_id + 1) % 
					TX_RING_SIZE)
						break;
				skb = np->tx_skbuff[entry];
				/* Free the original skb. */
				pci_unmap_single(np->pci_dev,
					np->tx_ring[entry].frag[0].addr,
					skb->len, PCI_DMA_TODEVICE);
				dev_kfree_skb_irq (np->tx_skbuff[entry]);
				np->tx_skbuff[entry] = NULL;
				np->tx_ring[entry].frag[0].addr = 0;
				np->tx_ring[entry].frag[0].length = 0;
			}
			spin_unlock(&np->lock);
		} else {
			spin_lock(&np->lock);
			for (; np->cur_tx - np->dirty_tx > 0; np->dirty_tx++) {
				int entry = np->dirty_tx % TX_RING_SIZE;
				struct sk_buff *skb;
				if (!(le32_to_cpu(np->tx_ring[entry].status) 
							& 0x00010000))
					break;
				skb = np->tx_skbuff[entry];
				/* Free the original skb. */
				pci_unmap_single(np->pci_dev,
					np->tx_ring[entry].frag[0].addr,
					skb->len, PCI_DMA_TODEVICE);
				dev_kfree_skb_irq (np->tx_skbuff[entry]);
				np->tx_skbuff[entry] = NULL;
				np->tx_ring[entry].frag[0].addr = 0;
				np->tx_ring[entry].frag[0].length = 0;
			}
			spin_unlock(&np->lock);
		}
		
		if (netif_queue_stopped(dev) &&
			np->cur_tx - np->dirty_tx < TX_QUEUE_LEN - 4) {
			/* The ring is no longer full, clear busy flag. */
			netif_wake_queue (dev);
		}
		/* Abnormal error summary/uncommon events handlers. */
		if (intr_status & (IntrPCIErr | LinkChange | StatsMax))
			netdev_error(dev, intr_status);
	} while (0);
	return (handled);
}

static void rx_poll(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	struct netdev_private *np = netdev_priv(dev);
	int entry = np->cur_rx % RX_RING_SIZE;
	int boguscnt = np->budget;
	void  *ioaddr = np->base;
	int received = 0;

	/* If EOP is set on the next entry, it's a new packet. Send it up. */
	while (1) {
		struct netdev_desc *desc = &(np->rx_ring[entry]);
		u32 frame_status = le32_to_cpu(desc->status);
		int pkt_len;

		if (--boguscnt < 0) {
			goto not_done;
		}
		if (!(frame_status & DescOwn))
			break;
		pkt_len = frame_status & 0x1fff;	/* Chip omits the CRC. */
		if (frame_status & 0x001f4000) {
			/* There was a error. */
				printk(KERN_DEBUG "  netdev_rx() Rx error was %8.8x.\n",
					   frame_status);
			np->stats.rx_errors++;
			if (frame_status & 0x00100000) np->stats.rx_length_errors++;
			if (frame_status & 0x00010000) np->stats.rx_fifo_errors++;
			if (frame_status & 0x00060000) np->stats.rx_frame_errors++;
			if (frame_status & 0x00080000) np->stats.rx_crc_errors++;
			if (frame_status & 0x00100000) {
				printk(KERN_WARNING "%s: Oversized Ethernet frame,"
					   " status %8.8x.\n",
					   dev->name, frame_status);
			}
		} else {
			struct sk_buff *skb;
			/* Check if the packet is long enough to accept without copying
			   to a minimally-sized skbuff. */
			if (pkt_len < rx_copybreak
				&& (skb = dev_alloc_skb(pkt_len + 4)) != NULL) {
				skb->dev = dev;
				skb_reserve(skb, 2);	/* 16 byte align the IP header */
				pci_dma_sync_single_for_cpu(np->pci_dev,
							    desc->frag[0].addr,
							    np->rx_buf_sz,
							    PCI_DMA_FROMDEVICE);

				memcpy(skb->data, np->rx_skbuff[entry]->data, pkt_len);
				pci_dma_sync_single_for_device(np->pci_dev,
							       desc->frag[0].addr,
							       np->rx_buf_sz,
							       PCI_DMA_FROMDEVICE);
				skb_put(skb, pkt_len);
			} else {
				pci_unmap_single(np->pci_dev,
					desc->frag[0].addr,
					np->rx_buf_sz,
					PCI_DMA_FROMDEVICE);
				skb_put(skb = np->rx_skbuff[entry], pkt_len);
				np->rx_skbuff[entry] = NULL;
			}
			//skb->protocol = eth_type_trans(skb, dev);
			/* Note: checksum -> skb->ip_summed = CHECKSUM_UNNECESSARY; */
			netif_rx(skb);
			dev->last_rx = jiffies;
		}
		entry = (entry + 1) % RX_RING_SIZE;
		received++;
	}
	np->cur_rx = entry;
	refill_rx (dev);
	np->budget -= received;
	iowrite16(DEFAULT_INTR, ioaddr + IntrEnable);
	return;

not_done:
	np->cur_rx = entry;
	refill_rx (dev);
	if (!received)
		received = 1;
	np->budget -= received;
	if (np->budget <= 0)
		np->budget = RX_BUDGET;
//	tasklet_schedule(&np->rx_tasklet);
	return;
}

static void refill_rx (struct net_device *dev)
{
	struct netdev_private *np = netdev_priv(dev);
	int entry;
	int cnt = 0;

	/* Refill the Rx ring buffers. */
	for (;(np->cur_rx - np->dirty_rx + RX_RING_SIZE) % RX_RING_SIZE > 0;
		np->dirty_rx = (np->dirty_rx + 1) % RX_RING_SIZE) {
		struct sk_buff *skb;
		entry = np->dirty_rx % RX_RING_SIZE;
		if (np->rx_skbuff[entry] == NULL) {
			skb = dev_alloc_skb(np->rx_buf_sz);
			np->rx_skbuff[entry] = skb;
			if (skb == NULL)
				break;		/* Better luck next round. */
			skb->dev = dev;		/* Mark as being used by this device. */
			skb_reserve(skb, 2);	/* Align IP on 16 byte boundaries */
			np->rx_ring[entry].frag[0].addr = cpu_to_le32(
				pci_map_single(np->pci_dev, skb->data,
					np->rx_buf_sz, PCI_DMA_FROMDEVICE));
		}
		/* Perhaps we need not reset this field. */
		np->rx_ring[entry].frag[0].length =
			cpu_to_le32(np->rx_buf_sz | LastFrag);
		np->rx_ring[entry].status = 0;
		cnt++;
	}
	return;
}
static void netdev_error(struct net_device *dev, int intr_status)
{
	struct netdev_private *np = netdev_priv(dev);
	void  *ioaddr = np->base;
	u16 mii_ctl, mii_advertise, mii_lpa;
	int speed;

	if (intr_status & LinkChange) {
		if (np->an_enable) {
			mii_advertise = mdio_read (dev, np->phys[0], MII_ADVERTISE);
			mii_lpa= mdio_read (dev, np->phys[0], MII_LPA);
			mii_advertise &= mii_lpa;
			printk (KERN_INFO "%s: Link changed: ", dev->name);
			if (mii_advertise & ADVERTISE_100FULL) {
				np->speed = 100;
				printk ("100Mbps, full duplex\n");
			} else if (mii_advertise & ADVERTISE_100HALF) {
				np->speed = 100;
				printk ("100Mbps, half duplex\n");
			} else if (mii_advertise & ADVERTISE_10FULL) {
				np->speed = 10;
				printk ("10Mbps, full duplex\n");
			} else if (mii_advertise & ADVERTISE_10HALF) {
				np->speed = 10;
				printk ("10Mbps, half duplex\n");
			} else
				printk ("\n");

		} else {
			mii_ctl = mdio_read (dev, np->phys[0], MII_BMCR);
			speed = (mii_ctl & BMCR_SPEED100) ? 100 : 10;
			np->speed = speed;
			printk (KERN_INFO "%s: Link changed: %dMbps ,",
				dev->name, speed);
			printk ("%s duplex.\n", (mii_ctl & BMCR_FULLDPLX) ?
				"full" : "half");
		}
		check_duplex (dev);
		if (np->flowctrl && np->mii_if.full_duplex) {
			iowrite16(ioread16(ioaddr + MulticastFilter1+2) | 0x0200,
				ioaddr + MulticastFilter1+2);
			iowrite16(ioread16(ioaddr + MACCtrl0) | EnbFlowCtrl,
				ioaddr + MACCtrl0);
		}
	}
	if (intr_status & StatsMax) {
		get_stats(dev);
	}
	if (intr_status & IntrPCIErr) {
		printk(KERN_ERR "%s: Something Wicked happened! %4.4x.\n",
			   dev->name, intr_status);
		/* We must do a global reset of DMA to continue. */
	}
}

static struct net_device_stats *get_stats(struct net_device *dev)
{
	struct netdev_private *np = netdev_priv(dev);
	void  *ioaddr = np->base;
	int i;

	/* We should lock this segment of code for SMP eventually, although
	   the vulnerability window is very small and statistics are
	   non-critical. */
	/* The chip only need report frame silently dropped. */
	np->stats.rx_missed_errors	+= ioread8(ioaddr + RxMissed);
	np->stats.tx_packets += ioread16(ioaddr + TxFramesOK);
	np->stats.rx_packets += ioread16(ioaddr + RxFramesOK);
	np->stats.collisions += ioread8(ioaddr + StatsLateColl);
	np->stats.collisions += ioread8(ioaddr + StatsMultiColl);
	np->stats.collisions += ioread8(ioaddr + StatsOneColl);
	np->stats.tx_carrier_errors += ioread8(ioaddr + StatsCarrierError);
	ioread8(ioaddr + StatsTxDefer);
	for (i = StatsTxDefer; i <= StatsMcastRx; i++)
		ioread8(ioaddr + i);
	np->stats.tx_bytes += ioread16(ioaddr + TxOctetsLow);
	np->stats.tx_bytes += ioread16(ioaddr + TxOctetsHigh) << 16;
	np->stats.rx_bytes += ioread16(ioaddr + RxOctetsLow);
	np->stats.rx_bytes += ioread16(ioaddr + RxOctetsHigh) << 16;

	return &np->stats;
}

static void set_rx_mode(struct net_device *dev)
{
	struct netdev_private *np = netdev_priv(dev);
	void  *ioaddr = np->base;
	u16 mc_filter[4];			/* Multicast hash filter */
	u32 rx_mode;
	int i;

		/* Unconditionally log net taps. */
		printk(KERN_NOTICE "%s: Promiscuous mode enabled.\n", dev->name);
		memset(mc_filter, 0xff, sizeof(mc_filter));
		rx_mode = AcceptBroadcast | AcceptMulticast | AcceptAll | AcceptMyPhys;
	
	if (np->mii_if.full_duplex && np->flowctrl)
		mc_filter[3] |= 0x0200;

	for (i = 0; i < 4; i++)
		iowrite16(mc_filter[i], ioaddr + MulticastFilter0 + i*2);
	iowrite8(rx_mode, ioaddr + RxMode);
}

static int __set_mac_addr(struct net_device *dev)
{
	struct netdev_private *np = netdev_priv(dev);
	u16 addr16;

	addr16 = (dev->dev_addr[0] | (dev->dev_addr[1] << 8));
	iowrite16(addr16, np->base + StationAddr);
	addr16 = (dev->dev_addr[2] | (dev->dev_addr[3] << 8));
	iowrite16(addr16, np->base + StationAddr+2);
	addr16 = (dev->dev_addr[4] | (dev->dev_addr[5] << 8));
	iowrite16(addr16, np->base + StationAddr+4);
	return 0;
}


static int netdev_close(struct net_device *dev)
{
	struct netdev_private *np = netdev_priv(dev);
	void *ioaddr = np->base;
	struct sk_buff *skb;
	int i;

	netif_stop_queue(dev);


	/* Disable interrupts by clearing the interrupt mask. */
	iowrite16(0x0000, ioaddr + IntrEnable);

	/* Stop the chip's Tx and Rx processes. */
	iowrite16(TxDisable | RxDisable | StatsDisable, ioaddr + MACCtrl1);

	/* Free all the skbuffs in the Rx queue. */
	for (i = 0; i < RX_RING_SIZE; i++) {
		np->rx_ring[i].status = 0;
		np->rx_ring[i].frag[0].addr = 0xBADF00D0; /* An invalid address. */
		skb = np->rx_skbuff[i];
		if (skb) {
			pci_unmap_single(np->pci_dev,
				np->rx_ring[i].frag[0].addr, np->rx_buf_sz,
				PCI_DMA_FROMDEVICE);
			dev_kfree_skb(skb);
			np->rx_skbuff[i] = NULL;
		}
	}
	for (i = 0; i < TX_RING_SIZE; i++) {
		skb = np->tx_skbuff[i];
		if (skb) {
			pci_unmap_single(np->pci_dev,
				np->tx_ring[i].frag[0].addr, skb->len,
				PCI_DMA_TODEVICE);
			dev_kfree_skb(skb);
			np->tx_skbuff[i] = NULL;
		}
	}

	return 0;
}

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
	netdev->priv=kmalloc(sizeof(struct netdev_private));//&netdev->em;
	memset(netdev->priv,0,sizeof(struct netdev_private));
	netdev->addr_len=6;
	netdev->pcidev.irq=irq++;
}

static int ste_ether_ioctl(struct ifnet *ifp,FXP_IOCTLCMD_TYPE cmd,caddr_t data);

static struct pci_device_id *ste_pci_id=0;
/*
 * Check if a device is an 82557.
 */
static void ste_start(struct ifnet *ifp);
static int
ste_match(parent, match, aux)
	struct device *parent;
#if defined(__BROKEN_INDIRECT_CONFIG) || defined(__OpenBSD__)
	void *match;
#else
	struct cfdata *match;
#endif
	void *aux;
{
	struct pci_attach_args *pa = aux;
ste_pci_id=pci_match_device(sundance_pci_tbl,pa);
return ste_pci_id?1:0;
}

static void
ste_shutdown(sc)
        void *sc;
{
}


extern char activeif_name[];
static int ste_intr(void *data)
{
struct net_device *netdev = data;
int irq=netdev->irq;
struct ifnet *ifp = &netdev->arpcom.ac_if;
	if(ifp->if_flags & IFF_RUNNING)
	{
			sundance_intr(data);
		   if (ifp->if_snd.ifq_head != NULL)
		   ste_start(ifp);
	return 1;
	}
	return 0;
}

struct net_device *mynic_ste;
static void
ste_attach(parent, self, aux)
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

	mynic_ste = sc;

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
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, ste_intr, sc, self->dv_xname);
#else
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, ste_intr, sc);
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
	if (sundance_probe1(sc,&sc->pcidev,ste_pci_id)) {
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
	ifp->if_ioctl = ste_ether_ioctl;
	ifp->if_start = ste_start;
	ifp->if_watchdog = ste_watchdog;

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
	shutdownhook_establish(ste_shutdown, sc);

#ifndef PMON
	/*
	 * Add suspend hook, for similiar reasons..
	 */
	powerhook_establish(ste_power, sc);
#endif
}


/*
 * Start packet transmission on the interface.
 */



static void ste_start(struct ifnet *ifp)
{
	struct net_device *sc = ifp->if_softc;
	struct mbuf *mb_head;		
	struct sk_buff *skb;

	while(ifp->if_snd.ifq_head != NULL ){
		
		IF_DEQUEUE(&ifp->if_snd, mb_head);
		
		skb=dev_alloc_skb(mb_head->m_pkthdr.len);
		m_copydata(mb_head, 0, mb_head->m_pkthdr.len, skb->data);
		skb->len=mb_head->m_pkthdr.len;
		start_tx(skb,sc);

		m_freem(mb_head);
		wbflush();
	} 
}

static int
ste_init(struct net_device *netdev)
{
    struct ifnet *ifp = &netdev->arpcom.ac_if;
	int stat=0;
    ifp->if_flags |= IFF_RUNNING;
	if(!netdev->opencount){ stat=netdev_open(netdev);netdev->opencount++;}
	return stat;
}

static int
ste_stop(struct net_device *netdev)
{
    struct ifnet *ifp = &netdev->arpcom.ac_if;
	ifp->if_timer = 0;
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	if(netdev->opencount){netdev_close(netdev);netdev->opencount--;}
	return 0;
}

static int
ste_ether_ioctl(ifp, cmd, data)
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
			error = ste_init(sc);
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
			error = ste_init(sc);
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
			error = ste_init(sc);
			if(error <0 )
				return(error);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				ste_stop(sc);
		}
		break;

        case SIOCETHTOOL:
        {
        long *p=data;
        mynic_ste = sc;
        cmd_setmac_ste(p[0],p[1]);
        }
        break;
       case SIOCRDEEPROM:
                {
                long *p=data;
                mynic_ste = sc;
                cmd_reprom_ste(p[0],p[1]);
                }
                break;
       case SIOCWREEPROM:
                {
                long *p=data;
                mynic_ste = sc;
                cmd_wrprom_ste(p[0],p[1]);
                }
                break;
	default:
		error = EINVAL;
	}

	splx(s);
	return (error);
}

struct cfattach ste_ca = {
	sizeof(struct net_device), ste_match, ste_attach
};

struct cfdriver ste_cd = {
	NULL, "ste", DV_IFNET
};
#if 1
#include <pmon.h>
int cmd_ifm_ste(int ac, char *av[])
{
        int speed100=0, fullduplex=0, mii_ctrl = 0x0;
        struct netdev_private *np = netdev_priv(mynic_ste);
        struct net_device *nic_ifm = mynic_ste;

        if(nic_ifm  == NULL){
                printf("IP100A interface not initialized\n");
                return 0;
        }

        if(ac !=1 && ac!=2 && ac!=3){
                printf("usage: ifm_ip100a [100|10|auto]  [full|half]\n");
                return 0;
        }
        if(ac == 1){
                speed100 = RTL_R8(nic_ifm ,  MIICtrl) & PhySpeedStatus ;
                fullduplex = RTL_R8(nic_ifm ,  MIICtrl) & PhyDuplexStatus ;
                printf(" %sMbps %s-DUPLEX.\n", speed100 ? "100" : "10",
                                                fullduplex ? "FULL" : "HALF");
                return 0;
        }

        if(strcmp("100", av[1]) == 0){
                mii_ctrl = 0;
                 mii_ctrl |= BMCR_SPEED100;
                if(strcmp("full", av[2]) == 0)
                        mii_ctrl |= BMCR_FULLDPLX ;
                else
                        mii_ctrl &= ~BMCR_FULLDPLX;
                printf("mii_ctrl_100 or : 0x%x\n",mii_ctrl);
                mdio_write (nic_ifm , np->phys[0] , MII_BMCR, mii_ctrl);
                printf("mii_ctrl_100 write : 0x%x\n",mii_ctrl);
mdelay(10);
                mii_ctrl= mdio_read(nic_ifm, np->phys[0], MII_BMCR);
                printf("mii_ctrl_100 read status : 0x%x\n",mii_ctrl);

        } else if(strcmp("10", av[1]) ==0){
                mii_ctrl = 0x0;
                mii_ctrl &= ~BMCR_SPEED100 ;
                if(strcmp("full", av[2]) == 0)
                        mii_ctrl |= BMCR_FULLDPLX ;
                else
                        mii_ctrl &= ~BMCR_FULLDPLX;
                printf("mii_ctrl_10 and : 0x%x\n",mii_ctrl);
                mdio_write (nic_ifm , np->phys[0] , MII_BMCR, mii_ctrl);
                printf("mii_ctrl_10 write : 0x%x\n",mii_ctrl);
mdelay(10);
                mii_ctrl= mdio_read(nic_ifm, np->phys[0], MII_BMCR);
                printf("mii_ctrl_10 read status : 0x%x\n",mii_ctrl);
        } else if(strcmp("auto", av[1])==0){
                mii_ctrl = 0x0;
                mii_ctrl |= BMCR_ANENABLE|BMCR_ANRESTART ;
                mdio_write (nic_ifm, np->phys[0] , MII_BMCR, mii_ctrl);
                printf("mii_ctrl_auto write : 0x%x\n",mii_ctrl);
mdelay(10);
                mii_ctrl= mdio_read(nic_ifm, np->phys[0], MII_BMCR);
                printf("mii_ctrl_auto read status : 0x%x\n",mii_ctrl);
        }
         else{
                printf("usage: ifm_ip100a [100|10|auto|full|half]\n");
        }
        return 0;
}

static long long ste_read_mac(struct net_device *nic)
{
        int i;
        long long mac_tmp = 0;
        unsigned short u16tmp;
        struct netdev_private *np = netdev_priv(mynic_ste);
        struct net_device *nic = mynic_ste;
        void  *ioaddr = np->base;

        for (i = 0; i < 3; i++) {
#if 1
//#ifndef EPLC46
                u16tmp = read_eeprom( ioaddr, EEPROM_SA_OFFSET + i);
#else
                u16tmp = read_eeprom(nic, ioaddr, (EEPROM_SA_OFFSET + i*2));
                u16tmp = u16tmp | (read_eeprom(nic, ioaddr, (EEPROM_SA_OFFSET + i*2 +1)) << 8);
#endif
                mac_tmp <<= 16;
                mac_tmp |= ((u16tmp & 0xff) << 8) | ((u16tmp >> 8) & 0xff);

                printf("ip100a_read_mac 1 : 0x%4x \n",mac_tmp);
        }
                printf("ip100a_read_mac all : 0x%12x \n",mac_tmp);
        return mac_tmp;
}

int cmd_setmac_ste(int ac, char *av[])
{
        int i;
        unsigned short val = 0, v;
	 struct netdev_private *np = netdev_priv(mynic_ste);
        struct net_device *nic = mynic_ste;
        void  *ioaddr = np->base;        
        if(nic == NULL){
                printf("IP100A interface not initialized\n");
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
        macaddr=ste_read_mac(nic);
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

#if 1
//#ifndef EPLC46
                iowrite16(val, ioaddr + StationAddr + 2*i);  //zgj
                write_eeprom(ioaddr, EEPROM_SA_OFFSET + i, val);
#else
                write_eeprom8(ioaddr, (EEPROM_SA_OFFSET + i*2 ), val & 0xff);
                write_eeprom8(ioaddr, (EEPROM_SA_OFFSET + i*2 + 1), (val >> 8) & 0xff);
#endif

        }
#endif
        return 0;
}

int cmd_reprom_ste(int ac, char *av)
{
        int i;
        unsigned short data;
        struct netdev_private *np = netdev_priv(mynic_ste);
        struct net_device *nic = mynic_ste;
        void  *ioaddr = np->base;

	 printf("dump eprom:\n");

        for(i=0; i< 64;){
#if 1
//#ifndef EPLC46
                data = read_eeprom(ioaddr, i);
#else
                data = read_eeprom(mynic_ste, ioaddr, 2*i);
                data = data | (read_eeprom(mynic_ste, ioaddr, 2*i+1)) << 8;
#endif
                printf("%04x ", data);
                ++i;
                if( i%8 == 0 )
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

int cmd_wrprom_ste(int ac, char *av)
{
        int i=0;
        struct netdev_private *np = netdev_priv(mynic_ste);
        struct net_device *nic = mynic_ste;
        void  *ioaddr = np->base;
        unsigned long clocks_num=0;
        unsigned char tmp[4];
        unsigned short eeprom_data;
                unsigned short rom[] = {
                        0x2af8 ,0x8061 ,0x13f0 ,0x0201 ,0x0000 ,0x0000 ,0x0000 ,0x0000,
                        0x0000 ,0x0000 ,0x8003 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000,
                        0x0500 ,0x0000 ,0x0400 ,0x0064 ,0x0000 ,0x0000 ,0x0000 ,0x0000,
                        0x3400 ,0x0100 ,0x3702 ,0x0300 ,0x803a ,0x3f04 ,0x0303 ,0x0103,
                        0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000,
                        0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000,
                        0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000,
                        0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000 ,0x0000};
        printf("Now beginningwrite whole eprom\n");

#if 1
                clocks_num =CPU_GetCOUNT();
                mysrand(clocks_num);
                for( i = 0; i < 4;i++ )
                {
                        tmp[i]=myrand()%256;
                        printf( " tmp[%d]=02x%x\n", i,tmp[i]);
                }
                eeprom_data =tmp[1] |( tmp[0]<<8);
                printf("eeprom_data [17] = 0x%4x\n",eeprom_data);
		rom[17] = eeprom_data;
                eeprom_data =tmp[3] |( tmp[2]<<8);
                printf("eeprom_data [18] = 0x%4x\n",eeprom_data);
		rom[18] = eeprom_data;
#endif

                for (i = 0; i < 64; i++)
                {
#ifndef EPLC46
                        write_eeprom(ioaddr, i, rom[i]);
#else
                        write_eeprom8(ioaddr, 2*i, ((unsigned char *)rom)[2*i]);
                        write_eeprom8(ioaddr, 2*i+1, ((unsigned char *)rom)[2*i+1]);
#endif
                }

}

static const Optdesc netdmp_opts[] =
{
    {"<interface>", "Interface name"},
    {"<netdmp>", "IP Address"},
    {0}
};

static const Cmd Cmds[] =
{
        {"IP100A"},
        {"ifm_ste", "", NULL,
                    "Set IP100A interface mode: Usage: ifm_ip100a [100|10|auto] [full|half] ", cmd_ifm_ste, 1, 3, 0},
        {"setmac_ste", "", NULL,
                    "Set mac address into IP100A eeprom", cmd_setmac_ste, 1, 5, 0},
        {"readrom_ste", "", NULL,
                        "dump ip100a eprom content", cmd_reprom_ste, 1, 1,0},
        {"writeprom_ste", "", NULL,
                        "write the whole ip100a eprom content", cmd_wrprom_ste, 1, 1,0},
        {0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
        cmdlist_expand(Cmds, 1);
}

#endif

