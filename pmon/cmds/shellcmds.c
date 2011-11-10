/* $Id: shellcmds.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */
/*
 * Copyright (c) 2001 Opsycon AB  (www.opsycon.se / www.opsycon.com)
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
#include <endian.h>
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
#include "cmd_shell.h"



const Optdesc sh_opts[] = {
	{"^H",		"delete char before cursor"},
#if NCMD_HIST > 0
	{"^D",		"delete char at cursor"},
	{"^K",		"delete line from cursor"},
	{"^W",		"delete line"},
	{"^F",		"move cursor right"},
	{"^B",		"move cursor left"},
	{"^A",		"move cursor far left"},
	{"^E",		"move cursor far right"},
	{"^P",		"recall previous cmd"},
	{"^N",		"recall next cmd"},
	{"",		""},
	{"!!",		"repeat last cmd"},
	{"!str",	"recall and execute cmd str"},
	{"!num",	"recall and execute cmd num"},
	{"",		""},
	{"+-*/()",	"operators"},
	{"^addr",	"contents of address"},
	{"@name",	"contents of register"},
	{"&name",	"value of symbol"},
	{"0xnum",	"hex number"},
	{"0onum",	"octal number"},
	{"0tnum",	"decimal number"},
#endif
	{0}
};


int cmd_help __P((int, char *[]));
int cmd_vers __P((int, char *[]));
int cmd_eval __P((int, char *[]));


/*
 *    BASIC SHELL COMMANDS
 *    ====================
 */

#if (NCMD_SHELL + NCMD_EVAL) > 0
/*
 *  Evaluate an expression and print the result.
 */
int
cmd_eval(int ac, char *av[])
{
	register_t v;

	if(ac != 2) {
		return(-1);
	}

	if(!get_rsa_reg(&v, av[1])) {
                return (-1);
        }
#ifdef HAVE_QUAD
	printf("0x%llx  0%llo  %lld\n", v, v, v);
#else
	printf("0x%x  0%o  %d\n", v, v, v);
#endif
	return(0);
}
#endif /* NCMD_EVAL */


#if (NCMD_SHELL + NCMD_VERS) > 0
/*
 *  Print out version info
 *  ----------------------
 */
const Optdesc vers_opts[] = {
	{"-a", "list all known version numbers"},
	{0}
};

int
cmd_vers(int ac, char *av[])
{
	int c;
	int aflag = 0;
extern int optind;

	optind = 0;
	while((c = getopt(ac, av, "a")) != EOF) {
		switch(c) {
		case 'a':
			aflag = 1;
			break;

		default:
			return(-1);
		}
	}

	if(optind > ac) {
		return(-1);
	}

	printf ("PMON: %s\n", vers);

	if(aflag) {
		tgt_cmd_vers();
	}

	return(0);
}
#endif /* NCMD_VERS */

#if (NCMD_SHELL + NCMD_HELP) > 0
/*
 *  Help command
 *  ------------
 *
 *  Display a list of avialable commands. If a specific command is
 *  specified, show command specific help.
 */
static int
prhelp (const Cmd *cmd, int *lnp, int siz, int namemax, int optsmax)
{
	char prnbuf[LINESZ + 8];
	const Optdesc *p;
	int i;

	sprintf (prnbuf, "%*s  %-*s %s", 
			namemax, cmd->name, optsmax, cmd->opts, cmd->desc);
	if(more (prnbuf, lnp, siz)) {
		return (1);
	}

	p = cmd->optdesc;
	if(p != 0) {
		for (i = 0; p[i].name; i++) {
			sprintf (prnbuf, "%*s  %15s    %s", namemax, "",
					p[i].name, p[i].desc);
			if(more (prnbuf, lnp, siz)) {
				return (1);
			}
		}
	}
	return (0);
}

int
cmd_help(int ac, char *av[])
{
	char prnbuf[LINESZ + 8];
	int i, j, namemax, optsmax, descmax, len;
	int ln, siz;
	int clistno;
	int expert;
	const Cmd *CmdTable;

	namemax = optsmax = descmax = 0;
	expert = getenv(EXPERT) != 0;

	clistno = 0;
	while((CmdTable = CmdList[clistno++]) != 0) {
		for (i = 1; CmdTable[i].name != 0; i++) {
			len = strlen (CmdTable[i].name);
			if(len > namemax) {
				namemax = len;
			}
			len = strlen (CmdTable[i].opts);
			if(len > optsmax) {
				optsmax = len;
			}
			len = strlen (CmdTable[i].desc);
			if(len > descmax) {
				descmax = len;
			}
		}
	}

	siz = moresz;
	ioctl (STDIN, CBREAK, NULL);

	ln = siz;
	if(ac >= 2 && !strcmp(av[1], "*")) {	/* all commands */
		clistno = 0;
		while((CmdTable = CmdList[clistno++]) != 0) {
			for (i = 1; CmdTable[i].name != 0; i++) {
				if((CmdTable[i].flag & CMD_HIDE && !expert) ||
				    CmdTable[i].flag & CMD_ALIAS) {
				}
				else if(prhelp (&CmdTable[i], &ln, siz, namemax, optsmax)) {
					return(0);
				}
			}
		}
	}
	else if(ac >= 2) {		/* specific commands */
		for (j = 1; j < ac; j++) {
			clistno = 0;
			while((CmdTable = CmdList[clistno++]) != 0) {
				for (i = 1; CmdTable[i].name != 0; i++) {
					if(!strcmp(CmdTable[i].name, av[j])) {
						break;
					}
				}
				if(CmdTable[i].name && (!(CmdTable[i].flag & CMD_HIDE) || expert)) {
					if(prhelp (&CmdTable[i], &ln, siz, namemax, optsmax)) {
						return(0);
					}
					break;
				}
			}
			if(CmdTable == 0) {
				printf ("%s: not found\n", av[j]);
			}
		}
	}
	else {			/* general help only */
		const char *p = "";
		clistno = 0;
		i = 0;
		j = 0;
		while((CmdTable = CmdList[clistno++]) != 0) {
			if(strcmp(CmdTable->name, p)) {
				if(i % 2 != 0) {
					if(more(prnbuf, &ln, siz)) {
						return(0);
					}
				}
				printf("%~70s\n", CmdTable->name);
				i = 0;
				j = 0;
				p = CmdTable->name;
			}
			CmdTable++;
			while(CmdTable->name != 0) {
				if (!(CmdTable->flag & CMD_ALIAS) &&
				    (!(CmdTable->flag & CMD_HIDE) || expert)) {
					j += sprintf(prnbuf + j, "%*s  %-*s", namemax,
					       CmdTable->name, descmax, CmdTable->desc);
					if(i % 2 != 0) {
						if(more(prnbuf, &ln, siz)) {
							return(0);
						}
						j = 0;
					}
					else {
						j += sprintf(prnbuf + j, "   ");
					}
					i++;
				}
				CmdTable++;
			}
		}
		if(i % 2 != 0) {
			printf ("%s\n", prnbuf);
		}
	}
	return (0);
}

#endif /* NCMD_HELP */


/*
 *  Command table registration.
 */
static const Cmd Cmds[] =
{
	{"Shell"},
#if (NCMD_SHELL + NCMD_HELP) > 0
	{"h",		"[*|cmd..]",
			0,
			"on-line help",
			cmd_help, 1, 99, 0},
#endif
	{"sh",		"",
			sh_opts,
			"command shell",
			no_cmd, 1, 99, CMD_REPEAT},
#if (NCMD_SHELL + NCMD_VERS) > 0
	{"vers",	"[-a]",
			vers_opts,
			"print version info",
			cmd_vers, 1, 99, CMD_REPEAT },
#endif
#if (NCMD_SHELL + NCMD_EVAL) > 0
	{"eval",	"expression",
			0,
			"evaluate and print result",
			cmd_eval, 1, 99, CMD_REPEAT },
#endif
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 0);
}
