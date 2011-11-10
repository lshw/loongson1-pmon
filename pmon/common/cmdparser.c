/* $Id: cmdparser.c,v 1.1.1.1 2006/06/29 06:43:25 cpu Exp $ */
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
 *      This product includes software developed by Opsycon AB, Sweden.
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
 *  This code was created from code released to Public Domain
 *  by LSI Logic and Algorithmics UK.
 */ 

#include <stdio.h>
#include <termio.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <ctype.h>
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

#include "cmd_hist.h"

int             repeating_cmd;

unsigned int getln __P((char **, char *));
static char *expand __P((char *));

/*
 *  Execute command
 *  ---------------
 *
 *  Execute the comman string given. If the command string given is
 *  empty, check the 'rptcmd' environment var and repeat the last
 *  command accordingly. If value is 'off' no repeat takes place. If
 *  the value is 'on' all commands repeatable can be repeated. If
 *  value is 'trace' only the 't' and 'to' commands will be repeated.
 */
int 
do_cmd(p)
	char *p;
{
	char	*av[MAX_AC];	/* argument holder */
	int32_t	ac;		/* # of arguments */
	char	*cmdlist[20];
	int	i, nc, j, c;
	int clistno;
	const Cmd *CmdTable;
	int expert;

	repeating_cmd = 0;
	expert = getenv(EXPERT) != 0;

	/* Check for empty command string and determine repeat. */

	if(!*p || strempty(p)) {
#if NCMD_HIST > 0
		char	*t, tmp[11];
		int	rptcmd;

		rptcmd = matchenv ("rptcmd");

		if(rptcmd) {
			repeating_cmd = 1;
			t = gethistn(histno - 1);
			if(rptcmd == 1) {
				strcpy (p, t);	/* all cmds */
			}
			else if(rptcmd == 2) {	/* trace only */
				if(wordsz(t) > 10) {
					return -1;
				}
				getword(tmp, t);
				if (!strcmp(tmp, "t") || !strcmp(tmp, "to")) {
					strcpy (p, tmp);
				}
				else {
					return -1;
				}
			}
			else {
				printf ("check rptcmd value\n");
				return -1;
			}
		}
		else {
			return -1;
		}
#else
		return -1;
#endif /* NCMD_HIST */
	}

	/* Expand command substituting $vars. Breakup into separate cmds */

	if(!(p = expand (p))) {
		return -1;
	}

	nc = 0;
	cmdlist[nc++] = p;
	for (; *p;) {
		c = *p;
		if(c == '\'' || c == '"') {
			p++;
			while (*p && *p != c) {
				++p;
			}
			if(*p) {
				p++;
			}
			else {
				printf("unbalanced %c\n", c);
				return -1;
			}
		}
		else if(c == ';') {
			*p++ = 0;
			cmdlist[nc++] = p;
		}
		else {
			p++;
		}
	}

	/*
	 *  Lookup command in command list and dispatch.
	 */
	for (j = 0; j < nc; j++) {
		int stat = -1;
		ac = argvize (av, cmdlist[j]);
		if(ac == 0) {
			return -1;
		}

		clistno = 0;
		while((CmdTable = CmdList[clistno++]) != 0) {
			for (i = 1; CmdTable[i].name != 0; i++) {
				if (!strcmp (CmdTable[i].name, av[0])) {
					break;
				}
			}
			if(CmdTable[i].name != 0) {
				break;
			}
		}
		if(CmdTable == 0 || (CmdTable[i].flag & CMD_HIDE && !expert)) {
			printf ("%s: Command not found. Try 'h' for help!\n", av[0]);
			return -1;
		}

		if(repeating_cmd && !(CmdTable[i].flag & CMD_REPEAT) ) {
			repeating_cmd = 0;
			return -1;
		}
	
		if(ac < CmdTable[i].minac) {
			printf ("%s: not enough arguments\n", CmdTable[i].name);
		}
		else if(ac > CmdTable[i].maxac) {
			printf ("%s: too many arguments\n", CmdTable[i].name);
		}
		else {
			stat = (CmdTable[i].func) (ac, av);
		}

		if(stat < 0) {
			printf ("usage: %s %s\n", CmdTable[i].name, CmdTable[i].opts);
		}

		if(stat != 0) {
			//break;	/* skip commands after ';' */
			return stat;
		}
	}
	return 0;
}

/*
 * expand(cmdline) - expand environment variables
 * entry:
 *	char *cmdline pointer to input command line
 * returns:
 *	pointer to static buffer containing expanded line.
 */
static char *
expand(cmdline)
	char *cmdline;
{
	char *ip, *op, *v;
	char var[256];
	static char expline[LINESZ + 8];

	if(!strchr (cmdline, '$')) {
		return cmdline;
	}

	ip = cmdline;
	op = expline;
	while (*ip) {
		if(op >= &expline[sizeof(expline) - 1]) {
			printf ("Line too long after expansion\n");
			return (0);
		}

		if(*ip != '$') {
			*op++ = *ip++;
			continue;
		}

		ip++;
		if(*ip == '$') {
			*op++ = '$';
			ip++;
			continue;
		}

		/* get variable name */
		v = var;
		if(*ip == '{') {
			/* allow ${xxx} */
			ip++;
			while (*ip && *ip != '}') {
				*v++ = *ip++;
			}
			if(*ip && *ip != '}') {
				printf ("Variable syntax\n");
				return (0);
			}
			ip++;
		} else {
			/* look for $[A-Za-z0-9]* */
			while (isalpha(*ip) || isdigit(*ip)) {
				*v++ = *ip++;
			}
		}

		*v = 0;
		if(!(v = getenv (var))) {
			printf ("'%s': undefined\n", var);
			return (0);
		}

		if(op + strlen(v) >= &expline[sizeof(expline) - 1]) {
			printf ("Line expansion ovf.\n");
			return (0);
		}

		while (*v) {
			*op++ = *v++;
		}
	}
	*op = '\0';
	return (expline);
}
