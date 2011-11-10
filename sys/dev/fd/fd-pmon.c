#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
//#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <vm/vm.h>
#include <include/autoconf.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include<linux/io.h>
#include <stdlib.h>

#ifdef BONITOEL  
#include <include/bonito.h>
#endif

#include <target/sbd.h>
#define HZ 100
#define printk printf
#define FDF_LOADED    0x10
#define DBG(x...) printf("%s,%d:%s\n",__FILE__,__LINE__,x)
#define DEVICE_INTR do_floppy
#define BLOCK_SIZE 1024
#define DEVICE_NAME "fd"
#define READ 0
#define WRITE 1
#define wdlookup(unit) (struct wd_softc *)device_lookup(&wd_cd, (unit))

#define RQ_INACTIVE		(-1)
#define RQ_ACTIVE		1
#define RQ_SCSI_BUSY		0xffff
#define RQ_SCSI_DONE		0xfffe
#define RQ_SCSI_DISCONNECTING	0xffe0

struct inode{
int i_rdev;
};

typedef int kdev_t;
struct buffer_head {
    struct buffer_head *b_next; /* Hash queue list */
    unsigned long b_blocknr;    /* block number */
    unsigned short b_size;      /* block size */
    unsigned short b_list;      /* List that this buffer appears */
    kdev_t b_dev;           /* device (B_FREE = free) */
    kdev_t b_rdev;          /* Real device */
    unsigned long b_state;      /* buffer state bitmap (see above) */
	struct {
        caddr_t b_addr;     /* Memory, superblocks, indirect etc. */
    } b_un;
    void (*b_end_io)(struct buffer_head *bh, int uptodate); /* I/O completion */
    void *b_private;        /* reserved for b_end_io */
    void *b_journal_head;       /* ext3 journal_heads */
    unsigned long b_rsector;    /* Real buffer location on disk */
	struct buf *bp;
};
/* bh state bits */
enum bh_state_bits {
    BH_Uptodate,    /* 1 if the buffer contains valid data */
    BH_Dirty,   /* 1 if the buffer is dirty */
    BH_Lock,    /* 1 if the buffer is locked */
    BH_Req,     /* 0 if the buffer has been invalidated */
    BH_Mapped,  /* 1 if the buffer has a disk mapping */
    BH_New,     /* 1 if the buffer is new and not yet written out */
    BH_Async,   /* 1 if the buffer is under end_buffer_io_async I/O */
    BH_Wait_IO, /* 1 if we should write out this buffer */
    BH_Launder, /* 1 if we can throttle on this buffer */
    BH_Attached,    /* 1 if b_inode_buffers is linked into a list */
    BH_JBD,     /* 1 if it has an attached journal_head */
    BH_Sync,    /* 1 if the buffer is a sync read */
    BH_Delay,       /* 1 if the buffer is delayed allocate */

    BH_PrivateStart,/* not a state bit, but the first bit available
             * for private allocation by other entities
             */
};

struct request {
    volatile int rq_status; /* should split this into a few status bits */
#define RQ_INACTIVE     (-1)
#define RQ_ACTIVE       1
#define RQ_SCSI_BUSY        0xffff
#define RQ_SCSI_DONE        0xfffe
#define RQ_SCSI_DISCONNECTING   0xffe0

    kdev_t rq_dev;
    int cmd;        /* READ or WRITE */
    int errors;
    unsigned long start_time;
    unsigned long sector;
    unsigned long nr_sectors;
    unsigned long hard_sector, hard_nr_sectors;
    unsigned int nr_segments;
    unsigned int nr_hw_segments;
    unsigned long current_nr_sectors, hard_cur_sectors;
    void * special;
    char * buffer;
    int * waiting;
    struct buffer_head * bh;
    struct buffer_head * bhtail;
//    struct request * next;
};


struct hw_interrupt_type {
    const char * typename;
    unsigned int (*startup)(unsigned int irq);
    void (*shutdown)(unsigned int irq);
    void (*enable)(unsigned int irq);
    void (*disable)(unsigned int irq);
    void (*ack)(unsigned int irq);
    void (*end)(unsigned int irq);
    void (*set_affinity)(unsigned int irq, unsigned long mask);
};
//-----------------------------------------------------------------------
extern int ticks;
inline int lock_fdc(int drive,int flags){return  0;}
#define unlock_fdc(...)
#define spin_lock_irqsave(...)
#define spin_unlock_irqrestore(...)
#define request_region(...) 1
#define release_region(...)
#define devfs_register(...)
#define devfs_mk_dir(...) 0
#define devfs_register_blkdev(...) 0
#define devfs_unregister_blkdev(...)
#define blk_cleanup_queue(...)
#define DECLARE_WAITQUEUE(q,t) int q
#define __init
#define KERN_INFO
#define MOD_INC_USE_COUNT
#define MOD_DEC_USE_COUNT
#define kmalloc(size,type) malloc(size)

typedef int devfs_handle_t;
typedef int spinlock_t;
typedef int64_t u64;

#define PHYSADDR(a) (((unsigned long)(a)) & 0x1fffffff)
#define virt_to_bus(x)  (PHYSADDR((x)))

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
//----------------------------------------------------------------------------------

#define HZ 100
extern __inline__ void
__delay(unsigned long loops)
{
	__asm__ __volatile__ (
		".set\tnoreorder\n"
		"1:\tbnez\t%0,1b\n\t"
		"subu\t%0,1\n\t"
		".set\treorder"
		:"=r" (loops)
		:"0" (loops));
}

/*
 * Division by multiplication: you don't have to worry about
 * loss of precision.
 *
 * Use only for very small delays ( < 1 msec).  Should probably use a
 * lookup table, really, as the multiplications take much too long with
 * short delays.  This is a "reasonable" implementation, though (and the
 * first constant multiplications gets optimized away if the delay is
 * a constant)
 */
extern __inline__ void __udelay(unsigned long usecs, unsigned long lpj)
{
	unsigned long lo;

	/*
	 * Excessive precission?  Probably ...
	 */
	usecs *= (unsigned long) (((0x8000000000000000ULL / (500000 / HZ)) +
	                           0x80000000ULL) >> 32);
	__asm__("multu\t%2,%3"
		:"=h" (usecs), "=l" (lo)
		:"r" (usecs),"r" (lpj));
	__delay(usecs);
}

extern __inline__ void __ndelay(unsigned long nsecs, unsigned long lpj)
{
	unsigned long lo;

	/*
	 * Excessive precission?  Probably ...
	 */
	nsecs *= (unsigned long) (((0x8000000000000000ULL / (500000000 / HZ)) +
	                           0x80000000ULL) >> 32);
	__asm__("multu\t%2,%3"
		:"=h" (nsecs), "=l" (lo)
		:"r" (nsecs),"r" (lpj));
	__delay(nsecs);
}
#define __udelay_val 1024
#define udelay(usecs) __udelay((usecs),__udelay_val)
#define ndelay(nsecs) __ndelay((nsecs),__udelay_val)

#define FD_RESET_DELAY 20
//---------------------------------------------------------------------

#include "dma.h"

struct fd_ops {
	unsigned char (*fd_inb)(unsigned int port);
	void (*fd_outb)(unsigned char value, unsigned int port);

	/*
	 * How to access the floppy DMA functions.
	 */
	void (*fd_enable_dma)(int channel);
	void (*fd_disable_dma)(int channel);
	int (*fd_request_dma)(int channel);
	void (*fd_free_dma)(int channel);
	void (*fd_clear_dma_ff)(int channel);
	void (*fd_set_dma_mode)(int channel, char mode);
	void (*fd_set_dma_addr)(int channel, unsigned int a);
	void (*fd_set_dma_count)(int channel, unsigned int count);
	int (*fd_get_dma_residue)(int channel);
	void (*fd_enable_irq)(int irq);
	void (*fd_disable_irq)(int irq);
	unsigned long (*fd_getfdaddr1)(void);
	unsigned long (*fd_dma_mem_alloc)(unsigned long size);
	void (*fd_dma_mem_free)(unsigned long addr, unsigned long size);
	unsigned long (*fd_drive_type)(unsigned long);
};

extern struct fd_ops *fd_ops;

#define fd_inb(port)		fd_ops->fd_inb(port)
#define fd_outb(value,port)	fd_ops->fd_outb(value,port)

#define fd_enable_dma()		fd_ops->fd_enable_dma(FLOPPY_DMA)
#define fd_disable_dma()	fd_ops->fd_disable_dma(FLOPPY_DMA)
#define fd_request_dma()	fd_ops->fd_request_dma(FLOPPY_DMA)
#define fd_free_dma()		fd_ops->fd_free_dma(FLOPPY_DMA)
#define fd_clear_dma_ff()	fd_ops->fd_clear_dma_ff(FLOPPY_DMA)
#define fd_set_dma_mode(mode)	fd_ops->fd_set_dma_mode(FLOPPY_DMA, mode)
#define fd_set_dma_addr(addr)	fd_ops->fd_set_dma_addr(FLOPPY_DMA, \
				                       virt_to_bus(addr))
#define fd_set_dma_count(count)	fd_ops->fd_set_dma_count(FLOPPY_DMA,count)
#define fd_get_dma_residue()	fd_ops->fd_get_dma_residue(FLOPPY_DMA)

#define fd_enable_irq()		fd_ops->fd_enable_irq(FLOPPY_IRQ)
#define fd_disable_irq()	fd_ops->fd_disable_irq(FLOPPY_IRQ)
#define fd_dma_mem_alloc(size)	fd_ops->fd_dma_mem_alloc(size)
#define fd_dma_mem_free(mem,size) fd_ops->fd_dma_mem_free(mem,size)
#define fd_drive_type(n)	fd_ops->fd_drive_type(n)
#define fd_cacheflush(addr,size) CPU_FlushCache() //flushdcache(addr,size)//CPU_FlushDCache(addr,size);

#define MAX_BUFFER_SECTORS 24


/*
 * And on Mips's the CMOS info fails also ...
 *
 * FIXME: This information should come from the ARC configuration tree
 *        or whereever a particular machine has stored this ...
 */
#define FLOPPY0_TYPE 			fd_drive_type(0)
#define FLOPPY1_TYPE			fd_drive_type(1)

#define FDC1			fd_ops->fd_getfdaddr1();

#define N_FDC 1			/* do you *really* want a second controller? */
#define N_DRIVE 8

#define FLOPPY_MOTOR_MASK 0xf0

/*
 * The DMA channel used by the floppy controller cannot access data at
 * addresses >= 16MB
 *
 * Went back to the 1MB limit, as some people had problems with the floppy
 * driver otherwise. It doesn't matter much for performance anyway, as most
 * floppy accesses go through the track buffer.
 *
 * On MIPSes using vdma, this actually means that *all* transfers go thru
 * the * track buffer since 0x1000000 is always smaller than KSEG0/1.
 * Actually this needs to be a bit more complicated since the so much different
 * hardware available with MIPS CPUs ...
 */
#define CROSS_64KB(a,s) ((unsigned long)(a)/K_64 != ((unsigned long)(a) + (s) - 1) / K_64)

#define EXTRA_FLOPPY_PARAMS
//------------------------------------------------------------------------
#define mark_bh(...)
#define fdlookup(unit) (struct fd_softc *)device_lookup(&fd_cd, (unit))

typedef struct request *request_queue_t;
#define CURRENT current_request
#define CURRENT_DEV DEVICE_NR(CURRENT->dev)
#define LOCAL_END_REQUEST

extern int floppy_init(void);
extern int rd_doload;		/* 1 = load ramdisk, 0 = don't load */
extern int rd_prompt;		/* 1 = prompt for ramdisk, 0 = don't prompt */
extern int rd_image_start;	/* starting block # of image */


		 
/*
 * end_request() and friends. Must be called with the request queue spinlock
 * acquired. All functions called within end_request() _must_be_ atomic.
 *
 * Several drivers define their own end_request and call
 * end_that_request_first() and end_that_request_last()
 * for parts of the original function. This prevents
 * code duplication in drivers.
 */


int end_that_request_first(struct request *req, int uptodate, char *name);
void end_that_request_last(struct request *req);


#undef DEVICE_ON
#undef DEVICE_OFF

/*
 * Add entries as needed.
 */


static void floppy_off(unsigned int nr);

//#define DEVICE_NAME "floppy"
#define MAJOR(dev)  ((dev)>>8)
#define MINOR(dev)  ((dev) & 0xff)
#define MKDEV(ma,mi)    ((ma)<<8 | (mi))
#define DEVICE_INTR do_floppy
#define DEVICE_REQUEST do_fd_request
#define DEVICE_NR(device) ( (MINOR(device) & 3) | ((MINOR(device) & 0x80 ) >> 5 ))
#define DEVICE_OFF(device) floppy_off(DEVICE_NR(device))


/* provide DEVICE_xxx defaults, if not explicitly defined
 * above in the MAJOR_NR==xxx if-elif tree */
#ifndef DEVICE_ON
#define DEVICE_ON(device) do {} while (0)
#endif
#ifndef DEVICE_OFF
#define DEVICE_OFF(device) do {} while (0)
#endif

#ifndef QUEUE_EMPTY
#define QUEUE_EMPTY (!CURRENT)
#endif

#ifndef DEVICE_NAME
#define DEVICE_NAME "unknown"
#endif

#ifdef DEVICE_INTR
static void (*DEVICE_INTR)(void) = NULL;
#endif

#define SET_INTR(x) (DEVICE_INTR = (x))

#ifdef DEVICE_REQUEST
static void (DEVICE_REQUEST)(request_queue_t *);
#endif 
  
#ifdef DEVICE_INTR
#define CLEAR_INTR SET_INTR(NULL)
#else
#define CLEAR_INTR
#endif

#define INIT_REQUEST \
	if (QUEUE_EMPTY) {\
		CLEAR_INTR; \
		return; \
	} 

static int floppy_open(struct inode * inode);
static int floppy_release(struct inode * inode);

#include "fd.h"
#include "fdreg.h"
#include "dma.h"

#define SPIN_LOCK_UNLOCKED 1
//-------------------------------------------------------------------------------------
void enable_8259A_irq(unsigned int irq);
void disable_8259A_irq(unsigned int irq);

/*
 * This is the 'legacy' 8259A Programmable Interrupt Controller,
 * present in the majority of PC/AT boxes.
 * plus some generic x86 specific things if generic specifics makes
 * any sense at all.
 * this file should become arch/i386/kernel/irq.c when the old irq.c
 * moves to arch independent land
 */

static spinlock_t i8259A_lock = SPIN_LOCK_UNLOCKED;

static void end_8259A_irq (unsigned int irq)
{
		enable_8259A_irq(irq);
}

#define shutdown_8259A_irq	disable_8259A_irq

void mask_and_ack_8259A(unsigned int);

static unsigned int startup_8259A_irq(unsigned int irq)
{
	enable_8259A_irq(irq);
//	prom_printf("8259 irq %x enabled\n", irq);

	return 0; /* never anything pending */
}

static struct hw_interrupt_type i8259A_irq_type = {
	"XT-PIC",
	startup_8259A_irq,
	shutdown_8259A_irq,
	enable_8259A_irq,
	disable_8259A_irq,
	mask_and_ack_8259A,
	end_8259A_irq,
	NULL
};

/*
 * 8259A PIC functions to handle ISA devices:
 */

/*
 * This contains the irq mask for both 8259A irq controllers,
 */
static unsigned int cached_irq_mask = 0xffff;

#define cached_21	(cached_irq_mask)
#define cached_A1	(cached_irq_mask >> 8)

void disable_8259A_irq(unsigned int irq)
{
	unsigned int mask = 1 << irq;
	unsigned long flags;

	cached_irq_mask |= mask;
	if (irq & 8)
		linux_outb(cached_A1,0xA1);
	else
		linux_outb(cached_21,0x21);
}

void enable_8259A_irq(unsigned int irq)
{
	unsigned int mask = ~(1 << irq);
	unsigned long flags;

	cached_irq_mask &= mask;
	if (irq & 8)
		linux_outb(cached_A1,0xA1);
	else
		linux_outb(cached_21,0x21);
}

int i8259A_irq_pending(unsigned int irq)
{
	unsigned int mask = 1 << irq;
	unsigned long flags;
	int ret;

	if (irq < 8)
		ret = linux_inb(0x20) & mask;
	else
		ret = linux_inb(0xA0) & (mask >> 8);

	return ret;
}


/*
 * This function assumes to be called rarely. Switching between
 * 8259A registers is slow.
 * This has to be protected by the irq controller spinlock
 * before being called.
 */
static inline int i8259A_irq_real(unsigned int irq)
{
	int value;
	int irqmask = 1 << irq;

	if (irq < 8) {
		linux_outb(0x0B,0x20);		/* ISR register */
		value = linux_inb(0x20) & irqmask;
		linux_outb(0x0A,0x20);		/* back to the IRR register */
		return value;
	}
	linux_outb(0x0B,0xA0);		/* ISR register */
	value = linux_inb(0xA0) & (irqmask >> 8);
	linux_outb(0x0A,0xA0);		/* back to the IRR register */
	return value;
}

/*
 * Careful! The 8259A is a fragile beast, it pretty
 * much _has_ to be done exactly like this (mask it
 * first, _then_ send the EOI, and the order of EOI
 * to the two 8259s is important!
 */
void mask_and_ack_8259A(unsigned int irq)
{
	unsigned int irqmask = 1 << irq;
	unsigned long flags;

	/*
	 * Lightweight spurious IRQ detection. We do not want to overdo
	 * spurious IRQ handling - it's usually a sign of hardware problems, so
	 * we only do the checks we can do without slowing down good hardware
	 * nnecesserily.
	 *
	 * Note that IRQ7 and IRQ15 (the two spurious IRQs usually resulting
	 * rom the 8259A-1|2 PICs) occur even if the IRQ is masked in the 8259A.
	 * Thus we can check spurious 8259A IRQs without doing the quite slow
	 * i8259A_irq_real() call for every IRQ.  This does not cover 100% of
	 * spurious interrupts, but should be enough to warn the user that
	 * there is something bad going on ...
	 */
	if (cached_irq_mask & irqmask)
		goto spurious_8259A_irq;
	cached_irq_mask |= irqmask;

handle_real_irq:
	if (irq & 8) {
		linux_inb(0xA1);		/* DUMMY - (do we need this?) */
		linux_outb(cached_A1,0xA1);
		linux_outb(0x60+(irq&7),0xA0);/* 'Specific EOI' to slave */
		linux_outb(0x62,0x20);	/* 'Specific EOI' to master-IRQ2 */
	} else {
		linux_inb(0x21);		/* DUMMY - (do we need this?) */
		linux_outb(cached_21,0x21);
		linux_outb(0x60+irq,0x20);	/* 'Specific EOI' to master */
	}
	return;

spurious_8259A_irq:
	/*
	 * this is the slow path - should happen rarely.
	 */
	if (i8259A_irq_real(irq))
		/*
		 * oops, the IRQ _is_ in service according to the
		 * 8259A - not spurious, go handle it.
		 */
		goto handle_real_irq;

	{
		static int spurious_irq_mask = 0;
		/*
		 * At this point we can be sure the IRQ is spurious,
		 * lets ACK and report it. [once per IRQ]
		 */
		if (!(spurious_irq_mask & irqmask)) {
			printk("spurious 8259A interrupt: IRQ%d.\n", irq);
			spurious_irq_mask |= irqmask;
		}
		/*
		 * Theoretically we do not have to handle this IRQ,
		 * but in Linux this does not cause problems and is
		 * simpler for us.
		 */
		goto handle_real_irq;
	}
}

void  init_8259A(int auto_eoi)
{


	linux_outb(0xff, 0x21);	/* mask all of 8259A-1 */
	linux_outb(0xff, 0xA1);	/* mask all of 8259A-2 */

	/*
	 * linux_outb_p - this has to work on a wide range of PC hardware.
	 */
	linux_outb_p(0x11, 0x20);	/* ICW1: select 8259A-1 init */
	linux_outb_p(0x00, 0x21);	/* ICW2: 8259A-1 IR0-7 mapped to 0x00-0x07 */
	linux_outb_p(0x04, 0x21);	/* 8259A-1 (the master) has a slave on IR2 */
	if (auto_eoi)
		linux_outb_p(0x03, 0x21);	/* master does Auto EOI */
	else
		linux_outb_p(0x01, 0x21);	/* master expects normal EOI */

	linux_outb_p(0x11, 0xA0);	/* ICW1: select 8259A-2 init */
	linux_outb_p(0x08, 0xA1);	/* ICW2: 8259A-2 IR0-7 mapped to 0x08-0x0f */
	linux_outb_p(0x02, 0xA1);	/* 8259A-2 is a slave on master's IR2 */
	linux_outb_p(0x01, 0xA1);	/* (slave's support for AEOI in flat mode
				    is to be investigated) */

	if (auto_eoi)
		/*
		 * in AEOI mode we just have to mask the interrupt
		 * when acking.
		 */
		i8259A_irq_type.ack = disable_8259A_irq;
	else
		i8259A_irq_type.ack = mask_and_ack_8259A;

	udelay(100);		/* wait for 8259A to initialize */

	linux_outb(cached_21, 0x21);	/* restore master IRQ mask */
	linux_outb(cached_A1, 0xA1);	/* restore slave IRQ mask */

}



/*
 * On systems with i8259-style interrupt controllers we assume for
 * driver compatibility reasons interrupts 0 - 15 to be the i8295
 * interrupts even if the hardware uses a different interrupt numbering.
 */
void  init_i8259_irqs (void)
{

	init_8259A(0);

	i8259A_irq_type.startup(2);
}
//-----------------------------------------------------------------------------------
#if 1
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#ifndef IDE_PORT_FIX1
#define IDE_PORT_FIX1  0x3f50 //0xff10
#define IDE_PORT_FIX2  0x3f60 //0xff80
#endif
void fixup_ide()
        {
			pcitag_t pdev;
			int i;
			for(i=0;i<32;i++)
			{
				/*82371 fixup*/
				pdev=_pci_make_tag(0,i,1);
				if(_pci_conf_read(pdev,0)==0x71118086)
				{
					_pci_conf_write(pdev,0x20,IDE_PORT_FIX1);
					pdev=_pci_make_tag(0,i,2);
					_pci_conf_write(pdev,0x20,IDE_PORT_FIX2);
					break;
				}
			}

        }
#endif
//----------------------------------------------------------------------------------

static unsigned char std_fd_inb(unsigned int port)
{
//	myprintf("std_fd_inb(0x%x)\n",port);
	return linux_inb(port);
}

static void std_fd_outb(unsigned char value, unsigned int port)
{
//	myprintf("std_fd_out(0x%x,0x%x)\n",value,port);
	linux_outb(value, port);
}

/*
 * How to access the floppy DMA functions.
 */
static void std_fd_enable_dma(int channel)
{
	enable_dma(channel);
}

static void std_fd_disable_dma(int channel)
{
	disable_dma(channel);
}

static int std_fd_request_dma(int channel)
{
	return 0;//request_dma(channel, "floppy");
}

static void std_fd_free_dma(int channel)
{
	//free_dma(channel);
}

static void std_fd_clear_dma_ff(int channel)
{
	clear_dma_ff(channel);
}

static void std_fd_set_dma_mode(int channel, char mode)
{
	set_dma_mode(channel, mode);
}

static void std_fd_set_dma_addr(int channel, unsigned int addr)
{
	set_dma_addr(channel, addr);
}

static void std_fd_set_dma_count(int channel, unsigned int count)
{
	set_dma_count(channel, count);
}

static int std_fd_get_dma_residue(int channel)
{
	return get_dma_residue(channel);
}

static void std_fd_enable_irq(int irq)
{
i8259A_irq_type.enable(irq);
}

static void std_fd_disable_irq(int irq)
{
i8259A_irq_type.disable(irq);
}

static unsigned long std_fd_getfdaddr1(void)
{
return 0x3f0;
}

#define USE_MY_ALLOC
#ifdef USE_MY_ALLOC
static unsigned long fd_dma_base=0x80800000;
#else
static unsigned long realmem;
#endif
static unsigned long std_fd_dma_mem_alloc(unsigned long size)
{
	long mem;
#ifdef USE_MY_ALLOC
	mem=fd_dma_base;
	fd_dma_base +=size;
#else
	realmem=malloc(size+0x10000);
    mem=(realmem+0x10000-1)&~(0x10000-1);
#endif
	return mem;
}

static void std_fd_dma_mem_free(unsigned long addr, unsigned long size)
{
#ifdef USE_MY_ALLOC
	if(fd_dma_base==(addr+size))fd_dma_base=addr;
#else
	free(realmem);
#endif
}

static unsigned long std_fd_drive_type(unsigned long n)
{
	if (n == 0)
		return 4;	/* 3,5", 1.44mb */

	return 0;
}


struct fd_ops std_fd_ops = {
	/*
	 * How to access the floppy controller's ports
	 */
	std_fd_inb,
	std_fd_outb,
	/*
	 * How to access the floppy DMA functions.
	 */
	std_fd_enable_dma,
	std_fd_disable_dma,
	std_fd_request_dma,
	std_fd_free_dma,
	std_fd_clear_dma_ff,
	std_fd_set_dma_mode,
	std_fd_set_dma_addr,
	std_fd_set_dma_count,
	std_fd_get_dma_residue,
	std_fd_enable_irq,
	std_fd_disable_irq,
        std_fd_getfdaddr1,
        std_fd_dma_mem_alloc,
        std_fd_dma_mem_free,
	std_fd_drive_type
};

struct fd_ops *fd_ops=&std_fd_ops;

//---------------------------------------------------------------------
struct fd_softc {
    /* General disk infos */
    struct device sc_dev;
    int d_secsize;
    int sc_flags;
};

char tmp_floppy_area[1024];
int fdmatch( struct device *parent, void *match, void *aux);
void fdattach(struct device *parent, struct device *self, void *aux);

struct cfattach fd_ca = {
        sizeof(struct fd_softc), fdmatch, fdattach,
};

struct cfdriver fd_cd = {
    NULL, "fd", DV_DISK
};
struct request *current_request=0;

void *
tgt_poll_register( int level, int(*func) __P((void *)), void *arg);

void floppy_on(unsigned int nr);
void floppy_off(unsigned int nr);
void floppy_off_timer(int nr);
void floppy_on_timer(int nr);
extern struct cfdriver fd_cd;
int ticks_to_floppy_on(unsigned int nr);

extern int command_done;
void wake_up(void *p)
{
    int s = splhigh ();
	volatile int *ps=p;
	*ps=0;
    (void) splx (s);
}

void sleep_on(void *p)
{
	int s = splhigh ();
	volatile int *ps=p;
		*ps=1;
		do{
		idle ();
		}while(*ps);
	splx(s);
}

void interruptible_sleep_on(void *p)
{
sleep_on(p);
}


#define cli(...) 
#define sti(...)
#include "list.h"

struct timer_list {
	struct list_head list;
    struct timer_list *next;
    struct timer_list *prev;
    unsigned long expires;
    unsigned long data;
    void (*function)(unsigned long);
};

static inline int timer_pending (const struct timer_list * timer)
{
	    return timer->list.next != NULL;
}

void add_timer(struct timer_list *timer)
{
timeout(( void (*) __P((void *)))timer->function,(void *)timer->data,timer->expires-ticks);
}

void del_timer(struct timer_list *timer)
{
untimeout(( void (*) __P((void *)))timer->function,(void *)timer->data);
}
typedef struct list_head task_queue;

struct tq_struct {
	struct list_head list;
    void (*routine)(void *);    /* function to call */
	int sync;
    void *data;         /* argument to function */
};

#define TQ_ACTIVE(q)        (!list_empty(&q))
static inline int queue_task(struct tq_struct *bh_pointer, task_queue *bh_list)
{
    int ret = 0;
    if (!test_and_set_bit(0,&bh_pointer->sync)) {
        list_add_tail(&bh_pointer->list, bh_list);
        ret = 1;
    }
    return ret;
}

void __run_task_queue(task_queue *list)
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

static inline void run_task_queue(task_queue *list)
{
	    if (TQ_ACTIVE(*list))
			        __run_task_queue(list);
}


#define DECLARE_TASK_QUEUE(q)   LIST_HEAD1(q)
DECLARE_TASK_QUEUE(tq_immediate);

void fdstrategy(struct buf *bp);

static struct inode inode_fd0={.i_rdev=0x200};
int
fdopen(
    dev_t dev,
    int flag, int fmt,
    struct proc *p)
{
floppy_open(&inode_fd0);
	return 0;
}

int floppy_init(void);

int
fdread(
    dev_t dev,
    struct uio *uio,
    int flags)
{
int ret;
    ret=physio(fdstrategy, NULL, dev, B_READ, minphys, uio);
return ret;
}

int
fdwrite(
    dev_t dev,
    struct uio *uio,
    int flags)
{
    return (physio(fdstrategy, NULL, dev, B_WRITE, minphys, uio));
}


int
fdclose( dev_t dev,
	int flag, int fmt,
	struct proc *p)
{
floppy_release(&inode_fd0);
	return 0;
}

int
fdmatch(parent, match, aux)
    struct device *parent;
    void *match, *aux;
{
    struct confargs *ca = aux;
    int found = 0;
    if (strcmp(ca->ca_name, fd_cd.cd_name) != 0)
        return (found);
	if(!getenv("nofdc"))found=1;

    return found;
}


void
fdattach(parent, self, aux)
    struct device *parent, *self;
    void *aux;
{
struct fd_softc *fd = (void *)self;
fd->d_secsize=0x200;
fd->sc_flags= FDF_LOADED;
enable_dma(4);
fixup_ide();
init_i8259_irqs();
floppy_init();
}


struct buffer_head lcureent_bh;

static void mywakeup(struct buffer_head *bh,int uptodate)
{
	if (!uptodate) {
	   bh->bp->b_flags |= B_ERROR;
	}
 biodone(bh->bp);
}

void fd_enqueue_request(struct buf *bp)
{
	struct buffer_head *bh=&lcureent_bh;
	bh->b_size=bp->b_bcount;
	bh->b_rsector=bp->b_blkno;
	bh->b_rdev=0x200;
	bh->b_data=bp->b_data;
	bh->b_end_io=mywakeup;
	bh->bp=bp;

	add_request(bp->b_flags&B_READ?READ:WRITE,bh);
}


extern int usage_count;
static void do_fd_request(request_queue_t * q);
void
fdstrategy(struct buf *bp)
{
	struct fd_softc *fd;
	int s;
//???
	fd = fdlookup(DISKUNIT(bp->b_dev));
	if (fd == NULL) {
		bp->b_error = ENXIO;
		goto bad;
	}

	
	/* Valid request?  */
	if (bp->b_blkno < 0 ||
	    (bp->b_bcount % fd->d_secsize) != 0 ||
	    (bp->b_bcount / fd->d_secsize) >= (1 << NBBY)) {
		bp->b_error = EINVAL;
		goto bad;
	}
	
	/* If device invalidated (e.g. media change, door open), error. */
	if ((fd->sc_flags & FDF_LOADED) == 0) {
		bp->b_error = EIO;
		goto bad;
	}

	/* If it's a null transfer, return immediately. */
	if (bp->b_bcount == 0)
		goto done;

	/*
	 * Do bounds checking, adjust transfer. if error, process.
	 * If end of partition, just return.
	 */
	/* Queue transfer on drive, activate drive and controller if idle. */
	s = splbio();

usage_count++;
	fd_enqueue_request(bp);//disksort(&fd->sc_q, bp);
usage_count--;
	splx(s);
	device_unref(&fd->sc_dev);
	return;
bad:
	bp->b_flags |= B_ERROR;
done:
	/* Toss transfer; we're done early. */
	bp->b_resid = bp->b_bcount;
	biodone(bp);
	if (fd != NULL)
		device_unref(&fd->sc_dev);
}
#define DECLARE_WAIT_QUEUE_HEAD(x)  int x
#define set_current_state(...)

static struct tq_struct floppy_tq;


//device 0xa, crf3
void floppy_interrupt(int irq, void *dev_id, void * regs);
#include "fdreg.h"
static void floppy_interrupt_pmon(void)
{

	if(i8259A_irq_pending(FLOPPY_IRQ))
	{
		i8259A_irq_type.ack(FLOPPY_IRQ);
		floppy_interrupt(6,0,0);
		i8259A_irq_type.end(FLOPPY_IRQ);
	}
	run_task_queue(&tq_immediate);
}

int fd_request_irq() 
{
static int requested=0;
	if(!requested)
	{
		tgt_poll_register(IPL_BIO,(int(*) __P((void *)))&floppy_interrupt_pmon,0);
		requested=1;
	}
	i8259A_irq_type.startup(FLOPPY_IRQ);
return 0;
}

void fd_free_irq()	 
{
	i8259A_irq_type.shutdown(FLOPPY_IRQ);
}

//----------------------------------------------------------------------
DECLARE_WAIT_QUEUE_HEAD(mywaitqueue);
static void mywakeup1(struct buffer_head *bh,int update)
{
 wake_up(&mywaitqueue);
}

void read_a_sector(int block,char *buf)
{
static struct buffer_head mybuffer;
struct buffer_head *bh=&mybuffer;
struct inode inode_fd0;
inode_fd0.i_rdev=0x200;
floppy_open(&inode_fd0);
bh->b_size=512;
bh->b_rsector=block;
bh->b_rdev=0x200;
bh->b_data=buf;
bh->b_end_io=mywakeup1;
add_request(READ,bh);
sleep_on(&mywaitqueue);
floppy_release(&inode_fd0);
}

void write_a_sector(int block,char *buf)
{
static struct buffer_head mybuffer;
struct buffer_head *bh=&mybuffer;
struct inode inode_fd0;
inode_fd0.i_rdev=0x200;
floppy_open(&inode_fd0);
bh->b_size=512;
bh->b_rsector=block;
bh->b_rdev=0x200;
bh->b_data=buf;
bh->b_end_io=mywakeup1;
add_request(WRITE,bh);
sleep_on(&mywaitqueue);
floppy_release(&inode_fd0);
}

static char fdbuf[512];
static int readfd(int argc,char **argv)
{
char str[0x100];
int block;
if(argc>1)block=strtol(argv[1],0,0);
else block=0;
memset(fdbuf,0,512);
read_a_sector(block,fdbuf);
sprintf(str,"pcs -1;d1 %p 256",fdbuf);
printf("%s\n",str);
do_cmd(str);
return 0;
}

static int writefd(int argc,char **argv)
{
int block;
if(argc>1)block=strtol(argv[1],0,0);
else block=0;
write_a_sector(block,fdbuf);
return 0;
}

void setup_fdvar()
{
char str[0x100];
do_cmd("set iomap 0");
sprintf(str,"%p",fdbuf);
setenv("faddr",str);
}

#include <pmon.h>
static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"readfd","readfd block",0,"read a block from fd",readfd,0,99,CMD_REPEAT},
	{"writefd","writefd block",0,"write a block to fd",writefd,0,99,CMD_REPEAT},
	{0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
