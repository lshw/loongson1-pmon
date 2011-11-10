/* $Id: stty.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */
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
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef _KERNEL
#undef _KERNEL
#endif
#include <sys/ioctl.h>

#include <pmon.h>

extern struct termio consterm;

const Optdesc         cmd_stty_opts[] =
{
    {"-a", "list all settings"},
    {"<baud>", "set baud rate"},
    {"sane", "set sane settings"},
    {"<mode>", "set mode"},
    {"-<mode>", "clear mode"},
    {0}};

/** stty(ac,av), the 'stty' command */

struct iocmodes {
	char	*mode;
	int	mask;
};

int cmd_stty __P((int, char *[]));
void modeprint(const struct iocmodes *, unsigned short);
int modedecode(const struct iocmodes *, char *, unsigned short *, int);

static const
struct iocmodes imodes[] = {
	{ "istrip", ISTRIP },
	{ "ixoff", IXOFF },
	{ "ixon", IXON },
	{ "ixany", IXANY },
	{ "icrnl", ICRNL },
	{ NULL, 0 }
};

static const
struct iocmodes omodes[] = {
	{ "onlcr", ONLCR },
	{ NULL, 0 }
};

static const
struct iocmodes lmodes[] = {
	{ "icanon", ICANON },
	{ "echo", ECHO },
	{ "echoe", ECHOE },
	{ NULL, 0 }
};

int
cmd_stty(int ac, char *av[])
{
	struct termio tbuf;
	char buf[16];
	char *opt;
	int fd, i, flag, aflag;
	int minus;

	fd = STDIN;
	flag = 0;
	aflag = 0;
	ioctl (fd, TCGETA, &tbuf);

	for (i = 1; i < ac; i++) {
		opt = av[i];
		if (!strcmp(opt, "sane")) {
			flag = 1;
			ioctl (fd, SETSANE);
			ioctl (fd, TCGETA, &tbuf);
			continue;
		}

		if (isdigit(*opt)) {	/* set baud rate */
			int cbaud;
			flag = 1;
			cbaud = getbaudrate(opt);
			if (cbaud == 0) {
				printf ("%s: bad baud rate\n", av[i]);
				if (fd != STDIN)
					close(fd);
				return -1;
			}
			tbuf.c_ispeed = cbaud;
			tbuf.c_ospeed = cbaud;
			continue;
		}

		minus = 0;
		if (*opt == '-') {	/* -option */
			opt++;
			if (*opt == 'a' && *(opt + 1) == '\0') {
				aflag = 1;
				continue;
			}
			minus = 1;
		}

		if (modedecode(imodes, opt, &tbuf.c_iflag, minus)) {
				flag = 1;
				continue;
		}

		if (modedecode(omodes, opt, &tbuf.c_oflag, minus)) {
				flag = 1;
				continue;
		}

		if (modedecode(lmodes, opt, &tbuf.c_lflag, minus)) {
				flag = 1;
				continue;
		}

		if (strpat(opt, "*tty?")) {	/* select device */
			if (flag) {
				printf("tty must be given before options\n");
				return 1;
			}
			fd = open(opt, 0);
			if (fd == -1) {
				printf ("can't open %s\n", opt);
				return (1);
			}
			ioctl (fd, TCGETA, &tbuf);
			continue;
		}

		printf("unkown parameter '%s'\n", opt);
		if (fd != STDIN)
			close(fd);
		return -1;
	}

	if (flag) {
		ioctl (fd, TCSETAF, &tbuf);
		if (fd != STDIN)
			close(fd);
		ioctl(STDIN, TCGETA, &consterm); /* Update console if changed */
		return 0;
	}

	ioctl (fd, TCGETA, &tbuf);
	printf ("baudrate=%d\n", getbaudval (tbuf.c_ispeed));

	if (aflag) {
		modeprint(imodes, tbuf.c_iflag);
		modeprint(omodes, tbuf.c_oflag);
		modeprint(lmodes, tbuf.c_lflag);
		printf ("\n");

		printf ("erase=%s ", cc2str (buf, tbuf.c_cc[VERASE]));
		printf ("stop=%s ", cc2str (buf, tbuf.c_cc[V_STOP]));
		printf ("start=%s ", cc2str (buf, tbuf.c_cc[V_START]));
		printf ("eol=%s ", cc2str (buf, tbuf.c_cc[VEOL]));
		printf ("eol2=%s ", cc2str (buf, tbuf.c_cc[VEOL2]));
		printf ("vintr=%s ", cc2str (buf, tbuf.c_cc[VINTR]));
		printf ("\n");
	}
	if (fd != STDIN)
		close(fd);
	return 0;
}

void
modeprint(const struct iocmodes *m, unsigned short c_xflag)
{
	while (m->mode) {
		printf("%s%s ", ((c_xflag & m->mask) ? "" : "-"), m->mode);
		m++;
	}
}

int
modedecode(const struct iocmodes *m, char *opt, unsigned short *c_xflag,
	   int minus)
{
	while (m->mode) {
		if (!strcmp(m->mode, opt)) {
			if (minus)
				*c_xflag &= ~m->mask;
			else
				*c_xflag |= m->mask;
			return 1;
		}
		m++;
	}
	return 0;
}

static const Cmd Cmds[] =
{
	{"Shell"},
	{"stty",        "[dev][opts]",
			cmd_stty_opts,
			"set tty options",
			cmd_stty, 1, 99, 1},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));
static void
init_cmd()
{
	cmdlist_expand(Cmds, 0);
}
