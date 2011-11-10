/* $Id: miscmds.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */
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

int cmd_flush __P((int, char *[]));
int cmd_reboot __P((int, char *[]));

const Optdesc         cmd_flush_opts[] = {
    {"-i", "flush I-cache"},
    {"-d", "flush D-cache"},
    {0}
};

/** flush(ac,av), the 'flush' command */
int
cmd_flush (ac, av)
	int ac;
	char *av[];
{
extern int optind;
	int c;
	int flags = 0;

	optind = 0;
	while ((c = getopt (ac, av, "id")) != EOF) {

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
	}

	/* Always do dcache before icache */
	if (!flags) {
		flush_cache (DCACHE|ICACHE, NULL);
	}

	if (flags & 1) {
		flush_cache (DCACHE, NULL);
	}

	if (flags & 2) {
		flush_cache (ICACHE, NULL);
	}

	return (0);
}


int
cmd_reboot (ac, av)
    int             ac;
    char           *av[];
{
    printf ("Rebooting...\n");
    //delay (1000000);
    tgt_reboot();
    return(0); /* Shut up gcc */
}

int
cmd_poweroff (ac, av)
    int             ac;
    char           *av[];
{
    printf ("poweroff...\n");
    tgt_poweroff();
    return(0); 
}

static const Cmd Cmds[] =
{
	{"Misc"},
	{"flush",	"[-di]",
			cmd_flush_opts,
			"flush caches",
			cmd_flush, 1, 3, CMD_REPEAT},
	{"reboot",	"",
			0,
			"reboot system",
			 cmd_reboot, 1, 99, 0},
	{"poweroff",	"",
			0,
			"reboot system",
			 cmd_poweroff, 1, 99, 0},
	{"halt",	"",
			0,
			"reboot system",
			 cmd_poweroff, 1, 99, 0},
        
	{0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
