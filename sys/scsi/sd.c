/*	$OpenBSD: sd.c,v 1.32 1998/10/04 01:37:55 millert Exp $	*/
/*	$NetBSD: sd.c,v 1.111 1997/04/02 02:29:41 mycroft Exp $	*/

/*
 * Copyright (c) 1994, 1995, 1997 Charles M. Hannum.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Originally written by Julian Elischer (julian@dialix.oz.au)
 * for TRW Financial Systems for use under the MACH(2.5) operating system.
 *
 * TRW Financial Systems, in accordance with their agreement with Carnegie
 * Mellon University, makes this software available to CMU to distribute
 * or use in any manner that they see fit as long as this message is kept with
 * the software. For this reason TFS also grants any other persons or
 * organisations permission to use or modify this software.
 *
 * TFS supplies this software to be publicly redistributed
 * on the understanding that TFS is not responsible for the correct
 * functioning of this software in any circumstances.
 *
 * Ported to run under 386BSD by Julian Elischer (julian@dialix.oz.au) Sept 1992
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#if 0
#include <sys/mtio.h>
#endif
#include "queue.h"
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#if 0
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/device.h>
#include <sys/conf.h>
#else
#include <sys/proc.h>
#include <sys/device.h>
#endif
#include <sys/scsiio.h>

#include "scsi_all.h"
#include "scsi_disk.h"
#include "scsiconf.h"

extern void disksort __P((struct buf *, struct buf *));

#define	SDOUTSTANDING	4
#define	SDRETRIES	4

#define	SDUNIT(dev)			(dev & 0xff)

struct sd_softc {
	struct device sc_dev;
	struct disk {
		char	*dk_name;
		int	dk_openmask;
		int	dk_copenmask;
		int	dk_bopenmask;
	} sc_dk;

	int flags;
#define	SDF_LOCKED	0x01
#define	SDF_WANTED	0x02
#define	SDF_WLABEL	0x04		/* label is writable */
#define	SDF_LABELLING	0x08		/* writing label */
#define	SDF_ANCIENT	0x10		/* disk is ancient; for minphys */
	struct scsi_link *sc_link;	/* contains our targ, lun, etc. */
	struct disk_parms {
		u_char heads;		/* number of heads */
		u_short cyls;		/* number of cylinders */
		u_char sectors;		/* number of sectors/track */
		int blksize;		/* number of bytes/sector */
		u_long disksize;	/* total number sectors */
	} params;
#define sd_secsize params.blksize
	struct disk_name {
		char vendor[9];		/* disk vendor/manufacturer */
		char product[17];	/* disk product model */
		char revision[5];	/* drive/firmware revision */
	} name;
	struct buf buf_queue;
	u_int8_t type;
};

struct scsi_mode_sense_data {
	struct scsi_mode_header header;
	struct scsi_blk_desc blk_desc;
	union disk_pages pages;
};

int sdopen __P((dev_t dev, int flags, int mode, struct proc *));
int sdread __P((dev_t dev, struct uio *uio, int flag));
int sdwrite __P((dev_t dev, struct uio *uio, int flag));
int sdclose __P((dev_t dev, int flag, int mode, struct proc *)); 
void sdstrategy __P((struct buf *));

int	sdmatch __P((struct device *, void *, void *));
void	sdattach __P((struct device *, struct device *, void *));
void	sdstart __P((void *));
void	sddone __P((struct scsi_xfer *));
int	sd_get_optparms __P((struct sd_softc *, int, struct disk_parms *));
int	sd_get_parms __P((struct sd_softc *, int));
static	int sd_mode_sense __P((struct sd_softc *, struct scsi_mode_sense_data *,
			      int, int));
void	viscpy __P((u_char *, u_char *, int));

struct cfattach sd_ca = {
        sizeof(struct sd_softc), sdmatch, sdattach
};

struct cfdriver sd_cd = {
        NULL, "sd", DV_DISK
}; 

struct scsi_device sd_switch = {
	NULL,			/* Use default error handler */
	sdstart,		/* have a queue, served by this */
	NULL,			/* have no async handler */
	sddone,			/* deal with stats at interrupt time */
};

struct scsi_inquiry_pattern sd_patterns[] = {
	{T_DIRECT, T_FIXED,
	 "",         "",                 ""},
	{T_DIRECT, T_REMOV,
	 "",         "",                 ""},
	{T_OPTICAL, T_FIXED,
	 "",         "",                 ""},
	{T_OPTICAL, T_REMOV,
	 "",         "",                 ""},
};

int
sdmatch(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct scsibus_attach_args *sa = aux;
	int priority;

	(void)scsi_inqmatch(sa->sa_inqbuf,
	    (caddr_t)sd_patterns, sizeof(sd_patterns)/sizeof(sd_patterns[0]),
	    sizeof(sd_patterns[0]), &priority);
	return (priority);
}

/*
 * The routine called by the low level scsi routine when it discovers
 * a device suitable for this driver.
 */
void
sdattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	int error;
	struct sd_softc *sd = (void *)self;
	struct disk_parms *dp = &sd->params;
	struct scsibus_attach_args *sa = aux;
	struct scsi_link *sc_link = sa->sa_sc_link;

	SC_DEBUG(sc_link, SDEV_DB2, ("sdattach: "));

	/*
	 * Store information needed to contact our base driver
	 */
	sd->sc_link = sc_link;
	sd->type = (sa->sa_inqbuf->device & SID_TYPE);
	sc_link->device = &sd_switch;
	sc_link->device_softc = sd;
	if (sc_link->openings > SDOUTSTANDING)
		sc_link->openings = SDOUTSTANDING;

	/*
	 * Initialize and attach the disk structure.
	 */
	sd->sc_dk.dk_name = sd->sc_dev.dv_xname;

	/*
	 * Use the subdriver to request information regarding
	 * the drive. We cannot use interrupts yet, so the
	 * request must specify this.
	 */
	printf("\n");
	printf("%s: ", sd->sc_dev.dv_xname);

	if ((sd->sc_link->quirks & SDEV_NOSTARTUNIT) == 0) {
		error = scsi_start(sd->sc_link, SSS_START,
				   SCSI_AUTOCONF | SCSI_IGNORE_ILLEGAL_REQUEST |
				   SCSI_IGNORE_MEDIA_CHANGE | SCSI_SILENT);
	} else
		error = 0;

	/* Fill in name struct for spoofed label */
	viscpy(sd->name.vendor, sa->sa_inqbuf->vendor, 8);
	viscpy(sd->name.product, sa->sa_inqbuf->product, 16);
	viscpy(sd->name.revision, sa->sa_inqbuf->revision, 4);

	if (error || sd_get_parms(sd, SCSI_AUTOCONF) != 0)
		printf("drive offline\n");
	else
	        printf("%ldMB, %d cyl, %d head, %d sec, %d bytes/sec, %ld sec total\n",
		    dp->disksize / (1048576 / dp->blksize), dp->cyls,
		    dp->heads, dp->sectors, dp->blksize, dp->disksize);
}

/*
 * open the device. Make sure the partition info is a up-to-date as can be.
 */
int
sdopen(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	struct sd_softc *sd;
	struct scsi_link *sc_link;
	int unit, part;
	int error;

	unit = SDUNIT(dev);
	if (unit >= sd_cd.cd_ndevs)
		return ENXIO;
	sd = sd_cd.cd_devs[unit];
	if (!sd)
		return ENXIO;

	sc_link = sd->sc_link;

	if (sd->sc_dk.dk_openmask != 0) {
		/*
		 * If any partition is open, but the disk has been invalidated,
		 * disallow further opens.
		 */
		if ((sc_link->flags & SDEV_MEDIA_LOADED) == 0) {
			error = EIO;
			goto bad3;
		}
	} else {
		/* Check that it is still responding and ok. */
		error = scsi_test_unit_ready(sc_link,
					     SCSI_IGNORE_ILLEGAL_REQUEST |
					     SCSI_IGNORE_MEDIA_CHANGE |
					     SCSI_IGNORE_NOT_READY);
		if (error)
			goto bad3;

		/* Start the pack spinning if necessary. */
		if ((sc_link->quirks & SDEV_NOSTARTUNIT) == 0) {
			error = scsi_start(sc_link, SSS_START,
					   SCSI_IGNORE_ILLEGAL_REQUEST |
					   SCSI_IGNORE_MEDIA_CHANGE |
					   SCSI_SILENT);
			if (error)
				goto bad3;
		}

		sc_link->flags |= SDEV_OPEN;

		/* Lock the pack in. */
		error = scsi_prevent(sc_link, PR_PREVENT,
				     SCSI_IGNORE_ILLEGAL_REQUEST |
				     SCSI_IGNORE_MEDIA_CHANGE);
		if (error)
			goto bad;

		if ((sc_link->flags & SDEV_MEDIA_LOADED) == 0) {
			sc_link->flags |= SDEV_MEDIA_LOADED;

			/* Load the physical device parameters. */
			if (sd_get_parms(sd, 0) != 0) {
				error = ENXIO;
				goto bad2;
			}
			SC_DEBUG(sc_link, SDEV_DB3, ("Params loaded "));
		}
	}

	/* Insure only one open at a time. */
	switch (fmt) {
	case S_IFCHR:
		sd->sc_dk.dk_copenmask |= (1 << part);
		break;
	case S_IFBLK:
		sd->sc_dk.dk_bopenmask |= (1 << part);
		break;
	}
	sd->sc_dk.dk_openmask = sd->sc_dk.dk_copenmask | sd->sc_dk.dk_bopenmask;

	SC_DEBUG(sc_link, SDEV_DB3, ("open complete\n"));
	return 0;

bad2:
	sc_link->flags &= ~SDEV_MEDIA_LOADED;

bad:
	if (sd->sc_dk.dk_openmask == 0) {
		scsi_prevent(sc_link, PR_ALLOW,
		    SCSI_IGNORE_ILLEGAL_REQUEST | SCSI_IGNORE_MEDIA_CHANGE);
		sc_link->flags &= ~SDEV_OPEN;
	}

bad3:
	return error;
}

/*
 * close the device.. only called if we are the LAST occurence of an open
 * device.  Convenient now but usually a pain.
 */
int 
sdclose(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	struct sd_softc *sd = sd_cd.cd_devs[SDUNIT(dev)];
	int part = 0;

	switch (fmt) {
	case S_IFCHR:
		sd->sc_dk.dk_copenmask &= ~(1 << part);
		break;
	case S_IFBLK:
		sd->sc_dk.dk_bopenmask &= ~(1 << part);
		break;
	}
	sd->sc_dk.dk_openmask = sd->sc_dk.dk_copenmask | sd->sc_dk.dk_bopenmask;

	if (sd->sc_dk.dk_openmask == 0) {
		/* XXX Must wait for I/O to complete! */

		scsi_prevent(sd->sc_link, PR_ALLOW,
		    SCSI_IGNORE_ILLEGAL_REQUEST | SCSI_IGNORE_NOT_READY);
		sd->sc_link->flags &= ~(SDEV_OPEN|SDEV_MEDIA_LOADED);

		if (sd->sc_link->flags & SDEV_EJECTING) {
		    scsi_start(sd->sc_link, SSS_STOP|SSS_LOEJ, 0);

		    sd->sc_link->flags &= ~SDEV_EJECTING;
		}
	}

	return 0;
}

/*
 * Actually translate the requested transfer into one the physical driver
 * can understand.  The transfer is described by a buf and will include
 * only one physical transfer.
 */
void
sdstrategy(bp)
	struct buf *bp;
{
	struct sd_softc *sd = sd_cd.cd_devs[SDUNIT(bp->b_dev)];
	int s;

	SC_DEBUG(sd->sc_link, SDEV_DB2, ("sdstrategy "));
	SC_DEBUG(sd->sc_link, SDEV_DB1,
	    ("%ld bytes @ blk %d\n", bp->b_bcount, bp->b_blkno));
	/*
	 * The transfer must be a whole number of blocks.
	 */
	if ((bp->b_bcount % sd->sd_secsize) != 0) {
		bp->b_error = EINVAL;
		goto bad;
	}
	/*
	 * If the device has been made invalid, error out
	 */
	if ((sd->sc_link->flags & SDEV_MEDIA_LOADED) == 0) {
		bp->b_error = EIO;
		goto bad;
	}
	/*
	 * If it's a null transfer, return immediatly
	 */
	if (bp->b_bcount == 0)
		goto done;

	s = splbio();

	/*
	 * Place it in the queue of disk activities for this disk
	 */
	disksort(&sd->buf_queue, bp);

	/*
	 * Tell the device to get going on the transfer if it's
	 * not doing anything, otherwise just wait for completion
	 */
	sdstart(sd);

	splx(s);
	return;

bad:
	bp->b_flags |= B_ERROR;
done:
	/*
	 * Correctly set the buf to indicate a completed xfer
	 */
	bp->b_resid = bp->b_bcount;
	biodone(bp);
}

/*
 * sdstart looks to see if there is a buf waiting for the device
 * and that the device is not already busy. If both are true,
 * It dequeues the buf and creates a scsi command to perform the
 * transfer in the buf. The transfer request will call scsi_done
 * on completion, which will in turn call this routine again
 * so that the next queued transfer is performed.
 * The bufs are queued by the strategy routine (sdstrategy)
 *
 * This routine is also called after other non-queued requests
 * have been made of the scsi driver, to ensure that the queue
 * continues to be drained.
 *
 * must be called at the correct (highish) spl level
 * sdstart() is called at splbio from sdstrategy and scsi_done
 */
void 
sdstart(v)
	register void *v;
{
	register struct sd_softc *sd = v;
	register struct	scsi_link *sc_link = sd->sc_link;
	struct buf *bp = 0;
	struct buf *dp;
	struct scsi_rw_big cmd_big;
	struct scsi_rw cmd_small;
	struct scsi_generic *cmdp;
	int blkno, nblks, cmdlen, error;

	SC_DEBUG(sc_link, SDEV_DB2, ("sdstart "));
	/*
	 * Check if the device has room for another command
	 */
	while (sc_link->openings > 0) {
		/*
		 * there is excess capacity, but a special waits
		 * It'll need the adapter as soon as we clear out of the
		 * way and let it run (user level wait).
		 */
		if (sc_link->flags & SDEV_WAITING) {
			sc_link->flags &= ~SDEV_WAITING;
			wakeup((caddr_t)sc_link);
			return;
		}

		/*
		 * See if there is a buf with work for us to do..
		 */
		dp = &sd->buf_queue;
		if ((bp = dp->b_actf) == NULL)	/* yes, an assign */
			return;
		dp->b_actf = bp->b_actf;

		/*
		 * If the device has become invalid, abort all the
		 * reads and writes until all files have been closed and
		 * re-opened
		 */
		if ((sc_link->flags & SDEV_MEDIA_LOADED) == 0) {
			bp->b_error = EIO;
			bp->b_flags |= B_ERROR;
			bp->b_resid = bp->b_bcount;
			biodone(bp);
			continue;
		}

		/*
		 * We have a buf, now we should make a command
		 *
		 * First, translate the block to absolute and put it in terms
		 * of the logical blocksize of the device.
		 */
		blkno = bp->b_blkno / (sd->sd_secsize / DEV_BSIZE);
		nblks = howmany(bp->b_bcount, sd->sd_secsize);

		/*
		 *  Fill out the scsi command.  If the transfer will
		 *  fit in a "small" cdb, use it.
		 */
		if (((blkno & 0x1fffff) == blkno) &&
		    ((nblks & 0xff) == nblks)) {
			/*
			 * We can fit in a small cdb.
			 */
			bzero(&cmd_small, sizeof(cmd_small));
			cmd_small.opcode = (bp->b_flags & B_READ) ?
			    READ_COMMAND : WRITE_COMMAND;
			_lto3b(blkno, cmd_small.addr);
			cmd_small.length = nblks & 0xff;
			cmdlen = sizeof(cmd_small);
			cmdp = (struct scsi_generic *)&cmd_small;
		} else {
			/*
			 * Need a large cdb.
			 */
			bzero(&cmd_big, sizeof(cmd_big));
			cmd_big.opcode = (bp->b_flags & B_READ) ?
			    READ_BIG : WRITE_BIG;
			_lto4b(blkno, cmd_big.addr);
			_lto2b(nblks, cmd_big.length);
			cmdlen = sizeof(cmd_big);
			cmdp = (struct scsi_generic *)&cmd_big;
		}

		/*
		 * Call the routine that chats with the adapter.
		 * Note: we cannot sleep as we may be an interrupt
		 */
		error = scsi_scsi_cmd(sc_link, cmdp, cmdlen,
		    (u_char *)bp->b_data, bp->b_bcount,
		    SDRETRIES, 60000, bp, SCSI_NOSLEEP |
		    ((bp->b_flags & B_READ) ? SCSI_DATA_IN : SCSI_DATA_OUT));
		if (error) {
			printf("%s: not queued, error %d\n",
			    sd->sc_dev.dv_xname, error);
		}
	}
}

void
sddone(xs)
	struct scsi_xfer *xs;
{
}

int
sdread(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{

	return (physio(sdstrategy, NULL, dev, B_READ, NULL, uio));
}

int
sdwrite(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{

	return (physio(sdstrategy, NULL, dev, B_WRITE, NULL, uio));
}

static int
sd_mode_sense(sd, scsi_sense, page, flags)
	struct sd_softc *sd;
	struct scsi_mode_sense_data *scsi_sense;
	int page, flags;
{
	struct scsi_mode_sense scsi_cmd;

	/*
	 * Make sure the sense buffer is clean before we do
	 * the mode sense, so that checks for bogus values of
	 * 0 will work in case the mode sense fails.
	 */
	bzero(scsi_sense, sizeof(*scsi_sense));

	bzero(&scsi_cmd, sizeof(scsi_cmd));
	scsi_cmd.opcode = MODE_SENSE;
	scsi_cmd.page = page;
	scsi_cmd.length = 0x20;
	/*
	 * If the command worked, use the results to fill out
	 * the parameter structure
	 */
	return scsi_scsi_cmd(sd->sc_link, (struct scsi_generic *)&scsi_cmd,
	    sizeof(scsi_cmd), (u_char *)scsi_sense, sizeof(*scsi_sense),
	    SDRETRIES, 6000, NULL, flags | SCSI_DATA_IN | SCSI_SILENT);
}

int
sd_get_optparms(sd, flags, dp)
	struct sd_softc *sd;
	int flags;
	struct disk_parms *dp;
{
	struct scsi_mode_sense scsi_cmd;
	struct scsi_mode_sense_data {
		struct scsi_mode_header header;
		struct scsi_blk_desc blk_desc;
		union disk_pages pages;
	} scsi_sense;
	u_long sectors;
	int error;

	dp->blksize = DEV_BSIZE;
	if ((sectors = scsi_size(sd->sc_link, flags)) == 0)
		return 1;

	/* XXX
	 * It is better to get the following params from the
	 * mode sense page 6 only (optical device parameter page).
	 * However, there are stupid optical devices which does NOT
	 * support the page 6. Ghaa....
	 */
	bzero(&scsi_cmd, sizeof(scsi_cmd));
	scsi_cmd.opcode = MODE_SENSE;
	scsi_cmd.page = 0x3f;	/* all pages */
	scsi_cmd.length = sizeof(struct scsi_mode_header) +
	    sizeof(struct scsi_blk_desc);

	if ((error = scsi_scsi_cmd(sd->sc_link,  
	    (struct scsi_generic *)&scsi_cmd, sizeof(scsi_cmd),  
	    (u_char *)&scsi_sense, sizeof(scsi_sense), SDRETRIES,
	    6000, NULL, flags | SCSI_DATA_IN)) != 0)
		return error;

	dp->blksize = _3btol(scsi_sense.blk_desc.blklen);
	if (dp->blksize == 0) 
		dp->blksize = DEV_BSIZE;

	/*
	 * Create a pseudo-geometry.
	 */
	dp->heads = 64;
	dp->sectors = 32;
	dp->cyls = sectors / (dp->heads * dp->sectors);
	dp->disksize = sectors;

	return 0;
}

/*
 * Get the scsi driver to send a full inquiry to the * device and use the
 * results to fill out the disk parameter structure.
 */
int
sd_get_parms(sd, flags)
	struct sd_softc *sd;
	int flags;
{
	struct disk_parms *dp = &sd->params;
	struct scsi_mode_sense_data scsi_sense;
	u_long sectors;
	int page;
	int error;

	if (sd->type == T_OPTICAL) {
		if ((error = sd_get_optparms(sd, flags, dp)) != 0)
			sd->sc_link->flags &= ~SDEV_MEDIA_LOADED;
		return error;
	}

	if ((error = sd_mode_sense(sd, &scsi_sense, page = 4, flags)) == 0) {
		SC_DEBUG(sd->sc_link, SDEV_DB3,
		    ("%d cyls, %d heads, %d precomp, %d red_write, %d land_zone\n",
		    _3btol(scsi_sense.pages.rigid_geometry.ncyl),
		    scsi_sense.pages.rigid_geometry.nheads,
		    _2btol(scsi_sense.pages.rigid_geometry.st_cyl_wp),
		    _2btol(scsi_sense.pages.rigid_geometry.st_cyl_rwc),
		    _2btol(scsi_sense.pages.rigid_geometry.land_zone)));

		/*
		 * KLUDGE!! (for zone recorded disks)
		 * give a number of sectors so that sec * trks * cyls
		 * is <= disk_size
		 * can lead to wasted space! THINK ABOUT THIS !
		 */
		dp->heads = scsi_sense.pages.rigid_geometry.nheads;
		dp->cyls = _3btol(scsi_sense.pages.rigid_geometry.ncyl);
		dp->blksize = _3btol(scsi_sense.blk_desc.blklen);

		if (dp->heads == 0 || dp->cyls == 0)
			goto fake_it;

		if (dp->blksize == 0)
			dp->blksize = DEV_BSIZE;

		sectors = scsi_size(sd->sc_link, flags);
		dp->disksize = sectors;
		sectors /= (dp->heads * dp->cyls);
		dp->sectors = sectors;	/* XXX dubious on SCSI */

		return 0;
	}

	if ((error = sd_mode_sense(sd, &scsi_sense, page = 5, flags)) == 0) {
		dp->heads = scsi_sense.pages.flex_geometry.nheads;
		dp->cyls = _2btol(scsi_sense.pages.flex_geometry.ncyl);
		dp->blksize = _3btol(scsi_sense.blk_desc.blklen);
		dp->sectors = scsi_sense.pages.flex_geometry.ph_sec_tr;
		dp->disksize = dp->heads * dp->cyls * dp->sectors;
		if (dp->disksize == 0)
			goto fake_it;

		if (dp->blksize == 0)
			dp->blksize = DEV_BSIZE;

		return 0;
	}

fake_it:
	if ((sd->sc_link->quirks & SDEV_NOMODESENSE) == 0) {
		if (error == 0)
			printf("%s: mode sense (%d) returned nonsense",
			    sd->sc_dev.dv_xname, page);
		else
			printf("%s: could not mode sense (4/5)",
			    sd->sc_dev.dv_xname);
		printf("; using fictitious geometry\n");
	}
	/*
	 * use adaptec standard fictitious geometry
	 * this depends on which controller (e.g. 1542C is
	 * different. but we have to put SOMETHING here..)
	 */
	sectors = scsi_size(sd->sc_link, flags);
	dp->heads = 64;
	dp->sectors = 32;
	dp->cyls = sectors / (64 * 32);
	dp->blksize = DEV_BSIZE;
	dp->disksize = sectors;
	return 0;
}

/*
 * Copy up to len chars from src to dst, ignoring non-printables.
 * Must be room for len+1 chars in dst so we can write the NUL.
 * Does not assume src is NUL-terminated.
 */
void
viscpy(dst, src, len)
	u_char *dst;
	u_char *src;
	int len;
{
	while (len > 0 && *src != '\0') {
		if (*src < 0x20 || *src >= 0x80) {
			src++;
			continue;
		}
		*dst++ = *src++;
		len--;
	}
	*dst = '\0';
}
