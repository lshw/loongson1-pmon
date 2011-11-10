/* do print messages for unexpected interrupts */
static int print_unex=1;

static int slow_floppy;
#define FLOPPY_MAJOR 2
//--------------------------------------------

//#define printk printf
struct request;
struct buffer_head;
struct request *current_request;
#undef QUEUE_EMPTY
#define QUEUE_EMPTY (!CURRENT)

//---------------------------------------------
#undef FLOPPY_SANITY_CHECK
#undef  FLOPPY_SILENT_DCL_CLEAR

//#define DEBUGT 2
#undef DEBUGT
#undef  DCL_DEBUG /* debug disk change line */



/*
 * 1998/1/21 -- Richard Gooch <rgooch@atnf.csiro.au> -- devfs support
 */

/*
 * PS/2 floppies have much slower step rates than regular floppies.
 * It's been recommended that take about 1/4 of the default speed
 * in some more extreme cases.
 */
static int slow_floppy;

static int FLOPPY_IRQ=6;
static int FLOPPY_DMA=2;
static int can_use_virtual_dma=2;
/* =======
 * can use virtual DMA:
 * 0 = use of virtual DMA disallowed by config
 * 1 = use of virtual DMA prescribed by config
 * 2 = no virtual DMA preference configured.  By default try hard DMA,
 * but fall back on virtual DMA when not enough memory available
 */

static int use_virtual_dma;
/* =======
 * use virtual DMA
 * 0 using hard DMA
 * 1 using virtual DMA
 * This variable is set to virtual when a DMA mem problem arises, and
 * reset back in floppy_grab_irq_and_dma.
 * It is not safe to reset it in other circumstances, because the floppy
 * driver may have several buffers in use at once, and we do currently not
 * record each buffers capabilities
 */

static unsigned short virtual_dma_port=0x3f0;
void floppy_interrupt(int irq, void *dev_id, void * regs);
static int set_dor(int fdc, char mask, char data);

#define K_64	0x10000		/* 64KB */

/* the following is the mask of allowed drives. By default units 2 and
 * 3 of both floppy controllers are disabled, because switching on the
 * motor of these drives causes system hangs on some PCI computers. drive
 * 0 is the low bit (0x1), and drive 7 is the high bit (0x80). Bits are on if
 * a drive is allowed.
 *
 * NOTE: This must come before we include the arch floppy header because
 *       some ports reference this variable from there. -DaveM
 */



static int irqdma_allocated;

#define MAJOR_NR FLOPPY_MAJOR
#ifdef PMON
#include "fd-pmon.c"
#else
#include "fd-linux.c"
#endif

static int allowed_drive_mask = 0x33;

static int end_request(int uptodate)
{
	struct buffer_head * bh;
	int nsect;

	DEVICE_OFF(CURRENT->rq_dev);
	if (!uptodate)
		printk("end_request: I/O error, dev %s , sector %lu\n",
			"fd0", CURRENT->sector);

	if ((bh = CURRENT->bh) != NULL) {
		bh->b_end_io(bh, uptodate);
	}
	CURRENT=0;
	return 0;
}

#ifndef fd_get_dma_residue
#define fd_get_dma_residue() get_dma_residue(FLOPPY_DMA)
#endif

/* Dma Memory related stuff */

#ifndef fd_dma_mem_free
#define fd_dma_mem_free(addr, size) free_pages(addr, get_order(size))
#endif

#ifndef fd_dma_mem_alloc
#define fd_dma_mem_alloc(size) __get_dma_pages(GFP_KERNEL,get_order(size))
#endif

static inline void fallback_on_nodma_alloc(char **addr, size_t l)
{
#ifdef FLOPPY_CAN_FALLBACK_ON_NODMA
	if (*addr)
		return; /* we have the memory */
	if (can_use_virtual_dma != 2)
		return; /* no fallback allowed */
	printk("DMA memory shortage. Temporarily falling back on virtual DMA\n");
	*addr = (char *) nodma_mem_alloc(l);
#else
	return;
#endif
}

/* End dma memory related stuff */

static unsigned long fake_change;
static int initialising=1;
static inline int TYPE(kdev_t x) {
	return  (MINOR(x)>>2) & 0x1f;
}
static inline int DRIVE(kdev_t x) {
	return (MINOR(x)&0x03) | ((MINOR(x)&0x80) >> 5);
}
#define ITYPE(x) (((x)>>2) & 0x1f)
#define TOMINOR(x) ((x & 3) | ((x & 4) << 5))
#define UNIT(x) ((x) & 0x03)		/* drive on fdc */
#define FDC(x) (((x) & 0x04) >> 2)  /* fdc of drive */
#define REVDRIVE(fdc, unit) ((unit) + ((fdc) << 2))
				/* reverse mapping from unit and fdc to drive */
#define DP (&drive_params[current_drive])
#define DRS (&drive_state[current_drive])
#define DRWE (&write_errors[current_drive])
#define FDCS (&fdc_state[fdc])
#define CLEARF(x) (clear_bit(x##_BIT, &DRS->flags))
#define SETF(x) (set_bit(x##_BIT, &DRS->flags))
#define TESTF(x) (test_bit(x##_BIT, &DRS->flags))

#define UDP (&drive_params[drive])
#define UDRS (&drive_state[drive])
#define UDRWE (&write_errors[drive])
#define UFDCS (&fdc_state[FDC(drive)])
#define UCLEARF(x) (clear_bit(x##_BIT, &UDRS->flags))
#define USETF(x) (set_bit(x##_BIT, &UDRS->flags))
#define UTESTF(x) (test_bit(x##_BIT, &UDRS->flags))

#define DPRINT(format, args...) printk(DEVICE_NAME "%d: " format, current_drive , ## args)

#define PH_HEAD(floppy,head) (((((floppy)->stretch & 2) >>1) ^ head) << 2)
#define STRETCH(floppy) ((floppy)->stretch & FD_STRETCH)

#define CLEARSTRUCT(x) memset((x), 0, sizeof(*(x)))

/* read/write */
#define COMMAND raw_cmd->cmd[0]
#define DR_SELECT raw_cmd->cmd[1]
#define TRACK raw_cmd->cmd[2]
#define HEAD raw_cmd->cmd[3]
#define SECTOR raw_cmd->cmd[4]
#define SIZECODE raw_cmd->cmd[5]
#define SECT_PER_TRACK raw_cmd->cmd[6]
#define GAP raw_cmd->cmd[7]
#define SIZECODE2 raw_cmd->cmd[8]
#define NR_RW 9

/* format */
#define F_SIZECODE raw_cmd->cmd[2]
#define F_SECT_PER_TRACK raw_cmd->cmd[3]
#define F_GAP raw_cmd->cmd[4]
#define F_FILL raw_cmd->cmd[5]
#define NR_F 6

/*
 * Maximum disk size (in kilobytes). This default is used whenever the
 * current disk size is unknown.
 * [Now it is rather a minimum]
 */
#define MAX_DISK_SIZE 4 /* 3984*/


/*
 * globals used by 'result()'
 */
#define MAX_REPLIES 16
static unsigned char reply_buffer[MAX_REPLIES];
static int inr; /* size of reply buffer, when called from interrupt */
#define ST0 (reply_buffer[0])
#define ST1 (reply_buffer[1])
#define ST2 (reply_buffer[2])
#define ST3 (reply_buffer[0]) /* result of GETSTATUS */
#define R_TRACK (reply_buffer[3])
#define R_HEAD (reply_buffer[4])
#define R_SECTOR (reply_buffer[5])
#define R_SIZECODE (reply_buffer[6])

#define SEL_DLY (2*HZ/100)

/*
 * this struct defines the different floppy drive types.
 */
static struct {
	struct floppy_drive_params params;
	const char *name; /* name printed while booting */
} default_drive_params[]= {
/* NOTE: the time values in ticks should be in msec!
 CMOS drive type
  |     Maximum data rate supported by drive type
  |     |   Head load time, msec
  |     |   |   Head unload time, msec (not used)
  |     |   |   |     Step rate interval, usec
  |     |   |   |     |       Time needed for spinup time (ticks)
  |     |   |   |     |       |      Timeout for spinning down (ticks)
  |     |   |   |     |       |      |   Spindown offset (where disk stops)
  |     |   |   |     |       |      |   |     Select delay
  |     |   |   |     |       |      |   |     |     RPS
  |     |   |   |     |       |      |   |     |     |    Max number of tracks
  |     |   |   |     |       |      |   |     |     |    |     Interrupt timeout
  |     |   |   |     |       |      |   |     |     |    |     |   Max nonintlv. sectors
  |     |   |   |     |       |      |   |     |     |    |     |   | -Max Errors- flags */
{{0,  500, 16, 16, 8000,    1*HZ, 3*HZ,  0, SEL_DLY, 5,  80, 3*HZ, 20, {3,1,2,0,2}, 0,
      0, { 7, 4, 8, 2, 1, 5, 3,10}, 3*HZ/2, 0 }, "unknown" },

{{1,  300, 16, 16, 8000,    1*HZ, 3*HZ,  0, SEL_DLY, 5,  40, 3*HZ, 17, {3,1,2,0,2}, 0,
      0, { 1, 0, 0, 0, 0, 0, 0, 0}, 3*HZ/2, 1 }, "360K PC" }, /*5 1/4 360 KB PC*/

{{2,  500, 16, 16, 6000, 4*HZ/10, 3*HZ, 14, SEL_DLY, 6,  83, 3*HZ, 17, {3,1,2,0,2}, 0,
      0, { 2, 5, 6,23,10,20,12, 0}, 3*HZ/2, 2 }, "1.2M" }, /*5 1/4 HD AT*/

{{3,  250, 16, 16, 3000,    1*HZ, 3*HZ,  0, SEL_DLY, 5,  83, 3*HZ, 20, {3,1,2,0,2}, 0,
      0, { 4,22,21,30, 3, 0, 0, 0}, 3*HZ/2, 4 }, "720k" }, /*3 1/2 DD*/

{{4,  500, 16, 16, 4000, 4*HZ/10, 3*HZ, 10, SEL_DLY, 5,  83, 3*HZ, 20, {3,1,2,0,2}, 0,
      0, { 7, 4,25,22,31,21,29,11}, 3*HZ/2, 7 }, "1.44M" }, /*3 1/2 HD*/

{{5, 1000, 15,  8, 3000, 4*HZ/10, 3*HZ, 10, SEL_DLY, 5,  83, 3*HZ, 40, {3,1,2,0,2}, 0,
      0, { 7, 8, 4,25,28,22,31,21}, 3*HZ/2, 8 }, "2.88M AMI BIOS" }, /*3 1/2 ED*/

{{6, 1000, 15,  8, 3000, 4*HZ/10, 3*HZ, 10, SEL_DLY, 5,  83, 3*HZ, 40, {3,1,2,0,2}, 0,
      0, { 7, 8, 4,25,28,22,31,21}, 3*HZ/2, 8 }, "2.88M" } /*3 1/2 ED*/
/*    |  --autodetected formats---    |      |      |
 *    read_track                      |      |    Name printed when booting
 *				      |     Native format
 *	            Frequency of disk change checks */
};

static struct floppy_drive_params drive_params[N_DRIVE];
static struct floppy_drive_struct drive_state[N_DRIVE];
static struct floppy_write_errors write_errors[N_DRIVE];
static struct timer_list motor_off_timer[N_DRIVE];
static struct floppy_raw_cmd *raw_cmd, default_raw_cmd;

/*
 * This struct defines the different floppy types.
 *
 * Bit 0 of 'stretch' tells if the tracks need to be doubled for some
 * types (e.g. 360kB diskette in 1.2MB drive, etc.).  Bit 1 of 'stretch'
 * tells if the disk is in Commodore 1581 format, which means side 0 sectors
 * are located on side 1 of the disk but with a side 0 ID, and vice-versa.
 * This is the same as the Sharp MZ-80 5.25" CP/M disk format, except that the
 * 1581's logical side 0 is on physical side 1, whereas the Sharp's logical
 * side 0 is on physical side 0 (but with the misnamed sector IDs).
 * 'stretch' should probably be renamed to something more general, like
 * 'options'.  Other parameters should be self-explanatory (see also
 * setfdprm(8)).
 */
/*
	    Size
	     |  Sectors per track
	     |  | Head
	     |  | |  Tracks
	     |  | |  | Stretch
	     |  | |  | |  Gap 1 size
	     |  | |  | |    |  Data rate, | 0x40 for perp
	     |  | |  | |    |    |  Spec1 (stepping rate, head unload
	     |  | |  | |    |    |    |    /fmt gap (gap2) */
static struct floppy_struct floppy_type[32] = {
	{    0, 0,0, 0,0,0x00,0x00,0x00,0x00,NULL    },	/*  0 no testing    */
	{  720, 9,2,40,0,0x2A,0x02,0xDF,0x50,"d360"  }, /*  1 360KB PC      */
	{ 2400,15,2,80,0,0x1B,0x00,0xDF,0x54,"h1200" },	/*  2 1.2MB AT      */
	{  720, 9,1,80,0,0x2A,0x02,0xDF,0x50,"D360"  },	/*  3 360KB SS 3.5" */
	{ 1440, 9,2,80,0,0x2A,0x02,0xDF,0x50,"D720"  },	/*  4 720KB 3.5"    */
	{  720, 9,2,40,1,0x23,0x01,0xDF,0x50,"h360"  },	/*  5 360KB AT      */
	{ 1440, 9,2,80,0,0x23,0x01,0xDF,0x50,"h720"  },	/*  6 720KB AT      */
	{ 2880,18,2,80,0,0x1B,0x00,0xCF,0x6C,"H1440" },	/*  7 1.44MB 3.5"   */
	{ 5760,36,2,80,0,0x1B,0x43,0xAF,0x54,"E2880" },	/*  8 2.88MB 3.5"   */
	{ 6240,39,2,80,0,0x1B,0x43,0xAF,0x28,"E3120" },	/*  9 3.12MB 3.5"   */

	{ 2880,18,2,80,0,0x25,0x00,0xDF,0x02,"h1440" }, /* 10 1.44MB 5.25"  */
	{ 3360,21,2,80,0,0x1C,0x00,0xCF,0x0C,"H1680" }, /* 11 1.68MB 3.5"   */
	{  820,10,2,41,1,0x25,0x01,0xDF,0x2E,"h410"  },	/* 12 410KB 5.25"   */
	{ 1640,10,2,82,0,0x25,0x02,0xDF,0x2E,"H820"  },	/* 13 820KB 3.5"    */
	{ 2952,18,2,82,0,0x25,0x00,0xDF,0x02,"h1476" },	/* 14 1.48MB 5.25"  */
	{ 3444,21,2,82,0,0x25,0x00,0xDF,0x0C,"H1722" },	/* 15 1.72MB 3.5"   */
	{  840,10,2,42,1,0x25,0x01,0xDF,0x2E,"h420"  },	/* 16 420KB 5.25"   */
	{ 1660,10,2,83,0,0x25,0x02,0xDF,0x2E,"H830"  },	/* 17 830KB 3.5"    */
	{ 2988,18,2,83,0,0x25,0x00,0xDF,0x02,"h1494" },	/* 18 1.49MB 5.25"  */
	{ 3486,21,2,83,0,0x25,0x00,0xDF,0x0C,"H1743" }, /* 19 1.74 MB 3.5"  */

	{ 1760,11,2,80,0,0x1C,0x09,0xCF,0x00,"h880"  }, /* 20 880KB 5.25"   */
	{ 2080,13,2,80,0,0x1C,0x01,0xCF,0x00,"D1040" }, /* 21 1.04MB 3.5"   */
	{ 2240,14,2,80,0,0x1C,0x19,0xCF,0x00,"D1120" }, /* 22 1.12MB 3.5"   */
	{ 3200,20,2,80,0,0x1C,0x20,0xCF,0x2C,"h1600" }, /* 23 1.6MB 5.25"   */
	{ 3520,22,2,80,0,0x1C,0x08,0xCF,0x2e,"H1760" }, /* 24 1.76MB 3.5"   */
	{ 3840,24,2,80,0,0x1C,0x20,0xCF,0x00,"H1920" }, /* 25 1.92MB 3.5"   */
	{ 6400,40,2,80,0,0x25,0x5B,0xCF,0x00,"E3200" }, /* 26 3.20MB 3.5"   */
	{ 7040,44,2,80,0,0x25,0x5B,0xCF,0x00,"E3520" }, /* 27 3.52MB 3.5"   */
	{ 7680,48,2,80,0,0x25,0x63,0xCF,0x00,"E3840" }, /* 28 3.84MB 3.5"   */

	{ 3680,23,2,80,0,0x1C,0x10,0xCF,0x00,"H1840" }, /* 29 1.84MB 3.5"   */
	{ 1600,10,2,80,0,0x25,0x02,0xDF,0x2E,"D800"  },	/* 30 800KB 3.5"    */
	{ 3200,20,2,80,0,0x1C,0x00,0xCF,0x2C,"H1600" }, /* 31 1.6MB 3.5"    */
};

#define	NUMBER(x)	(sizeof(x) / sizeof(*(x)))
#define SECTSIZE (_FD_SECTSIZE(*floppy))

/* Auto-detection: Disk type used until the next media change occurs. */
static struct floppy_struct *current_type[N_DRIVE];

/*
 * User-provided type information. current_type points to
 * the respective entry of this array.
 */

static int floppy_sizes[256];
static int floppy_blocksizes[256];

/*
 * The driver is trying to determine the correct media format
 * while probing is set. rw_interrupt() clears it after a
 * successful access.
 */
static int probing;

/* Synchronization of FDC access. */
#define FD_COMMAND_NONE -1
#define FD_COMMAND_ERROR 2
#define FD_COMMAND_OKAY 3

static volatile int command_status = FD_COMMAND_NONE;
static unsigned long fdc_busy;
static DECLARE_WAIT_QUEUE_HEAD(fdc_wait);
static DECLARE_WAIT_QUEUE_HEAD(command_done);

#define NO_SIGNAL 1
#define CALL(x) if ((x) == -EINTR) return -EINTR
#define ECALL(x) if ((ret = (x))) return ret;
#define _WAIT(x,i) CALL(ret=wait_til_done((x),i))
#define WAIT(x) _WAIT((x),interruptible)
#define IWAIT(x) _WAIT((x),1)


/*
 * Rate is 0 for 500kb/s, 1 for 300kbps, 2 for 250kbps
 * Spec1 is 0xSH, where S is stepping rate (F=1ms, E=2ms, D=3ms etc),
 * H is head unload time (1=16ms, 2=32ms, etc)
 */

/*
 * Track buffer
 * Because these are written to by the DMA controller, they must
 * not contain a 64k byte boundary crossing, or data will be
 * corrupted/lost.
 */
static char *floppy_track_buffer;
static int max_buffer_sectors;

static int *errors;
typedef void (*done_f)(int);
static struct cont_t {
	void (*interrupt)(void); /* this is called after the interrupt of the
				  * main command */
	void (*redo)(void); /* this is called to retry the operation */
	void (*error)(void); /* this is called to tally an error */
	done_f done; /* this is called to say if the operation has 
		      * succeeded/failed */
} *cont;

static void floppy_ready(void);
static void floppy_start(void);
static void process_fd_request(void);
static void recalibrate_floppy(void);
static void floppy_shutdown(void);

static int floppy_grab_irq_and_dma(void);
static void floppy_release_irq_and_dma(void);

/*
 * The "reset" variable should be tested whenever an interrupt is scheduled,
 * after the commands have been sent. This is to ensure that the driver doesn't
 * get wedged when the interrupt doesn't come because of a failed command.
 * reset doesn't need to be tested before sending commands, because
 * output_byte is automatically disabled when reset is set.
 */
#define CHECK_RESET { if (FDCS->reset){ reset_fdc(); return; } }
static void reset_fdc(void);

/*
 * These are global variables, as that's the easiest way to give
 * information to interrupts. They are the data used for the current
 * request.
 */
#define NO_TRACK -1
#define NEED_1_RECAL -2
#define NEED_2_RECAL -3

static int usage_count;

/* buffer related variables */
static int buffer_track = -1;
static int buffer_drive = -1;
static int buffer_min = -1;
static int buffer_max = -1;

/* fdc related variables, should end up in a struct */
static struct floppy_fdc_state fdc_state[N_FDC];
static int fdc; /* current fdc */

static struct floppy_struct *_floppy = floppy_type;
static unsigned char current_drive;
static long current_count_sectors;
static unsigned char sector_t; /* sector in track */
static unsigned char in_sector_offset;	/* offset within physical sector,
					 * expressed in units of 512 bytes */

#ifndef fd_eject
#define fd_eject(x) -EINVAL
#endif

#ifdef DEBUGT
static long unsigned debugtimer;
#endif

/*
 * Debugging
 * =========
 */
static inline void set_debugt(void)
{
#ifdef DEBUGT
	debugtimer = ticks;
#endif
}

static inline void debugt(const char *message)
{
#ifdef DEBUGT
	if (DP->flags & DEBUGT)
		printk("%s dtime=%lu\n", message, ticks-debugtimer);
#endif
}

typedef void (*timeout_fn)(unsigned long);
static struct timer_list fd_timeout ={ function: (timeout_fn) floppy_shutdown };

static const char *timeout_message;


#if 1 //def FLOPPY_SANITY_CHECK

static void is_alive(const char *message)
{
	/* this routine checks whether the floppy driver is "alive" */
	if (fdc_busy && command_status < 2 && !timer_pending(&fd_timeout)){
		DPRINT("timeout handler died: %s\n",message);
	}
}

#define OLOGSIZE 20

static void (*lasthandler)(void);
static unsigned long interruptticks;
static unsigned long resultticks;
static int resultsize;
static unsigned long lastredo;

static struct output_log {
	unsigned char data;
	unsigned char status;
	unsigned long ticks;
} output_log[OLOGSIZE];

static int output_log_pos;
#endif

#define CURRENTD -1
#define MAXTIMEOUT -2

static void reschedule_timeout(int drive, const char *message, int marg)
{
	if (drive == CURRENTD)
		drive = current_drive;
	del_timer(&fd_timeout);
	if (drive < 0 || drive > N_DRIVE) {
		fd_timeout.expires = ticks + 20UL*HZ;
		drive=0;
	} else
		fd_timeout.expires = ticks + UDP->timeout;
	add_timer(&fd_timeout);
	if (UDP->flags & FD_DEBUG){
		DPRINT("reschedule timeout ");
		printk(message, marg);
		printk("\n");
	}
	timeout_message = message;
}

static int maximum(int a, int b)
{
	if (a > b)
		return a;
	else
		return b;
}
#define INFBOUND(a,b) (a)=maximum((a),(b));

static int minimum(int a, int b)
{
	if (a < b)
		return a;
	else
		return b;
}
#define SUPBOUND(a,b) (a)=minimum((a),(b));


/*
 * Bottom half floppy driver.
 * ==========================
 *
 * This part of the file contains the code talking directly to the hardware,
 * and also the main service loop (seek-configure-spinup-command)
 */

/*
 * disk change.
 * This routine is responsible for maintaining the FD_DISK_CHANGE flag,
 * and the last_checked date.
 *
 * last_checked is the date of the last check which showed 'no disk change'
 * FD_DISK_CHANGE is set under two conditions:
 * 1. The floppy has been changed after some i/o to that floppy already
 *    took place.
 * 2. No floppy disk is in the drive. This is done in order to ensure that
 *    requests are quickly flushed in case there is no disk in the drive. It
 *    follows that FD_DISK_CHANGE can only be cleared if there is a disk in
 *    the drive.
 *
 * For 1., maxblock is observed. Maxblock is 0 if no i/o has taken place yet.
 * For 2., FD_DISK_NEWCHANGE is watched. FD_DISK_NEWCHANGE is cleared on
 *  each seek. If a disk is present, the disk change line should also be
 *  cleared on each seek. Thus, if FD_DISK_NEWCHANGE is clear, but the disk
 *  change line is set, this means either that no disk is in the drive, or
 *  that it has been removed since the last seek.
 *
 * This means that we really have a third possibility too:
 *  The floppy has been changed after the last seek.
 */

static int disk_change(int drive)
{
	int fdc=FDC(drive);
#ifdef FLOPPY_SANITY_CHECK
	if (ticks - UDRS->select_date < UDP->select_delay)
		DPRINT("WARNING disk change called early\n");
	if (!(FDCS->dor & (0x10 << UNIT(drive))) ||
	    (FDCS->dor & 3) != UNIT(drive) ||
	    fdc != FDC(drive)){
		DPRINT("probing disk change on unselected drive\n");
		DPRINT("drive=%d fdc=%d dor=%x\n",drive, FDC(drive),
			(unsigned int)FDCS->dor);
	}
#endif

#ifdef DCL_DEBUG
	if (UDP->flags & FD_DEBUG){
		DPRINT("checking disk change line for drive %d\n",drive);
		DPRINT("ticks=%lu\n", ticks);
		DPRINT("disk change line=%x\n",fd_inb(FD_DIR)&0x80);
		DPRINT("flags=%lx\n",UDRS->flags);
	}
#endif
	if (UDP->flags & FD_BROKEN_DCL)
		return UTESTF(FD_DISK_CHANGED);
	if ((fd_inb(FD_DIR) ^ UDP->flags) & 0x80){
		USETF(FD_VERIFY); /* verify write protection */
		if (UDRS->maxblock){
			/* mark it changed */
			USETF(FD_DISK_CHANGED);
		}

		/* invalidate its geometry */
		if (UDRS->keep_data >= 0) {
			if ((UDP->flags & FTD_MSG) &&
			    current_type[drive] != NULL)
				DPRINT("Disk type is undefined after "
				       "disk change\n");
			current_type[drive] = NULL;
			floppy_sizes[TOMINOR(drive)] = MAX_DISK_SIZE;
		}

		/*USETF(FD_DISK_NEWCHANGE);*/
		return 1;
	} else {
		UDRS->last_checked=ticks;
		UCLEARF(FD_DISK_NEWCHANGE);
	}
	return 0;
}

static inline int is_selected(int dor, int unit)
{
	return ((dor  & (0x10 << unit)) && (dor &3) == unit);
}

static int set_dor(int fdc, char mask, char data)
{
	register unsigned char drive, unit, newdor,olddor;

	if (FDCS->address == -1)
		return -1;

	olddor = FDCS->dor;
	newdor =  (olddor & mask) | data;
	if (newdor != olddor){
		unit = olddor & 0x3;
		if (is_selected(olddor, unit) && !is_selected(newdor,unit)){
			drive = REVDRIVE(fdc,unit);
#ifdef DCL_DEBUG
			if (UDP->flags & FD_DEBUG){
				DPRINT("calling disk change from set_dor\n");
			}
#endif
			disk_change(drive);
		}
		FDCS->dor = newdor;
		fd_outb(newdor, FD_DOR);

		unit = newdor & 0x3;
		if (!is_selected(olddor, unit) && is_selected(newdor,unit)){
			drive = REVDRIVE(fdc,unit);
			UDRS->select_date = ticks;
		}
	}
	/*
	 *	We should propogate failures to grab the resources back
	 *	nicely from here. Actually we ought to rewrite the fd
	 *	driver some day too.
	 */
	if (newdor & FLOPPY_MOTOR_MASK)
		floppy_grab_irq_and_dma();
#if 1
	if (olddor & FLOPPY_MOTOR_MASK)
		floppy_release_irq_and_dma();
#endif
	return olddor;
}

static void twaddle(void)
{
	if (DP->select_delay)
		return;
	fd_outb(FDCS->dor & ~(0x10<<UNIT(current_drive)), FD_DOR);
	fd_outb(FDCS->dor, FD_DOR);
	DRS->select_date = ticks;
}

/* reset all driver information about the current fdc. This is needed after
 * a reset, and after a raw command. */
static void reset_fdc_info(int mode)
{
	int drive;

	FDCS->spec1 = FDCS->spec2 = -1;
	FDCS->need_configure = 1;
	FDCS->perp_mode = 1;
	FDCS->rawcmd = 0;
	for (drive = 0; drive < N_DRIVE; drive++)
		if (FDC(drive) == fdc &&
		    (mode || UDRS->track != NEED_1_RECAL))
			UDRS->track = NEED_2_RECAL;
}


/* selects the fdc and drive, and enables the fdc's input/dma. */
static void set_fdc(int drive)
{
	if (drive >= 0 && drive < N_DRIVE){
		fdc = FDC(drive);
		current_drive = drive;
	}
	if (fdc != 1 && fdc != 0) {
		printk("bad fdc value\n");
		return;
	}
	set_dor(fdc,~0,8);
#if N_FDC > 1
	set_dor(1-fdc, ~8, 0);
#endif
	if (FDCS->rawcmd == 2)
		reset_fdc_info(1);
	if (fd_inb(FD_STATUS) != STATUS_READY)
		FDCS->reset = 1;
}


#define LOCK_FDC(drive,interruptible) \
if (lock_fdc(drive,interruptible)) return -EINTR;


/* switches the motor off after a given timeout */
void motor_off_callback(unsigned long nr)
{
	unsigned char mask = ~(0x10 << UNIT(nr));
	set_dor(FDC(nr), mask, 0);
}

/* schedules motor off */
static void floppy_off(unsigned int drive)
{
	unsigned long volatile delta;
	register int fdc=FDC(drive);

	if (!(FDCS->dor & (0x10 << UNIT(drive))))
		return;

	del_timer(motor_off_timer+drive);

	/* make spindle stop in a position which minimizes spinup time
	 * next time */
	if (UDP->rps){
		delta = ticks - UDRS->first_read_date + HZ -
			UDP->spindown_offset;
		delta = ((delta * UDP->rps) % HZ) / UDP->rps;
		motor_off_timer[drive].expires = ticks + UDP->spindown - delta;
	}
	add_timer(motor_off_timer+drive);
}

/*
 * cycle through all N_DRIVE floppy drives, for disk change testing.
 * stopping at current drive. This is done before any long operation, to
 * be sure to have up to date disk change information.
 */
static void scandrives(void)
{
	int i, drive, saved_drive;

	if (DP->select_delay)
		return;

	saved_drive = current_drive;
	for (i=0; i < N_DRIVE; i++){
		drive = (saved_drive + i + 1) % N_DRIVE;
		if (UDRS->fd_ref == 0 || UDP->select_delay != 0)
			continue; /* skip closed drives */
		set_fdc(drive);
		if (!(set_dor(fdc, ~3, UNIT(drive) | (0x10 << UNIT(drive))) &
		      (0x10 << UNIT(drive))))
			/* switch the motor off again, if it was off to
			 * begin with */
			set_dor(fdc, ~(0x10 << UNIT(drive)), 0);
	}
	set_fdc(saved_drive);
}

static void empty(void)
{
}

#if 1
static struct tq_struct floppy_tq;

static void schedule_bh( void (*handler)(void*) )
{
	floppy_tq.routine = (void *)(void *) handler;
	queue_task(&floppy_tq, &tq_immediate);
	mark_bh(IMMEDIATE_BH);
}
#endif

static struct timer_list fd_timer;

static void cancel_activity(void)
{
	CLEAR_INTR;
	floppy_tq.routine = (void *)(void *) empty;
	del_timer(&fd_timer);
}

/* this function makes sure that the disk stays in the drive during the
 * transfer */
static void fd_watchdog(void)
{
#ifdef DCL_DEBUG
	if (DP->flags & FD_DEBUG){
		DPRINT("calling disk change from watchdog\n");
	}
#endif

	if (disk_change(current_drive)){
		DPRINT("disk removed during i/o\n");
		cancel_activity();
		cont->done(0);
		reset_fdc();
	} else {
		del_timer(&fd_timer);
		fd_timer.function = (timeout_fn) fd_watchdog;
		fd_timer.expires = ticks + HZ / 10;
		add_timer(&fd_timer);
	}
}

static void main_command_interrupt(void)
{
	del_timer(&fd_timer);
	cont->interrupt();
}

/* waits for a delay (spinup or select) to pass */
static int fd_wait_for_completion(unsigned long delay, timeout_fn function)
{
	if (FDCS->reset){
		reset_fdc(); /* do the reset during sleep to win time
			      * if we don't need to sleep, it's a good
			      * occasion anyways */
		return 1;
	}

	if ((signed) (ticks - delay) < 0){
		del_timer(&fd_timer);
		fd_timer.function = function;
		fd_timer.expires = delay;
		add_timer(&fd_timer);
		return 1;
	}
	return 0;
}

static spinlock_t floppy_hlt_lock = SPIN_LOCK_UNLOCKED;
static int hlt_disabled;
static void floppy_disable_hlt(void)
{
	unsigned long flags;

	spin_lock_irqsave(&floppy_hlt_lock, flags);
	if (!hlt_disabled) {
		hlt_disabled=1;
#ifdef HAVE_DISABLE_HLT
		disable_hlt();
#endif
	}
	spin_unlock_irqrestore(&floppy_hlt_lock, flags);
}

static void floppy_enable_hlt(void)
{
	unsigned long flags;

	spin_lock_irqsave(&floppy_hlt_lock, flags);
	if (hlt_disabled){
		hlt_disabled=0;
#ifdef HAVE_DISABLE_HLT
		enable_hlt();
#endif
	}
	spin_unlock_irqrestore(&floppy_hlt_lock, flags);
}


static void setup_DMA(void)
{
	unsigned long f, i;

#ifdef FLOPPY_SANITY_CHECK
	if (raw_cmd->length == 0){
		int i;

		printk("zero dma transfer size:");
		for (i=0; i < raw_cmd->cmd_count; i++)
			printk("%x,", raw_cmd->cmd[i]);
		printk("\n");
		cont->done(0);
		FDCS->reset = 1;
		return;
	}
	if (((unsigned long) raw_cmd->kernel_data) % 512){
		printk("non aligned address: %p\n", raw_cmd->kernel_data);
		cont->done(0);
		FDCS->reset=1;
		return;
	}
#endif
	f=claim_dma_lock();
	fd_disable_dma();
#ifdef fd_dma_setup
	if (fd_dma_setup(raw_cmd->kernel_data, raw_cmd->length, 
			(raw_cmd->flags & FD_RAW_READ)?
			DMA_MODE_READ : DMA_MODE_WRITE,
			FDCS->address) < 0) {
		release_dma_lock(f);
		cont->done(0);
		FDCS->reset=1;
		return;
	}
	release_dma_lock(f);
#else	
	fd_clear_dma_ff();
	fd_cacheflush(raw_cmd->kernel_data, raw_cmd->length);
#undef DEBUG_QIAO 
#ifdef DEBUG_QIAO
	if(raw_cmd->flags & FD_RAW_READ)
	{
	memset(raw_cmd->kernel_data,0,raw_cmd->length);
	fd_cacheflush(raw_cmd->kernel_data, raw_cmd->length);
	}
#endif
	fd_set_dma_mode((raw_cmd->flags & FD_RAW_READ)?
			DMA_MODE_READ : DMA_MODE_WRITE);
	fd_set_dma_addr(raw_cmd->kernel_data);
	fd_set_dma_count(raw_cmd->length);
	virtual_dma_port = FDCS->address;
	fd_enable_dma();
	release_dma_lock(f);
#endif
	floppy_disable_hlt();
}

static void show_floppy(void);

/* waits until the fdc becomes ready */
static int wait_til_ready(void)
{
	int counter, status;
	if (FDCS->reset)
		return -1;
	for (counter = 0; counter < 10000; counter++) {
		status = fd_inb(FD_STATUS);		
		if (status & STATUS_READY)
			return status;
	}
	if (!initialising) {
		DPRINT("Getstatus times out (%x) on fdc %d\n",
			status, fdc);
		show_floppy();
	}
	FDCS->reset = 1;
	return -1;
}

/* sends a command byte to the fdc */
static int output_byte(char byte)
{
	int status;

	if ((status = wait_til_ready()) < 0)
		return -1;
	if ((status & (STATUS_READY|STATUS_DIR|STATUS_DMA)) == STATUS_READY){
		fd_outb(byte,FD_DATA);
#ifdef FLOPPY_SANITY_CHECK
		output_log[output_log_pos].data = byte;
		output_log[output_log_pos].status = status;
		output_log[output_log_pos].ticks = ticks;
		output_log_pos = (output_log_pos + 1) % OLOGSIZE;
#endif
		return 0;
	}
	FDCS->reset = 1;
	if (!initialising) {
		DPRINT("Unable to send byte %x to FDC. Fdc=%x Status=%x\n",
		       byte, fdc, status);
		show_floppy();
	}
	return -1;
}
#define LAST_OUT(x) if (output_byte(x)<0){ reset_fdc();return;}

/* gets the response from the fdc */
static int result(void)
{
	int i, status=0;

	for(i=0; i < MAX_REPLIES; i++) {
		if ((status = wait_til_ready()) < 0)
			break;
		status &= STATUS_DIR|STATUS_READY|STATUS_BUSY|STATUS_DMA;
		if ((status & ~STATUS_BUSY) == STATUS_READY){
#ifdef FLOPPY_SANITY_CHECK
			resultticks = ticks;
			resultsize = i;
#endif
			return i;
		}
		if (status == (STATUS_DIR|STATUS_READY|STATUS_BUSY))
			reply_buffer[i] = fd_inb(FD_DATA);
		else
			break;
	}
	if (!initialising) {
		DPRINT("get result error. Fdc=%d Last status=%x Read bytes=%d\n",
		       fdc, status, i);
		show_floppy();
	}
	FDCS->reset = 1;
	return -1;
}

#define MORE_OUTPUT -2
/* does the fdc need more output? */
static int need_more_output(void)
{
	int status;
	if ((status = wait_til_ready()) < 0)
		return -1;
	if ((status & (STATUS_READY|STATUS_DIR|STATUS_DMA)) == STATUS_READY)
		return MORE_OUTPUT;
	return result();
}

/* Set perpendicular mode as required, based on data rate, if supported.
 * 82077 Now tested. 1Mbps data rate only possible with 82077-1.
 */
static inline void perpendicular_mode(void)
{
	unsigned char perp_mode;

	if (raw_cmd->rate & 0x40){
		switch(raw_cmd->rate & 3){
			case 0:
				perp_mode=2;
				break;
			case 3:
				perp_mode=3;
				break;
			default:
				DPRINT("Invalid data rate for perpendicular mode!\n");
				cont->done(0);
				FDCS->reset = 1; /* convenient way to return to
						  * redo without to much hassle (deep
						  * stack et al. */
				return;
		}
	} else
		perp_mode = 0;

	if (FDCS->perp_mode == perp_mode)
		return;
	if (FDCS->version >= FDC_82077_ORIG) {
		output_byte(FD_PERPENDICULAR);
		output_byte(perp_mode);
		FDCS->perp_mode = perp_mode;
	} else if (perp_mode) {
		DPRINT("perpendicular mode not supported by this FDC.\n");
	}
} /* perpendicular_mode */

static int fifo_depth = 0xa;
static int no_fifo;

static int fdc_configure(void)
{
	/* Turn on FIFO */
	output_byte(FD_CONFIGURE);
	if (need_more_output() != MORE_OUTPUT)
		return 0;
	output_byte(0);
	output_byte(0x10 | (no_fifo & 0x20) | (fifo_depth & 0xf));
	output_byte(0);	/* pre-compensation from track 
			   0 upwards */
	return 1;
}	

#define NOMINAL_DTR 500

/* Issue a "SPECIFY" command to set the step rate time, head unload time,
 * head load time, and DMA disable flag to values needed by floppy.
 *
 * The value "dtr" is the data transfer rate in Kbps.  It is needed
 * to account for the data rate-based scaling done by the 82072 and 82077
 * FDC types.  This parameter is ignored for other types of FDCs (i.e.
 * 8272a).
 *
 * Note that changing the data transfer rate has a (probably deleterious)
 * effect on the parameters subject to scaling for 82072/82077 FDCs, so
 * fdc_specify is called again after each data transfer rate
 * change.
 *
 * srt: 1000 to 16000 in microseconds
 * hut: 16 to 240 milliseconds
 * hlt: 2 to 254 milliseconds
 *
 * These values are rounded up to the next highest available delay time.
 */
static void fdc_specify(void)
{
	unsigned char spec1, spec2;
	unsigned long srt, hlt, hut;
	unsigned long dtr = NOMINAL_DTR;
	unsigned long scale_dtr = NOMINAL_DTR;
	int hlt_max_code = 0x7f;
	int hut_max_code = 0xf;

	if (FDCS->need_configure && FDCS->version >= FDC_82072A) {
		fdc_configure();
		FDCS->need_configure = 0;
		/*DPRINT("FIFO enabled\n");*/
	}

	switch (raw_cmd->rate & 0x03) {
		case 3:
			dtr = 1000;
			break;
		case 1:
			dtr = 300;
			if (FDCS->version >= FDC_82078) {
				/* chose the default rate table, not the one
				 * where 1 = 2 Mbps */
				output_byte(FD_DRIVESPEC);
				if (need_more_output() == MORE_OUTPUT) {
					output_byte(UNIT(current_drive));
					output_byte(0xc0);
				}
			}
			break;
		case 2:
			dtr = 250;
			break;
	}

	if (FDCS->version >= FDC_82072) {
		scale_dtr = dtr;
		hlt_max_code = 0x00; /* 0==256msec*dtr0/dtr (not linear!) */
		hut_max_code = 0x0; /* 0==256msec*dtr0/dtr (not linear!) */
	}

	/* Convert step rate from microseconds to milliseconds and 4 bits */
	srt = 16 - (DP->srt*scale_dtr/1000 + NOMINAL_DTR - 1)/NOMINAL_DTR;
	if( slow_floppy ) {
		srt = srt / 4;
	}
	SUPBOUND(srt, 0xf);
	INFBOUND(srt, 0);

	hlt = (DP->hlt*scale_dtr/2 + NOMINAL_DTR - 1)/NOMINAL_DTR;
	if (hlt < 0x01)
		hlt = 0x01;
	else if (hlt > 0x7f)
		hlt = hlt_max_code;

	hut = (DP->hut*scale_dtr/16 + NOMINAL_DTR - 1)/NOMINAL_DTR;
	if (hut < 0x1)
		hut = 0x1;
	else if (hut > 0xf)
		hut = hut_max_code;

	spec1 = (srt << 4) | hut;
	spec2 = (hlt << 1) | (use_virtual_dma & 1);

	/* If these parameters did not change, just return with success */
	if (FDCS->spec1 != spec1 || FDCS->spec2 != spec2) {
		/* Go ahead and set spec1 and spec2 */
		output_byte(FD_SPECIFY);
		output_byte(FDCS->spec1 = spec1);
		output_byte(FDCS->spec2 = spec2);
	}
} /* fdc_specify */

/* Set the FDC's data transfer rate on behalf of the specified drive.
 * NOTE: with 82072/82077 FDCs, changing the data rate requires a reissue
 * of the specify command (i.e. using the fdc_specify function).
 */
static int fdc_dtr(void)
{
	/* If data rate not already set to desired value, set it. */
	if ((raw_cmd->rate & 3) == FDCS->dtr)
		return 0;

	/* Set dtr */
	fd_outb(raw_cmd->rate & 3, FD_DCR);

	/* TODO: some FDC/drive combinations (C&T 82C711 with TEAC 1.2MB)
	 * need a stabilization period of several milliseconds to be
	 * enforced after data rate changes before R/W operations.
	 * Pause 5 msec to avoid trouble. (Needs to be 2 ticks)
	 */
	FDCS->dtr = raw_cmd->rate & 3;
	return(fd_wait_for_completion(ticks+2UL*HZ/100,
				   (timeout_fn) floppy_ready));
} /* fdc_dtr */

static void tell_sector(void)
{
	printk(": track %d, head %d, sector %d, size %d",
	       R_TRACK, R_HEAD, R_SECTOR, R_SIZECODE);
} /* tell_sector */


/*
 * OK, this error interpreting routine is called after a
 * DMA read/write has succeeded
 * or failed, so we check the results, and copy any buffers.
 * hhb: Added better error reporting.
 * ak: Made this into a separate routine.
 */
static int interpret_errors(void)
{
	char bad;

	if (inr!=7) {
		DPRINT("-- FDC reply error");
		FDCS->reset = 1;
		return 1;
	}

	/* check IC to find cause of interrupt */
	switch (ST0 & ST0_INTR) {
		case 0x40:	/* error occurred during command execution */
			if (ST1 & ST1_EOC)
				return 0; /* occurs with pseudo-DMA */
			bad = 1;
			if (ST1 & ST1_WP) {
				DPRINT("Drive is write protected\n");
				CLEARF(FD_DISK_WRITABLE);
				cont->done(0);
				bad = 2;
			} else if (ST1 & ST1_ND) {
				SETF(FD_NEED_TWADDLE);
			} else if (ST1 & ST1_OR) {
				if (DP->flags & FTD_MSG)
					DPRINT("Over/Underrun - retrying\n");
				bad = 0;
			}else if (*errors >= DP->max_errors.reporting){
				DPRINT("");
				if (ST0 & ST0_ECE) {
					printk("Recalibrate failed!");
				} else if (ST2 & ST2_CRC) {
					printk("data CRC error");
					tell_sector();
				} else if (ST1 & ST1_CRC) {
					printk("CRC error");
					tell_sector();
				} else if ((ST1 & (ST1_MAM|ST1_ND)) || (ST2 & ST2_MAM)) {
					if (!probing) {
						printk("sector not found");
						tell_sector();
					} else
						printk("probe failed...");
				} else if (ST2 & ST2_WC) {	/* seek error */
					printk("wrong cylinder");
				} else if (ST2 & ST2_BC) {	/* cylinder marked as bad */
					printk("bad cylinder");
				} else {
					printk("unknown error. ST[0..2] are: 0x%x 0x%x 0x%x", ST0, ST1, ST2);
					tell_sector();
				}
				printk("\n");

			}
			if (ST2 & ST2_WC || ST2 & ST2_BC)
				/* wrong cylinder => recal */
				DRS->track = NEED_2_RECAL;
			return bad;
		case 0x80: /* invalid command given */
			DPRINT("Invalid FDC command given!\n");
			cont->done(0);
			return 2;
		case 0xc0:
			DPRINT("Abnormal termination caused by polling\n");
			cont->error();
			return 2;
		default: /* (0) Normal command termination */
			return 0;
	}
}

/*
 * This routine is called when everything should be correctly set up
 * for the transfer (i.e. floppy motor is on, the correct floppy is
 * selected, and the head is sitting on the right track).
 */
static void setup_rw_floppy(void)
{
	int i,r, flags,dflags;
	unsigned long ready_date;
	timeout_fn function;

	flags = raw_cmd->flags;
	if (flags & (FD_RAW_READ | FD_RAW_WRITE))
		flags |= FD_RAW_INTR;

	if ((flags & FD_RAW_SPIN) && !(flags & FD_RAW_NO_MOTOR)){
		ready_date = DRS->spinup_date + DP->spinup;
		/* If spinup will take a long time, rerun scandrives
		 * again just before spinup completion. Beware that
		 * after scandrives, we must again wait for selection.
		 */
		if ((signed) (ready_date - ticks) > DP->select_delay){
			ready_date -= DP->select_delay;
			function = (timeout_fn) floppy_start;
		} else
			function = (timeout_fn) setup_rw_floppy;

		/* wait until the floppy is spinning fast enough */
		if (fd_wait_for_completion(ready_date,function))
			return;
	}
	dflags = DRS->flags;

	if ((flags & FD_RAW_READ) || (flags & FD_RAW_WRITE))
		setup_DMA();

	if (flags & FD_RAW_INTR)
		SET_INTR(main_command_interrupt);

	r=0;
	for (i=0; i< raw_cmd->cmd_count; i++)
		r|=output_byte(raw_cmd->cmd[i]);

#ifdef DEBUGT
	debugt("rw_command: ");
#endif
	if (r){
		cont->error();
		reset_fdc();
		return;
	}

	if (!(flags & FD_RAW_INTR)){
		inr = result();
		cont->interrupt();
	} else if (flags & FD_RAW_NEED_DISK)
		fd_watchdog();
}

static int blind_seek;

/*
 * This is the routine called after every seek (or recalibrate) interrupt
 * from the floppy controller.
 */
static void seek_interrupt(void)
{
#ifdef DEBUGT
	debugt("seek interrupt:");
#endif
	if (inr != 2 || (ST0 & 0xF8) != 0x20) {
		DPRINT("seek failed\n");
		DRS->track = NEED_2_RECAL;
		cont->error();
		cont->redo();
		return;
	}
	if (DRS->track >= 0 && DRS->track != ST1 && !blind_seek){
#ifdef DCL_DEBUG
		if (DP->flags & FD_DEBUG){
			DPRINT("clearing NEWCHANGE flag because of effective seek\n");
			DPRINT("ticks=%lu\n", ticks);
		}
#endif
		CLEARF(FD_DISK_NEWCHANGE); /* effective seek */
		DRS->select_date = ticks;
	}
	DRS->track = ST1;
	floppy_ready();
}

static void check_wp(void)
{
	if (TESTF(FD_VERIFY)) {
		/* check write protection */
		output_byte(FD_GETSTATUS);
		output_byte(UNIT(current_drive));
		if (result() != 1){
			FDCS->reset = 1;
			return;
		}
		CLEARF(FD_VERIFY);
		CLEARF(FD_NEED_TWADDLE);
#ifdef DCL_DEBUG
		if (DP->flags & FD_DEBUG){
			DPRINT("checking whether disk is write protected\n");
			DPRINT("wp=%x\n",ST3 & 0x40);
		}
#endif
		if (!(ST3  & 0x40))
			SETF(FD_DISK_WRITABLE);
		else
			CLEARF(FD_DISK_WRITABLE);
	}
}

static void seek_floppy(void)
{
	int track;

	blind_seek=0;

#ifdef DCL_DEBUG
	if (DP->flags & FD_DEBUG){
		DPRINT("calling disk change from seek\n");
	}
#endif

	if (!TESTF(FD_DISK_NEWCHANGE) &&
	    disk_change(current_drive) &&
	    (raw_cmd->flags & FD_RAW_NEED_DISK)){
		/* the media changed flag should be cleared after the seek.
		 * If it isn't, this means that there is really no disk in
		 * the drive.
		 */
		SETF(FD_DISK_CHANGED);
		cont->done(0);
		cont->redo();
		return;
	}
	if (DRS->track <= NEED_1_RECAL){
		recalibrate_floppy();
		return;
	} else if (TESTF(FD_DISK_NEWCHANGE) &&
		   (raw_cmd->flags & FD_RAW_NEED_DISK) &&
		   (DRS->track <= NO_TRACK || DRS->track == raw_cmd->track)) {
		/* we seek to clear the media-changed condition. Does anybody
		 * know a more elegant way, which works on all drives? */
		if (raw_cmd->track)
			track = raw_cmd->track - 1;
		else {
			if (DP->flags & FD_SILENT_DCL_CLEAR){
				set_dor(fdc, ~(0x10 << UNIT(current_drive)), 0);
				blind_seek = 1;
				raw_cmd->flags |= FD_RAW_NEED_SEEK;
			}
			track = 1;
		}
	} else {
		check_wp();
		if (raw_cmd->track != DRS->track &&
		    (raw_cmd->flags & FD_RAW_NEED_SEEK))
			track = raw_cmd->track;
		else {
			setup_rw_floppy();
			return;
		}
	}

	SET_INTR(seek_interrupt);
	output_byte(FD_SEEK);
	output_byte(UNIT(current_drive));
	LAST_OUT(track);
#ifdef DEBUGT
	debugt("seek command:");
#endif
}

static void recal_interrupt(void)
{
#ifdef DEBUGT
	debugt("recal interrupt:");
#endif
	if (inr !=2)
		FDCS->reset = 1;
	else if (ST0 & ST0_ECE) {
	       	switch(DRS->track){
			case NEED_1_RECAL:
#ifdef DEBUGT
				debugt("recal interrupt need 1 recal:");
#endif
				/* after a second recalibrate, we still haven't
				 * reached track 0. Probably no drive. Raise an
				 * error, as failing immediately might upset
				 * computers possessed by the Devil :-) */
				cont->error();
				cont->redo();
				return;
			case NEED_2_RECAL:
#ifdef DEBUGT
				debugt("recal interrupt need 2 recal:");
#endif
				/* If we already did a recalibrate,
				 * and we are not at track 0, this
				 * means we have moved. (The only way
				 * not to move at recalibration is to
				 * be already at track 0.) Clear the
				 * new change flag */
#ifdef DCL_DEBUG
				if (DP->flags & FD_DEBUG){
					DPRINT("clearing NEWCHANGE flag because of second recalibrate\n");
				}
#endif

				CLEARF(FD_DISK_NEWCHANGE);
				DRS->select_date = ticks;
				/* fall through */
			default:
#ifdef DEBUGT
				debugt("recal interrupt default:");
#endif
				/* Recalibrate moves the head by at
				 * most 80 steps. If after one
				 * recalibrate we don't have reached
				 * track 0, this might mean that we
				 * started beyond track 80.  Try
				 * again.  */
				DRS->track = NEED_1_RECAL;
				break;
		}
	} else
		DRS->track = ST1;
	floppy_ready();
}

static void print_result(char *message, int inr)
{
	int i;

	DPRINT("%s ", message);
	if (inr >= 0)
		for (i=0; i<inr; i++)
			printk("repl[%d]=%x ", i, reply_buffer[i]);
	printk("\n");
}


/* interrupt handler. Note that this can be called externally on the Sparc */
void floppy_interrupt(int irq, void *dev_id, void * regs)
{
	void (*handler)(void) = DEVICE_INTR;
	int do_print;
	unsigned long f;

	lasthandler = handler;
	interruptticks = ticks;

	f=claim_dma_lock();
	fd_disable_dma();
	release_dma_lock(f);
	
	floppy_enable_hlt();
	CLEAR_INTR;
	if (fdc >= N_FDC || FDCS->address == -1){
		/* we don't even know which FDC is the culprit */
		printk("DOR0=%x\n", fdc_state[0].dor);
		printk("floppy interrupt on bizarre fdc %d\n",fdc);
		printk("handler=%p\n", handler);
		is_alive("bizarre fdc");
		return;
	}

	FDCS->reset = 0;
	/* We have to clear the reset flag here, because apparently on boxes
	 * with level triggered interrupts (PS/2, Sparc, ...), it is needed to
	 * emit SENSEI's to clear the interrupt line. And FDCS->reset blocks the
	 * emission of the SENSEI's.
	 * It is OK to emit floppy commands because we are in an interrupt
	 * handler here, and thus we have to fear no interference of other
	 * activity.
	 */

	do_print = !handler && print_unex && !initialising;

	inr = result();
	if (do_print)
		print_result("unexpected interrupt", inr);
	if (inr == 0){
		int max_sensei = 4;
		do {
			output_byte(FD_SENSEI);
			inr = result();
			if (do_print)
				print_result("sensei", inr);
			max_sensei--;
		} while ((ST0 & 0x83) != UNIT(current_drive) && inr == 2 && max_sensei);
	}
				//print_result("test", inr);
	
	if (handler) {
		schedule_bh( (void *)(void *) handler);
	} else
		FDCS->reset = 1;
	is_alive("normal interrupt end");
}

static void recalibrate_floppy(void)
{
#ifdef DEBUGT
	debugt("recalibrate floppy:");
#endif
	SET_INTR(recal_interrupt);
	output_byte(FD_RECALIBRATE);
	LAST_OUT(UNIT(current_drive));
}

/*
 * Must do 4 FD_SENSEIs after reset because of ``drive polling''.
 */
static void reset_interrupt(void)
{
#ifdef DEBUGT
	debugt("reset interrupt:");
#endif
	result();		/* get the status ready for set_fdc */
	if (FDCS->reset) {
		printk("reset set in interrupt, calling %p\n", cont->error);
		cont->error(); /* a reset just after a reset. BAD! */
	}
	cont->redo();
}

/*
 * reset is done by pulling bit 2 of DOR low for a while (old FDCs),
 * or by setting the self clearing bit 7 of STATUS (newer FDCs)
 */
static void reset_fdc(void)
{
	unsigned long flags;

	SET_INTR(reset_interrupt);
	FDCS->reset = 0;
	reset_fdc_info(0);

	/* Pseudo-DMA may intercept 'reset finished' interrupt.  */
	/* Irrelevant for systems with true DMA (i386).          */
	
	flags=claim_dma_lock();
	fd_disable_dma();
	release_dma_lock(flags);

	if (FDCS->version >= FDC_82072A)
		fd_outb(0x80 | (FDCS->dtr &3), FD_STATUS);
	else {
		fd_outb(FDCS->dor & ~0x04, FD_DOR);
		udelay(FD_RESET_DELAY);
		fd_outb(FDCS->dor, FD_DOR);
	}
}

static void show_floppy(void)
{
	int i;

	printk("\n");
	printk("floppy driver state\n");
	printk("-------------------\n");
	printk("now=%lu last interrupt=%lu diff=%lu last called handler=%p\n",
	       ticks, interruptticks, ticks-interruptticks, lasthandler);


#ifdef FLOPPY_SANITY_CHECK
	printk("timeout_message=%s\n", timeout_message);
	printk("last output bytes:\n");
	for (i=0; i < OLOGSIZE; i++)
		printk("%2x %2x %lu\n",
		       output_log[(i+output_log_pos) % OLOGSIZE].data,
		       output_log[(i+output_log_pos) % OLOGSIZE].status,
		       output_log[(i+output_log_pos) % OLOGSIZE].ticks);
	printk("last result at %lu\n", resultticks);
	printk("last redo_fd_request at %lu\n", lastredo);
	for (i=0; i<resultsize; i++){
		printk("%2x ", reply_buffer[i]);
	}
	printk("\n");
#endif

	printk("status=%x\n", fd_inb(FD_STATUS));
	printk("fdc_busy=%lu\n", fdc_busy);
	if (DEVICE_INTR)
		printk("DEVICE_INTR=%p\n", DEVICE_INTR);
	if (floppy_tq.sync)
		printk("floppy_tq.routine=%p\n", floppy_tq.routine);
	if (timer_pending(&fd_timer))
		printk("fd_timer.function=%p\n", fd_timer.function);
	if (timer_pending(&fd_timeout)){
		printk("timer_function=%p\n",fd_timeout.function);
		printk("expires=%lu\n",fd_timeout.expires-ticks);
		printk("now=%lu\n",ticks);
	}
	printk("cont=%p\n", cont);
	printk("CURRENT=%p\n", CURRENT);
	printk("command_status=%d\n", command_status);
	printk("\n");
}

static void floppy_shutdown(void)
{
	unsigned long flags;
	
	if (!initialising)
		show_floppy();
	cancel_activity();

	floppy_enable_hlt();
	
	flags=claim_dma_lock();
	fd_disable_dma();
	release_dma_lock(flags);
	
	/* avoid dma going to a random drive after shutdown */

	if (!initialising)
		DPRINT("floppy timeout called\n");
	FDCS->reset = 1;
	if (cont){
		cont->done(0);
		cont->redo(); /* this will recall reset when needed */
	} else {
		printk("no cont in shutdown!\n");
		process_fd_request();
	}
	is_alive("floppy shutdown");
}
/*typedef void (*timeout_fn)(unsigned long);*/

/* start motor, check media-changed condition and write protection */
static int start_motor(void (*function)(void) )
{
	int mask, data;

	mask = 0xfc;
	data = UNIT(current_drive);
	if (!(raw_cmd->flags & FD_RAW_NO_MOTOR)){
		if (!(FDCS->dor & (0x10 << UNIT(current_drive)))){
			set_debugt();
			/* no read since this drive is running */
			DRS->first_read_date = 0;
			/* note motor start time if motor is not yet running */
			DRS->spinup_date = ticks;
			data |= (0x10 << UNIT(current_drive));
		}
	} else
		if (FDCS->dor & (0x10 << UNIT(current_drive)))
			mask &= ~(0x10 << UNIT(current_drive));

	/* starts motor and selects floppy */
	del_timer(motor_off_timer + current_drive);
	set_dor(fdc, mask, data);
	/* wait_for_completion also schedules reset if needed. */
	return(fd_wait_for_completion(DRS->select_date+DP->select_delay,
				   (timeout_fn) function));
}

static void floppy_ready(void)
{
	CHECK_RESET;
	if (start_motor(floppy_ready)) return;
	if (fdc_dtr()) return;

#ifdef DCL_DEBUG
	if (DP->flags & FD_DEBUG){
		DPRINT("calling disk change from floppy_ready\n");
	}
#endif
	if (!(raw_cmd->flags & FD_RAW_NO_MOTOR) &&
	   disk_change(current_drive) &&
	   !DP->select_delay)
		twaddle(); /* this clears the dcl on certain drive/controller
			    * combinations */

#ifdef fd_chose_dma_mode
	if ((raw_cmd->flags & FD_RAW_READ) || 
	    (raw_cmd->flags & FD_RAW_WRITE))
	{
		unsigned long flags = claim_dma_lock();
		fd_chose_dma_mode(raw_cmd->kernel_data,
				  raw_cmd->length);
		release_dma_lock(flags);
	}
#endif

	if (raw_cmd->flags & (FD_RAW_NEED_SEEK | FD_RAW_NEED_DISK)){
		perpendicular_mode();
		fdc_specify(); /* must be done here because of hut, hlt ... */
		seek_floppy();
	} else {
		if ((raw_cmd->flags & FD_RAW_READ) || 
		    (raw_cmd->flags & FD_RAW_WRITE))
			fdc_specify();
		setup_rw_floppy();
	}
}

static void floppy_start(void)
{
	reschedule_timeout(CURRENTD, "floppy start", 0);

	scandrives();
#ifdef DCL_DEBUG
	if (DP->flags & FD_DEBUG){
		DPRINT("setting NEWCHANGE in floppy_start\n");
	}
#endif
	SETF(FD_DISK_NEWCHANGE);
	floppy_ready();
}

/*
 * ========================================================================
 * here ends the bottom half. Exported routines are:
 * floppy_start, floppy_off, floppy_ready, lock_fdc, unlock_fdc, set_fdc,
 * start_motor, reset_fdc, reset_fdc_info, interpret_errors.
 * Initialization also uses output_byte, result, set_dor, floppy_interrupt
 * and set_dor.
 * ========================================================================
 */
/*
 * General purpose continuations.
 * ==============================
 */

static void do_wakeup(void)
{
	reschedule_timeout(MAXTIMEOUT, "do wakeup", 0);
	cont = 0;
	command_status += 2;
	wake_up(&command_done);
}

static struct cont_t wakeup_cont={
	empty,
	do_wakeup,
	empty,
	(done_f)empty
};


static struct cont_t intr_cont={
	empty,
	process_fd_request,
	empty,
	(done_f) empty
};

static int wait_til_done(void (*handler)(void), int interruptible)
{
	int ret;

	schedule_bh((void *)(void *)handler);

	if (command_status < 2 && NO_SIGNAL) {

		for (;;) {

			if (command_status >= 2 || !NO_SIGNAL)
				break;

			is_alive("wait_til_done");

			sleep_on(&command_done);

		}

	}

	if (command_status < 2){
		cancel_activity();
		cont = &intr_cont;
		reset_fdc();
		return -EINTR;
	}

	if (FDCS->reset)
		command_status = FD_COMMAND_ERROR;
	if (command_status == FD_COMMAND_OKAY)
		ret=0;
	else
		ret=-EIO;
	command_status = FD_COMMAND_NONE;
	return ret;
}

static void generic_done(int result)
{
	command_status = result;
	cont = &wakeup_cont;
}

static void generic_success(void)
{
	cont->done(1);
}

static void generic_failure(void)
{
	cont->done(0);
}

static void success_and_wakeup(void)
{
	generic_success();
	cont->redo();
}


/*
 * formatting and rw support.
 * ==========================
 */

static int next_valid_format(void)
{
	int probed_format;

	probed_format = DRS->probed_format;
	while(1){
		if (probed_format >= 8 ||
		     !DP->autodetect[probed_format]){
			DRS->probed_format = 0;
			return 1;
		}
		if (floppy_type[DP->autodetect[probed_format]].sect){
			DRS->probed_format = probed_format;
			return 0;
		}
		probed_format++;
	}
}

static void bad_flp_intr(void)
{
	if (probing){
		DRS->probed_format++;
		if (!next_valid_format())
			return;
	}
	(*errors)++;
	INFBOUND(DRWE->badness, *errors);
	if (*errors > DP->max_errors.abort)
		cont->done(0);
	if (*errors > DP->max_errors.reset)
		FDCS->reset = 1;
	else if (*errors > DP->max_errors.recal)
		DRS->track = NEED_2_RECAL;
}

static void set_floppy(kdev_t device)
{
	if (TYPE(device))
		_floppy = TYPE(device) + floppy_type;
	else
		_floppy = current_type[ DRIVE(device) ];
}

#define CODE2SIZE (ssize = ((1 << SIZECODE) + 3) >> 2)
#define FM_MODE(x,y) ((y) & ~(((x)->rate & 0x80) >>1))
#define CT(x) ((x) | 0xc0)

/*
 * Buffer read/write and support
 * =============================
 */

/* new request_done. Can handle physical sectors which are smaller than a
 * logical buffer */
static void add_request(int rw,struct buffer_head *bh)
{
	static struct request lrequest;
	struct request *req=&lrequest;
	    unsigned int sector, count, sync;

    count = bh->b_size >> 9;
    sector = bh->b_rsector;
    sync = test_and_clear_bit(BH_Sync, &bh->b_state);



    req->cmd = rw;
    req->errors = 0;
    req->hard_sector = req->sector = sector;
    req->hard_nr_sectors = req->nr_sectors = count;
    req->current_nr_sectors = req->hard_cur_sectors = count;
    req->nr_segments = 1; /* Always 1 for a new request. */
    req->nr_hw_segments = 1; /* Always 1 for a new request. */
    req->buffer = bh->b_data;
    req->waiting = NULL;
    req->bh = bh;
    req->bhtail = bh;
    req->rq_dev = bh->b_rdev;
    req->start_time = ticks;
	current_request=req;
	do_fd_request(req);
}


static void request_done(int uptodate)
{
	int block;
	unsigned long flags;

	probing = 0;
	//reschedule_timeout(MAXTIMEOUT, "request done %d", uptodate);
	del_timer(&fd_timeout);

	if (QUEUE_EMPTY){
		DPRINT("request list destroyed in floppy request done\n");
		return;
	}

	if (uptodate){
		/* maintain values for invalidation on geometry
		 * change */
		block = current_count_sectors + CURRENT->sector;
		INFBOUND(DRS->maxblock, block);
		if (block > _floppy->sect)
			DRS->maxtrack = 1;

		/* unlock chained buffers */
		spin_lock_irqsave(&io_request_lock, flags);
		while (current_count_sectors && !QUEUE_EMPTY &&
		       current_count_sectors >= CURRENT->current_nr_sectors){
			current_count_sectors -= CURRENT->current_nr_sectors;
			CURRENT->nr_sectors -= CURRENT->current_nr_sectors;
			CURRENT->sector += CURRENT->current_nr_sectors;
			end_request(1);
		}
		spin_unlock_irqrestore(&io_request_lock, flags);

		if (current_count_sectors && !QUEUE_EMPTY){
			/* "unlock" last subsector */
			CURRENT->buffer += current_count_sectors <<9;
			CURRENT->current_nr_sectors -= current_count_sectors;
			CURRENT->nr_sectors -= current_count_sectors;
			CURRENT->sector += current_count_sectors;
			return;
		}

		if (current_count_sectors && QUEUE_EMPTY)
			DPRINT("request list destroyed in floppy request done\n");

	} else {
		if (CURRENT->cmd == WRITE) {
			/* record write error information */
			DRWE->write_errors++;
			if (DRWE->write_errors == 1) {
				DRWE->first_error_sector = CURRENT->sector;
				DRWE->first_error_generation = DRS->generation;
			}
			DRWE->last_error_sector = CURRENT->sector;
			DRWE->last_error_generation = DRS->generation;
		}
		spin_lock_irqsave(&io_request_lock, flags);
		end_request(0);
		spin_unlock_irqrestore(&io_request_lock, flags);
	}
}

/* Interrupt handler evaluating the result of the r/w operation */
static void rw_interrupt(void)
{
	int nr_sectors, ssize, eoc, heads;
	int errors_result;


	if (R_HEAD >= 2) {
	    /* some Toshiba floppy controllers occasionnally seem to
	     * return bogus interrupts after read/write operations, which
	     * can be recognized by a bad head number (>= 2) */
	     return;
	}  

	if (!DRS->first_read_date)
		DRS->first_read_date = ticks;

	nr_sectors = 0;
	CODE2SIZE;

	if (ST1 & ST1_EOC)
		eoc = 1;
	else
		eoc = 0;

	if (COMMAND & 0x80)
		heads = 2;
	else
		heads = 1;
	nr_sectors = (((R_TRACK-TRACK) * heads +
				   R_HEAD-HEAD) * SECT_PER_TRACK +
				   R_SECTOR-SECTOR + eoc) << SIZECODE >> 2;
#ifdef FLOPPY_SANITY_CHECK
	if (nr_sectors / ssize > 
		(in_sector_offset + current_count_sectors + ssize - 1) / ssize) {
		DPRINT("long rw: %x instead of %lx\n",
			nr_sectors, current_count_sectors);
		printk("rs=%d s=%d\n", R_SECTOR, SECTOR);
		printk("rh=%d h=%d\n", R_HEAD, HEAD);
		printk("rt=%d t=%d\n", R_TRACK, TRACK);
		printk("heads=%d eoc=%d\n", heads, eoc);
		printk("spt=%d st=%d ss=%d\n", SECT_PER_TRACK,
		       sector_t, ssize);
		printk("in_sector_offset=%d\n", in_sector_offset);
	}
#endif
	nr_sectors -= in_sector_offset;
	INFBOUND(nr_sectors,0);
	SUPBOUND(current_count_sectors, nr_sectors);

	errors_result=interpret_errors();
	switch (errors_result){
		case 2:
			cont->redo();
			return;
		case 1:
			if (!current_count_sectors){
				cont->error();
				cont->redo();
				return;
			}
			break;
		case 0:
			if (!current_count_sectors){
				cont->redo(); //error from here,maybe floppy_init have fault
				return;
			}
			current_type[current_drive] = _floppy;
			floppy_sizes[TOMINOR(current_drive) ]= 
				(_floppy->size+1)>>1;
			break;
	}
	if (probing) {
		if (DP->flags & FTD_MSG)
			DPRINT("Auto-detected floppy type %s in fd%d\n",
				_floppy->name,current_drive);
		current_type[current_drive] = _floppy;
		floppy_sizes[TOMINOR(current_drive)] = (_floppy->size+1) >> 1;
		probing = 0;
	}

	if (CT(COMMAND) != FD_READ || 
	     raw_cmd->kernel_data == CURRENT->buffer){
		/* transfer directly from buffer */
		cont->done(1);
	} else if (CT(COMMAND) == FD_READ){
#if 0 //zhb
		if (raw_cmd->cmd[1] == 0) {
			for (i = 0; i < 512; i+=4) {
				if (i % 32 == 0)
					prom_printf("\n");
				prom_printf("%8x ", (unsigned long)raw_cmd->kernel_data[i]);
			}
		}
#endif
		buffer_track = raw_cmd->track;
		buffer_drive = current_drive;
		INFBOUND(buffer_max, nr_sectors + sector_t);
	}
	cont->redo();
}

/* Compute maximal contiguous buffer size. */
static int buffer_chain_size(void)
{
	struct buffer_head *bh;
	int size;
	char *base;

	base = CURRENT->buffer;
	size = CURRENT->current_nr_sectors << 9;
#if 0
	bh = CURRENT->bh;

	if (bh){
		bh = bh->b_reqnext;
		while (bh && bh->b_data == base + size){
			size += bh->b_size;
			bh = bh->b_reqnext;
		}
	}
#endif
	return size >> 9;
}

/* Compute the maximal transfer size */
static int transfer_size(int ssize, int max_sector, int max_size)
{
	SUPBOUND(max_sector, sector_t + max_size);

	/* alignment */
	max_sector -= (max_sector % _floppy->sect) % ssize;

	/* transfer size, beginning not aligned */
	current_count_sectors = max_sector - sector_t ;

	return max_sector;
}

/*
 * Move data from/to the track buffer to/from the buffer cache.
 */
static void copy_buffer(int ssize, int max_sector, int max_sector_2)
{
	int remaining; /* number of transferred 512-byte sectors */
	struct buffer_head *bh;
	char *buffer, *dma_buffer;
	int size;

	max_sector = transfer_size(ssize,
				   minimum(max_sector, max_sector_2),
				   CURRENT->nr_sectors);

	if (current_count_sectors <= 0 && CT(COMMAND) == FD_WRITE &&
	    buffer_max > sector_t + CURRENT->nr_sectors)
		current_count_sectors = minimum(buffer_max - sector_t,
						CURRENT->nr_sectors);

	remaining = current_count_sectors << 9;
#ifdef FLOPPY_SANITY_CHECK
	if ((remaining >> 9) > CURRENT->nr_sectors  &&
	    CT(COMMAND) == FD_WRITE){
		DPRINT("in copy buffer\n");
		printk("current_count_sectors=%ld\n", current_count_sectors);
		printk("remaining=%d\n", remaining >> 9);
		printk("CURRENT->nr_sectors=%ld\n",CURRENT->nr_sectors);
		printk("CURRENT->current_nr_sectors=%ld\n",
		       CURRENT->current_nr_sectors);
		printk("max_sector=%d\n", max_sector);
		printk("ssize=%d\n", ssize);
	}
#endif

	buffer_max = maximum(max_sector, buffer_max);

	dma_buffer = floppy_track_buffer + ((sector_t - buffer_min) << 9);

	bh = CURRENT->bh;
	size = CURRENT->current_nr_sectors << 9;
	buffer = CURRENT->buffer;

	while (remaining > 0){
		SUPBOUND(size, remaining);
#ifdef FLOPPY_SANITY_CHECK
		if (dma_buffer + size >
		    floppy_track_buffer + (max_buffer_sectors << 10) ||
		    dma_buffer < floppy_track_buffer){
			DPRINT("buffer overrun in copy buffer %d\n",
				(int) ((floppy_track_buffer - dma_buffer) >>9));
			printk("sector_t=%d buffer_min=%d\n",
			       sector_t, buffer_min);
			printk("current_count_sectors=%ld\n",
			       current_count_sectors);
			if (CT(COMMAND) == FD_READ)
				printk("read\n");
			if (CT(COMMAND) == FD_READ)
				printk("write\n");
			break;
		}
		if (((unsigned long)buffer) % 512)
			DPRINT("%p buffer not aligned\n", buffer);
#endif
		if (CT(COMMAND) == FD_READ)
			memcpy(buffer, dma_buffer, size);
		else
			memcpy(dma_buffer, buffer, size);
		remaining -= size;
		if (!remaining)
			break;

		dma_buffer += size;
		//bh = bh->b_reqnext;
#ifdef FLOPPY_SANITY_CHECK
		if (!bh){
			DPRINT("bh=null in copy buffer after copy\n");
			break;
		}
#endif
		size = bh->b_size;
		buffer = bh->b_data;
	}
#ifdef FLOPPY_SANITY_CHECK
	if (remaining){
		if (remaining > 0)
			max_sector -= remaining >> 9;
		DPRINT("weirdness: remaining %d\n", remaining>>9);
	}
#endif
}

#if 0
static inline int check_dma_crossing(char *start, 
				     unsigned long length, char *message)
{
	if (CROSS_64KB(start, length)) {
		printk("DMA xfer crosses 64KB boundary in %s %p-%p\n", 
		       message, start, start+length);
		return 1;
	} else
		return 0;
}
#endif

/* work around a bug in pseudo DMA
 * (on some FDCs) pseudo DMA does not stop when the CPU stops
 * sending data.  Hence we need a different way to signal the
 * transfer length:  We use SECT_PER_TRACK.  Unfortunately, this
 * does not work with MT, hence we can only transfer one head at
 * a time
 */
static void virtualdmabug_workaround(void)
{
	int hard_sectors, end_sector;

	if(CT(COMMAND) == FD_WRITE) {
		COMMAND &= ~0x80; /* switch off multiple track mode */

		hard_sectors = raw_cmd->length >> (7 + SIZECODE);
		end_sector = SECTOR + hard_sectors - 1;
#ifdef FLOPPY_SANITY_CHECK
		if(end_sector > SECT_PER_TRACK) {
			printk("too many sectors %d > %d\n",
			       end_sector, SECT_PER_TRACK);
			return;
		}
#endif
		SECT_PER_TRACK = end_sector; /* make sure SECT_PER_TRACK points
					      * to end of transfer */
	}
}

/*
 * Formulate a read/write request.
 * this routine decides where to load the data (directly to buffer, or to
 * tmp floppy area), how much data to load (the size of the buffer, the whole
 * track, or a single sector)
 * All floppy_track_buffer handling goes in here. If we ever add track buffer
 * allocation on the fly, it should be done here. No other part should need
 * modification.
 */

static int make_raw_rw_request(void)
{
	int aligned_sector_t;
	int max_sector, max_size, tracksize, ssize;
	

	if(max_buffer_sectors == 0) {
		printk("VFS: Block I/O scheduled on unopened device\n");
		return 0;
	}
	

	set_fdc(DRIVE(CURRENT->rq_dev));

	raw_cmd = &default_raw_cmd;
	raw_cmd->flags = FD_RAW_SPIN | FD_RAW_NEED_DISK | FD_RAW_NEED_DISK |
		FD_RAW_NEED_SEEK;
	raw_cmd->cmd_count = NR_RW;
	if (CURRENT->cmd == READ){
		raw_cmd->flags |= FD_RAW_READ;
		COMMAND = FM_MODE(_floppy,FD_READ);
	} else if (CURRENT->cmd == WRITE){
		raw_cmd->flags |= FD_RAW_WRITE;
		COMMAND = FM_MODE(_floppy,FD_WRITE);
	} else {
		DPRINT("make_raw_rw_request: unknown command\n");
		return 0;
	}

	max_sector = _floppy->sect * _floppy->head;

	TRACK = CURRENT->sector / max_sector;
	sector_t = CURRENT->sector % max_sector;
	if (_floppy->track && TRACK >= _floppy->track) {
		if (CURRENT->current_nr_sectors & 1) {
			current_count_sectors = 1;
			return 1;
		} else
			return 0;
	}
	HEAD = sector_t / _floppy->sect;

	if (((_floppy->stretch & FD_SWAPSIDES) || TESTF(FD_NEED_TWADDLE)) &&
	    sector_t < _floppy->sect)
		max_sector = _floppy->sect;

	/* 2M disks have phantom sectors on the first track */
	if ((_floppy->rate & FD_2M) && (!TRACK) && (!HEAD)){
		max_sector = 2 * _floppy->sect / 3;
		if (sector_t >= max_sector){
			current_count_sectors = minimum(_floppy->sect - sector_t,
							CURRENT->nr_sectors);
			return 1;
		}
		SIZECODE = 2;
	} else
		SIZECODE = FD_SIZECODE(_floppy);
	raw_cmd->rate = _floppy->rate & 0x43;
	if ((_floppy->rate & FD_2M) &&
	    (TRACK || HEAD) &&
	    raw_cmd->rate == 2)
		raw_cmd->rate = 1;

	if (SIZECODE)
		SIZECODE2 = 0xff;
	else
		SIZECODE2 = 0x80;
	raw_cmd->track = TRACK << STRETCH(_floppy);
	DR_SELECT = UNIT(current_drive) + PH_HEAD(_floppy,HEAD);
	GAP = _floppy->gap;
	CODE2SIZE;
	SECT_PER_TRACK = _floppy->sect << 2 >> SIZECODE;
	SECTOR = ((sector_t % _floppy->sect) << 2 >> SIZECODE) + 1;

	/* tracksize describes the size which can be filled up with sectors
	 * of size ssize.
	 */
	tracksize = _floppy->sect - _floppy->sect % ssize;
	if (tracksize < _floppy->sect){
		SECT_PER_TRACK ++;
		if (tracksize <= sector_t % _floppy->sect)
			SECTOR--;

		/* if we are beyond tracksize, fill up using smaller sectors */
		while (tracksize <= sector_t % _floppy->sect){
			while(tracksize + ssize > _floppy->sect){
				SIZECODE--;
				ssize >>= 1;
			}
			SECTOR++; SECT_PER_TRACK ++;
			tracksize += ssize;
		}
		max_sector = HEAD * _floppy->sect + tracksize;
	} else if (!TRACK && !HEAD && !(_floppy->rate & FD_2M) && probing) {
		max_sector = _floppy->sect;
	} else if (!HEAD && CT(COMMAND) == FD_WRITE) {
		/* for virtual DMA bug workaround */
		max_sector = _floppy->sect;
	}

	in_sector_offset = (sector_t % _floppy->sect) % ssize;
	aligned_sector_t = sector_t - in_sector_offset;
	max_size = CURRENT->nr_sectors;
	if ((raw_cmd->track == buffer_track) && 
	    (current_drive == buffer_drive) &&
	    (sector_t >= buffer_min) && (sector_t < buffer_max)) {
		/* data already in track buffer */
		if (CT(COMMAND) == FD_READ) {
			copy_buffer(1, max_sector, buffer_max);
			return 1;
		}
	} else if (in_sector_offset || CURRENT->nr_sectors < ssize){
		if (CT(COMMAND) == FD_WRITE){
			if (sector_t + CURRENT->nr_sectors > ssize &&
			    sector_t + CURRENT->nr_sectors < ssize + ssize)
				max_size = ssize + ssize;
			else
				max_size = ssize;
		}
		raw_cmd->flags &= ~FD_RAW_WRITE;
		raw_cmd->flags |= FD_RAW_READ;
		COMMAND = FM_MODE(_floppy,FD_READ);
	} else if ((unsigned long)CURRENT->buffer < MAX_DMA_ADDRESS) {
		unsigned long dma_limit;
		int direct, indirect;

		indirect= transfer_size(ssize,max_sector,max_buffer_sectors*2) -
			sector_t;

		/*
		 * Do NOT use minimum() here---MAX_DMA_ADDRESS is 64 bits wide
		 * on a 64 bit machine!
		 */
		max_size = buffer_chain_size();
		dma_limit = (MAX_DMA_ADDRESS - ((unsigned long) CURRENT->buffer)) >> 9;
		if ((unsigned long) max_size > dma_limit) {
			max_size = dma_limit;
		}
		/* 64 kb boundaries */
		if (CROSS_64KB(CURRENT->buffer, max_size << 9))
			max_size = (K_64 - 
				    ((unsigned long)CURRENT->buffer) % K_64)>>9;
#ifndef PMON
		direct = transfer_size(ssize,max_sector,max_size) - sector_t;
#else
        direct=0;
#endif
		/*
		 * We try to read tracks, but if we get too many errors, we
		 * go back to reading just one sector at a time.
		 *
		 * This means we should be able to read a sector even if there
		 * are other bad sectors on this track.
		 */
		if (!direct ||
		    (indirect * 2 > direct * 3 &&
		     *errors < DP->max_errors.read_track &&
		     /*!TESTF(FD_NEED_TWADDLE) &&*/
		     ((!probing || (DP->read_track&(1<<DRS->probed_format)))))){
			max_size = CURRENT->nr_sectors;
		} else {
			raw_cmd->kernel_data = CURRENT->buffer;
			raw_cmd->length = current_count_sectors << 9;
			if (raw_cmd->length == 0){
				DPRINT("zero dma transfer attempted from make_raw_request\n");
				DPRINT("indirect=%d direct=%d sector_t=%d",
					indirect, direct, sector_t);
				return 0;
			}
/*			check_dma_crossing(raw_cmd->kernel_data, 
					   raw_cmd->length, 
					   "end of make_raw_request [1]");*/

			virtualdmabug_workaround();
			return 2;
		}
	}

	if (CT(COMMAND) == FD_READ)
		max_size = max_sector; /* unbounded */

	/* claim buffer track if needed */
	if (buffer_track != raw_cmd->track ||  /* bad track */
	    buffer_drive !=current_drive || /* bad drive */
	    sector_t > buffer_max ||
	    sector_t < buffer_min ||
	    ((CT(COMMAND) == FD_READ ||
	      (!in_sector_offset && CURRENT->nr_sectors >= ssize))&&
	     max_sector > 2 * max_buffer_sectors + buffer_min &&
	     max_size + sector_t > 2 * max_buffer_sectors + buffer_min)
	    /* not enough space */){
		buffer_track = -1;
		buffer_drive = current_drive;
		buffer_max = buffer_min = aligned_sector_t;
	}
	raw_cmd->kernel_data = floppy_track_buffer + 
		((aligned_sector_t-buffer_min)<<9);

	if (CT(COMMAND) == FD_WRITE){
		/* copy write buffer to track buffer.
		 * if we get here, we know that the write
		 * is either aligned or the data already in the buffer
		 * (buffer will be overwritten) */
#ifdef FLOPPY_SANITY_CHECK
		if (in_sector_offset && buffer_track == -1)
			DPRINT("internal error offset !=0 on write\n");
#endif
		buffer_track = raw_cmd->track;
		buffer_drive = current_drive;
		copy_buffer(ssize, max_sector, 2*max_buffer_sectors+buffer_min);
	} else
		transfer_size(ssize, max_sector,
			      2*max_buffer_sectors+buffer_min-aligned_sector_t);

	/* round up current_count_sectors to get dma xfer size */
	raw_cmd->length = in_sector_offset+current_count_sectors;
	raw_cmd->length = ((raw_cmd->length -1)|(ssize-1))+1;
	raw_cmd->length <<= 9;
#ifdef FLOPPY_SANITY_CHECK
	/*check_dma_crossing(raw_cmd->kernel_data, raw_cmd->length, 
	  "end of make_raw_request");*/
	if ((raw_cmd->length < current_count_sectors << 9) ||
	    (raw_cmd->kernel_data != CURRENT->buffer &&
	     CT(COMMAND) == FD_WRITE &&
	     (aligned_sector_t + (raw_cmd->length >> 9) > buffer_max ||
	      aligned_sector_t < buffer_min)) ||
	    raw_cmd->length % (128 << SIZECODE) ||
	    raw_cmd->length <= 0 || current_count_sectors <= 0){
		DPRINT("fractionary current count b=%lx s=%lx\n",
			raw_cmd->length, current_count_sectors);
		if (raw_cmd->kernel_data != CURRENT->buffer)
			printk("addr=%d, length=%ld\n",
			       (int) ((raw_cmd->kernel_data - 
				       floppy_track_buffer) >> 9),
			       current_count_sectors);
		printk("st=%d ast=%d mse=%d msi=%d\n",
		       sector_t, aligned_sector_t, max_sector, max_size);
		printk("ssize=%x SIZECODE=%d\n", ssize, SIZECODE);
		printk("command=%x SECTOR=%d HEAD=%d, TRACK=%d\n",
		       COMMAND, SECTOR, HEAD, TRACK);
		printk("buffer drive=%d\n", buffer_drive);
		printk("buffer track=%d\n", buffer_track);
		printk("buffer_min=%d\n", buffer_min);
		printk("buffer_max=%d\n", buffer_max);
		return 0;
	}

	if (raw_cmd->kernel_data != CURRENT->buffer){
		if (raw_cmd->kernel_data < floppy_track_buffer ||
		    current_count_sectors < 0 ||
		    raw_cmd->length < 0 ||
		    raw_cmd->kernel_data + raw_cmd->length >
		    floppy_track_buffer + (max_buffer_sectors  << 10)){
			DPRINT("buffer overrun in schedule dma\n");
			printk("sector_t=%d buffer_min=%d current_count=%ld\n",
			       sector_t, buffer_min,
			       raw_cmd->length >> 9);
			printk("current_count_sectors=%ld\n",
			       current_count_sectors);
			if (CT(COMMAND) == FD_READ)
				printk("read\n");
			if (CT(COMMAND) == FD_READ)
				printk("write\n");
			return 0;
		}
	} else if (raw_cmd->length > CURRENT->nr_sectors << 9 ||
		   current_count_sectors > CURRENT->nr_sectors){
		DPRINT("buffer overrun in direct transfer\n");
		return 0;
	} else if (raw_cmd->length < current_count_sectors << 9){
		DPRINT("more sectors than bytes\n");
		printk("bytes=%ld\n", raw_cmd->length >> 9);
		printk("sectors=%ld\n", current_count_sectors);
	}
	if (raw_cmd->length == 0){
		DPRINT("zero dma transfer attempted from make_raw_request\n");
		return 0;
	}
#endif

	virtualdmabug_workaround();
	return 2;
}

static void redo_fd_request(void)
{
#define REPEAT {request_done(0); continue; }
	kdev_t device;
	int tmp;

	lastredo = ticks;
	if (current_drive < N_DRIVE)
		floppy_off(current_drive);


	while(1){
		if (QUEUE_EMPTY) {
			CLEAR_INTR;
			unlock_fdc();
			return;
		}
#if 0
		if (MAJOR(CURRENT->rq_dev) != MAJOR_NR)
			panic(DEVICE_NAME ": request list destroyed");
		if (CURRENT->bh && !buffer_locked(CURRENT->bh))
			panic(DEVICE_NAME ": block not locked");
#endif

		device = CURRENT->rq_dev;
		set_fdc(DRIVE(device));
		reschedule_timeout(CURRENTD, "redo fd request", 0);

		set_floppy(device);
		raw_cmd = & default_raw_cmd;
		raw_cmd->flags = 0;
		if (start_motor(redo_fd_request)) return;
		disk_change(current_drive);
		if (test_bit(current_drive, &fake_change) ||
		   TESTF(FD_DISK_CHANGED)){
			DPRINT("disk absent or changed during operation\n");
			REPEAT;
		}
		if (!_floppy) { /* Autodetection */
			if (!probing){
				DRS->probed_format = 0;
				if (next_valid_format()){
					DPRINT("no autodetectable formats\n");
					_floppy = NULL;
					REPEAT;
				}
			}
			probing = 1;
			_floppy = floppy_type+DP->autodetect[DRS->probed_format];
		} else
			probing = 0;
		errors = & (CURRENT->errors);
		tmp = make_raw_rw_request();

		if (tmp < 2){
			request_done(tmp);
			continue;
		}

		if (TESTF(FD_NEED_TWADDLE))
			twaddle();
		schedule_bh( (void *)(void *) floppy_start);
#ifdef DEBUGT
		debugt("queue fd request");
#endif
		return;
	}
#undef REPEAT
}

static struct cont_t rw_cont={
	rw_interrupt,
	redo_fd_request,
	bad_flp_intr,
	request_done };

static void process_fd_request(void)
{
	cont = &rw_cont;
	schedule_bh( (void *)(void *) redo_fd_request);
}

static void do_fd_request(request_queue_t * q)
{
	if(max_buffer_sectors == 0) {
		printk("VFS: do_fd_request called on non-open device\n");
		return;
	}

	if (usage_count == 0) {
		printk("warning: usage count=0, CURRENT=%p exiting\n", CURRENT);
		printk("sect=%ld cmd=%d\n", CURRENT->sector, CURRENT->cmd);
		return;
	}
	if (fdc_busy){
		/* fdc busy, this new request will be treated when the
		   current one is done */
		is_alive("do fd request, old request running");
		return;
	}
	lock_fdc(MAXTIMEOUT,0);
	process_fd_request();
	is_alive("do fd request");
}

/*
 * User triggered reset
 * ====================
 */

static void reset_intr(void)
{
	printk("weird, reset interrupt called\n");
}

static struct cont_t reset_cont={
	reset_intr,
	success_and_wakeup,
	generic_failure,
	generic_done };

static int user_reset_fdc(int drive, int arg, int interruptible)
{
	int ret;

	ret=0;
	LOCK_FDC(drive,interruptible);
	if (arg == FD_RESET_ALWAYS)
		FDCS->reset=1;
	if (FDCS->reset){
		cont = &reset_cont;
		WAIT(reset_fdc);
	}
	process_fd_request();
	return ret;
}


static void __init config_types(void)
{
	int first=1;
	int drive;

	/* read drive info out of physical CMOS */
	drive=0;
	if (!UDP->cmos)
		UDP->cmos = FLOPPY0_TYPE;
	drive=1;
	if (!UDP->cmos && FLOPPY1_TYPE)
		UDP->cmos = FLOPPY1_TYPE;

	/* XXX */
	/* additional physical CMOS drive detection should go here */

	for (drive=0; drive < N_DRIVE; drive++){
		unsigned int type = UDP->cmos;
		struct floppy_drive_params *params;
		const char *name = NULL;
		static char temparea[32];

		if (type < NUMBER(default_drive_params)) {
			params = &default_drive_params[type].params;
			if (type) {
				name = default_drive_params[type].name;
				allowed_drive_mask |= 1 << drive;
			}
		} else {
			params = &default_drive_params[0].params;
			sprintf(temparea, "unknown type %d (usb?)", type);
			name = temparea;
		}
		if (name) {
			const char * prepend = ",";
			if (first) {
				prepend = KERN_INFO "Floppy drive(s):";
				first = 0;
			}
			printk("%s fd%d is %s", prepend, drive, name);
		}
		*UDP = *params;
#ifdef DEBUGT
		UDP->flags |= FD_DEBUG;
#endif
	}
	if (!first)
		printk("\n");
}



/*
 * Floppy Driver initialization
 * =============================
 */

/* Determine the floppy disk controller type */
/* This routine was written by David C. Niemi */
static char __init get_fdc_version(void)
{
	int r;

	output_byte(FD_DUMPREGS);	/* 82072 and better know DUMPREGS */
	if (FDCS->reset)
		return FDC_NONE;
	if ((r = result()) <= 0x00)
		return FDC_NONE;	/* No FDC present ??? */
	if ((r==1) && (reply_buffer[0] == 0x80)){
		printk(KERN_INFO "FDC %d is an 8272A\n",fdc);
		return FDC_8272A;	/* 8272a/765 don't know DUMPREGS */
	}
	if (r != 10) {
		printk("FDC %d init: DUMPREGS: unexpected return of %d bytes.\n",
		       fdc, r);
		return FDC_UNKNOWN;
	}

	if (!fdc_configure()) {
		printk(KERN_INFO "FDC %d is an 82072\n",fdc);
		return FDC_82072;      	/* 82072 doesn't know CONFIGURE */
	}

	output_byte(FD_PERPENDICULAR);
	if (need_more_output() == MORE_OUTPUT) {
		output_byte(0);
	} else {
		printk(KERN_INFO "FDC %d is an 82072A\n", fdc);
		return FDC_82072A;	/* 82072A as found on Sparcs. */
	}

	output_byte(FD_UNLOCK);
	r = result();
	if ((r == 1) && (reply_buffer[0] == 0x80)){
		printk(KERN_INFO "FDC %d is a pre-1991 82077\n", fdc);
		return FDC_82077_ORIG;	/* Pre-1991 82077, doesn't know 
					 * LOCK/UNLOCK */
	}
	if ((r != 1) || (reply_buffer[0] != 0x00)) {
		printk("FDC %d init: UNLOCK: unexpected return of %d bytes.\n",
		       fdc, r);
		return FDC_UNKNOWN;
	}
	output_byte(FD_PARTID);
	r = result();
	if (r != 1) {
		printk("FDC %d init: PARTID: unexpected return of %d bytes.\n",
		       fdc, r);
		return FDC_UNKNOWN;
	}
	if (reply_buffer[0] == 0x80) {
		printk(KERN_INFO "FDC %d is a post-1991 82077\n",fdc);
		return FDC_82077;	/* Revised 82077AA passes all the tests */
	}
	switch (reply_buffer[0] >> 5) {
		case 0x0:
			/* Either a 82078-1 or a 82078SL running at 5Volt */
			printk(KERN_INFO "FDC %d is an 82078.\n",fdc);
			return FDC_82078;
		case 0x1:
			printk(KERN_INFO "FDC %d is a 44pin 82078\n",fdc);
			return FDC_82078;
		case 0x2:
			printk(KERN_INFO "FDC %d is a S82078B\n", fdc);
			return FDC_S82078B;
		case 0x3:
			printk(KERN_INFO "FDC %d is a National Semiconductor PC87306\n", fdc);
			return FDC_87306;
		default:
			printk(KERN_INFO "FDC %d init: 82078 variant with unknown PARTID=%d.\n",
			       fdc, reply_buffer[0] >> 5);
			return FDC_82078_UNKN;
	}
} /* get_fdc_version */


static int have_no_fdc= -ENODEV;


int __init floppy_init(void)
{
	int i,unit,drive;

	raw_cmd = NULL;


	for (i=0; i<256; i++)
	{
		if (ITYPE(i))
			floppy_sizes[i] = (floppy_type[ITYPE(i)].size+1) >> 1;
		else
			floppy_sizes[i] = MAX_DISK_SIZE;
        //floppy_blocksizes[i] = 512;
        //floppy_maxsectors[i] = 64;
	}
#if 0
	blk_size[MAJOR_NR] = floppy_sizes;
	blksize_size[MAJOR_NR] = floppy_blocksizes;
	blk_init_queue(BLK_DEFAULT_QUEUE(MAJOR_NR), DEVICE_REQUEST);
#endif
	reschedule_timeout(MAXTIMEOUT, "floppy init", MAXTIMEOUT);
	config_types();

	for (i = 0; i < N_FDC; i++) {
		fdc = i;
		CLEARSTRUCT(FDCS);
		FDCS->dtr = -1;
		FDCS->dor = 0x4;
#if defined(__sparc__) || defined(__mc68000__)
		/*sparcs/sun3x don't have a DOR reset which we can fall back on to*/
#ifdef __mc68000__
		if(MACH_IS_SUN3X)
#endif
			FDCS->version = FDC_82072A;		
#endif
	}

	use_virtual_dma = can_use_virtual_dma & 1;
	fdc_state[0].address = FDC1;
	if (fdc_state[0].address == -1) {
		del_timer(&fd_timeout);
		//blk_cleanup_queue(BLK_DEFAULT_QUEUE(MAJOR_NR));
		return -ENODEV;
	}
#if N_FDC > 1
	fdc_state[1].address = FDC2;
#endif

	fdc = 0; /* reset fdc in case of unexpected interrupt */
	if (floppy_grab_irq_and_dma()){
		del_timer(&fd_timeout);
		blk_cleanup_queue(BLK_DEFAULT_QUEUE(MAJOR_NR));
		return -EBUSY;
	}

	/* initialise drive state */
	for (drive = 0; drive < N_DRIVE; drive++) {
		CLEARSTRUCT(UDRS);
		CLEARSTRUCT(UDRWE);
		USETF(FD_DISK_NEWCHANGE);
		USETF(FD_DISK_CHANGED);
		USETF(FD_VERIFY);
		UDRS->fd_device = -1;
		floppy_track_buffer = NULL;
		max_buffer_sectors = 0;
	}

	for (i = 0; i < N_FDC; i++) {
		fdc = i;
		FDCS->driver_version = FD_DRIVER_VERSION;
		for (unit=0; unit<4; unit++)
			FDCS->track[unit] = 0;
		if (FDCS->address == -1)
			continue;
		FDCS->rawcmd = 2;
		if (user_reset_fdc(-1,FD_RESET_ALWAYS,0)){
 			/* free ioports reserved by floppy_grab_irq_and_dma() */
 			release_region(FDCS->address+2, 4);
 			release_region(FDCS->address+7, 1);
			FDCS->address = -1;
			FDCS->version = FDC_NONE;
			continue;
		}
		/* Try to determine the floppy controller type */
		FDCS->version = get_fdc_version();
		if (FDCS->version == FDC_NONE){
 			/* free ioports reserved by floppy_grab_irq_and_dma() */
 			release_region(FDCS->address+2, 4);
 			release_region(FDCS->address+7, 1);
			FDCS->address = -1;
			continue;
		}
		if (can_use_virtual_dma == 2 && FDCS->version < FDC_82072A)
			can_use_virtual_dma = 0;

		have_no_fdc = 0;
		/* Not all FDCs seem to be able to handle the version command
		 * properly, so force a reset for the standard FDC clones,
		 * to avoid interrupt garbage.
		 */
		user_reset_fdc(-1,FD_RESET_ALWAYS,0);
	}
	fdc=0;
	del_timer(&fd_timeout);
	current_drive = 0;
	floppy_release_irq_and_dma();
	initialising=0;
	if (have_no_fdc) 
	{
		DPRINT("no floppy controllers found\n");
		run_task_queue(&tq_immediate);
		if (usage_count)
			floppy_release_irq_and_dma();
		blk_cleanup_queue(BLK_DEFAULT_QUEUE(MAJOR_NR));
	}
	
	for (drive = 0; drive < N_DRIVE; drive++) {
		motor_off_timer[drive].data = drive;
		motor_off_timer[drive].function = motor_off_callback;
		if (!(allowed_drive_mask & (1 << drive)))
			continue;
		if (fdc_state[FDC(drive)].version == FDC_NONE)
			continue;
#if 0
		for (i = 0; i<NUMBER(floppy_type); i++)
			register_disk(NULL, MKDEV(MAJOR_NR,TOMINOR(drive)+i*4),
					1, &floppy_fops, 0);
#endif
	}

	return have_no_fdc;
}

static spinlock_t floppy_usage_lock = SPIN_LOCK_UNLOCKED;

static int floppy_grab_irq_and_dma(void)
{
	unsigned long flags;

	spin_lock_irqsave(&floppy_usage_lock, flags);
	if (usage_count++){
		spin_unlock_irqrestore(&floppy_usage_lock, flags);
		return 0;
	}
	spin_unlock_irqrestore(&floppy_usage_lock, flags);
	MOD_INC_USE_COUNT;
	if (fd_request_irq()) {
		DPRINT("Unable to grab IRQ%d for the floppy driver\n",
			FLOPPY_IRQ);
		MOD_DEC_USE_COUNT;
		spin_lock_irqsave(&floppy_usage_lock, flags);
		usage_count--;
		spin_unlock_irqrestore(&floppy_usage_lock, flags);
		return -1;
	}
	if (fd_request_dma()) {
		DPRINT("Unable to grab DMA%d for the floppy driver\n",
			FLOPPY_DMA);
		fd_free_irq();
		MOD_DEC_USE_COUNT;
		spin_lock_irqsave(&floppy_usage_lock, flags);
		usage_count--;
		spin_unlock_irqrestore(&floppy_usage_lock, flags);
		return -1;
	}

	for (fdc=0; fdc< N_FDC; fdc++){
		if (FDCS->address != -1){
			if (!request_region(FDCS->address+2, 4, "floppy")) {
				DPRINT("Floppy io-port 0x%04lx in use\n", FDCS->address + 2);
				goto cleanup1;
			}
			if (!request_region(FDCS->address+7, 1, "floppy DIR")) {
				DPRINT("Floppy io-port 0x%04lx in use\n", FDCS->address + 7);
				goto cleanup2;
			}
			/* address + 6 is reserved, and may be taken by IDE.
			 * Unfortunately, Adaptec doesn't know this :-(, */
		}
	}
	for (fdc=0; fdc< N_FDC; fdc++){
		if (FDCS->address != -1){
			reset_fdc_info(1);
			fd_outb(FDCS->dor, FD_DOR);
		}
	}
	fdc = 0;
	set_dor(0, ~0, 8);  /* avoid immediate interrupt */

	for (fdc = 0; fdc < N_FDC; fdc++)
		if (FDCS->address != -1)
			fd_outb(FDCS->dor, FD_DOR);
	/*
	 *	The driver will try and free resources and relies on us
	 *	to know if they were allocated or not.
	 */
	fdc = 0;
	irqdma_allocated = 1;
	return 0;
cleanup2:
	release_region(FDCS->address + 2, 4);
cleanup1:
	fd_free_irq();
	fd_free_dma();
	while(--fdc >= 0) {
		release_region(FDCS->address + 2, 4);
		release_region(FDCS->address + 7, 1);
	}
	MOD_DEC_USE_COUNT;
	spin_lock_irqsave(&floppy_usage_lock, flags);
	usage_count--;
	spin_unlock_irqrestore(&floppy_usage_lock, flags);
	return -1;
}

static void floppy_release_irq_and_dma(void)
{
	int old_fdc;
#ifdef FLOPPY_SANITY_CHECK
#ifndef __sparc__
	int drive;
#endif
#endif
	long tmpsize;
	unsigned long tmpaddr;
	unsigned long flags;

	spin_lock_irqsave(&floppy_usage_lock, flags);
	if (--usage_count){
		spin_unlock_irqrestore(&floppy_usage_lock, flags);
		return;
	}
	spin_unlock_irqrestore(&floppy_usage_lock, flags);
	if(irqdma_allocated)
	{
		fd_disable_dma();
		fd_free_dma();
		fd_free_irq();
		irqdma_allocated=0;
	}
	set_dor(0, ~0, 8);
#if N_FDC > 1
	set_dor(1, ~8, 0);
#endif
	floppy_enable_hlt();

	if (floppy_track_buffer && max_buffer_sectors) {
		tmpsize = max_buffer_sectors*1024;
		tmpaddr = (unsigned long)floppy_track_buffer;
		floppy_track_buffer = NULL;
		max_buffer_sectors = 0;
		buffer_min = buffer_max = -1;
		fd_dma_mem_free(tmpaddr, tmpsize);
	}

#ifdef FLOPPY_SANITY_CHECK
#ifndef __sparc__
	for (drive=0; drive < N_FDC * 4; drive++)
		if (timer_pending(motor_off_timer + drive))
			printk("motor off timer %d still active\n", drive);
#endif

	if (timer_pending(&fd_timeout))
		printk("floppy timer still active:%s\n", timeout_message);
	if (timer_pending(&fd_timer))
		printk("auxiliary floppy timer still active\n");
	if (floppy_tq.sync)
		printk("task queue still active\n");
#endif
	old_fdc = fdc;
	for (fdc = 0; fdc < N_FDC; fdc++)
		if (FDCS->address != -1) {
			release_region(FDCS->address+2, 4);
			release_region(FDCS->address+7, 1);
		}
	fdc = old_fdc;
}

//---------------------------------------------------------------------------------------------

static int check_disk_change(kdev_t dev)
{
	if(!check_floppy_change(dev))return 0;

	floppy_revalidate(dev);
    return 1;
}

#define RETERR(x) do{floppy_release(inode); return -(x);}while(0)
static int floppy_open(struct inode * inode)
{
	int drive;
	int old_dev;
	int try;
	char *tmp;


	drive = DRIVE(inode->i_rdev);
	if (drive >= N_DRIVE ||
	    !(allowed_drive_mask & (1 << drive)) ||
	    fdc_state[FDC(drive)].version == FDC_NONE)
		return -ENXIO;

	if (TYPE(inode->i_rdev) >= NUMBER(floppy_type))
		return -ENXIO;
	old_dev = UDRS->fd_device;
	if (UDRS->fd_ref && old_dev != MINOR(inode->i_rdev))
		return -EBUSY;

	if (!UDRS->fd_ref && (UDP->flags & FD_BROKEN_DCL)){
		USETF(FD_DISK_CHANGED);
		USETF(FD_VERIFY);
	}

	if (UDRS->fd_ref == -1 )
		return -EBUSY;

	if (floppy_grab_irq_and_dma())
		return -EBUSY;

		UDRS->fd_ref++;

	if (!floppy_track_buffer){
		/* if opening an ED drive, reserve a big buffer,
		 * else reserve a small one */
		if ((UDP->cmos == 6) || (UDP->cmos == 5))
			try = 64; /* Only 48 actually useful */
		else
			try = 32; /* Only 24 actually useful */

		tmp=(char *)fd_dma_mem_alloc(1024 * try);
		if (!tmp && !floppy_track_buffer) {
			try >>= 1; /* buffer only one side */
			INFBOUND(try, 16);
			tmp= (char *)fd_dma_mem_alloc(1024*try);
		}
		if (!tmp && !floppy_track_buffer) {
			fallback_on_nodma_alloc(&tmp, 2048 * try);
		}
		if (!tmp && !floppy_track_buffer) {
			DPRINT("Unable to allocate DMA memory\n");
			RETERR(ENXIO);
		}
		if (floppy_track_buffer) {
			if (tmp)
				fd_dma_mem_free((unsigned long)tmp,try*1024);
		} else {
			buffer_min = buffer_max = -1;
			floppy_track_buffer = tmp;
			max_buffer_sectors = try;
		}
	}

	UDRS->fd_device = MINOR(inode->i_rdev);
	if (old_dev != -1 && old_dev != MINOR(inode->i_rdev)) {
		if (buffer_drive == drive)
			buffer_track = -1;
	//	invalidate_buffers(MKDEV(FLOPPY_MAJOR,old_dev));
	}


	if (UFDCS->rawcmd == 1)
		UFDCS->rawcmd = 2;

	if (1) {
		UDRS->last_checked = 0;
		check_disk_change(inode->i_rdev);
		if (UTESTF(FD_DISK_CHANGED))
			RETERR(ENXIO);
	}
	return 0;
#undef RETERR
}

static int floppy_release(struct inode * inode)
{
	int drive = DRIVE(inode->i_rdev);

	if (UDRS->fd_ref < 0)
		UDRS->fd_ref=0;
	else if (!UDRS->fd_ref--) {
		DPRINT("floppy_release with fd_ref == 0");
		UDRS->fd_ref = 0;
	}
	floppy_release_irq_and_dma();
	return 0;
}

static struct cont_t poll_cont={
	success_and_wakeup,
	floppy_ready,
	generic_failure,
	generic_done };

static int poll_drive(int interruptible, int flag)
{
	int ret;
	/* no auto-sense, just clear dcl */
	raw_cmd = &default_raw_cmd;
	raw_cmd->flags= flag;
	raw_cmd->track=0;
	raw_cmd->cmd_count=0;
	cont = &poll_cont;
#ifdef DCL_DEBUG
	if (DP->flags & FD_DEBUG){
		DPRINT("setting NEWCHANGE in poll_drive\n");
	}
#endif
	SETF(FD_DISK_NEWCHANGE);
	WAIT(floppy_ready);
	return ret;
}

static int floppy_revalidate(kdev_t dev)
{
#define NO_GEOM (!current_type[drive] && !TYPE(dev))
//	struct buffer_head * bh;
	int drive=DRIVE(dev);
	int cf;

	if (UTESTF(FD_DISK_CHANGED) ||
	    UTESTF(FD_VERIFY) ||
	    test_bit(drive, &fake_change) ||
	    NO_GEOM){
		if(usage_count == 0) {
			printk("VFS: revalidate called on non-open device.\n");
			return -EFAULT;
		}
		lock_fdc(drive,0);
		cf = UTESTF(FD_DISK_CHANGED) || UTESTF(FD_VERIFY);
		if (!(cf || test_bit(drive, &fake_change) || NO_GEOM)){
			process_fd_request(); /*already done by another thread*/
			return 0;
		}
		UDRS->maxblock = 0;
		UDRS->maxtrack = 0;
		if (buffer_drive == drive)
			buffer_track = -1;
		clear_bit(drive, &fake_change);
		UCLEARF(FD_DISK_CHANGED);
		if (cf)
			UDRS->generation++;
		if (NO_GEOM){
			/* auto-sensing */
#if 0
			int size = floppy_blocksizes[MINOR(dev)];
			if (!size)
				size = 512;
			if (!(bh = getblk(dev,0,size))){
				process_fd_request();
				return -ENXIO;
			}
			if (bh && !buffer_uptodate(bh))
				ll_rw_block(READ, 1, &bh);
			process_fd_request();
			wait_on_buffer(bh);
			brelse(bh);
#endif
			return 0;
		}
		if (cf)
			poll_drive(0, FD_RAW_NEED_DISK);
		process_fd_request();
	}
	return 0;
}

/*
 * Check if the disk has been changed or if a change has been faked.
 */
static int check_floppy_change(kdev_t dev)
{
	int drive = DRIVE(dev);


	if (UTESTF(FD_DISK_CHANGED) || UTESTF(FD_VERIFY))
		return 1;

	if (UDP->checkfreq < (int)(ticks - UDRS->last_checked)) {
		if(floppy_grab_irq_and_dma()) {
			return 1;
		}

		lock_fdc(drive,0);
		poll_drive(0,0);
		process_fd_request();
		floppy_release_irq_and_dma();
	}

	if (UTESTF(FD_DISK_CHANGED) ||
	   UTESTF(FD_VERIFY) ||
	   test_bit(drive, &fake_change) ||
	   (!TYPE(dev) && !current_type[drive]))
		return 1;
	return 0;
}

void floppy_info()
{
config_types();
get_fdc_version();
}
