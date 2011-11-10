/* $Id: debugger.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2000-2002 Opsycon AB  (www.opsycon.se)
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
 *	This product includes software developed by Opsycon AB.
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

#include "mod_symbols.h"

extern void _go __P((void));

int             trace_mode;
unsigned long   trace_count;
int             trace_verbose;
int             trace_invalid;
int             trace_over;
int             trace_bflag;
int             trace_cflag;

Bps             Bpt[MAX_BPT];   /* user break points                    */
Bps             BptTmp;         /* tmp bpt used for continue cmd        */
Bps             BptTrc;         /* bpt for tracing                      */
Bps             BptTrcb;        /* bpt for tracing through branches     */

extern int      clkdat;

Stopentry       stopval[STOPMAX];

unsigned long   pchist_d[PCHISTSZ + 1];
int             pchist_ip, pchist_op;

/*
 *  addstop(adr,val,name,sense)
 */
int
addstop (adr, val, name, sense)
	u_int32_t adr, val;
	char *name, sense;
{
	int i;

	for (i = 0; i < STOPMAX; i++) {
		if (stopval[i].addr == 0) {
			break;
		}
	}
	if (i >= STOPMAX) {
		printf ("stopval table full\n");
		return (0);
	}
	stopval[i].addr = adr;
	stopval[i].value = val;
	strcpy (stopval[i].name, name);
	if (sense == 'M' || sense == 'R') {
		stopval[i].sense = 1;
	}
	else {
		stopval[i].sense = 0;
	}
	return (1);
}

/*
 *  dspbpts() display all breakpoints
 */
void
dspbpts ()
{
	int	i, ln, siz;
	char	tmp[64], buf[100];

	siz = moresz;
	ioctl (STDIN, CBREAK, NULL);
	ln = siz;
	for (i = 0; i < MAX_BPT; i++)
		if (Bpt[i].addr != NO_BPT) {
			sprintf (buf, "Bpt %2d = %08x ", i, Bpt[i].addr);
#if NMOD_SYMBOLS > 0
			if (adr2symoff (tmp, Bpt[i].addr, 0)) {
				strcat (buf, tmp);
			}
#endif
			if (Bpt[i].addr & 3L) {
				strcat (buf, " -> ??");
			}
			if (Bpt[i].cmdstr) {
				sprintf (tmp, " \"%s\"", Bpt[i].cmdstr);
				strcat (buf, tmp);
			}
			if (more (buf, &ln, siz)) {
				break;
			}
		}
#if 0//def R4000
	if (WatchLo & (WATCH_R | WATCH_W)) {
		register_t pa = WatchLo & WATCH_PA;
		printf ("Bpt %2d = %08x", MAX_BPT, pa);
		if (adr2symoff (tmp, pa, 0)) {
			printf (" %s", tmp);
		}
		printf (" [");
		if (WatchLo & WATCH_R) {
			printf (" read");
		}
		if (WatchLo & WATCH_W) {
			printf (" write");
		}
		printf (" ]\n");
	}
#endif
}
/*
 *  rm_bpts()
 */
void
rm_bpts ()
{
	int	i;

	if (BptTmp.addr != NO_BPT && load_word (BptTmp.addr) == BPT_CODE) {
		store_word ((void *)BptTmp.addr, BptTmp.value);
	}

	for(i = 0; i < MAX_BPT; i++) {
		if(Bpt[i].addr != NO_BPT && load_word(Bpt[i].addr) == BPT_CODE) {
			store_word((void *)Bpt[i].addr, Bpt[i].value);
		}
	}
	remove_trace_breakpoint();
}

/*
 *  clrbpts()
 */
void
clrbpts ()
{
	int	i;

	for (i = 0; i < MAX_BPT; i++) {
		Bpt[i].addr = NO_BPT;
		if (Bpt[i].cmdstr)
		free (Bpt[i].cmdstr);
	}
#if 0//defined(R4000)
	WatchLo = 0;
#endif
}

/*
 *  goclient()
 */
int
goclient ()
{
	if(is_break_point(md_getpc(NULL))) {
		if(setTrcbp(md_getpc(NULL), 0)) {
			return (1);
		}
#if 0
		printf("BptTrc.addr=0x%x\n",BptTrc);
		printf("BptTrcb.addr=0x%x\n",BptTrc);
#endif
		trace_mode = TRACE_TG;
		store_trace_breakpoint ();
	}
	else {
		trace_mode = TRACE_GB;
		store_breakpoint ();
	}

	console_state(2);
#if defined(SMP)
	if (whatcpu != 0) {
		tgt_smpschedule(whatcpu);
		return(1);
	}
#endif
	_go ();
	return(0);
}

/*
 * sstep()
 */
void
sstep()
{
	if (setTrcbp (md_getpc(NULL), 0))
		return;
	trace_mode = TRACE_DS;
	store_trace_breakpoint ();
	console_state(2);
	_go ();
}

/*
 *  pmon_stop(cmdstr)
 */
void
pmon_stop (cmdstr)
	char *cmdstr;
{
	char cmd[LINESZ];

	console_state(1);
	trace_mode = TRACE_NO;
	if (cmdstr) {
		strcpy (cmd, cmdstr);
	}
	else {
		strcpy (cmd, getenv ("brkcmd"));
	}
	do_cmd (cmd);
	main ();
}

/*
 *  store_breakpoint()
 */
void
store_breakpoint ()
{
    int             i;

	/*
	 *  Verify that a trace breakpoint won't
	 *  clash with a 'real' breakpoint. If
	 *  that is the case simply remove trace.
	 */
	for (i = 0; i < MAX_BPT; i++) {
		if (BptTmp.addr == Bpt[i].addr) {
			BptTmp.addr = NO_BPT;
		}
		if (BptTrc.addr == Bpt[i].addr) {
			BptTrc.addr = NO_BPT;
		}
		if (BptTrcb.addr == Bpt[i].addr) {
			BptTrcb.addr = NO_BPT;
		}
	}

	/*
	 *  Now do the same check with the trace breaks.
	 */
	if (BptTrc.addr == BptTmp.addr) {
		BptTrc.addr = NO_BPT;
	}
	if (BptTrcb.addr == BptTmp.addr || BptTrcb.addr == BptTrc.addr) {
		BptTrcb.addr = NO_BPT;
	}

	/*
	 *  Activate the 'real' breakpoints.
	 */
	for (i = 0; i < MAX_BPT; i++) {
		if (Bpt[i].addr != NO_BPT) {
			Bpt[i].value = load_word (Bpt[i].addr);
			store_word ((void *)Bpt[i].addr, BPT_CODE);
		}
	}
	if (BptTmp.addr != NO_BPT) {
		BptTmp.value = load_word (BptTmp.addr);
		store_word ((void *)BptTmp.addr, BPT_CODE);
	}
	store_trace_breakpoint ();
}

/*
 *  When doing single step tracing we need to deal with different
 *  aproaches. Some arches have a trace function while others don't
 *  have HW support for it. The following two functions either embed
 *  the traced instruction with a breakpoint or uses the HW trace.
 *  
 *  store_trace_breakpoint()
 *  remove_trace_breakpoint()
 */
void
store_trace_breakpoint ()
{
#ifdef HAVE_TRACE
	if (BptTrc.addr != NO_BPT) {
		md_settrace();
	}
#else
	if (BptTrc.addr != NO_BPT) {
		BptTrc.value = load_word (BptTrc.addr);
		store_word ((void *)BptTrc.addr, BPT_CODE);
	}
	if (BptTrcb.addr != NO_BPT) {
		BptTrcb.value = load_word (BptTrcb.addr);
		store_word ((void *)BptTrcb.addr, BPT_CODE);
	}
#endif
}

void
remove_trace_breakpoint ()
{
#ifndef HAVE_TRACE
	if (BptTrc.addr != NO_BPT && load_word (BptTrc.addr) == BPT_CODE) {
		store_word ((void *)BptTrc.addr, BptTrc.value);
	}
	if (BptTrcb.addr != NO_BPT && load_word (BptTrcb.addr) == BPT_CODE) {
		store_word ((void *)BptTrcb.addr, BptTrcb.value);
	}

	BptTrc.addr = NO_BPT;
	BptTrcb.addr = NO_BPT;
#endif
}

/*
 *  int is_break_point(adr)
 */
int 
is_break_point (adr)
     void *adr;
{
	int i;
#if 0 
	for(i = 0;i <MAX_BPT;i++){
		printf("Bpt[%d].addr=0x%x\n",i,Bpt[i].addr);
	}
	printf("addr=0x%x\n",adr);
#endif
	for (i = 0; i < MAX_BPT; i++) {
		if (Bpt[i].addr == (int)adr) {
			return (1);
		}
		if (BptTmp.addr == (int)adr) {
			return (1);
		}
	}
	return (0);
}

#define NVALIDPC	10
static unsigned long	validpc[NVALIDPC];
static int		nvalidpc = -1;


void
flush_validpc ()
{
    nvalidpc = -1;
}


/* chg_validpc: called if variable is changed */
int
chg_validpc (name, value)
    char *name, *value;
{
    char           *av[NVALIDPC], tmp[80];

    strcpy (tmp, value);
    if (argvize (av, tmp) % 2 != 0) {
	printf ("validpc variable must have even number of values\n");
	return (0);
    }
    /* don't check the values here, symbols may not be loaded */
    flush_validpc ();
    return (1);
}


void
compute_validpc ()
{
    char           *av[NVALIDPC], tmp[80];
    int		    ac, i;

    strcpy (tmp, getenv ("validpc"));
    ac = argvize (av, tmp);
    nvalidpc = 0;
    
    for (i = 0; i < ac; i += 2) {
	if (!get_rsa ((u_int32_t *)&validpc[nvalidpc], av[i]))
	  continue;
	if (!get_rsa ((u_int32_t *)&validpc[nvalidpc+1], av[i+1]))
	  continue;
	nvalidpc += 2;
    }
}


/*
 *  is_validpc(adr)
 */
int
is_validpc(adr)
	void *adr;
{
	int i;

	if (nvalidpc < 0) {
		compute_validpc();
	}

	for (i = 0; i < nvalidpc; i += 2) {
		if ((u_int32_t)adr >= validpc[i] &&
		    (u_int32_t)adr < validpc[i+1]) {
			return (1);
		}
	}
	return (0);
}


/*
 *  addpchist(adr)
 */
void
addpchist(adr)
	void *adr;
{
	pchist_d[pchist_ip] = (int)adr;
	pchist_ip = incmod (pchist_ip, PCHISTSZ);
	if (pchist_ip == pchist_op) {
		pchist_op = incmod (pchist_op, PCHISTSZ);
	}
}

/*
 *  clrpchist()
 */
void
clrpchist ()
{
    pchist_ip = pchist_op = 0;
}

/*
 *  unsigned long getpchist(n)
 */
u_int32_t 
getpchist (n)
     int             n;
{
    int             i;

    i = pchist_ip - n - 1;
    if (i < 0)
	i += PCHISTSZ + 1;
    if (incmod (i, PCHISTSZ) == pchist_op)
	return (0);
    return (pchist_d[i]);
}

/*
 *  setTrcbp(adr,stepover)
 */
int
setTrcbp (adr, stepover)
     void *adr;
     int stepover;
{

	BptTrc.addr = NO_BPT;
	BptTrcb.addr = NO_BPT;
#ifdef HAVE_TRACE
	BptTrc.addr = (int)adr + 4;
#else
	if (md_is_branch(adr)) {
		void *target;
#if 0
		if (md_is_branch ((void *)(adr + 4))) {
			printf ("branch in delay slot\n");
			return (1);
		}
#endif
		target = md_branch_target(adr);
		if (target == adr)
			target = adr + 8;	/* skip self branches */
		if(md_is_cond_branch(adr) && target != adr + 8)
			BptTrc.addr = (int)adr + 8;
		if(md_is_call(adr) && stepover)
			BptTrc.addr = (int)adr + 8;
		else if(md_is_jr(adr) && !md_is_writeable((void *)target))
			BptTrc.addr = md_getlink(NULL);
		else
			BptTrcb.addr = (int)target;
	}
	else {
		BptTrc.addr = (int)adr + 4;
	}
#endif /* HAVE_TRACE */
    return (0);
}
