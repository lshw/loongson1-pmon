/*	$OpenBSD: raw_ip.c,v 1.19 1999/09/23 07:20:35 deraadt Exp $	*/
/*	$NetBSD: raw_ip.c,v 1.25 1996/02/18 18:58:33 christos Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)raw_ip.c	8.2 (Berkeley) 1/4/94
 */

/*
%%% portions-copyright-nrl-95
Portions of this software are Copyright 1995-1998 by Randall Atkinson,
Ronald Lee, Daniel McDonald, Bao Phan, and Chris Winters. All Rights
Reserved. All rights under this copyright have been assigned to the US
Naval Research Laboratory (NRL). The NRL Copyright Notice and License
Agreement Version 1.1 (January 17, 1995) applies to these portions of the
software.
You should have received a copy of the license with this software. If you
didn't get a copy, you may request one from <license@ipv6.nrl.navy.mil>.
*/

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/domain.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_mroute.h>
#include <netinet/ip_var.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_icmp.h>
#include <sys/netinet/if_ether.h>


#include <machine/stdarg.h>

struct inpcbtable rawcbtable;

/*
 * Nominal space allocated to a raw ip socket.
 */
#define	RIPSNDQ		8192
#define	RIPRCVQ		8192

/*
 * Raw interface to ether.
 */

	struct socket *last = 0;
/*
 * Initialize raw connection block q.
 */
void
rether_init()
{
	in_pcbinit(&rawcbtable, 1);
}


/*
 * Setup generic address and protocol structures
 * for raw_input routine, then pass them along with
 * mbuf chain.
 */
void
#if __STDC__
rether_input(struct mbuf *m, ...)
#else
rether_input(m, va_alist)
	struct mbuf *m;
	va_dcl
#endif
{
	register struct ip *ip = mtod(m, struct ip *);
	register struct inpcb *inp;
	struct socket *so = 0;
	struct ifnet *ifp;
	va_list ap;
	va_start(ap, m);
	ifp = va_arg(ap,struct ifnet *);
	va_end(ap);

	m->m_len +=sizeof(struct ether_header);
	m->m_pkthdr.len = m->m_len;
	m->m_data -= sizeof(struct ether_header);
	for (inp = rawcbtable.inpt_queue.cqh_first;
	    inp != (struct inpcb *)&rawcbtable.inpt_queue;
	    inp = inp->inp_queue.cqe_next) {
			struct mbuf *n;
		if (strcmp(inp->inp_ifname,ifp->if_xname))
		continue;
		so=inp->inp_socket;
			if ((n = m_copy(m, 0, (int)M_COPYALL)) != NULL) {
				sbappend(&so->so_rcv,n);
				sorwakeup(so);
			}
     }
	m->m_len -=sizeof(struct ether_header);
	m->m_pkthdr.len = m->m_len;
	m->m_data += sizeof(struct ether_header);
}

/*
 * Generate IP header and pass packet to ip_output.
 * Tack on options user may have setup with control call.
 */
int
#if __STDC__
rether_output(struct mbuf *m, ...)
#else
rether_output(m, va_alist)
	struct mbuf *m;
	va_dcl
#endif
{
	struct socket *so;
	u_long dst;
	register struct ip *ip;
	register struct inpcb *inp;
	int flags;
	va_list ap;
	int s;
	struct ifnet *ifp;

	va_start(ap, m);
	so = va_arg(ap, struct socket *);
	ifp =ifunit(va_arg(ap,char *));
	va_end(ap);
	if(!ifp)return -1;


	s = splimp();
	/*
	 * Queue message on interface, and start output if interface
	 * not yet active.
	 */
	if (IF_QFULL(&ifp->if_snd)) {
		IF_DROP(&ifp->if_snd);
		splx(s);
	}
	ifp->if_obytes += m->m_pkthdr.len;
	IF_ENQUEUE(&ifp->if_snd, m);
	if (m->m_flags & M_MCAST)
		ifp->if_omcasts++;
	if ((ifp->if_flags & IFF_OACTIVE) == 0)
		(*ifp->if_start)(ifp);
	splx(s);
	return 0;
}

/*
 * Raw IP socket option processing.
 */
int
rether_ctloutput(op, so, level, optname, m)
	int op;
	struct socket *so;
	int level, optname;
	struct mbuf **m;
{
		return (EOPNOTSUPP);
}

u_long	rether_sendspace = RIPSNDQ;
u_long	rether_recvspace = RIPRCVQ;

/*ARGSUSED*/
int
rether_usrreq(so, req, m, nam, control)
	register struct socket *so;
	int req;
	struct mbuf *m, *nam, *control;
{
	register int error = 0;
	register struct inpcb *inp = sotoinpcb(so);
	struct sockaddr *sa;
#ifdef MROUTING
	extern struct socket *ip_mrouter;
#endif
	if(nam) sa=mtod(nam,struct sockaddr *);
//nam is addr
	if (req == PRU_CONTROL)
		return (in_control(so, (u_long)m, (caddr_t)nam,
			(struct ifnet *)control));

	if (inp == NULL && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}

	switch (req) {

	case PRU_ATTACH:
		if (inp)
			panic("rether_attach");
		if ((so->so_state & SS_PRIV) == 0) {
			error = EACCES;
			break;
		}
		if ((error = soreserve(so, rether_sendspace, rether_recvspace)) ||
		    (error = in_pcballoc(so, &rawcbtable)))
			break;
		inp = (struct inpcb *)so->so_pcb;
		inp->inp_ifname[0]=0;
		inp->inp_ofname[0]=0;
		break;

	case PRU_DISCONNECT:
		if ((so->so_state & SS_ISCONNECTED) == 0) {
			error = ENOTCONN;
			break;
		}
		/* FALLTHROUGH */
	case PRU_ABORT:
		soisdisconnected(so);
		/* FALLTHROUGH */
	case PRU_DETACH:
		if (inp == 0)
			panic("rether_detach");
#ifdef MROUTING
		if (so == ip_mrouter)
			ip_mrouter_done();
#endif
		in_pcbdetach(inp);
		break;

	case PRU_BIND:
	    {
		strcpy(inp->inp_ifname,sa->sa_data);
		break;
	    }
	case PRU_CONNECT:
	    {
		strcpy(inp->inp_ofname,sa->sa_data);

		soisconnected(so);
		break;
	    }

	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		break;

	/*
	 * Mark the connection as being incapable of further input.
	 */
	case PRU_SHUTDOWN:
		socantsendmore(so);
		break;

	/*
	 * Ship a packet out.  The appropriate raw output
	 * routine handles any massaging necessary.
	 */
	case PRU_SEND:
	    {
		char *ifname[16];
			if (nam == NULL) {
				error = ENOTCONN;
				break;
			}
			strcpy(ifname,sa->sa_data);
		error = rether_output(m, so, ifname);
	}
		m = NULL;
		break;

	case PRU_SENSE:
		/*
		 * stat: don't bother with a blocksize.
		 */
		return (0);

	/*
	 * Not supported.
	 */
	case PRU_RCVOOB:
	case PRU_RCVD:
	case PRU_LISTEN:
	case PRU_ACCEPT:
	case PRU_SENDOOB:
		error = EOPNOTSUPP;
		break;

	case PRU_SOCKADDR:
		in_setsockaddr(inp, nam);
		break;

	case PRU_PEERADDR:
		in_setpeeraddr(inp, nam);
		break;

	default:
		panic("rether_usrreq");
	}
release:
	if (m != NULL)
		m_freem(m);
	return (error);
}

extern	struct domain etherdomain;

struct protosw ethersw[] = {
{ SOCK_RAW,	&etherdomain,	0,	0,
  rether_input,	rether_output,	0,		0,
  rether_usrreq,
  0,		0,		0,		0,
},
};
struct domain etherdomain =
    { AF_UNSPEC, "ether", 0, 0, 0, 
      ethersw, &ethersw[sizeof(ethersw)/sizeof(ethersw[0])], 0,
      0, 32, sizeof(struct sockaddr_in) };

