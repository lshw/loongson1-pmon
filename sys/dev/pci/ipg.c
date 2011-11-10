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
            (ids->subdevice == PCI_ANY_ID || ids->subdevice == subsystem_device) &&
            !((ids->class ^ class) & ids->class_mask))
            return ids;
        ids++;
    }
    return NULL;
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
	  if(direction == 1)
pci_sync_cache(hwdev,addr,size, SYNC_W);
else
 pci_sync_cache(hwdev,addr,size, SYNC_R);
return _pci_dmamap(addr,size);
}

static inline void pci_unmap_single(struct pci_dev *hwdev, dma_addr_t dma_addr,
                    size_t size, int direction)
{
if(direction == 1)
pci_sync_cache(hwdev, _pci_cpumap(dma_addr,size), size, SYNC_W);
else
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
/* Socket configuration controls. */
#define SIOCGIFNAME	0x8910		/* get iface name		*/
#define SIOCSIFLINK	0x8911		/* set iface channel		*/
#define SIOCGIFMEM	0x891f		/* get memory address (BSD)	*/
#define SIOCSIFMEM	0x8920		/* set memory address (BSD)	*/
#define SIOCSIFNAME	0x8923		/* set interface name */
#define	SIOCSIFHWADDR	0x8924		/* set hardware address 	*/
#define SIOCGIFENCAP	0x8925		/* get/set encapsulations       */
#define SIOCSIFENCAP	0x8926		
#define SIOCGIFHWADDR	0x8927		/* Get hardware address		*/
#define SIOCGIFSLAVE	0x8929		/* Driver slaving support	*/
#define SIOCSIFSLAVE	0x8930
#define SIOCGIFINDEX	0x8933		/* name -> if_index mapping	*/
#define SIOGIFINDEX	SIOCGIFINDEX	/* misprint compatibility :-)	*/
#define SIOCSIFPFLAGS	0x8934		/* set/get extended flags set	*/
#define SIOCGIFPFLAGS	0x8935
#define	SIOCSIFHWBROADCAST	0x8937	/* set hardware broadcast addr	*/
#define SIOCGIFCOUNT	0x8938		/* get number of devices */

#define SIOCGIFBR	0x8940		/* Bridging support		*/
#define SIOCSIFBR	0x8941		/* Set bridging options 	*/

#define SIOCGIFTXQLEN	0x8942		/* Get the tx queue length	*/
#define SIOCSIFTXQLEN	0x8943		/* Set the tx queue length 	*/

#define SIOCGIFDIVERT	0x8944		/* Frame diversion support */
#define SIOCSIFDIVERT	0x8945		/* Set frame diversion options */

#define SIOCETHTOOL	0x8946		/* Ethtool interface		*/

#define SIOCGMIIPHY	0x8947		/* Get address of MII PHY in use. */
#define SIOCGMIIREG	0x8948		/* Read MII PHY register.	*/
#define SIOCSMIIREG	0x8949		/* Write MII PHY register.	*/

#define SIOCWANDEV	0x894A		/* get/set netdev parameters	*/

/* ARP cache control calls. */
		    /*  0x8950 - 0x8952  * obsolete calls, don't re-use */
#define SIOCDARP	0x8953		/* delete ARP table entry	*/
#define SIOCGARP	0x8954		/* get ARP table entry		*/
#define SIOCSARP	0x8955		/* set ARP table entry		*/

/* RARP cache control calls. */
#define SIOCDRARP	0x8960		/* delete RARP table entry	*/
#define SIOCGRARP	0x8961		/* get RARP table entry		*/
#define SIOCSRARP	0x8962		/* set RARP table entry		*/

/* Driver configuration calls */

#define SIOCGIFMAP	0x8970		/* Get device parameters	*/
#define SIOCSIFMAP	0x8971		/* Set device parameters	*/

/* DLCI configuration calls */

#define SIOCADDDLCI	0x8980		/* Create new DLCI device	*/
#define SIOCDELDLCI	0x8981		/* Delete DLCI device		*/

#define SIOCGIFVLAN	0x8982		/* 802.1Q VLAN support		*/
#define SIOCSIFVLAN	0x8983		/* Set 802.1Q VLAN options 	*/

/* bonding calls */

#define SIOCBONDENSLAVE	0x8990		/* enslave a device to the bond */
#define SIOCBONDRELEASE 0x8991		/* release a slave from the bond*/
#define SIOCBONDSETHWADDR      0x8992	/* set the hw addr of the bond  */
#define SIOCBONDSLAVEINFOQUERY 0x8993   /* rtn info about slave state   */
#define SIOCBONDINFOQUERY      0x8994	/* rtn info about bond state    */
#define SIOCBONDCHANGEACTIVE   0x8995   /* update to a new active slave */
			
/* bridge calls */
#define SIOCBRADDBR     0x89a0		/* create new bridge device     */
#define SIOCBRDELBR     0x89a1		/* remove bridge device         */
#define SIOCBRADDIF	0x89a2		/* add interface to bridge      */
#define SIOCBRDELIF	0x89a3		/* remove interface from bridge */

/* Device private ioctl calls */

/*
 *	These 16 ioctls are available to devices via the do_ioctl() device
 *	vector. Each device should include this file and redefine these names
 *	as their own. Because these are device dependent it is a good idea
 *	_NOT_ to issue them to random objects and hope.
 *
 *	THESE IOCTLS ARE _DEPRECATED_ AND WILL DISAPPEAR IN 2.5.X -DaveM
 */
 
#define SIOCDEVPRIVATE	0x89F0	/* to 89FF */

/*
 *	These 16 ioctl calls are protocol private
 */
 
#define SIOCPROTOPRIVATE 0x89E0 /* to 89EF */
//--------------------------------------------------------------------
#define MAX_ADDR_LEN    8       /* Largest hardware address length */
#define IFNAMSIZ 16
#define		IPG_MAX_RXFRAME_SIZE		0x0600
struct dev_mc_list 
{
    struct dev_mc_list  *next;
    u_int8_t       dmi_addr[MAX_ADDR_LEN];
    unsigned char       dmi_addrlen;
    int         dmi_users;
    int         dmi_gusers;
};
struct TFD
{
	unsigned long long 		TFDNextPtr;
	unsigned long long 		TFC;
	unsigned long long		FragInfo;
};

/* Receive Frame Descriptor. Note, each RFD field is 64 bits wide.
 */
struct RFD
{
	unsigned long long		RFDNextPtr;
	unsigned long long 		RFS;
	unsigned long long		FragInfo;
};
#define dma_addr_t u32
struct ipg_dmabuff
{
	dma_addr_t	dmahandle;
	unsigned long	len;
};

struct SJumbo
{
	int FoundStart;
	int CurrentSize;
	struct sk_buff			*skb;
};
#define		IPG_TFDLIST_LENGTH		0x100
#define		IPG_RFDLIST_LENGTH		0x100
struct net_device *root_ipg_ethernet_device = NULL;
/* Structure of IPG NIC specific data. */
struct ipg_nic_private
{
        struct TFD    		*TFDList;
        struct RFD    		*RFDList;
	dma_addr_t		TFDListDMAhandle;
	dma_addr_t		RFDListDMAhandle;
	struct ipg_dmabuff	TxBuffDMAhandle[IPG_TFDLIST_LENGTH];
	struct ipg_dmabuff	RxBuffDMAhandle[IPG_RFDLIST_LENGTH];
	struct sk_buff		*TxBuff[IPG_TFDLIST_LENGTH];
	struct sk_buff		*RxBuff[IPG_RFDLIST_LENGTH];
	unsigned short			CurrentTxFrameID;
	int			CurrentTFD;
	int			LastFreedTxBuff;
	int			CurrentRFD;
// Add by Grace 2005/05/19
#ifdef JUMBO_FRAME
    	struct SJumbo      Jumbo;
#endif
	int			LastRestoredRxBuff;
	int			RxBuffNotReady;
	struct pci_dev		*ipg_pci_device;
	struct net_device_stats	stats;
	struct net_device	*next_ipg_ethernet_device;
        spinlock_t              lock;
	int			tenmbpsmode;
	
	/*Jesse20040128EEPROM_VALUE */
	u16         LED_Mode;
	u16         StationAddr0;   /* Station Address in EEPROM Reg 0x10 */
   u16         StationAddr1;   /* Station Address in EEPROM Reg 0x11 */        
	u16         StationAddr2;   /* Station Address in EEPROM Reg 0x12 */   

#ifdef IPG_DEBUG
	int			TFDunavailCount;
	int			RFDlistendCount;
	int			RFDListCheckedCount;
	int			EmptyRFDListCount;
#endif
};
#ifndef PCI_VENDOR_ID_ICPLUS
#  define       PCI_VENDOR_ID_ICPLUS            0x13F0
#endif
#ifndef PCI_DEVICE_ID_IP1000
#  define       PCI_DEVICE_ID_IP1000		    0x1023
#endif
#ifndef PCI_VENDOR_ID_SUNDANCE
#  define       PCI_VENDOR_ID_SUNDANCE          0x13F0
#endif
#ifndef PCI_DEVICE_ID_SUNDANCE_IPG
#  define       PCI_DEVICE_ID_SUNDANCE_ST2021   0x2021
#endif
#ifndef PCI_DEVICE_ID_TAMARACK_TC9020_9021_ALT
#  define       PCI_DEVICE_ID_TAMARACK_TC9020_9021_ALT 0x9021
#endif
#ifndef PCI_DEVICE_ID_TAMARACK_TC9020_9021
#  define       PCI_DEVICE_ID_TAMARACK_TC9020_9021 0x1021
#endif
#ifndef PCI_VENDOR_ID_DLINK
#  define       PCI_VENDOR_ID_DLINK             0x1186
#endif
#ifndef PCI_DEVICE_ID_DLINK_1002
#  define       PCI_DEVICE_ID_DLINK_1002        0x4000
#endif
#ifndef PCI_DEVICE_ID_DLINK_IP1000A
#  define PCI_DEVICE_ID_DLINK_IP1000A		0x4020
#endif

/* Miscellaneous Constants. */
#define   TRUE  1
#define   FALSE 0

/* Assign IPG_APPEND_FCS_ON_TX > 0 for auto FCS append on TX. */
#define         IPG_APPEND_FCS_ON_TX         TRUE

/* Assign IPG_APPEND_FCS_ON_TX > 0 for auto FCS strip on RX. */
#define         IPG_STRIP_FCS_ON_RX          TRUE

/* Assign IPG_DROP_ON_RX_ETH_ERRORS > 0 to drop RX frames with
 * Ethernet errors.
 */
#define         IPG_DROP_ON_RX_ETH_ERRORS    TRUE

/* Assign IPG_INSERT_MANUAL_VLAN_TAG > 0 to insert VLAN tags manually
 * (via TFC).
 */
#define		IPG_INSERT_MANUAL_VLAN_TAG   FALSE

/* Assign IPG_ADD_IPCHECKSUM_ON_TX > 0 for auto IP checksum on TX. */
#define         IPG_ADD_IPCHECKSUM_ON_TX     FALSE

/* Assign IPG_ADD_TCPCHECKSUM_ON_TX > 0 for auto TCP checksum on TX.
 * DO NOT USE FOR SILICON REVISIONS B3 AND EARLIER.
 */
#define         IPG_ADD_TCPCHECKSUM_ON_TX    FALSE

/* Assign IPG_ADD_UDPCHECKSUM_ON_TX > 0 for auto UDP checksum on TX.
 * DO NOT USE FOR SILICON REVISIONS B3 AND EARLIER.
 */
#define         IPG_ADD_UDPCHECKSUM_ON_TX    FALSE

/* If inserting VLAN tags manually, assign the IPG_MANUAL_VLAN_xx
 * constants as desired.
 */
#define		IPG_MANUAL_VLAN_VID		0xABC
#define		IPG_MANUAL_VLAN_CFI		0x1
#define		IPG_MANUAL_VLAN_USERPRIORITY 0x5

#define         IPG_IO_REG_RANGE		0xFF
#define         IPG_MEM_REG_RANGE		0x154
#define         IPG_DRIVER_NAME		"Sundance Technology IPG Triple-Speed Ethernet"
#define         IPG_NIC_PHY_ADDRESS          0x01
#define		IPG_DMALIST_ALIGN_PAD	0x07
#define		IPG_MULTICAST_HASHTABLE_SIZE	0x40

/* Number of miliseconds to wait after issuing a software reset.
 * 0x05 <= IPG_AC_RESETWAIT to account for proper 10Mbps operation.
 */
#define         IPG_AC_RESETWAIT             0x05

/* Number of IPG_AC_RESETWAIT timeperiods before declaring timeout. */
#define         IPG_AC_RESET_TIMEOUT         0x0A

/* Minimum number of miliseconds used to toggle MDC clock during
 * MII/GMII register access.
 */
#define         IPG_PC_PHYCTRLWAIT           0x01

#define		IPG_TFDLIST_LENGTH		0x100

/* Number of frames between TxDMAComplete interrupt.
 * 0 < IPG_FRAMESBETWEENTXDMACOMPLETES <= IPG_TFDLIST_LENGTH
 */
#define		IPG_FRAMESBETWEENTXDMACOMPLETES 0x1
#ifdef JUMBO_FRAME

# ifdef JUMBO_FRAME_SIZE_2K
# define JUMBO_FRAME_SIZE 2048
# define __IPG_RXFRAG_SIZE 2048
# else
#  ifdef JUMBO_FRAME_SIZE_3K
#  define JUMBO_FRAME_SIZE 3072
#  define __IPG_RXFRAG_SIZE 3072
#  else
#   ifdef JUMBO_FRAME_SIZE_4K
#   define JUMBO_FRAME_SIZE 4096
#   define __IPG_RXFRAG_SIZE 4088
#   else
#    ifdef JUMBO_FRAME_SIZE_5K
#    define JUMBO_FRAME_SIZE 5120
#    define __IPG_RXFRAG_SIZE 4088
#    else
#     ifdef JUMBO_FRAME_SIZE_6K
#     define JUMBO_FRAME_SIZE 6144
#     define __IPG_RXFRAG_SIZE 4088
#     else
#      ifdef JUMBO_FRAME_SIZE_7K
#      define JUMBO_FRAME_SIZE 7168
#      define __IPG_RXFRAG_SIZE 4088
#      else
#       ifdef JUMBO_FRAME_SIZE_8K
#       define JUMBO_FRAME_SIZE 8192
#       define __IPG_RXFRAG_SIZE 4088
#       else
#        ifdef JUMBO_FRAME_SIZE_9K
#        define JUMBO_FRAME_SIZE 9216
#        define __IPG_RXFRAG_SIZE 4088
#        else
#         ifdef JUMBO_FRAME_SIZE_10K
#         define JUMBO_FRAME_SIZE 10240
#         define __IPG_RXFRAG_SIZE 4088
#         else
#         define JUMBO_FRAME_SIZE 4096
#         endif 
#        endif 
#       endif 
#      endif 
#     endif 
#    endif 
#   endif 
#  endif 
# endif 
#endif

/* Size of allocated received buffers. Nominally 0x0600.
 * Define larger if expecting jumbo frames.
 */
#ifdef JUMBO_FRAME
//IPG_TXFRAG_SIZE must <= 0x2b00, or TX will crash
#define		IPG_TXFRAG_SIZE		JUMBO_FRAME_SIZE
#endif


/* Size of allocated received buffers. Nominally 0x0600.
 * Define larger if expecting jumbo frames.
 */
#ifdef JUMBO_FRAME
//4088=4096-8
#define		IPG_RXFRAG_SIZE		__IPG_RXFRAG_SIZE
#define     IPG_RXSUPPORT_SIZE   IPG_MAX_RXFRAME_SIZE
#else
#define		IPG_RXFRAG_SIZE		0x0600
#define     IPG_RXSUPPORT_SIZE   IPG_RXFRAG_SIZE
#endif

/* IPG_MAX_RXFRAME_SIZE <= IPG_RXFRAG_SIZE */
#ifdef JUMBO_FRAME
#define		IPG_MAX_RXFRAME_SIZE		JUMBO_FRAME_SIZE
#else
#define		IPG_MAX_RXFRAME_SIZE		0x0600
#endif


/* Maximum number of RFDs to process per interrupt.
 * 1 < IPG_MAXRFDPROCESS_COUNT < IPG_RFDLIST_LENGTH
 */
#define		IPG_MAXRFDPROCESS_COUNT	0x80

/* Minimum margin between last freed RFD, and current RFD.
 * 1 < IPG_MINUSEDRFDSTOFREE < IPG_RFDLIST_LENGTH
 */
#define		IPG_MINUSEDRFDSTOFREE	0x80

/* Specify priority threshhold for a RxDMAPriority interrupt. */
#define		IPG_PRIORITY_THRESH		0x07

/* Specify the number of receive frames transferred via DMA
 * before a RX interrupt is issued.
 */
#define		IPG_RXFRAME_COUNT		0x08

/* specify the jumbo frame maximum size
 * per unit is 0x600 (the RxBuffer size that one RFD can carry)
 */
#define     MAX_JUMBOSIZE	        0x8   // max is 12K

/* Specify the maximum amount of time (in 64ns increments) to wait
 * before issuing a RX interrupt if number of frames received
 * is less than IPG_RXFRAME_COUNT.
 *
 * Value	Time
 * -------------------
 * 0x3D09	~1ms
 * 0x061A	~100us
 * 0x009C	~10us
 * 0x000F	~1us
 */
#define		IPG_RXDMAWAIT_TIME		0x009C

/* Key register values loaded at driver start up. */

/* TXDMAPollPeriod is specified in 320ns increments.
 *
 * Value	Time
 * ---------------------
 * 0x00-0x01	320ns
 * 0x03		~1us
 * 0x1F		~10us
 * 0xFF		~82us
 */
#define		IPG_TXDMAPOLLPERIOD_VALUE	0x26
#define		IPG_TXSTARTTHRESH_VALUE	0x0FFF

/* TxDMAUrgentThresh specifies the minimum amount of
 * data in the transmit FIFO before asserting an
 * urgent transmit DMA request.
 *
 * Value	Min TxFIFO occupied space before urgent TX request
 * ---------------------------------------------------------------
 * 0x00-0x04	128 bytes (1024 bits)
 * 0x27		1248 bytes (~10000 bits)
 * 0x30		1536 bytes (12288 bits)
 * 0xFF		8192 bytes (65535 bits)
 */
#define		IPG_TXDMAURGENTTHRESH_VALUE	0x04

/* TxDMABurstThresh specifies the minimum amount of
 * free space in the transmit FIFO before asserting an
 * transmit DMA request.
 *
 * Value	Min TxFIFO free space before TX request
 * ----------------------------------------------------
 * 0x00-0x08	256 bytes
 * 0x30		1536 bytes
 * 0xFF		8192 bytes
 */
#define		IPG_TXDMABURSTTHRESH_VALUE	0x30

/* RXDMAPollPeriod is specified in 320ns increments.
 *
 * Value	Time
 * ---------------------
 * 0x00-0x01	320ns
 * 0x03		~1us
 * 0x1F		~10us
 * 0xFF		~82us
 */
#define		IPG_RXDMAPOLLPERIOD_VALUE	0x01
#define		IPG_RXEARLYTHRESH_VALUE	0x07FF

/* RxDMAUrgentThresh specifies the minimum amount of
 * free space within the receive FIFO before asserting
 * a urgent receive DMA request.
 *
 * Value	Min RxFIFO free space before urgent RX request
 * ---------------------------------------------------------------
 * 0x00-0x04	128 bytes (1024 bits)
 * 0x27		1248 bytes (~10000 bits)
 * 0x30		1536 bytes (12288 bits)
 * 0xFF		8192 bytes (65535 bits)
 */
#define		IPG_RXDMAURGENTTHRESH_VALUE	0x30

/* RxDMABurstThresh specifies the minimum amount of
 * occupied space within the receive FIFO before asserting
 * a receive DMA request.
 *
 * Value	Min TxFIFO free space before TX request
 * ----------------------------------------------------
 * 0x00-0x08	256 bytes
 * 0x30		1536 bytes
 * 0xFF		8192 bytes
 */
#define		IPG_RXDMABURSTTHRESH_VALUE	0x30

/* FlowOnThresh specifies the maximum amount of occupied
 * space in the receive FIFO before a PAUSE frame with
 * maximum pause time transmitted.
 *
 * Value	Max RxFIFO occupied space before PAUSE
 * ---------------------------------------------------
 * 0x0000	0 bytes
 * 0x0740	29,696 bytes
 * 0x07FF	32,752 bytes
 */
#define		IPG_FLOWONTHRESH_VALUE	0x03ff

/* FlowOffThresh specifies the minimum amount of occupied
 * space in the receive FIFO before a PAUSE frame with
 * zero pause time is transmitted.
 *
 * Value	Max RxFIFO occupied space before PAUSE
 * ---------------------------------------------------
 * 0x0000	0 bytes
 * 0x00BF	3056 bytes
 * 0x07FF	32,752 bytes
 */
#define		IPG_FLOWOFFTHRESH_VALUE	0x00BF

/* end ipg_tune.h */


/* Marco for printing debug statements.
#  define IPG_DDEBUG_MSG(args...) printk(KERN_DEBUG "IPG: " ## args) */
#define IPG_DEBUG
#ifdef IPG_DEBUG
#  define IPG_DEBUG_MSG     
#  define IPG_DDEBUG_MSG   
#  define IPG_DUMPRFDLIST    ipg_dump_rfdlist(argsu)
#  define IPG_DUMPTFDLIST(args) ipg_dump_tfdlist(args)
#else
#  define IPG_DEBUG_MSG(args...)
#  define IPG_DDEBUG_MSG(args...)
#  define IPG_DUMPRFDLIST(args)
#  define IPG_DUMPTFDLIST(args)
#endif

/*
 * End miscellaneous macros.
 */

/*
 * Register access macros.
 */

/* Use memory access for IPG registers. */

#define RD8    ioread8
#define RD16   ioread16
#define RD32   ioread32
#define WR8    iowrite8
#define WR16   iowrite16
#define WR32   iowrite32

#define		IPG_READ_BYTEREG(regaddr)	RD8(regaddr)

#define		IPG_READ_WORDREG(regaddr) RD16(regaddr)

#define		IPG_READ_LONGREG(regaddr)	RD32(regaddr)

#define		IPG_WRITE_BYTEREG(regaddr, writevalue)	WR8(writevalue, regaddr)

#define		IPG_WRITE_WORDREG(regaddr, writevalue)	WR16(writevalue, regaddr)

#define		IPG_WRITE_LONGREG(regaddr, writevalue)	WR32(writevalue, regaddr)

#define		IPG_READ_ASICCTRL(baseaddr)	RD32(baseaddr + IPG_ASICCTRL)

//Jesse20040128EEPROM_VALUE
//#define		IPG_WRITE_ASICCTRL(baseaddr, writevalue)	WR32(IPG_AC_RSVD_MASK & (writevalue), baseaddr + IPG_ASICCTRL)
#define		IPG_WRITE_ASICCTRL(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_ASICCTRL)

#define		IPG_READ_EEPROMCTRL(baseaddr)	RD16(baseaddr + IPG_EEPROMCTRL)

#define		IPG_WRITE_EEPROMCTRL(baseaddr, writevalue)	WR16(IPG_EC_RSVD_MASK & (writevalue), baseaddr + IPG_EEPROMCTRL)

#define		IPG_READ_EEPROMDATA(baseaddr)	RD16(baseaddr + IPG_EEPROMDATA)

#define		IPG_WRITE_EEPROMDATA(baseaddr, writevalue)	WR16(writevalue, (baseaddr + IPG_EEPROMDATA)

#define		IPG_READ_PHYSET(baseaddr)	RD8(baseaddr + IPG_PHYSET)//Jesse20040128EEPROM_VALUE

#define		IPG_WRITE_PHYSET(baseaddr, writevalue)	WR8(writevalue, baseaddr + IPG_PHYSET)//Jesse20040128EEPROM_VALUE

#define		IPG_READ_PHYCTRL(baseaddr)	RD8(baseaddr + IPG_PHYCTRL)

#define		IPG_WRITE_PHYCTRL(baseaddr, writevalue)	WR8(IPG_PC_RSVD_MASK & (writevalue), baseaddr + IPG_PHYCTRL)

#define		IPG_READ_RECEIVEMODE(baseaddr)	RD8(baseaddr + IPG_RECEIVEMODE)

#define		IPG_WRITE_RECEIVEMODE(baseaddr, writevalue)	WR8(IPG_RM_RSVD_MASK & (writevalue), baseaddr + IPG_RECEIVEMODE)

#define		IPG_READ_MAXFRAMESIZE(baseaddr)	RD16(baseaddr + IPG_MAXFRAMESIZE)

#define		IPG_WRITE_MAXFRAMESIZE(baseaddr, writevalue)	WR16(writevalue, baseaddr + IPG_MAXFRAMESIZE)

#define		IPG_READ_MACCTRL(baseaddr)	RD32(baseaddr + IPG_MACCTRL)

#define		IPG_WRITE_MACCTRL(baseaddr, writevalue)	WR32(IPG_MC_RSVD_MASK & (writevalue), baseaddr + IPG_MACCTRL)

#define		IPG_READ_INTSTATUSACK(baseaddr)	RD16(baseaddr + IPG_INTSTATUSACK)

#define		IPG_READ_INTSTATUS(baseaddr)	RD16(baseaddr + IPG_INTSTATUS)

#define		IPG_WRITE_INTSTATUS(baseaddr, writevalue)	WR16(IPG_IS_RSVD_MASK & (writevalue), baseaddr + IPG_INTSTATUS)

#define		IPG_READ_INTENABLE(baseaddr)	RD16(baseaddr + IPG_INTENABLE)

#define		IPG_WRITE_INTENABLE(baseaddr, writevalue)	WR16(IPG_IE_RSVD_MASK & (writevalue), baseaddr + IPG_INTENABLE)

#define		IPG_READ_WAKEEVENT(baseaddr)	RD8(baseaddr + IPG_WAKEEVENT)

#define		IPG_WRITE_WAKEEVENT(baseaddr, writevalue)	WR8(writevalue, baseaddr + IPG_WAKEEVENT)

#define		IPG_READ_RXEARLYTHRESH(baseaddr)	RD16(baseaddr + IPG_RXEARLYTHRESH)

#define		IPG_WRITE_RXEARLYTHRESH(baseaddr, writevalue)	WR16(IPG_RE_RSVD_MASK & (writevalue), baseaddr + IPG_RXEARLYTHRESH)

#define		IPG_READ_TXSTARTTHRESH(baseaddr)	RD32(baseaddr + IPG_TXSTARTTHRESH)

#define		IPG_WRITE_TXSTARTTHRESH(baseaddr, writevalue)	WR32(IPG_TT_RSVD_MASK & (writevalue), baseaddr + IPG_TXSTARTTHRESH)

#define		IPG_READ_FIFOCTRL(baseaddr)	RD16(baseaddr + IPG_FIFOCTRL)

#define		IPG_WRITE_FIFOCTRL(baseaddr, writevalue)	WR16(IPG_FC_RSVD_MASK & (writevalue), baseaddr + IPG_FIFOCTRL)

#define		IPG_READ_RXDMAPOLLPERIOD(baseaddr)	RD8(baseaddr + IPG_RXDMAPOLLPERIOD)

#define		IPG_WRITE_RXDMAPOLLPERIOD(baseaddr, writevalue)	WR8(IPG_RP_RSVD_MASK & (writevalue), baseaddr + IPG_RXDMAPOLLPERIOD)

#define		IPG_READ_RXDMAURGENTTHRESH(baseaddr)	RD8(baseaddr + IPG_RXDMAURGENTTHRESH)

#define		IPG_WRITE_RXDMAURGENTTHRESH(baseaddr, writevalue)	WR8(IPG_RU_RSVD_MASK & (writevalue), baseaddr + IPG_RXDMAURGENTTHRESH)

#define		IPG_READ_RXDMABURSTTHRESH(baseaddr)	RD8(baseaddr + IPG_RXDMABURSTTHRESH)

#define		IPG_WRITE_RXDMABURSTTHRESH(baseaddr, writevalue)	WR8(writevalue, baseaddr + IPG_RXDMABURSTTHRESH)

#define		IPG_READ_RFDLISTPTR0(baseaddr)	RD32(baseaddr + IPG_RFDLISTPTR0)

#define		IPG_WRITE_RFDLISTPTR0(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_RFDLISTPTR0)

#define		IPG_READ_RFDLISTPTR1(baseaddr)	RD32(baseaddr + IPG_RFDLISTPTR1)

#define		IPG_WRITE_RFDLISTPTR1(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_RFDLISTPTR1)

#define		IPG_READ_TXDMAPOLLPERIOD(baseaddr)	RD8(baseaddr + IPG_TXDMAPOLLPERIOD)

#define		IPG_WRITE_TXDMAPOLLPERIOD(baseaddr, writevalue)	WR8(IPG_TP_RSVD_MASK & (writevalue), baseaddr + IPG_TXDMAPOLLPERIOD)

#define		IPG_READ_TXDMAURGENTTHRESH(baseaddr)	RD8(baseaddr + IPG_TXDMAURGENTTHRESH)

#define		IPG_WRITE_TXDMAURGENTTHRESH(baseaddr, writevalue)	WR8(IPG_TU_RSVD_MASK & (writevalue), baseaddr + IPG_TXDMAURGENTTHRESH)

#define		IPG_READ_TXDMABURSTTHRESH(baseaddr)	RD8(baseaddr + IPG_TXDMABURSTTHRESH)

#define		IPG_WRITE_TXDMABURSTTHRESH(baseaddr, writevalue)	WR8(IPG_TB_RSVD_MASK & (writevalue), baseaddr + IPG_TXDMABURSTTHRESH)

#define		IPG_READ_TFDLISTPTR0(baseaddr)	RD32(baseaddr + IPG_TFDLISTPTR0)

#define		IPG_WRITE_TFDLISTPTR0(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_TFDLISTPTR0)

#define		IPG_READ_TFDLISTPTR1(baseaddr)	RD32(baseaddr + IPG_TFDLISTPTR1)

#define		IPG_WRITE_TFDLISTPTR1(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_TFDLISTPTR1)

#define		IPG_READ_DMACTRL(baseaddr)	RD32(baseaddr + IPG_DMACTRL)

#define		IPG_WRITE_DMACTRL(baseaddr, writevalue)	WR32(IPG_DC_RSVD_MASK & (writevalue), baseaddr + IPG_DMACTRL)

#define		IPG_READ_TXSTATUS(baseaddr)	RD32(baseaddr + IPG_TXSTATUS)

#define		IPG_WRITE_TXSTATUS(baseaddr, writevalue)	WR32(IPG_TS_RSVD_MASK & (writevalue), baseaddr + IPG_TXSTATUS)

#define		IPG_READ_STATIONADDRESS0(baseaddr)	RD16(baseaddr + IPG_STATIONADDRESS0)

#define		IPG_READ_STATIONADDRESS1(baseaddr)	RD16(baseaddr + IPG_STATIONADDRESS1)

#define		IPG_READ_STATIONADDRESS2(baseaddr)	RD16(baseaddr + IPG_STATIONADDRESS2)

#define		IPG_WRITE_STATIONADDRESS0(baseaddr,writevalue)	WR16(writevalue,baseaddr + IPG_STATIONADDRESS0)//JES20040127EEPROM

#define		IPG_WRITE_STATIONADDRESS1(baseaddr,writevalue)	WR16(writevalue,baseaddr + IPG_STATIONADDRESS1)//JES20040127EEPROM

#define		IPG_WRITE_STATIONADDRESS2(baseaddr,writevalue)	WR16(writevalue,baseaddr + IPG_STATIONADDRESS2)//JES20040127EEPROM

#define		IPG_READ_COUNTDOWN(baseaddr)	RD32(baseaddr + IPG_COUNTDOWN)

#define		IPG_WRITE_COUNTDOWN(baseaddr, writevalue)	WR32(IPG_CD_RSVD_MASK & (writevalue), baseaddr + IPG_COUNTDOWN)

#define		IPG_READ_RXDMASTATUS(baseaddr)	RD16(baseaddr + IPG_RXDMASTATUS)

#define		IPG_WRITE_HASHTABLE0(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_HASHTABLE0)

#define		IPG_WRITE_HASHTABLE1(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_HASHTABLE1)

#define		IPG_READ_STATISTICSMASK(baseaddr)	RD32(baseaddr + IPG_STATISTICSMASK)

#define		IPG_WRITE_STATISTICSMASK(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_STATISTICSMASK)

#define		IPG_READ_RMONSTATISTICSMASK(baseaddr)	RD32(baseaddr + IPG_RMONSTATISTICSMASK)

#define		IPG_WRITE_RMONSTATISTICSMASK(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_RMONSTATISTICSMASK)

#define		IPG_READ_VLANTAG(baseaddr)	RD32(baseaddr + IPG_VLANTAG)

#define		IPG_WRITE_VLANTAG(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_VLANTAG)

#define		IPG_READ_FLOWONTHRESH(baseaddr)	RD16(baseaddr + IPG_FLOWONTHRESH)

#define		IPG_WRITE_FLOWONTHRESH(baseaddr, writevalue)	WR16(writevalue, baseaddr + IPG_FLOWONTHRESH)

#define		IPG_READ_FLOWOFFTHRESH(baseaddr)	RD16(baseaddr + IPG_FLOWOFFTHRESH)

#define		IPG_WRITE_FLOWOFFTHRESH(baseaddr, writevalue)	WR16(writevalue, baseaddr + IPG_FLOWOFFTHRESH)

#define		IPG_READ_DEBUGCTRL(baseaddr)	RD16(baseaddr + IPG_DEBUGCTRL)

#define		IPG_WRITE_DEBUGCTRL(baseaddr, writevalue)	WR16(writevalue, baseaddr + IPG_DEBUGCTRL)

#define		IPG_READ_RXDMAINTCTRL(baseaddr)	RD32(baseaddr + IPG_RXDMAINTCTRL)

#define		IPG_WRITE_RXDMAINTCTRL(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_RXDMAINTCTRL)

#define		IPG_READ_TXJUMBOFRAMES(baseaddr)	RD16(baseaddr + IPG_TXJUMBOFRAMES)

#define		IPG_WRITE_TXJUMBOFRAMES(baseaddr, writevalue)	WR16(writevalue, baseaddr + IPG_TXJUMBOFRAMES)

#define		IPG_READ_UDPCHECKSUMERRORS(baseaddr)	RD16(baseaddr + IPG_UDPCHECKSUMERRORS)

#define		IPG_WRITE_UDPCHECKSUMERRORS(baseaddr, writevalue)	WR16(writevalue, baseaddr + IPG_UDPCHECKSUMERRORS)

#define		IPG_READ_IPCHECKSUMERRORS(baseaddr)	RD16(baseaddr + IPG_IPCHECKSUMERRORS)

#define		IPG_WRITE_IPCHECKSUMERRORS(baseaddr, writevalue)	WR16(writevalue, baseaddr + IPG_IPCHECKSUMERRORS)

#define		IPG_READ_TCPCHECKSUMERRORS(baseaddr)	RD16(baseaddr + IPG_TCPCHECKSUMERRORS)

#define		IPG_WRITE_TCPCHECKSUMERRORS(baseaddr, writevalue)	WR16(writevalue, baseaddr + IPG_TCPCHECKSUMERRORS)

#define		IPG_READ_RXJUMBOFRAMES(baseaddr)	RD16(baseaddr + IPG_RXJUMBOFRAMES)

#define		IPG_WRITE_RXJUMBOFRAMES(baseaddr, writevalue)	WR16(writevalue, baseaddr + IPG_RXJUMBOFRAMES)



/* Statistic register read/write macros. */

#define		IPG_READ_OCTETRCVOK(baseaddr)	RD32(baseaddr + IPG_OCTETRCVOK)

#define		IPG_WRITE_OCTETRCVOK(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_OCTETRCVOK)

#define		IPG_READ_MCSTOCTETRCVDOK(baseaddr)	RD32(baseaddr + IPG_MCSTOCTETRCVDOK)

#define		IPG_WRITE_MCSTOCTETRCVDOK(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_MCSTOCTETRCVDOK)

#define		IPG_READ_BCSTOCTETRCVOK(baseaddr)	RD32(baseaddr + IPG_BCSTOCTETRCVOK)

#define		IPG_WRITE_BCSTOCTETRCVOK(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_BCSTOCTETRCVOK)

#define		IPG_READ_FRAMESRCVDOK(baseaddr)	RD32(baseaddr + IPG_FRAMESRCVDOK)

#define		IPG_WRITE_FRAMESRCVDOK(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_FRAMESRCVDOK)

#define		IPG_READ_MCSTFRAMESRCVDOK(baseaddr)	RD32(baseaddr + IPG_MCSTFRAMESRCVDOK)

#define		IPG_WRITE_MCSTFRAMESRCVDOK(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_MCSTFRAMESRCVDOK)

#define		IPG_READ_BCSTFRAMESRCVDOK(baseaddr)	RD16(baseaddr + IPG_BCSTFRAMESRCVDOK)

#define		IPG_WRITE_BCSTFRAMESRCVDOK(baseaddr, writevalue)	WR16(writevalue, baseaddr + IPG_BCSTFRAMESRCVDOK)

#define		IPG_READ_MACCONTROLFRAMESRCVD(baseaddr)	RD16(baseaddr + IPG_MACCONTROLFRAMESRCVD)

#define		IPG_WRITE_MACCONTROLFRAMESRCVD(baseaddr, writevalue)	WR16(writevalue, baseaddr + IPG_MACCONTROLFRAMESRCVD)

#define		IPG_READ_FRAMETOOLONGERRRORS(baseaddr)	RD16(baseaddr + IPG_FRAMETOOLONGERRRORS)

#define		IPG_WRITE_FRAMETOOLONGERRRORS(baseaddr, writevalue)	WR16(writevalue, baseaddr + IPG_FRAMETOOLONGERRRORS)

#define		IPG_READ_INRANGELENGTHERRORS(baseaddr)	RD16(baseaddr + IPG_INRANGELENGTHERRORS)

#define		IPG_WRITE_INRANGELENGTHERRORS(baseaddr, writevalue)	WR16(writevalue, baseaddr + IPG_INRANGELENGTHERRORS)

#define		IPG_READ_FRAMECHECKSEQERRORS(baseaddr)	RD16(baseaddr + IPG_FRAMECHECKSEQERRORS)

#define		IPG_WRITE_FRAMECHECKSEQERRORS(baseaddr, writevalue)	WR16(writevalue, baseaddr + IPG_FRAMECHECKSEQERRORS)

#define		IPG_READ_FRAMESLOSTRXERRORS(baseaddr)	RD16(baseaddr + IPG_FRAMESLOSTRXERRORS)

#define		IPG_WRITE_FRAMESLOSTRXERRORS(baseaddr, writevalue) WR16(writevalue, baseaddr + IPG_FRAMESLOSTRXERRORS)

#define		IPG_READ_FRAMESLOSTRXERRORS(baseaddr)	RD16(baseaddr + IPG_FRAMESLOSTRXERRORS)

#define		IPG_WRITE_FRAMESLOSTRXERRORS(baseaddr, writevalue)	WR16(writevalue, baseaddr + IPG_FRAMESLOSTRXERRORS)

#define		IPG_READ_OCTETXMTOK(baseaddr)	RD32(baseaddr + IPG_OCTETXMTOK)

#define		IPG_WRITE_OCTETXMTOK(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_OCTETXMTOK)

#define		IPG_READ_MCSTOCTETXMTOK(baseaddr)	RD32(baseaddr + IPG_MCSTOCTETXMTOK)

#define		IPG_WRITE_MCSTOCTETXMTOK(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_MCSTOCTETXMTOK)

#define		IPG_READ_BCSTOCTETXMTOK(baseaddr)	RD32(baseaddr + IPG_BCSTOCTETXMTOK)

#define		IPG_WRITE_BCSTOCTETXMTOK(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_BCSTOCTETXMTOK)

#define		IPG_READ_FRAMESXMTDOK(baseaddr)	RD32(baseaddr + IPG_FRAMESXMTDOK)

#define		IPG_WRITE_FRAMESXMTDOK(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_FRAMESXMTDOK)

#define		IPG_READ_MCSTFRAMESXMTDOK(baseaddr)	RD32(baseaddr + IPG_MCSTFRAMESXMTDOK)

#define		IPG_WRITE_MCSTFRAMESXMTDOK(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_MCSTFRAMESXMTDOK)

#define		IPG_READ_FRAMESWDEFERREDXMT(baseaddr)	RD32(baseaddr + IPG_FRAMESWDEFERREDXMT)

#define		IPG_WRITE_FRAMESWDEFERREDXMT(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_FRAMESWDEFERREDXMT)

#define		IPG_READ_LATECOLLISIONS(baseaddr)	RD32(baseaddr + IPG_LATECOLLISIONS)

#define		IPG_WRITE_LATECOLLISIONS(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_LATECOLLISIONS)

#define		IPG_READ_MULTICOLFRAMES(baseaddr)	RD32(baseaddr + IPG_MULTICOLFRAMES)

#define		IPG_WRITE_MULTICOLFRAMES(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_MULTICOLFRAMES)

#define		IPG_READ_SINGLECOLFRAMES(baseaddr)	RD32(baseaddr + IPG_SINGLECOLFRAMES)

#define		IPG_WRITE_SINGLECOLFRAMES(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_SINGLECOLFRAMES)

#define		IPG_READ_BCSTFRAMESXMTDOK(baseaddr)	RD16(baseaddr + IPG_BCSTFRAMESXMTDOK)

#define		IPG_WRITE_BCSTFRAMESXMTDOK(baseaddr, writevalue)	WR16(writevalue, baseaddr + IPG_BCSTFRAMESXMTDOK)

#define		IPG_READ_CARRIERSENSEERRORS(baseaddr)	RD16(baseaddr + IPG_CARRIERSENSEERRORS)

#define		IPG_WRITE_CARRIERSENSEERRORS(baseaddr, writevalue)	WR16(writevalue, baseaddr + IPG_CARRIERSENSEERRORS)

#define		IPG_READ_MACCONTROLFRAMESXMTDOK(baseaddr)	RD16(baseaddr + IPG_MACCONTROLFRAMESXMTDOK)

#define		IPG_WRITE_MACCONTROLFRAMESXMTDOK(baseaddr, writevalue) WR16(writevalue, baseaddr + IPG_MACCONTROLFRAMESXMTDOK)

#define		IPG_READ_FRAMESABORTXSCOLLS(baseaddr)	RD16(baseaddr + IPG_FRAMESABORTXSCOLLS)

#define		IPG_WRITE_FRAMESABORTXSCOLLS(baseaddr, writevalue)	WR16(writevalue, baseaddr + IPG_FRAMESABORTXSCOLLS)

#define		IPG_READ_FRAMESWEXDEFERRAL(baseaddr)	RD16(baseaddr + IPG_FRAMESWEXDEFERRAL)

#define		IPG_WRITE_FRAMESWEXDEFERRAL(baseaddr, writevalue)	WR16(writevalue, baseaddr + IPG_FRAMESWEXDEFERRAL)

/* RMON statistic register read/write macros. */

#define		IPG_READ_ETHERSTATSCOLLISIONS(baseaddr)	RD32(baseaddr + IPG_ETHERSTATSCOLLISIONS)

#define		IPG_WRITE_ETHERSTATSCOLLISIONS(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_ETHERSTATSCOLLISIONS)

#define		IPG_READ_ETHERSTATSOCTETSTRANSMIT(baseaddr)	RD32(baseaddr + IPG_ETHERSTATSOCTETSTRANSMIT)

#define		IPG_WRITE_ETHERSTATSOCTETSTRANSMIT(baseaddr, writevalue) WR32(writevalue, baseaddr + IPG_ETHERSTATSOCTETSTRANSMIT)

#define		IPG_READ_ETHERSTATSPKTSTRANSMIT(baseaddr)	RD32(baseaddr + IPG_ETHERSTATSPKTSTRANSMIT)

#define		IPG_WRITE_ETHERSTATSPKTSTRANSMIT(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_ETHERSTATSPKTSTRANSMIT)

#define		IPG_READ_ETHERSTATSPKTS64OCTESTSTRANSMIT(baseaddr)	RD32(baseaddr + IPG_ETHERSTATSPKTS64OCTESTSTRANSMIT)

#define		IPG_WRITE_ETHERSTATSPKTS64OCTESTSTRANSMIT(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_ETHERSTATSPKTS64OCTESTSTRANSMIT)

#define		IPG_READ_ETHERSTATSPKTS65TO127OCTESTSTRANSMIT(baseaddr) RD32(baseaddr + IPG_ETHERSTATSPKTS65TO127OCTESTSTRANSMIT)

#define		IPG_WRITE_ETHERSTATSPKTS65TO127OCTESTSTRANSMIT(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_ETHERSTATSPKTS65TO127OCTESTSTRANSMIT)

#define		IPG_READ_ETHERSTATSPKTS128TO255OCTESTSTRANSMIT(baseaddr)	RD32(baseaddr + IPG_ETHERSTATSPKTS128TO255OCTESTSTRANSMIT)

#define		IPG_WRITE_ETHERSTATSPKTS128TO255OCTESTSTRANSMIT(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_ETHERSTATSPKTS128TO255OCTESTSTRANSMIT)

#define		IPG_READ_ETHERSTATSPKTS256TO511OCTESTSTRANSMIT(baseaddr) RD32(baseaddr + IPG_ETHERSTATSPKTS256TO511OCTESTSTRANSMIT)

#define		IPG_WRITE_ETHERSTATSPKTS256TO511OCTESTSTRANSMIT(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_ETHERSTATSPKTS256TO511OCTESTSTRANSMIT)

#define		IPG_READ_ETHERSTATSPKTS512TO1023OCTESTSTRANSMIT(baseaddr) RD32(baseaddr + IPG_ETHERSTATSPKTS512TO1023OCTESTSTRANSMIT)

#define		IPG_WRITE_ETHERSTATSPKTS512TO1023OCTESTSTRANSMIT(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_ETHERSTATSPKTS512TO1023OCTESTSTRANSMIT)

#define		IPG_READ_ETHERSTATSPKTS1024TO1518OCTESTSTRANSMIT(baseaddr) RD32(baseaddr + IPG_ETHERSTATSPKTS1024TO1518OCTESTSTRANSMIT)

#define		IPG_WRITE_ETHERSTATSPKTS1024TO1518OCTESTSTRANSMIT(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_ETHERSTATSPKTS1024TO1518OCTESTSTRANSMIT)

#define		IPG_READ_ETHERSTATSCRCALIGNERRORS(baseaddr)	RD32(baseaddr + IPG_ETHERSTATSCRCALIGNERRORS)

#define		IPG_WRITE_ETHERSTATSCRCALIGNERRORS(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_ETHERSTATSCRCALIGNERRORS)

#define		IPG_READ_ETHERSTATSUNDERSIZEPKTS(baseaddr)	RD32(baseaddr + IPG_ETHERSTATSUNDERSIZEPKTS)

#define		IPG_WRITE_ETHERSTATSUNDERSIZEPKTS(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_ETHERSTATSUNDERSIZEPKTS)

#define		IPG_READ_ETHERSTATSFRAGMENTS(baseaddr)	RD32(baseaddr + IPG_ETHERSTATSFRAGMENTS)

#define		IPG_WRITE_ETHERSTATSFRAGMENTS(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_ETHERSTATSFRAGMENTS)

#define		IPG_READ_ETHERSTATSJABBERS(baseaddr)	RD32(baseaddr + IPG_ETHERSTATSJABBERS)

#define		IPG_WRITE_ETHERSTATSJABBERS(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_ETHERSTATSJABBERS)

#define		IPG_READ_ETHERSTATSOCTETS(baseaddr)	RD32(baseaddr + IPG_ETHERSTATSOCTETS)

#define		IPG_WRITE_ETHERSTATSOCTETS(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_ETHERSTATSOCTETS)

#define		IPG_READ_ETHERSTATSPKTS(baseaddr)	RD32(baseaddr + IPG_ETHERSTATSPKTS)

#define		IPG_WRITE_ETHERSTATSPKTS(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_ETHERSTATSPKTS)

#define		IPG_READ_ETHERSTATSPKTS64OCTESTS(baseaddr)	RD32(baseaddr + IPG_ETHERSTATSPKTS64OCTESTS)

#define		IPG_WRITE_ETHERSTATSPKTS64OCTESTS(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_ETHERSTATSPKTS64OCTESTS)

#define		IPG_READ_ETHERSTATSPKTS65TO127OCTESTS(baseaddr)	RD32(baseaddr + IPG_ETHERSTATSPKTS65TO127OCTESTS)

#define		IPG_WRITE_ETHERSTATSPKTS65TO127OCTESTS(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_ETHERSTATSPKTS65TO127OCTESTS)

#define		IPG_READ_ETHERSTATSPKTS128TO255OCTESTS(baseaddr)	RD32(baseaddr + IPG_ETHERSTATSPKTS128TO255OCTESTS)

#define		IPG_WRITE_ETHERSTATSPKTS128TO255OCTESTS(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_ETHERSTATSPKTS128TO255OCTESTS)

#define		IPG_READ_ETHERSTATSPKTS256TO511OCTESTS(baseaddr)	RD32(baseaddr + IPG_ETHERSTATSPKTS256TO511OCTESTS)

#define		IPG_WRITE_ETHERSTATSPKTS256TO511OCTESTS(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_ETHERSTATSPKTS256TO511OCTESTS)

#define		IPG_READ_ETHERSTATSPKTS512TO1023OCTESTS(baseaddr)	RD32(baseaddr + IPG_ETHERSTATSPKTS512TO1023OCTESTS)

#define		IPG_WRITE_ETHERSTATSPKTS512TO1023OCTESTS(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_ETHERSTATSPKTS512TO1023OCTESTS)

#define		IPG_READ_ETHERSTATSPKTS1024TO1518OCTESTS(baseaddr)	RD32(baseaddr + IPG_ETHERSTATSPKTS1024TO1518OCTESTS)

#define		IPG_WRITE_ETHERSTATSPKTS1024TO1518OCTESTS(baseaddr, writevalue)	WR32(writevalue, baseaddr + IPG_ETHERSTATSPKTS1024TO1518OCTESTS)

/*
 * End register access macros.
 */

#define		NS				0x2000
#define		MARVELL				0x0141
#define		ICPLUS_PHY		0x243

/* NIC Physical Layer Device MII register addresses. */
#define         MII_PHY_CONTROL                 0x00
#define         MII_PHY_STATUS                  0x01
#define         MII_PHY_IDENTIFIER_MSB          0x02
#define         MII_PHY_IDENTIFIER_LSB          0x03
#define         MII_PHY_AUTONEGADVERTISEMENT    0x04
#define         MII_PHY_AUTONEGLINKPARTABILITY  0x05
#define         MII_PHY_AUTONEGEXPANSION        0x06
#define         MII_PHY_AUTONEGNP_TRANSMIT      0x07
#define         MII_PHY_AUTONEGLP_RECEIVE_NP    0x08
#define         MII_PHY_EXTENDED_STATUS         0x0F

/* NIC Physical Layer Device MII register fields. */
#define         MII_PHY_CONTROL_RESTARTAN       0x0200
#define         MII_PHY_SELECTORFIELD           0x001F
#define         MII_PHY_SELECTOR_IEEE8023       0x0001
#define         MII_PHY_TECHABILITYFIELD        0x1FE0
#define         MII_PHY_TECHABILITY_10BT        0x0020
#define         MII_PHY_TECHABILITY_10BTFD      0x0040
#define         MII_PHY_TECHABILITY_100BTX      0x0080
#define         MII_PHY_TECHABILITY_100BTXFD    0x0100
#define         MII_PHY_TECHABILITY_100BT4      0x0200
#define         MII_PHY_TECHABILITY_PAUSE_FIELDS 0x0C00
#define         MII_PHY_TECHABILITY_PAUSE       0x0400
#define         MII_PHY_TECHABILITY_ASM_DIR     0x0800
#define         MII_PHY_TECHABILITY_RSVD2       0x1000
#define         MII_PHY_STATUS_AUTONEG_ABILITY  0x0008

/* NIC Physical Layer Device GMII register addresses. */
#define         GMII_PHY_CONTROL                 0x00
#define         GMII_PHY_STATUS                  0x01
#define			GMII_PHY_ID_1                    0x02
#define         GMII_PHY_AUTONEGADVERTISEMENT    0x04
#define         GMII_PHY_AUTONEGLINKPARTABILITY  0x05
#define         GMII_PHY_AUTONEGEXPANSION        0x06
#define         GMII_PHY_AUTONEGNEXTPAGE         0x07
#define         GMII_PHY_AUTONEGLINKPARTNEXTPAGE 0x08
#define         GMII_PHY_EXTENDEDSTATUS          0x0F

#define         GMII_PHY_1000BASETCONTROL        0x09
#define         GMII_PHY_1000BASETSTATUS         0x0A

/* GMII_PHY_1000 need to set to prefer master */
#define         GMII_PHY_1000BASETCONTROL_PreferMaster 0x0400

#define         GMII_PHY_1000BASETCONTROL_FULL_DUPLEX 0x0200
#define         GMII_PHY_1000BASETCONTROL_HALF_DUPLEX 0x0100
#define         GMII_PHY_1000BASETSTATUS_FULL_DUPLEX 0x0800
#define         GMII_PHY_1000BASETSTATUS_HALF_DUPLEX 0x0400

/* NIC Physical Layer Device GMII register Fields. */
#define         GMII_PHY_CONTROL_RESET          0x8000
#define         GMII_PHY_CONTROL_FULL_DUPLEX    0x0100
#define         GMII_PHY_STATUS_AUTONEG_ABILITY 0x0008
#define         GMII_PHY_ADV_FULL_DUPLEX        0x0020
#define         GMII_PHY_ADV_HALF_DUPLEX        0x0040
#define         GMII_PHY_ADV_PAUSE              0x0180
#define         GMII_PHY_ADV_PAUSE_PS1          0x0080
#define         GMII_PHY_ADV_ASM_DIR_PS2        0x0100

/* NIC Physical Layer Device GMII constants. */
#define         GMII_PREAMBLE                    0xFFFFFFFF
#define         GMII_ST                          0x1
#define         GMII_READ                        0x2
#define         GMII_WRITE                       0x1
#define         GMII_TA_READ_MASK                0x1
#define         GMII_TA_WRITE                    0x2

/* PCI register addresses. */
#define         IPG_POWERMGMTCTRL		0x54
#define         IPG_POWERMGMTCAP		0x52
#define         IPG_NEXTITEMPTR		0x51
#define         IPG_CAPID			0x50
#define         IPG_MAXLAT			0x3F
#define         IPG_MINGNT			0x3E
#define         IPG_INTERRUPTPIN		0x3D
#define         IPG_INTERRUPTLINE		0x3C
#define         IPG_CAPPTR			0x34
#define         IPG_EXPROMBASEADDRESS	0x30
#define         IPG_SUBSYSTEMID		0x2E
#define         IPG_SUBSYSTEMVENDORID	0x2C
#define         IPG_MEMBASEADDRESS		0x14
#define         IPG_IOBASEADDRESS		0x10
#define         IPG_HEADERTYPE		0x0E
#define         IPG_LATENCYTIMER		0x0D
#define         IPG_CACHELINESIZE		0x0C
#define         IPG_CLASSCODE		0x09
#define         IPG_REVISIONID		0x08
#define         IPG_CONFIGSTATUS		0x06
#define         IPG_CONFIGCOMMAND		0x04
#define         IPG_DEVICEID			0x02
#define         IPG_VENDORID			0x00

/* I/O register offsets. */
#define	IPG_DMACTRL			0x00
#define	IPG_RXDMASTATUS		0x08 /* RESERVED */
#define	IPG_TFDLISTPTR0		0x10
#define	IPG_TFDLISTPTR1		0x14
#define	IPG_TXDMABURSTTHRESH		0x18
#define	IPG_TXDMAURGENTTHRESH	0x19
#define	IPG_TXDMAPOLLPERIOD		0x1A
#define	IPG_RFDLISTPTR0		0x1C
#define	IPG_RFDLISTPTR1		0x20
#define	IPG_RXDMABURSTTHRESH		0x24
#define	IPG_RXDMAURGENTTHRESH	0x25
#define	IPG_RXDMAPOLLPERIOD		0x26
#define	IPG_RXDMAINTCTRL		0x28
#define	IPG_DEBUGCTRL		0x2C
#define	IPG_ASICCTRL			0x30
#define	IPG_FIFOCTRL			0x38
#define	IPG_RXEARLYTHRESH		0x3A
#define	IPG_FLOWOFFTHRESH		0x3C
#define	IPG_FLOWONTHRESH		0x3E
#define	IPG_TXSTARTTHRESH		0x44
#define	IPG_EEPROMDATA		0x48
#define	IPG_EEPROMCTRL		0x4A
#define	IPG_EXPROMADDR		0x4C
#define	IPG_EXPROMDATA		0x50
#define	IPG_WAKEEVENT		0x51
#define	IPG_COUNTDOWN		0x54
#define	IPG_INTSTATUSACK		0x5A
#define	IPG_INTENABLE		0x5C
#define	IPG_INTSTATUS		0x5E
#define	IPG_TXSTATUS			0x60
#define	IPG_MACCTRL			0x6C
#define	IPG_VLANTAG			0x70
#define	IPG_PHYSET			0x75	//JES20040127EEPROM
#define	IPG_PHYCTRL			0x76
#define	IPG_STATIONADDRESS0		0x78
#define	IPG_STATIONADDRESS1		0x7A
#define	IPG_STATIONADDRESS2		0x7C
#define	IPG_MAXFRAMESIZE		0x86
#define	IPG_RECEIVEMODE		0x88
#define	IPG_HASHTABLE0		0x8C
#define	IPG_HASHTABLE1		0x90
#define	IPG_RMONSTATISTICSMASK	0x98
#define	IPG_STATISTICSMASK		0x9C
#define	IPG_RXJUMBOFRAMES		0xBC
#define	IPG_TCPCHECKSUMERRORS	0xC0
#define	IPG_IPCHECKSUMERRORS		0xC2
#define	IPG_UDPCHECKSUMERRORS	0xC4
#define	IPG_TXJUMBOFRAMES		0xF4

/* Ethernet MIB statistic register offsets. */
#define	IPG_OCTETRCVOK		0xA8
#define	IPG_MCSTOCTETRCVDOK		0xAC
#define	IPG_BCSTOCTETRCVOK		0xB0
#define	IPG_FRAMESRCVDOK		0xB4
#define	IPG_MCSTFRAMESRCVDOK		0xB8
#define	IPG_BCSTFRAMESRCVDOK		0xBE
#define	IPG_MACCONTROLFRAMESRCVD	0xC6
#define	IPG_FRAMETOOLONGERRRORS	0xC8
#define	IPG_INRANGELENGTHERRORS	0xCA
#define	IPG_FRAMECHECKSEQERRORS	0xCC
#define	IPG_FRAMESLOSTRXERRORS	0xCE
#define	IPG_OCTETXMTOK		0xD0
#define	IPG_MCSTOCTETXMTOK		0xD4
#define	IPG_BCSTOCTETXMTOK		0xD8
#define	IPG_FRAMESXMTDOK		0xDC
#define	IPG_MCSTFRAMESXMTDOK		0xE0
#define	IPG_FRAMESWDEFERREDXMT	0xE4
#define	IPG_LATECOLLISIONS		0xE8
#define	IPG_MULTICOLFRAMES		0xEC
#define	IPG_SINGLECOLFRAMES		0xF0
#define	IPG_BCSTFRAMESXMTDOK		0xF6
#define	IPG_CARRIERSENSEERRORS	0xF8
#define	IPG_MACCONTROLFRAMESXMTDOK	0xFA
#define	IPG_FRAMESABORTXSCOLLS	0xFC
#define	IPG_FRAMESWEXDEFERRAL	0xFE

/* RMON statistic register offsets. */
#define	IPG_ETHERSTATSCOLLISIONS			0x100
#define	IPG_ETHERSTATSOCTETSTRANSMIT			0x104
#define	IPG_ETHERSTATSPKTSTRANSMIT			0x108
#define	IPG_ETHERSTATSPKTS64OCTESTSTRANSMIT		0x10C
#define	IPG_ETHERSTATSPKTS65TO127OCTESTSTRANSMIT	0x110
#define	IPG_ETHERSTATSPKTS128TO255OCTESTSTRANSMIT	0x114
#define	IPG_ETHERSTATSPKTS256TO511OCTESTSTRANSMIT	0x118
#define	IPG_ETHERSTATSPKTS512TO1023OCTESTSTRANSMIT	0x11C
#define	IPG_ETHERSTATSPKTS1024TO1518OCTESTSTRANSMIT	0x120
#define	IPG_ETHERSTATSCRCALIGNERRORS			0x124
#define	IPG_ETHERSTATSUNDERSIZEPKTS			0x128
#define	IPG_ETHERSTATSFRAGMENTS			0x12C
#define	IPG_ETHERSTATSJABBERS			0x130
#define	IPG_ETHERSTATSOCTETS				0x134
#define	IPG_ETHERSTATSPKTS				0x138
#define	IPG_ETHERSTATSPKTS64OCTESTS			0x13C
#define	IPG_ETHERSTATSPKTS65TO127OCTESTS		0x140
#define	IPG_ETHERSTATSPKTS128TO255OCTESTS		0x144
#define	IPG_ETHERSTATSPKTS256TO511OCTESTS		0x148
#define	IPG_ETHERSTATSPKTS512TO1023OCTESTS		0x14C
#define	IPG_ETHERSTATSPKTS1024TO1518OCTESTS		0x150

/* RMON statistic register equivalents. */
#define	IPG_ETHERSTATSMULTICASTPKTSTRANSMIT		0xE0
#define	IPG_ETHERSTATSBROADCASTPKTSTRANSMIT		0xF6
#define	IPG_ETHERSTATSMULTICASTPKTS			0xB8
#define	IPG_ETHERSTATSBROADCASTPKTS			0xBE
#define	IPG_ETHERSTATSOVERSIZEPKTS			0xC8
#define	IPG_ETHERSTATSDROPEVENTS			0xCE

/* Serial EEPROM offsets */
#define	IPG_EEPROM_CONFIGPARAM	0x00
#define	IPG_EEPROM_ASICCTRL		0x01
#define	IPG_EEPROM_SUBSYSTEMVENDORID	0x02
#define	IPG_EEPROM_SUBSYSTEMID	0x03
#define	IPG_EEPROM_STATIONADDRESS0	0x10
#define	IPG_EEPROM_STATIONADDRESS1	0x11
#define	IPG_EEPROM_STATIONADDRESS2	0x12


/* Register & data structure bit masks */

/* PCI register masks. */

/* IOBaseAddress */
#define         IPG_PIB_RSVD_MASK		0xFFFFFE01
#define         IPG_PIB_IOBASEADDRESS	0xFFFFFF00
#define         IPG_PIB_IOBASEADDRIND	0x00000001

/* MemBaseAddress */
#define         IPG_PMB_RSVD_MASK		0xFFFFFE07
#define         IPG_PMB_MEMBASEADDRIND	0x00000001
#define         IPG_PMB_MEMMAPTYPE		0x00000006
#define         IPG_PMB_MEMMAPTYPE0		0x00000002
#define         IPG_PMB_MEMMAPTYPE1		0x00000004
#define         IPG_PMB_MEMBASEADDRESS	0xFFFFFE00

/* ConfigStatus */
#define IPG_CS_RSVD_MASK                0xFFB0
#define IPG_CS_CAPABILITIES             0x0010
#define IPG_CS_66MHZCAPABLE             0x0020
#define IPG_CS_FASTBACK2BACK            0x0080
#define IPG_CS_DATAPARITYREPORTED       0x0100
#define IPG_CS_DEVSELTIMING             0x0600
#define IPG_CS_SIGNALEDTARGETABORT      0x0800
#define IPG_CS_RECEIVEDTARGETABORT      0x1000
#define IPG_CS_RECEIVEDMASTERABORT      0x2000
#define IPG_CS_SIGNALEDSYSTEMERROR      0x4000
#define IPG_CS_DETECTEDPARITYERROR      0x8000

/* TFD data structure masks. */

/* TFDList, TFC */
#define	IPG_TFC_RSVD_MASK			0x0000FFFF9FFFFFFF
#define	IPG_TFC_FRAMEID			0x000000000000FFFF
#define	IPG_TFC_WORDALIGN			0x0000000000030000
#define	IPG_TFC_WORDALIGNTODWORD		0x0000000000000000
#define	IPG_TFC_WORDALIGNTOWORD		0x0000000000020000
#define	IPG_TFC_WORDALIGNDISABLED		0x0000000000030000
#define	IPG_TFC_TCPCHECKSUMENABLE		0x0000000000040000
#define	IPG_TFC_UDPCHECKSUMENABLE		0x0000000000080000
#define	IPG_TFC_IPCHECKSUMENABLE		0x0000000000100000
#define	IPG_TFC_FCSAPPENDDISABLE		0x0000000000200000
#define	IPG_TFC_TXINDICATE			0x0000000000400000
#define	IPG_TFC_TXDMAINDICATE		0x0000000000800000
#define	IPG_TFC_FRAGCOUNT			0x000000000F000000
#define	IPG_TFC_VLANTAGINSERT		0x0000000010000000
#define	IPG_TFC_TFDDONE			0x0000000080000000
#define	IPG_TFC_VID				0x00000FFF00000000
#define	IPG_TFC_CFI				0x0000100000000000
#define	IPG_TFC_USERPRIORITY			0x0000E00000000000

/* TFDList, FragInfo */
#define	IPG_TFI_RSVD_MASK			0xFFFF00FFFFFFFFFF
#define	IPG_TFI_FRAGADDR			0x000000FFFFFFFFFF
#define	IPG_TFI_FRAGLEN			0xFFFF000000000000LL

/* RFD data structure masks. */

/* RFDList, RFS */
#define	IPG_RFS_RSVD_MASK			0x0000FFFFFFFFFFFF
#define	IPG_RFS_RXFRAMELEN			0x000000000000FFFF
#define	IPG_RFS_RXFIFOOVERRUN		0x0000000000010000
#define	IPG_RFS_RXRUNTFRAME			0x0000000000020000
#define	IPG_RFS_RXALIGNMENTERROR		0x0000000000040000
#define	IPG_RFS_RXFCSERROR			0x0000000000080000
#define	IPG_RFS_RXOVERSIZEDFRAME		0x0000000000100000
#define	IPG_RFS_RXLENGTHERROR		0x0000000000200000
#define	IPG_RFS_VLANDETECTED			0x0000000000400000
#define	IPG_RFS_TCPDETECTED			0x0000000000800000
#define	IPG_RFS_TCPERROR			0x0000000001000000
#define	IPG_RFS_UDPDETECTED			0x0000000002000000
#define	IPG_RFS_UDPERROR			0x0000000004000000
#define	IPG_RFS_IPDETECTED			0x0000000008000000
#define	IPG_RFS_IPERROR			0x0000000010000000
#define	IPG_RFS_FRAMESTART			0x0000000020000000
#define	IPG_RFS_FRAMEEND			0x0000000040000000
#define	IPG_RFS_RFDDONE			0x0000000080000000
#define	IPG_RFS_TCI				0x0000FFFF00000000


/* RFDList, FragInfo */
#define	IPG_RFI_RSVD_MASK			0xFFFF00FFFFFFFFFF
#define	IPG_RFI_FRAGADDR			0x000000FFFFFFFFFF
#define	IPG_RFI_FRAGLEN			0xFFFF000000000000LL

/* I/O Register masks. */

/* RMON Statistics Mask */
#define	IPG_RZ_ALL					0x0FFFFFFF

/* Statistics Mask */
#define	IPG_SM_ALL					0x0FFFFFFF
#define	IPG_SM_OCTETRCVOK_FRAMESRCVDOK		0x00000001
#define	IPG_SM_MCSTOCTETRCVDOK_MCSTFRAMESRCVDOK	0x00000002
#define	IPG_SM_BCSTOCTETRCVDOK_BCSTFRAMESRCVDOK	0x00000004
#define	IPG_SM_RXJUMBOFRAMES				0x00000008
#define	IPG_SM_TCPCHECKSUMERRORS			0x00000010
#define	IPG_SM_IPCHECKSUMERRORS			0x00000020
#define	IPG_SM_UDPCHECKSUMERRORS			0x00000040
#define	IPG_SM_MACCONTROLFRAMESRCVD			0x00000080
#define	IPG_SM_FRAMESTOOLONGERRORS			0x00000100
#define	IPG_SM_INRANGELENGTHERRORS			0x00000200
#define	IPG_SM_FRAMECHECKSEQERRORS			0x00000400
#define	IPG_SM_FRAMESLOSTRXERRORS			0x00000800
#define	IPG_SM_OCTETXMTOK_FRAMESXMTOK		0x00001000
#define	IPG_SM_MCSTOCTETXMTOK_MCSTFRAMESXMTDOK	0x00002000
#define	IPG_SM_BCSTOCTETXMTOK_BCSTFRAMESXMTDOK	0x00004000
#define	IPG_SM_FRAMESWDEFERREDXMT			0x00008000
#define	IPG_SM_LATECOLLISIONS			0x00010000
#define	IPG_SM_MULTICOLFRAMES			0x00020000
#define	IPG_SM_SINGLECOLFRAMES			0x00040000
#define	IPG_SM_TXJUMBOFRAMES				0x00080000
#define	IPG_SM_CARRIERSENSEERRORS			0x00100000
#define	IPG_SM_MACCONTROLFRAMESXMTD			0x00200000
#define	IPG_SM_FRAMESABORTXSCOLLS			0x00400000
#define	IPG_SM_FRAMESWEXDEFERAL			0x00800000


/* Countdown */
#define	IPG_CD_RSVD_MASK		0x0700FFFF
#define	IPG_CD_COUNT			0x0000FFFF
#define	IPG_CD_COUNTDOWNSPEED	0x01000000
#define	IPG_CD_COUNTDOWNMODE		0x02000000
#define	IPG_CD_COUNTINTENABLED	0x04000000

/* TxDMABurstThresh */
#define IPG_TB_RSVD_MASK                0xFF

/* TxDMAUrgentThresh */
#define IPG_TU_RSVD_MASK                0xFF

/* TxDMAPollPeriod */
#define IPG_TP_RSVD_MASK                0xFF

/* RxDMAUrgentThresh */
#define IPG_RU_RSVD_MASK                0xFF

/* RxDMAPollPeriod */
#define IPG_RP_RSVD_MASK                0xFF

/* TxStartThresh */
#define IPG_TT_RSVD_MASK                0x0FFF

/* RxEarlyThresh */
#define IPG_RE_RSVD_MASK                0x07FF

/* ReceiveMode */
#define IPG_RM_RSVD_MASK                0x3F
#define IPG_RM_RECEIVEUNICAST           0x01
#define IPG_RM_RECEIVEMULTICAST         0x02
#define IPG_RM_RECEIVEBROADCAST         0x04
#define IPG_RM_RECEIVEALLFRAMES         0x08
#define IPG_RM_RECEIVEMULTICASTHASH     0x10
#define IPG_RM_RECEIVEIPMULTICAST       0x20

/* PhySet JES20040127EEPROM*/
#define IPG_PS_MEM_LENB9B               0x01
#define IPG_PS_MEM_LEN9                 0x02
#define IPG_PS_NON_COMPDET              0x04

/* PhyCtrl */
#define IPG_PC_RSVD_MASK                0xFF
#define IPG_PC_MGMTCLK_LO               0x00
#define IPG_PC_MGMTCLK_HI               0x01
#define IPG_PC_MGMTCLK                  0x01
#define IPG_PC_MGMTDATA                 0x02
#define IPG_PC_MGMTDIR                  0x04
#define IPG_PC_DUPLEX_POLARITY          0x08
#define IPG_PC_DUPLEX_STATUS            0x10
#define IPG_PC_LINK_POLARITY            0x20
#define IPG_PC_LINK_SPEED               0xC0
#define IPG_PC_LINK_SPEED_10MBPS        0x40
#define IPG_PC_LINK_SPEED_100MBPS       0x80
#define IPG_PC_LINK_SPEED_1000MBPS      0xC0

/* DMACtrl */
#define IPG_DC_RSVD_MASK                0xC07D9818
#define IPG_DC_RX_DMA_COMPLETE          0x00000008
#define IPG_DC_RX_DMA_POLL_NOW          0x00000010
#define IPG_DC_TX_DMA_COMPLETE          0x00000800
#define IPG_DC_TX_DMA_POLL_NOW          0x00001000
#define IPG_DC_TX_DMA_IN_PROG           0x00008000
#define IPG_DC_RX_EARLY_DISABLE         0x00010000
#define IPG_DC_MWI_DISABLE              0x00040000
#define IPG_DC_TX_WRITE_BACK_DISABLE    0x00080000
#define IPG_DC_TX_BURST_LIMIT           0x00700000
#define IPG_DC_TARGET_ABORT             0x40000000
#define IPG_DC_MASTER_ABORT             0x80000000

/* ASICCtrl */
#define IPG_AC_RSVD_MASK                0x07FFEFF2
#define IPG_AC_EXP_ROM_SIZE             0x00000002
#define IPG_AC_PHY_SPEED10              0x00000010
#define IPG_AC_PHY_SPEED100             0x00000020
#define IPG_AC_PHY_SPEED1000            0x00000040
#define IPG_AC_PHY_MEDIA                0x00000080
#define IPG_AC_FORCED_CFG               0x00000700
#define IPG_AC_D3RESETDISABLE           0x00000800
#define IPG_AC_SPEED_UP_MODE            0x00002000
#define IPG_AC_LED_MODE                 0x00004000
#define IPG_AC_RST_OUT_POLARITY         0x00008000
#define IPG_AC_GLOBAL_RESET             0x00010000
#define IPG_AC_RX_RESET                 0x00020000
#define IPG_AC_TX_RESET                 0x00040000
#define IPG_AC_DMA                      0x00080000
#define IPG_AC_FIFO                     0x00100000
#define IPG_AC_NETWORK                  0x00200000
#define IPG_AC_HOST                     0x00400000
#define IPG_AC_AUTO_INIT                0x00800000
#define IPG_AC_RST_OUT                  0x01000000
#define IPG_AC_INT_REQUEST              0x02000000
#define IPG_AC_RESET_BUSY               0x04000000
#define IPG_AC_LED_SPEED                0x08000000	//JES20040127EEPROM
#define IPG_AC_LED_MODE_BIT_1           0x20000000 //JES20040127EEPROM

/* EepromCtrl */
#define IPG_EC_RSVD_MASK                0x83FF
#define IPG_EC_EEPROM_ADDR              0x00FF
#define IPG_EC_EEPROM_OPCODE            0x0300
#define IPG_EC_EEPROM_SUBCOMMAD         0x0000
#define IPG_EC_EEPROM_WRITEOPCODE       0x0100
#define IPG_EC_EEPROM_READOPCODE        0x0200
#define IPG_EC_EEPROM_ERASEOPCODE       0x0300
#define IPG_EC_EEPROM_BUSY              0x8000

/* FIFOCtrl */
#define IPG_FC_RSVD_MASK                0xC001
#define IPG_FC_RAM_TEST_MODE            0x0001
#define IPG_FC_TRANSMITTING             0x4000
#define IPG_FC_RECEIVING                0x8000

/* TxStatus */
#define IPG_TS_RSVD_MASK                0xFFFF00DD
#define IPG_TS_TX_ERROR                 0x00000001
#define IPG_TS_LATE_COLLISION           0x00000004
#define IPG_TS_TX_MAX_COLL              0x00000008
#define IPG_TS_TX_UNDERRUN              0x00000010
#define IPG_TS_TX_IND_REQD              0x00000040
#define IPG_TS_TX_COMPLETE              0x00000080
#define IPG_TS_TX_FRAMEID               0xFFFF0000

/* WakeEvent */
#define IPG_WE_WAKE_PKT_ENABLE          0x01
#define IPG_WE_MAGIC_PKT_ENABLE         0x02
#define IPG_WE_LINK_EVT_ENABLE          0x04
#define IPG_WE_WAKE_POLARITY            0x08
#define IPG_WE_WAKE_PKT_EVT             0x10
#define IPG_WE_MAGIC_PKT_EVT            0x20
#define IPG_WE_LINK_EVT                 0x40
#define IPG_WE_WOL_ENABLE               0x80

/* IntEnable */
#define IPG_IE_RSVD_MASK                0x1FFE
#define IPG_IE_HOST_ERROR               0x0002
#define IPG_IE_TX_COMPLETE              0x0004
#define IPG_IE_MAC_CTRL_FRAME           0x0008
#define IPG_IE_RX_COMPLETE              0x0010
#define IPG_IE_RX_EARLY                 0x0020
#define IPG_IE_INT_REQUESTED            0x0040
#define IPG_IE_UPDATE_STATS             0x0080
#define IPG_IE_LINK_EVENT               0x0100
#define IPG_IE_TX_DMA_COMPLETE          0x0200
#define IPG_IE_RX_DMA_COMPLETE          0x0400
#define IPG_IE_RFD_LIST_END             0x0800
#define IPG_IE_RX_DMA_PRIORITY          0x1000

/* IntStatus */
#define IPG_IS_RSVD_MASK                0x1FFF
#define IPG_IS_INTERRUPT_STATUS         0x0001
#define IPG_IS_HOST_ERROR               0x0002
#define IPG_IS_TX_COMPLETE              0x0004
#define IPG_IS_MAC_CTRL_FRAME           0x0008
#define IPG_IS_RX_COMPLETE              0x0010
#define IPG_IS_RX_EARLY                 0x0020
#define IPG_IS_INT_REQUESTED            0x0040
#define IPG_IS_UPDATE_STATS             0x0080
#define IPG_IS_LINK_EVENT               0x0100
#define IPG_IS_TX_DMA_COMPLETE          0x0200
#define IPG_IS_RX_DMA_COMPLETE          0x0400
#define IPG_IS_RFD_LIST_END             0x0800
#define IPG_IS_RX_DMA_PRIORITY          0x1000

/* MACCtrl */
#define IPG_MC_RSVD_MASK                0x7FE33FA3
#define IPG_MC_IFS_SELECT               0x00000003
#define IPG_MC_IFS_4352BIT              0x00000003
#define IPG_MC_IFS_1792BIT              0x00000002
#define IPG_MC_IFS_1024BIT              0x00000001
#define IPG_MC_IFS_96BIT                0x00000000
#define IPG_MC_DUPLEX_SELECT            0x00000020
#define IPG_MC_DUPLEX_SELECT_FD         0x00000020
#define IPG_MC_DUPLEX_SELECT_HD         0x00000000
#define IPG_MC_TX_FLOW_CONTROL_ENABLE   0x00000080
#define IPG_MC_RX_FLOW_CONTROL_ENABLE   0x00000100
#define IPG_MC_RCV_FCS                  0x00000200
#define IPG_MC_FIFO_LOOPBACK            0x00000400
#define IPG_MC_MAC_LOOPBACK             0x00000800
#define IPG_MC_AUTO_VLAN_TAGGING        0x00001000
#define IPG_MC_AUTO_VLAN_UNTAGGING      0x00002000
#define IPG_MC_COLLISION_DETECT         0x00010000
#define IPG_MC_CARRIER_SENSE            0x00020000
#define IPG_MC_STATISTICS_ENABLE        0x00200000
#define IPG_MC_STATISTICS_DISABLE       0x00400000
#define IPG_MC_STATISTICS_ENABLED       0x00800000
#define IPG_MC_TX_ENABLE                0x01000000
#define IPG_MC_TX_DISABLE               0x02000000
#define IPG_MC_TX_ENABLED               0x04000000
#define IPG_MC_RX_ENABLE                0x08000000
#define IPG_MC_RX_DISABLE               0x10000000
#define IPG_MC_RX_ENABLED               0x20000000
#define IPG_MC_PAUSED                   0x40000000

/* RxDMAIntCtrl */
#define IPG_RI_RSVD_MASK                0xFFFF1CFF
#define IPG_RI_RXFRAME_COUNT            0x000000FF
#define IPG_RI_PRIORITY_THRESH          0x00001C00
#define IPG_RI_RXDMAWAIT_TIME           0xFFFF0000

struct	nic_id
{
	char*		NICname;
	int		vendorid;
	int		deviceid;
};

struct	nic_id	nics_supported[] =
{
	{"IC PLUS IP1000 1000/100/10 based NIC",
	 PCI_VENDOR_ID_ICPLUS,
	 PCI_DEVICE_ID_IP1000},
	{"Sundance Technology ST2021 based NIC",
     PCI_VENDOR_ID_SUNDANCE,
	 PCI_DEVICE_ID_SUNDANCE_ST2021},
	{"Tamarack Microelectronics TC9020/9021 based NIC",
	 PCI_VENDOR_ID_SUNDANCE,
	 PCI_DEVICE_ID_TAMARACK_TC9020_9021},
	{"Tamarack Microelectronics TC9020/9021 based NIC",
	 PCI_VENDOR_ID_SUNDANCE,
	 PCI_DEVICE_ID_TAMARACK_TC9020_9021_ALT},
	{"D-Link NIC",
	 PCI_VENDOR_ID_DLINK,
	 PCI_DEVICE_ID_DLINK_1002},
	{"D-Link NIC IP1000A",
	 PCI_VENDOR_ID_DLINK,
	 PCI_DEVICE_ID_DLINK_IP1000A},
	 
	{"N/A", 0xFFFF, 0}
};

#ifdef IPG_LINUX2_4
struct	pci_device_id	pci_devices_supported[] =
{
	{PCI_VENDOR_ID_ICPLUS,
	 PCI_DEVICE_ID_IP1000,
	 PCI_ANY_ID,
	 PCI_ANY_ID,
	 0x020000,
	 0xFFFFFF,
	 0},

	{PCI_VENDOR_ID_SUNDANCE,
	 PCI_DEVICE_ID_SUNDANCE_ST2021,
	 PCI_ANY_ID,
	 PCI_ANY_ID,
	 0x020000,
	 0xFFFFFF,
	 1},

	{PCI_VENDOR_ID_SUNDANCE,
	 PCI_DEVICE_ID_TAMARACK_TC9020_9021,
	 PCI_ANY_ID,
	 PCI_ANY_ID,
	 0x020000,
	 0xFFFFFF,
	 2},

	{PCI_VENDOR_ID_DLINK,
	 PCI_DEVICE_ID_DLINK_1002,
	 PCI_ANY_ID,
	 PCI_ANY_ID,
	 0x020000,
	 0xFFFFFF,
	 3},

	{0,}
};
#endif
/* end ipg_structs.h */

// PhyParam.h
//variable record -- index by leading revision/length
//Revision/Length(=N*4), Address1, Data1, Address2, Data2,...,AddressN,DataN
unsigned short DefaultPhyParam[] = {
	// 11/12/03 IP1000A v1-3 rev=0x40
	/*--------------------------------------------------------------------------
	(0x4000|(15*4)), 31, 0x0001, 27, 0x01e0, 31, 0x0002, 22, 0x85bd, 24, 0xfff2,
		    		 27, 0x0c10, 28, 0x0c10, 29, 0x2c10, 31, 0x0003, 23, 0x92f6,
		    		 31, 0x0000, 23, 0x003d, 30, 0x00de, 20, 0x20e7,  9, 0x0700,
	  --------------------------------------------------------------------------*/
	// 12/17/03 IP1000A v1-4 rev=0x40
	(0x4000|(07*4)), 31, 0x0001, 27, 0x01e0, 31, 0x0002, 27, 0xeb8e, 31, 0x0000,
	                 30, 0x005e,  9, 0x0700,
	// 01/09/04 IP1000A v1-5 rev=0x41
	(0x4100|(07*4)), 31, 0x0001, 27, 0x01e0, 31, 0x0002, 27, 0xeb8e, 31, 0x0000,
	                 30, 0x005e,  9, 0x0700,
	// 01/09/04 IP1000A v1-5 rev=0x42
	(0x4200|(07*4)), 31, 0x0001, 27, 0x01e0, 31, 0x0002, 27, 0xeb8e, 31, 0x0000,
	                 30, 0x005e,  9, 0x0700,
	// 01/09/04 IP1000A v1-5 rev=0x43
	(0x4300|(07*4)), 31, 0x0001, 27, 0x01e0, 31, 0x0002, 27, 0xeb8e, 31, 0x0000,
	                 30, 0x005e,  9, 0x0700,
	// 01/09/04 IP1000A v1-5 rev=0x44
	(0x4400|(07*4)), 31, 0x0001, 27, 0x01e0, 31, 0x0002, 27, 0xeb8e, 31, 0x0000,
	                 30, 0x005e,  9, 0x0700,
	// 01/09/04 IP1000A v1-5 rev=0x45
	(0x4500|(07*4)), 31, 0x0001, 27, 0x01e0, 31, 0x0002, 27, 0xeb8e, 31, 0x0000,
	                 30, 0x005e,  9, 0x0700,
	// 01/09/04 IP1000A v1-5 rev=0x46
	(0x4600|(07*4)), 31, 0x0001, 27, 0x01e0, 31, 0x0002, 27, 0xeb8e, 31, 0x0000,
	                 30, 0x005e,  9, 0x0700,
	// 01/09/04 IP1000A v1-5 rev=0x47
	(0x4700|(07*4)), 31, 0x0001, 27, 0x01e0, 31, 0x0002, 27, 0xeb8e, 31, 0x0000,
	                 30, 0x005e,  9, 0x0700,
	// 01/09/04 IP1000A v1-5 rev=0x48
	(0x4800|(07*4)), 31, 0x0001, 27, 0x01e0, 31, 0x0002, 27, 0xeb8e, 31, 0x0000,
	                 30, 0x005e,  9, 0x0700,
	// 01/09/04 IP1000A v1-5 rev=0x49
	(0x4900|(07*4)), 31, 0x0001, 27, 0x01e0, 31, 0x0002, 27, 0xeb8e, 31, 0x0000,
	                 30, 0x005e,  9, 0x0700,
	// 01/09/04 IP1000A v1-5 rev=0x4A
	(0x4A00|(07*4)), 31, 0x0001, 27, 0x01e0, 31, 0x0002, 27, 0xeb8e, 31, 0x0000,
	                 30, 0x005e,  9, 0x0700,
	// 01/09/04 IP1000A v1-5 rev=0x4B
	(0x4B00|(07*4)), 31, 0x0001, 27, 0x01e0, 31, 0x0002, 27, 0xeb8e, 31, 0x0000,
	                 30, 0x005e,  9, 0x0700,
	// 01/09/04 IP1000A v1-5 rev=0x4C
	(0x4C00|(07*4)), 31, 0x0001, 27, 0x01e0, 31, 0x0002, 27, 0xeb8e, 31, 0x0000,
	                 30, 0x005e,  9, 0x0700,
	// 01/09/04 IP1000A v1-5 rev=0x4D
	(0x4D00|(07*4)), 31, 0x0001, 27, 0x01e0, 31, 0x0002, 27, 0xeb8e, 31, 0x0000,
	                 30, 0x005e,  9, 0x0700,
	// 01/09/04 IP1000A v1-5 rev=0x4E
	(0x4E00|(07*4)), 31, 0x0001, 27, 0x01e0, 31, 0x0002, 27, 0xeb8e, 31, 0x0000,
	                 30, 0x005e,  9, 0x0700,
	 0x0000};
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
//static int debug = 1;			/* 1 normal messages, 0 quiet .. 7 verbose. */
/* Maximum number of multicast addresses to filter (vs. rx-all-multicast).
   Typical is a 64 element hash table based on the Ethernet CRC.  */
static const int multicast_filter_limit = 32;

/* Set the copy breakpoint for the copy-only-tiny-frames scheme.
   Setting to > 1518 effectively disables this feature.
   This chip can receive into offset buffers, so the Alpha does not
   need a copy-align. */
//static int rx_copybreak;
//static int flowctrl=1;

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
//static char *media[MAX_UNITS];


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

static const struct pci_device_id ipg_pci_tbl[] = {
	//{ 0x1186, 0x1002, 0x1186, 0x1002, 0, 0, 0 },
//	{ 0x1186, 0x1002, 0x1186, 0x1003, 0, 0, 1 },
//	{ 0x1186, 0x1002, 0x1186, 0x1012, 0, 0, 2 },
//	{ 0x1186, 0x1002, 0x1186, 0x1040, 0, 0, 3 },
	{ 0x1186, 0x4020, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 4 },
//	{ 0x13F0, 0x0201, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 5 },
//	{ 0x13F0, 0x0200, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 6 },
//	{ 0x13F0, 0x1023, PCI_ANY_ID, PCI_ANY_ID, 0x020000, 0xFFFFFF, 0 },
	{ 0x13F0, 0x1023, PCI_ANY_ID, PCI_ANY_ID, 0x0, 0xFFFFFF, 0 },
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

/* Countdown */
#define	IPG_CD_RSVD_MASK		0x0700FFFF
#define	IPG_CD_COUNT			0x0000FFFF
#define	IPG_CD_COUNTDOWNSPEED	0x01000000
#define	IPG_CD_COUNTDOWNMODE		0x02000000
#define	IPG_CD_COUNTINTENABLED	0x04000000

/* TxDMABurstThresh */
#define IPG_TB_RSVD_MASK                0xFF

/* TxDMAUrgentThresh */
#define IPG_TU_RSVD_MASK                0xFF

/* TxDMAPollPeriod */
#define IPG_TP_RSVD_MASK                0xFF

/* RxDMAUrgentThresh */
#define IPG_RU_RSVD_MASK                0xFF

/* RxDMAPollPeriod */
#define IPG_RP_RSVD_MASK                0xFF

/* TxStartThresh */
#define IPG_TT_RSVD_MASK                0x0FFF

/* RxEarlyThresh */
#define IPG_RE_RSVD_MASK                0x07FF

#define PRIV_ALIGN	15 	/* Required alignment mask */
/* Use  __attribute__((aligned (L1_CACHE_BYTES)))  to maintain alignment
   within the structure. */
#define MII_CNT		4
/* The station address location in the EEPROM. */
#define EEPROM_SA_OFFSET	0x10
#define DEFAULT_INTR (IntrRxDMADone | IntrPCIErr | \
			IntrDrvRqst | IntrTxDone | StatsMax | \
			LinkChange)

static int  netdev_open(struct net_device *dev);
static int ipg_io_config(struct net_device *dev);
static int init_rfdlist(struct net_device *dev);
static int init_tfdlist(struct net_device *dev);
static int ipg_intr(void *);
static int read_eeprom(struct net_device *dev,int i);

static int  sundance_probe1 (struct net_device *dev,struct pci_dev *pdev,
				      const struct pci_device_id *ent)
{
	struct ipg_nic_private *sp;
	int irq;
	unsigned long baseaddr;
	int bar = 0;

	_pci_conf_write(pdev->pa.pa_tag,4,7);
	baseaddr=0xbfd00000|(_pci_conf_read(pdev->pa.pa_tag,bar*4+0x10)&~3);
	printf("probe,baseaddr=%x\n",baseaddr);

	if (!baseaddr)
		return -1;

	dev->base_addr = baseaddr;
	dev->irq = irq;

	sp = netdev_priv(dev);
	sp->ipg_pci_device=pdev;
	sp->StationAddr0=read_eeprom(dev, 16);
	sp->StationAddr1=read_eeprom(dev, 17);
	sp->StationAddr2=read_eeprom(dev, 18);
	printf("Addr0=%x,Addr1=%x,Addr2=%x\n",sp->StationAddr0,sp->StationAddr1,sp->StationAddr2);
	/* Write MAC Address to Station Address */
	IPG_WRITE_STATIONADDRESS0(baseaddr,sp->StationAddr0);
	IPG_WRITE_STATIONADDRESS1(baseaddr,sp->StationAddr1);
	IPG_WRITE_STATIONADDRESS2(baseaddr,sp->StationAddr2);

	/* Set station address in ethernet_device structure. */
	dev->dev_addr[0] =
	  IPG_READ_STATIONADDRESS0(baseaddr) & 0x00FF;
	dev->dev_addr[1] =
	  (IPG_READ_STATIONADDRESS0(baseaddr) & 0xFF00) >> 8;
	dev->dev_addr[2] =
	  IPG_READ_STATIONADDRESS1(baseaddr) & 0x00FF;
	dev->dev_addr[3] =
	  (IPG_READ_STATIONADDRESS1(baseaddr) & 0xFF00) >> 8;
	dev->dev_addr[4] =
	  IPG_READ_STATIONADDRESS2(baseaddr) & 0x00FF;
	dev->dev_addr[5] =
	  (IPG_READ_STATIONADDRESS2(baseaddr) & 0xFF00) >> 8;
	spin_lock_init(&np->lock);

	return 0;

}
#if 0
static void sundance_reset(struct net_device *dev, unsigned long reset_cmd)
{
//	struct ipg_nic_private *np = netdev_priv(dev);
	unsigned long ioaddr = dev->base_addr + ASICCtrl;
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
#endif
/////////////////////////////liqing///////////////////
#if 1
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
static  int set_bit(int nr,long * addr)
{
    int mask, retval;

    addr += nr >> 5;
    mask = 1 << (nr & 0x1f);
//    cli();
    retval = (mask & *addr) != 0;
    *addr |= mask;
//    sti();
    return retval;
}
#ifdef IPG_DEBUG
static void ipg_dump_rfdlist(struct net_device *ipg_ethernet_device)
{
	unsigned long				baseaddr;
	int				i;
	u32				offset;
	struct ipg_nic_private	*sp = (struct ipg_nic_private *)
					      ipg_ethernet_device->priv;

	IPG_DEBUG_MSG("_dump_rfdlist\n");

	baseaddr = ipg_ethernet_device->base_addr;

	printk(KERN_INFO "CurrentRFD         = %2.2x\n", sp->CurrentRFD);
	printk(KERN_INFO "LastRestoredRxBuff = %2.2x\n",
	       sp->LastRestoredRxBuff);
	printk(KERN_INFO "RFDList start address = %16.16lx\n",
	       (unsigned long int)(sp->RFDListDMAhandle));
	printk(KERN_INFO "RFDListPtr register   = %8.8x%8.8x\n",
	       IPG_READ_RFDLISTPTR1(baseaddr),
	       IPG_READ_RFDLISTPTR0(baseaddr));

	for(i=0;i<IPG_RFDLIST_LENGTH;i++)
	{
		offset = (u32)(&sp->RFDList[i].RFDNextPtr)
		         - (u32)(sp->RFDList);
		printk(KERN_INFO "%2.2x %4.4x RFDNextPtr = %16.16lx\n", i,
		       offset, (unsigned long int) sp->RFDList[i].RFDNextPtr);
		offset = (u32)(&sp->RFDList[i].RFS)
		         - (u32)(sp->RFDList);
		printk(KERN_INFO "%2.2x %4.4x RFS        = %16.16lx\n", i,
		       offset, (unsigned long int)sp->RFDList[i].RFS);
		offset = (u32)(&sp->RFDList[i].FragInfo)
		         - (u32)(sp->RFDList);
		printk(KERN_INFO "%2.2x %4.4x FragInfo   = %16.16lx\n", i,
		       offset, (unsigned long int)sp->RFDList[i].FragInfo);
	}

	return;
}

static void ipg_dump_tfdlist(struct net_device	*ipg_ethernet_device)
{
	unsigned long				baseaddr;
	int				i;
	u32				offset;
	struct ipg_nic_private	*sp = (struct ipg_nic_private *)
					      ipg_ethernet_device->priv;

//	IPG_DEBUG_MSG("_dump_tfdlist\n");

	baseaddr = ipg_ethernet_device->base_addr;

//	printk(KERN_INFO "CurrentTFD         = %2.2x\n", sp->CurrentTFD);
//	printk(KERN_INFO "LastFreedTxBuff = %2.2x\n",
//	       sp->LastFreedTxBuff);
//	printk(KERN_INFO "TFDList start address = %16.16lx\n",
//	       (unsigned long int)(sp->TFDListDMAhandle));
//	printk(KERN_INFO "TFDListPtr register   = %8.8x%8.8x\n",
//	       IPG_READ_TFDLISTPTR1(baseaddr),
//	       IPG_READ_TFDLISTPTR0(baseaddr));

	for(i=0;i<IPG_TFDLIST_LENGTH;i++)
	{
		offset = (u32)(&sp->TFDList[i].TFDNextPtr)
		         - (u32)(sp->TFDList);
//		printk(KERN_INFO "%2.2x %4.4x TFDNextPtr = %16.16lx\n", i,
//		       offset, (unsigned long int)sp->TFDList[i].TFDNextPtr);

		offset = (u32)(&sp->TFDList[i].TFC)
		         - (u32)(sp->TFDList);
//		printk(KERN_INFO "%2.2x %4.4x TFC        = %16.16lx\n", i,
//		       offset, (unsigned long int)sp->TFDList[i].TFC);
		offset = (u32)(&sp->TFDList[i].FragInfo)
		         - (u32)(sp->TFDList);
//		printk(KERN_INFO "%2.2x %4.4x FragInfo   = %16.16lx\n", i,
//		       offset, (unsigned long int)sp->TFDList[i].FragInfo);
	}

	return;
}
#else /* Not in debug mode. */
#endif

static void send_three_state(unsigned long baseaddr, u8 phyctrlpolarity)
{
	IPG_WRITE_PHYCTRL(baseaddr, IPG_PC_MGMTCLK_LO |
							(IPG_PC_MGMTDATA & 0) |
		                     IPG_PC_MGMTDIR | phyctrlpolarity);

	mdelay(IPG_PC_PHYCTRLWAIT);

	IPG_WRITE_PHYCTRL(baseaddr, IPG_PC_MGMTCLK_HI |
		                     (IPG_PC_MGMTDATA & 0) |
		                     IPG_PC_MGMTDIR | phyctrlpolarity);

	mdelay(IPG_PC_PHYCTRLWAIT);
	return;
}

static void send_end(unsigned long baseaddr, u8 phyctrlpolarity)
{
	IPG_WRITE_PHYCTRL(baseaddr, IPG_PC_MGMTCLK_LO |
							(IPG_PC_MGMTDATA & 0) |
		                     IPG_PC_MGMTDIR | phyctrlpolarity);
	return;
}


static u16 read_phy_bit(unsigned long baseaddr, u8 phyctrlpolarity)
{   u16 bit_data;
	IPG_WRITE_PHYCTRL(baseaddr, IPG_PC_MGMTCLK_LO |
		                     phyctrlpolarity);

	mdelay(IPG_PC_PHYCTRLWAIT);

	bit_data=((IPG_READ_PHYCTRL(baseaddr) & IPG_PC_MGMTDATA) >> 1) & 1;

	IPG_WRITE_PHYCTRL(baseaddr, IPG_PC_MGMTCLK_HI |
		                     phyctrlpolarity);

	mdelay(IPG_PC_PHYCTRLWAIT);
	return bit_data;
}

static u16 read_phy_register(struct net_device *ipg_ethernet_device,
	                  int phy_address, int phy_register)
{
	/* Read a register from the Physical Layer device located
	 * on the IPG NIC, using the IPG PHYCTRL register.
	 */

	unsigned long	baseaddr;
	int	i;
	int	j;
	int	fieldlen[8];
	u32	field[8];
	u8	databit;
        u8      phyctrlpolarity;

//	IPG_DEBUG_MSG("read_phy_register\n");

	baseaddr = ipg_ethernet_device->base_addr;

	/* The GMII mangement frame structure for a read is as follows:
	 *
	 * |Preamble|st|op|phyad|regad|ta|      data      |idle|
	 * |< 32 1s>|01|10|AAAAA|RRRRR|z0|DDDDDDDDDDDDDDDD|z   |
	 *
	 * <32 1s> = 32 consecutive logic 1 values
	 * A = bit of Physical Layer device address (MSB first)
	 * R = bit of register address (MSB first)
	 * z = High impedance state
	 * D = bit of read data (MSB first)
	 *
	 * Transmission order is 'Preamble' field first, bits transmitted
	 * left to right (first to last).
	 */

	field[0]    = GMII_PREAMBLE;
	fieldlen[0] = 32;		/* Preamble */
	field[1]    = GMII_ST;
	fieldlen[1] = 2;		/* ST */
	field[2]    = GMII_READ;
	fieldlen[2] = 2;		/* OP */
	field[3]    = phy_address;
	fieldlen[3] = 5;		/* PHYAD */
	field[4]    = phy_register;
	fieldlen[4] = 5;		/* REGAD */
	field[5]    = 0x0000;
	fieldlen[5] = 2;		/* TA */
	field[6]    = 0x0000;
	fieldlen[6] = 16;		/* DATA */
	field[7]    = 0x0000;
	fieldlen[7] = 1;		/* IDLE */

        /* Store the polarity values of PHYCTRL. */
        phyctrlpolarity = IPG_READ_PHYCTRL(baseaddr) &
                          (IPG_PC_DUPLEX_POLARITY |
	                   IPG_PC_LINK_POLARITY);

	/* Create the Preamble, ST, OP, PHYAD, and REGAD field. */
	for(j=0; j<5; j++)
	for(i=0; i<fieldlen[j]; i++)
	{
		/* For each variable length field, the MSB must be
		 * transmitted first. Rotate through the field bits,
		 * starting with the MSB, and move each bit into the
		 * the 1st (2^1) bit position (this is the bit position
		 * corresponding to the MgmtData bit of the PhyCtrl
		 * register for the IPG).
		 *
		 * Example: ST = 01;
		 *
		 *          First write a '0' to bit 1 of the PhyCtrl
		 *          register, then write a '1' to bit 1 of the
		 *          PhyCtrl register.
		 *
		 * To do this, right shift the MSB of ST by the value:
		 * [field length - 1 - #ST bits already written]
		 * then left shift this result by 1.
		 */
		databit = (field[j] >> (fieldlen[j] - 1 - i)) << 1;

		IPG_WRITE_PHYCTRL(baseaddr, IPG_PC_MGMTCLK_LO |
		                     (IPG_PC_MGMTDATA & databit) |
		                     IPG_PC_MGMTDIR | phyctrlpolarity);

		mdelay(IPG_PC_PHYCTRLWAIT);

		IPG_WRITE_PHYCTRL(baseaddr, IPG_PC_MGMTCLK_HI |
		                     (IPG_PC_MGMTDATA & databit) |
		                     IPG_PC_MGMTDIR | phyctrlpolarity);

		mdelay(IPG_PC_PHYCTRLWAIT);
	}

	send_three_state(baseaddr, phyctrlpolarity);

	read_phy_bit(baseaddr, phyctrlpolarity);

	/* For a read cycle, the bits for the next two fields (TA and
	 * DATA) are driven by the PHY (the IPG reads these bits).
	 */
//	for(j=6; j<8; j++)
	for(i=0; i<fieldlen[6]; i++)
	{
		field[6] |= (read_phy_bit(baseaddr, phyctrlpolarity) << (fieldlen[6]- 1 - i));

	}

	send_three_state(baseaddr, phyctrlpolarity);
	send_three_state(baseaddr, phyctrlpolarity);
	send_three_state(baseaddr, phyctrlpolarity);
	send_end(baseaddr, phyctrlpolarity);

	/* Return the value of the DATA field. */
	return field[6];
}

static void write_phy_register(struct net_device *ipg_ethernet_device,
	                  int phy_address, int phy_register, u16 writeval)
{
	/* Write to a register from the Physical Layer device located
	 * on the IPG NIC, using the IPG PHYCTRL register.
	 */

	unsigned long	baseaddr;
	int	i;
	int	j;
	int	fieldlen[8];
	u32	field[8];
	u8	databit;
	u8	phyctrlpolarity;

//	IPG_DEBUG_MSG("write_phy_register\n");

	baseaddr = ipg_ethernet_device->base_addr;

	/* The GMII mangement frame structure for a read is as follows:
	 *
	 * |Preamble|st|op|phyad|regad|ta|      data      |idle|
	 * |< 32 1s>|01|10|AAAAA|RRRRR|z0|DDDDDDDDDDDDDDDD|z   |
	 *
	 * <32 1s> = 32 consecutive logic 1 values
	 * A = bit of Physical Layer device address (MSB first)
	 * R = bit of register address (MSB first)
	 * z = High impedance state
	 * D = bit of write data (MSB first)
	 *
	 * Transmission order is 'Preamble' field first, bits transmitted
	 * left to right (first to last).
	 */

	field[0]    = GMII_PREAMBLE;
	fieldlen[0] = 32;		/* Preamble */
	field[1]    = GMII_ST;
	fieldlen[1] = 2;		/* ST */
	field[2]    = GMII_WRITE;
	fieldlen[2] = 2;		/* OP */
	field[3]    = phy_address;
	fieldlen[3] = 5;		/* PHYAD */
	field[4]    = phy_register;
	fieldlen[4] = 5;		/* REGAD */
	field[5]    = 0x0002;
	fieldlen[5] = 2;		/* TA */
	field[6]    = writeval;
	fieldlen[6] = 16;		/* DATA */
	field[7]    = 0x0000;
	fieldlen[7] = 1;		/* IDLE */

        /* Store the polarity values of PHYCTRL. */
        phyctrlpolarity = IPG_READ_PHYCTRL(baseaddr) &
                          (IPG_PC_DUPLEX_POLARITY |
	                   IPG_PC_LINK_POLARITY);

	/* Create the Preamble, ST, OP, PHYAD, and REGAD field. */
	for(j=0; j<7; j++)
	for(i=0; i<fieldlen[j]; i++)
	{
		/* For each variable length field, the MSB must be
		 * transmitted first. Rotate through the field bits,
		 * starting with the MSB, and move each bit into the
		 * the 1st (2^1) bit position (this is the bit position
		 * corresponding to the MgmtData bit of the PhyCtrl
		 * register for the IPG).
		 *
		 * Example: ST = 01;
		 *
		 *          First write a '0' to bit 1 of the PhyCtrl
		 *          register, then write a '1' to bit 1 of the
		 *          PhyCtrl register.
		 *
		 * To do this, right shift the MSB of ST by the value:
		 * [field length - 1 - #ST bits already written]
		 * then left shift this result by 1.
		 */
		databit = (field[j] >> (fieldlen[j] - 1 - i)) << 1;

		IPG_WRITE_PHYCTRL(baseaddr, IPG_PC_MGMTCLK_LO |
		                     (IPG_PC_MGMTDATA & databit) |
		                     IPG_PC_MGMTDIR | phyctrlpolarity);

		mdelay(IPG_PC_PHYCTRLWAIT);

		IPG_WRITE_PHYCTRL(baseaddr, IPG_PC_MGMTCLK_HI |
		                     (IPG_PC_MGMTDATA & databit) |
		                     IPG_PC_MGMTDIR | phyctrlpolarity);

		mdelay(IPG_PC_PHYCTRLWAIT);
	}

	/* The last cycle is a tri-state, so read from the PHY.
	 */
	for(j=7; j<8; j++)
	for(i=0; i<fieldlen[j]; i++)
	{
		IPG_WRITE_PHYCTRL(baseaddr, IPG_PC_MGMTCLK_LO |
		                     phyctrlpolarity);

		mdelay(IPG_PC_PHYCTRLWAIT);

		field[j] |= ((IPG_READ_PHYCTRL(baseaddr) &
		             IPG_PC_MGMTDATA) >> 1)
		            << (fieldlen[j] - 1 - i);

		IPG_WRITE_PHYCTRL(baseaddr, IPG_PC_MGMTCLK_HI |
		                     phyctrlpolarity);

		mdelay(IPG_PC_PHYCTRLWAIT);

	}

	return;
}
//int     ipg_reset(u32 baseaddr, u32 resetflags) //JES20040127EEPROM: change type of param1
static int ipg_reset(struct net_device *ipg_ethernet_device, u32 resetflags)
{
	/* Assert functional resets via the IPG AsicCtrl
	 * register as specified by the 'resetflags' input
	 * parameter.
	 */
        unsigned long	baseaddr;//JES20040127EEPROM:	
	int timeout_count;
	baseaddr = ipg_ethernet_device->base_addr;//JES20040127EEPROM:

	IPG_DEBUG_MSG("_reset\n");

	IPG_WRITE_ASICCTRL(baseaddr, IPG_READ_ASICCTRL(baseaddr) |
	               resetflags);

	/* Wait until IPG reset is complete. */
	timeout_count = 0;

	/* Delay added to account for problem with 10Mbps reset. */
	mdelay(IPG_AC_RESETWAIT);

	while (IPG_AC_RESET_BUSY & IPG_READ_ASICCTRL(baseaddr))
	{
		mdelay(IPG_AC_RESETWAIT);
		timeout_count++;
		if (timeout_count > IPG_AC_RESET_TIMEOUT)
			return -1;
	}
   /* Set LED Mode in Asic Control JES20040127EEPROM */
   Set_LED_Mode(ipg_ethernet_device);
        
   /* Set PHYSet Register Value JES20040127EEPROM */
	Set_PHYSet(ipg_ethernet_device);
	return 0;
}

static int ipg_sti_fiber_detect(struct net_device *ipg_ethernet_device)
{
	/* Determine if NIC is fiber based by reading the PhyMedia
	 * bit in the AsicCtrl register.
	 */

	u32				asicctrl;
	unsigned long				baseaddr;

	IPG_DEBUG_MSG("_sti_fiber_detect\n");

	baseaddr = ipg_ethernet_device->base_addr;
	asicctrl = IPG_READ_ASICCTRL(baseaddr);

        if (asicctrl & IPG_AC_PHY_MEDIA)
	{
		/* Fiber NIC. */
		return 1;
	} else
	{
		/* Not a fiber NIC. */
		return 0;
	}
}

static int ipg_tmi_fiber_detect(struct net_device *ipg_ethernet_device,
	                        int phyaddr)
{
	/* Determine if NIC is fiber based by reading the ID register
	 * of the PHY and the GMII address.
	 */

	u16				phyid;
	unsigned long				baseaddr;

	IPG_DEBUG_MSG("_tmi_fiber_detect\n");

	baseaddr = ipg_ethernet_device->base_addr;
	phyid = read_phy_register(ipg_ethernet_device,
	                          phyaddr, GMII_PHY_ID_1);

	IPG_DEBUG_MSG("PHY ID = %x\n", phyid);

	/* We conclude the mode is fiber if the GMII address
	 * is 0x1 and the PHY ID is 0x0000.
	 */
        if ((phyaddr == 0x1) && (phyid == 0x0000))
	{
		/* Fiber NIC. */
		return 1;
	} else
	{
		/* Not a fiber NIC. */
		return 0;
	}
}

static int ipg_find_phyaddr(struct net_device *ipg_ethernet_device)
{
	/* Find the GMII PHY address. */

	int	i;
	int	phyaddr;
	u32	status;

        for(i=0;i<32;i++)
        {
                /* Search for the correct PHY address among 32 possible. */
                phyaddr = (IPG_NIC_PHY_ADDRESS + i) % 32;

				/* 10/22/03 Grace change verify from GMII_PHY_STATUS to
				   GMII_PHY_ID1
				 */

                status = read_phy_register(ipg_ethernet_device,
                                           phyaddr, GMII_PHY_STATUS);

 				// if (status != 0xFFFF)
                if ((status != 0xFFFF) && (status != 0))
                      return phyaddr;

                /*----------------------------------------------------
                status = read_phy_register(ipg_ethernet_device,
                						   phyaddr, GMII_PHY_ID_1);
				if (status == 0x243) {
					printk("PHY Addr = %x\n", phyaddr);
					return phyaddr;
				}
				-------------------------------------------------*/
        }

	return -1;
}

#ifdef NOTGRACE
static int ipg_config_autoneg(struct net_device *ipg_ethernet_device)
{
	/* Configure IPG based on result of IEEE 802.3 PHY
	 * auto-negotiation.
	 */

        int                             phyaddr = 0;
	u8				phyctrl;
	u32				asicctrl;
	unsigned long				baseaddr;
        u16                             status = 0;
	u16				advertisement;
	u16				linkpartner_ability;
	u16				gigadvertisement;
	u16				giglinkpartner_ability;
        u16                             techabilities;
        int                             fiber;
        int                             gig;
        int                             fullduplex;
        int                             txflowcontrol;
        int                             rxflowcontrol;
	struct ipg_nic_private	*sp = (struct ipg_nic_private *)
					      ipg_ethernet_device->priv;

	IPG_DEBUG_MSG("_config_autoneg\n");
	printk("NOTGRACE\n");

	baseaddr = ipg_ethernet_device->base_addr;
	asicctrl = IPG_READ_ASICCTRL(baseaddr);
    phyctrl = IPG_READ_PHYCTRL(baseaddr);

	/* Set flags for use in resolving auto-negotation, assuming
	 * non-1000Mbps, half duplex, no flow control.
	 */
	fiber = 0;
	fullduplex = 0;
	txflowcontrol = 0;
	rxflowcontrol = 0;
	gig = 0;

	/* To accomodate a problem in 10Mbps operation,
	 * set a global flag if PHY running in 10Mbps mode.
	 */
 	sp->tenmbpsmode = 0;

	printk("Link speed = ");

	/* Determine actual speed of operation. */
	switch (phyctrl & IPG_PC_LINK_SPEED)
	{
		case IPG_PC_LINK_SPEED_10MBPS :
			printk("10Mbps.\n");
                	printk(KERN_INFO "%s: 10Mbps operational mode enabled.\n",ipg_ethernet_device->name);
	 		sp->tenmbpsmode = 1;
			break;
		case IPG_PC_LINK_SPEED_100MBPS :
			printk("100Mbps.\n");
			break;
		case IPG_PC_LINK_SPEED_1000MBPS :
			printk("1000Mbps.\n");
			gig = 1;
			break;
		default : printk("undefined!\n");
	}

#ifndef IPG_TMI_FIBER_DETECT
	fiber = ipg_sti_fiber_detect(ipg_ethernet_device);

        /* Determine if auto-negotiation resolution is necessary.
	 * First check for fiber based media 10/100 media.
	 */
        if ((fiber == 1) && (asicctrl &
	    (IPG_AC_PHY_SPEED10 | IPG_AC_PHY_SPEED100)))
        {
                printk(KERN_INFO "%s: Fiber based PHY, setting full duplex, no flow control.\n", ipg_ethernet_device->name);
                return -EILSEQ;
                IPG_WRITE_MACCTRL(baseaddr, (IPG_READ_MACCTRL(baseaddr) |
                                                IPG_MC_DUPLEX_SELECT_FD) &
                                     ~IPG_MC_TX_FLOW_CONTROL_ENABLE &
                                     ~IPG_MC_RX_FLOW_CONTROL_ENABLE);

                return 0;
        }
#endif

        /* Determine if PHY is auto-negotiation capable. */
	phyaddr = ipg_find_phyaddr(ipg_ethernet_device);

        if (phyaddr == -1)
        {
                printk(KERN_INFO "%s: Error on read to GMII/MII Status register.\n",ipg_ethernet_device->name);
                return -EILSEQ;
        }

	IPG_DEBUG_MSG("GMII/MII PHY address = %x\n", phyaddr);

        status = read_phy_register(ipg_ethernet_device,
                                   phyaddr, GMII_PHY_STATUS);

	printk("PHYStatus = %x \n", status);
        if ((status & GMII_PHY_STATUS_AUTONEG_ABILITY) == 0)
        {
                printk(KERN_INFO "%s: Error PHY unable to perform auto-negotiation.\n",
			ipg_ethernet_device->name);
                return -EILSEQ;
        }

	advertisement = read_phy_register(ipg_ethernet_device, phyaddr,
	                                  GMII_PHY_AUTONEGADVERTISEMENT);
	linkpartner_ability = read_phy_register(ipg_ethernet_device, phyaddr,
	                                        GMII_PHY_AUTONEGLINKPARTABILITY);

	printk("PHYadvertisement=%x LinkPartner=%x \n",advertisement,linkpartner_ability);
	if ((advertisement == 0xFFFF) || (linkpartner_ability == 0xFFFF))
	{
		printk(KERN_INFO "%s: Error on read to GMII/MII registers 4 and/or 5.\n", ipg_ethernet_device->name);
		return -EILSEQ;
	}

#ifdef IPG_TMI_FIBER_DETECT
	fiber = ipg_tmi_fiber_detect(ipg_ethernet_device, phyaddr);
#endif

        /* Resolve full/half duplex if 1000BASE-X. */
	if ((gig == 1) && (fiber == 1))
        {
		/* Compare the full duplex bits in the GMII registers
		 * for the local device, and the link partner. If these
		 * bits are logic 1 in both registers, configure the
		 * IPG for full duplex operation.
		 */
		if ((advertisement & GMII_PHY_ADV_FULL_DUPLEX) ==
		    (linkpartner_ability & GMII_PHY_ADV_FULL_DUPLEX))
		{
			fullduplex = 1;

			/* In 1000BASE-X using IPG's internal PCS
			 * layer, so write to the GMII duplex bit.
			 */
			write_phy_register(ipg_ethernet_device,
			                   phyaddr,
			                   GMII_PHY_CONTROL,
			                   read_phy_register(ipg_ethernet_device,
			                   phyaddr,
			                   GMII_PHY_CONTROL) |
			                   GMII_PHY_CONTROL_FULL_DUPLEX);

		} else
		{
			fullduplex = 0;

			/* In 1000BASE-X using IPG's internal PCS
			 * layer, so write to the GMII duplex bit.
			 */
			write_phy_register(ipg_ethernet_device,
			                   phyaddr,
			                   GMII_PHY_CONTROL,
			                   read_phy_register(ipg_ethernet_device,
			                   phyaddr,
			                   GMII_PHY_CONTROL) &
			                   ~GMII_PHY_CONTROL_FULL_DUPLEX);
		}
	}

        /* Resolve full/half duplex if 1000BASE-T. */
	if ((gig == 1) && (fiber == 0))
        {
		/* Read the 1000BASE-T "Control" and "Status"
		 * registers which represent the advertised and
		 * link partner abilities exchanged via next page
		 * transfers.
		 */
		gigadvertisement = read_phy_register(ipg_ethernet_device,
		                                     phyaddr,
	                                         GMII_PHY_1000BASETCONTROL);
		giglinkpartner_ability = read_phy_register(ipg_ethernet_device,
		                                           phyaddr,
	                                          GMII_PHY_1000BASETSTATUS);

		/* Compare the full duplex bits in the 1000BASE-T GMII
		 * registers for the local device, and the link partner.
		 * If these bits are logic 1 in both registers, configure
		 * the IPG for full duplex operation.
		 */
		if ((gigadvertisement & GMII_PHY_1000BASETCONTROL_FULL_DUPLEX) &&
		    (giglinkpartner_ability & GMII_PHY_1000BASETSTATUS_FULL_DUPLEX))
		{
			fullduplex = 1;
		} else
		{
			fullduplex = 0;
		}
	}

        /* Resolve full/half duplex for 10/100BASE-T. */
	if (gig == 0)
	{
	        /* Autonegotiation Priority Resolution algorithm, as defined in
	       	 * IEEE 802.3 Annex 28B.
	       	 */
	        if (((advertisement & MII_PHY_SELECTORFIELD) ==
	             MII_PHY_SELECTOR_IEEE8023) &&
	            ((linkpartner_ability & MII_PHY_SELECTORFIELD) ==
	             MII_PHY_SELECTOR_IEEE8023))
	        {
	                techabilities = (advertisement & linkpartner_ability &
	                                 MII_PHY_TECHABILITYFIELD);

	                fullduplex = 0;

	                /* 10BASE-TX half duplex is lowest priority. */
	                if (techabilities & MII_PHY_TECHABILITY_10BT)
	                {
	                        fullduplex = 0;
	                }

	                if (techabilities & MII_PHY_TECHABILITY_10BTFD)
	                {
	                        fullduplex = 1;
	                }

	                if (techabilities & MII_PHY_TECHABILITY_100BTX)
	                {
	                        fullduplex = 0;
	                }

	                if (techabilities & MII_PHY_TECHABILITY_100BT4)
	                {
	                        fullduplex = 0;
	                }

	                /* 100BASE-TX half duplex is highest priority. */ //Sorbica full duplex ?
	                if (techabilities & MII_PHY_TECHABILITY_100BTXFD)
	                {
	                        fullduplex = 1;
	                }

	                if (fullduplex == 1)
	                {
	                        /* If in full duplex mode, determine if PAUSE
	                         * functionality is supported by the local
				 * device, and the link partner.
	                         */
	                        if (techabilities & MII_PHY_TECHABILITY_PAUSE)
	                        {
					txflowcontrol = 1;
					rxflowcontrol = 1;
	                        }
	                        else
	                        {
					txflowcontrol = 0;
					rxflowcontrol = 0;
	                        }
	                }
	        }
	}

	/* If in 1000Mbps, fiber, and full duplex mode, resolve
	 * 1000BASE-X PAUSE capabilities. */
	if ((fullduplex == 1) && (fiber == 1) && (gig == 1))
	{
		/* In full duplex mode, resolve PAUSE
		 * functionality.
		 */
		switch(((advertisement & GMII_PHY_ADV_PAUSE) >> 5) |
		       ((linkpartner_ability & GMII_PHY_ADV_PAUSE) >> 7))
		{
			case 0x7 :
			txflowcontrol = 1;
			rxflowcontrol = 0;
			break;

			case 0xA : case 0xB: case 0xE: case 0xF:
			txflowcontrol = 1;
			rxflowcontrol = 1;
			break;

			case 0xD :
			txflowcontrol = 0;
			rxflowcontrol = 1;
			break;

			default :
			txflowcontrol = 0;
			rxflowcontrol = 0;
		}
	}

	/* If in 1000Mbps, non-fiber, full duplex mode, resolve
	 * 1000BASE-T PAUSE capabilities. */
	if ((fullduplex == 1) && (fiber == 0) && (gig == 1))
	{
		/* Make sure the PHY is advertising we are PAUSE
		 * capable.
		 */
		if (!(advertisement & (MII_PHY_TECHABILITY_PAUSE |
		                       MII_PHY_TECHABILITY_ASM_DIR)))
		{
			/* PAUSE is not being advertised. Advertise
			 * PAUSE and restart auto-negotiation.
			 */
			write_phy_register(ipg_ethernet_device,
		                           phyaddr,
			                   MII_PHY_AUTONEGADVERTISEMENT,
			                   (advertisement |
                                            MII_PHY_TECHABILITY_PAUSE |
			                    MII_PHY_TECHABILITY_ASM_DIR));
			write_phy_register(ipg_ethernet_device,
		                           phyaddr,
			                   MII_PHY_CONTROL,
			                   MII_PHY_CONTROL_RESTARTAN);

			return -EAGAIN;
		}

		/* In full duplex mode, resolve PAUSE
		 * functionality.
		 */
		switch(((advertisement &
		         MII_PHY_TECHABILITY_PAUSE_FIELDS) >> 0x8) |
		       ((linkpartner_ability &
		         MII_PHY_TECHABILITY_PAUSE_FIELDS) >> 0xA))
		{
			case 0x7 :
			txflowcontrol = 1;
			rxflowcontrol = 0;
			break;

			case 0xA : case 0xB: case 0xE: case 0xF:
			txflowcontrol = 1;
			rxflowcontrol = 1;
			break;

			case 0xD :
			txflowcontrol = 0;
			rxflowcontrol = 1;
			break;

			default :
			txflowcontrol = 0;
			rxflowcontrol = 0;
		}
	}

	/* If in 10/100Mbps, non-fiber, full duplex mode, assure
	 * 10/100BASE-T PAUSE capabilities are advertised. */
	if ((fullduplex == 1) && (fiber == 0) && (gig == 0))
	{
		/* Make sure the PHY is advertising we are PAUSE
		 * capable.
		 */
		if (!(advertisement & (MII_PHY_TECHABILITY_PAUSE)))
		{
			/* PAUSE is not being advertised. Advertise
			 * PAUSE and restart auto-negotiation.
			 */
			write_phy_register(ipg_ethernet_device,
		                           phyaddr,
			                   MII_PHY_AUTONEGADVERTISEMENT,
			                   (advertisement |
                                            MII_PHY_TECHABILITY_PAUSE));
			write_phy_register(ipg_ethernet_device,
		                           phyaddr,
			                   MII_PHY_CONTROL,
			                   MII_PHY_CONTROL_RESTARTAN);

			return -EAGAIN;
		}

	}

        if (fiber == 1)
	{
        	printk(KERN_INFO "%s: Fiber based PHY, ",
		       ipg_ethernet_device->name);
	} else
	{
        	printk(KERN_INFO "%s: Copper based PHY, ",
		       ipg_ethernet_device->name);
	}

	/* Configure full duplex, and flow control. */
	if (fullduplex == 1)
	{
		/* Configure IPG for full duplex operation. */
        	printk("setting full duplex, ");

		IPG_WRITE_MACCTRL(baseaddr, IPG_READ_MACCTRL(baseaddr) |
	                             IPG_MC_DUPLEX_SELECT_FD);

		if (txflowcontrol == 1)
		{
        		printk("TX flow control");
			IPG_WRITE_MACCTRL(baseaddr,
			                     IPG_READ_MACCTRL(baseaddr) |
			                     IPG_MC_TX_FLOW_CONTROL_ENABLE);
		} else
		{
        		printk("no TX flow control");
			IPG_WRITE_MACCTRL(baseaddr,
			                     IPG_READ_MACCTRL(baseaddr) &
			                     ~IPG_MC_TX_FLOW_CONTROL_ENABLE);
		}

		if (rxflowcontrol == 1)
		{
        		printk(", RX flow control.");
			IPG_WRITE_MACCTRL(baseaddr,
			                     IPG_READ_MACCTRL(baseaddr) |
			                     IPG_MC_RX_FLOW_CONTROL_ENABLE);
		} else
		{
        		printk(", no RX flow control.");
			IPG_WRITE_MACCTRL(baseaddr,
			                     IPG_READ_MACCTRL(baseaddr) &
			                     ~IPG_MC_RX_FLOW_CONTROL_ENABLE);
		}

		printk("\n");
	} else
	{
		/* Configure IPG for half duplex operation. */
	        printk("setting half duplex, no TX flow control, no RX flow control.\n");

		IPG_WRITE_MACCTRL(baseaddr, IPG_READ_MACCTRL(baseaddr) &
		                     ~IPG_MC_DUPLEX_SELECT_FD &
		                     ~IPG_MC_TX_FLOW_CONTROL_ENABLE &
		                     ~IPG_MC_RX_FLOW_CONTROL_ENABLE);
	}


	IPG_DEBUG_MSG("G/MII reg 4 (advertisement) = %4.4x\n", advertisement);
	IPG_DEBUG_MSG("G/MII reg 5 (link partner)  = %4.4x\n", linkpartner_ability);
	IPG_DEBUG_MSG("G/MII reg 9 (1000BASE-T control) = %4.4x\n", advertisement);
	IPG_DEBUG_MSG("G/MII reg 10 (1000BASE-T status) = %4.4x\n", linkpartner_ability);

	IPG_DEBUG_MSG("Auto-neg complete, MACCTRL = %8.8x\n",
	          IPG_READ_MACCTRL(baseaddr));

	return 0;
}
#else
static int ipg_config_autoneg(struct net_device *ipg_ethernet_device)
{
	/* Configure IPG based on result of IEEE 802.3 PHY
	 * auto-negotiation.
	 */

//    int                             phyaddr = 0;
	u8				phyctrl;
	u32				asicctrl;
	unsigned long				baseaddr;
//    u16                             status = 0;
//	u16				advertisement;
//	u16				linkpartner_ability;
//	u16				gigadvertisement;
//	u16				giglinkpartner_ability;
//        u16                             techabilities;
        int                             fiber;
        int                             gig;
        int                             fullduplex;
        int                             txflowcontrol;
        int                             rxflowcontrol;
	struct ipg_nic_private	*sp = (struct ipg_nic_private *)
					      ipg_ethernet_device->priv;

//	IPG_DEBUG_MSG("_config_autoneg\n");

 //       printk("TGRACE\n");
	baseaddr = ipg_ethernet_device->base_addr;
	asicctrl = IPG_READ_ASICCTRL(baseaddr);
        phyctrl = IPG_READ_PHYCTRL(baseaddr);

	/* Set flags for use in resolving auto-negotation, assuming
	 * non-1000Mbps, half duplex, no flow control.
	 */
	fiber = 0;
	fullduplex = 0;
	txflowcontrol = 0;
	rxflowcontrol = 0;
	gig = 0;

	/* To accomodate a problem in 10Mbps operation,
	 * set a global flag if PHY running in 10Mbps mode.
	 */
 	sp->tenmbpsmode = 0;

	printk("Link speed = ");

	/* Determine actual speed of operation. */
	switch (phyctrl & IPG_PC_LINK_SPEED)
	{
		case IPG_PC_LINK_SPEED_10MBPS :
			printk("10Mbps.\n");
                	printk(KERN_INFO "%s: 10Mbps operational mode enabled.\n",ipg_ethernet_device->name);
	 		sp->tenmbpsmode = 1;
			break;
		case IPG_PC_LINK_SPEED_100MBPS :
			printk("100Mbps.\n");
			break;
		case IPG_PC_LINK_SPEED_1000MBPS :
			printk("1000Mbps.\n");
			gig = 1;
			break;
		default : printk("undefined!\n");
				  return 0;
	}

	if ( phyctrl & IPG_PC_DUPLEX_STATUS)
	{
		fullduplex = 1;
		txflowcontrol = 1;
		rxflowcontrol = 1;
	}
	else
	{
		fullduplex = 0;
		txflowcontrol = 0;
		rxflowcontrol = 0;
	}


	/* Configure full duplex, and flow control. */
	if (fullduplex == 1)
	{
		/* Configure IPG for full duplex operation. */
        printk("setting full duplex, ");

		IPG_WRITE_MACCTRL(baseaddr, IPG_READ_MACCTRL(baseaddr) |
	                             IPG_MC_DUPLEX_SELECT_FD);

		if (txflowcontrol == 1)
		{
        	printk("TX flow control");
			IPG_WRITE_MACCTRL(baseaddr,
			                     IPG_READ_MACCTRL(baseaddr) |
			                     IPG_MC_TX_FLOW_CONTROL_ENABLE);
		} else
		{
        	printk("no TX flow control");
			IPG_WRITE_MACCTRL(baseaddr,
			                     IPG_READ_MACCTRL(baseaddr) &
			                     ~IPG_MC_TX_FLOW_CONTROL_ENABLE);
		}

		if (rxflowcontrol == 1)
		{
        		printk(", RX flow control.");
			IPG_WRITE_MACCTRL(baseaddr,
			                     IPG_READ_MACCTRL(baseaddr) |
			                     IPG_MC_RX_FLOW_CONTROL_ENABLE);
		} else
		{
        		printk(", no RX flow control.");
			IPG_WRITE_MACCTRL(baseaddr,
			                     IPG_READ_MACCTRL(baseaddr) &
			                     ~IPG_MC_RX_FLOW_CONTROL_ENABLE);
		}

		printk("\n");
	} else
	{
		/* Configure IPG for half duplex operation. */
	        printk("setting half duplex, no TX flow control, no RX flow control.\n");

		IPG_WRITE_MACCTRL(baseaddr, IPG_READ_MACCTRL(baseaddr) &
		                     ~IPG_MC_DUPLEX_SELECT_FD &
		                     ~IPG_MC_TX_FLOW_CONTROL_ENABLE &
		                     ~IPG_MC_RX_FLOW_CONTROL_ENABLE);
	}
	return 0;
}

#endif

static int ipg_io_config(struct net_device *ipg_ethernet_device)
{
	/* Initialize the IPG I/O registers. */

	unsigned long		baseaddr;
	u32		origmacctrl;
	u32		restoremacctrl;

	IPG_DEBUG_MSG("_io_config\n");

	baseaddr = ipg_ethernet_device->base_addr;

	/* Save the original value of MACCTRL. */
	origmacctrl = IPG_READ_MACCTRL(baseaddr);

	/* Establish a vlaue to restore MACCTRL when done. */
	restoremacctrl = origmacctrl;

	/* Enable statistics gathering. */
	restoremacctrl |= IPG_MC_STATISTICS_ENABLE;

        /* Based on compilation option, determine if FCS is to be
         * stripped on receive frames by IPG.
         */
        if (!(IPG_STRIP_FCS_ON_RX))
        {
		restoremacctrl |= IPG_MC_RCV_FCS;
        }

	/* Determine if transmitter and/or receiver are
	 * enabled so we may restore MACCTRL correctly.
	 */
	if (origmacctrl & IPG_MC_TX_ENABLED)
	{
		restoremacctrl |= IPG_MC_TX_ENABLE;
	}

	if (origmacctrl & IPG_MC_RX_ENABLED)
	{
		restoremacctrl |= IPG_MC_RX_ENABLE;
	}

	/* Transmitter and receiver must be disabled before setting
	 * IFSSelect.
	 */
	IPG_WRITE_MACCTRL(baseaddr, origmacctrl & (IPG_MC_RX_DISABLE |
	                     IPG_MC_TX_DISABLE));

	/* Now that transmitter and receiver are disabled, write
	 * to IFSSelect.
	 */
	IPG_WRITE_MACCTRL(baseaddr, origmacctrl & IPG_MC_IFS_96BIT);

	/* Set RECEIVEMODE register. */
	ipg_nic_set_multicast_list(ipg_ethernet_device);

	IPG_WRITE_MAXFRAMESIZE(baseaddr, IPG_MAX_RXFRAME_SIZE);
	IPG_WRITE_RXEARLYTHRESH(baseaddr, IPG_RXEARLYTHRESH_VALUE);
	IPG_WRITE_TXSTARTTHRESH(baseaddr, IPG_TXSTARTTHRESH_VALUE);
	IPG_WRITE_RXDMAINTCTRL(baseaddr, (IPG_RI_RSVD_MASK &
	                              ((IPG_RI_RXFRAME_COUNT &
	                                IPG_RXFRAME_COUNT) |
	                               (IPG_RI_PRIORITY_THRESH &
	                                (IPG_PRIORITY_THRESH << 12)) |
	                               (IPG_RI_RXDMAWAIT_TIME &
	                                (IPG_RXDMAWAIT_TIME << 16)))));
	IPG_WRITE_RXDMAPOLLPERIOD(baseaddr, IPG_RXDMAPOLLPERIOD_VALUE);
	IPG_WRITE_RXDMAURGENTTHRESH(baseaddr,
	                               IPG_RXDMAURGENTTHRESH_VALUE);
	IPG_WRITE_RXDMABURSTTHRESH(baseaddr,
	                              IPG_RXDMABURSTTHRESH_VALUE);
	IPG_WRITE_TXDMAPOLLPERIOD(baseaddr,
	                             IPG_TXDMAPOLLPERIOD_VALUE);
	IPG_WRITE_TXDMAURGENTTHRESH(baseaddr,
	                               IPG_TXDMAURGENTTHRESH_VALUE);
	IPG_WRITE_TXDMABURSTTHRESH(baseaddr,
	                              IPG_TXDMABURSTTHRESH_VALUE);
	IPG_WRITE_INTENABLE(baseaddr, IPG_IE_HOST_ERROR |
	                          IPG_IE_TX_DMA_COMPLETE |
	                          IPG_IE_TX_COMPLETE |
	                          IPG_IE_INT_REQUESTED |
	                          IPG_IE_UPDATE_STATS |
	                          IPG_IE_LINK_EVENT |
	                          IPG_IE_RX_DMA_COMPLETE |
	                          //IPG_IE_RFD_LIST_END |   //20041019Jesse_For_SmartBit: remove
	                          IPG_IE_RX_DMA_PRIORITY);

	IPG_WRITE_FLOWONTHRESH(baseaddr, IPG_FLOWONTHRESH_VALUE);
	IPG_WRITE_FLOWOFFTHRESH(baseaddr, IPG_FLOWOFFTHRESH_VALUE);

	/* IPG multi-frag frame bug workaround.
	 * Per silicon revision B3 eratta.
	 */
	IPG_WRITE_DEBUGCTRL(baseaddr,
	                       IPG_READ_DEBUGCTRL(baseaddr) | 0x0200);

	/* IPG TX poll now bug workaround.
	 * Per silicon revision B3 eratta.
	 */
	IPG_WRITE_DEBUGCTRL(baseaddr,
	                       IPG_READ_DEBUGCTRL(baseaddr) | 0x0010);

	/* IPG RX poll now bug workaround.
	 * Per silicon revision B3 eratta.
	 */
	IPG_WRITE_DEBUGCTRL(baseaddr,
	                       IPG_READ_DEBUGCTRL(baseaddr) | 0x0020);

	/* Now restore MACCTRL to original setting. */
	IPG_WRITE_MACCTRL(baseaddr, restoremacctrl);

	/* Disable unused RMON statistics. */
	IPG_WRITE_RMONSTATISTICSMASK(baseaddr, IPG_RZ_ALL);

	/* Disable unused MIB statistics. */
	IPG_WRITE_STATISTICSMASK(baseaddr,
	                     IPG_SM_MACCONTROLFRAMESXMTD |
	                     IPG_SM_BCSTOCTETXMTOK_BCSTFRAMESXMTDOK |
	                     IPG_SM_MCSTOCTETXMTOK_MCSTFRAMESXMTDOK |
	                     IPG_SM_MACCONTROLFRAMESRCVD |
	                     IPG_SM_BCSTOCTETRCVDOK_BCSTFRAMESRCVDOK |
	                     IPG_SM_TXJUMBOFRAMES |
	                     IPG_SM_UDPCHECKSUMERRORS |
	                     IPG_SM_IPCHECKSUMERRORS |
	                     IPG_SM_TCPCHECKSUMERRORS |
	                     IPG_SM_RXJUMBOFRAMES);

	return 0;
}

static void ipg_nic_txcleanup(struct net_device *ipg_ethernet_device)
{
	/* For TxComplete interrupts, free all transmit
	 * buffers which have already been transfered via DMA
	 * to the IPG.
	 */

	int				maxtfdcount;
	unsigned long				baseaddr;
	u32				txstatusdword;
	struct ipg_nic_private	*sp = (struct ipg_nic_private *)
					      ipg_ethernet_device->priv;

	IPG_DEBUG_MSG("_nic_txcleanup\n");

	baseaddr = ipg_ethernet_device->base_addr;
	maxtfdcount = IPG_TFDLIST_LENGTH;

	do
	{
		/* Reading the TXSTATUS register clears the
		 * TX_COMPLETE interrupt.
		 */
		txstatusdword = IPG_READ_TXSTATUS(baseaddr);

		IPG_DEBUG_MSG("TxStatus = %8.8x\n",txstatusdword);

		/* Check for Transmit errors. Error bits only valid if
		 * TX_COMPLETE bit in the TXSTATUS register is a 1.
		 */
		if (txstatusdword & IPG_TS_TX_COMPLETE)
		{

			/* If in 10Mbps mode, indicate transmit is ready. */
/*			if (sp->tenmbpsmode)
			{
				spin_lock(&sp->lock);
				IPG_TX_NOTBUSY(ipg_ethernet_device);
				spin_unlock(&sp->lock);
			}*/

			/* Transmit error, increment stat counters. */
			if (txstatusdword & IPG_TS_TX_ERROR)
			{
				IPG_DEBUG_MSG("Transmit error.\n");
				sp->stats.tx_errors++;
			}

			/* Late collision, re-enable transmitter. */
			if (txstatusdword & IPG_TS_LATE_COLLISION)
			{
				IPG_DEBUG_MSG("Late collision on transmit.\n");
				IPG_WRITE_MACCTRL(baseaddr,
				  IPG_READ_MACCTRL(baseaddr) |
				  IPG_MC_TX_ENABLE);
			}

			/* Maximum collisions, re-enable transmitter. */
			if (txstatusdword & IPG_TS_TX_MAX_COLL)
			{
				IPG_DEBUG_MSG("Maximum collisions on transmit.\n");

				IPG_WRITE_MACCTRL(baseaddr,
				  IPG_READ_MACCTRL(baseaddr) |
				  IPG_MC_TX_ENABLE);
			}

			/* Transmit underrun, reset and re-enable
			 * transmitter.
			 */
			if (txstatusdword & IPG_TS_TX_UNDERRUN)
			{
				IPG_DEBUG_MSG("Transmitter underrun.\n");
				sp->stats.tx_fifo_errors++;
				ipg_reset(ipg_ethernet_device, IPG_AC_TX_RESET |
				                       IPG_AC_DMA |
				                       IPG_AC_NETWORK);

				/* Re-configure after DMA reset. */
				if ((ipg_io_config(ipg_ethernet_device) < 0) ||
				    (init_tfdlist(ipg_ethernet_device) < 0))
				{
					printk(KERN_INFO "%s: Error during re-configuration.\n",
				               ipg_ethernet_device->name);
				}

				IPG_WRITE_MACCTRL(baseaddr,
				  IPG_READ_MACCTRL(baseaddr) |
				  IPG_MC_TX_ENABLE);
			}
		}
		else
			break;

		maxtfdcount--;

	}
	while(maxtfdcount != 0);

	ipg_nic_txfree(ipg_ethernet_device);

	return;
}

static void ipg_nic_txfree(struct net_device *ipg_ethernet_device)
{
        /* Free all transmit buffers which have already been transfered
         * via DMA to the IPG.
         */

        int                             NextToFree;
        int                             maxtfdcount;
	struct ipg_nic_private	*sp = (struct ipg_nic_private *)
					      ipg_ethernet_device->priv;

//	IPG_DEBUG_MSG("_nic_txfree\n");

	maxtfdcount = IPG_TFDLIST_LENGTH;

	/* Set the CurrentTxFrameID to skip the next
	 * TxDMACompleteInterrupt.
	 */
	sp->CurrentTxFrameID = 1;

	do
	{
		/* Calculate next TFD to release. */
		NextToFree = (sp->LastFreedTxBuff + 1) % IPG_TFDLIST_LENGTH;

//		IPG_DEBUG_MSG("TFC = %16.16lx\n", (unsigned long int)
//		                 sp->TFDList[NextToFree].TFC);

		/* Look at each TFD's TFC field beginning
		 * at the last freed TFD up to the current TFD.
		 * If the TFDDone bit is set, free the associated
		 * buffer.
		 */
		if (((sp->TFDList[NextToFree].TFC) &
		     IPG_TFC_TFDDONE) &&
		    (NextToFree != sp->CurrentTFD))
		{
			/* Free the transmit buffer. */
			if (sp->TxBuff[NextToFree] != NULL)
			{
				pci_unmap_single(sp->ipg_pci_device,
		                 sp->TxBuffDMAhandle[NextToFree].dmahandle,
				 sp->TxBuffDMAhandle[NextToFree].len,
				 PCI_DMA_TODEVICE);

				dev_kfree_skb(sp->TxBuff[NextToFree]);

				sp->TxBuff[NextToFree] = NULL;
			}

			sp->LastFreedTxBuff = NextToFree;
		}
                else
                        break;

                maxtfdcount--;

        } while(maxtfdcount != 0);

	return;
}


static int init_rfdlist(struct net_device *ipg_ethernet_device)
{
	/* Initialize the RFDList. */

	int				i;
	unsigned long				baseaddr;
	struct ipg_nic_private	*sp = (struct ipg_nic_private *)
					      ipg_ethernet_device->priv;

	IPG_DEBUG_MSG("_init_rfdlist\n");

	baseaddr = ipg_ethernet_device->base_addr;

	/* Clear the receive buffer not ready flag. */
	sp->RxBuffNotReady = 0;

	for(i=0; i<IPG_RFDLIST_LENGTH; i++)
	{
		/* Free any allocated receive buffers. */
		if((sp->RxBuff[i] != NULL) && sp->RxBuffDMAhandle[i].len && sp->RxBuffDMAhandle[i].dmahandle)
		pci_unmap_single(sp->ipg_pci_device,
                                 sp->RxBuffDMAhandle[i].dmahandle,
		                 sp->RxBuffDMAhandle[i].len,
		                 PCI_DMA_FROMDEVICE);
		if (sp->RxBuff[i] != NULL)
			dev_kfree_skb(sp->RxBuff[i]);
		sp->RxBuff[i] = NULL;

		/* Clear out the RFS field. */
	 	sp->RFDList[i].RFS = 0x0000000000000000;

		if (ipg_get_rxbuff(ipg_ethernet_device, i) < 0)
		{
			/* A receive buffer was not ready, break the
			 * RFD list here and set the receive buffer
			 * not ready flag.
			 */
			sp->RxBuffNotReady = 1;

			IPG_DEBUG_MSG("Cannot allocate Rx buffer.\n");

			/* Just in case we cannot allocate a single RFD.
			 * Should not occur.
			 */
			if (i == 0)
			{
				printk(KERN_ERR "%s: No memory available for RFD list.\n",
				       ipg_ethernet_device->name);
				return -ENOMEM;
			}
		}

		/* Set up RFDs to point to each other. A ring structure. */
	 	sp->RFDList[i].RFDNextPtr = (
		                            sp->RFDListDMAhandle +
		                            ((sizeof(struct RFD)) *
		                             ((i + 1) %
		                              IPG_RFDLIST_LENGTH)));
	}

	sp->CurrentRFD = 0;
	sp->LastRestoredRxBuff = i - 1;

	/* Write the location of the RFDList to the IPG. */

	IPG_WRITE_RFDLISTPTR0(baseaddr, (u32)(sp->RFDListDMAhandle));
	IPG_WRITE_RFDLISTPTR1(baseaddr, 0x00000000);

	return 0;
}

static int init_tfdlist(struct net_device *ipg_ethernet_device)
{
	/* Initialize TFDList. */

	int				i;
	unsigned long				baseaddr;
	struct ipg_nic_private	*sp = (struct ipg_nic_private *)
					      ipg_ethernet_device->priv;

	IPG_DEBUG_MSG("_init_tfdlist\n");

	baseaddr = ipg_ethernet_device->base_addr;

	for(i=0; i<IPG_TFDLIST_LENGTH; i++)
	{
	 	sp->TFDList[i].TFDNextPtr = (
		                            sp->TFDListDMAhandle +
		                            ((sizeof(struct TFD)) *
		                             ((i + 1) %
		                              IPG_TFDLIST_LENGTH)));

	 	sp->TFDList[i].TFC = (IPG_TFC_TFDDONE);
		if (sp->TxBuff[i] != NULL)
			dev_kfree_skb(sp->TxBuff[i]);
		sp->TxBuff[i] = NULL;
	}

	sp->CurrentTFD = IPG_TFDLIST_LENGTH - 1;
	sp->CurrentTxFrameID = 0;
	sp->LastFreedTxBuff = IPG_TFDLIST_LENGTH - 1;

	/* Write the location of the TFDList to the IPG. */

	IPG_DDEBUG_MSG("Starting TFDListPtr = %8.8x\n",
	  (u32)(sp->TFDListDMAhandle));
	IPG_WRITE_TFDLISTPTR0(baseaddr, (u32)(sp->TFDListDMAhandle));
	IPG_WRITE_TFDLISTPTR1(baseaddr, 0x00000000);

	return 0;
}

static int ipg_get_rxbuff(struct net_device *ipg_ethernet_device, int rfd)
{
	/* Create a receive buffer within system memory and update
	 * NIC private structure appropriately.
	 */
	unsigned long long				rxfragsize;
	struct sk_buff			*skb;
	struct ipg_nic_private	*sp = (struct ipg_nic_private *)
					      ipg_ethernet_device->priv;

//	IPG_DEBUG_MSG("_get_rxbuff\n");

	/* Allocate memory buffers for receive frames. Pad by
	 * 2 to account for IP field alignment.
	 */
	skb = dev_alloc_skb(IPG_RXSUPPORT_SIZE + 2);

	if (skb == NULL)
	{
		sp->RxBuff[rfd] = NULL;
		return -ENOMEM;
	}

	/* Adjust the data start location within the buffer to
	 * align IP address field to a 16 byte boundary.
	 */
	skb_reserve(skb, 2);

	/* Associate the receive buffer with the IPG NIC. */
	skb->dev = ipg_ethernet_device;

	/* Save the address of the sk_buff structure. */
	sp->RxBuff[rfd] = skb;

	sp->RxBuffDMAhandle[rfd].len = IPG_RXSUPPORT_SIZE;
	sp->RxBuffDMAhandle[rfd].dmahandle = pci_map_single(
	                                      sp->ipg_pci_device,
	                                      skb->data, IPG_RXSUPPORT_SIZE,
	                                      PCI_DMA_FROMDEVICE);
	sp->RFDList[rfd].FragInfo = (
	                                sp->RxBuffDMAhandle[rfd].dmahandle);

	/* Set the RFD fragment length. */
	rxfragsize = IPG_RXFRAG_SIZE;
 	sp->RFDList[rfd].FragInfo |= ((rxfragsize << 48) &
	                                         IPG_RFI_FRAGLEN);

	return 0;
}

static int ipg_nic_stop(struct net_device *ipg_ethernet_device)
{
	/* Release resources requested by driver open function. */

	int	i;
	int	error;
	unsigned long	baseaddr;
	struct ipg_nic_private	*sp = (struct ipg_nic_private *)
					      ipg_ethernet_device->priv;

	IPG_DEBUG_MSG("_nic_stop\n");
	
   netif_stop_queue(ipg_ethernet_device);
   //netif_msg_ifdown(sp);

	baseaddr = ipg_ethernet_device->base_addr;

#ifdef IPG_DEBUG
//	IPG_DDEBUG_MSG("TFDunavailCount = %i\n", sp->TFDunavailCount);
//	IPG_DDEBUG_MSG("RFDlistendCount = %i\n", sp->RFDlistendCount);
//	IPG_DDEBUG_MSG("RFDListCheckedCount = %i\n", sp->RFDListCheckedCount);
//	IPG_DDEBUG_MSG("EmptyRFDListCount = %i\n", sp->EmptyRFDListCount);
//	IPG_DUMPTFDLIST(ipg_ethernet_device);
#endif
	/* Reset all functions within the IPG to shut down the
	 IP1000* .
	 */
	i = IPG_AC_GLOBAL_RESET | IPG_AC_RX_RESET |
	    IPG_AC_TX_RESET | IPG_AC_DMA |
	    IPG_AC_FIFO | IPG_AC_NETWORK |
	    IPG_AC_HOST | IPG_AC_AUTO_INIT;

	error = ipg_reset(ipg_ethernet_device, i);
	if (error < 0)
	{
		return error;
	}


	/* Free all receive buffers. */
	for(i=0; i<IPG_RFDLIST_LENGTH; i++)
	{
		if((sp->RxBuff[i] != NULL) && sp->RxBuffDMAhandle[i].len && sp->RxBuffDMAhandle[i].dmahandle)
		pci_unmap_single(sp->ipg_pci_device,
                                 sp->RxBuffDMAhandle[i].dmahandle,
		                 sp->RxBuffDMAhandle[i].len,
		                 PCI_DMA_FROMDEVICE);
		if (sp->RxBuff[i] != NULL)
			dev_kfree_skb(sp->RxBuff[i]);
		sp->RxBuff[i] = NULL;
	}

	/* Free all transmit buffers. */
	for(i=0; i<IPG_TFDLIST_LENGTH; i++)
	{
		if (sp->TxBuff[i] != NULL)
			dev_kfree_skb(sp->TxBuff[i]);
		sp->TxBuff[i] = NULL;
	}

	netif_stop_queue(ipg_ethernet_device);

	/* Free memory associated with the RFDList. */
	pci_free_consistent(sp->ipg_pci_device,
                            (sizeof(struct RFD) *
                            IPG_RFDLIST_LENGTH),
	                    sp->RFDList, sp->RFDListDMAhandle);

	/* Free memory associated with the TFDList. */
	pci_free_consistent(sp->ipg_pci_device,
                            (sizeof(struct TFD) *
                            IPG_TFDLIST_LENGTH),
	                    sp->TFDList, sp->TFDListDMAhandle);

	/* Release interrupt line. */
//	free_irq(ipg_ethernet_device->irq, ipg_ethernet_device);
         return 0;
}

static int ipg_nic_hard_start_xmit(struct sk_buff *skb,
	                          struct net_device *ipg_ethernet_device)
{
	/* Transmit an Ethernet frame. */

	unsigned long long				fraglen;
	unsigned long long				vlanvid;
	unsigned long long				vlancfi;
	unsigned long long				vlanuserpriority;
        unsigned long                   flags;
	int				NextTFD;
	unsigned long				baseaddr;
	struct ipg_nic_private	*sp = (struct ipg_nic_private *)
					      ipg_ethernet_device->priv;

//        IPG_DDEBUG_MSG("_nic_hard_start_xmit\n");

	baseaddr = ipg_ethernet_device->base_addr;


        /* Disable interrupts. */
        spin_lock_irqsave(&sp->lock, flags);

	/* If in 10Mbps mode, stop the transmit queue so
	 * no more transmit frames are accepted.
	 */
	if (sp->tenmbpsmode)
	{
		netif_stop_queue(ipg_ethernet_device);
	}

	/* Next TFD is found by incrementing the CurrentTFD
	 * counter, modulus the length of the TFDList.
	 */
	NextTFD = (sp->CurrentTFD + 1) % IPG_TFDLIST_LENGTH;

	/* Check for availability of next TFD. */
	if (!((sp->TFDList[NextTFD].TFC) &
	      IPG_TFC_TFDDONE) || (NextTFD == sp->LastFreedTxBuff))
	{
		IPG_DEBUG_MSG("Next TFD not available.\n");

		/* Attempt to free any used TFDs. */
		ipg_nic_txfree(ipg_ethernet_device);


                /* Restore interrupts. */
                spin_unlock_irqrestore(&sp->lock, flags);

#ifdef IPG_DEBUG
		/* Increment the TFDunavailCount counter. */
//		sp->TFDunavailCount++;
#endif

                return -ENOMEM;
	}

	sp->TxBuffDMAhandle[NextTFD].len = skb->len;
	sp->TxBuffDMAhandle[NextTFD].dmahandle = pci_map_single(
	                                          sp->ipg_pci_device,
	                                          skb->data, skb->len,
	                                          PCI_DMA_TODEVICE);

	/* Save the sk_buff pointer so interrupt handler can later free
	 * memory occupied by buffer.
	 */
	sp->TxBuff[NextTFD] = skb;

	/* Clear all TFC fields, except TFDDONE. */
	sp->TFDList[NextTFD].TFC = (IPG_TFC_TFDDONE);

	/* Specify the TFC field within the TFD. */
	sp->TFDList[NextTFD].
		TFC |= (IPG_TFC_WORDALIGNDISABLED |
		                   (IPG_TFC_FRAMEID &
		                    (sp->CurrentTxFrameID)) |
		                   (IPG_TFC_FRAGCOUNT & (1 << 24)));

	/* Request TxComplete interrupts at an interval defined
	 * by the constant IPG_FRAMESBETWEENTXCOMPLETES.
	 * Request TxComplete interrupt for every frame
	 * if in 10Mbps mode to accomodate problem with 10Mbps
	 * processing.
	 */
	if (sp->tenmbpsmode)
	{
		sp->TFDList[NextTFD].TFC |=
	        (IPG_TFC_TXINDICATE);
	}
	else if ((sp->CurrentTxFrameID %
	         IPG_FRAMESBETWEENTXDMACOMPLETES) == 0)
	{
		sp->TFDList[NextTFD].TFC |=
	        (IPG_TFC_TXDMAINDICATE);
	}

        /* Based on compilation option, determine if FCS is to be
         * appended to transmit frame by IPG.
         */
        if (!(IPG_APPEND_FCS_ON_TX))
        {
		sp->TFDList[NextTFD].
                        TFC |= (IPG_TFC_FCSAPPENDDISABLE);
        }

        /* Based on compilation option, determine if IP, TCP and/or
	 * UDP checksums are to be added to transmit frame by IPG.
         */
        if (IPG_ADD_IPCHECKSUM_ON_TX)
        {
		sp->TFDList[NextTFD].
                        TFC |= (IPG_TFC_IPCHECKSUMENABLE);
        }
        if (IPG_ADD_TCPCHECKSUM_ON_TX)
        {
		sp->TFDList[NextTFD].
                        TFC |= (IPG_TFC_TCPCHECKSUMENABLE);
        }
        if (IPG_ADD_UDPCHECKSUM_ON_TX)
        {
		sp->TFDList[NextTFD].
                        TFC |= (IPG_TFC_UDPCHECKSUMENABLE);
        }

        /* Based on compilation option, determine if VLAN tag info is to be
         * inserted into transmit frame by IPG.
         */
        if (IPG_INSERT_MANUAL_VLAN_TAG)
        {
		vlanvid = IPG_MANUAL_VLAN_VID;
		vlancfi = IPG_MANUAL_VLAN_CFI;
		vlanuserpriority = IPG_MANUAL_VLAN_USERPRIORITY;

		sp->TFDList[NextTFD].TFC |= (
		                            IPG_TFC_VLANTAGINSERT |
                                            (vlanvid << 32) |
			                    (vlancfi << 44) |
			                    (vlanuserpriority << 45));
        }

	/* The fragment start location within system memory is defined
	 * by the sk_buff structure's data field. The physical address
	 * of this location within the system's virtual memory space
	 * is determined using the IPG_HOST2BUS_MAP function.
	 */

	sp->TFDList[NextTFD].FragInfo = (
	                                sp->TxBuffDMAhandle[NextTFD].dmahandle);

	/* The length of the fragment within system memory is defined by
	 * the sk_buff structure's len field.
	 */
	fraglen = (u16)skb->len;
	sp->TFDList[NextTFD].FragInfo |= (IPG_TFI_FRAGLEN &
		                         (fraglen << 48));

	/* Clear the TFDDone bit last to indicate the TFD is ready
	 * for transfer to the IPG.
	 */
	sp->TFDList[NextTFD].TFC &= (~IPG_TFC_TFDDONE);

	/* Record frame transmit start time (jiffies = Linux
	 * kernel current time stamp).
	 */
	ipg_ethernet_device->trans_start = jiffies;

	/* Update current TFD indicator. */
	sp->CurrentTFD = NextTFD;

	/* Calculate the new ID for the next transmit frame by
	 * incrementing the CurrentTxFrameID counter.
	 */
	sp->CurrentTxFrameID++;

	/* Force a transmit DMA poll event. */
	IPG_WRITE_DMACTRL(baseaddr, IPG_DC_TX_DMA_POLL_NOW);

        /* Restore interrupts. */
        spin_unlock_irqrestore(&sp->lock, flags);

	return 0;
}
#ifdef JUMBO_FRAME

/* use jumboindex and jumbosize to control jumbo frame status
   initial status is jumboindex=-1 and jumbosize=0
   1. jumboindex = -1 and jumbosize=0 : previous jumbo frame has been done.
   2. jumboindex != -1 and jumbosize != 0 : jumbo frame is not over size and receiving
   3. jumboindex = -1 and jumbosize != 0 : jumbo frame is over size, already dump
                previous receiving and need to continue dumping the current one
*/
enum {NormalPacket,ErrorPacket};
enum {Frame_NoStart_NoEnd=0,Frame_WithStart=1,Frame_WithEnd=10,Frame_WithStart_WithEnd=11};
inline void ipg_nic_rx__FreeSkb(struct net_device *ipg_ethernet_device)
{
	struct ipg_nic_private	*sp = (struct ipg_nic_private *)ipg_ethernet_device->priv;
					if (sp->RxBuff[sp->CurrentRFD] != NULL)
				{
						pci_unmap_single(sp->ipg_pci_device,sp->RxBuffDMAhandle[sp->CurrentRFD].dmahandle,sp->RxBuffDMAhandle[sp->CurrentRFD].len,PCI_DMA_FROMDEVICE);
						dev_kfree_skb(sp->RxBuff[sp->CurrentRFD]);
						sp->RxBuff[sp->CurrentRFD] = NULL;
				}		
}
static inline int ipg_nic_rx__CheckFrameSEType(struct net_device *ipg_ethernet_device)
{
	struct ipg_nic_private	*sp = (struct ipg_nic_private *)ipg_ethernet_device->priv;
	int FoundStartEnd=0;
	
	if((sp->RFDList[sp->CurrentRFD].RFS)&IPG_RFS_FRAMESTART)FoundStartEnd+=1;
	if((sp->RFDList[sp->CurrentRFD].RFS)&IPG_RFS_FRAMEEND)FoundStartEnd+=10;
	return FoundStartEnd; //Frame_NoStart_NoEnd=0,Frame_WithStart=1,Frame_WithEnd=10,Frame_WithStart_WithEnd=11
}
static inline int ipg_nic_rx__CheckError(struct net_device *ipg_ethernet_device)
{
	struct ipg_nic_private	*sp = (struct ipg_nic_private *)ipg_ethernet_device->priv;
	
				   if (IPG_DROP_ON_RX_ETH_ERRORS &&
		     		((sp->RFDList[sp->CurrentRFD].RFS) &
		      		(IPG_RFS_RXFIFOOVERRUN | IPG_RFS_RXRUNTFRAME |
		       		 IPG_RFS_RXALIGNMENTERROR | IPG_RFS_RXFCSERROR |
		             IPG_RFS_RXOVERSIZEDFRAME | IPG_RFS_RXLENGTHERROR)))
			  {
					IPG_DEBUG_MSG("Rx error, RFS = %16.16lx\n",
					   (unsigned long int) sp->RFDList[sp->CurrentRFD].RFS);

					/* Increment general receive error statistic. */
					sp->stats.rx_errors++;

					/* Increment detailed receive error statistics. */
					if ((sp->RFDList[sp->CurrentRFD].RFS) & IPG_RFS_RXFIFOOVERRUN)
					{
						IPG_DEBUG_MSG("RX FIFO overrun occured.\n");

						sp->stats.rx_fifo_errors++;

						if (sp->RxBuffNotReady == 1)
						{
							/* If experience a RxFIFO overrun, and
							 * the RxBuffNotReady flag is set,
							 * assume the FIFO overran due to lack
							 * of an RFD.
							 */
							sp->stats.rx_dropped++;
						}
					}

					if ((sp->RFDList[sp->CurrentRFD].RFS) & IPG_RFS_RXRUNTFRAME)
					{
						IPG_DEBUG_MSG("RX runt occured.\n");
						sp->stats.rx_length_errors++;
					}

					/* Do nothing for IPG_RFS_RXOVERSIZEDFRAME,
					 * error count handled by a IPG statistic register.
			 		 */

					if ((sp->RFDList[sp->CurrentRFD].RFS) & IPG_RFS_RXALIGNMENTERROR)
					{
						IPG_DEBUG_MSG("RX alignment error occured.\n");
						sp->stats.rx_frame_errors++;
					}


					/* Do nothing for IPG_RFS_RXFCSERROR, error count
					 * handled by a IPG statistic register.
					 */

					/* Free the memory associated with the RX
					 * buffer since it is erroneous and we will
					 * not pass it to higher layer processes.
					 */
					if (sp->RxBuff[sp->CurrentRFD] != NULL)
					{
						pci_unmap_single(sp->ipg_pci_device,
                                 sp->RxBuffDMAhandle[sp->CurrentRFD].dmahandle,
		    		             sp->RxBuffDMAhandle[sp->CurrentRFD].len,
		    		             PCI_DMA_FROMDEVICE);

						dev_kfree_skb(sp->RxBuff[sp->CurrentRFD]);
						sp->RxBuff[sp->CurrentRFD] = NULL;
					}
				return ErrorPacket;
			  }
			  return NormalPacket;
}
static int ipg_nic_rx(ipg_device_type *ipg_ethernet_device)
{
	/* transfer received ethernet frames to higher network layers. */

	int				maxrfdcount;
	int				framelen;
	int            thisendframelen;
	unsigned long				baseaddr;
	struct ipg_nic_private	*sp = (struct ipg_nic_private *)
					      ipg_ethernet_device->priv;
	struct sk_buff			*skb;


//	IPG_DEBUG_MSG("_nic_rx\n");

	baseaddr = ipg_ethernet_device->base_addr;
	maxrfdcount = ipg_maxrfdprocess_count;

    while((sp->rfdlist[sp->currentrfd].rfs) & ipg_rfs_rfddone)
    {
	 	if (--maxrfdcount == 0)
	    {
	        /* there are more rfds to process, however the
	     	 * allocated amount of rfd processing time has
			 * expired. assert interrupt requested to make
			 * sure we come back to process the remaining rfds.
	         */
		    ipg_write_asicctrl(baseaddr,
			                   ipg_read_asicctrl(baseaddr) |
		                       ipg_ac_int_request);
		    break;
        }

		   switch(ipg_nic_rx__checkframesetype(ipg_ethernet_device))
		   { // frame in one rfd
//-----------
		   case frame_withstart_withend: 	
		   	if(sp->jumbo.foundstart)
		   	{
					dev_kfree_skb(sp->jumbo.skb);
					sp->jumbo.foundstart=0;
    				sp->jumbo.currentsize=0;
    				sp->jumbo.skb=null;
    			}
		     if(ipg_nic_rx__checkerror(ipg_ethernet_device)==normalpacket)//1: found error, 0 no error
			  { // accept this frame and send to upper layer
					skb = sp->rxbuff[sp->currentrfd];
					if(skb)
					{
						framelen=(sp->rfdlist[sp->currentrfd].rfs) & ipg_rfs_rxframelen;
						if (framelen > ipg_rxfrag_size) framelen=ipg_rxfrag_size;
						skb_put(skb, framelen);
						/* set the buffer's protocol field to ehternet */
//						skb->protocol=eth_type_trans(skb, ipg_ethernet_device);
						/* not handle tcp/udp/ip checksum */
						skb->ip_summed=checksum_none;
						netif_rx(skb);
						ipg_ethernet_device->last_rx=jiffies;
						sp->rxbuff[sp->currentrfd] = null;
					}
			  }//if(ipg_nic_rx__checkerror(ipg_ethernet_device)==0)//1: found error, 0 no error
			break;//case frame_withstart_withend: 	
//-----------
			case frame_withstart: 
		     if(ipg_nic_rx__checkerror(ipg_ethernet_device)==normalpacket)//1: found error, 0 no error
			  { // accept this frame and send to upper layer
					skb = sp->rxbuff[sp->currentrfd];
					if(skb)
					{
						if(sp->jumbo.foundstart)
						{
							dev_kfree_skb(sp->jumbo.skb);
						}
				pci_unmap_single(sp->ipg_pci_device,
                                 sp->rxbuffdmahandle[sp->currentrfd].dmahandle,
		                 sp->rxbuffdmahandle[sp->currentrfd].len,
		                 PCI_DMA_FROMDEVICE);
    						sp->jumbo.foundstart=1;
    						sp->jumbo.currentsize=ipg_rxfrag_size;    						
    						sp->jumbo.skb=skb;
    						skb_put(sp->jumbo.skb, ipg_rxfrag_size);
							sp->rxbuff[sp->currentrfd] = null;
							ipg_ethernet_device->last_rx=jiffies;
					}
			  }//if(ipg_nic_rx__checkerror(ipg_ethernet_device)==0)//1: found error, 0 no error
			break;//case frame_withstart: 
//-----------
			case frame_withend: 
		     if(ipg_nic_rx__checkerror(ipg_ethernet_device)==normalpacket)//1: found error, 0 no error
			  { // accept this frame and send to upper layer
					skb = sp->rxbuff[sp->currentrfd];
					if(skb)
					{
						if(sp->jumbo.foundstart)
						{
							framelen=(sp->rfdlist[sp->currentrfd].rfs) & ipg_rfs_rxframelen;
							thisendframelen=framelen-sp->jumbo.currentsize;
							//if (framelen > ipg_rxfrag_size) framelen=ipg_rxfrag_size;
							if(framelen>ipg_rxsupport_size)
							{
								dev_kfree_skb(sp->jumbo.skb);
							}
							else
							{
								memcpy(skb_put(sp->jumbo.skb,thisendframelen),skb->data,thisendframelen);
								/* set the buffer's protocol field to ehternet */
//								sp->jumbo.skb->protocol=eth_type_trans(sp->jumbo.skb, ipg_ethernet_device);
								/* not handle tcp/udp/ip checksum */
								sp->jumbo.skb->ip_summed=checksum_none;
								netif_rx(sp->jumbo.skb);							
							}							
						}//"if(sp->jumbo.foundstart)"
						
						ipg_ethernet_device->last_rx=jiffies;
						sp->jumbo.foundstart=0;
    					sp->jumbo.currentsize=0;
    					sp->jumbo.skb=null;
    					//free this buffer(jc-advance)
						ipg_nic_rx__freeskb(ipg_ethernet_device);
					}//"if(skb)"
			  }//"if(ipg_nic_rx__checkerror(ipg_ethernet_device)==0)//1: found error, 0 no error"
			  else
			  {
			  		dev_kfree_skb(sp->jumbo.skb);
					sp->jumbo.foundstart=0;
    				sp->jumbo.currentsize=0;
    				sp->jumbo.skb=null;
			  }
			break;//case frame_withend:
//-----------
			case frame_nostart_noend: 
		     if(ipg_nic_rx__checkerror(ipg_ethernet_device)==normalpacket)//1: found error, 0 no error
			  { // accept this frame and send to upper layer
					skb = sp->rxbuff[sp->currentrfd];
					if(skb)
					{
						if(sp->jumbo.foundstart)
						{
							//if (framelen > ipg_rxfrag_size) framelen=ipg_rxfrag_size;
							sp->jumbo.currentsize+=ipg_rxfrag_size;
							if(sp->jumbo.currentsize>ipg_rxsupport_size)
							{
								/*dev_kfree_skb(sp->jumbo.skb);
								sp->jumbo.foundstart=0;
    							sp->jumbo.currentsize=0;
    							sp->jumbo.skb=null;*/
							}
							else
							{
								memcpy(skb_put(sp->jumbo.skb,ipg_rxfrag_size),skb->data,ipg_rxfrag_size);
							}
						}//"if(sp->jumbo.foundstart)"
						ipg_ethernet_device->last_rx=jiffies;
						ipg_nic_rx__freeskb(ipg_ethernet_device);
					}
			  }//if(ipg_nic_rx__checkerror(ipg_ethernet_device)==0)//1: found error, 0 no error
			  else
			  {
			  		dev_kfree_skb(sp->jumbo.skb);
					sp->jumbo.foundstart=0;
    				sp->jumbo.currentsize=0;
    				sp->jumbo.skb=null;
			  }
			break;//case frame_nostart_noend:
			}//switch(ipg_nic_rx__checkframesetype(ipg_ethernet_device))

	    sp->currentrfd = (sp->currentrfd+1) % IPG_RFDLIST_LENGTH;
	} /* end of while(ipg_rfs_rfddone)*/
	
	ipg_nic_rxrestore(ipg_ethernet_device);
	return 0;
}


#else
#define CHECKSUM_NONE 0
static int ipg_nic_rx(struct net_device *ipg_ethernet_device)
{
	/* Transfer received Ethernet frames to higher network layers. */

	int				maxrfdcount;
	int				framelen;
	unsigned long				baseaddr;
	struct ipg_nic_private	*sp = (struct ipg_nic_private *)
					      ipg_ethernet_device->priv;
	struct sk_buff			*skb;

//	IPG_DEBUG_MSG("_nic_rx\n");

	baseaddr = ipg_ethernet_device->base_addr;
	maxrfdcount = IPG_MAXRFDPROCESS_COUNT;

	while(((sp->RFDList[sp->CurrentRFD].RFS)) &
	       IPG_RFS_RFDDONE &&
	      ((sp->RFDList[sp->CurrentRFD].RFS)) &
	       IPG_RFS_FRAMESTART &&
	      ((sp->RFDList[sp->CurrentRFD].RFS)) &
	       IPG_RFS_FRAMEEND &&
	      (sp->RxBuff[sp->CurrentRFD] != NULL))
	{
                if (--maxrfdcount == 0)
                {
                        /* There are more RFDs to process, however the
			 * allocated amount of RFD processing time has
			 * expired. Assert Interrupt Requested to make
			 * sure we come back to process the remaining RFDs.
                         */
                        IPG_WRITE_ASICCTRL(baseaddr,
			                      IPG_READ_ASICCTRL(baseaddr) |
                                              IPG_AC_INT_REQUEST);
                        break;
                }

		/* Get received frame length. */
		framelen = (sp->RFDList[sp->CurrentRFD].RFS) &
	       		        IPG_RFS_RXFRAMELEN;

		/* Check for jumbo frame arrival with too small
		 * RXFRAG_SIZE.
		 */
		if (framelen > IPG_RXFRAG_SIZE)
		{
			IPG_DEBUG_MSG("RFS FrameLen > allocated fragment size.\n");

			framelen = IPG_RXFRAG_SIZE;
		}

		/* Get the received frame buffer. */
		skb = sp->RxBuff[sp->CurrentRFD];

		if ((IPG_DROP_ON_RX_ETH_ERRORS &&
		     ((sp->RFDList[sp->CurrentRFD].RFS &
		      (IPG_RFS_RXFIFOOVERRUN |
		       IPG_RFS_RXRUNTFRAME |
		       IPG_RFS_RXALIGNMENTERROR |
		       IPG_RFS_RXFCSERROR |
		       IPG_RFS_RXOVERSIZEDFRAME |
		       IPG_RFS_RXLENGTHERROR)))))
		{

			IPG_DEBUG_MSG("Rx error, RFS = %16.16lx\n", (unsigned long int) sp->RFDList[sp->CurrentRFD].RFS);

			/* Increment general receive error statistic. */
			sp->stats.rx_errors++;

			/* Increment detailed receive error statistics. */
			if ((sp->RFDList[sp->CurrentRFD].RFS &
			    IPG_RFS_RXFIFOOVERRUN))
			{
				IPG_DEBUG_MSG("RX FIFO overrun occured.\n");

				sp->stats.rx_fifo_errors++;

				if (sp->RxBuffNotReady == 1)
				{
					/* If experience a RxFIFO overrun, and
					 * the RxBuffNotReady flag is set,
					 * assume the FIFO overran due to lack
					 * of an RFD.
					 */
					sp->stats.rx_dropped++;
				}
			}

			if ((sp->RFDList[sp->CurrentRFD].RFS &
			    IPG_RFS_RXRUNTFRAME))
			{
				IPG_DEBUG_MSG("RX runt occured.\n");
				sp->stats.rx_length_errors++;
			}

			if ((sp->RFDList[sp->CurrentRFD].RFS &
			    IPG_RFS_RXOVERSIZEDFRAME));
			/* Do nothing, error count handled by a IPG
			 * statistic register.
			 */

			if ((sp->RFDList[sp->CurrentRFD].RFS &
			    IPG_RFS_RXALIGNMENTERROR))
			{
				IPG_DEBUG_MSG("RX alignment error occured.\n");
				sp->stats.rx_frame_errors++;
			}


			if ((sp->RFDList[sp->CurrentRFD].RFS &
			    IPG_RFS_RXFCSERROR));
			/* Do nothing, error count handled by a IPG
			 * statistic register.
			 */

			/* Free the memory associated with the RX
			 * buffer since it is erroneous and we will
			 * not pass it to higher layer processes.
			 */
			if (sp->RxBuff[sp->CurrentRFD] != NULL)
			{
#ifndef RX_COPY_MODE
				pci_unmap_single(sp->ipg_pci_device,
                                 sp->RxBuffDMAhandle[sp->CurrentRFD].dmahandle,
		                 sp->RxBuffDMAhandle[sp->CurrentRFD].len,
		                 PCI_DMA_FROMDEVICE);

				dev_kfree_skb(sp->RxBuff[sp->CurrentRFD]);
#endif /* RX_COPY_MODE */
			}

		}
		else
		{
#ifdef RX_COPY_MODE
                        struct sk_buff *skb_from_nic = sp->RxBuff[sp->CurrentRFD];
			skb = dev_alloc_skb(framelen + 2);
			skb_reserve(skb, 2);
			skb->dev = ipg_ethernet_device;
			memcpy(skb->data, skb_from_nic->data, framelen);
#endif
			/* Adjust the new buffer length to accomodate the size
			 * of the received frame.
			 */
			skb_put(skb, framelen);

			/* Set the buffer's protocol field to Ethernet. */
//			skb->protocol = eth_type_trans(skb,
//			                               ipg_ethernet_device);

			/* If the frame contains an IP/TCP/UDP frame,
			 * determine if upper layer must check IP/TCP/UDP
			 * checksums.
			 *
			 * NOTE: DO NOT RELY ON THE TCP/UDP CHECKSUM
			 *       VERIFICATION FOR SILICON REVISIONS B3
			 *       AND EARLIER!
			 *
			if (((sp->RFDList[sp->CurrentRFD].RFS &
			     (IPG_RFS_TCPDETECTED |
			      IPG_RFS_UDPDETECTED |
			      IPG_RFS_IPDETECTED))) &&
			    !((sp->RFDList[sp->CurrentRFD].RFS &
			      (IPG_RFS_TCPERROR |
			       IPG_RFS_UDPERROR |
			       IPG_RFS_IPERROR))))
			{
				* Indicate IP checksums were performed
				 * by the IPG.
				 *
				skb->ip_summed = CHECKSUM_UNNECESSARY;
			}
			else
			*/
			if (1==1)
			{
				/* The IPG encountered an error with (or
				 * there were no) IP/TCP/UDP checksums.
				 * This may or may not indicate an invalid
				 * IP/TCP/UDP frame was received. Let the
				 * upper layer decide.
				 */
//				skb->ip_summed = CHECKSUM_NONE;
			}

			/* Hand off frame for higher layer processing.
			 * The function netif_rx() releases the sk_buff
			 * when processing completes.
			 */
			netif_rx(skb);/* jc_db */

			/* Record frame receive time (jiffies = Linux
			 * kernel current time stamp).
			 */
			ipg_ethernet_device->last_rx = jiffies;
		}
#ifndef RX_COPY_MODE
		/* Assure RX buffer is not reused by IPG. */
		sp->RxBuff[sp->CurrentRFD] = NULL;
#endif
		/* Increment the current RFD counter. */
		sp->CurrentRFD = (sp->CurrentRFD + 1) % IPG_RFDLIST_LENGTH;

	}


#ifdef IPG_DEBUG
	/* Check if the RFD list contained no receive frame data. */
	if (maxrfdcount == IPG_MAXRFDPROCESS_COUNT)
	{
//		sp->EmptyRFDListCount++;
	}
#endif
	while(((sp->RFDList[sp->CurrentRFD].RFS &
	       IPG_RFS_RFDDONE)) &&
	      !(((sp->RFDList[sp->CurrentRFD].RFS &
	       IPG_RFS_FRAMESTART)) &&
	      ((sp->RFDList[sp->CurrentRFD].RFS &
	       IPG_RFS_FRAMEEND))))
	{
		IPG_DEBUG_MSG("Frame requires multiple RFDs.\n");

		/* An unexpected event, additional code needed to handle
		 * properly. So for the time being, just disregard the
		 * frame.
		 */

		/* Free the memory associated with the RX
		 * buffer since it is erroneous and we will
		 * not pass it to higher layer processes.
		 */
#ifndef RX_COPY_MODE
		if (sp->RxBuff[sp->CurrentRFD] != NULL)
		{
			pci_unmap_single(sp->ipg_pci_device,
                                sp->RxBuffDMAhandle[sp->CurrentRFD].dmahandle,
		                sp->RxBuffDMAhandle[sp->CurrentRFD].len,
		                PCI_DMA_FROMDEVICE);
			dev_kfree_skb(sp->RxBuff[sp->CurrentRFD]);
		}

		/* Assure RX buffer is not reused by IPG. */
		sp->RxBuff[sp->CurrentRFD] = NULL;

#endif /* RX_COPY_MODE */

		/* Increment the current RFD counter. */
		sp->CurrentRFD = (sp->CurrentRFD + 1) % IPG_RFDLIST_LENGTH;
	}

        /* Check to see if there are a minimum number of used
         * RFDs before restoring any (should improve performance.)
         */
        if (((sp->CurrentRFD > sp->LastRestoredRxBuff) &&
             ((sp->LastRestoredRxBuff + IPG_MINUSEDRFDSTOFREE) <=
              sp->CurrentRFD)) ||
            ((sp->CurrentRFD < sp->LastRestoredRxBuff) &&
             ((sp->LastRestoredRxBuff + IPG_MINUSEDRFDSTOFREE) <=
              (sp->CurrentRFD + IPG_RFDLIST_LENGTH))))
        {
		ipg_nic_rxrestore(ipg_ethernet_device);
	}

	return 0;
}
#endif
static int ipg_nic_rxrestore(struct net_device *ipg_ethernet_device)
{
	/* restore used receive buffers. */

	int                             i;
	struct ipg_nic_private        *sp = (struct ipg_nic_private *)
					ipg_ethernet_device->priv;

//	IPG_DEBUG_MSG("_nic_rxrestore\n");

	/* assume receive buffers will be available. */
	sp->RxBuffNotReady= 0;

        while (sp->RxBuff[i = ((sp->LastRestoredRxBuff + 1) %
               IPG_RFDLIST_LENGTH)] == NULL)
	{
		/* generate a new receive buffer to replace the
		 * current buffer (which will be released by the
		 * linux system).
		 */
		if (ipg_get_rxbuff(ipg_ethernet_device, i) < 0)
		{
			IPG_DEBUG_MSG("cannot allocate new rx buffer.\n");

			/* mark a flag indicating a receive buffer
			 * was not available. use this flag to update
			 * the rx_dropped linux statistic.
			 */
			sp->RxBuffNotReady = 1;

			break;
		}

		/* reset the rfs field. */
		sp->RFDList[i].RFS = 0x0000000000000000;

		sp->LastRestoredRxBuff = i;
	}

	return 0;
}
#if 0
ipg_stats_type* ipg_nic_get_stats(struct net_device *ipg_ethernet_device)
{
	/* provides statistical information about the ipg nic. */

	u16				temp1;
	u16				temp2;
	unsigned long				baseaddr;
	struct ipg_nic_private	*sp = (struct ipg_nic_private *)
					      ipg_ethernet_device->priv;

	IPG_DEBUG_MSG("_nic_get_stats\n");

	/* check to see if the nic has been initialized via nic_open,
	 * before trying to read statistic registers.
	 */
	if (!test_bit(__link_state_start, &ipg_ethernet_device->state))
	{
		return &sp->stats;
	}


	baseaddr = ipg_ethernet_device->base_addr;

        sp->stats.rx_packets += ipg_read_framesrcvdok(baseaddr);
        sp->stats.tx_packets += ipg_read_framesxmtdok(baseaddr);
        sp->stats.rx_bytes   += ipg_read_octetrcvok(baseaddr);
        sp->stats.tx_bytes   += ipg_read_octetxmtok(baseaddr);
	temp1 = ipg_read_frameslostrxerrors(baseaddr);
        sp->stats.rx_errors  += temp1;
        sp->stats.rx_missed_errors += temp1;
	temp1 = ipg_read_singlecolframes(baseaddr) +
	        ipg_read_multicolframes(baseaddr) +
	        ipg_read_latecollisions(baseaddr);
	temp2 = ipg_read_carriersenseerrors(baseaddr);
        sp->stats.collisions += temp1;
	sp->stats.tx_dropped += ipg_read_framesabortxscolls(baseaddr);
        sp->stats.tx_errors  += ipg_read_frameswexdeferral(baseaddr) +
	                        ipg_read_frameswdeferredxmt(baseaddr) +
	                        temp1 + temp2;
        sp->stats.multicast  += ipg_read_mcstoctetrcvdok(baseaddr);

        /* detailed tx_errors */
        sp->stats.tx_carrier_errors += temp2;

        /* detailed rx_errors */
	sp->stats.rx_length_errors += ipg_read_inrangelengtherrors(baseaddr) +
	                              ipg_read_frametoolongerrrors(baseaddr);
	sp->stats.rx_crc_errors += ipg_read_framecheckseqerrors(baseaddr);

	/* unutilized ipg statistic registers. */
	ipg_read_mcstframesrcvdok(baseaddr);

        /* masked ipg statistic registers (need not be read to clear)
	ipg_read_maccontrolframesxmtdok(baseaddr);
	ipg_read_bcstframesxmtdok(baseaddr);
	ipg_read_mcstframesxmtdok(baseaddr);
	ipg_read_bcstoctetxmtok(baseaddr);
	ipg_read_mcstoctetxmtok(baseaddr);
	ipg_read_maccontrolframesrcvd(baseaddr);
	ipg_read_bcstframesrcvdok(baseaddr);
	ipg_read_bcstoctetrcvok(baseaddr);
	ipg_read_txjumboframes(baseaddr);
	ipg_read_udpchecksumerrors(baseaddr);
	ipg_read_ipchecksumerrors(baseaddr);
	ipg_read_tcpchecksumerrors(baseaddr);
	ipg_read_rxjumboframes(baseaddr);
	*/


	/* unutilized rmon statistic registers. */

        /* masked ipg statistic registers (need not be read to clear)
	ipg_read_etherstatscollisions(baseaddr);
	ipg_read_etherstatsoctetstransmit(baseaddr);
	ipg_read_etherstatspktstransmit(baseaddr);
	ipg_read_etherstatspkts64octeststransmit(baseaddr);
	ipg_read_etherstatspkts65to127octeststransmit(baseaddr);
	ipg_read_etherstatspkts128to255octeststransmit(baseaddr);
	ipg_read_etherstatspkts256to511octeststransmit(baseaddr);
	ipg_read_etherstatspkts512to1023octeststransmit(baseaddr);
	ipg_read_etherstatspkts1024to1518octeststransmit(baseaddr);
	ipg_read_etherstatscrcalignerrors(baseaddr);
	ipg_read_etherstatsundersizepkts(baseaddr);
	ipg_read_etherstatsfragments(baseaddr);
	ipg_read_etherstatsjabbers(baseaddr);
	ipg_read_etherstatsoctets(baseaddr);
	ipg_read_etherstatspkts(baseaddr);
	ipg_read_etherstatspkts64octests(baseaddr);
	ipg_read_etherstatspkts65to127octests(baseaddr);
	ipg_read_etherstatspkts128to255octests(baseaddr);
	ipg_read_etherstatspkts256to511octests(baseaddr);
	ipg_read_etherstatspkts512to1023octests(baseaddr);
	ipg_read_etherstatspkts1024to1518octests(baseaddr);
	*/

	return &sp->stats;
}
#endif
static void ipg_nic_set_multicast_list(struct net_device *ipg_ethernet_device)
{
	/* determine and configure multicast operation and set
	 * receive mode for ipg.
	 */
	u8			receivemode;
	u32			hashtable[2];
	unsigned int		hashindex;
	unsigned long			baseaddr;
	struct dev_mc_list	*mc_list_ptr;


	IPG_DEBUG_MSG("_nic_set_multicast_list\n");

	baseaddr = ipg_ethernet_device->base_addr;

	receivemode = IPG_RM_RECEIVEUNICAST |
	              IPG_RM_RECEIVEBROADCAST;

	if (ipg_ethernet_device->flags & IFF_PROMISC)
	{
		/* nic to be configured in promiscuous mode. */
		receivemode = IPG_RM_RECEIVEALLFRAMES;
	}
	else if ((ipg_ethernet_device->flags & IFF_ALLMULTI) ||
	         (ipg_ethernet_device->flags & IFF_MULTICAST &
	          (ipg_ethernet_device->mc_count >
	           IPG_MULTICAST_HASHTABLE_SIZE)))
	{
		/* nic to be configured to receive all multicast
		 * frames. */
		receivemode |= IPG_RM_RECEIVEMULTICAST;
	}
	else if (ipg_ethernet_device->flags & IFF_MULTICAST &
	    (ipg_ethernet_device->mc_count > 0))
	{
		/* nic to be configured to receive selected
		 * multicast addresses. */
		receivemode |= IPG_RM_RECEIVEMULTICASTHASH;
	}

	/* calculate the bits to set for the 64 bit, ipg hashtable.
	 * the ipg applies a cyclic-redundancy-check (the same crc
	 * used to calculate the frame data fcs) to the destination
	 * address all incoming multicast frames whose destination
	 * address has the multicast bit set. the least significant
	 * 6 bits of the crc result are used as an addressing index
	 * into the hash table. if the value of the bit addressed by
	 * this index is a 1, the frame is passed to the host system.
	 */

	/* clear hashtable. */
	hashtable[0] = 0x00000000;
	hashtable[1] = 0x00000000;

	/* cycle through all multicast addresses to filter.*/
	for (mc_list_ptr = ipg_ethernet_device->mc_list;
	     mc_list_ptr != NULL;
	     mc_list_ptr = mc_list_ptr->next)
	{
		/* calculate crc result for each multicast address. */
		hashindex = ether_crc_le(ETH_ALEN, mc_list_ptr->dmi_addr);

		/* use only the least significant 6 bits. */
		hashindex = hashindex & 0x3f;

		/* within "hashtable", set bit number "hashindex"
		 * to a logic 1.
		 */
		set_bit(hashindex, (void*)hashtable);
	}

	/* write the value of the hashtable, to the 4, 16 bit
	 * hashtable ipg registers.
	 */
	IPG_WRITE_HASHTABLE0(baseaddr, hashtable[0]);
	IPG_WRITE_HASHTABLE1(baseaddr, hashtable[1]);

	IPG_WRITE_RECEIVEMODE(baseaddr, receivemode);

	IPG_DEBUG_MSG("receivemode = %x\n",IPG_READ_RECEIVEMODE(baseaddr));

	return;
}

/*
 * the following code fragment was authored by donald becker.
 */

/* the little-endian autodin ii ethernet crc calculations.
   a big-endian version is also available.
   this is slow but compact code.  do not use this routine for bulk data,
   use a table-based routine instead.
   this is common code and should be moved to net/core/crc.c.
   chips may use the upper or lower crc bits, and may reverse and/or invert
   them.  select the endian-ness that results in minimal calculations.
*/
unsigned const ethernet_polynomial_le = 0xedb88320u;
static unsigned ether_crc_le(int length, unsigned char *data)
{
        unsigned int crc = 0xffffffff;  /* initial value. */
        while(--length >= 0) {
                unsigned char current_octet = *data++;
                int bit;
                for (bit = 8; --bit >= 0; current_octet >>= 1) {
                        if ((crc ^ current_octet) & 1) {
                                crc >>= 1;
                                crc ^= ethernet_polynomial_le;
                        } else
                                crc >>= 1;
                }
        }
        return crc;
}
/*
 * end of code fragment authored by donald becker.
 */
#if 0
int ipg_nic_init(struct net_device *ipg_ethernet_device)
{
	/* initialize ipg nic. */

	struct ipg_nic_private	*sp = NULL;

	IPG_DEBUG_MSG("_nic_init\n");

	/* register the ipg nic in the list of ethernet devices. */
   ipg_ethernet_device=alloc_etherdev(sizeof(struct ipg_nic_private));

	if (ipg_ethernet_device == NULL)
	{
		printf("could not initialize ip1000 based nic.\n");
		return -ENODEV;
	}
	/* reserve memory for ipg_nic_private structure. */
	sp = kmalloc(sizeof(struct ipg_nic_private),
		            GFP_KERNEL);

	if (sp == NULL)
	{
		printf("%s: no memory available for ip1000 private strucutre.\n", ipg_ethernet_device->name);
		return -ENOMEM;
	}
	else
	{
		/* fill the allocated memory space with 0s.
		 * essentially sets all ipg_nic_private
		 * structure fields to 0.
		 */
		memset(sp, 0, sizeof(*sp));
		ipg_ethernet_device->priv = sp;
	}
	/* assign the new device to the list of ipg ethernet devices. */
	sp->next_ipg_ethernet_device = root_ipg_ethernet_device;
	root_ipg_ethernet_device = ipg_ethernet_device;

	/* declare ipg nic functions for ethernet device methods.
	 */
	ipg_ethernet_device->open = &ipg_nic_open;
	ipg_ethernet_device->stop = &ipg_nic_stop;
//	ipg_ethernet_device->hard_start_xmit = &ipg_nic_hard_start_xmit;
//	ipg_ethernet_device->get_stats = &ipg_nic_get_stats;
	ipg_ethernet_device->set_multicast_list =
	  &ipg_nic_set_multicast_list;
	ipg_ethernet_device->do_ioctl = & ipg_nic_do_ioctl;
	/* rebuild_header not defined. */
	/* hard_header not defined. */
	/* set_config not defined */
	/* set_mac_address not defined. */
	/* header_cache_bind not defined. */
	/* header_cache_update not defined. */
        ipg_ethernet_device->change_mtu = &ipg_nic_change_mtu;
        /* ipg_ethernet_device->tx_timouet not defined. */
        /* ipg_ethernet_device->watchdog_timeo not defined. */

	return 0;
}
#endif
static int ipg_nic_do_ioctl(struct net_device  *ipg_ethernet_device,
                        struct ifreq *req, int cmd)
{
	/* ioctl commands for ipg nic.
	 *
	 * siocdevprivate	nothing
	 * siocdevprivate+1	register read
	 *			ifr_data[0] = 0x08, 0x10, 0x20
	 *			ifr_data[1] = register offset
	 *			ifr_data[2] = value read
	 * siocdevprivate+2	register write
	 *			ifr_data[0] = 0x08, 0x10, 0x20
	 *			ifr_data[1] = register offset
	 *			ifr_data[2] = value to write
	 * siocdevprivate+3	gmii register read
	 *			ifr_data[1] = register offset
	 * siocdevprivate+4	gmii register write
	 *			ifr_data[1] = register offset
	 *			ifr_data[2] = value to write
	 * siocdevprivate+5	pci register read
	 *			ifr_data[0] = 0x08, 0x10, 0x20
	 *			ifr_data[1] = register offset
	 *			ifr_data[2] = value read
	 * siocdevprivate+6	pci register write
	 *			ifr_data[0] = 0x08, 0x10, 0x20
	 *			ifr_data[1] = register offset
	 *			ifr_data[2] = value to write
	 *
	 */

	u8				val8;
	u16				val16;
	u32				val32;
	unsigned int			*data;
        int                             phyaddr = 0;
	unsigned long				baseaddr;
	struct ipg_nic_private	*sp = (struct ipg_nic_private *)
					      ipg_ethernet_device->priv;

	IPG_DEBUG_MSG("_nic_do_ioctl\n");

        data = (unsigned int *)&req->ifr_data;
	baseaddr = ipg_ethernet_device->base_addr;

        switch(cmd)
	{
	        case SIOCDEVPRIVATE:
		return 0;

		case SIOCDEVPRIVATE+1:
		switch(data[0])
		{
			case 0x08:
			data[2] = IPG_READ_BYTEREG(baseaddr + data[1]);
			return 0;

			case 0x10:
			data[2] = IPG_READ_WORDREG(baseaddr + data[1]);
			return 0;

			case 0x20:
			data[2] = IPG_READ_LONGREG(baseaddr + data[1]);
			return 0;

			default:
			data[2] = 0x00;
			return -EINVAL;
		}

		case SIOCDEVPRIVATE+2:
		switch(data[0])
		{
			case 0x08:
			IPG_WRITE_BYTEREG(baseaddr + data[1], data[2]);
			return 0;

			case 0x10:
			IPG_WRITE_WORDREG(baseaddr + data[1], data[2]);
			return 0;

			case 0x20:
			IPG_WRITE_LONGREG(baseaddr + data[1], data[2]);
			return 0;

			default:
			return -EINVAL;
		}

		case SIOCDEVPRIVATE+3:
        	phyaddr = ipg_find_phyaddr(ipg_ethernet_device);

		if (phyaddr == -1)
			return -EINVAL;

		data[2] = read_phy_register(ipg_ethernet_device,
		                            phyaddr, data[1]);

		return 0;

		case SIOCDEVPRIVATE+4:
        	phyaddr = ipg_find_phyaddr(ipg_ethernet_device);

		if (phyaddr == -1)
			return -EINVAL;

		write_phy_register(ipg_ethernet_device,
		                   phyaddr, data[1], (u16)data[2]);

		return 0;

		case SIOCDEVPRIVATE+5:
		switch(data[0])
		{
			case 0x08:
			pci_read_config_byte(sp->ipg_pci_device,data[1],
			                     &val8);
			data[2] = (unsigned int)val8;
			return 0;

			case 0x10:
			pci_read_config_word(sp->ipg_pci_device,data[1],
			                     &val16);
			data[2] = (unsigned int)val16;
			return 0;

			case 0x20:
			pci_read_config_dword(sp->ipg_pci_device,data[1],
			                     &val32);
			data[2] = (unsigned int)val32;
			return 0;

			default:
			data[2] = 0x00;
			return -EINVAL;
		}

		case SIOCDEVPRIVATE+6:
		switch(data[0])
		{
			case 0x08:
			pci_write_config_byte(sp->ipg_pci_device,data[1],
			                      (u8)data[2]);
			return 0;

			case 0x10:
			pci_write_config_word(sp->ipg_pci_device,data[1],
			                      (u16)data[2]);
			return 0;

			case 0x20:
			pci_write_config_dword(sp->ipg_pci_device,data[1],
			                       (u32)data[2]);
			return 0;

			default:
			return -EINVAL;
		}

		case SIOCSIFMTU:
		{
			return 0;
		}

		default:
		return -EOPNOTSUPP;
        }
}

static int ipg_nic_change_mtu(struct net_device *ipg_ethernet_device,
                          int new_mtu)
{
        /* Function to accomodate changes to Maximum Transfer Unit
         * (or MTU) of IPG NIC. Cannot use default function since
         * the default will not allow for MTU > 1500 bytes.
         */

	IPG_DEBUG_MSG("_nic_change_mtu\n");

        /* Check that the new MTU value is between 68 (14 byte header, 46
         * byte payload, 4 byte FCS) and IPG_MAX_RXFRAME_SIZE, which
         * corresponds to the MAXFRAMESIZE register in the IPG.
         */
        if ((new_mtu < 68) || (new_mtu > IPG_MAX_RXFRAME_SIZE))
        {
                return -EINVAL;
        }

        ipg_ethernet_device->mtu = new_mtu;

        return 0;
}

static void bSetPhyDefaultParam(unsigned char Rev,
		struct net_device *ipg_ethernet_device,int phy_address)
{
	unsigned short Length;
	unsigned char Revision;
	unsigned short *pPHYParam;
	unsigned short address,value;
      	
	pPHYParam = &DefaultPhyParam[0];
	Length = *pPHYParam & 0x00FF;
	Revision = (unsigned char) ((*pPHYParam) >> 8);
	pPHYParam++;
	while(Length != 0)
	{
		if(Rev == Revision)
		{
			while(Length >1)
			{
				address=*pPHYParam; 
				value=*(pPHYParam+1);
				pPHYParam+=2;
				write_phy_register(ipg_ethernet_device,phy_address,address, value);
				Length -= 4;
			}

			break;
		}
		else // advanced to next revision
		{
			pPHYParam += Length/2;
			Length = *pPHYParam & 0x00FF;
			Revision = (unsigned char) ((*pPHYParam) >> 8);
			pPHYParam++;
		}
	}
	return;
}

/*jes20040127eeprom*/
static int read_eeprom(struct net_device *ipg_ethernet_device, int eep_addr)
{
   unsigned long	baseaddr;
   int            i = 1000;
	baseaddr = ipg_ethernet_device->base_addr;
	
//	printf("read_eeprom baseaddr=%x\n",baseaddr);
   IPG_WRITE_EEPROMCTRL(baseaddr,  IPG_EC_EEPROM_READOPCODE | (eep_addr & 0xff));
   while (i-- > 0) {
   	mdelay(10);
        if (!(IPG_READ_EEPROMCTRL(baseaddr)&IPG_EC_EEPROM_BUSY)) {
                return IPG_READ_EEPROMDATA (baseaddr);
	}
   }
   return 0;
}

/* Write EEPROM JES20040127EEPROM */
//void Eeprom_Write(unsigned int card_index, unsigned eep_addr, unsigned writedata)
static void write_eeprom(struct net_device *ipg_ethernet_device, unsigned int eep_addr, unsigned int writedata)
{
/* 
   unsigned long	baseaddr;
   baseaddr = ipg_ethernet_device->base_addr;
   IPG_WRITE_EEPROMDATA(baseaddr, writedata );
   while ( (IPG_READ_EEPROMCTRL(baseaddr) & IPG_EC_EEPROM_BUSY ) ) {   
   }
   IPG_WRITE_EEPROMCTRL (baseaddr, 0xC0 );
   while ( (IPG_READ_EEPROMCTRL(baseaddr)&IPG_EC_EEPROM_BUSY) ) {   
   }
   IPG_WRITE_EEPROMCTRL(baseaddr, IPG_EC_EEPROM_WRITEOPCODE | (eep_addr & 0xff) );
   while ( (IPG_READ_EEPROMCTRL(baseaddr)&IPG_EC_EEPROM_BUSY) ) {   
   }
   
   return;*/
}

/* Set LED_Mode JES20040127EEPROM */
static void Set_LED_Mode(struct net_device *ipg_ethernet_device)
{
   u32 LED_Mode_Value;
   unsigned long	baseaddr;	
   struct ipg_nic_private	*sp = (struct ipg_nic_private *)
					      ipg_ethernet_device->priv;
	baseaddr = ipg_ethernet_device->base_addr;					      
   
   LED_Mode_Value=IPG_READ_ASICCTRL(baseaddr);
   LED_Mode_Value &= ~(IPG_AC_LED_MODE_BIT_1 | IPG_AC_LED_MODE |IPG_AC_LED_SPEED);
   
   if((sp->LED_Mode & 0x03) > 1){
   	/* Write Asic Control Bit 29 */
   	LED_Mode_Value |=IPG_AC_LED_MODE_BIT_1;
   }
   if((sp->LED_Mode & 0x01) == 1){
        /* Write Asic Control Bit 14 */
        LED_Mode_Value |=IPG_AC_LED_MODE;
   }
   if((sp->LED_Mode & 0x08) == 8){
        /* Write Asic Control Bit 27 */
        LED_Mode_Value |=IPG_AC_LED_SPEED;
   }   
   IPG_WRITE_ASICCTRL(baseaddr,LED_Mode_Value);   	
   
   return;
}

/* Set PHYSet JES20040127EEPROM */
static void Set_PHYSet(struct net_device *ipg_ethernet_device)
{
   int PHYSet_Value;
   unsigned long	baseaddr;	
   struct ipg_nic_private	*sp = (struct ipg_nic_private *)
					      ipg_ethernet_device->priv;
	baseaddr = ipg_ethernet_device->base_addr;					      
   
   PHYSet_Value=IPG_READ_PHYSET(baseaddr);
   PHYSet_Value &= ~(IPG_PS_MEM_LENB9B | IPG_PS_MEM_LEN9 |IPG_PS_NON_COMPDET);
   PHYSet_Value |= ((sp->LED_Mode & 0x70) >> 4);
   IPG_WRITE_PHYSET(baseaddr,PHYSet_Value);

   return;
}
#endif
/* end ipg.c */
static int netdev_open(struct  net_device* ipg_ethernet_device)
{
	struct ipg_nic_private	*sp = (struct ipg_nic_private *)
					      ipg_ethernet_device->priv;
	
	int i;
	int error = 0;
	int phyaddr = 0;
	unsigned long				baseaddr;
	unsigned char revisionid=0;

	/* do we need to reset the chip??? */
	baseaddr = ipg_ethernet_device->base_addr;

	i = IPG_AC_GLOBAL_RESET | IPG_AC_RX_RESET |
	    IPG_AC_TX_RESET | IPG_AC_DMA |
	    IPG_AC_FIFO | IPG_AC_NETWORK |
	    IPG_AC_HOST | IPG_AC_AUTO_INIT;

	sp->LED_Mode=read_eeprom(ipg_ethernet_device, 6);

	error = ipg_reset(ipg_ethernet_device, i);
	if (error < 0)
	{
		return error;
	}

	/* reset phy. */
	phyaddr = ipg_find_phyaddr(ipg_ethernet_device);

	if (phyaddr != -1)
	{   u16 mii_phyctrl, mii_1000cr;

	        mii_1000cr = read_phy_register(ipg_ethernet_device,
			                   phyaddr, GMII_PHY_1000BASETCONTROL);
	        write_phy_register(ipg_ethernet_device, phyaddr,
			        			GMII_PHY_1000BASETCONTROL,
			        			mii_1000cr |
								GMII_PHY_1000BASETCONTROL_FULL_DUPLEX |
								GMII_PHY_1000BASETCONTROL_HALF_DUPLEX |
								GMII_PHY_1000BASETCONTROL_PreferMaster);

	        mii_phyctrl = read_phy_register(ipg_ethernet_device,
						phyaddr, GMII_PHY_CONTROL);
			/* Set default phyparam*/
			pci_read_config_byte(sp->ipg_pci_device,PCI_REVISION_ID,&revisionid);
			bSetPhyDefaultParam(revisionid,ipg_ethernet_device,phyaddr);
			/* reset Phy*/			
			write_phy_register(ipg_ethernet_device,
		                   	phyaddr, GMII_PHY_CONTROL,
				   			(mii_phyctrl | GMII_PHY_CONTROL_RESET |
				    		MII_PHY_CONTROL_RESTARTAN));

	}

        /* intitalize lock variable before requesting interrupt. */
//    sp->lock = (spinlock_t) SPIN_LOCK_UNLOCKED;

	/* Check for interrupt line conflicts, and request interrupt
	 * line for IPG.
	 *
	 * IMPORTANT: Disable IPG interrupts prior to registering
	 *            IRQ.
	 */
	IPG_WRITE_INTENABLE(baseaddr, 0x0000);

	sp->RFDList = pci_alloc_consistent(sp->ipg_pci_device,
	                                   (sizeof(struct RFD) *
	                                   IPG_RFDLIST_LENGTH),
	                                   &sp->RFDListDMAhandle);

	sp->TFDList = pci_alloc_consistent(sp->ipg_pci_device,
	                                   (sizeof(struct TFD) *
	                                   IPG_TFDLIST_LENGTH),
	                                   &sp->TFDListDMAhandle);

	if ((sp->RFDList == NULL) || (sp->TFDList == NULL))
	{
		printf("%s: No memory available for IP1000 RFD and/or TFD lists.\n", ipg_ethernet_device->name);
		return -1;
	}

	error = init_rfdlist(ipg_ethernet_device);
	if (error < 0)
	{
		printf("%s: Error during configuration.\n",
                               ipg_ethernet_device->name);
		return error;
	}

	error = init_tfdlist(ipg_ethernet_device);
	if (error < 0)
	{
		printf("%s: Error during configuration.\n",
                               ipg_ethernet_device->name);
		return error;
	}
	
	/* Read MAC Address from EERPOM Jesse20040128EEPROM_VALUE */
	sp->StationAddr0=read_eeprom(ipg_ethernet_device, 16);
	sp->StationAddr1=read_eeprom(ipg_ethernet_device, 17);
	sp->StationAddr2=read_eeprom(ipg_ethernet_device, 18);
	/* Write MAC Address to Station Address */
	IPG_WRITE_STATIONADDRESS0(baseaddr,sp->StationAddr0);
	IPG_WRITE_STATIONADDRESS1(baseaddr,sp->StationAddr1);
	IPG_WRITE_STATIONADDRESS2(baseaddr,sp->StationAddr2);

	/* Set station address in ethernet_device structure. */
	ipg_ethernet_device->dev_addr[0] =
	  IPG_READ_STATIONADDRESS0(baseaddr) & 0x00FF;
	ipg_ethernet_device->dev_addr[1] =
	  (IPG_READ_STATIONADDRESS0(baseaddr) & 0xFF00) >> 8;
	ipg_ethernet_device->dev_addr[2] =
	  IPG_READ_STATIONADDRESS1(baseaddr) & 0x00FF;
	ipg_ethernet_device->dev_addr[3] =
	  (IPG_READ_STATIONADDRESS1(baseaddr) & 0xFF00) >> 8;
	ipg_ethernet_device->dev_addr[4] =
	  IPG_READ_STATIONADDRESS2(baseaddr) & 0x00FF;
	ipg_ethernet_device->dev_addr[5] =
	  (IPG_READ_STATIONADDRESS2(baseaddr) & 0xFF00) >> 8;

	/* Configure IPG I/O registers. */
	error = ipg_io_config(ipg_ethernet_device);
	if (error < 0)
	{
		printf("%s: Error during configuration.\n",
                               ipg_ethernet_device->name);
		return error;
	}

	/* Resolve autonegotiation. */
	if (ipg_config_autoneg(ipg_ethernet_device) < 0)
	{
		printf("%s: Auto-negotiation error.\n",
		       ipg_ethernet_device->name);
	}

#ifdef JUMBO_FRAME
    /* initialize JUMBO Frame control variable */
    sp->Jumbo.FoundStart=0;
    sp->Jumbo.CurrentSize=0;
    sp->Jumbo.skb=0;
    ipg_ethernet_device->mtu = IPG_TXFRAG_SIZE;
#endif

	/* Enable transmit and receive operation of the IPG. */
	IPG_WRITE_MACCTRL(baseaddr, IPG_READ_MACCTRL(baseaddr) |
	                     IPG_MC_RX_ENABLE | IPG_MC_TX_ENABLE);

	netif_start_queue(ipg_ethernet_device);
	return 0;
}

struct net_device *mynic_ipg;
#if 0
static void check_duplex(struct net_device *dev)
{
        int speed100=0, speed,fullduplex=0;
//        struct ipg_nic_private *np = netdev_priv(dev);
//        struct net_device *nic_ifm = mynic_ipg;
        struct net_device *nic_ifm = dev;
//	void  *ioaddr = np->base;
	void  *ioaddr = dev->base_addr;

        if(nic_ifm  == NULL){
                printf("IP100A interface not initialized\n");
                return ;
        }

         speed100 = RTL_R8(nic_ifm ,  PhyCtrl) & PcLinkSpeed ;
	 switch(speed100)
	 {
	    case 0x40:
	           speed=10;
	           break;
	    case 0x80:
	           speed=100;
	           break;
	    case 0xc0:
	           speed=1000;
	           break;
	 }
         fullduplex = RTL_R8(nic_ifm ,  PhyCtrl) &  PcDuplexStatus;
         printf(" %sMbps %s-DUPLEX.\n", speed,fullduplex ? "FULL" : "HALF");
//	if (fullduplex) {
			iowrite32 (ioread32 (ioaddr + MACCtrl) | McDuplexSelect,
				ioaddr + MACCtrl);
//	}

		return;
}
#endif
static int sundance_intr(void *dev_instance)
{
	int				error;
	unsigned long				baseaddr;
	u16				intstatusackword;
        struct net_device		*ipg_ethernet_device =
		                        (struct net_device *)dev_instance;

        int intr_handled= 1;//IRQ_NONE;

#ifdef IPG_DEBUG
	struct ipg_nic_private	*sp = (struct ipg_nic_private *)
					      ipg_ethernet_device->priv;
#endif

//	IPG_DEBUG_MSG("_interrupt_handler\n");

	baseaddr = ipg_ethernet_device->base_addr;

#ifdef JUMBO_FRAME
	ipg_nic_rxrestore(ipg_ethernet_device);
#endif
	/* Get interrupt source information, and acknowledge
	 * some (i.e. TxDMAComplete, RxDMAComplete, RxEarly,
	 * IntRequested, MacControlFrame, LinkEvent) interrupts
	 * if issued. Also, all IPG interrupts are disabled by
	 * reading IntStatusAck.
	 */
	intstatusackword = IPG_READ_INTSTATUSACK(baseaddr);

//	IPG_DEBUG_MSG("IntStatusAck = %4.4x\n", intstatusackword);

	/* If RFDListEnd interrupt, restore all used RFDs. */
	if (intstatusackword & IPG_IS_RFD_LIST_END)
	{
		IPG_DEBUG_MSG("RFDListEnd Interrupt.\n");

		/* The RFD list end indicates an RFD was encountered
		 * with a 0 NextPtr, or with an RFDDone bit set to 1
		 * (indicating the RFD is not read for use by the
		 * IPG.) Try to restore all RFDs.
		 */
		ipg_nic_rxrestore(ipg_ethernet_device);

#ifdef IPG_DEBUG
		/* Increment the RFDlistendCount counter. */
//		sp->RFDlistendCount++;
#endif
	}

	/* If RFDListEnd, RxDMAPriority, RxDMAComplete, or
	 * IntRequested interrupt, process received frames. */
#ifndef PMON
	if ((intstatusackword & IPG_IS_RX_DMA_PRIORITY) ||
	    (intstatusackword & IPG_IS_RFD_LIST_END) ||
	    (intstatusackword & IPG_IS_RX_DMA_COMPLETE) ||
	    (intstatusackword & IPG_IS_INT_REQUESTED))
#endif
	{

#if 0
	/* Increment the RFD list checked counter if interrupted
	 * only to check the RFD list. */
	if (intstatusackword & (~(IPG_IS_RX_DMA_PRIORITY |
	    IPG_IS_RFD_LIST_END | IPG_IS_RX_DMA_COMPLETE |
	    IPG_IS_INT_REQUESTED) &
	                          (IPG_IS_HOST_ERROR |
	                          IPG_IS_TX_DMA_COMPLETE |
	                          IPG_IS_TX_COMPLETE |
	                          IPG_IS_UPDATE_STATS |
	                          IPG_IS_LINK_EVENT)))

	{
		sp->RFDListCheckedCount++;
	}
#endif

		ipg_nic_rx(ipg_ethernet_device);
	}

	/* If TxDMAComplete interrupt, free used TFDs. */
	if (intstatusackword & IPG_IS_TX_DMA_COMPLETE)
	{
		/* Free used TFDs. */
		ipg_nic_txfree(ipg_ethernet_device);
	}

	/* TxComplete interrupts indicate one of numerous actions.
	 * Determine what action to take based on TXSTATUS register.
	 */
	if (intstatusackword & IPG_IS_TX_COMPLETE)
	{
		ipg_nic_txcleanup(ipg_ethernet_device);
	}

	/* If UpdateStats interrupt, update Linux Ethernet statistics */
//	if (intstatusackword & IPG_IS_UPDATE_STATS)
//	{
//		ipg_nic_get_stats(ipg_ethernet_device);
//	}

	/* If HostError interrupt, reset IPG. */
	if (intstatusackword & IPG_IS_HOST_ERROR)
	{
		IPG_DDEBUG_MSG("HostError Interrupt\n");

		IPG_DDEBUG_MSG("DMACtrl = %8.8x\n",
		 IPG_READ_DMACTRL(baseaddr));

		/* Acknowledge HostError interrupt by resetting
		 * IPG DMA and HOST.
		 */
		ipg_reset(ipg_ethernet_device,
		            IPG_AC_GLOBAL_RESET |
		            IPG_AC_HOST |
		            IPG_AC_DMA);

		error = ipg_io_config(ipg_ethernet_device);
		if (error < 0)
		{
                	printk(KERN_INFO "%s: Cannot recover from PCI error.\n",
			       ipg_ethernet_device->name);
		}

		init_rfdlist(ipg_ethernet_device);

		init_tfdlist(ipg_ethernet_device);
	}

	/* If LinkEvent interrupt, resolve autonegotiation. */
	if (intstatusackword & IPG_IS_LINK_EVENT)
	{
		if (ipg_config_autoneg(ipg_ethernet_device) < 0)
			printk(KERN_INFO "%s: Auto-negotiation error.\n",
			       ipg_ethernet_device->name);

	}

	/* If MACCtrlFrame interrupt, do nothing. */
	if (intstatusackword & IPG_IS_MAC_CTRL_FRAME)
	{
		IPG_DEBUG_MSG("MACCtrlFrame interrupt.\n");
	}

	/* If RxComplete interrupt, do nothing. */
	if (intstatusackword & IPG_IS_RX_COMPLETE)
	{
		IPG_DEBUG_MSG("RxComplete interrupt.\n");
	}

	/* If RxEarly interrupt, do nothing. */
	if (intstatusackword & IPG_IS_RX_EARLY)
	{
		IPG_DEBUG_MSG("RxEarly interrupt.\n");
	}

	/* Re-enable IPG interrupts. */
	IPG_WRITE_INTENABLE(baseaddr, IPG_IE_HOST_ERROR |
	                          IPG_IE_TX_DMA_COMPLETE |
	                          IPG_IE_TX_COMPLETE |
	                          IPG_IE_INT_REQUESTED |
	                          IPG_IE_UPDATE_STATS |
	                          IPG_IE_LINK_EVENT |
	                          IPG_IE_RX_DMA_COMPLETE |
	                          //IPG_IE_RFD_LIST_END | //20041019Jesse_For_SmartBit: remove
	                          IPG_IE_RX_DMA_PRIORITY);

        /* Indicate to higher network layers the Ethernet NIC
         * interrupt servicing is complete.
         */

                        return intr_handled;
}
#if 0
static void netdev_error(struct net_device *dev, unsigned short intstatusackword)
{
	struct ipg_nic_private *np = netdev_priv(dev);
	void  *ioaddr = np->base;
	u8 phyctrl;
	u32 asicctrl;
	int fullduplex;
	int txflowcontrol;
	int rxflowcontrol;

        phyctrl = ioread8(ioaddr + phyctrl);
	asicctrl = ioread32(ioaddr + asicctrl );
	printf("phyctrl=%x\n",phyctrl);
	if (intstatusackword & linkchange) 
	        {
			printk (kern_info "%s: link changed: ", dev->name);
			switch (phyctrl & pclinkspeed)
			{ 
			     case pclinkspeed10mbps :
				np->speed = 10;
				printf ("10mbps\n");
				break;
			     case pclinkspeed100mbps :
				np->speed = 100;
				printf ("100mbps\n");
				break;
			     case pclinkspeed1000mbps :
			        np->speed = 1000;
				printf ("1000mbps\n");
				break;
			     default : 
			        printf("undefined!\n");
				break;
			}
			if ( phyctrl & pcduplexstatus)
			{ 
			    fullduplex = 1;
			    txflowcontrol = 1;
			    rxflowcontrol = 1;
			}
			else
			{
			     fullduplex = 0;
			     txflowcontrol = 0;
			     rxflowcontrol = 0;
			}

			if (fullduplex == 1)
			{
			  printf("setting full duplex, ");
			  iowrite32((ioread32(ioaddr + MACCtrl) | McDuplexSelectFd) & McRsvdMask,ioaddr + MACCtrl);

			  if(txflowcontrol == 1)
			  {
			     printf("TX flow control");
                             iowrite32((ioread32(ioaddr + MACCtrl) | McTxFlowControlEnable) & McRsvdMask,ioaddr + MACCtrl);
			  }
			  else
			  {
			     printf("no TX flow control");
			     iowrite32((ioread32(ioaddr + MACCtrl) & ~McTxFlowControlEnable) & McRsvdMask,ioaddr + MACCtrl);
			  }

			  if(rxflowcontrol == 1)
			  {
			      printf(", RX flow control.");
			      iowrite32((ioread32(ioaddr + MACCtrl) | McRxFlowControlEnable) & McRsvdMask,ioaddr + MACCtrl);
			  }
                          else
			  {
			       printf(", no RX flow control.");
			       iowrite32((ioread32(ioaddr + MACCtrl) & ~McRxFlowControlEnable) & McRsvdMask ,ioaddr + MACCtrl);
			  }
			  printf("\n");
			}
			else
			{ 
			   printf("setting half duplex, no TX flow control, no RX flow control.\n");
			   iowrite32((ioread32(ioaddr + MACCtrl) & ~McDuplexSelectFd & ~McTxFlowControlEnable & ~McRxFlowControlEnable) & McRsvdMask ,ioaddr + MACCtrl); 
			}
		}

}
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
	netdev->priv=kmalloc(sizeof(struct ipg_nic_private));//&netdev->em;
	memset(netdev->priv,0,sizeof(struct ipg_nic_private));
	netdev->addr_len=6;
	netdev->pcidev.irq=irq++;
}
static int ipg_ether_ioctl(struct ifnet *ifp,FXP_IOCTLCMD_TYPE cmd,caddr_t data);

static struct pci_device_id *ipg_pci_id=0;
/*
 * Check if a device is an 82557.
 */
static void ipg_start(struct ifnet *ifp);
static int
ipg_match(parent, match, aux)
	struct device *parent;
#if defined(__BROKEN_INDIRECT_CONFIG) || defined(__OpenBSD__)
	void *match;
#else
	struct cfdata *match;
#endif
	void *aux;
{
//int i;
   struct pci_attach_args *pa = aux;
  ipg_pci_id=pci_match_device(ipg_pci_tbl,pa);
//i=ipg_pci_id?1:0;
//printf("i=%d\n",i);
return ipg_pci_id?1:0;
}

static void
ipg_shutdown(sc)
        void *sc;
{
}


extern char activeif_name[];
static int ipg_intr(void *data)
{
struct net_device *netdev = data;
//int irq=netdev->irq;
struct ifnet *ifp = &netdev->arpcom.ac_if;
	if(ifp->if_flags & IFF_RUNNING)
	{
			sundance_intr(data);
		   if (ifp->if_snd.ifq_head != NULL)
		   ipg_start(ifp);
	return 1;
	}
	return 0;
}

static void
ipg_attach(parent, self, aux)
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

	mynic_ipg = sc;

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
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, ipg_intr, sc, self->dv_xname);
#else
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, ipg_intr, sc);
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
	if (sundance_probe1(sc,&sc->pcidev,ipg_pci_id)) {
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
	ifp->if_ioctl = ipg_ether_ioctl;
	ifp->if_start = ipg_start;
//	ifp->if_watchdog = ipg_watchdog;

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
	shutdownhook_establish(ipg_shutdown, sc);

#ifndef PMON
	/*
	 * Add suspend hook, for similiar reasons..
	 */
	powerhook_establish(ipg_power, sc);
#endif
}


/*
 * Start packet transmission on the interface.
 */



static void ipg_start(struct ifnet *ifp)
{
	struct net_device *sc = ifp->if_softc;
	struct mbuf *mb_head;		
	struct sk_buff *skb;

	while(ifp->if_snd.ifq_head != NULL ){
		
		IF_DEQUEUE(&ifp->if_snd, mb_head);
		
		skb=dev_alloc_skb(mb_head->m_pkthdr.len);
		m_copydata(mb_head, 0, mb_head->m_pkthdr.len, skb->data);
		skb->len=mb_head->m_pkthdr.len;
		ipg_nic_hard_start_xmit(skb,sc);

		m_freem(mb_head);
		wbflush();
	} 
}

static int
ipg_init(struct net_device *netdev)
{
    struct ifnet *ifp = &netdev->arpcom.ac_if;
	int stat=0;
    ifp->if_flags |= IFF_RUNNING;
	if(!netdev->opencount){ stat=netdev_open(netdev);netdev->opencount++;}
	return stat;
}

static int
ipg_stop(struct net_device *netdev)
{
    struct ifnet *ifp = &netdev->arpcom.ac_if;
	ifp->if_timer = 0;
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
//	if(netdev->opencount){netdev_close(netdev);netdev->opencount--;}
	return 0;
}

static int
ipg_ether_ioctl(ifp, cmd, data)
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
			error = ipg_init(sc);
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
			error = ipg_init(sc);
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
			error = ipg_init(sc);
			if(error <0 )
				return(error);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				ipg_stop(sc);
		}
		break;

#if 0
        case SIOCETHTOOL:
        {
        long *p=data;
        mynic_ipg = sc;
        cmd_setmac_ipg(p[0],p[1]);
        }
        break;
       case SIOCRDEEPROM:
                {
                long *p=data;
                mynic_ipg = sc;
                cmd_reprom_ipg(p[0],p[1]);
                }
                break;
       case SIOCWREEPROM:
                {
                long *p=data;
                mynic_ipg = sc;
                cmd_wrprom_ipg(p[0],p[1]);
                }
                break;
#endif 
	default:
		error = EINVAL;
	}

	splx(s);
	return (error);
}

struct cfattach ipg_ca = {
	sizeof(struct net_device), ipg_match, ipg_attach
};

struct cfdriver ipg_cd = {
	NULL, "ipg", DV_IFNET
};
