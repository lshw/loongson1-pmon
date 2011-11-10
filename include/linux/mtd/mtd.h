
/* $Id: mtd.h,v 1.1.1.1 2006/05/08 03:32:49 cpu Exp $ */

#ifndef __MTD_MTD_H__
#define __MTD_MTD_H__


#include <linux/types.h>
#include <linux/mtd/compatmac.h>
#include <linux/module.h>

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
typedef int spinlock_t;
typedef void* wait_queue_head_t;
typedef off_t loff_t;

#ifndef HZ
#define	HZ 100
#endif
extern int ticks;
#define EBADMSG 1
#define EUCLEAN 2
#define ENOTSUPP 3
#define spin_lock(...)
#define spin_unlock(...)
#define wake_up(...)
#define BUG(...)
#define schedule idle
#define jiffies ticks
#define printk printf
#define KERN_INFO 
#define KERN_WARNING
#define KERN_NOTICE
#define KERN_ERR
#define KERN_DEBUG
#define likely(x) (x)
#define unlikely(x) (x)
#define kmalloc(x,...) malloc(x)
#define kfree(x,...) free(x)
#define vmalloc(x,...) malloc(x)
#define vfree(x,...) free(x)
#define spin_lock_init(...)
#define udelay(x) delay(x)
#define init_waitqueue(...)
#define cpu_to_le16(x) (x)
#define time_after(a,b)		((long)(b) - (long)(a) < 0)
#define time_before(a,b)	time_after(b,a)
static inline void *ERR_PTR(long error)
{
	return (void *) error;
}

static inline long PTR_ERR(const void *ptr)
{
	return (long) ptr;
}

static inline long IS_ERR(const void *ptr)
{
	return (unsigned long)ptr > (unsigned long)-1000L;
}
#define led_trigger_event(...)
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
#define min_t(type,x,y) ({ type __x = (x); type __y = (y); __x < __y ? __x: __y; })

struct erase_info_user {
	u_int32_t start;
	u_int32_t length;
};

struct mtd_oob_buf {
	u_int32_t start;
	u_int32_t length;
	unsigned char *ptr;
};


#define MTD_CHAR_MAJOR 90
#define MTD_BLOCK_MAJOR 31
#define MAX_MTD_DEVICES 16



#define MTD_ABSENT		0
#define MTD_RAM			1
#define MTD_ROM			2
#define MTD_NORFLASH		3
#define MTD_NANDFLASH		4
#define MTD_PEROM		5
#define MTD_OTHER		14
#define MTD_UNKNOWN		15



#define MTD_CLEAR_BITS		1       // Bits can be cleared (flash)
#define MTD_SET_BITS		2       // Bits can be set
#define MTD_ERASEABLE		4       // Has an erase function
#define MTD_WRITEB_WRITEABLE	8       // Direct IO is possible
#define MTD_VOLATILE		16      // Set for RAMs
#define MTD_XIP			32	// eXecute-In-Place possible
#define MTD_OOB			64	// Out-of-band data (NAND flash)
#define MTD_ECC			128	// Device capable of automatic ECC

// Some common devices / combinations of capabilities
#define MTD_CAP_ROM		0
#define MTD_CAP_RAM		(MTD_CLEAR_BITS|MTD_SET_BITS|MTD_WRITEB_WRITEABLE)
#define MTD_CAP_NORFLASH        (MTD_CLEAR_BITS|MTD_ERASEABLE)
#define MTD_CAP_NANDFLASH       (MTD_CLEAR_BITS|MTD_ERASEABLE|MTD_OOB)
#define MTD_WRITEABLE		(MTD_CLEAR_BITS|MTD_SET_BITS)


// Types of automatic ECC/Checksum available
#define MTD_ECC_NONE		0 	// No automatic ECC available
#define MTD_ECC_RS_DiskOnChip	1	// Automatic ECC on DiskOnChip
#define MTD_ECC_SW		2	// SW ECC for Toshiba & Samsung devices

struct mtd_info_user {
	u_char type;
	u_int32_t flags;
	u_int32_t size;	 // Total size of the MTD
	u_int32_t erasesize;
	u_int32_t oobblock;  // Size of OOB blocks (e.g. 512)
	u_int32_t oobsize;   // Amount of OOB data per block (e.g. 16)
	u_int32_t ecctype;
	u_int32_t eccsize;
};

struct region_info_user {
	u_int32_t offset;		/* At which this region starts, 
					 * from the beginning of the MTD */
	u_int32_t erasesize;		/* For this region */
	u_int32_t numblocks;		/* Number of blocks in this region */
	u_int32_t regionindex;
};

#define MEMGETINFO              _IOR('M', 1, struct mtd_info_user)
#define MEMERASE                _IOW('M', 2, struct erase_info_user)
#define MEMWRITEOOB             _IOWR('M', 3, struct mtd_oob_buf)
#define MEMREADOOB              _IOWR('M', 4, struct mtd_oob_buf)
#define MEMLOCK                 _IOW('M', 5, struct erase_info_user)
#define MEMUNLOCK               _IOW('M', 6, struct erase_info_user)
#define MEMGETREGIONCOUNT	_IOR('M', 7, int)
#define MEMGETREGIONINFO	_IOWR('M', 8, struct region_info_user)



#define MTD_ERASE_PENDING      	0x01
#define MTD_ERASING		0x02
#define MTD_ERASE_SUSPEND	0x04
#define MTD_ERASE_DONE          0x08
#define MTD_ERASE_FAILED        0x10

struct erase_info {
	struct mtd_info *mtd;
	u_int32_t addr;
	u_int32_t fail_addr;
	u_int32_t len;
	u_long time;
	u_long retries;
	u_int dev;
	u_int cell;
	void (*callback) (struct erase_info *self);
	u_long priv;
	u_char state;
	struct erase_info *next;
};

struct mtd_erase_region_info {
	u_int32_t offset;			/* At which this region starts, from the beginning of the MTD */
	u_int32_t erasesize;		/* For this region */
	u_int32_t numblocks;		/* Number of blocks of erasesize in this region */
};


/**
 * struct mtd_ecc_stats - error correction stats
 *
 * @corrected:	number of corrected bits
 * @failed:	number of uncorrectable errors
 * @badblocks:	number of bad blocks in this partition
 * @bbtblocks:	number of blocks reserved for bad block tables
 */
struct mtd_ecc_stats {
	uint32_t corrected;
	uint32_t failed;
	uint32_t badblocks;
	uint32_t bbtblocks;
};

/*
 * Obsolete legacy interface. Keep it in order not to break userspace
 * interfaces
 */
struct nand_oobinfo {
	uint32_t useecc;
	uint32_t eccbytes;
	uint32_t oobfree[8][2];
	uint32_t eccpos[32];
};

struct nand_oobfree {
	uint32_t offset;
	uint32_t length;
};

/*
 * oob operation modes
 *
 * MTD_OOB_PLACE:	oob data are placed at the given offset
 * MTD_OOB_AUTO:	oob data are automatically placed at the free areas
 *			which are defined by the ecclayout
 * MTD_OOB_RAW:		mode to read raw data+oob in one chunk. The oob data
 *			is inserted into the data. Thats a raw image of the
 *			flash contents.
 */
typedef enum {
	MTD_OOB_PLACE,
	MTD_OOB_AUTO,
	MTD_OOB_RAW,
} mtd_oob_mode_t;

/**
 * struct mtd_oob_ops - oob operation operands
 * @mode:	operation mode
 *
 * @len:	number of bytes to write/read. When a data buffer is given
 *		(datbuf != NULL) this is the number of data bytes. When
 *		no data buffer is available this is the number of oob bytes.
 *
 * @retlen:	number of bytes written/read. When a data buffer is given
 *		(datbuf != NULL) this is the number of data bytes. When
 *		no data buffer is available this is the number of oob bytes.
 *
 * @ooblen:	number of oob bytes per page
 * @ooboffs:	offset of oob data in the oob area (only relevant when
 *		mode = MTD_OOB_PLACE)
 * @datbuf:	data buffer - if NULL only oob data are read/written
 * @oobbuf:	oob data buffer
 */
struct mtd_oob_ops {
	mtd_oob_mode_t	mode;
	size_t		len;
	size_t		retlen;
	size_t		ooblen;
	uint32_t	ooboffs;
	uint8_t		*datbuf;
	uint8_t		*oobbuf;
};


#define MTD_MAX_OOBFREE_ENTRIES	8
/*
 * ECC layout control structure. Exported to userspace for
 * diagnosis and to allow creation of raw images
 */
struct nand_ecclayout {
	uint32_t eccbytes;
	uint32_t eccpos[64];
	uint32_t oobavail;
	struct nand_oobfree oobfree[MTD_MAX_OOBFREE_ENTRIES];
};


struct mtd_info {
	u_char type;
	u_int32_t flags;
	u_int32_t size;	 // Total size of the MTD

	/* "Major" erase size for the device. Naïve users may take this
	 * to be the only erase size available, or may use the more detailed
	 * information below if they desire
	 */
	u_int32_t erasesize;

	u_int32_t oobblock;  // Size of OOB blocks (e.g. 512)
	u_int32_t oobsize;   // Amount of OOB data per block (e.g. 16)
	u_int32_t ecctype;
	u_int32_t eccsize;

	// Kernel-only stuff starts here.
	char *name;
	int index;

	/* Data for variable erase regions. If numeraseregions is zero,
	 * it means that the whole device has erasesize as given above. 
	 */
	int numeraseregions;
	struct mtd_erase_region_info *eraseregions; 

	/* This really shouldn't be here. It can go away in 2.5 */
	u_int32_t bank_size;

	struct module *module;

	/* ecc layout structure pointer - read only ! */
	struct nand_ecclayout *ecclayout;

	int (*erase) (struct mtd_info *mtd, struct erase_info *instr);

	/* This stuff for eXecute-In-Place */
	int (*point) (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char **mtdbuf);

	/* We probably shouldn't allow XIP if the unpoint isn't a NULL */
	void (*unpoint) (struct mtd_info *mtd, u_char * addr);


	int (*read) (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf);
	int (*write) (struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf);

	int (*read_ecc) (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf, u_char *eccbuf);
	int (*write_ecc) (struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf, u_char *eccbuf);

//	int (*read_oob) (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf);
//	int (*write_oob) (struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf);

	int (*read_oob) (struct mtd_info *mtd, loff_t from,
			 struct mtd_oob_ops *ops);
	int (*write_oob) (struct mtd_info *mtd, loff_t to,
			 struct mtd_oob_ops *ops);

	/* iovec-based read/write methods. We need these especially for NAND flash,
	   with its limited number of write cycles per erase.
	   NB: The 'count' parameter is the number of _vectors_, each of 
	   which contains an (ofs, len) tuple.
	*/
	int (*readv) (struct mtd_info *mtd, struct iovec *vecs, unsigned long count, loff_t from, size_t *retlen);
	int (*writev) (struct mtd_info *mtd, const struct iovec *vecs, unsigned long count, loff_t to, size_t *retlen);

	/* Sync */
	void (*sync) (struct mtd_info *mtd);

	/* Chip-supported device locking */
	int (*lock) (struct mtd_info *mtd, loff_t ofs, size_t len);
	int (*unlock) (struct mtd_info *mtd, loff_t ofs, size_t len);

	/* Power Management functions */
	int (*suspend) (struct mtd_info *mtd);
	void (*resume) (struct mtd_info *mtd);

	/* Bad block management functions */
	int (*block_isbad) (struct mtd_info *mtd, loff_t ofs);
	int (*block_markbad) (struct mtd_info *mtd, loff_t ofs);


	/* ECC status information */
	struct mtd_ecc_stats ecc_stats;
	u_int32_t writesize;


	void *priv;
};

static inline int call_new_read_oob (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf)
{

	struct mtd_oob_ops ops;
	int ret;
//ops.len is total bytes we want
//ops.ooblen is min(mtd->oobsize,len)
		ops.len = len;
		ops.ooblen = len;
		ops.ooboffs = from & (mtd->oobsize - 1);
		ops.datbuf = NULL;
		ops.oobbuf = buf;
		ops.mode = MTD_OOB_PLACE;

		if (ops.ooboffs && ops.len > (mtd->oobsize - ops.ooboffs))
			return -EINVAL;

		ret = mtd->read_oob(mtd, from, &ops);
		*retlen=ops.retlen;
		return ret;
}

static inline int call_new_write_oob (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf)
{

	struct mtd_oob_ops ops;
	int ret;
//ops.len is total bytes we want
//ops.ooblen is min(mtd->oobsize,len)
		ops.len = len;
		ops.ooblen = len;
		ops.ooboffs = from & (mtd->oobsize - 1);
		ops.datbuf = NULL;
		ops.oobbuf = buf;
		ops.mode = MTD_OOB_PLACE;

		if (ops.ooboffs && ops.len > (mtd->oobsize - ops.ooboffs))
			return -EINVAL;

		ret = mtd->write_oob(mtd, from, &ops);
		*retlen=ops.retlen;
		return ret;
}

static inline int call_old_read_oob(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops,int (*read_oob) (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf))
{

	return read_oob(mtd,from,ops->len,&ops->retlen,ops->oobbuf);
}

static inline int call_old_write_oob(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops,int (*write_oob) (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf))
{

	return write_oob(mtd,from,ops->len,&ops->retlen,ops->oobbuf);
}

	/* Kernel-side ioctl definitions */

extern int add_mtd_device(struct mtd_info *mtd,int offset,int size,char *name);
extern int del_mtd_device (struct mtd_info *mtd);

extern struct mtd_info *__get_mtd_device(struct mtd_info *mtd, int num);

static inline struct mtd_info *get_mtd_device(struct mtd_info *mtd, int num)
{
	struct mtd_info *ret;
	
	ret = __get_mtd_device(mtd, num);

	if (ret)
		return NULL;

	return ret;
}

static inline void put_mtd_device(struct mtd_info *mtd)
{
}


struct mtd_notifier {
	void (*add)(struct mtd_info *mtd);
	void (*remove)(struct mtd_info *mtd);
	struct mtd_notifier *next;
};


extern void register_mtd_user (struct mtd_notifier *new);
extern int unregister_mtd_user (struct mtd_notifier *old);


#ifndef MTDC
#define MTD_ERASE(mtd, args...) (*(mtd->erase))(mtd, args)
#define MTD_POINT(mtd, a,b,c,d) (*(mtd->point))(mtd, a,b,c, (u_char **)(d))
#define MTD_UNPOINT(mtd, arg) (*(mtd->unpoint))(mtd, (u_char *)arg)
#define MTD_READ(mtd, args...) (*(mtd->read))(mtd, args)
#define MTD_WRITE(mtd, args...) (*(mtd->write))(mtd, args)
#define MTD_READV(mtd, args...) (*(mtd->readv))(mtd, args)
#define MTD_WRITEV(mtd, args...) (*(mtd->writev))(mtd, args)
#define MTD_READECC(mtd, args...) (*(mtd->read_ecc))(mtd, args)
#define MTD_WRITEECC(mtd, args...) (*(mtd->write_ecc))(mtd, args)
#define MTD_READOOB(mtd, args...) call_new_read_oob(mtd, args)
#define MTD_WRITEOOB(mtd, args...) call_new_write_oob(mtd, args)
#define MTD_SYNC(mtd) do { if (mtd->sync) (*(mtd->sync))(mtd);  } while (0) 
#endif /* MTDC */

/*
 * Debugging macro and defines
 */
#define MTD_DEBUG_LEVEL0	(0)	/* Quiet   */
#define MTD_DEBUG_LEVEL1	(1)	/* Audible */
#define MTD_DEBUG_LEVEL2	(2)	/* Loud    */
#define MTD_DEBUG_LEVEL3	(3)	/* Noisy   */
#define CONFIG_MTD_DEBUG_VERBOSE 0
#define CONFIG_MTD_DEBUG

#ifdef CONFIG_MTD_DEBUG
#define DEBUG(n, args...)			\
	if (n <=  CONFIG_MTD_DEBUG_VERBOSE) {	\
		printk(KERN_INFO args);	\
	}
#else /* CONFIG_MTD_DEBUG */
#define DEBUG(n, args...)
#endif /* CONFIG_MTD_DEBUG */


#endif /* __MTD_MTD_H__ */
