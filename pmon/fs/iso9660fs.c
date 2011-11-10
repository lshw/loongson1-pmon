/* $Id: iso9660fs.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*	$OpenBSD: cd9660.c,v 1.7 1998/05/30 02:29:56 mickey Exp $	*/
/*	$NetBSD: cd9660.c,v 1.1 1996/09/30 16:01:19 ws Exp $	*/

/*
 * Copyright (C) 1996 Wolfgang Solfrank.
 * Copyright (C) 1996 TooLs GmbH.
 * All rights reserved.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Stand-alone ISO9660 file reading package.
 *
 * Note: This doesn't support Rock Ridge extensions, extended attributes,
 * blocksizes other than 2048 bytes, multi-extent files, etc.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <sys/device.h>
#include <stdlib.h>
#include <ctype.h>

#include <pmon.h>
#include <file.h>
#include <diskfs.h>

/* THIS IS AN UGLY HACK!!!			XXX */
struct fid;
struct mbuf;
struct nameidata;
struct netexport { int x; };
struct proc;
struct statfs;
struct ucred;

#include "iso9660.h"

/*
 *  ISO9660 uses disk devio for physical access
 */
extern int   devio_open (int, const char *, int, int);
extern int   devio_close (int);
extern int   devio_read (int, void *, size_t);
extern int   devio_write (int, const void *, size_t);
extern off_t devio_lseek (int, off_t, int);

extern int errno;

struct ptable_ent {
	char namlen	[ISODCL( 1, 1)];	/* 711 */
	char extlen	[ISODCL( 2, 2)];	/* 711 */
	char block	[ISODCL( 3, 6)];	/* 732 */
	char parent	[ISODCL( 7, 8)];	/* 722 */
	char name	[1];
};
#define	PTFIXSZ		8
#define	PTSIZE(pp)	roundup(PTFIXSZ + isonum_711((pp)->namlen), 2)


static struct iso9660dev {
        int   offs;
        int   bno;
        int   size;
} openfile[OPEN_MAX];

#define	cdb2devb(bno)	((off_t)(bno) * ISO_DEFAULT_BLOCK_SIZE)

int   iso9660_open (int, const char *, int, int);
int   iso9660_close (int);
int   iso9660_read (int, void *, size_t);
int   iso9660_write (int, const void *, size_t);
off_t iso9660_lseek (int, off_t, int);

static int pnmatch(const char *path, struct ptable_ent *pp);
static int dirmatch(const char *path, struct iso_directory_record *dp);

static int
pnmatch(const char *path, struct ptable_ent *pp)
{
	char *cp;
	int i;
	
	cp = pp->name;
	for (i = isonum_711(pp->namlen); --i >= 0; path++, cp++) {
		if (toupper(*path) == toupper(*cp))
			continue;
		return 0;
	}
	if (*path != '/')
		return 0;
	return 1;
}

static int
dirmatch(const char *path, struct iso_directory_record *dp)
{
	char *cp;
	int i;

	/* This needs to be a regular file */
	if (dp->flags[0] & 6)
		return 0;

	cp = dp->name;
	for (i = isonum_711(dp->name_len); --i >= 0; path++, cp++) {
		if (!*path)
			break;
		if (toupper(*path) == toupper(*cp))
			continue;
		return 0;
	}
	if (*path)
		return 0;
	/*
	 * Allow stripping of trailing dots and the version number.
	 * Note that this will find the first instead of the last version
	 * of a file.
	 */
	if (i >= 0 && (*cp == ';' || *cp == '.')) {
		/* This is to prevent matching of numeric extensions */
		if (*cp == '.' && cp[1] != ';')
			return 0;
		while (--i >= 0)
			if (*++cp != ';' && (*cp < '0' || *cp > '9'))
				return 0;
	}
	return 1;
}

int
iso9660_open(int fd, const char *path, int flags, int mode)
{
	char dpath[64];
	void *buf;
	const char *opath;
	struct iso_primary_descriptor *vd;
	size_t buf_size, psize, dsize;
	daddr_t bno;
	int parent, ent;
	struct ptable_ent *pp;
	struct iso_directory_record *dp;
	int rc;
	int showdir,lookupdir;
	showdir=0;
	if(path[strlen(path)-1]=='/')lookupdir=1;
	/*  Try to get to the physical device */
	opath = path;
	if (strncmp(opath, "/dev/", 5) == 0)
		opath += 5;
	if (strncmp(opath, "iso9660/", 8) == 0)
		opath += 8;
	else if (strncmp(opath, "iso9660@", 8) == 0)
		opath += 8;

	/* There has to be at least one more component after the devicename */
	if (strchr(opath, '/') == NULL) {
		errno = ENOENT;
		return -1;
	}

	/* Dig out the device name */
	strncpy(dpath, opath, sizeof(dpath));
	*(strchr(dpath, '/')) = '\0';

	/* Set opath to point at the isolated filname path */
	opath = strchr(opath, '/') + 1;

	if(opath[0]==0){opath="/";}
	/* Try to open the physical device */
	rc = devio_open(fd, dpath, flags, mode);
	if (rc < 0)
		return rc;

	/* First find the volume descriptor */
	buf = malloc(buf_size = ISO_DEFAULT_BLOCK_SIZE);
	dp = (struct iso_directory_record *)buf;
	vd = buf;
	for (bno = 16;; bno++) {
		devio_lseek(fd, cdb2devb(bno), SEEK_SET);
		rc = devio_read(fd, buf, ISO_DEFAULT_BLOCK_SIZE);
		if (rc != ISO_DEFAULT_BLOCK_SIZE) {
			goto out;
		}
		errno = ENOENT;		/* In case of error */
		if (bcmp(vd->id, ISO_STANDARD_ID, sizeof vd->id) != 0)
			goto out;
		if (isonum_711(vd->type) == ISO_VD_END)
			goto out;
		if (isonum_711(vd->type) == ISO_VD_PRIMARY)
			break;
	}
	if (isonum_723(vd->logical_block_size) != ISO_DEFAULT_BLOCK_SIZE)
		goto out;
	
	/* Now get the path table and lookup the directory of the file */
	bno = isonum_732(vd->type_m_path_table);
	psize = isonum_733(vd->path_table_size);
	
	if (psize > ISO_DEFAULT_BLOCK_SIZE) {
		free(buf);
		buf = malloc(buf_size = roundup(psize, ISO_DEFAULT_BLOCK_SIZE));
	}

	devio_lseek(fd, cdb2devb(bno), SEEK_SET);
	rc = devio_read(fd, buf, buf_size);
		if (rc != buf_size) {
		goto out;
	}
	
	parent = 1;
	pp = (struct ptable_ent *)buf;
	ent = 1;
	bno = isonum_732(pp->block) + isonum_711(pp->extlen);
	
	while (*opath) {
		if ((void *)pp >= buf + psize)
			break;
		if (isonum_722(pp->parent) != parent)
			break;
		if (!pnmatch(opath, pp)) {
			pp = (struct ptable_ent *)((void *)pp + PTSIZE(pp));
			ent++;
			continue;
		}
		opath += isonum_711(pp->namlen) + 1;
		if(lookupdir&&(opath[0]==0))
		{
			opath="/";
		}
		parent = ent;
		bno = isonum_732(pp->block) + isonum_711(pp->extlen);
		while ((void *)pp < buf + psize) {
			if (isonum_722(pp->parent) == parent)
				break;
			pp = (struct ptable_ent *)((void *)pp + PTSIZE(pp));
			ent++;
		}
	}

	/* Now bno has the start of the dir that supposedly contains the file */
	bno--;
	dsize = 1;		/* Something stupid, but > 0	XXX */
	for (psize = 0; psize < dsize;) {
		if (!(psize % ISO_DEFAULT_BLOCK_SIZE)) {
			bno++;
			devio_lseek(fd, cdb2devb(bno), SEEK_SET);
			rc = devio_read(fd, buf, ISO_DEFAULT_BLOCK_SIZE);
			if (rc != ISO_DEFAULT_BLOCK_SIZE) {
				goto out;
			}
			dp = (struct iso_directory_record *)buf;
		}
		if (!isonum_711(dp->length)) {
			if ((void *)dp == buf)
				psize += ISO_DEFAULT_BLOCK_SIZE;
			else
				psize = roundup(psize, ISO_DEFAULT_BLOCK_SIZE);
			continue;
		}
		if (dsize == 1)
			dsize = isonum_733(dp->size);
#if 1		
		 	if(opath[0]=='/') showdir=1;
		    if(showdir)
			{
			char name[102];
			int len;
			char *p;

			len=isonum_711(dp->name_len);
			if(len>100)len=100;
			strncpy(name,dp->name,len);
			name[len]=0;
			if((p=strchr(name,';')))p[0]=0;
			else strcat(name,"/");
			printf("%s ",name);
			}
#endif		
		if (dirmatch(opath, dp))
			break;
		psize += isonum_711(dp->length);
		dp = (struct iso_directory_record *)((void *)dp + isonum_711(dp->length));
	}

	if (psize >= dsize) {
		errno = ENOENT;
		goto out;
	}
	
	/* allocate file system specific data structure */

	openfile[fd].offs = 0;
	openfile[fd].bno = isonum_733(dp->extent);
	openfile[fd].size = isonum_733(dp->size);
	free(buf);
	return fd;
	
out:
	if(showdir)printf("\r\n");
	free(buf);
	rc = errno;
	(void)devio_close(fd);
	errno = rc;		/* Return real reason! */
	return -1;
}

int
iso9660_close(int fd)
{
	return devio_close(fd);
}

int
iso9660_read(int fd, void *start, size_t size)
{
	int rc = 0;
	daddr_t bno;
	char buf[ISO_DEFAULT_BLOCK_SIZE];
	char *dp;
	size_t asked, offs;
	
	asked = size;

	while (size) {
		if (openfile[fd].offs < 0 ||
		    openfile[fd].offs >= openfile[fd].size)
			break;
		bno = (openfile[fd].offs >> ISO_DEFAULT_BLOCK_SHIFT);
		bno += openfile[fd].bno;
		if (openfile[fd].offs & (ISO_DEFAULT_BLOCK_SIZE - 1)
		    || size < ISO_DEFAULT_BLOCK_SIZE)
			dp = buf;
		else
			dp = start;
		devio_lseek(fd, cdb2devb(bno), SEEK_SET);
		rc = devio_read(fd, dp, ISO_DEFAULT_BLOCK_SIZE);
		if (rc != ISO_DEFAULT_BLOCK_SIZE) {
			errno = EIO;
			return -1;
		}
		if (dp == buf) {
			offs = openfile[fd].offs & (ISO_DEFAULT_BLOCK_SIZE - 1);
			if (rc > offs + size)
				rc = offs + size;
			rc -= offs;
			bcopy(buf + offs, start, rc);
			start += rc;
			openfile[fd].offs += rc;
			size -= rc;
		} else {
			start += ISO_DEFAULT_BLOCK_SIZE;
			openfile[fd].offs += ISO_DEFAULT_BLOCK_SIZE;
			size -= ISO_DEFAULT_BLOCK_SIZE;
		}
	}
	return asked - size;
}

int
iso9660_write(int fd, const void *start, size_t size)
{
	return EROFS;
}

off_t
iso9660_lseek(int fd, off_t offset, int where)
{
	switch (where) {
	case SEEK_SET:
		openfile[fd].offs = offset;
		break;
	case SEEK_CUR:
		openfile[fd].offs += offset;
		break;
	case SEEK_END:
		openfile[fd].offs = openfile[fd].size - offset;
		break;
	default:
		return -1;
	}
	return openfile[fd].offs;
}


/*
 *  File system registration info.
 */

static DiskFileSystem diskfile={
        "iso9660",
        iso9660_open,
        iso9660_read,
        iso9660_write,
        iso9660_lseek,
        iso9660_close,
	NULL
};

static FileSystem iso9660fs = {
        "iso9660", FS_FILE,
        iso9660_open,
        iso9660_read,
        iso9660_write,
        iso9660_lseek,
        iso9660_close,
	NULL
};

static void init_fs(void) __attribute__ ((constructor));

static void
init_fs()
{
	filefs_init(&iso9660fs);
	diskfs_init(&diskfile);
}
