/* $Id: kern_sig.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2000 Opsycon AB  (www.opsycon.se)
 * Copyright (c) 2000 Rtmx, Inc   (www.rtmx.com)
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

/*
 *  Based on original work from Algorithmics UK released as free code.
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/map.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/syslog.h>

#define SIGPROP
#include <sys/signal.h>
#include <sys/signalvar.h>

#include <sys/syscallargs.h>

void psig (int);

/* ARGSUSED */
int
SYSCALL(sigaction)(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	register struct sys_sigaction_args /* {
		int	signum;
		struct	sigaction *nsa;
		struct	sigaction *osa;
	} */ *uap = v;
	struct sigaction vec;
	register struct sigaction *sa;
	register struct sigacts *ps = p->p_sigacts;
	register int sig;
	int bit, error;

	sig = SCARG(uap, signum);
	if (sig <= 0 || sig >= NSIG || sig == SIGKILL || sig == SIGSTOP)
		return (EINVAL);
	sa = &vec;
	if (SCARG(uap, osa)) {
		sa->sa_handler = ps->ps_sigact[sig];
		sa->sa_mask = ps->ps_catchmask[sig];
		bit = sigmask(sig);
		sa->sa_flags = 0;
#ifdef NOTUSED_BY_PMON
		if ((ps->ps_sigonstack & bit) != 0)
			sa->sa_flags |= SA_ONSTACK;
		if (p->p_flag & SNOCLDSTOP)
			sa->sa_flags |= SA_NOCLDSTOP;
#endif
		if ((ps->ps_sigintr & bit) == 0)
			sa->sa_flags |= SA_RESTART;
		if ((error = copyout((caddr_t)sa, (caddr_t)(SCARG(uap, osa)),
		    sizeof (vec))) != 0) {
			return (error);
		}
	}
	if (SCARG(uap, nsa)) {
		if ((error = copyin((caddr_t)(SCARG(uap, nsa)), (caddr_t)sa,
		    sizeof (vec))) != 0) {
			return (error);
		}
		setsigvec(p, sig, sa);
	}
	return (0);
}

void
setsigvec(p, sig, sa)
	register struct proc *p;
	int sig;
	register struct sigaction *sa;
{
	struct sigacts *ps = p->p_sigacts;
	int bit;
	int s;

	bit = sigmask(sig);
	/*
	 * Change setting atomically.
	 */
	s = splhigh();
	ps->ps_sigact[sig] = sa->sa_handler;
	ps->ps_catchmask[sig] = sa->sa_mask &~ sigcantmask;
	if ((sa->sa_flags & SA_RESTART) == 0)
		ps->ps_sigintr |= bit;
	else
		ps->ps_sigintr &= ~bit;
#ifdef NOTUSED_BY_PMON
	if (sa->sa_flags & SA_ONSTACK)
		ps->ps_sigonstack |= bit;
	else
		ps->ps_sigonstack &= ~bit;
	if (sig == SIGCHLD) {
		if (sa->sa_flags & SA_NOCLDSTOP)
			p->p_flag |= SNOCLDSTOP;
		else
			p->p_flag &= ~SNOCLDSTOP;
	}
#endif
	/*
	 * Set bit in p_sigignore for signals that are set to SIG_IGN,
	 * and for signals set to SIG_DFL where the default is to ignore.
	 * However, don't put SIGCONT in p_sigignore,
	 * as we have to restart the process.
	 */
	if (sa->sa_handler == SIG_IGN ||
	    (sigprop[sig] & SA_IGNORE && sa->sa_handler == SIG_DFL)) {
		p->p_siglist &= ~bit;		/* never to be seen again */
		if (sig != SIGCONT)
			p->p_sigignore |= bit;	/* easier in psignal */
		p->p_sigcatch &= ~bit;
	} else {
		p->p_sigignore &= ~bit;
		if (sa->sa_handler == SIG_DFL)
			p->p_sigcatch &= ~bit;
		else
			p->p_sigcatch |= bit;
	}
	splx(s);
}

/*
 * Initialize signal state for process 0;
 * set to ignore signals that are ignored by default.
 */
void
siginit(p)
	struct proc *p;
{
	register int i;

	for (i = 0; i < NSIG; i++)
		if (sigprop[i] & SA_IGNORE && i != SIGCONT)
			p->p_sigignore |= sigmask(i);
}

/*
 * Reset signals for an exec of the specified process.
 */
void
execsigs(p)
	register struct proc *p;
{
	register struct sigacts *ps = p->p_sigacts;
	register int nc, mask;

	/*
	 * Reset caught signals.  Held signals remain held
	 * through p_sigmask (unless they were caught,
	 * and are now ignored by default).
	 */
	while (p->p_sigcatch) {
		nc = ffs((long)p->p_sigcatch);
		mask = sigmask(nc);
		p->p_sigcatch &= ~mask;
		if (sigprop[nc] & SA_IGNORE) {
			if (nc != SIGCONT)
				p->p_sigignore |= mask;
			p->p_siglist &= ~mask;
		}
		ps->ps_sigact[nc] = SIG_DFL;
	}
#ifdef NOTUSED_BY_PMON
	/*
	 * Reset stack state to the user stack.
	 * Clear set of signals caught on the signal stack.
	 */
	ps->ps_onstack = 0;
	ps->ps_sigsp = 0;
	ps->ps_sigonstack = 0;
#endif
}

/*
 * Manipulate signal mask.
 * Note that we receive new mask, not pointer,
 * and return old mask as return value;
 * the library stub does the rest.
 */
int
SYSCALL(sigprocmask)(p, v, retval)
	register struct proc *p;
	void *v;
	register_t *retval;
{
	int s;
        struct sys_sigprocmask_args /* {
		int	how;
		sigset_t mask;
	} */ *uap = v;

	int error = 0;

	*retval = p->p_sigmask;
	s = splhigh();

	switch (SCARG(uap, how)) {
	case SIG_BLOCK:
		p->p_sigmask |= SCARG(uap, mask) &~ sigcantmask;
		break;

	case SIG_UNBLOCK:
		p->p_sigmask &= ~SCARG(uap, mask);
		break;

	case SIG_SETMASK:
		p->p_sigmask = SCARG(uap, mask) &~ sigcantmask;
		break;
	
	default:
		error = EINVAL;
		break;
	}
	splx(s);
	return (error);
}

/* ARGSUSED */
int
SYSCALL(sigpending)(p, uap, retval)
	struct proc *p;
	void *uap;
	register_t *retval;
{

	*retval = p->p_siglist;
	return (0);
}

/*
 * Suspend process until signal, providing mask to be set
 * in the meantime.  Note nonstandard calling convention:
 * libc stub passes mask, not pointer, to save a copyin.
 */
/* ARGSUSED */
int
SYSCALL(sigsuspend)(p, v, retval)
	register struct proc *p;
	void *v;
	register_t *retval;
{
        register struct sys_sigsuspend_args /* {
		sigset_t mask;
	} */ *uap = v;
	register struct sigacts *ps = p->p_sigacts;

	/*
	 * When returning from sigpause, we want
	 * the old mask to be restored after the
	 * signal handler has finished.  Thus, we
	 * save it here and mark the proc structure
	 * to indicate this (should be in sigacts).
	 */
	ps->ps_oldmask = p->p_sigmask;
	ps->ps_flags |= SAS_OLDMASK;
	p->p_sigmask = SCARG(uap, mask) &~ sigcantmask;
	(void) tsleep((caddr_t) ps, PPAUSE|PCATCH, "pause", 0);
	/* always return EINTR rather than ERESTART... */
	return (EINTR);
}


#if 0 /* XXX which is correct ?? */
void gsignal (pgid, sig)
#else
void gsignal (label_t *p, int sig)
#endif
{
    /* each process is its own group */
    if (curproc)
	psignal (curproc, sig);
}


void psignal (p, sig)
    struct proc *p;
    int sig;
{
    int mask, prop;
    sig_t action;
    int s;

    if ((unsigned)sig >= NSIG || sig == 0)
      panic("psignal sig");
    mask = sigmask(sig);
    prop = sigprop[sig];

    if (p->p_sigignore & mask)
      return;
    if (p->p_sigmask & mask)
      action = SIG_HOLD;
    else if (p->p_sigcatch & mask)
      action = SIG_CATCH;
    else
      action = SIG_DFL;

    p->p_siglist |= mask;
    if (action == SIG_HOLD)
      return;

    s = splhigh ();
    switch (p->p_stat) {
    case SSLEEP:
	if (p->p_flag & SSINTR)
	  /* sleeping interruptibly, wake up */
	  setrunnable (p);
	break;
    case SNOTKERN:
	/* exception? while not in syscall */
#ifdef NOTUSED_BY_PMON
	if (p == curproc && prop & SA_CORE)
#else
	if (p == curproc)
#endif
	  psig (sig);
	break;
    default:
	/* inside net kernel, will be picked up on soon */
	break;
    }
#ifndef NOTUSED_BY_PMON
    /* procreset needs handling NOW */
    if (p == curproc && sig == SIGKILL)
      psig (sig);
#endif
    (void) splx (s);
}


int
issignal(p)
    struct proc *p;
{
    int sig, mask, prop;

    for (;;) {
	mask = p->p_siglist &~ p->p_sigmask;
	if (mask == 0) {
	  sig = 0;
	  break;
	}
	sig = ffs((long)mask);
	mask = sigmask(sig);
	prop = sigprop[sig];
	if (mask & p->p_sigignore) {
	    p->p_siglist &= ~mask;
	    continue;
	}
	if (p->p_sigacts->ps_sigact[sig] != SIG_IGN) {
	    if (!(prop & SA_IGNORE))
	      break;
	}
	p->p_siglist &= ~mask;
    }
    return(sig);
}

void
psig (int sig)
{
    struct proc *p = curproc;
    struct sigacts *ps = p->p_sigacts;
    sig_t action;
    int mask, returnmask;
    int ostat;

    mask = sigmask (sig);
    p->p_siglist &= ~mask;
    action = ps->ps_sigact[sig];
    if (action == SIG_DFL) {
	sigexit (p, sig);
    } else {
	int s = splhigh ();
#ifdef notyet
	checkstack ();
#endif
	if (ps->ps_flags & SAS_OLDMASK) {
	    returnmask = ps->ps_oldmask;
	    ps->ps_flags &= ~SAS_OLDMASK;
	} else
	  returnmask = p->p_sigmask;
	p->p_sigmask |= ps->ps_catchmask[sig] | mask;
	(void) spl0 ();			/* XXX calling catcher is spl0 OK */
	ostat = p->p_stat;
	p->p_stat = SNOTKERN;
	(*action) (sig);
	p->p_stat = ostat;
	(void) splhigh ();
	p->p_sigmask = returnmask;
#ifdef notyet
	checkstack ();
#endif
	(void) splx (s);
    }
}

void
csignal(pgid, signum, uid, euid)
	pid_t pgid;
	int signum;
	uid_t uid, euid;
{
	if(pgid == 0) {
		return;
	}
	printf("csignal pgid %d\n", pgid);
}
