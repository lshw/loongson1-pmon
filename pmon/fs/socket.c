/* $Id: socket.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */
/*
 * Copyright (c) 2000-2002 Opsycon AB  (www.opsycon.se)
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
#include <stdio.h>
#include <file.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/socket.h>

static int socket_close (int);
static int socket_read (int, void *, size_t);
static int socket_write (int, const void *, size_t);
static int socket_ioctl (int fd, unsigned long op, ...);

extern int soc_close (int);
extern int soc_read (int, void *, size_t);
extern int soc_write (int, const void *, size_t);
extern int soc_ioctl (int fd, unsigned long op, ...);


static FileSystem socketfs =
{
	"socket", FS_SOCK,
	NULL,
	socket_read,
	socket_write,
	NULL,
	socket_close,
	socket_ioctl
};


/** close(fd) close fd */
static int
   socket_close (fd)
   int             fd;
{
	return(soc_close(fd));
}

/** read(fd,buf,n) read n bytes into buf from fd */
static int
   socket_read (fd, buf, n)
		int fd;
		void *buf;
		size_t n;
{
	return (soc_read(fd, buf, n));
}

static int
   socket_write (fd, buf, n)
   int fd;
const void *buf;
size_t n;
{
	return (soc_write(fd, buf, n));
}

int
   socket_ioctl (int fd, unsigned long op, ...)
{
	void *argp;
	va_list ap;

	va_start(ap, op);
	argp = va_arg(ap, void *);
	va_end(ap);

	return(soc_ioctl(fd, op, argp));
}


static void init_fs __P((void)) __attribute__ ((constructor));

static void
   init_fs()
{
	/*
	 * Install ram based file system.
	 */
	filefs_init(&socketfs);
}

