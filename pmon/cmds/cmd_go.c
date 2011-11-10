/* $Id: cmd_go.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */
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
#include <machine/frame.h>
#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif

#include <pmon.h>
#include <debugger.h>

#include "mod_debugger.h"

#include "initrd.h"

extern struct trapframe DBGREG;
extern struct trapframe TRPREG;
extern int retvalue;

extern void _go __P((void));

int cmd_go __P((int, char *[]));

char            clientcmd[LINESZ];
char           *clientav[MAX_AC];
int		clientac;

extern jmp_buf	go_return_jump;

const Optdesc         cmd_g_opts[] = {
	{"-s", "don't set client sp"},
#if 0
	{"-t", "time execution"},
#endif
	{"-e <adr>", "start address"},
#if NMOD_DEBUGGER > 0
	{"-b <bptadr>", "temporary breakpoint"},
#endif
	{"-- <args>", "args to be passed to client"},
	{0}
};

#if NMOD_USB_UHCI != 0
extern void usb_uhci_stop(void);
#endif
/*************************************************************
 *  go(ac,av), the 'g' command
 */
int
cmd_go (ac, av)
	int	ac;
	char	*av[];
{
	int32_t	adr;
	int	sflag;
	int	c;
extern int	optind;
extern char	*optarg;

	sflag = 0;
#if NMOD_DEBUGGER > 0
	BptTmp.addr = NO_BPT;
	BptTrc.addr = NO_BPT;
	BptTrcb.addr = NO_BPT;
#endif

	strcpy (clientcmd, av[0]);
	strcat (clientcmd, " ");

	optind = 0;
	while ((c = getopt (ac, av, "b:e:st")) != EOF) {
		switch (c) {
		case 's':
			sflag = 1; 
			break;
		case 't':
			strcpy (clientcmd, "time ");
			break;
#if NMOD_DEBUGGER > 0
		case 'b':
			if (!get_rsa (&adr, optarg)) {
				return (-1);
			}
			BptTmp.addr = adr;
#endif
			break;
		case 'e':
			if (!get_rsa (&adr, optarg)) {
				return (-1);
			}
			md_setentry(NULL, adr);
			break;
		default:
			return (-1);
		}
	}

	while (optind < ac) {
		strcat (clientcmd, av[optind++]);
		strcat (clientcmd, " ");
	}

	if (initrd_execed())
	{
		char buf[30];
		sprintf(buf, "rd_start=0x%x rd_size=0x%x ", 
			get_initrd_start(), get_initrd_size());
		strcat(clientcmd, buf);
	}

	if (!sflag) {
		md_adjstack(NULL, tgt_clienttos ());
	}

	clientac = argvize (clientav, clientcmd);

	initstack (clientac, clientav, 1);
	md_registers(1,NULL);
	clrhndlrs();
	closelst(2);		/* Init client terminal state */
	md_setsr(NULL, initial_sr);
	tgt_enable (tgt_getmachtype ()); /* set up i/u hardware */

//	CPU_FlushCache();		//lxy
//	CPU_DisableCache();
#ifdef VMWORKS
	if(getenv("vxWorks")) {
		strcpy ((void *)0x4200, getenv ("vxWorks"));
	}
#endif
#if NMOD_USB_UHCI > 0
	usb_uhci_stop();
#endif
#if NMOD_DEBUGGER > 0
	if (setjmp (go_return_jump) == 0) {	
		goclient ();
	}
#else
	if (setjmp (go_return_jump) == 0) {
		console_state(2);
		_go();
	}
#endif
	console_state(1);
	printf("\nclient return: %d\n", retvalue);
	return 0;
}


/*
 *  Command table registration
 *  ==========================
 */

static const Cmd DebugCmd[] =
{
	{"Debugger"},
#if NMOD_DEBUGGER > 0
	{"g",		"[-s][-b bpadr][-e adr][-- args]",
#else
	{"g",		"[-s][-e adr][-- args]",
#endif
			cmd_g_opts,
			"start execution (go)",
			cmd_go, 1, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(DebugCmd, 1);
}
