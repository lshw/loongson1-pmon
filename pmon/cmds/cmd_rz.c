/*	$Id: cmd_rz.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2002 Patrik Lindergren  (www.lindergren.com)
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
 *	This product includes software developed by
 *	Patrik Lindergren, Sweden.
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
#include <pmon/loaders/zmodem/zmodem.h>
#include <pmon/loaders/zmodem/zmdm.h>
#include <pmon.h>
#include "ramfiles.h"
extern char filename[];
extern long current_file_size;

#define VFLAG 0x0040		/* verbose */
/* control character used for download */
#define ETX	CNTRL('c')

static jmp_buf         ldjmpb;		/* non-local goto jump buffer */

#ifdef INET
static void
ldintr (int dummy)
{
    sigsetmask (0);
    longjmp (ldjmpb, 1);
}
#endif

/*
 * Prototypes
 */
int rz_cmd __P((int, char **av));

static const Optdesc rz_opts[] =
{
    {"-v", "verbose"},
    {"-o<address>", "destination address"},
    {"-s<hex>", "max size"},
    {"-u<num>", "set baud rate"},
    {"-h<port>", "load from <port>"},
    {0}};

extern FILE *logfp;

int
rz_cmd (ac, av)
    int             ac;
    char           *av[];
{
	FILE	   *fp;
	struct termio   otbuf, tbuf;
	char           *hostport;
	int             n, flags, istty, isnet;
	char            *baudrate;
	unsigned long	buffer;
	long		maxsize = -1;
	extern int	optind;
	extern char    *optarg;
#ifdef INET
	sig_t	    ointr;
#else
	jmp_buf	   *ointr;
#endif
	int		intr = 0;
	int             c;
#ifdef INET
	int ospl;
#endif
	logfp=vgaout;

	baudrate = 0;
	hostport = 0;
	flags = 0;
	istty = 0;
	isnet = 0;

	optind = 0;
	while ((c = getopt (ac, av, "vo:s:u:h:")) != EOF)
		switch (c) {
		case 'v':
			flags |= VFLAG; break;
		case 'o':
			if (!get_rsa ((u_int32_t *)&buffer, optarg))
				return (-1);
			break;
		case 's':
			if (!get_rsa ((u_int32_t *)&maxsize, optarg))
				return (-1);
			break;
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
	
	if (!hostport)
		hostport = getenv ("hostport");
	fp = fopen (hostport, "r+");
	if (!fp) {
		printf ("can't open %s\n", hostport);
		return (1);
	}
	
	if (ioctl (fileno (fp), TCGETA, &tbuf) >= 0) {
		istty = 1;
		
		otbuf = tbuf;
	
		if (baudrate) {
			if ((n = getbaudrate (baudrate)) == 0) {
				printf ("%s: unrecognized baud rate\n", baudrate);
				return (-1);
			}
			tbuf.c_cflag = n;
		}
	
		tbuf.c_iflag &= ~IXOFF;		/* we do input flow control */
		tbuf.c_lflag &= ~ISIG;		/* signal handling disabled */
		tbuf.c_lflag &= ~ECHO;	/* no automatic echo */

		/* allow LF or ETX as line terminator */
		tbuf.c_cc[VEOL] = '\n';
		tbuf.c_cc[VEOL2] = ETX;
	
		printf ("Downloading from %s, ^C to abort\n", hostport);
		ioctl (fileno (fp), TCSETAF, &tbuf);
		
#ifdef INET
		/* spot a network device because it doesn't support GETINTR */
		if (ioctl (fileno (fp), GETINTR, &ointr) < 0)
			isnet = 1;
#endif
	} else {
		printf ("Downloading from %s, ^C to abort\n", hostport);
	}

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

	/*
	 * Start Receiving file
	 */
	fprintf(logfp, "Before receiveFile()\n");
	receiveFile(fp, (char *)buffer, maxsize, (flags & VFLAG));
	fprintf(logfp, "After receiveFile()\n");
#if NRAMFILES > 0
	deleteRamFile(filename);
	addRamFile(filename,buffer,current_file_size,0);
#endif
 done:

#ifdef INET
	signal (SIGINT, ointr);
#else
	ioctl (STDIN, SETINTR, ointr);
#endif
	if (istty)			/* restore tty state */
		ioctl (fileno (fp), TCSETAF, &otbuf);

	fclose (fp);

	if (intr) {
		printf ("Got Interrupted\n");
		return (1);
	}
	
	return (0);
}

static const Cmd Cmds[] =
{
	{"Misc"},
	{"rz",		"[-v][-u baud][-o address][-s size][-h port]",
			rz_opts,
			"zmodem download",
			rz_cmd, 2, 4, 0},
	{0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

