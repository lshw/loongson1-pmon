/* $Id: loadfn.h,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2001-2002 Opsycon AB  (www.opsycon.se / www.opsycon.com)
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
#define DLREC	550		/* max download record size */

#define SFLAG 0x0001		/* Don't clear symbols */
#define BFLAG 0x0002		/* Don't clear breakpoints */
#define EFLAG 0x0004		/* Don't clear exceptions */
#define AFLAG 0x0008		/* Don't add offset to symbols */
#define TFLAG 0x0010		/* Load at top of memory */
#define IFLAG 0x0020		/* Ignore checksum errors */
#define VFLAG 0x0040		/* verbose */
#define FFLAG 0x0080		/* Load into flash */
#define NFLAG 0x0100		/* Don't load symbols */
#define YFLAG 0x0200		/* Only load symbols */
#define WFLAG 0x0400		/* Swapped endianness */
#define UFLAG 0x0800		/* PMON upgrade flag */
#define KFLAG 0x1000		/* Load symbols for kernel debugger */ 
#define RFLAG 0x2000
#define OFLAG 0x4000
#define ZFLAG 0x8000

#define DL_CONT		0
#define DL_DONE		1
#define DL_BADCHAR	2
#define DL_BADLEN	3
#define DL_BADTYPE	4
#define DL_BADSUM	5
#define DL_NOSYMSPACE	6
#define DL_BADADDR	7
#define DL_IOERR	8
#define DL_MAX		9

char const	*dl_err (int code);
int		dl_checkloadaddr (void *addr, int size, int verbose);
int		dl_checksetloadaddr (void *addr, int size, int verbose);
void		dl_initialise (unsigned long offset, int flags);
void		dl_setloadsyms (void);
int		dl_s3load (char *recbuf, int recsize, int *plen, int flags);
int		dl_fastload (char *recbuf, int recsize, int *plen, int flags);

extern long	dl_entry;	/* entry address */
extern long	dl_offset;	/* load offset */
extern long	dl_minaddr;	/* minimum modified address */
extern long	dl_maxaddr;	/* maximum modified address */
extern int	dl_chksum;

