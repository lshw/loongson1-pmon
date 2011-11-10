/*	$Id: disk_boot.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2000 Opsycon AB  (www.opsycon.se)
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
 *	This product includes software developed by
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

#include <stdio.h>
#include <termio.h>
#include <string.h>
#include <fcntl.h>
#include <setjmp.h>
#include <stdlib.h>

#include <pmon.h>

extern int   devio_open __P((char *, int, int));
extern int   devio_close __P((int));
extern int   devio_read __P((int, void *, size_t));
extern int   devio_write __P((int, void *, size_t));
extern off_t devio_lseek __P((int, off_t, int)); 
extern void  initstack __P((int, char **, int));
extern int   goclient __P((void));
int cmd_disk_boot __P((int, char *[]));

const Optdesc         cmd_disk_boot_opts[] = {
    {"-x", "don't exec boot"},
    {"-d <name>", "specify disk device"},
    {"-o <n>", "offset in disk blocks"},
    {"-l <n>", "length in disk blocks"},
    {"-a <addr>", "memory destination"},
    {"-m", "pass 'man' option to Level1 boot"},
    {"-- <args>", "args to be passed to client"},
    {0}
};

jmp_buf gobuf;

/*
 *  cmd_disk_boot(ac,av), the 'disk_boot' command
 */
int
cmd_disk_boot (ac, av)
     int             ac;
     char           *av[];
{
extern char  clientcmd[];	/* in go.c */
extern char *clientav[];	/* in go.c */
extern int   clientac;		/* in go.c */

	int c;
	int fd;
	int xflag = 0;
	int mflag = 0;
	int wflag = 0;
	char *dev = 0;
	char *buf = (char *)TGT_BOOT_ADR;
	int len = TGT_BOOT_SIZ;
	int seek = TGT_BOOT_OFF;

	optind = 0;
	while ((c = getopt (ac, av, "d:l:o:a:xmw")) != EOF) {

		switch(c) {
		case 'd':
			dev = optarg;
			break;

		case 'l':
			len = atoi(optarg) * 512;
			break;

		case 'o':
			seek = atoi(optarg) * 512;
			break;

		case 'a':
			if (!get_rsa ((void *)&buf, optarg))
			return (-1);
			break;

		case 'x':
			xflag = 1;
			break;

		case 'm':
			mflag = 1;
			break;

		case 'w':
			wflag = 1;
			xflag = 1;
			break;

		default:
			return(-1);
		}
	}
	if(dev == 0) {
		if(optind >= ac) {
			dev = getenv("bootfile");
			if(dev == 0) {
				dev = "/dev/sd0/bsd";
			}
			printf("Using default boot file '%s'\n", dev);
		}
		else {
			dev = av[optind++];
			if(optind < ac && strcmp(av[optind], "--") == 0) {
				optind++;
			}
		}
	}

	fd = devio_open(dev, O_RDONLY, 0);
	if(fd < 0) {
		printf("failed to open '%s'\n", dev);
		return(0);
	}

	devio_lseek(fd, seek, 0);

	if(wflag) {
		devio_write(fd, buf, len);
		return(0);
	}
	else if(devio_read(fd, buf, len) != len) {
		printf("boot read failed!\n");
		devio_close(fd);
		return(-1);
	}
	devio_close(fd);
	flush_cache(DCACHE, NULL);
	flush_cache(ICACHE, NULL);

	if(xflag) {
		return(0);
	}

	if(mflag) {
		sprintf(clientcmd, "boot man ");
	}
	else {
		sprintf(clientcmd, "boot %s ", dev);
	}

	while (optind < ac) {
		strcat (clientcmd, av[optind++]);
		strcat (clientcmd, " ");
	}

	/* XXX Skip over any ZERO region in the boot block. */
	/* XXX This is to take care of zeroed out headers.  */
	while(*(unsigned int *)buf == 0) {
		((unsigned int *)buf)++;
	}

	md_setentry((register_t)(int)buf); /* set start address */
	clientac = argvize (clientav, clientcmd);
	initstack (clientac, clientav, 1);
	clrhndlrs ();
	closelst (2);
	md_setsr(initial_sr);
	tgt_enable(tgt_getmachtype ()); /* set up i/u hardware */
#ifdef FLOATINGPT
	Fcr = 0;            /* clear any outstanding exceptions / enables */
#endif

#ifdef __powerpc__
        if(getenv("vxWorks")) {
                strcpy ((void *)0x4200, getenv ("vxWorks"));
        }
#endif

	if (setjmp (gobuf) == 0) {
		goclient ();
	}
	console_state(1);
	return 0;
}
