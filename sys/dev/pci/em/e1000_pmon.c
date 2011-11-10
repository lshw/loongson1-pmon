#ifndef EM_LINUX
#define EM_LINUX
#include "list.h"
//-------------------------------------------------------------------------------
static inline int ls1bit32(unsigned int x)
{
        int b = 31, s;

        s = 16; if (x << 16 == 0) s = 0; b -= s; x <<= s;
        s =  8; if (x <<  8 == 0) s = 0; b -= s; x <<= s;
        s =  4; if (x <<  4 == 0) s = 0; b -= s; x <<= s;
        s =  2; if (x <<  2 == 0) s = 0; b -= s; x <<= s;
        s =  1; if (x <<  1 == 0) s = 0; b -= s;

        return b;
}

static inline int hs1bit32(unsigned int x)
{
        int b = 0, s;

        s = 16; if (x >> 16 == 0) s = 0; b += s; x >>= s;
        s =  8; if (x >>  8 == 0) s = 0; b += s; x >>= s;
        s =  4; if (x >>  4 == 0) s = 0; b += s; x >>= s;
        s =  2; if (x >>  2 == 0) s = 0; b += s; x >>= s;
        s =  1; if (x >>  1 == 0) s = 0; b += s;

        return b;
}

#define MYMINBUCKET	4		/* 4 => min allocation of 16 bytes */
#define MYMAXBUCKET   20		/* 4 => min allocation of 16 bytes */

int bucketindx(unsigned int size)
{
 int b;
 b=hs1bit32(size);
 if(size^(1<<b))b++;
 return (b<MYMINBUCKET)?MYMINBUCKET:(b>MYMAXBUCKET)?MYMAXBUCKET:b;
}

/*
 * Array of descriptors that describe the contents of each page
 */
struct mykmemusage {
	short ku_indx;		/* bucket index */
	union {
		unsigned short freecnt;/* for small allocations, free pieces in page */
		unsigned short pagecnt;/* for large allocations, pages alloced */
	} ku_un;
};
#define	ku_freecnt ku_un.freecnt
#define	ku_pagecnt ku_un.pagecnt

/*
 * Set of buckets for each size of memory block that is retained
 */
struct mykmembuckets {
	caddr_t kb_next;	/* list of free blocks */
};
struct mykmembuckets mybucket[MYMAXBUCKET+1];
struct mykmemusage *mykmemusage;
char *mykmembase, *mykmemlimit;

#define	mybtokup(addr)	(&mykmemusage[((caddr_t)(addr) - mykmembase) >> CLSHIFT])

struct myfreelist {
	caddr_t	next;
};

void mykmeminit();
#define MYVM_KMEM_SIZE (8 * 1024 * 1024)
static vm_offset_t mykmem_offs;
char *mykmem_malloc(unsigned long size)
{
	vm_offset_t p;
if(!mykmembase)mykmeminit();

	size = (size + PGOFSET) & ~PGOFSET;
	if (mykmem_offs + size > MYVM_KMEM_SIZE) {
		log (LOG_DEBUG, "kmem_malloc: request for %d bytes fails\n", size);
		return 0;
	}
	p = (vm_offset_t) &mykmembase[mykmem_offs];
	mykmem_offs += size;
	return p;
}

void
mykmem_free (addr, size)
	void *addr;
	 unsigned int size;
{
	if(!addr)panic ("mykmem_free");
}

/*
 * Allocate a block of memory
 */
void *
mymalloc(size)
	unsigned long size;
{
	register struct mykmembuckets *kbp;
	register struct mykmemusage *kup;
	register struct myfreelist *freep;
	long indx, npg, allocsize;
	int s;
	caddr_t va, cp, savedlist;
	indx = bucketindx(size);
	kbp = &mybucket[indx];
	s = splimp();
	if (kbp->kb_next == NULL) {
		if (size > MAXALLOCSAVE)
			allocsize = clrnd(round_page(size));
		else
			allocsize = 1 << indx;
		npg = clrnd(btoc(allocsize));
		va = (caddr_t)mykmem_malloc( (vsize_t)ctob(npg));
		if (va == NULL) {
			/*
			 * Kmem_malloc() can return NULL, even if it can
			 * wait, if there is no map space available, because
			 * it can't fix that problem.  Neither can we,
			 * right now.  (We should release pages which
			 * are completely free and which are in buckets
			 * with too many free elements.)
			 */
			panic("mymalloc: out of space in kmem_map");
			splx(s);
			return ((void *) NULL);
		}
		kup = mybtokup(va);
		kup->ku_indx = indx;
		if (allocsize > (1<<MYMAXBUCKET)) {
			if (npg > 65535)
				panic("mymalloc: allocation too large");
			kup->ku_pagecnt = npg;
			goto out;
		}
		/*
		 * Just in case we blocked while allocating memory,
		 * and someone else also allocated memory for this
		 * bucket, don't assume the list is still empty.
		 */
		savedlist = kbp->kb_next;
		kbp->kb_next = cp = va + (npg * NBPG) - allocsize;
		for (;;) {
			freep = (struct myfreelist *)cp;
			if (cp <= va)
				break;
			cp -= allocsize;
			freep->next = cp;
		}
		freep->next = savedlist;
	}
	va = kbp->kb_next;
	kbp->kb_next = ((struct myfreelist *)va)->next;
out:
	splx(s);
	return ((void *) va);
}


/*
 * Free a block of memory allocated by malloc.
 */
void
myfree(addr)
	void *addr;
{
	register struct mykmembuckets *kbp;
	register struct mykmemusage *kup;
	register struct myfreelist *freep;
	long size;
	int s;

	kup = mybtokup(addr);
	size = 1 << kup->ku_indx;
	kbp = &mybucket[kup->ku_indx];
	s = splimp();
	if (size > (1<<MYMAXBUCKET)) {
		mykmem_free(addr, ctob(kup->ku_pagecnt));
		splx(s);
		return;
	}
	freep = (struct myfreelist *)addr;
	freep->next = kbp->kb_next;
	kbp->kb_next=freep;
	splx(s);
}

extern int memorysize;
#define  mymemorysize memorysize
/*
 * Initialize the kernel memory allocator
 */
void
mykmeminit()
{
	int npg;

	npg = MYVM_KMEM_SIZE/ NBPG;
 mymemorysize = (mymemorysize -MYVM_KMEM_SIZE) & ~PGOFSET;
 mykmembase= PHYS_TO_CACHED (mymemorysize);
 mykmemusage = (struct mykmemusage *) mykmem_malloc( (vsize_t)(npg * sizeof(struct mykmemusage)));
}
//-------------------------------------------------------------------------------
#define test_and_clear_bit clear_bit
#define test_and_set_bit set_bit
extern __inline__ int set_bit(int nr,long * addr)
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

extern __inline__ int clear_bit(int nr, long * addr)
{
    int mask, retval;

    addr += nr >> 5;
    mask = 1 << (nr & 0x1f);
//    cli();
    retval = (mask & *addr) != 0;
    *addr &= ~mask;
//    sti();
    return retval;
}

extern __inline__ int test_bit(int nr, long * addr)
{
    int mask;

    addr += nr >> 5;
    mask = 1 << (nr & 0x1f);
    return ((mask & *addr) != 0);
}

//--------------------------------------------------------------------------------
#define BUG() printk("BUG()\n");
#define spin_lock_init(x)   
typedef int spinlock_t;
typedef int atomic_t;
inline atomic_t atomic_read(atomic_t *v){  return *v;}
inline atomic_t atomic_inc(atomic_t *v){*v++;return *v;}
inline atomic_t atomic_dec_and_test(atomic_t *v){*v--;return *v; }
#define atomic_set(v,i) (*v = (i))
#define spin_lock_irqsave(...)
#define spin_unlock_irqrestore(...)
#define synchronize_irq()
static int irqstate=0;

int request_irq(irq,b,c,d,e)
{
	irqstate|=(1<<irq);
	return 0;
}

static void free_irq(irq,b)
{
	irqstate &=~(1<<irq);
}

//----------------------------------------------------------------------
#define MODULE_DEVICE_TABLE(...)
#define jiffies ticks
#define eth_type_trans(...) 0
//-----------------------------------------------------
//

struct pt_regs {
	void *unused;
};

#define ioremap(a,l) (a)
#define NETIF_F_SG      1   /* Scatter/gather IO. */
#define NETIF_F_IP_CSUM     2   /* Can checksum only TCP/UDP over IPv4. */
#define NETIF_F_NO_CSUM     4   /* Does not require checksum. F.e. loopack. */
#define NETIF_F_HW_CSUM     8   /* Can checksum all the packets. */
#define NETIF_F_DYNALLOC    16  /* Self-dectructable device. */
#define NETIF_F_HIGHDMA     32  /* Can DMA to high memory. */
#define NETIF_F_FRAGLIST    64  /* Scatter/gather IO. */
#define NETIF_F_HW_VLAN_TX  128 /* Transmit VLAN hw acceleration */
#define NETIF_F_HW_VLAN_RX  256 /* Receive VLAN hw acceleration */
#define NETIF_F_HW_VLAN_FILTER  512 /* Receive filtering on VLAN */
#define NETIF_F_VLAN_CHALLENGED 1024    /* Device cannot handle VLAN packets */

struct tq_struct {
    struct list_head list;      /* linked list of active bh's */
    unsigned long sync;     /* must be initialized to zero */
    void (*routine)(void *);    /* function to call */
    void *data;         /* argument to function */
};


#define TQ_ACTIVE(q)        (!list_empty(&q))
typedef struct list_head task_queue;
static inline int queue_task(struct tq_struct *bh_pointer, task_queue *bh_list)
{
    int ret = 0;
    if (!test_and_set_bit(0,&bh_pointer->sync)) {
        list_add_tail(&bh_pointer->list, bh_list);
        ret = 1;
    }
    return ret;
}

static inline void __run_task_queue(task_queue *list)
{
    struct list_head head, *next;

    list_add(&head, list);
    list_del_init(list);

    next = head.next;
    while (next != &head) {
        void (*f) (void *);
        struct tq_struct *p;
        void *data;

        p = list_entry(next, struct tq_struct, list);
        next = next->next;
        f = p->routine;
        data = p->data;
        p->sync = 0;
        if (f)
		{
            f(data);
		}
    }
}

#define TQ_ACTIVE(q)        (!list_empty(&q))
static inline void run_task_queue(task_queue *list)
{
	    if (TQ_ACTIVE(*list))
			        __run_task_queue(list);
}


#define DECLARE_TASK_QUEUE(q)   LIST_HEAD1(q)
DECLARE_TASK_QUEUE(tq_e1000);

static int schedule_task(struct tq_struct *task)
{
	queue_task(task,&tq_e1000);
	return 0;
}

//m_freem
#define TEST_FIFO_STALL 1
#define __devinitdata
#define __devinit
#define PCI_ANY_ID (~0)
#define __devexit
#define __init
typedef int irqreturn_t;
#define IRQ_NONE  -1  
#define IRQ_HANDLED  0
#define unlikely(x) (x)
#define likely(x) (x)
#define cpu_to_le64(x) (x)
#define cpu_to_le32(x) (x)


#define le16_to_cpu(x) (x)
#define readb(addr)     (*(volatile unsigned char *)(addr))
#define readw(addr)     (*(volatile unsigned short *)(addr))
#define readl(addr)     (*(volatile unsigned int *)(addr))

#define __raw_readb(addr)   (*(volatile unsigned char *)(addr))
#define __raw_readw(addr)   (*(volatile unsigned short *)(addr))
#define __raw_readl(addr)   (*(volatile unsigned int *)(addr))

#define writeb(b,addr) ((*(volatile unsigned char *)(addr)) = (b))
#define writew(w,addr) ((*(volatile unsigned short *)(addr)) = (w))
#define writel(l,addr) ((*(volatile unsigned int *)(addr)) = (l))

#define __raw_writeb(b,addr)    ((*(volatile unsigned char *)(addr)) = (b))
#define __raw_writew(w,addr)    ((*(volatile unsigned short *)(addr)) = (w))
#define __raw_writel(l,addr)    ((*(volatile unsigned int *)(addr)) = (l))

#define PCI_REVISION_ID         0x08    /* Revision ID */
#define PCI_SUBSYSTEM_VENDOR_ID 0x2c
#define PCI_SUBSYSTEM_ID    0x2e 

/* This defines the direction arg to the DMA mapping routines. */
#define PCI_DMA_BIDIRECTIONAL   0
#define PCI_DMA_TODEVICE    1
#define PCI_DMA_FROMDEVICE  2
#define PCI_DMA_NONE        3
#define HZ hz

#define time_after(t1,t2)            (((long)t1-t2) > 0)
extern int ticks;

typedef unsigned int dma_addr_t;
struct e1000_adapter;
struct net_device;
#define MAX_ADDR_LEN    8       /* Largest hardware address length */
#define IFNAMSIZ 16
struct dev_mc_list 
{
    struct dev_mc_list  *next;
    u_int8_t       dmi_addr[MAX_ADDR_LEN];
    unsigned char       dmi_addrlen;
    int         dmi_users;
    int         dmi_gusers;
};

struct sk_buff {
    unsigned int    len;            /* Length of actual data            */
    unsigned char   *data;          /* Data head pointer                */
    unsigned char   *head;          /* Data head pointer                */
	unsigned char protocol;
	struct net_device *dev;
};

struct pci_device_id {
    unsigned int vendor, device;        /* Vendor and device ID or PCI_ANY_ID */
    unsigned int subvendor, subdevice;  /* Subsystem ID's or PCI_ANY_ID */
    unsigned int class, class_mask;     /* (class,subclass,prog-if) triplet */
    unsigned long driver_data;      /* Data private to the driver */
};

#define printk printf
#define KERN_INFO
#define KERN_DEBUG
#define KERN_NOTICE
#define KERN_ERR
#define KERN_WARNING

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
struct timer_list {
	struct list_head list;
    unsigned long expires;
    unsigned long data;
    void (*function)(unsigned long);
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
static inline void *pci_get_drvdata (struct pci_dev *pdev)
{
    return pdev->dev;
}

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
};

enum netdev_state_t
{
    __LINK_STATE_XOFF=0,
    __LINK_STATE_START,
    __LINK_STATE_PRESENT,
    __LINK_STATE_SCHED,
    __LINK_STATE_NOCARRIER,
    __LINK_STATE_RX_SCHED
};

extern pcireg_t _pci_conf_readn(pcitag_t tag, int reg, int width);
extern void _pci_conf_writen(pcitag_t tag, int reg, pcireg_t data,int width);
static int pci_read_config_dword(struct pci_dev *linuxpd, int reg, u32 *val);
static int pci_write_config_dword(struct pci_dev *linuxpd, int reg, u32 val);
static int pci_read_config_word(struct pci_dev *linuxpd, int reg, u16 *val);
static int pci_write_config_word(struct pci_dev *linuxpd, int reg, u16 val);
static int pci_read_config_byte(struct pci_dev *linuxpd, int reg, u8 *val);
static int pci_write_config_byte(struct pci_dev *linuxpd, int reg, u8 val);


static inline int netif_carrier_ok(struct net_device *dev)
{
    return !test_bit(__LINK_STATE_NOCARRIER, &dev->state);
}

static inline void netif_start_queue(struct net_device *dev)
{
    clear_bit(__LINK_STATE_XOFF, &dev->state);
}

static inline void netif_wake_queue(struct net_device *dev)
{
    if (test_and_clear_bit(__LINK_STATE_XOFF, &dev->state));// __netif_schedule(dev);
}

static inline void netif_stop_queue(struct net_device *dev)
{
    set_bit(__LINK_STATE_XOFF, &dev->state);
}

static inline int netif_queue_stopped(struct net_device *dev)
{
    return test_bit(__LINK_STATE_XOFF, &dev->state);
}

static inline int netif_running(struct net_device *dev)
{
    return test_bit(__LINK_STATE_START, &dev->state);
}

static inline void netif_carrier_on(struct net_device *dev)
{
    clear_bit(__LINK_STATE_NOCARRIER, &dev->state);
    if (netif_running(dev));// __netdev_watchdog_up(dev);
}

static inline void netif_carrier_off(struct net_device *dev)
{
    set_bit(__LINK_STATE_NOCARRIER, &dev->state);
}

/* Hot-plugging. */
static inline int netif_device_present(struct net_device *dev)
{
    return test_bit(__LINK_STATE_PRESENT, &dev->state);
}
static inline void netif_device_detach(struct net_device *dev)
{
    if (test_and_clear_bit(__LINK_STATE_PRESENT, &dev->state) &&
        netif_running(dev)) {
        netif_stop_queue(dev);
    }
}

static inline void netif_device_attach(struct net_device *dev)
{
    if (!test_and_set_bit(__LINK_STATE_PRESENT, &dev->state) &&
        netif_running(dev)) {
//        netif_wake_queue(dev);
//        __netdev_watchdog_up(dev);
    }
}


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


static inline void init_timer(struct timer_list * timer)
{
    timer->list.next = timer->list.prev = NULL;
}

int mod_timer(struct timer_list *timer, unsigned long expires)
{
untimeout(( void (*) __P((void *)))timer->function,(void *)timer->data);
timeout(( void (*) __P((void *)))timer->function,(void *)timer->data,expires-ticks);
return 0;
}

int del_timer_sync(struct timer_list * timer)
{
untimeout(( void (*) __P((void *)))timer->function,(void *)timer->data);
return 0;
}

#define INIT_LIST_HEAD(ptr) do { \
    (ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

#define PREPARE_TQUEUE(_tq, _routine, _data)            \
    do {                            \
        (_tq)->routine = _routine;          \
        (_tq)->data = _data;                \
    } while (0)

#define INIT_TQUEUE(_tq, _routine, _data)           \
    do {                            \
        INIT_LIST_HEAD(&(_tq)->list);           \
        (_tq)->sync = 0;                \
        PREPARE_TQUEUE((_tq), (_routine), (_data)); \
    } while (0)

#define GFP_KERNEL 0
int mysetenv(char *name,char *fmt,...);
inline void * kmalloc (size_t size, int flags)
{
 char str[40];
 void *ret=mymalloc(size);

 
 if(!ret){
 printf("\nerror:kmalloc failed,press enter to continue\n");
 gets(str); 
 }
 return ret;
}

inline void *kfree(const void *objp)
{
myfree(objp);
}

#define SA_SHIRQ        0x02000000
#define SA_SAMPLE_RANDOM    0x002
#define CHECKSUM_NONE 0
#define CHECKSUM_HW 1
#define CHECKSUM_UNNECESSARY 2
#define MAX_SKB_FRAGS 6
struct pci_device;

#define PCI_MAP_REG_START 0x10
static int pci_resource_start(struct pci_dev *pdev,int bar)
{
	int reg=PCI_MAP_REG_START+bar*4;
	int rv,start,size,flags,type;
		type=pci_mapreg_type(0,pdev->pa.pa_tag,reg);
		rv=pci_mapreg_info(0,pdev->pa.pa_tag,reg,type,&start,&size,&flags);
		if (PCI_MAPREG_TYPE(type) == PCI_MAPREG_TYPE_IO)
			start=pdev->pa.pa_iot->bus_base+start;
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
			start=pdev->pa.pa_iot->bus_base+start;
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

#define  PCI_BASE_ADDRESS_SPACE_IO 0x01
#define  PCI_BASE_ADDRESS_SPACE_MEMORY 0x00
#define  PCI_BASE_ADDRESS_MEM_PREFETCH  0x08    /* prefetchable? */
#define IORESOURCE_IO       0x00000100  /* Resource type */
#define IORESOURCE_MEM      0x00000200
#define IORESOURCE_IRQ      0x00000400
#define IORESOURCE_DMA      0x00000800
#define IORESOURCE_PREFETCH 0x00001000  /* No side effects */
/*
 * Translate the low bits of the PCI base
 * to the resource type
 */
static inline unsigned int pci_calc_resource_flags(unsigned int flags)
{
    if (flags & PCI_BASE_ADDRESS_SPACE_IO)
        return IORESOURCE_IO;

    if (flags & PCI_BASE_ADDRESS_MEM_PREFETCH)
        return IORESOURCE_MEM | IORESOURCE_PREFETCH;

    return IORESOURCE_MEM;
}

static int pci_resource_flags(struct pci_dev *pdev,int bar)
{
int reg=PCI_MAP_REG_START+bar*4;
	return pci_calc_resource_flags(pci_mapreg_type(0,pdev->pa.pa_tag,reg));
}



void
pci_free_consistent(struct pci_dev *pdev, size_t size, void *cpu_addr,
            dma_addr_t dma_addr)
{
	kfree(cpu_addr);
}

//pci_alloc_consistent 最后一个参数是DMA地址，返回的是非cache的cpu地址。
static void *pci_alloc_consistent(void *hwdev, size_t size,
		               dma_addr_t * dma_handle)
{
void *buf;
    buf = kmalloc(size,GFP_KERNEL);
//    pci_sync_cache(hwdev, buf,size, SYNC_W);

    buf = (unsigned char *)CACHED_TO_UNCACHED(buf);
    *dma_handle =vtophys(buf);

	return (void *)buf;
}


/*
 * Unmap a single streaming mode DMA translation.  The dma_addr and size
 * must match what was provided for in a previous pci_map_single call.  All
 * other usages are undefined.
 *
 * After this call, reads by the cpu to the buffer are guarenteed to see
 * whatever the device wrote there.
 */
extern vm_offset_t _pci_cpumap(vm_offset_t pcia,unsigned int len);
extern vm_offset_t _pci_dmamap(vm_offset_t pcia,unsigned int len);
static inline void pci_unmap_single(struct pci_dev *hwdev, dma_addr_t dma_addr,
                    size_t size, int direction)
{
pci_sync_cache(hwdev, _pci_cpumap(dma_addr,size), size, SYNC_R);
}

/*
 *  * Map a single buffer of the indicated size for DMA in streaming mode.
 *   * The 32-bit bus address to use is returned.
 *    *
 *     * Once the device is given the dma address, the device owns this memory
 *      * until either pci_unmap_single or pci_dma_sync_single is performed.
 *       */
static inline dma_addr_t pci_map_single(struct pci_dev *hwdev, void *ptr,
		                    size_t size, int direction)
{
	    unsigned long addr = (unsigned long) ptr;
pci_sync_cache(hwdev,addr,size, SYNC_W);
return _pci_dmamap(addr,size);
}

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

static inline void pci_unmap_page(struct pci_dev *hwdev, dma_addr_t dma_address,
                  size_t size, int direction)
{
	pci_sync_cache(hwdev, _pci_cpumap(dma_address,size), size, SYNC_R);
}

#define PCI_COMMAND     0x04    /* 16 bits */
#define  PCI_COMMAND_INVALIDATE 0x10    /* Use memory write and invalidate */
#define SMP_CACHE_BYTES     (PCI_CACHE_LINE_SIZE<<2)
#define _PCI_CACHE_LINE_SIZE 0x0c    /* 8 bits */

int
pdev_set_mwi(struct pci_dev *dev)
{
    int rc = 0;
    u8 cache_size;

    /*
     * Looks like this is necessary to deal with on all architectures,
     * even this %$#%$# N440BX Intel based thing doesn't get it right.
     * Ie. having two NICs in the machine, one will have the cache
     * line set at boot time, the other will not.
     */
    pci_read_config_byte(dev, PCI_BHLC_REG, &cache_size);
    cache_size <<= 2;
    if (cache_size != SMP_CACHE_BYTES) {
        printk(KERN_WARNING "PCI:  PCI cache line size set incorrectly (%i bytes) by BIOS/FW.\n",
               cache_size);
        if (cache_size > SMP_CACHE_BYTES) {
            printk("PCI: cache line size too large - expecting %i.\n", SMP_CACHE_BYTES);
            rc = -EINVAL;
        } else {
            printk("PCI: PCI cache line size corrected to %i.\n", SMP_CACHE_BYTES);
            pci_write_config_byte(dev, PCI_BHLC_REG,
                          SMP_CACHE_BYTES >> 2);
        }
    }

    return rc;
}

int
pci_set_mwi(struct pci_dev *dev)
{
    int rc;
    u16 cmd;

    rc = pdev_set_mwi(dev);

    if (rc)
        return rc;

    pci_read_config_word(dev, PCI_COMMAND, &cmd);
    if (! (cmd & PCI_COMMAND_INVALIDATE)) {
        printk("PCI: Enabling Mem-Wr-Inval for device %s\n", dev->pa.pa_device);
        cmd |= PCI_COMMAND_INVALIDATE;
        pci_write_config_word(dev, PCI_COMMAND, cmd);
    }

    return 0;
}


void
pci_clear_mwi(struct pci_dev *dev)
{
    u16 cmd;

    pci_read_config_word(dev, PCI_COMMAND, &cmd);
    if (cmd & PCI_COMMAND_INVALIDATE) {
        cmd &= ~PCI_COMMAND_INVALIDATE;
        pci_write_config_word(dev, PCI_COMMAND, cmd);
    }
}

static inline int pci_save_state(struct pci_dev *dev, u32 *buffer) { return 0; }
static inline int pci_restore_state(struct pci_dev *dev, u32 *buffer) { return 0; }
static inline int pci_set_power_state(struct pci_dev *dev, int state) { return 0; }
static inline int pci_enable_wake(struct pci_dev *dev, u32 state, int enable) { return 0; }

static struct mbuf * getmbuf(struct net_device *nic)
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
	
	m->m_data += RFA_ALIGNMENT_FUDGE;
	return m;
}

/**
 * memcmp - Compare two areas of memory
 * @cs: One area of memory
 * @ct: Another area of memory
 * @count: The size of the area.
 */
static int memcmp(const void * cs,const void * ct,size_t count)
{
	const unsigned char *su1, *su2;
	signed char res = 0;

	for( su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
		if ((res = *su1 - *su2) != 0)
			break;
	return res;
}
static inline int is_valid_ether_addr( u_int8_t *addr )
{
    const char zaddr[6] = {0,};

    return !(addr[0]&1) && memcmp( addr, zaddr, 6);
}

static inline void iounmap(void *addr)
{
}
static inline void pci_disable_device(struct pci_dev *dev) { }

void wmb(void){}
void mdelay(int ms){while(ms--)delay(1000);}
#define udelay delay

#define MODULE_PARM(...)
#define MODULE_PARM_DESC(...)
#define __MODULE_STRING_1(x)    #x
#define __MODULE_STRING(x)  __MODULE_STRING_1(x)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

inline int netif_rx(struct sk_buff *skb)
{
	struct mbuf *m;
	struct ether_header *eh;
	struct net_device *netdev=skb->dev;
	struct ifnet *ifp = &netdev->arpcom.ac_if;
    m =getmbuf(netdev);
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
#endif

