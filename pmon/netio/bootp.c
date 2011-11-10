/* $Id: bootp.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2001-2002 Opsycon AB  (www.opsycon.se / www.opsycon.com)
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
 *	This product includes software developed by Opsycon AB, Sweden.
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
#include <sys/file.h>
#include <sys/syslog.h>
#include <sys/endian.h>
#ifdef _KERNEL
#undef _KERNEL
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#define _KERNEL
#else
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#endif

#include <net/if.h>
#include <net/if_dl.h>

#define KERNEL
#include <sys/time.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <setjmp.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include <pmon.h>

#include "bootparams.h"
#include "bootp.h"

extern struct timeval	boottime;
extern struct timeval	time;
int bcmp __P((const void *, const void *, size_t));

extern char          *inet_ntoa __P((struct in_addr));

#define MAXTMO	20	/* seconds */
#define MINTMO	2	/* seconds */
#define MAX_MSG (3 * 512)

/* Returns true if n_long's on the same net */
#define	SAMENET(a1, a2, m) ((a1.s_addr & m.s_addr) == (a2.s_addr & m.s_addr))

static	char vm_zero[4];
static	char vm_rfc1048[4] = VM_RFC1048;
static	char vm_cmu[4] = VM_CMU;

static int nvend;
static char *vend[] = {
	vm_zero,			/* try no explicit vendor type first */
	vm_cmu,
	vm_rfc1048			/* try variable format last */
};

/* Local forwards */
static	int bootprecv (struct bootparams *, struct bootp *, int);
static	void vend_cmu (struct bootparams *, u_char *);
static	void vend_rfc1048 (struct bootparams *, u_char *, u_int);

static	int xid;
static	jmp_buf jmpb;

int
getethaddr (unsigned char *eaddr, char *ifc)
{
    struct ifnet *ifp;
    struct ifaddr *ifa;

    /* look for first HW interface */
    for (ifp = ifnet.tqh_first; ifp != 0; ifp = ifp->if_list.tqe_next) {
        if (strcmp (ifp->if_xname, ifc) != 0) {
	    continue;
	}
        /* find link address */
        for (ifa = ifp->if_addrlist.tqh_first; ifa; ifa = ifa->ifa_list.tqe_next) {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_LINK) {
                struct sockaddr_dl *sdl;
                sdl = (struct sockaddr_dl *) ifa->ifa_addr;
                bcopy (LLADDR(sdl), eaddr, ifp->if_addrlen);
                return ifp->if_addrlen;
            }
        }
    }
    return -1;
}


static void
finish (int signo)
{
    longjmp (jmpb, 1);
}


/* Fetch required bootp infomation */
void
boot_bootp(struct bootparams *bootp, char *ifc)
{
    struct bootp wbp;
    u_char rbuf[MAX_MSG];
    struct sockaddr_in clnt, srvr;
    time_t tmo, tlast, tleft;
    struct timeval tv;
    fd_set rfds;
    sig_t osigint;
    int sock, cc, n, recvcnt;
    volatile int notified;

    bzero(&wbp, sizeof(wbp));
    wbp.bp_op = BOOTREQUEST;
    wbp.bp_htype = HTYPE_ETHERNET;
    wbp.bp_hlen = 6;
    if (getethaddr (wbp.bp_chaddr, ifc) < 0)
	return;
	    
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
	perror ("bootp socket");
	return;
    }

    n = 1;
    if (setsockopt (sock, SOL_SOCKET, SO_BROADCAST, &n, sizeof (n)) < 0) {
	perror ("bootp setsockopt(BROADCAST)");
	close (sock);
	return;
    }
    if (setsockopt (sock, SOL_SOCKET, SO_DONTROUTE, &n, sizeof (n)) < 0) {
	perror ("bootp setsockopt(DONTROUTE)");
	close (sock);
	return;
    }

    bzero ((char *)&clnt, sizeof (clnt));
    clnt.sin_family = AF_INET;
    clnt.sin_addr.s_addr = INADDR_ANY;
    clnt.sin_port = htons (IPPORT_BOOTPC);
    if (bind (sock, (struct sockaddr *)&clnt, sizeof(clnt)) < 0) {
	perror ("bootp bind");
	close (sock);
	return;
    }

    bzero ((char *)&srvr, sizeof (srvr));
    srvr.sin_family = AF_INET;
    srvr.sin_addr.s_addr = INADDR_BROADCAST;
    srvr.sin_port = htons (IPPORT_BOOTPS);

    notified = 0;
    
    osigint = signal(SIGINT, finish);
    if (setjmp (jmpb)) {
	if (notified)
	    fprintf (stderr, "interrupted\n");
	goto quit;
    }

    for (xid = time.tv_sec, n = 5; n >= 0; n--)
	xid = (xid << 5) + wbp.bp_chaddr[n];

    do {
	tlast = time.tv_sec;
	tmo = MINTMO;
	tleft = 0;
	recvcnt = 0;
	while (tmo < MAXTMO) {
	    if (tleft <= 0) {
		if ((nvend != 0 || tmo > MINTMO) && !notified) {
		    fprintf (stderr, "Waiting for bootp");
		    notified = 1;
		}
		if (notified)
		    fprintf (stderr, ".");

		wbp.bp_xid = htonl (++xid);
		wbp.bp_secs = htons (time.tv_sec - boottime.tv_sec);
		bzero(wbp.bp_file, sizeof(wbp.bp_file));

		bcopy(vend[nvend], wbp.bp_vend, sizeof(long));
		wbp.bp_vend[sizeof(long)] = TAG_END;

		/* If we need vendor info, cycle vendor next time */
		if ((bootp->need & ~bootp->have) & ~BOOT_ADDR)
		    nvend = (nvend + 1) % (sizeof(vend) / sizeof(vend[0]));

		cc = sendto (sock, &wbp, sizeof(wbp), 0,
			     (struct sockaddr *)&srvr, sizeof(srvr));
		if (cc != sizeof(wbp)) {
		    perror ("bootp sendto");
		    goto quit;
		}
		tleft = tmo;
	    }

	    FD_ZERO (&rfds);
	    FD_SET (sock, &rfds);
	    tv.tv_sec = tleft; tv.tv_usec = 0;
	    n = select (sock + 1, &rfds, 0, 0, &tv);
	    if (n < 0) {
		perror ("bootp select");
		goto quit;
	    }

	    if (n == 0) {
		/* timeout: did we get anything useful */
		if (recvcnt > 0)
		    break;
		tmo <<= 1;
	    } else {
		cc = recv (sock, rbuf, sizeof(rbuf), 0);
		if (cc < 0) {
		    perror ("bootp recv");
		    goto quit;
		}

		/* Got a packet, process it */
		cc = bootprecv (bootp, (struct bootp *)rbuf, cc);
		if (cc >= 0)
		    ++recvcnt;
	    }

	    /* reduce time left until timeout */
	    tleft -= time.tv_sec - tlast;
	    tlast = time.tv_sec;
	}
    } while ((bootp->have & bootp->need) != bootp->need && tmo < MAXTMO);

    if (notified)
	fprintf (stderr, "\n");

quit:
    (void) signal (SIGINT, osigint);
    close (sock);
    sigsetmask (0);
}

#define BOOTP_NO_CHECK_SIZE
/* Returns 0 if this is the packet we're waiting for else -1 */
static int
bootprecv(struct bootparams *bootp, struct bootp *bp, int rsize)
{
    static struct in_addr nmask;
    u_long ul;

    if (
#ifndef BOOTP_NO_CHECK_SIZE
rsize < sizeof(*bp) || 
#endif
			ntohl(bp->bp_xid) != xid) {
#ifdef DEBUG
#ifndef BOOTP_NO_CHECK_SIZE
	if (rsize < sizeof(*bp))
	    printf("bootprecv: expected %d bytes, got %d\n",
		   sizeof(*bp), rsize);
	else 
#endif
	    printf("bootprecv: expected xid 0x%x, got 0x%x\n",
		   xid, ntohl(bp->bp_xid));
#endif
	return (-1);
    }

#ifdef DEBUG
    printf("bootprecv: got one (len = %d)!\n", rsize);
#endif

    /* Pick up our ip address (and natural netmask) */
    if (bp->bp_yiaddr.s_addr != 0 && (bootp->have & BOOT_ADDR) == 0) {
	u_long addr;

	bootp->have |= BOOT_ADDR;
	bootp->addr = bp->bp_yiaddr;
#ifdef DEBUG
	printf("our ip address is %s\n", inet_ntoa(bootp->addr));
#endif
	
	addr = ntohl (bootp->addr.s_addr);
	if (IN_CLASSA(addr))
	    nmask.s_addr = htonl (IN_CLASSA_NET);
	else if (IN_CLASSB(addr))
	    nmask.s_addr = htonl (IN_CLASSB_NET);
	else
	    nmask.s_addr = htonl (IN_CLASSC_NET);
#ifdef DEBUG
	printf("'native netmask' is %s\n", inet_ntoa(nmask));
#endif
    }

    if (bp->bp_siaddr.s_addr != 0 && (bootp->have & BOOT_BOOTIP) == 0) {
	bootp->have |= BOOT_BOOTIP;
	bootp->bootip = bp->bp_siaddr;
    }

    if (bp->bp_file[0] != 0 && (bootp->have & BOOT_BOOTFILE) == 0) {
	bootp->have |= BOOT_BOOTFILE;
	strncpy (bootp->bootfile, bp->bp_file, sizeof (bootp->bootfile));
    }

    /* Suck out vendor info */
    if (bcmp(vm_cmu, bp->bp_vend, sizeof(vm_cmu)) == 0)
	vend_cmu(bootp, bp->bp_vend);
    else if (bcmp(vm_rfc1048, bp->bp_vend, sizeof(vm_rfc1048)) == 0)
	vend_rfc1048(bootp, bp->bp_vend, sizeof(bp->bp_vend));
    else if (bcmp(vm_zero, bp->bp_vend, sizeof(vm_zero)) != 0) {
	bcopy(bp->bp_vend, (void *)&ul, sizeof(ul));
	printf("bootprecv: unknown vendor 0x%x\n", ul);
    }

    /* Nothing more to do if we don't know our ip address yet */
    if ((bootp->have & BOOT_ADDR) == 0) {
	return (-1);
    }

    /* Check subnet mask against net mask; toss if bogus */
    if ((bootp->have & BOOT_MASK) != 0 &&
	(nmask.s_addr & bootp->mask.s_addr) != nmask.s_addr) {
#ifdef DEBUG
	printf("subnet mask (%s) bad\n", inet_ntoa(bootp->mask));
#endif
	bootp->have &= ~BOOT_MASK;
    }

    /* Get subnet (or natural net) mask */
    if ((bootp->have & BOOT_MASK) == 0)
	bootp->mask = nmask;

    /* Toss gateway if on a different net */
    if ((bootp->have & BOOT_GATEIP) != 0
	&& !SAMENET(bootp->addr, bootp->gateip, bootp->mask)) {
#ifdef DEBUG
	printf("gateway ip (%s) bad\n", inet_ntoa(bootp->gateip));
#endif
	bootp->gateip.s_addr = 0;
	bootp->have &= ~BOOT_GATEIP;
    }

#ifdef DEBUG
    if (bootp->have & BOOT_MASK)
	printf("smask: %s\n", inet_ntoa(bootp->mask));
    if (bootp->have & BOOT_GATEIP)
	printf("gateway ip: %s\n", inet_ntoa(bootp->gateip));
    if (bootp->have & BOOT_DNSIP)
	printf("nserver ip: %s\n", inet_ntoa(bootp->dnsip));
    if (bootp->have & BOOT_BOOTIP)
	printf("boot ip: %s\n", inet_ntoa(bootp->bootip));
    if (bootp->have & BOOT_HOSTNAME)
	printf ("hostname: %s\n", bootp->hostname);
    if (bootp->have & BOOT_DOMAINNAME)
	printf ("domainname: %s\n", bootp->domainname);
    if (bootp->have & BOOT_BOOTFILE)
	printf("boot file: %s\n", bootp->bootfile);
#endif

    return (0);
}


static void
vend_cmu(struct bootparams *bootp, u_char *cp)
{
    struct cmu_vend *vp;

    vp = (struct cmu_vend *)cp;

    if (vp->v_smask.s_addr != 0 && (bootp->have & BOOT_MASK) == 0) {
	bootp->have |= BOOT_MASK;
	bootp->mask = vp->v_smask;
    }
    if (vp->v_dgate.s_addr != 0 && (bootp->have & BOOT_GATEIP) == 0) {
	bootp->have |= BOOT_GATEIP;
	bootp->gateip = vp->v_dgate;
    }
    if (vp->v_dns1.s_addr != 0 && (bootp->have & BOOT_DNSIP) == 0) {
	bootp->have |= BOOT_DNSIP;
	bootp->dnsip = vp->v_dns1;
    }
}


static void
vend_rfc1048(struct bootparams *bootp, u_char *cp, u_int len)
{
    u_char *ep;
    int size;
    u_char tag;

    ep = cp + len;

    /* Step over magic cookie */
    cp += sizeof(long);

    while (cp < ep) {
	tag = *cp++;
	size = *cp++;
	if (tag == TAG_END)
	    break;
	switch (tag) {
	case TAG_SUBNET_MASK:
	    if (size == sizeof(long)
		&& (bootp->have & BOOT_MASK) == 0) {
		bootp->have |= BOOT_MASK;
		bcopy(cp, (void *)&bootp->mask, sizeof(long));
	    }
	    break;
	case TAG_GATEWAY:
	    if (size == sizeof(long)
		&& (bootp->have & BOOT_GATEIP) == 0) {
		bootp->have |= BOOT_GATEIP;
		bcopy(cp, (void *)&bootp->gateip, sizeof(long));
	    }
	    break;
	case TAG_DOMAIN_SERVER:
	    if (size == sizeof(long)
		&& (bootp->have & BOOT_DNSIP) == 0) {
		bootp->have |= BOOT_DNSIP;
		bcopy(cp, (void *)&bootp->dnsip, sizeof(long));
	    }
	    break;
	case TAG_HOST_NAME:
	    if (size < sizeof (bootp->hostname)
		&& (bootp->have & BOOT_HOSTNAME) == 0) {
		bootp->have |= BOOT_HOSTNAME;
		bcopy (cp, bootp->hostname, size);
		bootp->hostname[size] = '\0';
	    }
	    break;
	case TAG_DOMAIN_NAME:
	    if (size < sizeof (bootp->domainname)
		&& (bootp->have & BOOT_DOMAINNAME) == 0) {
		bootp->have |= BOOT_DOMAINNAME;
		bcopy (cp, bootp->domainname, size);
		bootp->domainname[size] = '\0';
	    }
	    break;
	}
	cp += size;
    }
}
