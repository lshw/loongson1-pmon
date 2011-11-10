/* $Id: transp.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */
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

int cmd_transp __P((int, char *[]));

/** transp(ac,av), the 'tr' command */
int
cmd_transp (ac, av)
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
	printf ("tr: error: bad trabort char\n");
	return 1;
    }
    hostport = getenv ("hostport");
    if (!strcmp(hostport, "tty0")) {
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

static const Cmd Cmds[] =
{
	{"Misc"},
	{"tr",		"",
			0,
			"transparent mode",
			cmd_transp, 1, 1, CMD_REPEAT},
	{0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
