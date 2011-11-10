/*	$Id: dbg_machdep.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2000-2001 Opsycon AB  (http://www.opsycon.se)
 * Copyright (c) 2000-2001 RTMX, Inc   (http://www.rtmx.com)
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
 *	This product includes software developed for Rtmx, Inc by
 *	Opsycon Open System Consulting AB, Sweden.
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
#include <ctype.h>
#include <stdlib.h>
#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif

#include <machine/cpu.h>

#include <pmon.h>

#include "mod_debugger.h"

extern int memorysize;
extern u_int8_t end[];


/*
 *  Dump out exception info for an unscheduled exception
 */
void
md_dumpexc(struct trapframe *tf)
{
	printf ("\r\nException Cause=%s, SR=%p, PC=%p\r\n",
		md_getexcname(tf), (int)md_getsr(tf), md_getpc(tf));
}


/*
 *  Set PC to a new value
 */
void
md_setpc(struct trapframe *tf, register_t pc)
{
	if (tf == NULL)
		cpuinfotab[whatcpu]->srr0 = pc;
	else
		tf->srr0 = pc;
}

/*
 *  This function clears all client registers for launch.
 */
void
md_clreg(struct trapframe *tf)
{
	int i;

	if (tf == NULL)
		tf = cpuinfotab[whatcpu];
	for(i = 0; i < 32; i++) {
		tf->fixreg[i] = 0;
	}
	tf->lr = 0;
	tf->cr = 0;
	tf->xer = 0;
	tf->ctr = 0;
	tf->srr0 = 0;
	tf->srr1 = initial_sr;

	savebat(&tf->batreg[0]);		/* Initially use PMON mapping */
}

/*
 *  This function sets the client stack pointer.
 */
void
md_setsp(struct trapframe *tf, register_t sp)
{
	if (tf == NULL)
		cpuinfotab[whatcpu]->fixreg[1] = sp;
	else
		tf->fixreg[1] = sp;
}

/*
 *  This function sets the client sr to the given value.
 */
void
md_setsr(struct trapframe *tf, register_t sr)
{
	if (tf == NULL)
		cpuinfotab[whatcpu]->srr1 = sr;
	else
		tf->srr1 = sr;
}

/*
 *  ator returns either 32 bit or 64 bit conversion
 *  depending on CPU word length. PPC is for now 32.
 */
int32_t
md_ator(vp, p, base)
	register_t *vp;
	char *p;
	int base;
{
	return(atob((void *)vp, p, base));
}

/*
 *  This function sets the SP to the new value given if it's
 *  non zero. The old value of sp is returned.
 */
register_t
md_adjstack(struct trapframe *tf, register_t newsp)
{
	register_t oldsp;

	if (tf == NULL)
		tf = cpuinfotab[whatcpu];
	oldsp = tf->fixreg[1];
	if(newsp != 0) {
		tf->fixreg[1] = newsp;
	}
	return(oldsp);
}

/*
 *  This function returns true if the address range given is
 *  invalid for load, eg will overwrite PMON or its working
 *  areas or other protected memory areas or load into
 *  existing memory.
 */
int
md_valid_load_addr(first_addr, last_addr)
	paddr_t first_addr;
	paddr_t last_addr;
{
/* XXX This should be more clever */
	if((first_addr < (paddr_t)end) || (last_addr > (paddr_t)memorysize)) {
		return(1);
	}
	return(0);
} 

/*
 *  This function sets the arguments to client code so
 *  the client sees the arguments as if it was called
 *  with the arguments.
 */
void
md_setargs(struct trapframe *tf, register_t a1, register_t a2,
		register_t a3, register_t a4)
{
	if (tf == NULL)
		tf = cpuinfotab[whatcpu];
	tf->fixreg[3] = a1;
	tf->fixreg[4] = a2;
	tf->fixreg[5] = a3;
	tf->fixreg[6] = a4;
}

void
md_setentry(struct trapframe *tf, register_t pc)
{
	if (tf == NULL)
		cpuinfotab[whatcpu]->srr0 = pc;
	else
		tf->srr0 = pc;
}

/*
 *  This function sets the client sr to do a trace.
 */
void
md_setlr(struct trapframe *tf, register_t lr)
{
	if (tf == NULL)
		cpuinfotab[whatcpu]->lr = lr;
	else
		tf->lr = lr;
}

#if NMOD_DEBUGGER > 0
/*
 *  This function returns the value of the clients sr.
 */
register_t
md_getsr(struct trapframe *tf)
{
	if (tf == NULL)
		return(cpuinfotab[whatcpu]->srr1);
	else
		return(tf->srr1);
}

/*
 *  This function sets the client sr to do a trace.
 */
void
md_settrace()
{
	cpuinfotab[whatcpu]->srr1 |= PSL_SE;
}

/*
 *  This function returns the PC value that is supposed
 *  to be used at restore to client state. Do not confuse
 *  with the value of the exception PC. (Diff on some arches).
 */
void *
md_getpc(struct trapframe *tf)
{
	if (tf == NULL)
		return((void *)cpuinfotab[whatcpu]->srr0);
	else
		return((void *)tf->srr0);
}

/*
 *  This function is called from exception(). It's purpose
 *  is to decode the exception and return the exception
 *  type to the caller for further processing.
 */
int
md_exc_type(struct trapframe *frame)
{
	switch(frame->exc) {
/* XXX DSI check for DABP */
	case EX_IABP:
		return(EXC_BPT);
	case EX_DECR:
	case EX_EINT:
		return(EXC_INT);
	case EX_TRACE:
		frame->srr1 &= ~(PSL_SE | PSL_BE);
		return(EXC_TRC);
	case EX_PRGM:			/* Check for BP instruction */
		if(load_word(md_get_excpc(frame)) == BPT_CODE) {
			return(EXC_BPT);
		}
		break;
	default:
		break;
	}
	return(EXC_BAD);
}

/*
 *  This function returns the value of the PC reg when an
 *  exception have been take.
 */
void *
md_get_excpc(frame)
	struct trapframe *frame;
{
	return((void *)frame->srr0);
}

const char *
md_getexcname(frame)
	struct trapframe *frame;
{
static char nn[16];
	switch(frame->exc) {
	default:
		sprintf(nn, "%x", frame->exc);
		return(nn);
	case EX_MCHEK:
		return("Machine Check");
	case EX_DSI:
		return("DSI");
	case EX_ISI:
		return("ISI");
	case EX_EINT:
		return("External Interrupt");
	case EX_ALIGN:
		return("Alignment");
	case EX_PRGM:
		return("Program");
	case EX_NOFP:
		return("No FP");
	case EX_DECR:
		return("Decrementer");	
	case EX_SC:
		return("System Call");
	case EX_TRACE:
		return("Trace");
	case EX_FPA:
		return("FP Assistance");
	case EX_PERF:
		return("Performance Monitor");
	case EX_IABP:
		return("Instruction BP");
	case EX_SMI:
		return("System Management");
	}
}

/*
 *  This function returns true if the instruction pointed
 *  by 'p' is a subroutine call instruction.
 */
int
md_is_call(p)
	void *p;
{
	return(0);
}

/*
 *  This function returns true if the instruction pointed
 *  by 'p' is a branch type instruction.
 */
int
md_is_branch(p)
	void *p;
{
	return(0);
}

/*
 *  Returns true and sets the location pointed by vp to the value
 *  of the specified register or false if not recognized. Register
 *  names can be n the form <regno> r<nr> or sp.
 *  Some special registers are also detected.
 */
int
md_getreg(vp, p)
	register_t *vp;
	char *p;
{
	int i;

	if(isdigit(*p)) {
		i = atoi(p);
		if(i >= 0 || i <= 31) {
			*vp = cpuinfotab[whatcpu]->fixreg[i];
			return(1);
		}
	}
	else if((*p == 'r' || *p == 'R') && isdigit(*(p+1))) {
		i = atoi(p+1);
		if(i >= 0 || i <= 31) {
			*vp = cpuinfotab[whatcpu]->fixreg[i];
			return(1);
		}
	}
	else if(strcmp(p, "cpc") == 0) {
		*vp = cpuinfotab[whatcpu]->srr0;
		return(1);
	}
	else if(strcmp(p, "sr") == 0) {
		*vp = cpuinfotab[whatcpu]->srr1;
		return(1);
	}
	else {
		return(0);
	}
}

int
md_getregaddr(r, p)
	register_t **r;
	char *p;
{
	int i;

	if(isdigit(*p)) {
		i = atoi(p);
		if(i >= 0 || i <= 31) {
			*r = (register_t *)&cpuinfotab[whatcpu]->fixreg[i];
			return(1);
		}
	}
	else if((*p == 'r' || *p == 'R') && isdigit(*(p+1))) {
		i = atoi(p+1);
		if(i >= 0 || i <= 31) {
			*r = (register_t *)&cpuinfotab[whatcpu]->fixreg[i];
			return(1);
		}
	}
	else if((*p == 'f' || *p == 'F') && isdigit(*(p+1))) {
		i = atoi(p+1);
		if(i >= 0 || i <= 31) {
			*r = (register_t *)&cpuinfotab[whatcpu]->floatreg[i+i];
			return(1);
		}
	}
	else if(strcmp(p, "cpc") == 0) {
		*r = (register_t *)&cpuinfotab[whatcpu]->srr0;
		return(1);
	}
	else if(strcmp(p, "sr") == 0) {
		*r = (register_t *)&cpuinfotab[whatcpu]->srr1;
		return(1);
	}
	else if(strcmp(p, "fsr") == 0) {
		*r = (register_t *)&cpuinfotab[whatcpu]->fsr[64];
		return(1);
	}
	else if(strcmp(p, "lr") == 0) {
		*r = (register_t *)&cpuinfotab[whatcpu]->lr;
		return(1);
	}
	else if(strcmp(p, "ctr") == 0) {
		*r = (register_t *)&cpuinfotab[whatcpu]->ctr;
		return(1);
	}
	else {
		return(0);
	}
	return(0);
}

int
md_disp_as_reg(r, c, w)
	register_t *r;
	char *c;
	int *w;
{
	return(0);
}

const Optdesc md_r_opts[] = {
	{"*", "display all registers"},
	{"r*", "display all general registers"},
	{"f*", "display all fp registers"},
	{"reg value", "set specified register"},
	{0}
};

static void
dsp_rregs(void)
{
	int regno;

	for(regno = 0; regno < 32; regno++) {
		if((regno % 8) == 0) {
			printf("\nr%02d-%02d ", regno, regno + 7);
		}
		printf(" %08x", cpuinfotab[whatcpu]->fixreg[regno]);
	}
	printf("\n");
}

static void
dsp_fregs(void)
{
	int regno;
	u_int32_t *p;

	for(regno = 0; regno < 32; regno++) {
		if((regno % 4) == 0) {
			printf("\nf%02d-%02d ", regno, regno + 3);
		}
		p = (void *)&cpuinfotab[whatcpu]->floatreg[regno];
		printf(" %08x%08x", *p, *p+1);
	}
	printf("\n");
	printf("fsr = %08x%08x\n", cpuinfotab[whatcpu]->fsr[0],
				   cpuinfotab[whatcpu]->fsr[1]);
}

/*
 *  Machine dependent register display/modify code.
 */
int
md_registers(ac, av)
	int ac;
	char *av[];
{
	register_t *rp;
	register_t rn;

	ioctl(STDIN, CBREAK, NULL);

	switch(ac) {
	case 1:	/* No args, display general registers */
		dsp_rregs();
		break;

	case 2:
		if(strcmp(av[1], "*") == 0) {
			dsp_rregs();
			dsp_fregs();
			printf("cpc = %08x, lr = %08x, ctr = %08x, sr = %08x\n",
			  cpuinfotab[whatcpu]->srr0, cpuinfotab[whatcpu]->lr,
			  cpuinfotab[whatcpu]->ctr, cpuinfotab[whatcpu]->srr1);
		}
		else if(strcmp(av[1], "r*") == 0) {
			dsp_rregs();
		}
		else if(strcmp(av[1], "f*") == 0) {
			dsp_fregs();
		}
		else {
			if(md_getregaddr(&rp, av[1])) {
				printf("%s = %08x\n", av[1], *rp);
			}
			else {
				printf("%s: unkown.\n", av[1]);
				return(-1);
			}
		}
		break;

	case 3:
		if(md_getregaddr(&rp, av[1]) == 0) {
			printf("%s: unkown.\n", av[1]);
			return(-1);
		}
		if(!get_rsa_reg(&rn, av[2])) {
			return(-1);
		}
		*rp = rn;
		break;
	}

	return(0);
}

#endif /* NMOD_DEBUGGER */
