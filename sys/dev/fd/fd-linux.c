
#define CURRENT current_request
#define LOCAL_END_REQUEST
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/tqueue.h>
#include <linux/fdreg.h>

/*
 * 1998/1/21 -- Richard Gooch <rgooch@atnf.csiro.au> -- devfs support
 */


#include <linux/fd.h>
#include <linux/hdreg.h>

#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/delay.h>
#include <linux/mc146818rtc.h> /* CMOS defines */
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/devfs_fs_kernel.h>

/*
 * PS/2 floppies have much slower step rates than regular floppies.
 * It's been recommended that take about 1/4 of the default speed
 * in some more extreme cases.
 */

#include <asm/dma.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/floppy.h>

#include <linux/blk.h>
#include <linux/blkpg.h>
#include <linux/cdrom.h> /* for the compatibility eject ioctl */

#define ticks jiffies

DECLARE_WAIT_QUEUE_HEAD(mywaitqueue);
static void mywakeup(struct buffer_head *bh,int update)
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
bh->b_end_io=mywakeup;
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
bh->b_end_io=mywakeup;
add_request(WRITE,bh);
sleep_on(&mywaitqueue);
floppy_release(&inode_fd0);
}

