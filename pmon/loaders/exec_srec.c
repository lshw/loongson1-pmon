/* $Id: exec_srec.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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
 *	This product includes software developed by Patrik Lindergren.
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
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <exec.h>

#include <pmon.h>
#include <pmon/loaders/loadfn.h>

#include "mod_debugger.h"
#include "mod_load.h"
#include "mod_symbols.h"
#include "gzip.h"
#if NGZIP > 0
#include <gzipfs.h>
#endif /* NGZIP */

static long   load_srec (int fd, char *buf, int *n, int flags);

static long
   load_srec (int fd, char *buf, int *n, int flags)
{
	char *p;
	int count = 0;
	int l, stat;

#if NGZIP > 0
	gz_open(fd);
	*n = 0;
	if (gz_lseek (fd, 0, SEEK_SET) != 0 || 
	    gz_read (fd, (void *)buf, 1) != 1) {
		gz_close(fd);
		return -1;
	}
#else
	lseek(fd,*n,0);
	l = read(fd, (void *)buf, 1);
	if (l != 1)
		return -1;
	*n += l;
#endif /* NGZIP */
	
	if(buf[0] != 'S') {
#if NGZIP > 0
		gz_close(fd);
#endif /* NGZIP */
		return -1;
	}
	
	fprintf (stderr, "(srec)  ");
	
	p = &buf[1];
	while(1) {
		l = 0;
		
		while (1) {
			char c;

			if (read (fd, &c, 1) != 1) {
				/* Error reading */
#if NGZIP > 0
				gz_close(fd);
#endif /* NGZIP */
				return (-1);
			}
		
			if (c == '\r')
				continue;

			if (c == (char)EOF) {
				*p++ = 0;
				break;
			}

			if (c == '\n') {
				*p++ = 0;
				break;
			}
			*p++ = c;
		}
		
		stat = dl_s3load (buf, 0, &l, flags);
		if (stat != DL_CONT)
			break;
		count += l;
		p = buf;
	}

#if NGZIP > 0
	gz_close(fd);
#endif /* NGZIP */

	if (stat != DL_DONE) {
		printf("\n\nLoad ERROR: %s\n", dl_err(stat));
		return -2;
	}
	else {
		printf("\b\b, loaded %d bytes", count);
		return(dl_entry);
	}
}


/*************************************************************
 *  s3_datarec(p,plen,flags)
 *      handle an S-record data record
 */
static int
   s3_datarec (char *p, int *plen, int flags)
{
	int len, i, cs;
	int addr, v;

	*plen = 0;
	dl_chksum = 0;

	if (flags & YFLAG)		/* only load symbols? */
		return DL_CONT;

	if (!gethex (&len, &p[2], 2))
		return (DL_BADCHAR);

	dl_chksum += len;

	if (len * 2 != strlen (p) - 4)
		return (DL_BADLEN);

	i = 4;
	addr = 0;
	switch (p[1]) {
		case '3':
			if (!gethex (&v, &p[i], 2))
				return (DL_BADCHAR);
			addr = (addr << 8) + v;
			dl_chksum += v;
			i += 2;
		case '2':
			if (!gethex (&v, &p[i], 2))
				return (DL_BADCHAR);
			addr = (addr << 8) + v;
			dl_chksum += v;
			i += 2;
		case '1':
			if (!gethex (&v, &p[i], 2))
				return (DL_BADCHAR);
			addr = (addr << 8) + v;
			dl_chksum += v;
			if (!gethex (&v, &p[i + 2], 2))
				return (-1);
			addr = (addr << 8) + v;
			dl_chksum += v;
			break;
		default:
			return (DL_BADTYPE);
	}

	addr += dl_offset;
	len -= (i / 2) + 1;
	if (!dl_checksetloadaddr ((void *)addr, len, flags & VFLAG))
		return (DL_BADADDR);

	p = &p[i + 4];
	for (i = 0; i < len; i++, p += 2) {
		if (!gethex (&v, p, 2))
			return (DL_BADCHAR);
		store_byte ((void *)(addr++), v);
		dl_chksum += v;
	}

	if (!gethex (&cs, p, 2))
		return (DL_BADCHAR);
	if (!(flags & IFLAG) && cs != ((~dl_chksum) & 0xff))
		return (DL_BADSUM);

	*plen = i;
	return (DL_CONT);
}

#if NMOD_SYMBOLS > 0
/*
 *  s3_dlsym(p, flags)
 *      handle S4 records, i.e., symbols
 */
static int
   s3_dlsym (char *p, int flags)
{
	int32_t         adr;
	int             len, csum;
	char            name[LINESZ], *t;

/* 
 * S4LLAAAAAAAANNNNNNNN,AAAAAAAANNNNNN,AAAAAAAANNNNNNNNNN,CC
 * LL=length AAAAAAAA=addr NNNN,=name CC=checksum 
 */

	if (flags & NFLAG)		/* no symbols? */
		return (DL_CONT);

	p += 2;			/* skip past S4 */
	if (!gethex (&len, p, 2))
		return (DL_BADCHAR);
	p += 2;			/* skip past length */
	for (; len > 2;) {
		if (!gethex (&adr, p, 8))
			return (DL_BADCHAR);
		p += 8;			/* skip past addr */
		len -= 8;

		t = strchr (p, ',');
		if (t == 0)
			return (DL_BADCHAR);
		strncpy (name, p, t - p);
		name[t - p] = '\0';
		len -= t - p;

		if (!newsym (name, (flags & AFLAG) ? adr : adr + dl_offset))
			return (DL_NOSYMSPACE);

		p = t + 1;
	}
	if (!gethex (&csum, p, 2))
		return (DL_BADCHAR);

    /* csum neither generated nor checked */
	return (DL_CONT);
}
#endif /* NMOD_SYMBOLS */

/*
 *  s3_endrec(p)
 *      handle the S-record termination record
 */
static int
   s3_endrec (char *p)
{
	int32_t            adr;

	if (gethex (&adr, &p[4], ('9' - p[1] + 2) * 2))
		dl_entry = adr + dl_offset;
	else
		dl_entry = 0xdeadbeef;

    /* always return DL_DONE regardless of record errors */
	return DL_DONE;
}

/*
 *  dl_s3load(recbuf,plen,flags)
 *      load Motorola S-records
 */
int
   dl_s3load (char *recbuf, int recsize, int *plen, int flags)
{
	*plen = 0;

	if (recbuf[0] != 'S')
		return DL_BADCHAR;

	if (recbuf[1] == '0')
		return (DL_CONT);

	if (recbuf[1] >= '1' && recbuf[1] <= '3')
		return (s3_datarec (recbuf, plen, flags));

#if NMOD_SYMBOLS
	if (recbuf[1] == '4')
		return (s3_dlsym (recbuf, flags));
#endif

	if (recbuf[1] >= '7' && recbuf[1] <= '9')
		return (s3_endrec (recbuf));

	return (DL_BADTYPE);
}


static ExecType srec_exec =
{
	"srec",
	load_srec,
	EXECFLAGS_NONE,
};


static void init_exec __P((void)) __attribute__ ((constructor));

static void
   init_exec()
{
	/*
	 * Install ram based file system.
	 */
	exec_init(&srec_exec);
}

