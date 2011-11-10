/* $Id: netio.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */
/*
 * Copyright (c) 2001-2002 Opsycon AB  (www.opsycon.se / www.opsycon.com)
 * Copyright (c) 2001-2002 Patrik Lindergren  (www.lindergren.com)
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
 *	This product includes software developed by Opsycon AB, Sweden.
 *	This product includes software developed by Patrik Lindergren, Sweden.
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
#include <sys/param.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/queue.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <file.h>

#include "netio.h"

extern int errno;

SLIST_HEAD(NetFileSystems, NetFileOps) NetFileSystems = SLIST_HEAD_INITIALIZER(NetFileSystems);

int
netfs_init(NetFileOps *fs)
{
	SLIST_INSERT_HEAD(&NetFileSystems, fs, i_next);
	return(0);
}

int netopen (int fd, const char *path, int flags, int perms)
{
	NetFileOps *ops;
	struct	Url url;
	NetFile *nfp;
	char *dname = (char *)path;

	if (strncmp (dname, "/dev/net/", 9) == 0) {
		dname += 9;
	}

	if(parseUrl(dname, &url) == -1) {
		return(-1);
	}

	SLIST_FOREACH(ops, &NetFileSystems, i_next) {

		if (strcmp (url.protocol, ops->protocol) == 0) {

		      nfp = (NetFile *)malloc(sizeof(NetFile));
		      if (nfp == NULL) {
			    fprintf(stderr, "Out of space for allocating a NetFile");
			    return(-1);
		      }
		      nfp->ops = ops;
		      _file[fd].data = (void *)nfp;
		      if((*(ops->fo_open)) (fd, &url, flags, perms) != 0) {
			    free(nfp);
			    return(-1);
		      }
		}
	}
	return (fd);
}


int netread (int fd, void *buf, size_t nb)
{
	NetFile *nfp;

	nfp = (NetFile *)_file[fd].data;

	if (nfp->ops->fo_read)
		return ((nfp->ops->fo_read) (fd, buf, nb));
	else {
		return (-1);
	}
}

int netwrite (int fd, const void *buf, size_t nb)
{
	NetFile *nfp;

	nfp = (NetFile *)_file[fd].data;

	if (nfp->ops->fo_write)
		return ((nfp->ops->fo_write) (fd, buf, nb));
	else
		return (-1);
}

off_t netlseek (int fd, off_t offs, int how)
{
	NetFile *nfp;

	nfp = (NetFile *)_file[fd].data;

	if (nfp->ops->fo_lseek)
		return ((nfp->ops->fo_lseek) (fd, offs, how));
	else
		return (-1);
}

int netioctl (int fd, unsigned long op, ...)
{
	NetFile *nfp;
	void *argp;
	va_list ap;

	va_start(ap, op);
	argp = va_arg(ap, void *);
	va_end(ap);

	nfp = (NetFile *)_file[fd].data;
	
	if (nfp->ops->fo_ioctl)
		return ((nfp->ops->fo_ioctl) (fd, op, argp));
	else
		return (-1);
}

int netclose (int fd)
{
	NetFile *nfp;

	nfp = (NetFile *)_file[fd].data;
	
	if (nfp->ops->fo_close)
		return ((nfp->ops->fo_close) (fd));
	else
		return (-1);
}


static FileSystem netfs =
{
	"net", FS_NET,
	netopen,
	netread,
	netwrite,
	netlseek,
	netclose,
	netioctl
};

static void init_fs __P((void)) __attribute__ ((constructor));

static void
   init_fs()
{
	/*
	 * Install ram based file system.
	 */
	filefs_init(&netfs);
}
