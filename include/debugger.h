/*	$Id: debugger.h,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */

/*
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
#ifndef __DEBUGGER_H__
#define __DEBUGGER_H__

#ifndef STOPMAX
#define STOPMAX 10
#endif /* STOPMAX */

#ifndef PCHISTSZ
#define PCHISTSZ  200
#endif /* PCHISTSZ */

extern int             trace_mode;
extern unsigned long   trace_count;
extern int             trace_verbose;
extern int             trace_invalid;
extern int             trace_over;
extern int             trace_bflag;
extern int             trace_cflag;
extern Stopentry       stopval[STOPMAX];

void	exception __P((struct trapframe *));

int	setTrcbp __P((void *, int));
int	is_break_point __P((void *));
int	is_validpc __P((void *));
void	addpchist __P((void *));
void	clrpchist __P((void));
void	store_trace_breakpoint __P((void));
void	remove_trace_breakpoint __P((void));
void	store_breakpoint __P((void));
void	sstep __P((void));
void	compute_validpc __P((void));
void	pmon_stop __P((char *));
void	rm_bpts __P((void));
void	dspbpts __P((void));
int	goclient __P((void));
int	addstop __P((u_int32_t, u_int32_t, char *, char));

#endif /* __DEBUGGER_H__ */
