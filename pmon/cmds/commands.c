/* $Id: commands.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */


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

#include <machine/cpu.h>

#include <pmon.h>

static void * dispmem __P((char *, void *, int));

char           *searching = "searching..  ";

char           *badhexsym = "%s: neither hex value nor symbol\n";

static __inline void era_line __P((char *));
static __inline void
era_line(char *p)
{
	while(*p++)
		printf("\b \b");
}


/** transp(ac,av), the 'tr' command */
int
transp (ac, av)
     int             ac;
     char           *av[];
{
    int             fd, n;
    jmp_buf         intrsave;
    char           *hostport, buf[80], abortch, *trabort;
    struct termio   tbuf, consave, hostsave;

    trabort = getenv ("trabort");
    abortch = str2cc (trabort);
    if (abortch == 0) {
	printf ("tr: error: trabort not set\n");
	return 1;
    }
    hostport = getenv ("hostport");
    if (strequ (hostport, "tty0")) {
	printf ("can't use tty0 as hostport in transparent mode\n");
	return 1;
    }
    fd = open (hostport, 0);
    if (fd == -1) {
	printf ("can't open %s\n", hostport);
	return 1;
    }
    printf ("Entering transparent mode, %s to abort\n", trabort);

    ioctl (fd, TCGETA, &tbuf);
    hostsave = tbuf;
    tbuf.c_lflag &= ~(ICANON | ECHO | ECHOE);
    tbuf.c_iflag &= ~(ICRNL);
    tbuf.c_iflag |= IXOFF;	/* enable tandem mode */
    tbuf.c_oflag &= ~ONLCR;
    tbuf.c_cc[4] = 1;
    ioctl (fd, TCSETAF, &tbuf);

    ioctl (STDIN, TCGETA, &tbuf);
    consave = tbuf;
    tbuf.c_lflag &= ~(ICANON | ECHO | ECHOE);
    tbuf.c_iflag &= ~(ICRNL | IXON);
    tbuf.c_oflag &= ~ONLCR;
    tbuf.c_cc[4] = 1;
    ioctl (STDIN, TCSETAF, &tbuf);

/* disable INTR char */
    ioctl (STDIN, GETINTR, intrsave);
    ioctl (STDIN, SETINTR, 0);

    for (;;) {
	ioctl (STDIN, FIONREAD, &n);
	if (n > 0) {
	    if (n > sizeof(buf) - 1)
		n = sizeof(buf) - 1;
	    n = read (STDIN, buf, n);
	    buf[n] = '\0';
	    if (strchr (buf, abortch))
		break;
	    write (fd, buf, n);
	}
	ioctl (fd, FIONREAD, &n);
	if (n > 0) {
	    if (n > sizeof(buf))
		n = sizeof(buf);
	    n = read (fd, buf, n);
	    write (STDOUT, buf, n);
	}
    }
    ioctl (STDIN, TCSETAF, &consave);
    ioctl (fd, TCSETAF, &hostsave);
    ioctl (STDIN, SETINTR, intrsave);
    return 0;
}

const Optdesc         m_opts[] =
{
    {"-b", "access bytes"},
    {"-h", "access half-words"},
    {"-w", "access words"},
#ifdef HAVE_QUAD
    {"-d", "access double-words"},
#endif
    {"-n", "non-interactive (no write)"},
    {"", "Interactive Options"},
    {"<hexval>", "set memory, forward one"},
    {"CR", "forward one, no change"},
    {"=", "re-read"},
    {"^|-", "back one"},
    {".", "quit"},
    {0}};

/** modify(ac,av), the 'm' command */
int
modify (ac, av)
     int             ac;
     char           *av[];
{
    int32_t	adr;
    register_t	v;
    char	*p;
    int		datasz = -2;
    int		nowrite = 0;
    extern int	optind;
    int		c;

    optind = 0;
    while ((c = getopt (ac, av, "dwhbn")) != EOF) {
	if (datasz != -2) {
	    printf ("multiple data types specified\n");
	    return (-1);
	}
	switch (c) {
#ifdef HAVE_QUAD
	case 'd':
	    datasz = 8;
	    break;
#endif
	case 'w':
	    datasz = 4;
	    break;
	case 'h':
	    datasz = 2;
	    break;
	case 'b':
	    datasz = 1;
	    break;
	case 'n':
	    nowrite = 1;
	    break;
	default:
	    return (-1);
	}
    }

    if (optind >= ac)
      return (-1);

    if (!get_rsa (&adr, av[optind++]))
      return (-1);

    if (datasz == -2)
      datasz = 1 << matchenv ("datasz");

    if (optind < ac) {
	/* command mode */
	for (; optind < ac; optind++) {
	    if (strequ (av[optind], "-s")) {
		if (++optind >= ac) {
		    printf ("bad arg count\n");
		    return (-1);
		}
		for (p = av[optind]; *p; p++)
		  store_byte ((void *)(adr++), *p);
	    } else {
		if (!get_rsa_reg (&v, av[optind]))
		    return (-1);
		if (adr & (datasz - 1)) {
		    printf ("%08x: unaligned address\n", adr);
		    return (1);
		}
		switch (datasz) {
		case 1:
		    store_byte ((void *)adr, v);
		    break;
		case 2:
		    store_half ((void *)adr, v);
		    break;
		case 4:
		    store_word ((void *)adr, v);
		    break;
#ifdef HAVE_QUAD
		case 8:
		    store_dword ((void *)adr, v);
		    break;
#endif
		}
		adr += datasz;
	    }
	}
    } else {
	/* interactive mode */
	if (adr & (datasz - 1)) {
	    printf ("%08x: unaligned address\n", adr);
	    return (1);
	}
	for (;;) {
	    switch (datasz) {
	    case 1:
		v = *(unsigned char *)adr;
		break;
	    case 2:
		v = *(unsigned short *)adr;
		break;
	    case 4:
		v = *(unsigned int *)adr;
		break;
#ifdef HAVE_QUAD
	    case 8:
		v = *(unsigned long long *)adr;
		break;
#endif
	    }
#if __mips >= 3
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
	    if (*p == '.')
	      break;
	    else if (*p == '\0')
	      adr += datasz;
	    else if (*p == '^' || *p == '-')
	      adr -= datasz;
	    else if (*p == '=')	
	      /* reread */;
	    else if (get_rsa_reg (&v, p)) {
		switch (datasz) {
		case 1:
		    store_byte ((void *)adr, v);
		    break;
		case 2:
		    store_half ((void *)adr, v);
		    break;
		case 4:
		    store_word ((void *)adr, v);
		    break;
#ifdef HAVE_QUAD
		case 8:
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


/** search(ac,av), the search command */
int
search (ac, av)
     int             ac;
     char           *av[];
{
    u_int32_t	from, to, adr, i, a;
    char	*s, *d, c;
    char	pat[PATSZ];
    int		siz, ln;

    ln = siz = moresz;
    ioctl (STDIN, CBREAK, NULL);

    if (!get_rsa (&from, av[1]) || !get_rsa (&to, av[2]))
	return (-1);
    for (d = pat, i = 3; i < ac; i++) {
	if (strequ (av[i], "-s")) {
	    if (++i >= ac) {
		printf ("bad arg count\n");
		return (-1);
	    }
	    for (s = av[i]; *s; s++)
	      *d++ = *s;
	} else {
	    if (!get_rsa (&a, av[i]))
		return (-1);
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
		if (*s != load_byte (adr))
		    break;
		s++;
		adr++;
	    }
	    if (d <= s) {
		era_line (searching);
		dispmem (prnbuf, (void *)(from - 1), 4);
		if (more (prnbuf, &ln, siz))
		    break;
		printf ("%s", searching);
	    } else
		dotik (1, 0);
	}
	if (from > to)
	    era_line (searching);
    } else {			/* backward search */
	from -= d - pat - 1;
	while (to <= from) {
	    s = pat;
	    adr = from--;
	    while (s < d) {
		if (*s != load_byte (adr))
		    break;
		s++;
		adr++;
	    }
	    if (d <= s) {
		era_line (searching);
		dispmem (prnbuf, (void *)(from - 1), 4);
		if (more (prnbuf, &ln, siz))
		    break;
		printf ("%s", searching);
	    }
	}
	if (to > from)
	    era_line (searching);
    }
    return (0);
}

/** call(ac,av), the call command */
int
call (ac, av)
     int             ac;
     char           *av[];
{
    int             i, j, k;
    char           *arg[10];
    int (*func) __P((char *, char *, char *, char *, char *));

    arg[0] = 0;
    k = 0;
    for (i = 1; i < ac; i++) {
	if (av[i][0] == '-') {
	    j = 1;
	    while (av[i][j] != 0) {
		if (av[i][j] == 's') {
		    if (++i >= ac) {
			printf ("bad arg count\n");
			return (-1);
		    }
		    arg[k++] = av[i];
		    break;
		} else {
		    printf ("%c: unknown option\n", av[i][j]);
		    return (-1);
		}
		j++;
	    }
	} else {
	    if (!get_rsa ((u_int32_t *)&arg[k], av[i]))
		return (-1);
	    k++;
	}
    }
    if (arg[0] == 0) {
	printf ("Function address not specified\n");
	return (-1);
    }
    func = (void *)arg[0];
    i =  (*func) (arg[1], arg[2], arg[3], arg[4], arg[5]);
    printf ("Function returns: 0x%x (%d)\n", i, i);
    return (0);
}


#ifdef E2PROM
e2program (ac, av)
     int             ac;
     char           *av[];
{
    uword	addr, size, offset;

    if (!get_rsa (&addr, av[1])) {
	printf ("bad address\n");
	return -1;
    }

    if (!get_rsa (&size, av[2])) {
	printf ("bad size\n");
	return -1;
    }

    if (!get_rsa (&offset, av[3])) {
	printf ("bad offset\n");
	return -1;
    }

    printf ("Address = %x, Size = %x, Offset = %x\n",
	    addr, size, offset);

    sbd_e2program (addr, size, offset);
    return (0);
}
#endif



void
flush_cache (type, adr)
     int type;
     void *adr;
{

    switch (type) {
    case ICACHE:
	flushicache((void *)0, memorysize);
	break;

    case DCACHE:
	flushdcache((void *)0, memorysize);
	break;

    case IADDR:
	syncicache((void *)((int)adr & ~3), 4);
	break;
    }
}

const Optdesc         flush_opts[] = {
    {"-i", "flush I-cache"},
    {"-d", "flush D-cache"},
    {0}
};

/** flush(ac,av), the 'flush' command */
int
flush (ac, av)
     int             ac;
     char           *av[];
{
    extern int      optind;
    int 	    c;
    int             flags = 0;

    optind = 0;
    while ((c = getopt (ac, av, "id")) != EOF) 
      switch (c) {
      case 'd':
	  flags |= 1;
	  break;
      case 'i':
	  flags |= 2;
	  break;
      default:
	  return (-1);
      }

    if (!flags)
	flags = 3;

    if (flags & 2)
	flush_cache (ICACHE, NULL);

    if (flags & 1)
	flush_cache (DCACHE, NULL);

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
}

void
store_word (adr, v)
     void	* adr;
     int32_t	v;
{
    *(int32_t *)adr = v;
    flush_cache (IADDR, adr);
}

void
store_half (adr, v)
     void	*adr;
     int16_t	v;
{
    *(int16_t *)adr = v;
    flush_cache (IADDR, adr);
}

void
store_byte (adr, v)
     void	*adr;
     int8_t	v;
{
    *(int8_t *)adr = v;
    flush_cache (IADDR, adr);
}

#ifndef NO_SERIAL
const Optdesc         dump_opts[] =
{
    {"-B", "dump binary image"},
    {"-h<port>", "send dump to host <port>"},
    {0}};

/** sdump(ac,av), the 'dump' command */
int
sdump (ac, av)
     int             ac;
     char           *av[];
{
    u_int32_t       adr, siz, len, i, a;
    char            *tmp;
    char           *uleof, *ulcr, *hostport = 0, *eol;
    int             fd, cs, v, binary = 0;
    struct termio   tbuf;
    extern int      optind;
    extern char    *optarg;
    int 	    c;

    optind = 0;
    while ((c = getopt (ac, av, "Bh:")) != EOF) 
      switch (c) {
      case 'B':
	  binary = 1;
	  break;
      case 'h':
	  hostport = optarg;
	  break;
      default:
	  return (-1);
      }

    if (optind + 2 > ac)
      return (-1);
    if (!get_rsa (&adr, av[optind++]))
      return (-1);
    if (!get_rsa (&siz, av[optind++]))
      return (-1);

    if (!hostport)
      hostport = (optind < ac) ? av[optind++] : getenv ("hostport");

    if (optind < ac)
      return (-1);

    fd = open (hostport, 1);
    if (fd == -1) {
	printf ("can't open %s\n", hostport);
	return (1);
    }

    if (binary) {
	if (ioctl (fd, TCGETA, &tbuf) >= 0) {
	    printf ("can't dump binary to tty\n");
	    return (1);
	}
	write (fd, (void *)adr, siz);
    } else {
	ioctl (fd, TCGETA, &tbuf);
	tbuf.c_iflag &= ~IXANY;
	tbuf.c_oflag &= ~ONLCR;
	ioctl (fd, TCSETAF, &tbuf);
	
	uleof = getenv ("uleof");
	ulcr = getenv ("ulcr");
	if (striequ (ulcr, "cr"))
	  eol = "\r";
	else if (striequ (ulcr, "lf"))
	  eol = "\n";
	else /* crlf */
	  eol = "\r\n";
	
	while (siz > 0) {
	    if (siz < 32)
	      len = siz;
	    else
	      len = 32;
	    cs = len + 5;
	    for (i = 0; i < 4; i++)
	      cs += (adr >> (i * 8)) & 0xff;
	    sprintf (line, "S3%02X%08X", len + 5, adr);
	    for (a = adr, tmp = line + 12, i = 0; i < len; a++, i++) {
		v = load_byte (a);
		cs += v;
		sprintf (tmp, "%02X", v & 0xff);
		tmp += 2;
	    }
	    sprintf (tmp, "%02X%s", (~cs) & 0xff, eol);
	    tmp += 2 + strlen (eol);
	    write (fd, line, tmp - line);
	    adr += len;
	    siz -= len;
	}
	sprintf (line, "S70500000000FA%s", eol);
	write (fd, line, strlen (line));
	write (fd, uleof, strlen (uleof));
    }

    close (fd);
    return (0);
}
#endif

/** copy(ac,av), the 'copy' command */
int
copy (ac, av)
     int             ac;
     char           *av[];
{
    u_int32_t	from, to, n;

    if (!get_rsa (&from, av[1]) || !get_rsa (&to, av[2]) || !get_rsa (&n, av[3]))
	return (-1);
    if (to < from)
	while (n-- > 0)
	    store_byte ((void *)(to++), load_byte (from++));
    else
	for (from += n, to += n; n-- > 0;)
	    store_byte ((void *)(--to), load_byte (--from));
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
    {"-r<reg>", "display as register"},
    {0}};

/** dump(ac,av), the 'd' command */
int
dump (ac, av)
     int             ac;
     char           *av[];
{
    void *adr;
    int32_t         x;
    char           *reg;
    int             siz, ln, i;
    int		    datasz = -2;
    extern int      optind;
    extern char    *optarg;
    int 	    c;
    static void *last_adr;

    optind = 0;
    while ((c = getopt (ac, av, "dwhbsr:")) != EOF) {
	if (datasz != -2) {
	    printf ("multiple data types specified\n");
	    return (-1);
	}
	switch (c) {
#ifdef HAVE_QUAD
	case 'd':
	    datasz = 8;
	    break;
#endif
	case 'w':
	    datasz = 4;
	    break;
	case 'h':
	    datasz = 2;
	    break;
	case 'b':
	    datasz = 1;
	    break;
	case 's':
	    datasz = 0;
	    break;
	case 'r':
	    reg = optarg;
	    datasz = -1;
	    break;
	default:
	    return (-1);
	}
    }

    /* get <addr> */
    if (optind >= ac || !get_rsa ((u_int32_t *)&adr, av[optind++]))
      return (-1);

    if( repeating_cmd )
      adr = last_adr;
    
    /* get [<size>|<reg>] */
    if (optind < ac) {
	if (get_rsa (&x, av[optind])) {
	    /* size */
	    optind++;
	    ln = x; siz = 0;
	} else if (md_getregaddr (0, av[optind])) {
	    if (datasz != -2) {
		printf ("multiple data types specified\n");
		return (-1);
	    }
	    reg = av[optind++];
	    datasz = -1;
	} else
	  return (-1);
    } else {
	ln = siz = moresz;
    }

    if (optind != ac) 
      return (-1);

    if (datasz == 0) {		/* -s, print string */
	strncpy (prnbuf, adr, 70);
	prnbuf[70] = '\0';
	for (i = 0; prnbuf[i] != 0; i++)
	    if (!isprint (prnbuf[i]))
		prnbuf[i] = '.';
	printf ("%s\n", prnbuf);
	return (0);
    }

    ioctl (STDIN, CBREAK, NULL);
    if (datasz == -1) {
	/* -r<reg> print as register */
	if (!md_disp_as_reg ((register_t *)adr, reg, &ln)) {
	    printf ("%s: bad register name\n", reg);
	    return (-1);
	}
    } else {
	if (datasz < 0)
	  datasz = 1 << matchenv ("datasz");
	while (1) {
	    last_adr = adr;
	    adr = dispmem (prnbuf, (void *)adr, datasz);
	    if (more (prnbuf, &ln, siz))
	      break;
	}
    }

    return (0);
}

static void * 
dispmem (p, adr, siz)
     char 	*p;
     void	*adr;
     int	siz;
{
    int i;
    char v;
    char tmp[18];
    union {
	double		dummy;	/* ensure 8-byte alignment */
	unsigned char   b[16];
    } buf;
    void *w;

    w = adr;
    for (i = 0; i < 16; i++) {
	v = load_byte (w++);
	buf.b[i] = v;
    }

    sprintf (p, "%08x  ", adr);
    for (i = 0; i < 16; i += siz) {
	if (i == 8)
	    strccat (p, ' ');
	switch (siz) {
	case 1:
	    sprintf (tmp, "%02x ", *(unsigned char *)&buf.b[i]);
	    break;
	case 2:
	    sprintf (tmp, "%04x ", *(unsigned short *)&buf.b[i]);
	    break;
	case 4:
	    sprintf (tmp, "%08x ", *(unsigned long *)&buf.b[i]);
	    break;
#ifdef HAVE_QUAD
	case 8:
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

/** fill(ac,av) the fill command */
int
fill (ac, av)
     int             ac;
     char           *av[];
{
    u_int32_t           from, to, i, a, w;
    u_int8_t	    *p, *d;
    union {
	u_int32_t	w;
	u_int16_t	h;
	u_int8_t	b[PATSZ];
    } pat;
    int             len;

    if (!get_rsa (&from, av[1]) || !get_rsa (&to, av[2]))
	return (-1);
    if (to < from)
	return (1);

    for (d = p = pat.b, i = 3; i < ac; i++) {
	if (strequ (av[i], "-s")) {
	    if (++i >= ac) {
		printf ("bad arg count\n");
		return (-1);
	    }
	    else {
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
	    if (!get_rsa (&a, av[i]))
		return (-1);
	    if (d >= &pat.b[PATSZ]) {
		printf ("pattern too long\n");
		return (-1);
	    }
	    *d++ = a;
	}
    }

    len = d - p;
    if ((len == 1 || len == 2 || len == 4)
	&& (from & 3) == 0 && (to & 3) == 3) {
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
	for (; from <= to; from += sizeof (u_int32_t))
	    *(u_int32_t *)from = w;
    } else {
	/* all other cases: byte by byte */
	for (; from <= to; from += sizeof (u_int8_t)) {
	    *(u_int8_t *)from = *p;
	    if (++p >= d)
	      p = pat.b;
	}
    }

    flush_cache (ICACHE, 0);
    return (0);
}

int
reboot_cmd (ac, av)
    int             ac;
    char           *av[];
{
    printf ("Rebooting...\n");
#ifdef GODSONEV1
    deley (100);
#else
    delay (1000000);
#endif
    tgt_reboot();
    return(0); /* Shut up gcc */
}

#ifdef SROM
const Optdesc         srom_opts[] =
{
    {"-n", "don't execute softROM code"},
    {0}};

sbd_srom (ac, av)
    int             ac;
    char           *av[];
{
    uword           from, to, n;
    int		    dogo, docopy;
    extern int      optind;
    int		    c;

    dogo = 1;
    docopy = 0;

    optind = 0;
    while ((c = getopt (ac, av, "n")) != EOF) {
	switch (c) {
	case 'n':
	    dogo = 0;
	    break;
	default:
	    return (-1);
	}
    }

    from = to = 0;
    if (optind+3 <= ac) {
	if (!get_rsa (&from, av[optind++]) ||
	    !get_rsa (&to, av[optind++]) ||
	    !get_rsa (&n, av[optind++]))
	    return (-1);
    }
    if (optind != ac)
	return (-1);

    /* copy softrom code */
    sbd_softromcopy (from, to, n);
    if (dogo)
	sbd_softromgo ();
    return (0);
}
#endif
