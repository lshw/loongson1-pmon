/*	$Id: exception.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2000-2002 Opsycon AB
 * Copyright (c) 2002 Patrik Lindergren (www.lindergren.com)
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
 *	This product includes software developed by Patrik Lindergren.
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
#include <ri.h>
#include "mod_debugger.h"

extern void _go __P((void));

/*
 *  For each CPU this table have a pointer to its cpuinfo area.
 */
struct trapframe *cpuinfotab[8];
#if NMOD_DEBUGGER > 0
/*
 *  exception()
 *      An exception has been generated. Need to sort out from where.
 *      frame is a pointer to cpu data saved on the PMON2000 stack.
 */
void
exception(frame)
	struct trapframe *frame;
{
	int exc_type;
	int i, flag;
	char *p = 0;
	struct trapframe *cpuinfo;
#ifdef BONITOEL
	struct pt_regs *xcp=(struct pt_regs *)malloc(sizeof(struct pt_regs));
	register_t *ptmp=(register_t *)frame;
#endif
extern char *sbddbgintr(unsigned int);

#if defined(SMP)
extern char _pmon_snap_trap;

	cpuinfo = cpuinfotab[tgt_smpwhoami()];
	*cpuinfo = *frame;		/* Save the frame */
	if (md_getpc(frame) == &_pmon_snap_trap) {
		/* XXX Instruction size */
		md_setpc(cpuinfo, (register_t)&_pmon_snap_trap+4);
		_go();
	}
	if (tgt_smpwhoami() != 0) {
		printf("PMON: CPU %d idling\n", tgt_smpwhoami() + 1);
		tgt_smpidle();
		_go();
	}
#else
	cpuinfo = cpuinfotab[0];
	*cpuinfo = *frame;		/* Save the frame */
#endif

	exc_type = md_exc_type(frame);

#if 0
    if (exc_type == EXC_INT && (p = sbddbgintr (Cause & Status)))
      printf ("\r\n%s Interrupt\r\n", p);

    if (trace_mode == TRACE_DC || trace_mode == TRACE_DS) {
	/* pass all exceptions to remote debugger */
	remove_trace_breakpoint ();
	dbgmode ();
    }
#endif
#ifdef BONITOEL
	if(exc_type ==EXC_RES){
		xcp->cp0_badvaddr=frame->badvaddr;
		xcp->cp0_cause=frame->cause;
		xcp->cp0_epc=frame->pc;
		xcp->cp0_status=frame->sr;
		xcp->lo=frame->mullo;
		xcp->hi=frame->mulhi;
		for (i=0;i<32;i++)	xcp->regs[i]=*(ptmp+i);
		for (i=0;i<6;i++)	xcp->pad0[i]=0;
		if(do_ri(xcp)) {exc_type=EXC_BAD;pmon_stop(0);}
	}
#endif

#ifdef BONITOEL
	else if(exc_type != EXC_BPT && exc_type != EXC_TRC && exc_type != EXC_WTCH) {	       	
#else
	if(exc_type != EXC_BPT && exc_type != EXC_TRC && exc_type != EXC_WTCH) {	       	
#endif
		if (!p) {
			md_dumpexc(frame);
		}
		pmon_stop (0);
	}
	else if (trace_mode == TRACE_NO) {	/* no bpts set */
		printf ("\r\nBreakpoint reached while not in trace mode!\r\n");
		md_fpsave(cpuinfo);
		pmon_stop (0);
	}
	else if (trace_mode == TRACE_GB) {	/* go & break */
		if (BptTmp.addr != NO_BPT) {
			store_word ((void *)BptTmp.addr, BptTmp.value);
			BptTmp.addr = NO_BPT;
		}
		for (i = 0; i < MAX_BPT; i++) {
			if (Bpt[i].addr != NO_BPT) {
				store_word ((void *)Bpt[i].addr, Bpt[i].value);
			}
		}
		if (exc_type == EXC_WTCH) {
			printf ("\r\nStopped at HW Bpt %d\r\n", MAX_BPT);
		}
		else {
			for (i = 0; i < MAX_BPT; i++) {
				if(Bpt[i].addr == (int)md_get_excpc(frame)) {
					printf ("\r\nStopped at Bpt %d\n", i);
					md_fpsave(cpuinfo);
					pmon_stop (Bpt[i].cmdstr);
				}
			}
		}
		md_fpsave(cpuinfo);
		pmon_stop (0);
	}

	remove_trace_breakpoint ();
	if (trace_mode == TRACE_TB) {
		md_fpsave(cpuinfo);
		pmon_stop (0);		/* trace & break */
	}
	else if (trace_mode == TRACE_TN) {
		for (i = 0; i < MAX_BPT; i++) {
			if(Bpt[i].addr == (int)md_get_excpc(frame)) {
				printf ("\r\nStopped at Bpt %d\r\n", i);
				md_fpsave(cpuinfo);
				pmon_stop (Bpt[i].cmdstr);
			}
		}
		if(trace_invalid && !is_validpc(md_get_excpc(frame))) {
			printf ("\r\nStopped: Invalid PC value\r\n");
			md_fpsave(cpuinfo);
			pmon_stop (0);
		}
		for (i = 0; i < STOPMAX; i++) {
			if (stopval[i].addr == 0) {
				continue;
			}
			if ((stopval[i].sense == 0 &&
				load_word (stopval[i].addr) == stopval[i].value)
				|| (stopval[i].sense == 1 &&
				load_word (stopval[i].addr) != stopval[i].value)) {
				if (stopval[i].sense == 0) {
					p = " == ";
				}
				else {
					p = " != ";
				}
				if (!strcmp(stopval[i].name, "MEMORY")) {
					printf ("\r\nStopped: 0x%08x%s0x%08x\r\n",
						stopval[i].addr, p, stopval[i].value);
				}
				else {
					printf ("\r\nStopped: %s%s0x%08x\r\n", stopval[i].name,
						p, stopval[i].value);
				}
				md_fpsave(cpuinfo);
				pmon_stop (0);
			}
		}
		flag = 1;
		if (trace_bflag || trace_cflag) {
			if(trace_bflag && md_is_branch(md_get_excpc(frame))) {
				flag = 1;
			}
			else if(trace_cflag && md_is_call(md_get_excpc(frame))) {
				flag = 1;
			}
			else {
				flag = 0;
			}
		}
		if (flag) {
			addpchist (md_get_excpc(frame));
			if (trace_verbose) {
#if NCMD_L > 0
				char tmp[80];
				md_disasm (tmp, md_get_excpc(frame));
				printf ("%s\r\n", tmp);
#endif /* */
#ifdef HAVE_DLYSLOT
				if (md_is_branch(md_get_excpc(frame))) {
				/* print the branch delay slot too */
					md_disasm(tmp, md_get_excpc(frame) + 4);
					printf ("%s\r\n", tmp);
				}
#endif
			}
			else {
				dotik (256, 1);
			}
		}
		else {
			dotik (256, 1);
		}
		if (trace_count) {
			trace_count--;
		}
		if (trace_count == 1) {
			trace_mode = TRACE_TB;
		}
		if (setTrcbp (md_get_excpc(frame), trace_over)) {
			md_fpsave(cpuinfo);
			pmon_stop (0);
		}
		store_trace_breakpoint ();
		console_state(2);
		_go ();
	}
/* else TRACE_TG  trace & go, set on g or c if starting at bpt */

	trace_mode = TRACE_GB;	/* go & break */
	store_breakpoint ();
	console_state(2);
	_go ();
}
#else
void
exception(frame)
	struct trapframe *frame;
{
#if 0
	int exc_type;
	int i, flag;
	char *p = 0;
extern char    *sbddbgintr(unsigned int);

	cpuinfo= *frame;

	exc_type = md_exc_type(frame);

	if(exc_type != EXC_BPT && exc_type != EXC_TRC && exc_type != EXC_WTCH) {
		if (!p) {
			printf ("\r\nException Cause=%s, SR=%p, PC=%p\r\n",
				md_getexcname(frame), (int)md_getsr(), md_getpc());
		}
		pmon_stop (0);
	}
	else if (trace_mode == TRACE_NO) {	/* no bpts set */
		printf ("\r\nBreakpoint reached while not in trace mode!\r\n");
		md_fpsave(cpuinfo);
		pmon_stop (0);
	}
	else if (trace_mode == TRACE_GB) {	/* go & break */
		if (BptTmp.addr != NO_BPT) {
			store_word ((void *)BptTmp.addr, BptTmp.value);
			BptTmp.addr = NO_BPT;
		}
		for (i = 0; i < MAX_BPT; i++) {
			if (Bpt[i].addr != NO_BPT) {
				store_word ((void *)Bpt[i].addr, Bpt[i].value);
			}
		}
		if (exc_type == EXC_WTCH) {
			printf ("\r\nStopped at HW Bpt %d\r\n", MAX_BPT);
		}
		else {
			for (i = 0; i < MAX_BPT; i++) {
				if(Bpt[i].addr == (int)md_get_excpc(frame)) {
					printf ("\r\nStopped at Bpt %d\n", i);
					md_fpsave(cpuinfo);
					pmon_stop (Bpt[i].cmdstr);
				}
			}
		}
		md_fpsave(cpuinfo);
		pmon_stop (0);
	}
    
	remove_trace_breakpoint ();
	if (trace_mode == TRACE_TB) {
		md_fpsave(cpuinfo);
		pmon_stop (0);		/* trace & break */
	}
	else if (trace_mode == TRACE_TN) {
		for (i = 0; i < MAX_BPT; i++) {
			if(Bpt[i].addr == (int)md_get_excpc(frame)) {
				printf ("\r\nStopped at Bpt %d\r\n", i);
				md_fpsave(cpuinfo);
				pmon_stop (Bpt[i].cmdstr);
			}
		}
		if(trace_invalid && !is_validpc(md_get_excpc(frame))) {
			printf ("\r\nStopped: Invalid PC value\r\n");
			md_fpsave(cpuinfo);
			pmon_stop (0);
		}
		for (i = 0; i < STOPMAX; i++) {
			if (stopval[i].addr == 0) {
				continue;
			}
			if ((stopval[i].sense == 0 &&
				load_word (stopval[i].addr) == stopval[i].value)
				|| (stopval[i].sense == 1 &&
				load_word (stopval[i].addr) != stopval[i].value)) {
				if (stopval[i].sense == 0) {
					p = " == ";
				}
				else {
					p = " != ";
				}
				if (!strcmp(stopval[i].name, "MEMORY")) {
					printf ("\r\nStopped: 0x%08x%s0x%08x\r\n",
						stopval[i].addr, p, stopval[i].value);
				}
				else {
					printf ("\r\nStopped: %s%s0x%08x\r\n", stopval[i].name,
						p, stopval[i].value);
				}
				md_fpsave(cpuinfo);
				pmon_stop (0);
			}
		}
		flag = 1;
		if (trace_bflag || trace_cflag) {
			if(trace_bflag && md_is_branch(md_get_excpc(frame))) {
				flag = 1;
			}
			else if(trace_cflag && md_is_call(md_get_excpc(frame))) {
				flag = 1;
			}
			else {
				flag = 0;
			}
		}
		if (flag) {
			addpchist (md_get_excpc(frame));
			if (trace_verbose) {
			}
			else {
				dotik (256, 1);
			}
		}
		else {
			dotik (256, 1);
		}
		if (trace_count) {
			trace_count--;
		}
		if (trace_count == 1) {
			trace_mode = TRACE_TB;
		}
		if (setTrcbp (md_get_excpc(frame), trace_over)) {
			md_fpsave(cpuinfo);
			pmon_stop (0);
		}
		store_trace_breakpoint ();
		console_state(2);
		_go ();
	}
/* else TRACE_TG  trace & go, set on g or c if starting at bpt */

	trace_mode = TRACE_GB;	/* go & break */
	store_breakpoint ();
#endif
	console_state(2);
	_go ();
}
#endif /* NMOD_DEBUGGER */



