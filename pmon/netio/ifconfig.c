/* $Id: ifconfig.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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
#ifdef _KERNEL
#undef _KERNEL
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/protosw.h>
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#endif
#include <net/route.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pmon.h>

#include "bootparams.h"
#include "bootp.h"

#define SIN(x) ((struct sockaddr_in *)&(x))
#define SAD(x) ((struct sockaddr *)&(x))

static void boot_getenv (struct bootparams *bootp, char *ifname);
static void boot_parsecfg (struct bootparams *bootp, char *cfg);
static void boot_setenv (struct bootparams *bootp, char *ifname, int level);

int ifconfig __P((char *, char *));

static void
setsin (struct sockaddr_in *sa, int family, u_long addr)
{
    bzero (sa, sizeof (*sa));
    sa->sin_len = sizeof (*sa);
    sa->sin_family = family;
    sa->sin_addr.s_addr = addr;
}

/*
 *  Configure an interface. 'ifname' is the interface to configure.
 *  'ipaddr' is a colon spearated string with parameters. Thus:
 *  <ipaddr>[:<netmask>[:<broadcast>[:<gateway>]]] | bootp
 */
char activeif_name[IFNAMSIZ];
int
ifconfig (ifname, ipaddr)
	char *ifname;
	char *ipaddr;
{
	struct sockaddr local, loop;
	struct sockaddr zmask;
	struct bootparams bootp;
	struct ifaliasreq addreq;
	char *gw;
	int s, bootplev;

	if (ifname == NULL) {
		printf("ifconfig: no ifc name\n");
		return (0);
	}
	strncpy(activeif_name, ifname, IFNAMSIZ);

	bzero (&bootp, sizeof (bootp));
	bootp.need = BOOT_ADDR;

	s = socket (AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		perror("ifconfig: socket");
		return(0);
	}

		//printf("bootp=%x\n",&bootp);
	/*
	 * Get the parameters for the ethernet interface 
	 */
#if 1
	bootplev = matchenv ("bootp");
	if (strncmp(ipaddr, "bootp", 5) == 0) {
		bootplev = 2;
	} else {
		boot_parsecfg(&bootp, ipaddr);
	}
#endif

	if (bootplev <= 1 && ipaddr != NULL) {
#if 1
		if (inet_aton (ipaddr, &bootp.addr)) {
			bootp.have |= BOOT_ADDR;
		}
#endif
		printf("bootp=%x\n",&bootp);
		boot_getenv (&bootp, ifname);	/* Get any others from env */
	}

	if (bootplev >= 1) {
		/* Enable interface with an initial dummy net address.
		   Note ipintr() has been kludged to route all input packets
		   to an interface on the loopback net. */
		bzero (&addreq, sizeof(addreq));
		strncpy(addreq.ifra_name, ifname, IFNAMSIZ);
		setsin (SIN(addreq.ifra_addr), AF_INET, htonl (0x7f000002));
		setsin (SIN(addreq.ifra_broadaddr), AF_INET, INADDR_ANY);
		if (ioctl(s, SIOCAIFADDR, &addreq) < 0) {
			perror("ioctl (SIOCAIFADDR) dummy");
			close (s);
			return(0);
		}

		/* get network parameters from network */
		boot_bootp (&bootp, ifname);

		/* delete dummy address */
		(void) ioctl(s, SIOCDIFADDR, &addreq);

		/* get any remaining unknown parameters from environment */
		boot_getenv (&bootp, ifname);
	}

	if (!(bootp.have & BOOT_ADDR)) {
		if (bootplev >= 1)
			fprintf(stderr, "\nERROR: bootp setup of %s failed\n", ifname);
		else
			fprintf(stderr, "\nNOTICE: set $ifconfig to enable network\n");
		close (s);
		return (0);
	}

	/* save parameters in volatile or non-volatile environment */
	boot_setenv (&bootp, ifname, bootplev);

	/* got everyting we can, put them into the ifaddreq structure */
	bzero (&addreq, sizeof(addreq));
	strncpy(addreq.ifra_name, ifname, IFNAMSIZ);
	setsin (SIN(addreq.ifra_addr), AF_INET, bootp.addr.s_addr);
	if (bootp.have & BOOT_MASK) {
		setsin (SIN(addreq.ifra_mask), 0, bootp.mask.s_addr);
	}
	if (bootp.have & BOOT_BROADADDR) {
		setsin (SIN(addreq.ifra_broadaddr), AF_INET, bootp.broadaddr.s_addr);
	}
	/* remember our local address for later */
	local = *SAD(addreq.ifra_addr);

	/* now set our actual address */
	if (ioctl(s, SIOCAIFADDR, &addreq) < 0) {
		/* Assume this means no network interface to attach to */
		fprintf (stderr, "\nNOTICE: No network interface available\n");
		/*perror("ioctl (SIOCAIFADDR)"); */
		close (s);
		return(0);
	}

	/*
	 * Configure the loopback i/f (lo0).
	 */
	bzero (&addreq, sizeof(addreq));
	strncpy(addreq.ifra_name, "lo0", IFNAMSIZ);
	setsin (SIN(addreq.ifra_addr), AF_INET, htonl (0x7f000001));
	setsin (SIN(addreq.ifra_mask), 0, htonl (0xff000000));
	/* remember loopback address for later */
	loop = *SAD(addreq.ifra_addr);

	if (ioctl(s, SIOCAIFADDR, &addreq) < 0) {
		perror("ioctl (SIOCAIFADDR) loopback");
		close (s);
		return(0);
	}

	/*
	 * Now setup the routing tables, equivalent to:
	 * route add default $gateway 0
	 * route add $ipaddr 127.0.0.1 0
	 */

	/* set a default routing mask */
	bzero (&zmask, sizeof(zmask));

	if ((bootp.have & BOOT_GATEIP) || (gw = getenv ("gateway"))) {
		/* install default route via the gateway */
		struct sockaddr dst;
		struct sockaddr gate;

		setsin (SIN(dst), AF_INET, INADDR_ANY);
		if (bootp.have & BOOT_GATEIP) {
			setsin (SIN(gate), AF_INET, bootp.gateip.s_addr);
		} else {
			/* try for a DNS lookup */
			struct hostent *hp;
			u_long addr = -1;
			if ((hp = gethostbyname (gw)) && hp->h_addrtype == AF_INET) {
				bcopy (hp->h_addr, (void *)&addr, sizeof(u_long));
			}
			setsin (SIN(gate), AF_INET, addr);
		}

		if (SIN(gate)->sin_addr.s_addr != (u_long)-1) {
			errno = rtrequest (RTM_ADD, &dst, &gate, &zmask,
			       RTF_UP | RTF_GATEWAY, (struct rtentry **)0);
			if (errno) {
				perror ("route add gateway");
			}
		} else {
			fprintf (stderr, "gateway %s: unknown host\n", gw);
		}
	}

	/*
	 * Route packets with local address via loopback i/f.
	 */
	errno = rtrequest (RTM_ADD, &local, &loop, &zmask, RTF_UP | RTF_HOST,
		       (struct rtentry **)0);
	if (errno && errno != EEXIST) {
		perror ("route add loopback");
	}

	close (s);
	return (1);
}


static int
ia_getenv (const char *name, struct in_addr *ia)
{
	char *s;
	if ((s = getenv (name)) != 0)
		return inet_aton (s, ia);
	return 0;
}


static void
boot_getenv (struct bootparams *bootp, char *ifname)
{
	/* these are the only ones that we have to know in this module */

	if (!bootp) {
	  printf("Bootp is null!\n");
	}

	if (!(bootp->have & BOOT_ADDR) && ia_getenv("ipaddr", &bootp->addr))
		bootp->have |= BOOT_ADDR;

	if (!(bootp->have & BOOT_MASK) && ia_getenv("netmask", &bootp->mask))
		bootp->have |= BOOT_MASK;

	if (!(bootp->have & BOOT_BROADADDR) && ia_getenv("broadcast", &bootp->broadaddr))
		bootp->have |= BOOT_BROADADDR;

	if (!(bootp->have & BOOT_GATEIP) && ia_getenv("gateway", &bootp->gateip))
		bootp->have |= BOOT_GATEIP;
}

static int
ia_getcfg (char *cfg, char **ncfg, struct in_addr *ia)
{
	char *s;

	s = strchr(cfg, ':');
	if (s == NULL)
		s = cfg + strlen(cfg);
	else
		*s++ = '\0';
	*ncfg = s;
	if ((s - cfg) <= 1)
		return(0);
	return(inet_aton(cfg, ia));
}

static void
boot_parsecfg (struct bootparams *bootp, char *cfg)
{
	char *ncfg;

	if (ia_getcfg(cfg, &ncfg, &bootp->addr))
		bootp->have |= BOOT_ADDR;
	cfg = ncfg;

	if (ia_getcfg (cfg, &ncfg, &bootp->mask))
		bootp->have |= BOOT_MASK;
	cfg = ncfg;

	if (ia_getcfg(cfg, &ncfg, &bootp->broadaddr))
		bootp->have |= BOOT_BROADADDR;
	cfg = ncfg;

	if (ia_getcfg (cfg, &ncfg, &bootp->gateip))
		bootp->have |= BOOT_GATEIP;
}

static void
__setenv (char *name, char *val, int level)
{
	if (level <= 1 && getenv(name))
		return;
	if (level >= 3)
		/* write to non-volatile environment */
		do_setenv (name, val, 0);
	else
		/* write to local environment only */
		do_setenv (name, val, 1);
}


static void
ia_setenv (char *name, char *ifname, struct in_addr addr, int level)
{
	char sn[30];
	char *s = inet_ntoa (addr);

	if (ifname)
		sprintf(sn, "%s.%s", ifname, name);
	else
		sprintf(sn, "%s", name);
	__setenv (sn, s, level);
}


static void
boot_setenv (struct bootparams *bootp, char *ifname, int level)
{
	if (bootp->have & BOOT_ADDR)
		ia_setenv ("ipaddr", ifname, bootp->addr, level);

	if (bootp->have & BOOT_MASK)
		ia_setenv ("netmask", ifname, bootp->mask, level);

	if (bootp->have & BOOT_BROADADDR)
		ia_setenv ("broadcast", ifname, bootp->broadaddr, level);

	if (bootp->have & BOOT_GATEIP)
		ia_setenv ("gateway", ifname, bootp->gateip, level);

	if (bootp->have & BOOT_DNSIP)
		ia_setenv ("nameserver", NULL, bootp->dnsip, level);

	if (bootp->have & BOOT_BOOTIP)
		ia_setenv ("bootaddr", NULL, bootp->bootip, level);

	if (bootp->have & BOOT_HOSTNAME)
		__setenv ("hostname", bootp->hostname, level);

	if (bootp->have & BOOT_DOMAINNAME)
		__setenv ("localdomain", bootp->domainname, level);

	if (bootp->have & BOOT_BOOTFILE)
		__setenv ("bootfile", bootp->bootfile, level);
}
