/* $Id: kern_synch.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/map.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/signal.h>
#include <sys/signalvar.h>

void idle __P((void));
void endtsleep __P((void *));

int
tsleep (chan, pri, mesg, timo)
	void *chan;
	int pri, timo;
	char *mesg;
{
	struct proc *p = curproc;
	int catch = pri & PCATCH;
	int sig = 0;
	int s = splhigh ();

	if (!chan) {
extern void _pmon_break(void);
		printf("chan = 0!\n");
		_pmon_break();
		panic ("tsleep !chan");
	}
	if (p->p_wchan || !chan) {
		panic ("tsleep");
	}
	p->p_wmesg = mesg;
	p->p_wchan = chan;
	p->p_pri = pri;
	if (timo) {
		timeout(endtsleep, (void *)p, timo);
	}

	if (catch) {
		p->p_flag |= SSINTR;
		if ((sig = CURSIG(p)) != 0) {
			unsleep (p);
			p->p_stat = SRUN;
			goto resume;
		}
	}

	p->p_stat = SSLEEP;
	do { 
		idle ();
	} while (p->p_wchan);

resume:
	(void) splx (s);
	p->p_flag &= ~SSINTR;
	if (p->p_flag & STIMO) {
		p->p_flag &= ~STIMO;
		if (catch == 0 || sig == 0) {
			return (EWOULDBLOCK);
		}
	}
	if (timo) {
		untimeout(endtsleep, (void *)p);
	}
	if (catch && (sig || CURSIG(p))) {
		if (p->p_sigacts->ps_sigintr & sigmask(sig)) {
			return (EINTR);
		}
		return (ERESTART);
	}
	return (0);
}


void
sleep (chan, pri)
    void *chan;
    int pri;
{
    tsleep (chan, pri, 0, 0);
}


void
unsleep (p)
    struct proc *p;
{
    int s = splhigh ();
    if (p->p_wchan) {
	p->p_wchan = 0;
    }
    (void) splx (s);
}


void
setrunnable (p)
    struct proc *p;
{
    int s = splhigh();
    if (p->p_stat != SSLEEP)
      panic ("setrunnable");
    unsleep (p);
    p->p_stat = SRUN;
    splx (s);
}


void
endtsleep(sp)
    void *sp;
{
    struct proc *p = sp;
    int s = splhigh();
    if (p->p_wchan) {
	setrunnable(p);
	p->p_flag |= STIMO;
    }
    splx(s);
}

void
wakeup (chan)
	void *chan;
{
	int s = splhigh ();
	if (curproc && curproc->p_wchan == chan) {
		unsleep (curproc);
	}
	(void) splx (s);
}

void
idle ()
{
    int s = spl0 ();
    scandevs ();
    (void) splx (s);
}

#include "../net/netisr.h"
#define DOISR(isr, handler) \
 if (netisr & (1 << (isr))) { \
	netisr &= ~(1 << (isr)); \
	handler (); \
 }

void
softnet ()
{
#ifdef INET
    DOISR(NETISR_ARP, arpintr);
    DOISR(NETISR_IP, ipintr);
#endif
#ifdef NS
    DOISR(NETISR_NS, nsintr);
#endif
#ifdef ISO
    DOISR(NETISR_ISO, clnlintr);
#endif
#ifdef CCITT
    DOISR(NETISR_CCITT, hdintr);
#endif
#ifdef NETISR_SCLK
    DOISR(NETISR_SCLK, softclock);
#endif
}
