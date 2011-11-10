/*	$Id: kern_syscall.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/syscallargs.h>

#include <machine/stdarg.h>

int errno;			/* has to be declared somewhere */
/*
 *  Some stuff taken from signal.h.
 */
static __inline int sigaddset(sigset_t *set, int signo) {

        if (signo <= 0 || signo >= _NSIG) {
                errno = 22;                     /* EINVAL */
                return -1;
        }
        *set |= (1 << ((signo)-1));             /* sigmask(signo) */
        return (0);
}
static __inline int sigdelset(sigset_t *set, int signo) {
        extern int errno;

        if (signo <= 0 || signo >= _NSIG) {
                errno = 22;                     /* EINVAL */
                return -1;
        }
        *set &= ~(1 << ((signo)-1));            /* sigmask(signo) */
        return (0);
}

static __inline int sigismember(const sigset_t *set, int signo) {
        extern int errno;

        if (signo <= 0 || signo >= _NSIG) {
                errno = 22;                     /* EINVAL */
                return -1;
        }
        return ((*set & (1 << ((signo)-1))) != 0);
}

#define sigemptyset(set)        (*(set) = 0, 0)


#define MAXARGS	6


static int gensyscall __P((int (*) __P((struct proc *, void *, register_t *)), int, long, va_list));

static int
gensyscall (int (*func) __P((struct proc *, void *, register_t *)), int nargs, long a1, va_list ap)
{
	extern int errno;
	struct args {register_t a[MAXARGS];} ua;
	struct proc *p = curproc;
	register_t rval[2];
	int error, sig, i;

	if (p->p_stat != SNOTKERN)
		panic ("nested syscall");

	if (nargs > 0) {
		ua.a[0] = a1;
	}
	for (i = 1; i < nargs; i++) {
		ua.a[i] = va_arg (ap, long);
	}

	while (1) {
		p->p_stat = SRUN;
		rval[0] = 0;
		error = (*func) (p, &ua, rval);
		while ((sig = CURSIG (p)) != 0)	/* handle signals here */
			psig (sig);
		p->p_stat = SNOTKERN;
		if (error != ERESTART) {
			if (error) {
				errno = error;
				return -1;
		    }
		    return rval[0];
		}
	}
}

#define syscall(pub, pri, nargs) \
int pub __P((long, ...)); \
int pub (long a1, ...) \
{ \
    int res; \
    va_list ap; \
    va_start (ap, a1); \
    res = gensyscall (SYSCALL(pri), nargs, a1, ap); \
    va_end (ap); \
    return(res); \
}

syscall(soc_read, read, 3)
syscall(soc_write, write, 3)
syscall(soc_close, close, 1)
syscall(recvmsg, recvmsg, 3)
syscall(sendmsg, sendmsg, 3)
syscall(recvfrom, recvfrom, 6)
syscall(accept, accept, 3)
syscall(getpeername, getpeername, 3)
syscall(getsockname, getsockname, 3)
syscall(soc_dup, dup,  2)
syscall(soc_ioctl, ioctl, 3)
#if 0
syscall(getdtablesize, getdtablesize, 0)
#endif
syscall(soc_dup2, dup2, 2)
syscall(soc_fcntl, fcntl, 3)
syscall(select, select, 5)
syscall(socket, socket, 3)
syscall(connect, connect, 3)
syscall(bind, bind,  3)
syscall(setsockopt, setsockopt,  5)
syscall(listen, listen,  2)
syscall(getsockopt, getsockopt,  5)
syscall(readv, readv,  3)
syscall(writev, writev,  3)
syscall(sendto, sendto,  6)
syscall(shutdown, shutdown,  2)
syscall(sigaction, sigaction, 3)
syscall(kernsigprocmask, sigprocmask, 2)
syscall(sigpending, sigpending, 0)
syscall(sigsuspend, sigsuspend, 1)
syscall(gettimeofday, gettimeofday, 2)
syscall(getitimer, getitimer, 2)
syscall(setitimer, setitimer, 3)

#ifdef notyet
/*
 * TBD: user callable socket system call
 */
int
soc_syscall ()
{
    extern int errno;
    errno = EINVAL;
    return -1;
}

#endif

/* 
 * user callable exit (distinct from prom's exit routine) 
 */
void soc_exit __P((int));
void
soc_exit (rv)
{
    exit1 (curproc, rv & 0xff);
}

/*
 * Dummy system calls
 */

int getuid __P((void));
int getuid()	{return 0;}
int geteuid __P((void));
int geteuid()	{return 0;}
int getegid __P((void));
int getegid()	{return 0;}
int getgid __P((void));
int getgid()	{return 0;}

int getpid __P((void));
int getpid()	
{
    return curproc->p_pid;
}

int getpgrp __P((int));
int getpgrp(pid)
{
    /* for us pgid == pid */
    return curproc->p_pid;
}

int gethostid __P((void));
int gethostid()
{
extern char *getenv __P((char *));
extern in_addr_t inet_addr __P((const char *));
    char *netaddr;
    int id;

    netaddr = getenv ("netaddr");
    if (netaddr && (id = inet_addr (netaddr)) != -1)
      return id;
    return 0;
}


int gethostname __P((char *buf, int n));
int
gethostname (char *buf, int n)
{
extern char *getenv __P((char *));
    char *hostname;
    extern int errno;

    hostname = getenv ("hostname");
    if (!hostname)
      hostname = "pmon";

    if (n < strlen (hostname) + 1) {
	errno = EINVAL;
	return -1;
    }
    strcpy (buf, hostname);
    return 0;
}

/*
 * User-level signal handling (4.3bsd emulation on top of POSIX)
 */
int sigvec __P((int, struct sigvec *, struct sigvec *));
int
sigvec(signo, sv, osv)
	int signo;
	struct sigvec *sv, *osv;
{
	int ret;

	if (sv)
		sv->sv_flags ^= SV_INTERRUPT;	/* !SA_INTERRUPT */
	ret = sigaction(signo, (struct sigaction *)sv, (struct sigaction *)osv);
	if (ret == 0 && osv)
		osv->sv_flags ^= SV_INTERRUPT;	/* !SA_INTERRUPT */
	return (ret);
}

/* 
 * The real sigprocmask system call takes only two args, and returns
 * the old mask.  This cover function munges the arguments appropriately.
 */
int sigprocmask __P((int how, const sigset_t *set, sigset_t *oset));
int
sigprocmask (int how, const sigset_t *set, sigset_t *oset)
{
    sigset_t new, old;
    int oerrno;

    if (!set) {
	how = SIG_BLOCK;
	new = 0;
    } else {
	new = *set;
    }

    oerrno = errno; errno = 0;
    old = kernsigprocmask (how, new);
    if (old == (sigset_t)-1 && errno)
      return -1;
    errno = oerrno;

    if (oset)
      *oset = old;
    return 0;
}

int sigsetmask __P((int));
int
sigsetmask(mask)
	int mask;
{
	int omask, n;

	n = sigprocmask(SIG_SETMASK, (sigset_t *) &mask, (sigset_t *) &omask);
	if (n)
		return (n);
	return (omask);
}

int sigblock __P((int));
int
sigblock(mask)
	int mask;
{
	int omask, n;

	n = sigprocmask(SIG_BLOCK, (sigset_t *) &mask, (sigset_t *) &omask);
	if (n)
		return (n);
	return (omask);
}

int sigpause __P((int));
int
sigpause(mask)
	int mask;
{
	return (sigsuspend((long)&mask));
}

sigset_t _sigintr;		/* shared with siginterrupt */

sig_t
signal(s, a)
	int s;
	sig_t a;
{
	struct sigaction sa, osa;

	sa.sa_handler = a;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (!sigismember(&_sigintr, s))
		sa.sa_flags |= SA_RESTART;
	if (sigaction(s, &sa, &osa) < 0)
		return (BADSIG);
	return (osa.sa_handler);
}

/*
 * Set signal state to prevent restart of system calls
 * after an instance of the indicated signal.
 */
int     siginterrupt __P((int, int));
int
siginterrupt(sig, flag)
	int sig, flag;
{
	extern sigset_t _sigintr;
	struct sigaction sa;
	int ret;

	if ((ret = sigaction(sig, (struct sigaction *)0, &sa)) < 0)
		return (ret);
	if (flag) {
		sigaddset(&_sigintr, sig);
		sa.sa_flags &= ~SA_RESTART;
	} else {
		sigdelset(&_sigintr, sig);
		sa.sa_flags |= SA_RESTART;
	}
	return (sigaction(sig, &sa, (struct sigaction *)0));
}

