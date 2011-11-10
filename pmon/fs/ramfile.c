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
/* $Id: ramfile.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

#include <stdio.h>
#include <string.h>
#include <termio.h>
#include <fcntl.h>
#include <file.h>
#include <ctype.h>
#include <ramfile.h>
#include <sys/unistd.h>
#include <stdlib.h>
#undef _KERNEL
#include <errno.h>
#include "logfile.h"
#include <pmon.h>

LIST_HEAD(Ramfiles, Ramfile) Ramfiles = LIST_HEAD_INITIALIZER(Ramfiles);

static int ramfile_open (int , const char *, int, int);
static int ramfile_close (int);
static int ramfile_read (int, void *, size_t);
static int ramfile_write (int, const void *, size_t);
static off_t ramfile_lseek (int, off_t, int);

int highmemcpy(long long dst,long long src,long long count);
void mycacheflush(long long addrin,unsigned int size,unsigned int rw);

/*
 * Supported paths:
 *	/dev/ram@address
 *	/dev/ram@address,size
 *	/dev/ram/logger
 */


static int
   ramfile_open (fd, fname, mode, perms)
   int	       fd;
   const char *fname;
   int         mode;
   int	       perms;
{
	Ramfile *p;
	char            *dname;
      
	dname = (char *)fname;
	if (strncmp (dname, "/dev/ram", 8) == 0)
		dname += 8;

	if (dname[0] == '@') {
		u_int32_t address;
		int size;
		int nseperator = 0;
		int i;
		char *straddr;
		char *strsize = NULL;
		int flags=0;
		
		dname += 1;
		if(dname[0] == 'p')
		{
			dname +=1;
			flags=RAMFILE_CPHY;
		}
		straddr = dname;

		/*
		 * Check that string is correct
		 */
		for (i=0; i < strlen(dname); i++) {
			if (dname[i] == ',') {
				nseperator++;
				if (nseperator != 1) {
					return (-1);
				} else {
					dname[i] = 0;
					strsize = &dname[i+1];
					continue;
				}
			}
		}
		
		if (!get_rsa ((u_int32_t *)&address, straddr)) {
			return (-1);
		}
		
		if (strsize != NULL) {
#if 0
			if (!get_rsa ((u_int32_t *)&size, strsize)) {
				return (-1);
			}
#endif
			size=strtoul(strsize,0,0);
		} else {
			size = -1;
		}

		p = addRamFile(NULL, address, size, RAMFILE_DYNAMIC|flags);
		if (p == NULL)
			return (-1);

	} else if (dname[0] == '/') {
		int found = 0;
		dname += 1;
		
		LIST_FOREACH(p, &Ramfiles, i_next) {
			if(dname[0]==0)printf("name:%s size:%d @%08x\n",p->name,p->size,p->base);
			if(strcmp(p->name, dname) == 0) {
				found = 1;
				break;
			}	
		}
		
		if(!found) {
			return(-1);
		}
	} else {
		return (-1);
	}
	
	p->refs++;
	if(p->flags & RAMFILE_CPHY) mycacheflush(0x9800000000000000ULL|p->base,p->size,0);

	_file[fd].posn = 0;
	_file[fd].data = (void *)p;
	
	return (fd);
}

/** close(fd) close fd */
static int
   ramfile_close (fd)
   int             fd;
{
	Ramfile *p;

	p = (Ramfile *)_file[fd].data;
	p->refs--;

	if ((p->refs == 0) && (p->flags & RAMFILE_DYNAMIC)) {
		char tmpname[20];
		sprintf(tmpname,"%08x",p->base);
		deleteRamFile(tmpname);
		addRamFile(tmpname, p->base, _file[fd].posn, RAMFILE_STATIC);
	   LIST_REMOVE(p, i_next);
	   free(p);
	}
	   
	return(0);
}

/** read(fd,buf,n) read n bytes into buf from fd */
static int
   ramfile_read (fd, buf, n)
   int fd;
   void *buf;
   size_t n;
{
	Ramfile        *p;

	p = (Ramfile *)_file[fd].data;

	if (_file[fd].posn + n > p->size)
		n = p->size - _file[fd].posn;

	if(p->flags & RAMFILE_CPHY)
	highmemcpy(buf,0x9800000000000000ULL|(p->base+_file[fd].posn),n);
	else
	memcpy (buf, (char *)p->base + _file[fd].posn, n);
	_file[fd].posn += n;
      
	return (n);
}

static int
   ramfile_write (fd, buf, n)
   int fd;
   const void *buf;
   size_t n;
{
	Ramfile        *p;

	p = (Ramfile *)_file[fd].data;

	if (_file[fd].posn + n > p->size)
		n = p->size - _file[fd].posn;

	if(p->flags & RAMFILE_CPHY)
	highmemcpy(0x9800000000000000ULL|(p->base+_file[fd].posn),buf,n);
	else
	memcpy ((char *)p->base + _file[fd].posn, buf, n);
	_file[fd].posn += n;

	return (n);
}

/*************************************************************
 *  ramfile_lseek(fd,offset,whence)
 */
static off_t
ramfile_lseek (fd, offset, whence)
	int             fd, whence;
	off_t            offset;
{
	Ramfile        *p;

	p = (Ramfile *)_file[fd].data;

	switch (whence) {
		case SEEK_SET:
			_file[fd].posn = offset;
			break;
		case SEEK_CUR:
			_file[fd].posn += offset;
			break;
		case SEEK_END:
			_file[fd].posn = p->size + offset;
			break;
		default:
			errno = EINVAL;
			return (-1);
	}
	return (_file[fd].posn);
}

struct Ramfile *addRamFile(char *filename, unsigned long base, unsigned long size, int flags)
{
	struct Ramfile *rmp;

	rmp = (struct Ramfile *)malloc(sizeof(struct Ramfile));
	if (rmp == NULL) {
		fprintf(stderr, "Out of space adding Ramfile");
		return(NULL);
	}

	bzero(rmp, sizeof(struct Ramfile));
	rmp->base = base;
	rmp->size = size;
	rmp->flags = flags;
	strncpy(rmp->name, filename, sizeof(rmp->name));
	LIST_INSERT_HEAD(&Ramfiles, rmp, i_next);
	return(rmp);
}

int deleteRamFile(char *filename)
{
	struct Ramfile *rmp;

	LIST_FOREACH(rmp, &Ramfiles, i_next) {
		if(strcmp(rmp->name, filename) == 0) {
			if(rmp->refs == 0) {
				LIST_REMOVE(rmp, i_next);
				free(rmp);
				return(0);
			} else {
				return(-1);
			}
		}
	}
	return(-1);
}



static FileSystem ramfs =
{
	"ram", FS_MEM,
	ramfile_open,
	ramfile_read,
	ramfile_write,
	ramfile_lseek,
	ramfile_close,
	NULL
};

static void init_fs __P((void)) __attribute__ ((constructor));

static void
   init_fs()
{
	/*
	 * Install ram based file system.
	 */
	filefs_init(&ramfs);

#if NLOGFILE > 0	
	addRamFile("logger", 0x3000, 0x1000, RAMFILE_STATIC);
#endif
	addRamFile("vmlinux", 0xbc000000, 0x800000, RAMFILE_STATIC);
	addRamFile("vmlinux1", 0xc0200000, 0xa00000, RAMFILE_CPHY);
	addRamFile("usermem", (long)CLIENTPC,0x10000000, RAMFILE_STATIC);
}

