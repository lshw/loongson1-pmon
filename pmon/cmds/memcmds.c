/*	$Id: memcmds.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2000-2001 Opsycon AB  (www.opsycon.se)
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

#include <stdio.h>
#include <termio.h>
#include <string.h>
#include <setjmp.h>
#include <sys/endian.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif

#include <pmon.h>

static void * dispmem __P((char *, void *, int, int));

int	cmd_modify __P((int, char *[])); 
int	cmd_search __P((int, char *[])); 
int	cmd_dump __P((int, char *[])); 
int	cmd_fill __P((int, char *[])); 
int	cmd_copy __P((int, char *[])); 
int	cmd_compare __P((int, char *[])); 

static char           *searching = "searching..  ";
char           *badhexsym = "%s: neither hex value nor symbol\n";

static __inline void
era_line(char *p)
{
	while(*p++)
		printf("\b \b");
}

const Optdesc m_opts[] =
{
	{"-b", "access bytes"},
	{"-h", "access half-words"},
	{"-w", "access words"},
	{"-x", "swap bytes"},
#ifdef HAVE_QUAD
	{"-d", "access double-words"},
#endif
	{"-n", "non-interactive (no write)"},
	{"Interactive Options", ""},
	{"<hexval>", "set memory, forward one"},
	{"CR", "forward one, no change"},
	{"=", "re-read"},
	{"^|-", "back one"},
	{".", "quit"},
	{0}};

/** cmd_modify(ac,av), the 'm' command */

int
cmd_modify (ac, av)
	int ac;
	char *av[];
{
	int32_t	adr;
	register_t v;
	char *p;
	int datasz = 0;
	int nowrite = 0;
	int reverse = 0;
	int c;

	optind = 0;
	while ((c = getopt (ac, av, "dwhbnx")) != EOF) {
		switch (c) {
#ifdef HAVE_QUAD
		case 'd':
			datasz |= 8;
			break;
#endif
		case 'w':
			datasz |= 4;
			break;

		case 'h':
			datasz |= 2;
			break;

		case 'b':
			datasz |= 1;
			break;

		case 'n':
			nowrite = 1;
			break;

		case 'x':
			reverse = 1;
			break;

		default:
			return (-1);
		}
	}
	if((datasz & (datasz - 1)) != 0) {
		printf ("multiple data types specified\n");
		return(-1);
	}

	if (optind >= ac) {
		return (-1);
	}

	if (!get_rsa (&adr, av[optind++])) {
		return (-1);
	}

	if (datasz == 0) {
		datasz = 1 << matchenv ("datasize");
	}

	if (optind < ac) {
	/* command mode */
		for (; optind < ac; optind++) {
			if (!strcmp(av[optind], "-s")) {
				if (++optind >= ac) {
					printf ("bad arg count\n");
					return (-1);
				}
				for (p = av[optind]; *p; p++) {
					store_byte ((void *)(adr++), *p);
				}
			}
			else {
				if (!get_rsa_reg (&v, av[optind])) {
					return (-1);
				}
				if (adr & (datasz - 1)) {
					printf ("%08x: unaligned address\n", adr);
					return (1);
				}
				switch (datasz) {
				case 1:
					store_byte ((void *)adr, v);
					break;

				case 2:
					if(reverse) {
						v = swap16(v);
					}
					store_half ((void *)adr, v);
					break;

				case 4:
					if(reverse) {
						v = swap32(v);
					}
					store_word ((void *)adr, v);
					break;
#ifdef HAVE_QUAD
				case 8:
					/* XXX FIXME SWAP */
					store_dword ((void *)adr, v);
					break;
#endif
				}
				adr += datasz;
			}
		}
	}
	else {
	/* interactive mode */
		if (adr & (datasz - 1)) {
			printf ("%08x: unaligned address\n", adr);
			return (1);
		}
		for (;;) {
			switch (datasz) {
			case 1:
				v = *(u_int8_t *)adr;
				break;

			case 2:
				v = *(u_int16_t *)adr;
				if(reverse) {
					v = swap16(v);
				}
				break;

			case 4:
				v = *(u_int32_t *)adr;
				if(reverse) {
					v = swap32(v);
				}
				break;
#ifdef HAVE_QUAD
			case 8:
				/* XXX FIXME SWAP */
				v = *(u_int64_t *)adr;
				break;
#endif
			}
#ifdef HAVE_QUAD
			printf ("%08x %0*llx ", adr, 2 * datasz, v);
#else
			printf ("%08x %0*lx ", adr, 2 * datasz, v);
#endif
			if (nowrite) {
				printf ("\n");
				break;
			}

			line[0] = '\0'; get_line (line, 0);
			for (p = line; *p == ' '; p++);
			if (*p == '.') {
				break;
			}
			else if (*p == '\0') {
				adr += datasz;
			}
			else if (*p == '^' || *p == '-') {
				adr -= datasz;
			}
			else if (*p == '=') {
				/* reread */;
			}
			else if (get_rsa_reg (&v, p)) {
				switch (datasz) {
				case 1:
					store_byte ((void *)adr, v);
					break;
				case 2:
					if(reverse) {
						v = swap16(v);
					}
					store_half ((void *)adr, v);
					break;
				case 4:
					if(reverse) {
						v = swap32(v);
					}
					store_word ((void *)adr, v);
					break;
#ifdef HAVE_QUAD
				case 8:
					/* XXX FIXME SWAP */
					store_dword ((void *)adr, v);
					break;
#endif
				}
				adr += datasz;
			}
		}
	}
	return (0);
}


const Optdesc search_opts[] =
{
	{"-s", "next arg is string"},
	{"", "match pattern can be combined from several"},
	{"", "match parameters like '0x13 -s Hi 0x13'"},
	{0,0}
};

/** cmd_search(ac,av), the search command */
int
cmd_search (ac, av)
	int ac;
	char *av[];
{
	u_int32_t from, to, adr, i, a;
	u_int8_t *s, *d, c;
	u_int8_t pat[PATSZ];
	int	siz, ln;

	ln = siz = moresz;
	ioctl (STDIN, CBREAK, NULL);

	if (!get_rsa (&from, av[1]) || !get_rsa (&to, av[2])) {
		return (-1);
	}

	for (d = pat, i = 3; i < ac; i++) {
		if (!strcmp(av[i], "-s")) {
			if (++i >= ac) {
				printf ("bad arg count\n");
				return (-1);
			}
			for (s = av[i]; *s; s++) {
				*d++ = *s;
			}
		}
		else {
			if (!get_rsa (&a, av[i])) {
				return (-1);
			}
			c = a;
			*d++ = c;
		}
	}

	if (to <= from) {
		printf ("'to' address too small\n");
		return (1);
	}

	printf ("%s", searching);
	if (from <= to) {		/* forward search */
		to -= d - pat - 1;
		while (from <= to) {
			s = pat;
			adr = from++;
			while (s < d) {
				if (*s != load_byte (adr)) {
					break;
				}
				s++;
				adr++;
			}
			if (d <= s) {
				era_line (searching);
				dispmem (prnbuf, (void *)(from - 1), 4, 0);
				if (more (prnbuf, &ln, siz)) {
					break;
				}
				printf ("%s", searching);
			}
			else {
				dotik (1, 0);
			}
		}
		if (from > to)
			era_line (searching);
	}
	else {			/* backward search */
		from -= d - pat - 1;
		while (to <= from) {
			s = pat;
			adr = from--;
			while (s < d) {
				if (*s != load_byte (adr)) {
					break;
				}
				s++;
				adr++;
			}
			if (d <= s) {
				era_line (searching);
				dispmem (prnbuf, (void *)(from - 1), 4, 0);
				if (more (prnbuf, &ln, siz)) {
					break;
				}
				printf ("%s", searching);
			}
		}
		if (to > from) {
			era_line (searching);
		}
	}
	return (0);
}

/*
 *  Functions to store data in memory. Always use these functions
 *  to store data that are potential instructions since these
 *  functions will guarantee I/D cache - memory integrity.
 */
void
store_dword (adr, v)
    void	*adr;
    int64_t	v;
{
    *(int64_t *)adr = v;
    flush_cache (IADDR, adr);
    flush_cache (IADDR, adr+4);
}

/** cmd_compare(ac,av), the 'comapre' command */
int
cmd_compare(ac, av)
	int ac;
	char *av[];
{
	u_int32_t from, to, with;

	if (!get_rsa (&from, av[1]) ||
	    !get_rsa (&to, av[2]) ||
	    !get_rsa (&with, av[3])) {
		return (-1);
	}

	while(from <= to) {
		if(*(char *)from != *(char *)with) {
			printf("%08x: %02x != %02x (%02x)\n", from,
				*(char *)from, *(char *)with,
				*(char *)from ^ *(char *)with);
		}
		from++;
		with++;
	}

	return (0);
}
/** cmd_copy(ac,av), the 'copy' command */
int
cmd_copy(ac, av)
	int ac;
	char *av[];
{
	u_int32_t from, to, n;

	if (!get_rsa (&from, av[1]) ||
	    !get_rsa (&to, av[2]) ||
	    !get_rsa (&n, av[3])) {
		return (-1);
	}

	bcopy((void *)from, (void *)to, n);

	flush_cache (DCACHE, 0);
	flush_cache (ICACHE, 0);
	return (0);
}

const Optdesc         d_opts[] =
{
    {"-b", "display as bytes"},
    {"-h", "display as half-words"},
    {"-w", "display as words"},
#ifdef HAVE_QUAD
    {"-d", "display as double-words"},
#endif
    {"-s", "display a null terminated string"},
    {"-x", "swap bytes"},
    {"-r<reg>", "display as register"},
    {0}};

/** cmd_dump(ac,av), the 'd' command */
int
cmd_dump (int ac, char *av[])
{
	void	*adr;
	int32_t	x;
	char	*reg;
	int	siz, ln, i, c;
	int	datasz = 0;
	int	reverse = 0;
static void *last_adr;
extern int optind;
extern char *optarg;

	optind = 0;
	while ((c = getopt (ac, av, "dwhbsxr:")) != EOF) {
		switch (c) {
		case 's':
			datasz |= 16;
			break;

#ifdef HAVE_QUAD
		case 'd':
			datasz |= 8;
			break;
#endif

		case 'w':
			datasz |= 4;
			break;

		case 'h':
			datasz |= 2;
			break;

		case 'b':
			datasz |= 1;
			break;

		case 'x':
			reverse = 1;
			break;

		case 'r':
			reg = optarg;
			datasz = -1;
			break;

		default:
			return (-1);
		}
	}
	if((datasz & (datasz - 1)) != 0) {
		printf ("multiple data types specified\n");
		return(-1);
	}

	/* get <addr> */
	if (optind >= ac || !get_rsa ((u_int32_t *)&adr, av[optind++])) {
		return (-1);
	}

	if( repeating_cmd ) {
		adr = last_adr;
	}
    
	/* get [<size>|<reg>] */
	if (optind < ac) {
		if (get_rsa (&x, av[optind])) {
		/* size */
			optind++;
			ln = x; siz = 0;
		}
		else if (md_getregaddr (0, av[optind])) {
			if (datasz != 0) {
				printf ("multiple data types specified\n");
				return (-1);
			}
			reg = av[optind++];
			datasz = -1;
		}
		else {
			return (-1);
		}
	}
	else {
		ln = siz = moresz;
	}

	if (optind != ac) {
		return (-1);
	}

	if (datasz == 16) {		/* -s, print string */
		strncpy (prnbuf, adr, 70);
		prnbuf[70] = '\0';
		for (i = 0; prnbuf[i] != 0; i++) {
			if (!isprint (prnbuf[i])) {
				prnbuf[i] = '.';
			}
		}
		printf ("%s\n", prnbuf);
		return (0);
	}

	ioctl (STDIN, CBREAK, NULL);
	if (datasz == -1) { /* -r<reg> print as register */
		if (!md_disp_as_reg ((register_t *)adr, reg, &ln)) {
			printf ("%s: bad register name\n", reg);
			return (-1);
		}
	}
	else {
		if (datasz == 0) {
			datasz = 1 << matchenv ("datasize");
		}
		while (1) {
			last_adr = adr;
			adr = dispmem (prnbuf, (void *)adr, datasz, reverse);
			if (more (prnbuf, &ln, siz)) {
				break;
			}
		}
	}
	return (0);
}

static void * 
dispmem(char *p, void *adr, int siz, int reverse)
{
	int i;
	char v;
	char tmp[18];
	union {
		double		dummy;	/* ensure 8-byte alignment */
		unsigned char	b[16];
	} buf;
	void *w;
	int val;

	w = adr;
	for (i = 0; i < 16; i++) {
		v = load_byte (w++);
		buf.b[i] = v;
	}

	sprintf (p, "%08x  ", adr);
	for (i = 0; i < 16; i += siz) {
		if (i == 8) {
			strccat (p, ' ');
		}
		switch (siz) {
		case 1:
			sprintf (tmp, "%02x ", *(unsigned char *)&buf.b[i]);
			break;
		case 2:
			val = *(u_int16_t *)&buf.b[i];
			if(reverse) {
				val = swap16(val);
			}
			sprintf (tmp, "%04x ", val);
			break;
		case 4:
			val = *(u_int32_t *)&buf.b[i];
			if(reverse) {
				val = swap32(val);
			}
			sprintf (tmp, "%08x ", val);
			break;
#ifdef HAVE_QUAD
		case 8:
			/* XXX FIXME SWAP */
			sprintf (tmp, "%016llx ", *(unsigned long long *)&buf.b[i]);
			break;
#endif
		}
		strcat (p, tmp);
	}
	strcat (p, "  ");
	for (i = 0; i < 16; i++) {
		v = buf.b[i];
		strccat (p, isprint (v) ? v : '.');
	}
	return (adr + 16);
}

const Optdesc fill_opts[] =
{
	{"-s", "next arg is string"},
	{"", "fill pattern can be combined from several"},
	{"", "parameters like '0x13 -s Hi 0x13'"},
	{0,0}
};

/** cmd_fill(ac,av) the fill command */
int
cmd_fill (ac, av)
	int ac;
	char *av[];
{
	u_int32_t from, to, i, a, w;
	u_int8_t *p, *d;
	union {
		u_int32_t	w;
		u_int16_t	h;
		u_int8_t	b[PATSZ];
	} pat;
	int len;

	if (!get_rsa (&from, av[1]) || !get_rsa (&to, av[2])) {
		return (-1);
	}

	if (to < from) {
		return (1);
	}

	for (d = p = pat.b, i = 3; i < ac; i++) {
		if (!strcmp(av[i], "-s")) {
			if (++i >= ac) {
				printf ("bad arg count\n");
				return (-1);
			} else {
				char *s;
				for (s = av[i]; *s; s++) {
					if (d >= &pat.b[PATSZ]) {
						printf ("pattern too long\n");
						return (-1);
					}
					*d++ = *s;
				}
			}
		} else {
			if (!get_rsa (&a, av[i])) {
				return (-1);
			}
			if (d >= &pat.b[PATSZ]) {
				printf ("pattern too long\n");
				return (-1);
			}
			*d++ = a;
		}
	}

	len = d - p;
	to++;
	if ((len < 5) && ((len & (len - 1)) == 0) && ((to | from) & 3) == 0) {
		/* special case using word writes */
		switch (len) {
		case 1:
			w = pat.b[0];
			w |= (w << 8);
			w |= (w << 16);
			break;
		case 2:
			w = pat.h;
			w |= (w << 16);
			break;
		case 4:
			w = pat.w;
			break;
		}

		for (; from < to; from += sizeof (u_int32_t)) {
			*(u_int32_t *)from = w;
		}
	} else {
		/* all other cases: byte by byte */
		for (; from < to; from += sizeof (u_int8_t)) {
			*(u_int8_t *)from = *p;
			if (++p >= d) {
				p = pat.b;
			}
		}
	}

	flush_cache (DCACHE, 0);
	flush_cache (ICACHE, 0);
	return (0);
}

static const Cmd Cmds[] =
{
	{"Memory"},
#ifdef HAVE_QUAD
	{"m",		"[-bhwdnx] adr [val|-s str]..",
#else
	{"m",		"[-bhwnx] adr [val|-s str]..",
#endif
			m_opts,
			"modify memory",
			cmd_modify, 2, 99, CMD_REPEAT},
#ifdef HAVE_QUAD
	{"d",		"[-bhwdsrx] adr [cnt]",
#else
	{"d",		"[-bhwsrx] adr [cnt]",
#endif
			d_opts,
			"display memory",
			cmd_dump, 2, 4, CMD_REPEAT},
	{"compare",	"from to with",
			0,
			"compare memory to memory",
			cmd_compare, 4, 4, CMD_REPEAT},
	{"copy",	"from to size",
			0,
			"copy memory to memory",
			cmd_copy, 4, 4, CMD_REPEAT},
	{"fill",	"from to {val|-s str}..",
			fill_opts,
			"fill memory",
			cmd_fill, 4, 99, CMD_REPEAT},
	{"search",	"from to {val|-s str}..",
			search_opts,
			"search memory",
			cmd_search, 4, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
