/* $Id: sdump.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */
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

int cmd_sdump __P((int, char *[]));

const Optdesc         cmd_dump_opts[] =
{
    {"-B", "dump binary image"},
    {"-h<port>", "send dump to host <port>"},
    {0}};

/** sdump(ac,av), the 'dump' command */
int
cmd_sdump (ac, av)
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

static const Cmd Cmds[] =
{
	{"Misc"},
	{"dump",	"[-B] adr siz [port]",
			cmd_dump_opts,
			"dump memory to hostport",
			cmd_sdump, 3, 6, 0},
	{0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
