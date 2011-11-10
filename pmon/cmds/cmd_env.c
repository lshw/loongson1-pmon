/* $Id: cmd_env.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */
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

/*
 *   Environment handling commands.
 *
 *   env	; lists all or specific environment variables.
 *   set	; sets a variable or lists.
 *   unset	; removes a variable and its setting.
 *   eset	; allows resetting of a variable using line editing.
 *
 *   Select options:
 *
 *   select	cmd_env		# selects all environment commands.
 *   select	cmd_set		# selects set and unset commands.
 *
 *	Note that cmd_env require cmd_hist.
 */

#include <stdio.h>
#include <termio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif

#include <pmon.h>
#include <pmonenv.h>

#include "cmd_env.h"
#include "cmd_hist.h"

#if (NCMD_ENV == 1) && (NCMD_HIST == 0) 
#error "select cmd_env require select cmd_hist. try cmd_set to avoid cmd_hist."
#endif

int	cmd_set __P((int, char *[]));
int	cmd_eset __P((int, char *[]));
int	cmd_unset __P((int, char *[]));


int
cmd_set(ac, av)
	int ac;
	char *av[];
{
	struct envpair *ep;
	char *s;
	int ln;
	int tempflag = 0;

	if(ac >= 2 && strcmp(av[1], "-t") == 0) {
		tempflag = 1;
		av++;
	}

	ln = moresz;
	switch (ac - tempflag) {
	case 1:			/* display all variables */
		ioctl (STDIN, CBREAK, NULL);
		for (ep = envvar; ep < &envvar[NVAR]; ep++) {
			if (ep->name && printvar(ep->name, ep->value, &ln)) {
				break;
			}
		}
		break;

	case 2:			/* display specific variable */
		if ((s = getenv (av[1])) != 0) {
			printvar (av[1], s, &ln);
		}
		else {
			printf("%s: not found\n", av[1]);
			return(1);
		}
		break;

	case 3:			/* set a variable */
		if(!do_setenv(av[1], av[2], tempflag)) {
			printf("%s: cannot set\n", av[1]);
			return(1);
		}
		break;

	default:
		return (-1);
	}
	return (0);
}

#if NCMD_ENV > 0

int
cmd_eset(ac, av)
	int ac;
	char *av[];
{
	char name[LINESZ];
	char val[LINESZ];
	char *s;
	int i;
	int tempflag = 0;

	if(ac >= 2 && strcmp(av[1], "-t") == 0) {
		tempflag = 1;
	}

	for (i = 1 + tempflag; i < ac; i++) {
		strcpy (name, av[i]);
		strtoupper(name);

		s = getenv (av[i]);
		if(!s) {
			printf ("%s: not found\n", name);
			continue;
		}

		printf("%s=", name); fflush (stdout);

		strcpy(val, s);
		get_line(val, 0);

		if (strcmp(val, s)) {
			if (!do_setenv (av[i], val, tempflag)) {
				printf ("%s: cannot set\n", name);
				return (1);
			}
		}
	}
	return (0);
}
#endif /* NCMD_ENV */

int
cmd_unset(ac, av)
	int ac;
	char *av[];
{
	int i;

	for (i = 1; i < ac; i++) {
		if (getenv(av[i])) {
			unsetenv(av[i]);
		}
		else {
			printf ("%s: no match\n", av[i]);
			return (1);
		}
	}
	return (0);
}

static const Cmd Cmds[] = {
	{"Environment"},
#if NCMD_ENV > 0
	{"env",		"[name]",
			0,
			"display variable",
			cmd_set, 1, 2, CMD_REPEAT},
#endif
	{"set",		"[[-t] name [value]]",
			0,
			"display/set variable",
			cmd_set, 1, 4, CMD_REPEAT},
	{"unset",	"name...",
			0,
			"unset variable(s)",
			cmd_unset, 2, 99, CMD_REPEAT},
#if NCMD_ENV > 0
	{"eset",	"name",
			0,
			"edit variable",
			cmd_eset, 2, 9, CMD_REPEAT},
#endif
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
