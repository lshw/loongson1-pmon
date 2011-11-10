/* $Id: netio.h,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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

#ifndef __NETIO_H__
#define __NETIO_H__

#include <sys/queue.h>
#include <url.h>

typedef struct NetFileOps {
	char		*protocol;
	int		(*fo_open)	(int, struct Url *, int, int);
	int		(*fo_read)	(int, void *, int);
	int		(*fo_write)	(int, const void *, int);
	off_t		(*fo_lseek)	(int, long, int);
	int		(*fo_ioctl)	(int, int, void *);
	int		(*fo_close)	(int);
	SLIST_ENTRY(NetFileOps)	i_next;
	
} NetFileOps;

typedef struct NetFile {
	NetFileOps	*ops;
	void		*data;
} NetFile;

extern int	netopen (int, const char *, int, int);
extern int	netread (int, void *, size_t);
extern int	netwrite (int, const void *, size_t);
extern off_t	netlseek (int, off_t, int);
extern int	netioctl (int, unsigned long, ...);
extern int	netclose (int);
extern int	netfs_init (NetFileOps *fs);

#endif /* __NETIO_H__ */