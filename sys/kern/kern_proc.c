/* $Id: kern_proc.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/errno.h>
#include <sys/syslog.h>

#ifndef NPROC
#define NPROC 3
#endif

static struct sigacts sa[NPROC];
static struct pstats stats[NPROC];
static struct filedesc0 fd0;

struct proc *curproc;
struct proc proc[NPROC];
int maxproc;
int nprocs;
int nextpid;

/*
 * Spawn a new "process", which runs til completion.
 * Callable directly by user.
 */
int
spawn (name, func, argc, argv)
	char *name;
	int (*func) __P((int, char **));
	int argc;
	char **argv;
{
	struct proc *fatherproc = curproc;
	struct proc *sonproc;
	extern int errno, nextpid;
	int sig, rv;

	if (nprocs >= maxproc) {
		tablefull ("proc");
		errno = EAGAIN;
		return (-1);
	}

	sonproc = &proc[nprocs++];
	if (sonproc->p_stat != SZOMB) {
		panic ("spawn");
	}
    
	sonproc->p_pid = ++nextpid;
	sonproc->p_comm = name;

	/* clear fields */
	sonproc->p_wchan = 0;
	sonproc->p_wmesg = 0;
	sonproc->p_siglist = 0;

	/* copy signal state, and update */
	sonproc->p_sigmask = fatherproc->p_sigmask;
	sonproc->p_sigignore = fatherproc->p_sigignore;
	sonproc->p_sigcatch = fatherproc->p_sigcatch;
	*sonproc->p_sigacts = *fatherproc->p_sigacts;
	execsigs (sonproc);

	/* clear real-time timer */
	timerclear (&sonproc->p_realtimer.it_interval);
	timerclear (&sonproc->p_realtimer.it_value);

	/* copy virtual interval timers */
	bcopy (fatherproc->p_stats, sonproc->p_stats, sizeof(struct pstats));

	/* copy file descriptors */
	sonproc->p_fd = fdcopy (fatherproc);

	if ((rv = setjmp (&sonproc->p_exitbuf)) != 0) {
		/* new process has exited */
		curproc = fatherproc;
		/* handle newly pending signals */
		while ((sig = CURSIG (fatherproc)) != 0) {
			psig (sig);
		}
		fatherproc->p_stat = SNOTKERN;
		spl0 ();
		return rv & 0xffff;
	}

	/* new process is active */
	fatherproc->p_stat = SRUN;		/* pretend old proc is locked in kernel */
	sonproc->p_stat = SNOTKERN;	/* new proc is not in kernel */
	curproc = sonproc;		/* new proc becomes current proc */

	/* make sure resolver state is initialised */
	_res_reset ();

	exit1 (sonproc, (*func)(argc, argv) & 0xff);
	return(0); /* will not likely happen... */
}


struct proc *
pfind (pid)
{
	struct proc *p;
	for (p = proc; p < &proc[maxproc]; p++)
	if (p->p_stat != SZOMB && p->p_pid == pid) {
		return (p);
	}
	return (0);
}


/*
 * Internal "process" exit handler
 */
void
exit1 (p, rv)
	struct proc *p;
{
	int s;

	if (p->p_stat == SZOMB) {
		panic ("zombie exit");
	}

	s = splhigh ();
	p->p_sigignore = ~0;
	p->p_siglist = 0;
	untimeout(realitexpire, (caddr_t)p);

	fdfree (p);
	--nprocs;
	(void) splx (s);

	p->p_stat = SZOMB;
	if (p == curproc) {
		curproc = 0;
	}

	if ((rv >> 8) != SIGKILL) {	/* XXX see procreset() below */
		if (p == &proc[0]) {
			panic ("prom process died");
		}
		else {
			longjmp (&p->p_exitbuf, rv | 0x10000 /* force non-zero */);
		}
		/* neither return */
	}
}


void
sigexit (p, sig)
	struct proc *p;
	int sig;
{
	if (sig != SIGINT) {
		log (LOG_ERR, "process %d (%s) dying from signal %d\n",
			p->p_pid, p->p_comm ? p->p_comm : "noname", sig & 0xff);
	}
	exit1 (p, sig << 8);
}


void
init_proc ()
{
    struct proc *p;
    int i;

	maxproc = NPROC;
	nprocs = 0;
	nextpid = 0;
	curproc = 0;

	bzero (&fd0, sizeof(fd0));
	fd0.fd_fd.fd_ofiles = fd0.fd_dfiles;
	fd0.fd_fd.fd_ofileflags = fd0.fd_dfileflags;
	fd0.fd_fd.fd_nfiles = NDFILE;
	fd0.fd_fd.fd_refcnt = 100;	/* never free */

	bzero (proc, sizeof (proc));
	bzero (sa, sizeof (sa));
	bzero (stats, sizeof (stats));
	for (p = proc, i = 0; p < &proc[NPROC]; p++, i++) {
		p->p_sigacts = &sa[i];
		p->p_stats = &stats[i];
	}

	p = &proc[nprocs++];
	p->p_stat = SNOTKERN;
	p->p_fd = &fd0.fd_fd;
	p->p_comm = "pmon";
	p->p_pid = ++nextpid;
	    
	/* initial signal state */
	siginit (p);

	curproc = p;
}


#if notyet
void
checkstack ()
{
    if (stackalert < RED && stack[RED_LIMIT] != STACK_MAGIC) {
	stackalert = RED;
	log (LOG_CRIT, "process \"%s\": stack overflow\n",
	     curproc ? curproc->p_comm : "???");
	if (curproc)
	  psignal (curproc, SIGSEGV);
	else
	  panic ("stack overflow");
    } else if (stackalert < YELLOW && stack[YELLOW_LIMIT] != STACK_MAGIC) {
	stackalert = YELLOW;
	log (LOG_WARNING, "process \"%s\": stack yellow alert\n",
	     curproc ? curproc->p_comm : "???");
    }
}
#endif


#if notyet
/*
 * Clean up on promexit()
 */
void
procreset ()
{
    struct proc *p;

    /* try to clean up processes and hanging connections */
    for (p = &proc[NPROC-1]; p >= proc; p--)
      if (p->p_stat != SZOMB)
	/* XXX see exit1() for special handling of SIGKILL */
	psignal (curproc = p, SIGKILL);

    if (nprocs > 0)
      panic ("procreset: couldn't kill all processes");

    init_proc ();
}
#endif

int
suser(struct ucred *cred, u_short *acflag)
{
	return(0);
}
