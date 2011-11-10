/* $Id: sym.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */
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
#include <stdio.h>
#include <termio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif

#include <pmon.h>

int cmd_sym __P((int, char *[]));
int cmd_ls __P((int, char *[]));

typedef struct Sym {
    struct Sym     *next;
    unsigned long   value;
    char            name[1];
} Sym;


static void defsym __P((char *, u_int32_t));
static int dopat __P((char *, int));
static void doval __P((char *, int));
static Sym * firstsym __P((int));
static Sym * nextsym __P((void));
static Sym * getsymbyvalue __P((u_int32_t));;
static Sym * getsymbyname __P((char *));


/*
 * We use a hash table to make it faster to add new symbols, and to
 * find them by name.
 */
#define NHASH	127
static Sym 	*symhash[NHASH];

/* 
 * We can generate a numerically and/or alphabetically sorted list (depending
 * on memory space).
 * The numeric list is used for 'ls -n' and to do a binary search to find
 * a symbol based on an address.
 * The alphabetic list is used only for 'ls' without '-n'.
 */
#define ALPHA	0
#define NUMERIC	1
static Sym	**symsort[2];

static int	maxsymlen;
static int	nsyms;


/* Generate a hash value from memory
 * generates a 28bit hash
 * Algorithm due to P. J. Weinberger. 
 */
static unsigned int
strhash (const char *s)
{
    unsigned int h;

    for (h = 0; *s; s++) {
        h = (h << 4) + *s;
        if (h > 0x0fffffff) {
            h ^= (h >> 24) & 0xf0;
            h &= 0x0fffffff;
        }
    }
    return h;
}


static void
flushsort (int type)
{
    if (symsort[type]) {
	free ((char *) symsort[type]);
	symsort[type] = 0;
    }
}


static int
namecmp (Sym **s1, Sym **s2)
{
    return strcmp ((*s1)->name, (*s2)->name);
}


static int
valcmp (Sym **s1, Sym **s2)
{
    /* unsigned values, so long-winded comparison */
    if ((*s1)->value < (*s2)->value)
      return (-1);
    if ((*s1)->value > (*s2)->value)
      return (1);
    return 0;
}


static Sym **
getsort (int type)
{
    Sym **syms, **t;
    Sym *sym;
    int i;

    if (nsyms == 0)
      return 0;

    if (!(syms = symsort[type])) {
	if ((syms = (Sym **) malloc (nsyms * sizeof (Sym *))) != NULL) {
	    /* create new list */
	    for (i = 0, t = syms; i < NHASH; i++) {
		for (sym = symhash[i]; sym; sym = sym->next)
		  *t++ = sym;
	    }
	} else if ((syms = symsort[type ^ 1]) != 0) {
	    /* take over other list */
	    symsort[type ^ 1] = 0;
	}
	if (syms) {
	    qsort (syms, nsyms, sizeof (Sym *),
		   (type == ALPHA) ?
			(int (*)(const void *, const void *))namecmp :
			(int (*)(const void *, const void *))valcmp);
	    symsort[type] = syms;
	}
    }
    return syms;
}


/*
 * Find symbol with exactly the given name.
 */
static Sym *
getsymbyname (name)
    char *name;
{
    unsigned h = strhash (name) % NHASH;
    Sym *sym;
    
    if (nsyms == 0)
      return 0;

    for (sym = symhash[h]; sym; sym = sym->next) {
	if (name[0] == sym->name[0] && !strcmp(name+1, sym->name+1))
	  return sym;
    }
    return sym;
}


/*
 * Find symbol closest to value, but below or equal to it.
 */
static Sym *
getsymbyvalue (addr)
    u_int32_t addr;
{
    Sym		**syms, *sym;
    int		lo, hi, mid;

    if (nsyms == 0)
      return 0;

    syms = getsort (NUMERIC);
    if (!syms) {
	/* no memory for sorted list, have to search the long way */
	Sym *bestsym = 0;
	for (hi = 0; hi < NHASH; hi++)
	  for (sym = symhash[hi]; sym; sym = sym->next) {
	      if (sym->value == addr)
		return sym;
	      if (sym->value < addr && (!bestsym || sym->value > bestsym->value))
		bestsym = sym;
	  }
	return bestsym;
    }

    /* We now assume that the symbols are sorted by ascending values.
       Check that at least one symbol is a suitable candidate. */
    if (addr < syms[0]->value)
	return 0;

    /* By iterating until the value associated with the current
       hi index (the endpoint of the test interval) is less than
       or equal to the desired address, we accomplish two things:
       (1) the case where the address is larger than any 
       symbol value is trivially solved, (2) the value associated
       with the hi index is always the one we want when the interation
       terminates.  In essence, we are iterating the test interval
       down until the address is pushed out of it from the high end. */

    lo = 0; hi = nsyms - 1;
    while (syms[hi]->value > addr) {
	/* addr is still strictly less than highest symbol */
	mid = (lo + hi) / 2;	/* mid always >= lo */
	if (lo == mid || syms[mid]->value >= addr)
	    hi = mid;
	else
	    lo = mid;
    }

    /* The symbol indexed by hi is the best one. */
    return syms[hi];
}


int
newsym (name, value)
    char	*name;
    u_int32_t	value;
{
    Sym         *sym;
    unsigned int h;
    int		len;

    h = strhash (name) % NHASH;
    for (sym = symhash[h]; sym; sym = sym->next) {
	if (sym->name[0] == name[0] && !strcmp(sym->name+1, name+1)) {
	    if (sym->value != value) {
		sym->value = value;
		flushsort (NUMERIC);
	    }
	    return (1);
	}
    }

    flushsort (ALPHA);
    flushsort (NUMERIC);

    len = strlen (name);
    sym = (Sym *) malloc (sizeof (Sym) + len);
    if (!sym)
      return (0);

    memcpy (sym->name, name, len + 1);
    sym->value = value;

    sym->next = symhash[h];
    symhash[h] = sym;

    if (len > maxsymlen)
      maxsymlen = len;
    nsyms++;
    return (1);
}


/** clrsyms() clear entire symbol table */
void
clrsyms ()
{
    Sym            *sym, *nsym;
    int 	   i;

    for (i = 0; i < NHASH; i++) {
	for (sym = symhash[i]; sym; sym = nsym) {
	    nsym = sym->next;
	    free ((char *) sym);
	}
	symhash[i] = 0;
    }
#if 0 /* XXX: PAtrik */
    flush_validpc ();
#endif
    flushsort (ALPHA);
    flushsort (NUMERIC);
    maxsymlen = 0;
    nsyms = 0;
}


/** sym2adr(vp,name) finds value from name */
int
sym2adr(vp, name)
	register_t *vp;
	char *name;
{
	Sym *sym;

	sym = getsymbyname (name);
	if (sym) {
		*vp = sym->value;
		return (1);
	}
	else {
		*vp = 0;
		return (0);
	}
}


/** adr2sym(name,v) finds name from value */
char           *
adr2sym (name, v)
     char           *name;
     unsigned long   v;
{
    Sym            *sym;

    /*
     * If we're not supposed to deal with putting prefix symbol
     * on disassembly etc, then don't
     */

    if (matchenv("showsym") == 0)
	return 0;
    
    sym = getsymbyvalue (v);
    if (!sym || sym->value != v) {
	if (name)
	  name[0] = '\0';
	return 0;
    }

    if (name) {
	strcpy (name, sym->name);
	return (name);
    }
    return (sym->name);
}


/** adr2symoff(dst,value,width) convert addr to symbol+offset */
int
adr2symoff (dst, value, width)
     char           *dst;
     unsigned int    value;
     int             width;
{
    Sym            *p;
    char           tmp[16];

    /*
     * If we're not supposed to deal with putting prefix symbol
     * on disassembly etc, then don't
     */

    if (matchenv("showsym") == 0)
	return 0;
    
    dst[0] = 0;
    p = getsymbyvalue (value);
    if (!p || p->value > value)
      return (0);

    /*
     * If symbol is more than 20k away, then ignore symbol.
     */
    if( ( value - p->value ) > 0x5000 )
	return( 0 );
    
    if (width == 0)
      sprintf (dst, "%s", p->name);
    else
      sprintf (dst, "%*.*s", width, width, p->name);
    if (value == p->value)
      strcat (dst, "       ");
    else {
	sprintf (tmp, "+0x%-4x", value - p->value);
	strcat (dst, tmp);
    }
    return (1);
}


/** cmd_sym(ac,av) sym command */
int
cmd_sym (ac, av)
     int             ac;
     char           *av[];
{
    u_int32_t v;

    if (!get_rsa (&v, av[2]))
	return (-1);
    if (!newsym (av[1], v)) {
	printf ("out of memory\n");
	return (1);
    }
#if 0 /* XXX PATRIK */
    flush_validpc ();
#endif
    return (0);
}


#define LFLAG 0x01	/* long listing */
#define AFLAG 0x02	/* lookup address */
#define NFLAG 0x04	/* sort numerically */
/*#define TFLAG 0x08*/
/*#define DFLAG 0x10*/
/*#define BFLAG 0x20*/


static Sym	**symlst;
static Sym	*symcur;
static int	symno;

static Sym *
nextsym ()
{
    if (symlst) {
	if (symno >= nsyms)
	  return 0;
	return symlst[symno++];
    } 

    if (!symcur || !symcur->next) {
	do {
	    if (symno >= NHASH)
	      return 0;
	    symcur = symhash[symno++];
	} while (!symcur);
    } else {
	symcur = symcur->next;
    }
    return symcur;
}


static Sym *
firstsym (sort)
    int sort;
{
    if (nsyms == 0)
      return 0;

    symno = 0;
    symcur = 0;
    symlst = getsort (sort);
    if (!symlst)
      printf ("Not enough memory to sort symbols, sorry\n");
    return nextsym ();
}


/** doval(val,flags) prints name from value */
static void
doval (val, flags)
     char           *val;
     int             flags;
{
    u_int32_t  value;
    char       tmp[LINESZ];

    if (!get_rsa (&value, val))
	return;

    if (!adr2symoff (tmp, value, 0)) {
	printf ("%08x: not found\n", value);
    }
    else {
        printf ("%08x = %s\n", value, tmp);
    }
}


/** dopat(pat,flags) prints all syms that match pat */
static int
dopat (pat, flags)
     char           *pat;
     int             flags;
{
    Sym            *sym;
    char            tmp[LINESZ];
    int             sort, l, siz, col, width;

    sort = (flags & NFLAG) ? NUMERIC : ALPHA;
    siz = l = moresz;

    if (flags & LFLAG) {
	/* long listing */
	for (sym = firstsym (sort); sym; sym = nextsym ()) {
	    if (strpat (sym->name, pat)) {
		sprintf (prnbuf, "%08x %s", sym->value, sym->name);
		if (more (prnbuf, &l, siz))
		  break;
	    }
	}
	return 1;
    }

    /* compact listing, so need to columnize */
    width = (maxsymlen + 8) & ~7;
    for (sym = firstsym (sort); sym; ) {
	prnbuf[0] = 0;
	col = 0;
	do {
	    if (strpat (sym->name, pat)) {
		strcpy (tmp, sym->name);
		col += width;
		if (col + width <= 79)
		  str_fmt (tmp, width, FMT_LJUST);
		strcat (prnbuf, tmp);
	    }
	} while ((sym = nextsym()) && col + width <= 79);

	if (more (prnbuf, &l, siz))
	  break;
    }

    return 1;
}


const Optdesc         ls_opts[] =
{
    {"-l", "long listing"},
    {"-n", "numeric order"},
    {"-a", "lookup address"},
    {0}};


/** cmd_ls(ac,av), the 'ls' command */
int
cmd_ls (ac, av)
     int             ac;
     char           *av[];
{
    int             i, j, flags, pats;

    flags = pats = 0;
    for (i = 1; i < ac; i++) {
	if (av[i][0] == '-') {
	    j = 1;
	    while (av[i][j] != 0) {
		if (av[i][j] == 'l')
		    flags |= LFLAG;
		else if (av[i][j] == 'n')
		    flags |= NFLAG;
		else if (av[i][j] == 'a')
		    flags |= AFLAG;
		/* t=text d=data b=bss
		 * else if (av[i][j] == 't') flags |= TFLAG;
		 * else if (av[i][j] == 'b') flags |= BFLAG;
		 * else if (av[i][j] == 'd') flags |= DFLAG;
		 */
		else {
		    printf ("bad option [%c]\n", av[i][j]);
		    return (-1);
		}
		j++;
	    }
	} else
	    pats++;
    }

    ioctl (STDIN, CBREAK, NULL);

    if (flags & AFLAG) {
	if (pats == 0) {
	    printf ("no values specified\n");
	    return (-1);
	}
	for (i = 1; i < ac; i++) {
	    if (av[i][0] != '-')
	      doval (av[i], flags);
	}
    } else if (pats > 0) {
	for (i = 1; i < ac; i++) {
	    if (av[i][0] != '-')
	      if (!dopat (av[i], flags))
		break;
	}
    } else {
	dopat ("*", flags);
    }
    return (0);
}


/* define a symbol only if it is not already defined */
static void
defsym (name, value)
    char	*name;
    u_int32_t	value;
{
    if (!getsymbyname (name))
	newsym (name, value);
}


void
defsyms (minaddr, maxaddr, pc)
    u_int32_t minaddr, maxaddr, pc;
{
    extern int	    start[];
    defsym ("Pmon", (unsigned long) start);
    if (minaddr != ~0)
	defsym ("_ftext", minaddr);
    if (maxaddr != 0)
	defsym ("etext", maxaddr);
    defsym ("start", pc);
}


void
syminit ()
{
#if !defined(BSOTSUN) && 0 
    extern char     _ftext[], etext[], _fdata[], edata[];
#endif
    extern int	    memorysize;

#ifdef __mips__
    defsyms ((u_int32_t)sbrk(0) + 1024, memorysize | (CLIENTPC & 0xc0000000), CLIENTPC);
#else
    defsyms ((u_int32_t)sbrk(0) + 1024, memorysize, CLIENTPC);
#endif
#if !defined(BSOTSUN) && 0
    newsym ("free", (etext - _ftext) + (edata - _fdata));
#endif
}


/*
 *  Command table registration
 *  ==========================
 */

static const Cmd Cmds[] =
{
	{"Debugger"},
	{"sym",		"name value",
			0,
			"define symbol",
			cmd_sym, 3, 5, CMD_REPEAT},
	{"ls",		"[-ln sym*|-va adr]",
			ls_opts,
			"list symbols",
			cmd_ls, 1, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
