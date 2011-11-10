/* $Id: oload.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */
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
#include <sys/param.h>
#include <signal.h>
#include <sys/endian.h>
#include <stdio.h>
#include <termio.h>
#include <string.h>
#include <setjmp.h>
#include <ctype.h>
#include <stdlib.h>
#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif

#include <pmon.h>
#include <pmon/loaders/loadfn.h>

#include "mod_debugger.h"
#include "mod_load.h"
#include "mod_symbols.h"

/* control character used for download */
#define ETX	CNTRL('c')
#define ACK	CNTRL('f')
#define NAK	CNTRL('u')
#define XON	CNTRL('q')
#define XOFF	CNTRL('s')

/* dlproto values */
#define NONE            0
#define XONXOFF         1
#define ETXACK          2

/* dlecho values */
#define OFF             0
#define ON              1
#define LFEED           2

extern char *heaptop;

static jmp_buf         ldjmpb;		/* non-local goto jump buffer */

#ifdef INET
static void
ldintr (int dummy)
{
    sigsetmask (0);
    longjmp (ldjmpb, 1);
}
#endif

int cmd_load __P((int, char *[]));

const Optdesc         cmd_load_opts[] =
{
#if NMOD_SYMBOLS > 0
    {"-s", "don't clear old symbols"},
#endif
#if NMOD_DEBUGGER > 0
    {"-b", "don't clear breakpoints"},
#endif
    {"-e", "don't clear exception handlers"},
    {"-a", "don't add offset to symbols"},
    {"-t", "load at top of memory"},
    {"-i", "ignore checksum errors"},
#if NMOD_SYMBOLS > 0
    {"-n", "don't load symbols"},
    {"-y", "only load symbols"},
#endif
    {"-v", "verbose messages"},
    {"-o<offs>", "load offset"},

    {"-c cmdstr", "send cmdstr to host"},
    {"-u<num>", "set baud rate"},
    {"-h<port>", "load from <port>"},

    {0}};

/*************************************************************
 *  cmd_load(ac,av)
 */
int
cmd_load (ac, av)
     int             ac;
     char           *av[];
{
    FILE	   *fp;
    struct termio   otbuf, tbuf;
    char           *hostport, *cmdstr;
    int             dlecho, dlproto, dltype, count, n;
    int             len, err, i, s, tot, recno, flags, istty, isnet;
    char            recbuf[DLREC], *ep, *baudrate;
    int             errs[DL_MAX];
    unsigned long   offset;
    extern int	    optind;
    extern char    *optarg;
#ifdef INET
    sig_t	    ointr;
#else
    jmp_buf	   *ointr;
#endif
    int		    intr;
    int             c;
#ifdef INET
    int ospl;
#endif

    baudrate = 0;
    count = 0;
    cmdstr = 0;
    hostport = 0;
    flags = 0;
    istty = 0;
    isnet = 0;
    dlecho = OFF;
    dlproto = NONE;
    offset = 0;

    optind = 0;
    while ((c = getopt (ac, av, "sbeatifnyvo:c:u:h:")) != EOF)
      switch (c) {
#if NMOD_SYMBOLS > 0
      case 's':
	  flags |= SFLAG; break;
#endif
#if NMOD_DEBUGGER > 0
      case 'b':
	  flags |= BFLAG; break;
#endif
      case 'e':
	  flags |= EFLAG; break;
      case 'a':
	  flags |= AFLAG; break;
      case 't':
	  flags |= TFLAG; break;
      case 'i':
	  flags |= IFLAG; break;
      case 'v':
	  flags |= VFLAG; break;
      case 'o':
	  if (!get_rsa ((u_int32_t *)&dl_offset, optarg))
	    return (-1);
	  break;

      case 'c':
	  cmdstr = optarg; break;
      case 'h':
	  hostport = optarg; break;
      case 'u':
	  baudrate = optarg; break;
      default:
	  return (-1);
      }

    if (optind < ac && !hostport)
      hostport = av[optind++];
    if (optind < ac)
      return (-1);

    dl_initialise (offset, flags);

    if (!hostport)
      hostport = getenv ("hostport");
    fp = fopen (hostport, "r");
    if (!fp) {
	printf ("can't open %s\n", hostport);
	return (1);
    }
    dltype = -1;		/* dltype is set by 1st char of 1st record */

    if (ioctl (fileno (fp), TCGETA, &tbuf) >= 0) {
	istty = 1;

	otbuf = tbuf;

	dlproto = matchenv ("dlproto");
	dlecho = matchenv ("dlecho");

	if (baudrate) {
	    if ((n = getbaudrate (baudrate)) == 0) {
		printf ("%s: unrecognized baud rate\n", baudrate);
		return (-1);
	    }
	    tbuf.c_cflag = n;
	}
	
	tbuf.c_iflag &= ~IXOFF;		/* we do input flow control */
	tbuf.c_lflag &= ~ISIG;		/* signal handling disabled */

	if (dlecho == OFF)
	  tbuf.c_lflag &= ~ECHO;	/* no automatic echo */

	/* allow LF or ETX as line terminator */
	tbuf.c_cc[VEOL] = '\n';
	tbuf.c_cc[VEOL2] = ETX;

	printf ("Downloading from %s, ^C to abort\n", hostport);
	ioctl (fileno (fp), TCSETAF, &tbuf);

	if (cmdstr) {
	    fputs (cmdstr, fp);
	    putc ('\r', fp);
	}

#ifdef INET
	/* spot a network device because it doesn't support GETINTR */
	if (ioctl (fileno (fp), GETINTR, &ointr) < 0)
	    isnet = 1;
#endif
    }
    else {
	printf ("Downloading from %s, ^C to abort\n", hostport);
    }

    err = intr = tot = recno = 0;
    bzero (errs, sizeof (errs));

#ifdef INET
    if (istty && !isnet)
	/* block network polling during serial download */
	ospl = splhigh ();
#endif

    if (setjmp (ldjmpb)) {
	intr = 1;
#ifdef INET
	if (istty && !isnet)
	    /* re-enable network polling */
	    splx (ospl);
#endif
	goto done;
    }

#ifdef INET
    ointr = signal (SIGINT, ldintr);
#else
    ioctl (STDIN, GETINTR, &ointr);
    ioctl (STDIN, SETINTR, ldjmpb);
#endif

    for (;;) {
	scandevs ();

	/* read one line */
	if (istty) {
	    n = fread (recbuf, 1, DLREC - 1, fp);
	    if (n < 0) {
		err++;
		errs[DL_IOERR]++;
		break;
	    }
	    if (n == 0)
		continue;
	} else {
	    if (!fgets (recbuf, DLREC - 1, fp))
	      break;
	    n = strlen (recbuf);
	}

	if (dlproto == XONXOFF)
	    putc (XOFF, fp);
	scandevs ();

#ifdef INET
	if (istty && !isnet) {
	    /* now safe to poll network */
	    splx (ospl);
	    scandevs ();
	}
#endif

	++recno;
	for (ep = recbuf + n; ep > recbuf; ep--) 
	  if (!isspace (ep[-1]) && ep[-1] != ETX)
	    break;

	if (ep > recbuf) {
	    *ep = '\0';	/* terminate line */

	    n = ep-recbuf;

	    if (dltype == -1) {
		char c = recbuf[0];
		if (c == '/')
		  dltype = 1;
		else if (c == 'S')
		  dltype = 0;
		else {
		    err++;
		    errs[DL_BADCHAR]++;
		    if (flags & VFLAG) {
			printf ("unknown record type '%c' (0x%x)\n",
				isprint(c) ? c : '?', c & 0xff);
		    }
		    break;
		}
	    }

	    switch (dltype) {
#if NMOD_S3LOAD > 0
	    case 0:
	      s = dl_s3load (recbuf, n, &len, flags);
	      break;
#endif
#if NMOD_FASTLOAD > 0
	    case 1:
	      s = dl_fastload (recbuf, n, &len, flags);
	      break;
#endif
	    default:
	      printf("load format not supported\n");
	      s = DL_DONE;
	      break;
	    }
	    
	    tot += len;
	    if (s == DL_DONE) {
		break;
	    }
	    if (s != DL_CONT) {
		err++;
		errs[s]++;
		if (flags & VFLAG) {
		    printf ("line %d: %s\n", recno, dl_err (s));
		    printf ("%s\n", recbuf);
		}
		if (dlproto == ETXACK)
		    break;
	    }
	}

#ifdef INET
	if (istty && !isnet)
	    /* block network polling again */
	    ospl = splhigh ();
#endif

	if (dlproto == XONXOFF)
	  putc (XON, fp);
	else if (dlproto == ETXACK)
	  putc (ACK, fp);
	if (dlecho == LFEED)
	  putc ('\n', fp);
    }

done:
#ifdef INET
    signal (SIGINT, ointr);
#else
    ioctl (STDIN, SETINTR, ointr);
#endif

    if (dlproto == XONXOFF)
      putc (XON, fp);
    else if (dlproto == ETXACK)
      putc (err ? NAK : ACK, fp);
    if (dlecho == LFEED)
      putc ('\n', fp);
    if (istty)			/* restore tty state */
	ioctl (fileno (fp), TCSETAF, &otbuf);
    fclose (fp);

    if (err != 0 || intr) {
	printf ("\n%d error%s%s\n", err, (err == 1) ? "" : "s", 
		intr ? " (interrupted)" : "");
	for (i = 0; i < DL_MAX; i++)
	    if (errs[i])
		printf ("   %3d %s\n", errs[i], dl_err(i));
	printf ("total = 0x%x bytes\n", tot);
	return (1);
    }

    
    printf ("\ntotal = 0x%x bytes\n", tot);

    if (!(flags & (FFLAG|YFLAG))) {
	printf ("Entry Address  = %08x\n", dl_entry);
	flush_cache (DCACHE, NULL);
	flush_cache (ICACHE, NULL);
	md_setpc(NULL, dl_entry);
#if NMOD_SYMBOLS > 0
	if (!(flags & SFLAG))
	    dl_setloadsyms ();
#endif
    }

    return (0);
}

/*
 *  Command table registration
 *  ==========================
 */
static const Cmd Cmds[] =
{
	{"Boot and Load"},
	{"oload",	"[-beastif][-u baud][-o offs][-c cmd][-h port]",
			cmd_load_opts,
			"load memory from hostport",
			cmd_load, 1, 16, 0},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
