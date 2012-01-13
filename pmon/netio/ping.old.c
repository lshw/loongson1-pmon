/* $Id: ping.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */
/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Muuss.
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
 */

/*
 *			P I N G . C
 *
 * Using the InterNet Control Message Protocol (ICMP) "ECHO" facility,
 * measure round-trip-delays and packet loss across network paths.
 *
 * Author -
 *	Mike Muuss
 *	U. S. Army Ballistic Research Laboratory
 *	December, 1983
 *
 * Status -
 *	Public Domain.  Distribution Unlimited.
 * Bugs -
 *	More statistics could always be gathered.
 *	This program has to run SUID to ROOT to access the ICMP socket.
 */
#undef _KERNEL

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip_var.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <pmon.h>

#define	DEFDATALEN	(64 - 8)	/* default data length */
#define	MAXIPLEN	60
#define	MAXICMPLEN	76
#ifdef PMON
#define	MAXPACKET	(1500 - 60 - 8)/* max packet size */
#else
#define	MAXPACKET	(65536 - 60 - 8)/* max packet size */
#endif
#define	MAXWAIT		10		/* max seconds to wait for response */
#define	NROUTES		9		/* number of record route slots */

#define	A(bit)		rcvd_tbl[(bit)>>3]	/* identify byte in array */
#define	B(bit)		(1 << ((bit) & 0x07))	/* identify bit in byte */
#define	SET(bit)	(A(bit) |= B(bit))
#define	CLR(bit)	(A(bit) &= (~B(bit)))
#define	TST(bit)	(A(bit) & B(bit))

/* various options */
static int options;
#define	F_FLOOD		0x001
#define	F_INTERVAL	0x002
#define	F_PINGFILLED	0x008
#define	F_QUIET		0x010
#define	F_RROUTE	0x020
#define	F_SO_DEBUG	0x040
#define	F_SO_DONTROUTE	0x080
#define	F_VERBOSE	0x100
#define	F_GROUP		0x200

/*
 * MAX_DUP_CHK is the number of bits in received table, i.e. the maximum
 * number of received sequence numbers we can keep track of.  Change 128
 * to 8192 for complete accuracy...
 */
#define	MAX_DUP_CHK	(8 * 128)
static const int mx_dup_ck = MAX_DUP_CHK;
static char rcvd_tbl[MAX_DUP_CHK / 8];

static struct sockaddr whereto;	/* who to ping */
static int s;				/* socket file descriptor */
static u_char outpack[MAXPACKET];
#ifdef PMON
static int datalen;
#define BSPACE '\b'		/* characters written for flood */
#define DOT '.'
#else
static int datalen = DEFDATALEN;
static char BSPACE = '\b';		/* characters written for flood */
static char DOT = '.';
#endif
static char *hostname;
static int ident;			/* process id to identify our packets */

/* counters */
static long npackets;		/* max packets to transmit */
static long nreceived;		/* # of packets we got back */
static long nrepeats;		/* number of duplicates */
static long ntransmitted;	/* sequence # for outbound packets = #sent */
struct itimerval interval;	/* interval between packets */
#ifdef PMON
static int groupsize;		/* 1 packet per group */
#else
static int groupsize = 1;	/* 1 packet per group */
#endif

/* timing */
static int timing;		/* flag to do timing */
#ifdef PMON
static long tmin;		/* minimum round trip time */
#else
static long tmin = LONG_MAX;	/* minimum round trip time */
#endif
static long tmax;		/* maximum round trip time */
static u_long tsum;		/* sum of all times, for doing average */

static void pinger __P((void));
static void pr_pack __P((char *, int, struct sockaddr_in *));
static char *pr_addr __P((ulong));
void tvsub __P((struct timeval *, struct timeval *));
void pr_iph __P((struct ip *));
void pr_icmph __P((struct icmp *));
void pr_retip __P((struct ip *));
static void catcher __P((void));
static void finish __P((void));
static int in_cksum __P((u_short *, int));

void pr_icmph __P((struct icmp *));
static void usage __P((void));
static int com_ping __P((int, char **));

#ifdef PMON
static int
com_ping(argc, argv)
#else
main(argc, argv)
#endif
	int argc;
	char **argv;
{
	extern int errno, optind;
	extern char *optarg;
	struct timeval timeout;
	struct hostent *hp;
	struct sockaddr_in *to;
	struct protoent *proto;
	register int i;
	int ch, fdmask, hold, packlen, preload;
	u_char *datap;
#ifdef PMON
	u_char packet[MAXPACKET];
#else
	u_char *packet;
#endif
	char *p, *target, hnamebuf[MAXHOSTNAMELEN];
#ifdef IP_OPTIONS
	char rspace[3 + 4 * NROUTES + 1];	/* record route space */
#endif

#ifdef PMON
	options = 0;
	npackets = 0;
	bzero (&interval, sizeof(interval));
	bzero (rcvd_tbl, sizeof(rcvd_tbl));
	groupsize = 1;
	nreceived = nrepeats = ntransmitted = 0;
	options = 0;
	datalen = DEFDATALEN;

	timing = 0;		/* flag to do timing */
	tmin = LONG_MAX;	/* minimum round trip time */
	tmax = 0;		/* maximum round trip time */
	tsum = 0;		/* sum of all times, for doing average */
#endif

	preload = 0;
	interval.it_value.tv_sec = 1;
	datap = &outpack[8 + sizeof(struct timeval)];
	optind = 0;
	while ((ch = getopt(argc, argv, "Rc:dfg:h:i:l:p:qrs:v")) != EOF)
		switch(ch) {
		case 'c':
			npackets = atoi(optarg);
			if (npackets <= 0) {
				(void)fprintf(stderr,
				    "ping: bad number of packets to transmit.\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'd':
			options |= F_SO_DEBUG;
			break;
		case 'f':
			if (getuid()) {
				(void)fprintf(stderr,
				    "ping: %s\n", strerror(EPERM));
				exit(EXIT_FAILURE);
			}
			options |= F_FLOOD;
#if 0
			setbuf(stdout, (char *)NULL);
#endif
			break;
		case 'g':
			groupsize = atoi(optarg);
			if (groupsize < 0) {
				(void)fprintf(stderr,
				    "ping: bad groupsize value.\n");
				exit(1);
			}
			options |= F_GROUP;
			break;
		case 'i':		/* wait between sending packets */
			interval.it_value.tv_sec = atoi(optarg);
			if ((p = index(optarg, '.')) != 0) {
			    for (i = 100000; *++p; i /= 10) {
				interval.it_value.tv_usec += i * (*p - '0');
			    }
			}
			if (interval.it_value.tv_sec < 0 ||
			    (interval.it_value.tv_sec == 0 &&
			    interval.it_value.tv_usec < 10000)) {
				(void)fprintf(stderr,
				    "ping: bad timing interval.\n");
				exit(1);
			}
			options |= F_INTERVAL;
			break;
		case 'l':
			preload = atoi(optarg);
			if (preload < 0) {
				(void)fprintf(stderr,
				    "ping: bad preload value.\n");
				exit(EXIT_FAILURE);
			}
			break;
#ifndef PMON
		case 'p':		/* fill buffer with user pattern */
			options |= F_PINGFILLED;
			fill((char *)datap, optarg);
				break;
#endif
		case 'q':
			options |= F_QUIET;
			break;
		case 'R':
			options |= F_RROUTE;
			break;
		case 'r':
			options |= F_SO_DONTROUTE;
			break;
		case 's':		/* size of packet to send */
			datalen = atoi(optarg);
			if (datalen > MAXPACKET) {
				(void)fprintf(stderr,
				    "ping: packet size too large.\n");
				exit(EXIT_FAILURE);
			}
			if (datalen <= 0) {
				(void)fprintf(stderr,
				    "ping: illegal packet size.\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'v':
			options |= F_VERBOSE;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();
	target = *argv;

	bzero((char *)&whereto, sizeof(struct sockaddr));
	to = (struct sockaddr_in *)&whereto;
	to->sin_family = AF_INET;
	to->sin_addr.s_addr = inet_addr(target);
	if (to->sin_addr.s_addr != (u_int)-1)
		hostname = target;
	else {
		hp = gethostbyname(target);
		if (!hp) {
			(void)fprintf(stderr,
			    "ping: unknown host %s\n", target);
			exit(EXIT_FAILURE);
		}
		to->sin_family = hp->h_addrtype;
		bcopy(hp->h_addr, (caddr_t)&to->sin_addr, hp->h_length);
		(void)strncpy(hnamebuf, hp->h_name, sizeof(hnamebuf) - 1);
		hnamebuf[sizeof(hnamebuf) - 1] = '\0';
		hostname = hnamebuf;
	}

	if (options & F_FLOOD && options & (F_INTERVAL | F_GROUP)) {
		(void)fprintf(stderr,
		    "ping: -f and -i/-g incompatible options.\n");
		exit(EXIT_FAILURE);
	}

	if (datalen >= sizeof(struct timeval))	/* can we time transfer */
		timing = 1;
	packlen = datalen + MAXIPLEN + MAXICMPLEN;
#ifdef PMON
	if (packlen > MAXPACKET) {
		(void)fprintf(stderr, "ping: packet size too large.\n");
		exit(EXIT_FAILURE);
	}
#else
	if (!(packet = (u_char *)malloc((u_int)packlen))) {
		(void)fprintf(stderr, "ping: out of memory.\n");
		exit(EXIT_FAILURE);
	}
#endif
	if (!(options & F_PINGFILLED))
		for (i = 8; i < datalen; ++i)
			*datap++ = i;

	ident = getpid() & 0xFFFF;

	if (!(proto = getprotobyname("icmp"))) {
		(void)fprintf(stderr, "ping: unknown protocol icmp.\n");
		exit(EXIT_FAILURE);
	}
	if ((s = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0) {
		perror("ping: socket");
		exit(EXIT_FAILURE);
	}
	hold = 1;
	if (options & F_SO_DEBUG)
		(void)setsockopt(s, SOL_SOCKET, SO_DEBUG, (char *)&hold,
		    sizeof(hold));
	if (options & F_SO_DONTROUTE)
		(void)setsockopt(s, SOL_SOCKET, SO_DONTROUTE, (char *)&hold,
		    sizeof(hold));

	/* record route option */
	if (options & F_RROUTE) {
#ifdef IP_OPTIONS
		rspace[IPOPT_OPTVAL] = IPOPT_RR;
		rspace[IPOPT_OLEN] = sizeof(rspace)-1;
		rspace[IPOPT_OFFSET] = IPOPT_MINOFF;
		if (setsockopt(s, IPPROTO_IP, IP_OPTIONS, rspace,
		    sizeof(rspace)) < 0) {
			perror("ping: record route");
			exit(EXIT_FAILURE);
		}
#else
		(void)fprintf(stderr,
		  "ping: record route not available in this implementation.\n");
		exit(EXIT_FAILURE);
#endif /* IP_OPTIONS */
	}

	/*
	 * When pinging the broadcast address, you can get a lot of answers.
	 * Doing something so evil is useful if you are trying to stress the
	 * ethernet, or just want to fill the arp cache to get some stuff for
	 * /etc/ethers.
	 */
	hold = 48 * 1024;
	(void)setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&hold,
	    sizeof(hold));

	if (to->sin_family == AF_INET)
		(void)printf("PING %s (%s): %d data bytes\n", hostname,
		    inet_ntoa(*(struct in_addr *)&to->sin_addr.s_addr),
		    datalen);
	else
		(void)printf("PING %s: %d data bytes\n", hostname, datalen);

	(void)signal(SIGINT, (sig_t)finish);
	(void)signal(SIGALRM, (sig_t)catcher);

	while (preload--)		/* fire off them quickies */
		pinger();

	if ((options & F_FLOOD) == 0)
		catcher();		/* start things going */

	for (;;) {
		struct sockaddr_in from;
		register int cc;
		int fromlen;

		if (options & F_FLOOD) {
			pinger();
			timeout.tv_sec = 0;
			timeout.tv_usec = 10000;
			fdmask = 1 << s;
			if (select(s + 1, (fd_set *)&fdmask, (fd_set *)NULL,
			    (fd_set *)NULL, &timeout) < 1)
				continue;
		}
		fromlen = sizeof(from);
		if ((cc = recvfrom(s, (char *)packet, packlen, 0,
		    (struct sockaddr *)&from, &fromlen)) < 0) {
			if (errno == EINTR)
				continue;
			perror("ping: recvfrom");
			continue;
		}
		pr_pack((char *)packet, cc, &from);
		if (npackets && nreceived >= npackets)
			break;
	}
	finish();
	/* NOTREACHED */
	return(0);
}

/*
 * catcher --
 *	This routine causes another PING to be transmitted, and then
 * schedules another SIGALRM for 1 second from now.
 * 
 * bug --
 *	Our sense of time will slowly skew (i.e., packets will not be
 * launched exactly at 1-second intervals).  This does not affect the
 * quality of the delay and loss statistics.
 */
static void
catcher()
{
	int waittime;

	pinger();
	if (!npackets || ntransmitted < npackets) {
		(void)signal(SIGALRM, (sig_t)catcher);
		setitimer(ITIMER_REAL, &interval, (struct itimerval *)NULL);
	} else {
#ifndef PMON
		if (nreceived) {
			waittime = 2 * tmax / 1000000;
			if (!waittime)
				waittime = 1;
		} else
#endif
			waittime = MAXWAIT;
		(void)signal(SIGALRM, (sig_t)finish);
		(void)alarm((u_int)waittime);
	}
}

/*
 * pinger --
 * 	Compose and transmit an ICMP ECHO REQUEST packet.  The IP packet
 * will be added on by the kernel.  The ID field is our UNIX process ID,
 * and the sequence number is an ascending integer.  The first 8 bytes
 * of the data portion are used to hold a UNIX "timeval" struct in VAX
 * byte-order, to compute the round-trip time.
 */
static void
pinger()
{
	register struct icmp *icp;
	register int cc;
	int i, n;

	icp = (struct icmp *)outpack;
	icp->icmp_type = ICMP_ECHO;
	icp->icmp_code = 0;
	icp->icmp_id = ident;			/* ID */

	if (timing)
		(void)gettimeofday((struct timeval *)&outpack[8],
		    (struct timezone *)NULL);

	cc = datalen + 8;			/* skips ICMP portion */

	for (n = 0; n < groupsize; n++) {
		icp->icmp_cksum = 0;
		icp->icmp_seq = ntransmitted++;
		/* compute ICMP checksum here */
		icp->icmp_cksum = in_cksum((u_short *)icp, cc);

		CLR(icp->icmp_seq % mx_dup_ck);
		i = sendto(s, (char *)outpack, cc, 0, &whereto,
		    sizeof(struct sockaddr));

		if (i < 0 || i != cc)  {
			if (i < 0)
				perror("ping: sendto");
			(void)printf("ping: wrote %s %d chars, ret=%d\n",
			    hostname, cc, i);
		}
	}
	if (!(options & F_QUIET) && options & F_FLOOD)
#ifdef PMON
	  {
	      putchar(DOT); fflush(stdout);
	  }
#else
		(void)write(STDOUT_FILENO, &DOT, 1);
#endif
}

/*
 * pr_pack --
 *	Print out the packet, if it came from us.  This logic is necessary
 * because ALL readers of the ICMP socket get a copy of ALL ICMP packets
 * which arrive ('tis only fair).  This permits multiple copies of this
 * program to be run without having intermingled output (or statistics!).
 */
static void
pr_pack(buf, cc, from)
	char *buf;
	int cc;
	struct sockaddr_in *from;
{
	register struct icmp *icp;
	register u_long l;
	register int i, j;
	register u_char *cp,*dp;
	static int old_rrlen;
	static char old_rr[MAX_IPOPTLEN];
	struct ip *ip;
	struct timeval tv, *tp;
	long triptime;
	int hlen, dupflag;

	(void)gettimeofday(&tv, (struct timezone *)NULL);

	/* Check the IP header */
	ip = (struct ip *)buf;
	hlen = ip->ip_hl << 2;
	if (cc < hlen + ICMP_MINLEN) {
		if (options & F_VERBOSE)
			(void)fprintf(stderr,
			  "ping: packet too short (%d bytes) from %s\n", cc,
			  inet_ntoa(*(struct in_addr *)&from->sin_addr.s_addr));
		return;
	}

	/* Now the ICMP part */
	cc -= hlen;
	icp = (struct icmp *)(buf + hlen);
	if (icp->icmp_type == ICMP_ECHOREPLY) {
		if (icp->icmp_id != ident)
			return;			/* 'Twas not our ECHO */
		++nreceived;
		if (timing) {
#ifndef icmp_data
			tp = (struct timeval *)&icp->icmp_ip;
#else
			tp = (struct timeval *)icp->icmp_data;
#endif
			tvsub(&tv, tp);
			triptime = tv.tv_sec * 1000000 + tv.tv_usec;
			tsum += triptime / 10;	/* stored in 10's of usec */
			if (triptime < tmin)
				tmin = triptime;
			if (triptime > tmax)
				tmax = triptime;
		}

		if (TST(icp->icmp_seq % mx_dup_ck)) {
			++nrepeats;
			--nreceived;
			dupflag = 1;
		} else {
			SET(icp->icmp_seq % mx_dup_ck);
			dupflag = 0;
		}

		if (options & F_QUIET)
			return;

		if (options & F_FLOOD)
#ifdef PMON
		  {
		      putchar(BSPACE); fflush(stdout);
		  }
#else
			(void)write(STDOUT_FILENO, &BSPACE, 1);
#endif
		else {
			(void)printf("%d bytes from %s: icmp_seq=%u", cc,
			   inet_ntoa(*(struct in_addr *)&from->sin_addr.s_addr),
			   icp->icmp_seq);
			(void)printf(" ttl=%d", ip->ip_ttl);
			if (timing)
				(void)printf(" time=%d.%03d ms", triptime/1000,
				    triptime % 1000);
		}

		if (options & F_VERBOSE) {
		    static int lastseq;
		    if (dupflag)
			(void)printf(" (DUP! id=%u prev=%u)", icp->icmp_seq,
				     lastseq);
		    lastseq = icp->icmp_seq;
		    if (in_cksum((u_short *)icp, cc) != 0)
			(void)printf(" (CSUM!)");
		}
		else {
		    if (dupflag)
			(void)printf(" (DUP!)");
		}
		    
		if (!(options & F_FLOOD) || (options & F_VERBOSE)) {
		    /* check the data */
		    cp = (u_char*)&icp->icmp_data[8];
		    dp = &outpack[8 + sizeof(struct timeval)];
		    for (i = 8; i < datalen; ++i, ++cp, ++dp) {
			if (*cp != *dp) {
			    (void)printf("\nwrong byte #%d want 0x%x got 0x%x xor 0x%x",
					 i, *dp, *cp, *dp ^ *cp);
			    cp = (u_char*)&icp->icmp_data[0];
			    for (i = 8; i < datalen; ++i, ++cp) {
				if ((i % 32) == 8)
				    (void)printf("\n\t");
				(void)printf("%x ", *cp);
			    }
			    break;
			}
		    }
		}
	} else {
		/* We've got something other than an ECHOREPLY */
		if (!(options & F_VERBOSE))
			return;
		(void)printf("%d bytes from %s: ", cc,
		    pr_addr(from->sin_addr.s_addr));
		pr_icmph(icp);
	}

	/* Display any IP options */
	cp = (u_char *)buf + sizeof(struct ip);

	for (; hlen > (int)sizeof(struct ip); --hlen, ++cp)
		switch (*cp) {
		case IPOPT_EOL:
			hlen = 0;
			break;
		case IPOPT_LSRR:
			(void)printf("\nLSRR: ");
			hlen -= 2;
			j = *++cp;
			++cp;
			if (j > IPOPT_MINOFF)
				for (;;) {
					l = *++cp;
					l = (l<<8) + *++cp;
					l = (l<<8) + *++cp;
					l = (l<<8) + *++cp;
					if (l == 0)
						(void)printf("\t0.0.0.0");
				else
					(void)printf("\t%s", pr_addr(ntohl(l)));
				hlen -= 4;
				j -= 4;
				if (j <= IPOPT_MINOFF)
					break;
				(void)putchar('\n');
			}
			break;
		case IPOPT_RR:
			j = *++cp;		/* get length */
			i = *++cp;		/* and pointer */
			hlen -= 2;
			if (i > j)
				i = j;
			i -= IPOPT_MINOFF;
			if (i <= 0)
				continue;
			if (i == old_rrlen
			    && cp == (u_char *)buf + sizeof(struct ip) + 2
			    && !bcmp((void *)cp, old_rr, i)
			    && !(options & F_FLOOD)) {
				(void)printf("\t(same route)");
				i = ((i + 3) / 4) * 4;
				hlen -= i;
				cp += i;
				break;
			}
			old_rrlen = i;
			bcopy((char *)cp, old_rr, i);
			(void)printf("\nRR: ");
			for (;;) {
				l = *++cp;
				l = (l<<8) + *++cp;
				l = (l<<8) + *++cp;
				l = (l<<8) + *++cp;
				if (l == 0)
					(void)printf("\t0.0.0.0");
				else
					(void)printf("\t%s", pr_addr(ntohl(l)));
				hlen -= 4;
				i -= 4;
				if (i <= 0)
					break;
				(void)putchar('\n');
			}
			break;
		case IPOPT_NOP:
			(void)printf("\nNOP");
			break;
		default:
			(void)printf("\nunknown option %x", *cp);
			break;
		}
	if (!(options & F_FLOOD)) {
		(void)putchar('\n');
		(void)fflush(stdout);
	}
}

/*
 * in_cksum --
 *	Checksum routine for Internet Protocol family headers (C Version)
 */
static int
in_cksum(addr, len)
	u_short *addr;
	int len;
{
	register int nleft = len;
	register u_short *w = addr;
	register int sum = 0;
	u_short answer = 0;

	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1) {
		*(u_char *)(&answer) = *(u_char *)w ;
		sum += answer;
	}

	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return(answer);
}

/*
 * tvsub --
 *	Subtract 2 timeval structs:  out = out - in.  Out is assumed to
 * be >= in.
 */
void
tvsub(out, in)
	struct timeval *out, *in;
{
	if ((out->tv_usec -= in->tv_usec) < 0) {
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

/*
 * finish --
 *	Print out statistics, and give up.
 */
static void
finish()
{
	(void)signal(SIGALRM, SIG_IGN);
	(void)signal(SIGINT, SIG_IGN);
	(void)putchar('\n');
	(void)fflush(stdout);
	(void)printf("--- %s ping statistics ---\n", hostname);
	(void)printf("%ld packets transmitted, ", ntransmitted);
	(void)printf("%ld packets received, ", nreceived);
	if (nrepeats)
		(void)printf("+%ld duplicates, ", nrepeats);
	if (ntransmitted) {
		if (nreceived > ntransmitted) {
			(void)printf("-- somebody's printing up packets!");
		}
		else {
			(void)printf("%d%% packet loss",
			    (int) (((ntransmitted - nreceived) * 100) /
			    ntransmitted));
		}
	}
	(void)putchar('\n');
	if (nreceived && timing)
		(void)printf("round-trip min/avg/max = %d.%03d/%u.%03d/%d.%03d ms\n",
		    tmin / 1000, tmin % 1000,
		    tsum / (nreceived + nrepeats) / 100,
		    (tsum / (nreceived + nrepeats) % 100) * 10,
		    tmax / 1000, tmax % 1000);
	close(s);
	exit(0);
}

#ifdef notdef
static char *ttab[] = {
	"Echo Reply",		/* ip + seq + udata */
	"Dest Unreachable",	/* net, host, proto, port, frag, sr + IP */
	"Source Quench",	/* IP */
	"Redirect",		/* redirect type, gateway, + IP  */
	"Echo",
	"Time Exceeded",	/* transit, frag reassem + IP */
	"Parameter Problem",	/* pointer + IP */
	"Timestamp",		/* id + seq + three timestamps */
	"Timestamp Reply",	/* " */
	"Info Request",		/* id + sq */
	"Info Reply"		/* " */
};
#endif

/*
 * pr_icmph --
 *	Print a descriptive string about an ICMP header.
 */
void
pr_icmph(icp)
	struct icmp *icp;
{
	switch(icp->icmp_type) {
	case ICMP_ECHOREPLY:
		(void)printf("Echo Reply\n");
		/* XXX ID + Seq + Data */
		break;
	case ICMP_UNREACH:
		switch(icp->icmp_code) {
		case ICMP_UNREACH_NET:
			(void)printf("Destination Net Unreachable\n");
			break;
		case ICMP_UNREACH_HOST:
			(void)printf("Destination Host Unreachable\n");
			break;
		case ICMP_UNREACH_PROTOCOL:
			(void)printf("Destination Protocol Unreachable\n");
			break;
		case ICMP_UNREACH_PORT:
			(void)printf("Destination Port Unreachable\n");
			break;
		case ICMP_UNREACH_NEEDFRAG:
			(void)printf("frag needed and DF set\n");
			break;
		case ICMP_UNREACH_SRCFAIL:
			(void)printf("Source Route Failed\n");
			break;
		default:
			(void)printf("Dest Unreachable, Bad Code: %d\n",
			    icp->icmp_code);
			break;
		}
		/* Print returned IP header information */
#ifndef icmp_data
		pr_retip(&icp->icmp_ip);
#else
		pr_retip((struct ip *)icp->icmp_data);
#endif
		break;
	case ICMP_SOURCEQUENCH:
		(void)printf("Source Quench\n");
#ifndef icmp_data
		pr_retip(&icp->icmp_ip);
#else
		pr_retip((struct ip *)icp->icmp_data);
#endif
		break;
	case ICMP_REDIRECT:
		switch(icp->icmp_code) {
		case ICMP_REDIRECT_NET:
			(void)printf("Redirect Network");
			break;
		case ICMP_REDIRECT_HOST:
			(void)printf("Redirect Host");
			break;
		case ICMP_REDIRECT_TOSNET:
			(void)printf("Redirect Type of Service and Network");
			break;
		case ICMP_REDIRECT_TOSHOST:
			(void)printf("Redirect Type of Service and Host");
			break;
		default:
			(void)printf("Redirect, Bad Code: %d", icp->icmp_code);
			break;
		}
		(void)printf("(New addr: 0x%08lx)\n", icp->icmp_gwaddr.s_addr);
#ifndef icmp_data
		pr_retip(&icp->icmp_ip);
#else
		pr_retip((struct ip *)icp->icmp_data);
#endif
		break;
	case ICMP_ECHO:
		(void)printf("Echo Request\n");
		/* XXX ID + Seq + Data */
		break;
	case ICMP_TIMXCEED:
		switch(icp->icmp_code) {
		case ICMP_TIMXCEED_INTRANS:
			(void)printf("Time to live exceeded\n");
			break;
		case ICMP_TIMXCEED_REASS:
			(void)printf("Frag reassembly time exceeded\n");
			break;
		default:
			(void)printf("Time exceeded, Bad Code: %d\n",
			    icp->icmp_code);
			break;
		}
#ifndef icmp_data
		pr_retip(&icp->icmp_ip);
#else
		pr_retip((struct ip *)icp->icmp_data);
#endif
		break;
	case ICMP_PARAMPROB:
		(void)printf("Parameter problem: pointer = 0x%02x\n",
		    icp->icmp_hun.ih_pptr);
#ifndef icmp_data
		pr_retip(&icp->icmp_ip);
#else
		pr_retip((struct ip *)icp->icmp_data);
#endif
		break;
	case ICMP_TSTAMP:
		(void)printf("Timestamp\n");
		/* XXX ID + Seq + 3 timestamps */
		break;
	case ICMP_TSTAMPREPLY:
		(void)printf("Timestamp Reply\n");
		/* XXX ID + Seq + 3 timestamps */
		break;
	case ICMP_IREQ:
		(void)printf("Information Request\n");
		/* XXX ID + Seq */
		break;
	case ICMP_IREQREPLY:
		(void)printf("Information Reply\n");
		/* XXX ID + Seq */
		break;
#ifdef ICMP_MASKREQ
	case ICMP_MASKREQ:
		(void)printf("Address Mask Request\n");
		break;
#endif
#ifdef ICMP_MASKREPLY
	case ICMP_MASKREPLY:
		(void)printf("Address Mask Reply\n");
		break;
#endif
	default:
		(void)printf("Bad ICMP type: %d\n", icp->icmp_type);
	}
}

/*
 * pr_iph --
 *	Print an IP header with options.
 */
void
pr_iph(ip)
	struct ip *ip;
{
	int hlen;
	u_char *cp;

	hlen = ip->ip_hl << 2;
	cp = (u_char *)ip + 20;		/* point to options */

	(void)printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src      Dst Data\n");
	(void)printf(" %1x  %1x  %02x %04x %04x",
	    ip->ip_v, ip->ip_hl, ip->ip_tos, ip->ip_len, ip->ip_id);
	(void)printf("   %1x %04x", ((ip->ip_off) & 0xe000) >> 13,
	    (ip->ip_off) & 0x1fff);
	(void)printf("  %02x  %02x %04x", ip->ip_ttl, ip->ip_p, ip->ip_sum);
	(void)printf(" %s ", inet_ntoa(*(struct in_addr *)&ip->ip_src.s_addr));
	(void)printf(" %s ", inet_ntoa(*(struct in_addr *)&ip->ip_dst.s_addr));
	/* dump and option bytes */
	while (hlen-- > 20) {
		(void)printf("%02x", *cp++);
	}
	(void)putchar('\n');
}

/*
 * pr_addr --
 *	Return an ascii host address as a dotted quad and optionally with
 * a hostname.
 */
static char *
pr_addr(l)
	u_long l;
{
	static char buf[80];

	(void)sprintf(buf, "%s", inet_ntoa(*(struct in_addr *)&l));
	return(buf);
}

/*
 * pr_retip --
 *	Dump some info on a returned (via ICMP) IP packet.
 */
void
pr_retip(ip)
	struct ip *ip;
{
	int hlen;
	u_char *cp;

	pr_iph(ip);
	hlen = ip->ip_hl << 2;
	cp = (u_char *)ip + hlen;

	if (ip->ip_p == 6)
		(void)printf("TCP: from port %u, to port %u (decimal)\n",
		    (*cp * 256 + *(cp + 1)), (*(cp + 2) * 256 + *(cp + 3)));
	else if (ip->ip_p == 17)
		(void)printf("UDP: from port %u, to port %u (decimal)\n",
			(*cp * 256 + *(cp + 1)), (*(cp + 2) * 256 + *(cp + 3)));
}

#ifndef PMON
fill(bp, patp)
	char *bp, *patp;
{
	register int ii, jj, kk;
	int pat[16];
	char *cp;

	for (cp = patp; *cp; cp++)
		if (!isxdigit(*cp)) {
			(void)fprintf(stderr,
			    "ping: patterns must be specified as hex digits.\n");
			exit(EXIT_FAILURE);
		}
	ii = sscanf(patp,
	    "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
	    &pat[0], &pat[1], &pat[2], &pat[3], &pat[4], &pat[5], &pat[6],
	    &pat[7], &pat[8], &pat[9], &pat[10], &pat[11], &pat[12],
	    &pat[13], &pat[14], &pat[15]);

	if (ii > 0)
		for (kk = 0; kk <= MAXPACKET - (8 + ii); kk += ii)
			for (jj = 0; jj < ii; ++jj)
				bp[jj + kk] = pat[jj];
	if (!(options & F_QUIET)) {
		(void)printf("PATTERN: 0x");
		for (jj = 0; jj < ii; ++jj)
			(void)printf("%02x", bp[jj] & 0xFF);
		(void)printf("\n");
	}
}
#endif

static void
usage()
{
#ifdef PMON
	(void)fprintf(stderr,
	    "usage: ping [-fqrv] [-c count] [-s size] host\n");
#else
	(void)fprintf(stderr,
	    "usage: ping [-Rdfnqrv] [-c count] [-g group] [-i wait] \n");
	(void)fprintf(stderr,
		      "\t[-l preload] [-p pattern] [-s packetsize] host\n");
#endif
	exit(EXIT_FAILURE);
}

#ifdef PMON
int		cmd_ping __P((int, char *[]));
int
cmd_ping (argc, argv)
    int argc; char **argv;
{
    int ret;
    ret = spawn ("ping", com_ping, argc, argv);
    return (ret & ~0xff) ? 1 : (signed char)ret;
}

/*
 *  Command table registration
 *  ==========================
 */

extern const Optdesc  flash_opts[];

static const Cmd Cmds[] =
{
	{"Network"},
	{"ping",	"[-fqrv] [-c cnt] [-s sz] host",
			0,
			"ping remote host",
			cmd_ping, 1, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
#endif
