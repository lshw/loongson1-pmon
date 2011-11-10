/* $Id: call.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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

int cmd_call __P((int, char *[]));


/*
 *  Execute the 'call' command
 *  ==========================
 *
 *  The call command invokes a function in the same way as
 *  it would be when called from a program. Arguments passed
 *  can be given as values or strings. On return the value
 *  returned is printed out.
 */
int
cmd_call(ac, av)
	int ac;
	char *av[];
{
	int i, k;
	char *arg[10];
	int (*func) (char *, char *, char *, char *, char *,
		     char *, char *, char *, char *, char *);

	arg[0] = 0;
	k = 0;

	for(i = 1; i < ac; i++) {
		if (i > 1 && ac >= i + 2 && strcmp(av[i], "-s") == 0) {
			arg[k++] = av[++i];	/* String */
		}
		else {
			if (!get_rsa ((u_int32_t *)&arg[k], av[i])) {
				return (-1);
			}
			k++;
		}
	}
	if (arg[0] == 0) {
		printf ("Function address not specified\n");
		return (-1);
	}
	func = (void *)arg[0];
	i =  (*func) (arg[1], arg[2], arg[3], arg[4], arg[5],
		      arg[6], arg[7], arg[8], arg[9], arg[10]);
	printf ("Function returns: 0x%x (%d)\n", i, i);
	return (0);
}


static const Cmd Cmds[] =
{
	{"Misc"},
	{"call",	"addr [val|-s str]..",
			0,
			"call function",
			cmd_call, 2, 99, CMD_REPEAT},
	{0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
