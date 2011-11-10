/* $Id */

/*	$OpenBSD: cd.c,v 1.54 2000/04/18 06:34:18 csapuntz Exp $	*/
/*	$NetBSD: cd.c,v 1.100 1997/04/02 02:29:30 mycroft Exp $	*/

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
 * Originally written by Julian Elischer (julian@tfs.com)
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
 * Ported to run under 386BSD by Julian Elischer (julian@tfs.com) Sept 1992
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/scsiio.h>
#include <sys/vnode.h>

#include <scsi/scsi_all.h>
#include <scsi/cd.h>
#include <scsi/scsi_disk.h>	/* rw_big and start_stop come from there */
#include <scsi/scsiconf.h>


#define	CDRETRIES	4
#define CDBLKSIZE	2048	/* XXX hardwired in */

struct cd_ops;

struct cd_softc {
	struct device sc_dev;
	struct disk sc_dk;

	int flags;
#define	CDF_LOCKED	0x01
#define	CDF_WANTED	0x02
#define	CDF_WLABEL	0x04		/* label is writable */
#define	CDF_LABELLING	0x08		/* writing label */
	struct scsi_link *sc_link;	/* contains our targ, lun, etc. */
	struct buf buf_queue;
	char name[16]; /* product name, for default disklabel */
};

#define	CDOUTSTANDING	4

#define	CDUNIT(z)			DISKUNIT(z)
#define	CDMINOR(unit, part)	       	DISKMINOR(unit, part)
#if 0
#define	CDPART(z)			DISKPART(z)
#else
#define	CDPART(z)			RAW_PART
#endif
#define	MAKECDDEV(maj, unit, part)	MAKEDISKDEV(maj, unit, part)

#define	CDLABELDEV(dev)	(MAKECDDEV(major(dev), CDUNIT(dev), RAW_PART))

int	cdmatch __P((struct device *, void *, void *));
void	cdattach __P((struct device *, struct device *, void *));
int	cdopen(dev_t dev, int flag, int fmt, struct proc *p);
int	cdclose(dev_t dev, int flag, int fmt, struct proc *p);
void	cdstrategy(struct buf *bp);
int	cdread(dev_t dev, struct uio *uio, int ioflag);
int	cdwrite(dev_t dev, struct uio *uio, int ioflag);

void	cdstart __P((void *));
void	cddone __P((struct scsi_xfer *));

struct cfattach cd_ca = {
	sizeof(struct cd_softc), cdmatch, cdattach
};

struct cfdriver cd_cd = {
	NULL, "cd", DV_DISK
};

struct scsi_device cd_switch = {
	NULL,			/* use default error handler */
	cdstart,		/* we have a queue, which is started by this */
	NULL,			/* we do not have an async handler */
	cddone,			/* deal with stats at interrupt time */
};

struct scsi_inquiry_pattern cd_patterns[] = {
	{T_CDROM, T_REMOV,
	 "",         "",                 ""},
	{T_WORM, T_REMOV,
	 "",         "",                 ""},
	{T_DIRECT, T_REMOV,
	 "NEC                 CD-ROM DRIVE:260", "", ""},
#if 0
	{T_CDROM, T_REMOV, /* more luns */
	 "PIONEER ", "CD-ROM DRM-600  ", ""},
#endif
};

#define cdlookup(unit) (struct cd_softc *)device_lookup(&cd_cd, (unit))

int
cdmatch(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct scsibus_attach_args *sa = aux;
	int priority;

	(void)scsi_inqmatch(sa->sa_inqbuf,
	    (caddr_t)cd_patterns, sizeof(cd_patterns)/sizeof(cd_patterns[0]),
	    sizeof(cd_patterns[0]), &priority);
	return (priority);
}

/*
 * The routine called by the low level scsi routine when it discovers
 * A device suitable for this driver
 */
void
cdattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct cd_softc *cd = (void *)self;
	struct scsibus_attach_args *sa = aux;
	struct scsi_link *sc_link = sa->sa_sc_link;

	SC_DEBUG(sc_link, SDEV_DB2, ("cdattach: "));

	/*
	 * Store information needed to contact our base driver
	 */
	cd->sc_link = sc_link;
	sc_link->device = &cd_switch;
	sc_link->device_softc = cd;
	if (sc_link->openings > CDOUTSTANDING)
		sc_link->openings = CDOUTSTANDING;

	/*
	 * Initialize and attach the disk structure.
	 */
	cd->sc_dk.dk_name = cd->sc_dev.dv_xname;
  
	printf("\n");
}

/*
 * open the device. Make sure the partition info is a up-to-date as can be.
 */
int 
cdopen(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	struct cd_softc *cd;
	struct scsi_link *sc_link;
	int unit, part;
	int error;

	unit = CDUNIT(dev);
	cd = cdlookup(unit);
	if (cd == NULL)
		return ENXIO;

	sc_link = cd->sc_link;

	SC_DEBUG(sc_link, SDEV_DB1,
	    ("cdopen: dev=0x%x (unit %d (of %d), partition %d)\n", dev, unit,
	    cd_cd.cd_ndevs, CDPART(dev)));

	if (cd->sc_dk.dk_openmask != 0) {
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
		error = scsi_start(sc_link, SSS_START,
				   SCSI_IGNORE_ILLEGAL_REQUEST |
				   SCSI_IGNORE_MEDIA_CHANGE | SCSI_SILENT);
		if (error)
			goto bad3;

		sc_link->flags |= SDEV_OPEN;

		/* Lock the pack in. */
		error = scsi_prevent(sc_link, PR_PREVENT,
				     SCSI_IGNORE_ILLEGAL_REQUEST |
				     SCSI_IGNORE_MEDIA_CHANGE);
		if (error)
			goto bad;

		if ((sc_link->flags & SDEV_MEDIA_LOADED) == 0) {
			sc_link->flags |= SDEV_MEDIA_LOADED;
		}
	}

	part = CDPART(dev);

	/*  Fake some label info that we are going to use. If we
         *  want to be more advanced in the future, eg not ony RAW
	 *  this label must probably be filled in some more...
	 */
	cd->sc_dk.dk_label->d_secsize = 2048; /* XXX Yeah... */

	/* Check that the partition exists. */
	if (part != RAW_PART &&
	    (part >= cd->sc_dk.dk_label->d_npartitions ||
	     cd->sc_dk.dk_label->d_partitions[part].p_fstype == FS_UNUSED)) {
		error = ENXIO;
		goto bad;
	}

	/* Insure only one open at a time. */
	switch (fmt) {
	case S_IFCHR:
		cd->sc_dk.dk_copenmask |= (1 << part);
		break;
	case S_IFBLK:
		cd->sc_dk.dk_bopenmask |= (1 << part);
		break;
	}
	cd->sc_dk.dk_openmask = cd->sc_dk.dk_copenmask | cd->sc_dk.dk_bopenmask;

	SC_DEBUG(sc_link, SDEV_DB3, ("open complete\n"));
	return 0;

	sc_link->flags &= ~SDEV_MEDIA_LOADED;

bad:
	if (cd->sc_dk.dk_openmask == 0) {
		scsi_prevent(sc_link, PR_ALLOW,
		    SCSI_IGNORE_ILLEGAL_REQUEST | SCSI_IGNORE_MEDIA_CHANGE);
		sc_link->flags &= ~SDEV_OPEN;
	}

bad3:
	return error;
}

/*
 * close the device.. only called if we are the LAST
 * occurence of an open device
 */
int 
cdclose(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	struct cd_softc *cd;
	int part = CDPART(dev);

	cd = cdlookup(CDUNIT(dev));
	if (cd == NULL)
		return ENXIO;

	switch (fmt) {
	case S_IFCHR:
		cd->sc_dk.dk_copenmask &= ~(1 << part);
		break;
	case S_IFBLK:
		cd->sc_dk.dk_bopenmask &= ~(1 << part);
		break;
	}
	cd->sc_dk.dk_openmask = cd->sc_dk.dk_copenmask | cd->sc_dk.dk_bopenmask;

	if (cd->sc_dk.dk_openmask == 0) {
		/* XXXX Must wait for I/O to complete! */

		scsi_prevent(cd->sc_link, PR_ALLOW,
		    SCSI_IGNORE_ILLEGAL_REQUEST | SCSI_IGNORE_NOT_READY);
		cd->sc_link->flags &= ~SDEV_OPEN;

		if (cd->sc_link->flags & SDEV_EJECTING) {
			scsi_start(cd->sc_link, SSS_STOP|SSS_LOEJ, 0);

			cd->sc_link->flags &= ~SDEV_EJECTING;
		}
	}

	return 0;
}

/*
 * Actually translate the requested transfer into one the physical driver can
 * understand.  The transfer is described by a buf and will include only one
 * physical transfer.
 */
void
cdstrategy(bp)
	struct buf *bp;
{
	struct cd_softc *cd;
	int opri;

	if ((cd = cdlookup(CDUNIT(bp->b_dev))) == NULL) {
		bp->b_error = ENXIO;
		goto bad;
	}

	SC_DEBUG(cd->sc_link, SDEV_DB2, ("cdstrategy "));
	SC_DEBUG(cd->sc_link, SDEV_DB1,
	    ("%ld bytes @ blk %d\n", bp->b_bcount, bp->b_blkno));
	/*
	 * The transfer must be a whole number of blocks.
	 */
	if ((bp->b_bcount % cd->sc_dk.dk_label->d_secsize) != 0) {
		bp->b_error = EINVAL;
		goto bad;
	}
	/*
	 * If the device has been made invalid, error out
	 * maybe the media changed
	 */
	if ((cd->sc_link->flags & SDEV_MEDIA_LOADED) == 0) {
		bp->b_error = EIO;
		goto bad;
	}
	/*
	 * If it's a null transfer, return immediately
	 */
	if (bp->b_bcount == 0)
		goto done;

	opri = splbio();

	/*
	 * Place it in the queue of disk activities for this disk
	 */
	disksort(&cd->buf_queue, bp);

	/*
	 * Tell the device to get going on the transfer if it's
	 * not doing anything, otherwise just wait for completion
	 */
	cdstart(cd);
	
	splx(opri);
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
 * cdstart looks to see if there is a buf waiting for the device
 * and that the device is not already busy. If both are true,
 * It deques the buf and creates a scsi command to perform the
 * transfer in the buf. The transfer request will call scsi_done
 * on completion, which will in turn call this routine again
 * so that the next queued transfer is performed.
 * The bufs are queued by the strategy routine (cdstrategy)
 *
 * This routine is also called after other non-queued requests
 * have been made of the scsi driver, to ensure that the queue
 * continues to be drained.
 *
 * must be called at the correct (highish) spl level
 * cdstart() is called at splbio from cdstrategy and scsi_done
 */
void 
cdstart(v)
	register void *v;
{
	register struct cd_softc *cd = v;
	register struct scsi_link *sc_link = cd->sc_link;
	struct buf *bp = 0;
	struct buf *dp;
	struct scsi_rw_big cmd_big;
	struct scsi_rw cmd_small;
	struct scsi_generic *cmdp;
	int blkno, nblks, cmdlen;
	struct partition *p;

	SC_DEBUG(sc_link, SDEV_DB2, ("cdstart "));
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
		dp = &cd->buf_queue;
		if ((bp = dp->b_actf) == NULL)	/* yes, an assign */
			return;
		dp->b_actf = bp->b_actf;

		/*
		 * If the deivce has become invalid, abort all the
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
		blkno =
		    bp->b_blkno / (cd->sc_dk.dk_label->d_secsize / DEV_BSIZE);
		if (CDPART(bp->b_dev) != RAW_PART) {
		      p = &cd->sc_dk.dk_label->d_partitions[CDPART(bp->b_dev)];
		      blkno += p->p_offset;
		}
		nblks = howmany(bp->b_bcount, cd->sc_dk.dk_label->d_secsize);

		/*
		 *  Fill out the scsi command.  If the transfer will
		 *  fit in a "small" cdb, use it.
		 */
		if (!(sc_link->flags & SDEV_ATAPI) &&
		    ((blkno & 0x1fffff) == blkno) &&
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
		if (scsi_scsi_cmd(sc_link, cmdp, cmdlen,
		    (u_char *) bp->b_data, bp->b_bcount,
		    CDRETRIES, 30000, bp, SCSI_NOSLEEP |
		    ((bp->b_flags & B_READ) ? SCSI_DATA_IN : SCSI_DATA_OUT))) {
			printf("%s: not queued", cd->sc_dev.dv_xname);
		}
	}
}

void
cddone(xs)
	struct scsi_xfer *xs;
{
}

int
cdread(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{

	return (physio(cdstrategy, NULL, dev, B_READ, NULL, uio));
}

int
cdwrite(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{

	return (physio(cdstrategy, NULL, dev, B_WRITE, NULL, uio));
}
