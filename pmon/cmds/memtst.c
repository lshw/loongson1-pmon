/* $Id: memtst.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2001 Opsycon AB  (www.opsycon.se / www.opsycon.com)
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

#include <stdio.h>
#include <stdlib.h>
#include <termio.h>
#ifdef _KERNEL
#undef _KERNEL
#endif
#include <sys/ioctl.h>

#include <pmon.h>

int cmd_memtst __P((int, char *[]));
static int do_mt __P((u_int32_t *, u_int32_t *, int));

const Optdesc         mt_opts[] = {
	{"-c", "continuous test"},
	{"-v", "verbose"},
	{0}
};

static const Cmd MtestCmd[] =
{
	{"Memory"},
	{"mt",		"[-cv] [start [end]]",
			mt_opts,
			"simple memory test",
			cmd_memtst,
			1, 4, CMD_REPEAT},
	{0, 0}
};

static void init_cmd_mtest __P((void)) __attribute__ ((constructor));

static void
init_cmd_mtest()
{
	cmdlist_expand(MtestCmd, 1);
}

extern char           *heaptop;
int
cmd_memtst(int ac, char **av)
{
	int cflag, vflag, err;
	u_int32_t saddr, eaddr;
	int c;

	cflag = 0;
	vflag = 0;

	saddr = heaptop;
#if defined(__mips__)
        eaddr = (memorysize & 0x3fffffff) + ((int)heaptop & 0xc0000000);
#else
	eaddr = memorysize;
#endif

	optind = 0;
	while((c = getopt(ac, av, "cv")) != EOF) {
		switch(c) {
		case 'v':
			vflag = 1;
			break;
		case 'c':
			cflag = 1;
			break;
		default:
			return(-1);
		}
	}

	if(optind < ac) {
		if (!get_rsa(&saddr, av[optind++])) {
			return(-1);
		}
		if(optind < ac) {
			if(!get_rsa(&eaddr, av[optind++])) {
				return (-1);
			}
		}
	}

	saddr = saddr & ~0x3;
	eaddr &= ~0x3;

	if(eaddr < saddr) {
		printf("end address less that start address!\n");
		return(-1);
	}

	ioctl (STDIN, CBREAK, NULL);

	printf ("Testing %08x to %08x %s\n",
			saddr, eaddr, (cflag) ? "continuous" : "");

	if (cflag) {
		while(!(err = do_mt((u_int32_t *)saddr, (u_int32_t *)eaddr, vflag)));
	}
	else {
		err = do_mt((u_int32_t *)saddr, (u_int32_t *)eaddr, vflag);
	}

	if (err) {
		printf ("There were %d errors.\n", err);
		return (1);
	}
	printf ("Test passed ok.\n");
	return (0);
}

static int
do_mt(u_int32_t *saddr, u_int32_t *eaddr, int vflag)
{
	int i, j, err, temp, siz;
	unsigned int w, *p, r;

	err = 0;
	siz = eaddr - saddr;

	/* walking ones test */
	if(vflag) {
		printf("Walking ones - %p  ", saddr);
	}
	else {
		printf("Testing...  ");
	}
	for (p = saddr, i = 0; i < siz; i++, p++) {
		w = 1;
		for (j = 0; j < 32; j++) {
			*p = w;
			temp = 0;
			r = *p;
			if (r != w) {
				err++;
				printf ("\b\nerror: addr=%08x read=%08x expected=%08x  ", p, r, w);
			}
			w <<= 1;
			dotik (1, 0);
		}
		if(vflag && ((int)p & 0xffff) == 0) {
			printf("\b\b\b\b\b\b\b\b\b\b\b\b%p  ", p);
		}
	}
	if(vflag) {
		printf("\b\b\b\b\b\b\b\b\b\b\b\bDone.          \n");
	}

	/* store the address in each address */
	if(vflag) {
		printf("Address test - %p  ", saddr);
	}
	for (p = saddr, i = 0; i < siz; i++, p++) {
		*p = (unsigned int)p;
		dotik (1, 0);
		if(vflag && ((int)p & 0xffff) == 0) {
			printf("\b\b\b\b\b\b\b\b\b\b\b\b%p  ", p);
		}
	}

	/* check that each address contains its address */
	for (p = saddr, i = 0; i < siz; i++, p++) {
		r = *p;
		if (r != (unsigned int)p) {
			err++;
			printf ("\b\nerror: adr=%08x read=%08x expected=%08x  ", p, r, p);
		}
		dotik (1, 0);
		if(vflag && ((int)p & 0xffff) == 0) {
			printf("\b\b\b\b\b\b\b\b\b\b\b\b%p  ", p);
		}
	}
	if(vflag) {
		printf("\b\b\b\b\b\b\b\b\b\b\b\bDone.          \n");
	}
	return (err);
}
