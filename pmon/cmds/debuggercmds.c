/* $Id: debuggercmds.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */
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
#include <stdlib.h>

#include <signal.h>
#include <machine/cpu.h>
#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif

#include <pmon.h>
#include <debugger.h>

extern void _go __P((void));

int cmd_cont (int, char *[]);
int cmd_trace (int, char *[]);
int cmd_setbp (int, char *[]);
int cmd_clrbp (int, char *[]);
int cmd_cpu (int, char *[]);

char            clientcmd[LINESZ];
char           *clientav[MAX_AC];
int		clientac;


/*************************************************************
 *  cont(ac,av) the continue command
 */
int
cmd_cont (ac, av)
	int	ac;
	char	*av[];
{
	int32_t	adr;

	BptTmp.addr = NO_BPT;
	BptTrc.addr = NO_BPT;
	BptTrcb.addr = NO_BPT;
	if (ac > 1) {
		if (!get_rsa (&adr, av[1])) {
			return (-1);
		}
		BptTmp.addr = adr;
	}
	goclient ();
	return(0); /* Shut up gcc */
}

/*************************************************************
 *  trace(ac,av) the 't' (single-step) command
 */
const Optdesc         cmd_t_opts[] = {
	{"-v", "verbose, list each step"},
	{"-b", "capture only branches"},
	{"-c", "capture only calls"},
	{"-i", "stop on pc invalid"},
	{"-m adr val", "stop on mem equal"},
	{"-M adr val", "stop on mem not equal"},
	{"-r reg val", "stop on reg equal"},
	{"-R reg val", "stop on reg not equal"},
	{0}
};

int
cmd_trace (ac, av)
	int ac;
	char *av[];
{
	int multi, i, j, n;
	register_t *reg;
	u_int32_t adr, val;
	trace_over = 0;
	if (!strcmp(av[0], "to")) {
		trace_over = 1;
	}

	n = 0;
	multi = 0;
	trace_verbose = 0;
	trace_invalid = 0;
	trace_bflag = 0;
	trace_cflag = 0;

	for (i = 0; i < STOPMAX; i++) {
		stopval[i].addr = 0;
	}
	for (i = 1; i < ac; i++) {
		if (av[i][0] == '-') {
			for (j = 1; av[i][j] != 0; j++) {
				if (av[i][j] == 'v') {
					trace_verbose = 1;
					trace_count = 0;
					multi = 1;
				}
				else if (av[i][j] == 'm' || av[i][j] == 'M') {
					if (i + 2 >= ac) {
						printf ("bad arg count\n");
						return (-1);
					}
					if (!get_rsa (&adr, av[i + 1])) {
						return (-1);
					}
					if (!get_rsa (&val, av[i + 2])) {
						return (-1);
					}
					if (!addstop (adr, val, "MEMORY", av[i][j])) {
						return (1);
					}
					trace_count = 0;
					multi = 1;
					i += 2;
					break;
				}
				else if (av[i][j] == 'r' || av[i][j] == 'R') {
					if (i + 2 >= ac) {
						printf ("bad arg count\n");
						return (-1);
					}
					if (!md_getregaddr (&reg, av[i + 1])) {
						printf ("%s: bad reg name\n", av[i + 1]);
						return (-1);
					}
					if (!get_rsa (&val, av[i + 2])) {
						return (-1);
					}
					if (!addstop (*reg, val, av[i + 1], av[i][j])) {
						return (1);
					}
					trace_count = 0;
					multi = 1;
					i += 2;
					break;
				}
				else if (av[i][j] == 'b') {
					trace_bflag = 1;
				}
				else if (av[i][j] == 'c') {
					trace_cflag = 1;
				}
				else if (av[i][j] == 'i') {
					trace_invalid = 1;
					trace_count = 0;
					multi = 1;
				}
				else {
					printf ("%c: unrecognized option\n", av[i][j]);
					return (-1);
				}
			}
		}
		else {
			if (n == 0) {
				if (!get_rsa ((u_int32_t *)&trace_count, av[i])) {
					return (-1);
				}
				multi = 1;
			}
			else {
				printf ("%s: unrecognized argument\n", av[i]);
				return (-1);
			}
			n++;
		}
	}
	if(setTrcbp(md_get_excpc(NULL), trace_over))
		return (1);
	clrpchist ();
	if (multi)
		trace_mode = TRACE_TN;
	else
		trace_mode = TRACE_TB;
	store_trace_breakpoint ();
	console_state(2);
	_go ();
	return(0); /* Shut up gcc */
}


/*************************************************************
 *  setbp(ac,av) the 'b' (set breakpoint) command
 */
const Optdesc         cmd_b_opts[] = {
#ifdef R4000
#define b_args "[-drws][adr].."
	{"-d", "hw bpt for data access"},
	{"-r", "hw bpt for data read only"},
	{"-w", "hw bpt for data write only"},
	{"-s", "command string"},
#else
#define b_args "[-s][adr].."
	{"-s", "command string"},
#endif
	{0}
};

int
cmd_setbp (ac, av)
	int	ac;
	char	*av[];
{
	int32_t	adr, i, j, w, x;
	char	*str;
	int	flag = 0;

	if (ac == 1) {
		dspbpts ();
		return (0);
	}
	w = 0;
	str = 0;
	for (i = 1; i < ac; i++) {
		if (av[i][0] == '-') {
			x = 0;
			for (j = 1; av[i][j] != 0; j++) {
				if (av[i][j] == 's') {
					i++;
					if (i >= ac) {
						printf ("bad arg count\n");
						return (-1);
					}
					str = av[i];
					break;
				}
#if 0//def R4000
		else if (av[i][j] == 'd')
		    w |= WATCH_R | WATCH_W;
		else if (av[i][j] == 'r')
		    w |= WATCH_R;
		else if (av[i][j] == 'w')
		    w |= WATCH_W;
#endif
				else {
					printf ("%c: unrecognized option\n", av[i][j]);
					return (-1);
				}
			}
		}
		else {
			flag = 1;
			if (!get_rsa (&adr, av[i])) {
				return (-1);
			}
		}
	}


	/*
	 *  Find a free breakpoint but reuse the same slot
	 *  if a break gets parameters changed.
	 */
	for (j = 0, i = MAX_BPT; j < MAX_BPT; j++) {
		if (Bpt[j].addr == adr) {
			Bpt[j].addr = NO_BPT;
			if (Bpt[j].cmdstr) {
				free (Bpt[j].cmdstr);
			}
			i = j;
		}
		if(i == MAX_BPT && Bpt[j].addr == NO_BPT) {
			i = j;
		}
	}
	if (MAX_BPT <= i) {
		printf ("too many breakpoints\n");
		return (1);
	}

	Bpt[i].addr = adr;
	printf ("Bpt %2d = %08x", i, adr);
	if (adr & 3L) {
		printf (" -> ??");
	}
	if (str != 0) {
		Bpt[i].cmdstr = malloc (strlen (str) + 1);
		strcpy (Bpt[i].cmdstr, str);
		str = 0;
		printf (" \"%s\"", Bpt[i].cmdstr);
	}
	else {
		Bpt[i].cmdstr = 0;
	}

	putchar ('\n');

	if (!flag) {
		printf ("break address not specified\n");
	}
	return (0);
}
/*************************************************************
 *  clrbp(ac,av)
 *      The 'db' command
 */
int
cmd_clrbp (ac, av)
	int	ac;
	char	*av[];
{
	int32_t	i, j;

	if (ac > 1) {
		for (i = j = 0; j < ac - 1; j++) {
			if (!strcmp(av[1 + j], "*")) {
				clrbpts ();
				continue;
			}
			if (!atob (&i, av[1 + j], 10)) {
				printf ("%s: decimal number expected\n", av[1 + j]);
				return (-1);
			}
			if (i < MAX_BPT) {
				if (Bpt[i].addr == NO_BPT) {
					printf ("Bpt %2d not set\n", i);
				}
				else {
					Bpt[i].addr = NO_BPT;
					if (Bpt[i].cmdstr) {
						free (Bpt[i].cmdstr);
					}
				}
			}
#if 0//def R4000
			else if (i == MAX_BPT) {
				if (WatchLo & (WATCH_R | WATCH_W)) {
					WatchLo = 0;
				}
				else {
					printf ("Bpt %2d not set\n", i);
				}

			}
#endif
			else {
				printf ("%d: breakpoint number too large\n", i);
			}
		}
	}
	else {
		dspbpts ();
	}
	return (0);
}

#if defined(SMP)
/*
 *  cmd:cpu - select cpuinfo area for debug commands.
 */
int
cmd_cpu (ac, av)
	int	ac;
	char	*av[];
{
	int32_t	cpu;

	if (ac > 1) {
		if (!get_rsa (&cpu, av[1])) {
			return (-1);
		}
		if (cpu < 1 || cpu > 2) {
			printf("Invalid CPU!\n");
			return (1);
		}
		whatcpu = cpu - 1;
	}
	printf("Selected CPU=%d\n", whatcpu + 1);
	return(0);
}
#endif

/*
 *  Command table registration
 *  ==========================
 */

static const Cmd DebugCmd[] =
{
	{"Debugger"},
#if defined(DBX)
	{"debug",	"[-svV][-h port][-- args]",
			cmd_debug_opts,
			"remote debug mode",
			cmd_debug, 1, 99, 0},
#endif
#if defined(SMP)
	{"cpu",		"[cpu]",
			0,
			"select cpuinfo",
			cmd_cpu, 1, 2, CMD_REPEAT},
#endif
	{"c",		"[bptadr]",
			0,
			"continue execution",
			cmd_cont, 1, 2, CMD_REPEAT},
	{"t",		"[-vibcmMrR] [cnt]",
			cmd_t_opts,
			"trace (single step)",
			cmd_trace, 1, 99, CMD_REPEAT},
	{"to",		"[-vibc] [cnt]",
			cmd_t_opts,
			"trace (step over)",
			cmd_trace, 1, 99, CMD_REPEAT},
	{"db",		"[numb|*]..",
			0,
			"delete break point(s)",
			cmd_clrbp, 1, 99, CMD_REPEAT},
	{"b",		b_args,
			cmd_b_opts,
			"set break point(s)",
			cmd_setbp, 1, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(DebugCmd, 1);
}
