/* $Id: loadfn.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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
/*
 * S3 & LSI support functions
 */

#include <stdio.h>
#include <string.h>

#include <pmon.h>
#include "loadfn.h"

#include "mod_debugger.h"
#include "mod_load.h"
#include "mod_symbols.h"

static const char *dlerrs[] = {
    "continue(!)",		/* DL_CONT */
    "done(!)",			/* DL_DONE */
    "bad char",			/* DL_BADCHAR */
    "bad length",		/* DL_BADLEN */
    "bad type",			/* DL_BADTYPE */
    "bad checksum",		/* DL_BADSUM */
    "out of symbol space",	/* DL_NOSYMSPACE */
    "bad load address",		/* DL_BADADDR */
    "i/o error"			/* DL_IOERR */
};
#define NERRS	(sizeof(dlerrs)/sizeof(dlerrs[0]))

long	dl_entry;	/* entry address */
long	dl_offset;	/* load offset */
long	dl_minaddr;	/* minimum modified address */
long	dl_maxaddr;	/* maximum modified address */

int	dl_chksum;


char const *
dl_err (int code)
{
    if (code >= DL_MAX)
	return "unknown";
    return dlerrs[code];
}

static __inline int overlap __P((u_long, u_long, u_long, u_long));

static __inline int
overlap (as, ae, bs, be)
    unsigned long as, ae, bs, be;
{
    return (ae > bs && be > as);
}

/*
 *  Check that virtual address is valid load range and set limits.
 */
int 
dl_checksetloadaddr (void *vaddr, int size, int verbose)
{
	unsigned long addr = (unsigned long)vaddr;

	if (!dl_checkloadaddr(vaddr, size, verbose))
		return (0);

	if (addr < dl_minaddr)
		dl_minaddr = addr;
	if (addr + size > dl_maxaddr)
		dl_maxaddr = addr + size;

	return 1;
}

/*
 *  Check that virtual address is valid load range, don't  set limits.
 */
int 
dl_checkloadaddr (void *vaddr, int size, int verbose)
{
	unsigned long addr = (unsigned long)vaddr;

	if (md_valid_load_addr(addr, addr + size)) {
		if (verbose)
			printf ("\nattempt to load into mappable region");
		return 0;
	}
	return 1;
}

void
dl_initialise (unsigned long offset, int flags)
{
#if NMOD_SYMBOLS > 0
    if (!(flags & SFLAG))
	clrsyms ();
#endif
#if NMOD_DEBUGGER > 0
    if (!(flags & BFLAG))
	clrbpts ();
#endif
    if (!(flags & EFLAG))
	clrhndlrs ();

    dl_minaddr = ~0;
    dl_maxaddr = 0;
    dl_chksum = 0;
    dl_offset = offset;
}

#if NMOD_SYMBOLS > 0
void
dl_setloadsyms (void)
{
    defsyms (dl_minaddr, dl_maxaddr, (u_int32_t)md_getpc(NULL));
}
#endif


#if NMOD_FASTLOAD > 0
/*
 *  LSI Fastload format.
 */

#define __ 64

static const unsigned char etob[128] = {
	__,	__,	__,	__,	__,	__,	__,	__,
	__,	__,	__,	__,	__,	__,	__,	__,
	__,	__,	__,	__,	__,	__,	__,	__,
	__,	__,	__,	__,	__,	__,	__,	__,
	__,	__,	__,	__,	__,	__,	__,	__,
	__,	__,	__,	__,	62,	__,	63,	__,
	52,	53,	54,	55,	56,	57,	58,	59,
	60,	61,	__,	__,	__,	__,	__,	__,
	__,	0,	1,	2,	3,	4,	5,	6,
	7,	8,	9,	10,	11,	12,	13,	14,
	15,	16,	17,	18,	19,	20,	21,	22,
	23,	24,	25,	__,	__,	__,	__,	__,
	__,	26,	27,	28,	29,	30,	31,	32,
	33,	34,	35,	36,	37,	38,	39,	40,
	41,	42,	43,	44,	45,	46,	47,	48,
	49,	50,	51,	__,	__,	__,	__,	__
};

#undef __

long   		nextaddr;
int             blkinx = 0;
int             blksz = 0;

/*
 *  getb64(vp,p,n)
 *      get a number of base-64 digits
 *      if n < 0 then don't add value to chksum.
 */
static int
getb64 (unsigned long *vp, char *p, int n)
{
    unsigned long   v, b;
    unsigned i;

    v = 0;
    for (i = ((n < 0) ? 0 - n : n); i != 0; i--) {
	b = etob[*p++ & 0x7f];
	if (b == 64) {
	    *vp = 0;
	    return (0);
	}
	v = (v << 6) | b;
    }

    /* update 12 bit checksum */
    switch (n) {
    case 6:
	dl_chksum += (v >> 24) & 0xfff;
    case 4:
	dl_chksum += (v >> 12) & 0xfff;
    case 2:
	dl_chksum += v & 0xfff;
    }

    *vp = v;
    return (1);
}


/*
 *  getrec(p,curblk)
 *      get the next 4-byte record (if there's any left)
 */
static __inline char *getrec __P((char *));

static __inline char *
getrec (curblk)
     char           *curblk;
{
    char *p;
    if (blkinx >= blksz)
	return (0);
    p = &curblk[blkinx];
    blkinx += 4;
    return (p);
}


/*
 *  dl_fastload(recbuf,offset,plen,flags)
 *      load a fast-mode record
 */
int
dl_fastload (char *recbuf, int recsize, int *plen, int flags)
{
    char	    *rp;

    int             i, len, s;
#if NMOD_SYMBOLS > 0
    int             j, c;
    char            name[LINESZ];
#endif
    unsigned long   bdat, zcnt;
    long   	    addr;

    *plen = 0;

    if (recsize % 4 != 0)
	return (DL_BADLEN);

    blksz = recsize;
    blkinx = 0;

    s = DL_CONT;
    for (len = 0;;) {
	if (!(rp = getrec (recbuf)))
	    break;
	if (rp[0] == '/') {
	    switch (rp[1]) {
	    case 'K':		/* klear checksum (sic) */
		dl_chksum = 0;
		break;
	    case 'C':		/* compare checksum */
		if (!getb64 (&zcnt, &rp[2], -2))
		    return (DL_BADCHAR);
		dl_chksum &= 0xfff;
		if (!(flags & IFLAG) && zcnt != dl_chksum)
		    return (DL_BADSUM);
		dl_chksum = 0;
		break;
	    case 'Z':		/* zero fill */
		if (!getb64 (&zcnt, &rp[2], 2))
		    return (DL_BADCHAR);
		zcnt *= 3;
		if (flags & YFLAG) { /* symbols only? */
		    nextaddr += zcnt;
		    break;
		}
		if (!dl_checksetloadaddr ((void *)nextaddr, zcnt, flags & VFLAG))
		    return (DL_BADADDR);
		len += zcnt;
		while (zcnt--)
		    store_byte ((void *)(nextaddr++), 0);
		break;
	    case 'B':		/* byte */
		if (!getb64 (&bdat, &rp[2], 2))
		    return (DL_BADCHAR);
		if (flags & YFLAG) { /* symbols only? */
		    nextaddr++;
		    break;
		}
		if (!dl_checksetloadaddr ((void *)nextaddr, 1, flags & VFLAG))
		    return (DL_BADADDR);
		store_byte ((void *)(nextaddr++), bdat);
		len++;
		break;
	    case 'A':		/* address */
		if (!getrec (recbuf)) /* get another 4 bytes */
		    return (DL_BADLEN);
		if (!getb64 (&addr, &rp[2], 6))
		    return (DL_BADCHAR);
#if 0 /*XXX Algor special format... */
		if ((flags & TFLAG) && Gpr[R_A3] == 0) {
		    memorysize -= addr;
		    Gpr[R_SP] = clienttos ();
		    nextaddr = memorysize + K0BASE;
		    Gpr[R_A3] = nextaddr;
		} else
#endif
		    nextaddr = addr + dl_offset;
		break;
	    case 'E':		/* end */
		dl_entry = nextaddr;
		s = DL_DONE;
		break;
#if NMOD_SYMBOLS > 0
	    case 'S':		/* symbol */
		i = 2; j = 0;
		while (1) {
		    c = rp[i++];
		    /* dl_chksum += c; */
		    if (c == ',')
			break;
		    name[j++] = c;
		    if (i >= 4) {
			if (!(rp = getrec (recbuf)))
			    return (DL_BADLEN);
			i = 0;
		    }
		}
		name[j] = '\0';
		if (flags & NFLAG) /* no symbols? */
		    break;
		if (!newsym (name, nextaddr))
		    return (DL_NOSYMSPACE);
		break;
#endif /* NMOD_SYMBOLS */
	    default:
		return (DL_BADTYPE);
	    }
	} else {		/* 24 bits of data */
	    if (!getb64 (&bdat, rp, 4))
		return (DL_BADLEN);
	    if (flags & YFLAG) {	/* symbols only? */
		nextaddr += 3;
		break;
	    }
	    if (!dl_checksetloadaddr ((void *)nextaddr, 3, flags & VFLAG))
		return (DL_BADADDR);
	    for (i = 16; i >= 0; i -= 8) {
		store_byte ((void *)(nextaddr++), bdat >> i);
	    }
	    len += 3;
	}
    }
    *plen = len;
    return (s);
}
#endif /* NMOD_FASTLOAD */
