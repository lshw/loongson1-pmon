/*
 * Copyright (c) 2000 Opsycon AB  (www.opsycon.se)
 * Copyright (c) 2002 Patrik Lindergren (www.lindergren.com)
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
 *	This product includes software developed by Opsycon AB.
 *	This product includes software developed by Patrik Lindergren.
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
/* $Id: diskfs.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

#include <stdio.h>
#include <string.h>
#include <termio.h>
#include <fcntl.h>
#include <file.h>
#include <ctype.h>
#include <diskfs.h>
#include <sys/unistd.h>
#include <stdlib.h>
#undef _KERNEL
#include <errno.h>
#include <pmon.h>

SLIST_HEAD(DiskFileSystems,DiskFileSystem)DiskFileSystems=SLIST_HEAD_INITIALIZER(DiskFileSystems);

static int diskfs_open (int , const char *, int, int);
static int diskfs_close (int);
static int diskfs_read (int, void *, size_t);
static int diskfs_write (int, const void *, size_t);
static off_t diskfs_lseek (int, off_t, int);

/*
 * Supported paths:
 *	/dev/fs/msdos@wd0/bsd
 *	/dev/fs/iso9660@cd0/bsd
 */


static int
   diskfs_open (fd, fname, mode, perms)
   int	       fd;
   const char *fname;
   int         mode;
   int	       perms;
{
	DiskFileSystem *p;
	DiskFile *dfp;
	char *dname;
	char *fsname = NULL;
	char *devname = NULL;
	int i;
	
	dname = (char *)fname;

	if (strncmp (dname, "/dev/fs/", 8) == 0)
		dname += 8;
	else
		return (-1);

	 devname=dname;

	for (i=0; i < strlen(dname); i++) {
		if (dname[i] == '@') {
			fsname = dname;
			dname[i] = 0;
			devname = &dname[i+1];
			break;
		}
	}
	if (fsname != NULL && fsname[0]) {
		int found = 0;
		SLIST_FOREACH(p, &DiskFileSystems, i_next) {
			if (strcmp (fsname, p->fsname) == 0) {
				found = 1;
				break;
			}
		}
		if (found == 0)
			return (-1);
	dfp = (DiskFile *)malloc(sizeof(DiskFile));
	if (dfp == NULL) {
		fprintf(stderr, "Out of space for allocating a DiskFile");
		return(-1);
	}

	dfp->dfs = p;
	
	_file[fd].posn = 0;
	_file[fd].data = (void *)dfp;
	if(p->open)
		return ((p->open)(fd,devname,mode,perms));	
	return -1;	
	} else {
	char path[100];
	strncpy(path,devname,100);
	dfp = (DiskFile *)malloc(sizeof(DiskFile));
	if (dfp == NULL) {
		fprintf(stderr, "Out of space for allocating a DiskFile");
		return(-1);
	}

		SLIST_FOREACH(p, &DiskFileSystems, i_next) {
	dfp->dfs = p;
	
	_file[fd].posn = 0;
	_file[fd].data = (void *)dfp;
	strncpy(devname,path,100);
	if(p->open && ((p->open)(fd,devname,mode,perms)==fd))
	return fd;
		}
		free(dfp);
		return -1;
	}
}

/** close(fd) close fd */
static int
   diskfs_close (fd)
   int             fd;
{
	DiskFile *p;

	p = (DiskFile *)_file[fd].data;

	if (p->dfs->close)
		return ((p->dfs->close) (fd));
	else
		return (-1);
}

/** read(fd,buf,n) read n bytes into buf from fd */
static int
   diskfs_read (fd, buf, n)
   int fd;
   void *buf;
   size_t n;
{
	DiskFile        *p;

	p = (DiskFile *)_file[fd].data;

	if (p->dfs->read)
		return ((p->dfs->read) (fd, buf, n));
	else {
		return (-1);
	}
}

static int
   diskfs_write (fd, buf, n)
   int fd;
   const void *buf;
   size_t n;
{
	DiskFile        *p;

	p = (DiskFile *)_file[fd].data;

	if (p->dfs->write)
		return ((p->dfs->write) (fd, buf, n));
	else
		return (-1);
}

static off_t
diskfs_lseek (fd, offset, whence)
	int             fd, whence;
	off_t            offset;
{
	DiskFile        *p;

	p = (DiskFile *)_file[fd].data;

	if (p->dfs->lseek)
		return ((p->dfs->lseek) (fd, offset, whence));
	else
		return (-1);
}

static FileSystem diskfs =
{
	"fs",FS_FILE,
	diskfs_open,
	diskfs_read,
	diskfs_write,
	diskfs_lseek,
	diskfs_close,
	NULL
};

static void init_fs __P((void)) __attribute__ ((constructor));

static void
   init_fs()
{
	/*
	 * Install diskfs based file system.
	 */
	filefs_init(&diskfs);
}
int diskfs_init(DiskFileSystem *dfs)
{
	SLIST_INSERT_HEAD(&DiskFileSystems,dfs,i_next);
	return (0);
}
