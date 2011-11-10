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

extern struct trapframe DBGREG;
extern u_int32_t FPREG[];
extern int memorysize;
extern u_int8_t end[];


/*
 *  Debug helpers.
 */
void
md_setpc(pc)
	register_t pc;
{
	DBGREG.srr0 = pc;
}

/*
 *  This function clears all client registers for launch.
 */
void
md_clreg()
{
	struct trapframe *tf;
	int i;

	tf = &DBGREG;
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
md_setsp(sp)
	register_t sp;
{
	DBGREG.fixreg[1] = sp;
}

/*
 *  This function sets the client sr to the given value.
 */
void
md_setsr(sr)
	register_t sr;
{
	DBGREG.srr1 = sr;
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
md_adjstack(newsp)
	register_t newsp;
{
	register_t oldsp;

	oldsp = DBGREG.fixreg[1];
	if(newsp != 0) {
		DBGREG.fixreg[1] = newsp;
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
md_setargs(a1, a2, a3, a4)
	register_t a1;
	register_t a2;
	register_t a3;
	register_t a4;
{
	struct trapframe *tf;

	tf = &DBGREG;
	tf->fixreg[3] = a1;
	tf->fixreg[4] = a2;
	tf->fixreg[5] = a3;
	tf->fixreg[6] = a4;
}

void
md_setentry(pc)
	register_t pc;
{
	DBGREG.srr0 = pc;
}

/*
 *  This function sets the client sr to do a trace.
 */
void
md_setlr(lr)
	register_t lr;
{
	DBGREG.lr = lr;
}

#if NMOD_DEBUGGER > 0
/*
 *  This function returns the value of the clients sr.
 */
register_t
md_getsr()
{
	return(DBGREG.srr1);
}

/*
 *  This function sets the client sr to do a trace.
 */
void
md_settrace()
{
	DBGREG.srr1 |= PSL_SE;
}

/*
 *  This function returns the PC value that is supposed
 *  to be used at restore to client state. Do not confuse
 *  with the value of the exception PC. (Diff on some arches).
 */
void *
md_getpc()
{
	return((void *)DBGREG.srr0);
}

/*
 *  This function is called from exception(). It's purpose
 *  is to decode the exception and return the exception
 *  type to the caller for further processing.
 */
int
md_exc_type(frame)
	struct trapframe *frame;
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
	switch(frame->exc) {
	default:
		return("????");
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
			*vp = DBGREG.fixreg[i];
			return(1);
		}
	}
	else if((*p == 'r' || *p == 'R') && isdigit(*(p+1))) {
		i = atoi(p+1);
		if(i >= 0 || i <= 31) {
			*vp = DBGREG.fixreg[i];
			return(1);
		}
	}
	else if(strcmp(p, "cpc") == 0) {
		*vp = DBGREG.srr0;
		return(1);
	}
	else if(strcmp(p, "sr") == 0) {
		*vp = DBGREG.srr1;
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
			*r = (register_t *)&DBGREG.fixreg[i];
			return(1);
		}
	}
	else if((*p == 'r' || *p == 'R') && isdigit(*(p+1))) {
		i = atoi(p+1);
		if(i >= 0 || i <= 31) {
			*r = (register_t *)&DBGREG.fixreg[i];
			return(1);
		}
	}
	else if((*p == 'f' || *p == 'F') && isdigit(*(p+1))) {
		i = atoi(p+1);
		if(i >= 0 || i <= 31) {
			*r = (register_t *)&FPREG[i+i];
			return(1);
		}
	}
	else if(strcmp(p, "cpc") == 0) {
		*r = (register_t *)&DBGREG.srr0;
		return(1);
	}
	else if(strcmp(p, "sr") == 0) {
		*r = (register_t *)&DBGREG.srr1;
		return(1);
	}
	else if(strcmp(p, "fsr") == 0) {
		*r = (register_t *)&FPREG[64];
		return(1);
	}
	else if(strcmp(p, "lr") == 0) {
		*r = (register_t *)&DBGREG.lr;
		return(1);
	}
	else if(strcmp(p, "ctr") == 0) {
		*r = (register_t *)&DBGREG.ctr;
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
		printf(" %08x", DBGREG.fixreg[regno]);
	}
	printf("\n");
}

static void
dsp_fregs(void)
{
	int regno;

	for(regno = 0; regno < 32; regno++) {
		if((regno % 4) == 0) {
			printf("\nf%02d-%02d ", regno, regno + 3);
		}
		printf(" %08x%08x", FPREG[regno*2], FPREG[regno*2+1]);
	}
	printf("\n");
	printf("fsr = %08x%08x\n", FPREG[64], FPREG[65]);
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
			    DBGREG.srr0, DBGREG.lr, DBGREG.ctr, DBGREG.srr1);
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
