/* $Id: ecoff.h,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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
/* Standard COFF File Header */
struct filehdr {
    unsigned short	f_magic;	/* magic number */
    unsigned short	f_nscns;	/* number of sections */
    long		f_timdat;	/* time & date stamp */
    long		f_symptr;	/* file pointer to symbolic header */
    long		f_nsyms;	/* sizeof(symbolic hdr) */
    unsigned short	f_opthdr;	/* sizeof(optional hdr) */
    unsigned short	f_flags;	/* flags */
};
#define	FILHSZ	sizeof(struct filehdr)

#define MIPSEBMAGIC	 0x160
#define MIPSELMAGIC	 0x162

#define MIPS2EBMAGIC	 0x163
#define MIPS2ELMAGIC	 0x166

#define MIPS3EBMAGIC	 0x140
#define MIPS3ELMAGIC	 0x142

#include <sys/endian.h>
#if BYTE_ORDER==BIG_ENDIAN
#define MIPSMAGIC	MIPSEBMAGIC
#define MIPS2MAGIC	MIPS2EBMAGIC
#define MIPS3MAGIC	MIPS3EBMAGIC
#else
#define MIPSMAGIC	MIPSELMAGIC
#define MIPS2MAGIC	MIPS2ELMAGIC
#define MIPS3MAGIC	MIPS3ELMAGIC
#endif

#define MIPS1_OK(x) ((x).f_magic == MIPSMAGIC)

#define MIPS2_OK(x) ((x).f_magic == MIPS2MAGIC)

#define MIPS3_OK(x) ((x).f_magic == MIPS3MAGIC)

#if CPU_R3000
#define MIPS_OK(x) MIPS1_OK(x)
#elif CPU_R4000
#define MIPS_OK(x) (MIPS1_OK(x) || MIPS2_OK(x) || MIPS3_OK(x))
#else
#define MIPS_OK(x) 0
#endif

/* MIPS a.out header */
struct aouthdr {
    short	magic;
    short	vstamp;		/* version stamp */
    long	tsize;		/* text size */
    long	dsize;		/* data size */
    long	bsize;		/* bss size */
    long	entry;		/* entry point */
    long	text_start;	/* text base address */
    long	data_start;	/* data base address */
    long	bss_start;	/* bss base address */
    long	dummy[6];
};
#define AOUTHSZ	sizeof(struct aouthdr)

#define	OMAGIC		0x0107
#define	NMAGIC		0x0108
#define	ZMAGIC		0x010b

#define	SCNHSZ		40

#define N_TXTOFF(f, a) \
 ((a).magic == ZMAGIC ? 0 : ((a).vstamp < 23 ? \
   ((FILHSZ + AOUTHSZ + (f).f_nscns * SCNHSZ + 7) & ~7) : \
   ((FILHSZ + AOUTHSZ + (f).f_nscns * SCNHSZ + 15) & ~15) ) )

#define	N_BADMAG(x) \
    (((x).magic)!=OMAGIC && ((x).magic)!=NMAGIC && ((x).magic)!=ZMAGIC)
