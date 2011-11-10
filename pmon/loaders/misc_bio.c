/* $Id: misc_bio.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 1998-2002 Per Fogelstrom, Opsycon AB
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
 *	This product includes software developed by Per Fogelstrom,
 *	Opsycon AB, Sweden.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 *	_Very_ simplified support functions to i/o subsystem.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/device.h>

#include <pmon.h>
#include <file.h>

#include "cd.h"
#include "sd.h"
#include "wd.h"

extern int errno;

struct devsw {
	char	*name;
	int	(*open) __P((dev_t, int flags, int mode, void *));
	int	(*read) __P((dev_t dev, void *uio, int flag));
	int	(*write) __P((dev_t dev, void *uio, int flag));
	int	(*close) __P((dev_t dev, int flag, int mode, void *));
};

extern int sdopen __P((dev_t dev, int flags, int mode, void *));
extern int sdread __P((dev_t dev, void *uio, int flag));
extern int sdwrite __P((dev_t dev, void *uio, int flag));
extern int sdclose __P((dev_t dev, int flag, int mode, void *));

extern int cdopen __P((dev_t dev, int flags, int mode, void *));
extern int cdread __P((dev_t dev, void *uio, int flag));
extern int cdwrite __P((dev_t dev, void *uio, int flag));
extern int cdclose __P((dev_t dev, int flag, int mode, void *));

extern int wdopen __P((dev_t dev, int flags, int mode, void *));
extern int wdread __P((dev_t dev, void *uio, int flag));
extern int wdwrite __P((dev_t dev, void *uio, int flag));
extern int wdclose __P((dev_t dev, int flag, int mode, void *));

void disksort __P((struct buf *, struct buf *));

struct devsw devswitch[] = {
	{ "console" },
#if NSD > 0
	{ "sd", sdopen, sdread, sdwrite, sdclose },	/* 0100 = sd<n> */
#endif
#if NWD > 0
	{ "wd", wdopen, wdread, wdwrite, wdclose },	/* 0200 = wd<n> */
#endif
#if NCD > 0
	{ "cd", cdopen, cdread, cdwrite, cdclose },	/* 0300 = cd<n> */
#endif
	{ NULL},
};

static struct biodev {
	dev_t	devno;
	off_t	offs;
	off_t	base;
	off_t	end;
	int	*maptab;
} opendevs[OPEN_MAX];

int   devio_open (int, const char *, int, int);
int   devio_close (int);
int   devio_read (int, void *, size_t);
int   devio_write (int, const void *, size_t);
off_t devio_lseek (int, off_t, int);

/* Forward */
dev_t find_device (const char **name);


/*
 *  No need to sort really. We can only have one request at a time.
 */
void
disksort(ap, bp)
	struct buf *ap, *bp;
{
	if(ap->b_actf == NULL) {
		bp->b_actf = NULL;
		ap->b_actf = bp;
	}
	else {
		panic("Whoa! Disk queue not empty!!");
	}
	return;
}

/*
 *  Bio done. Nothing to do, we run polled...
 */
void
biodone(buf)
	struct buf *buf;
{
	buf->b_flags &= ~B_BUSY;
	wakeup(buf);
}

/*
 *  Very simplified PHYSIO. Adjust and pass on.
 */
int
physio(strategy, bp, dev, flags, minphys, uio)
	void (*strategy) __P((struct buf *));
	struct buf *bp;
	dev_t dev;
	int flags;
	void (*minphys) __P((struct buf *));
	struct uio *uio;
{
	int error = 0;
	struct buf lbp;
	int s;

	if(bp == NULL) {
		memset((void *)&lbp, 0, sizeof(struct buf));
		bp = &lbp;
	}
	bp->b_dev    = dev;
	bp->b_error  = 0;
	bp->b_bcount = uio->uio_iov[0].iov_len;
	bp->b_data   = uio->uio_iov[0].iov_base;
	bp->b_blkno  = btodb(uio->uio_offset);
	bp->b_flags = B_BUSY | B_PHYS | B_RAW | flags;

	(*strategy)(bp);
	if (!(bp->b_flags & B_ERROR)) {
		s = splbio();
		while (!(bp->b_flags & B_ERROR) && bp->b_flags & B_BUSY) {
			tsleep(bp, PRIBIO + 1, "biowait", 0);
		}
	}

	splx(s);

	if (bp->b_flags & B_ERROR) {
		error = (bp->b_error ? bp->b_error : EIO);
	}

	return(error);
}


/*
 *  Lookup and 'open' a disk device.
 */

int
devio_open(fd, name, flags, mode)
	int fd;
	const char *name;
	int flags, mode;
{
	int mj;
	u_int32_t v;
	dev_t dev;
	struct biodev *devstat;
	char strbuf[64], *strp, *p;

	dev = find_device(&name);
	if(dev == -1) {
		errno = ENOENT;
		return(-1);
	}

	mj = dev >> 8;
	devstat = &opendevs[fd];
	devstat->devno = dev;
	devstat->offs = 0;
	devstat->base = 0;
	devstat->maptab = 0;
	devstat->end = 0x10000000000;
	curproc->p_stat = SRUN;
	errno = (*devswitch[mj].open)(dev, 0, S_IFCHR, NULL);
	curproc->p_stat = SNOTKERN;

	/* Check for any subsize specification */
	if (!errno && *name++ == '@') {
		strncpy(strbuf, name, sizeof(strbuf));
		strp = strpbrk(strbuf, "/,:");
		if(!strp) {
			strp = strbuf + sizeof(strbuf);
		}
		else if(*strp != '/') {
			*strp++ = '\0';
			if((p = index(strp, '/')))
				*p = 0;
			if(!get_rsa(&v, strp))
				errno = EBADF;
			else
				devstat->end = (off_t)v << DEV_BSHIFT;
		}
		*strp++ = '\0';
		if(!get_rsa(&v, strbuf)) {
			errno = EBADF;
		}
		else {
			devstat->base = (off_t)v << DEV_BSHIFT;
			devstat->offs = (off_t)v << DEV_BSHIFT;
			devstat->end += (off_t)v << DEV_BSHIFT;
		}
	}

	if(errno) {
		return(-1);
	}

	return(fd);
}

/*
 *  Lookup and 'close' a disk device. (we are not counting...)
 */

int
devio_close(fd)
	int fd;
{
	int mj;

	mj = opendevs[fd].devno >> 8;
	curproc->p_stat = SRUN;
	errno = (*devswitch[mj].close)(opendevs[fd].devno, 0, S_IFCHR, NULL);
	curproc->p_stat = SNOTKERN;
	opendevs[fd].devno = 0;
	if(errno) {
		return(-1);
	}
	return(0);
}

/*
 *  Read data from a device.
 */

int
devio_read(fd, buf, blen)
	int fd;
	void *buf;
	size_t blen;
{
	int mj;
	struct uio uio;
	struct iovec iovec;
static char lbuff[DEV_BSIZE];

	if(opendevs[fd].offs < opendevs[fd].base ||
	   opendevs[fd].offs > opendevs[fd].end) {
		errno = EINVAL;
		return(-1);
	}
	if(blen > (opendevs[fd].end - opendevs[fd].offs))
		blen = opendevs[fd].end - opendevs[fd].offs;

	if (blen == 0)
		return(0);

	/* Check for unaligned read, eg we need to buffer */
	if (opendevs[fd].offs & (DEV_BSIZE - 1) || blen < DEV_BSIZE) {
		int suboffs = opendevs[fd].offs & (DEV_BSIZE - 1);
		char *p = &lbuff[suboffs];
		int c, n = 0;
		opendevs[fd].offs &= ~(DEV_BSIZE - 1);
		if (devio_read(fd, lbuff, DEV_BSIZE) < 0) {
			opendevs[fd].offs += suboffs;
			return(-1);
		}
		while(blen && suboffs < DEV_BSIZE) {
			*((char *)buf)++ = *p++;
			blen--;
			suboffs++;
			n++;
		}
		opendevs[fd].offs -= (DEV_BSIZE - suboffs);
		c = devio_read(fd, buf, blen);
		if (c >= 0)
			return(c + n);
		else
			return(-1);
	}
	else if(blen & (DEV_BSIZE - 1)) {
		int c, n;
		c = devio_read(fd, buf, blen & ~(DEV_BSIZE - 1));
		if (c < 0)
			return(-1);
		buf += c;
		blen -= c;
		n = devio_read(fd, buf, blen);
		if (n >= 0)
			return(n + c);
		else
			return(-1);
	}
	if (blen > MAXPHYS) {
		int c, n = 0;
		do {
			c = devio_read(fd, buf, min(MAXPHYS, blen));
			if (c < 0)
				return(-1);
			n += c;
			blen -= c;
			buf += c;
		} while (blen > 0);
		return(n);
	}
	else {
		mj = opendevs[fd].devno >> 8;
		uio.uio_iovcnt = 1;
		uio.uio_iov = &iovec;
		uio.uio_rw = UIO_READ;
		uio.uio_offset = opendevs[fd].offs;
	
		iovec.iov_base = buf;
		iovec.iov_len = blen;

		curproc->p_stat = SRUN;
		errno = (*devswitch[mj].read)(opendevs[fd].devno, &uio, 0);
		curproc->p_stat = SNOTKERN;
	}

	if(errno) {
		return(-1);
	}
	opendevs[fd].offs += blen;
	return(blen);
}

/*
 *  Write data to a device.
 */

int
devio_write(fd, buf, blen)
	int fd;
	const void *buf;
	size_t blen;
{
	int mj;
	struct uio uio;
	struct iovec iovec;

	if (blen == 0)
		return(0);

	mj = opendevs[fd].devno >> 8;
	uio.uio_iovcnt = 1;
	uio.uio_iov = &iovec;
	uio.uio_rw = UIO_WRITE;
	uio.uio_offset = opendevs[fd].offs;
	
	iovec.iov_base = (void *)buf;
	iovec.iov_len = blen;

	curproc->p_stat = SRUN;
	errno = (*devswitch[mj].write)(opendevs[fd].devno, &uio, 0);
	curproc->p_stat = SNOTKERN;

	if(errno) {
		return(-1);
	}
	return(blen);
}

/*
 *  Move current pointer on device.
 */

off_t
devio_lseek(fd, offs, whence)
	int fd;
	off_t offs;
	int whence;
{
	if(whence == 0)
		opendevs[fd].offs = opendevs[fd].base + offs;
	else
		opendevs[fd].offs += offs;

	if(opendevs[fd].offs > opendevs[fd].end)
		opendevs[fd].offs = opendevs[fd].end;

	return(opendevs[fd].offs - opendevs[fd].base);
}

/*
 *  Lookup a phys device in the phys dev table.
 */
extern struct cfdata cfdata[];

dev_t
find_device(name)
	const char **name;
{
	struct cfdata *cf;
	struct cfdriver *cd;
	struct device *dv;
	dev_t dev;
	int i;
	struct devsw *devsw;
	const char *pname;

	/* Discard /dev/ and disk/ from name */
	pname = *name;
	if (strncmp(pname, "/dev/", 5) == 0)
		pname += 5;
	if (strncmp(pname, "disk/", 5) == 0) {
		pname += 5;
	}

	for(devsw = devswitch; devsw->name != NULL; devsw++) {
		if(strncmp(pname, devsw->name, strlen(devsw->name))) {
			continue;
		}
		break;
	}
	if(devsw->name == NULL) {
		return(-1);		/* Not found */
	}
	dev = devsw - devswitch;	/* Major number in dispatch table */

	for(cf = cfdata; (cd = cf->cf_driver); cf++) {
		if(cd->cd_devs == NULL)
			continue;

		for(i = 0; i < cd->cd_ndevs; i++) {
			if((dv = cd->cd_devs[i]) == NULL)
				continue;
			if(strncmp(pname, dv->dv_xname, strlen(dv->dv_xname)))
				continue;

			dev = (dev << 8) | i;
			*name = pname + strlen(dv->dv_xname);
			return(dev);
		}
	}
	return(-1);
}


/*
 *  File system registration info.
 */
static FileSystem diskfs = {
	"disk",
	devio_open,
	devio_read,
	devio_write,
	devio_lseek,
	devio_close,
	NULL
};

static void init_fs(void) __attribute__ ((constructor));

static void
init_fs()
{
	filefs_init(&diskfs);
}

