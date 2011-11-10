/*	$Id: boot.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2000 Opsycon AB  (http://www.opsycon.se)
 * Copyright (c) 2000 RTMX, Inc   (http://www.rtmx.com)
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <setjmp.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/endian.h>

#include <pmon.h>
#include <debugger.h>
#include <exec.h>
#include <pmon/loaders/loadfn.h>

int cmd_boot (int, char *[]);


const Optdesc         cmd_boot_opts[] =
{
	{"-s", "don't clear old symbols"},
	{"-b", "don't clear breakpoints"},
	{"-r", "load raw file"},
	{"-o<offs>", "load offset"},
	{"-e<entry>", "entry address"},
	{"-k", "prepare for kernel symbols"},
	{"path", "path and filename"},
	{0}
};

extern int errno;                       /* global error number */
extern char *heaptop;
extern jmp_buf go_return_jump;

static int	bootfd;

/*
 *  This is supposed to be a wrapper around the various boot
 *  commands that are provided.
 */

int
cmd_boot(int argc, char **argv)
{
	char path[256];
	char buf[DLREC+1];
	extern char  clientcmd[];	/* in go.c */
	extern char *clientav[];	/* in go.c */
	extern int   clientac;		/* in go.c */
	extern int optind;
	extern char *optarg;
	int c, err;
	long ep;
	int n;
	int flags;
	unsigned long offset = 0;
	unsigned long entry = 0;

	flags = 0;
	optind = err = 0;
	offset = 0;
	while ((c = getopt (argc, argv, "sbke:ro:")) != EOF) {
		switch (c) {
			case 's':
				flags |= SFLAG; break;
			case 'b':
				flags |= BFLAG; break;
			case 'k':
				flags |= KFLAG; break;
			case 'r':
				flags |= RFLAG; break;
			case 'e':
				if (!get_rsa ((u_int32_t *)&entry, optarg)) {
					err++;
				}
				break;
			case 'o':
				if (!get_rsa ((u_int32_t *)&offset, optarg)) {
					err++;
				}
				break;
			default:
				err++;
				break;
		}
	}

	if (err) {
		return EXIT_FAILURE;
	}

	if (optind < argc) {
		strcpy(path, argv[optind++]);
	} 
	else if (getenv("bootfile")) {
		strcpy(path, getenv("bootfile"));
	}
	else {
		printf("boot what?\n");
		return EXIT_FAILURE;
	}

	if ((bootfd = open (path, O_RDONLY | O_NONBLOCK)) < 0) {
		perror (path);
		return EXIT_FAILURE;
	}

	dl_initialise (offset, flags);

	fprintf (stderr, "Loading file: %s ", path);
	errno = 0;
	n = 0;
	if (flags & RFLAG) {
		ExecId id;

		id = getExec("bin");
		if (id != NULL) {
			ep = exec (id, bootfd, buf, &n, flags);
		} else {
			perror ("Can't find binary loader");
			return EXIT_FAILURE;
		}
	} else {
		ep = exec (NULL, bootfd, buf, &n, flags);
	}

	close (bootfd);
	putc ('\n', stderr);

	if (ep == -1) {
		fprintf (stderr, "%s: boot failed\n", path);
		return EXIT_FAILURE;
	}

	if (ep == -2) {
		fprintf (stderr, "%s: invalid file format\n", path);
		return EXIT_FAILURE;
	}

	if (entry)
		dl_entry = entry;
	else
		dl_entry = ep;

	sprintf(clientcmd, "boot %s ", path);
	while (optind < argc) {
		strcat(clientcmd, argv[optind++]);
		strcat(clientcmd, " ");
	}

	md_setentry(NULL, (register_t)(int)dl_entry); /* set start address */
	clientac = argvize (clientav, clientcmd);
	initstack (clientac, clientav, 1);
	clrhndlrs ();
	closelst (2);		/* Init client terminal state */
	md_setsr(NULL, initial_sr);
	tgt_enable(tgt_getmachtype ()); /* set up i/u hardware */

#ifdef __powerpc__
	if(getenv("vxWorks")) {
		strcpy ((void *)0x4200, getenv ("vxWorks"));
	}
#endif

	/* Flush caches if they are enabled */
	if (md_cachestat())
		flush_cache (DCACHE | ICACHE, NULL);

	if (setjmp (go_return_jump) == 0) {
		goclient ();
	}

	console_state(1);
	return 0;
}

/*
 *  Command table registration
 *  ==========================
 */

static const Cmd Cmds[] =
{
	{"Boot and Load"},
	{"boot",	"[-bsr][-o offs][-e entry]", cmd_boot_opts,
			"boot",
			cmd_boot, 1, 99, 0},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
