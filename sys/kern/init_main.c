/* $Id: init_main.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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
 *  Parts of this code orginates from Algorithmics UK PMON.
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/endian.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#define SYSLOG_NAMES
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_types.h>

#include <machine/intr.h>

#include <pmon.h>

#if 1
#include <vm/vm.h>
#endif

int sysloglevel;

extern void kmeminit __P((void));
extern void vminit __P((void));
extern void domaininit __P((void));
extern void ifinit __P((void));
extern int ifconfig __P((char *, char *));
extern void loopattach __P((int));

extern char *getenv __P((char *));
extern int strtoul __P((const char *, char **, int));

void init_net __P((int));

void
init_net (int hwok)
{
	char *e;
	int i, s;
	struct pdevinit *pdev;
	vm_offset_t maxaddr;
extern struct pdevinit pdevinit[];
extern vm_map_t mb_map, kernel_map;
    
	sysloglevel = LOG_NOTICE;
	if ((e = getenv ("loglevel")) != 0) {
		char *ee;
		u_long n = strtoul (e, &ee, 0);
		if (*ee == '\0' && n <= LOG_DEBUG) {
			sysloglevel = n;
		}
		else {
			CODE *code;
			for (code = prioritynames; code->c_name; code++) {
				if (strcmp (code->c_name, ee) == 0) {
					break;
				}
			}
			if (code->c_name) {
				sysloglevel = code->c_val;
			}
			else {
				log (LOG_ERR, "bad $loglevel variable\n");
			}
		}
	}

	/*
	 * Init system global parameters
	 */
	paraminit ();

	/*
	 * Initialise "virtual memory" maps
	 */
	vminit();

	/*
	 * Initialise memory allocator
	 */
	kmeminit();

	/*
	 * Initialize callouts
	 */
	callout = malloc(sizeof(struct callout) * ncallout, M_TEMP, M_NOWAIT);
	callfree = callout;
	for (i = 1; i < ncallout; i++) {
		callout[i-1].c_next = &callout[i];
	}

	if (hwok) {
		startrtclock(hz);
	}

	/*
	 * Initialise mbufs
	 */
	mclrefcnt = (char *)malloc(VM_KMEM_SIZE/MCLBYTES, M_MBUF, M_NOWAIT);
	bzero(mclrefcnt, NMBCLUSTERS+CLBYTES/MCLBYTES);
	mb_map = kmem_suballoc(kernel_map, (vm_offset_t *)&mbutl, &maxaddr,
			NMBCLUSTERS*MCLBYTES, FALSE);
	mbinit();

	/*
	 * Initialise network devices and protocols
	 */
	if (hwok) {
		s = splhigh();
		tgt_devconfig();
		for (pdev = pdevinit; pdev->pdev_attach != NULL; pdev++) {
			if (pdev->pdev_count > 0) {
				(*pdev->pdev_attach)(pdev->pdev_count);
			}
		}
		
		ifinit();
		printf("ifinit done.\n");
		domaininit();
		printf("domaininit done.\n");
		splx(s);
	}

	/* 
	 * Initialise process table, we become first "process" 
	 */
	printf("init_proc....\n");
	init_proc ();

	/* enable realtime clock interrupts */
	if (hwok) {
		enablertclock();		
	}

	boottime = time;
	spl0();
	/*delay(1000000);*/
	delay(10000);

#ifdef INET
	if (getenv("ifconfig") != 0) {
		/* configure the default ethernet interface */
		char *ip = getenv("ifconfig");
		char *p, *cp;
		char ifn[100];

		while (*ip) {
			/*
			 * Check for <ifc>:<config>;
			 */
			cp = ifn;
			p = NULL;
			while (*ip != ';' && *ip != '\0') {
				if (*ip == ':' && p == NULL) {
					*cp = '\0';
					p = ++cp;
				} else {
					*cp++ = *ip;
				}
				ip++;
			}
			if (*ip == ';')
				ip++;
			*cp = '\0';

			s = splhigh();
			printf("network configure '%s:%s'\n", ifn, p);
			splx(s);
			ifconfig (ifn, p);
		}
	}
#endif /* INET */
}

