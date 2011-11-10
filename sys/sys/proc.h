/* $Id: proc.h,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */
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

#ifndef _PROC_H_
#define _PROC_H_

#include <sys/ucred.h>

struct proc {
    pid_t	p_pid;
    void	*p_ucred;
    void	*p_cred;
    struct	filedesc *p_fd;
    struct	pstats *p_stats;	/* accounting/statistics (PROC ONLY) */
    u_short 	p_flag;
    u_short 	p_acflag;
    int 	p_stat;
    int 	p_pri;
    caddr_t	p_wchan;
    char	*p_wmesg;
    char	*p_comm;	/* process name */
    struct	itimerval p_realtimer;	/* alarm timer */
    int		p_siglist;		/* set of pending signals */
    sigset_t	p_sigmask;	/* current signal mask */
    sigset_t	p_sigignore;	/* signals being ignored */
    sigset_t	p_sigcatch;	/* signals being caught by user */
    struct sigacts *p_sigacts;
    label_t	p_exitbuf;
};

/* p_flag */
#define P_SELECT	0x1
#define STIMO	0x2
#define SSINTR	0x4
#define	P_TRACED	0x0000	/* No misstakes */

/* p_stat */
#define SZOMB	0		/* zombie process */
#define SSLEEP	1		/* process is sleeping */
#define SNOTKERN 2		/* running outside net kernel */
#define SRUN	3		/* running inside net kernel */

void init_proc __P((void));
void wakeup __P((void *));
int  tsleep __P((void *chan, int pri, char *wmesg, int timo));
struct proc *curproc;
struct proc *pfind __P((int));
extern struct proc proc[];
int nprocs;
int maxproc;

#ifdef __STDC__
#define SYSCALL(name) sys_##name
#else
#define SYSCALL(name) sys_/**/name
#endif

#define crhold(credp)
#define crfree(credp)

#endif /* !_PROC_H_ */
