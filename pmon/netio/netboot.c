/* $Id: netboot.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */
/*
 * Copyright (c) 2002 Opsycon AB  (www.opsycon.se)
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
 *	This product includes software developed by Opsycon AB.
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
/*  This code is based on code made freely available by Algorithmics, UK */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/endian.h>

#include "netio.h"
#include <pmon.h>
#include <pmon/loaders/loadfn.h>

#ifdef __mips__
#include <machine/cpu.h>
#endif

extern int errno;                       /* global error number */
extern char *heaptop;

static char	*bootpath;
static int	bootfd;
static int	bootbigend;

static long load_s3 __P((int fd, char *, int *, int));
static long load_fast __P((int fd, char *, int *, int));

int cmd_net_boot __P((int, char *[]));
static int net_boot __P((int, char **));

/* read up to '\n' skip any cr */
static int
fdgets (int fd, char *buf, int *n, int maxn)
{
	int nb = 0;

	for (;;) {
		if (read (fd, buf, 1) != 1)
			break;
		if (*buf == '\n')
			break;
		buf++;
		if (++nb >= maxn) {
			break;
		}
	}
	*buf = '\0';
	if (n)
		*n += nb;
	return nb;
}


static long
load_xx (int fd, char *buf, int *n, int flags, char *rectype, char recid,
	 int (*fn)(char *, int, int *, int))
{
	int s, len, nb;
	char *p = NULL;
	int left;

	/* Check if anything to test on, else read max DLREC chars */
	if (*n == 0) {
		if (fdgets (fd, buf, n, DLREC) < 0) {
			    return -1;
		}
	}

	if (*buf != recid) {
		return (-1);
	}

	fprintf (stderr, "(%s)\n", rectype);
	left = *n;

	do {
		nb = left;
		if (left == 0) {
			/* Buffer empty, get next line */
			if (fdgets (fd, buf, &nb, DLREC - nb) < 0) {
				return -1;
			}
		} else if ((p = strnchr (buf, '\n', left)) != 0) {
			/* Use complete line already in buffer */
			*p++ = '\0';
			nb = p - buf - 1;
			left -= nb + 1;
		} else {
			/* Fill up line if not complete */
			if (fdgets (fd, buf + nb, &nb, DLREC - nb) < 0) {
				return -1;
			}
			left = 0;
		}

		if (buf[nb-1] == '\r') {
			buf[--nb] = '\0';
		}
		s = (*fn)(buf, nb, &len, flags);

		if (left) {
			memcpy (buf, p, left);
		}
	} while (s == DL_CONT);

	if (s != DL_DONE) {
		fprintf(stderr, "load error: %s\n", dl_err (s));
		return (-2);
	} else {
		return (dl_entry);
	}
}

static long
load_s3 (int fd, char *buf, int *n, int flags)
{
    return load_xx (fd, buf, n, flags, "s3", 'S', dl_s3load);
}

static long
load_fast (int fd, char *buf, int *n, int flags)
{
    return load_xx (fd, buf, n, flags, "fast", '/', dl_fastload);
}


static long (* const loaders[]) __P((int, char *, int *, int)) = {
    load_elf,
    load_s3,
    load_fast,
    0
};

/* ------------------------------------------------------- */

const Optdesc         cmd_net_boot_opts[] =
{
    {"-s", "don't clear old symbols"},
    {"-b", "don't clear breakpoints"},
    {"-e", "don't clear exception handlers"},
    {"-a", "don't add offset to symbols"},
    {"-t", "load at top of memory"},
    {"-i", "ignore checksum errors"},
    {"-f<addr>", "load into flash"},
    {"-u", "upgrade boot flash"},
    {"-n", "don't load symbols"},
    {"-y", "only load symbols"},
    {"-v", "verbose messages"},
    {"-w", "reverse endianness"},
    {"-k", "prepare for kernel symbols"},
    {"-o<offs>", "load offset"},
    {"host:path", "internet host name, and file name"},

    {0}
};


static int
net_boot (argc, argv)
	int argc;
	char **argv;
{
/*	char *host, *file;
	int hostlen;*/
	char path[256];
	char buf[DLREC+1];
	long ep;
	int i, n;
	extern int optind;
	extern char *optarg;
	int c, err;
	int flags;
	unsigned long offset;
	void	    *flashaddr;
	size_t	    flashsize;

	flags = 0;
	optind = err = 0;
	offset = 0;
	while ((c = getopt (argc, argv, "sbeatif:nuvwyko:")) != EOF) {
		switch (c) {
		case 's':
			flags |= SFLAG; break;
		case 'b':
			flags |= BFLAG; break;
		case 'e':
			flags |= EFLAG; break;
		case 'a':
			flags |= AFLAG; break;
		case 't':
			flags |= TFLAG; break;
		case 'i':
			flags |= IFLAG; break;
		case 'f':
			if (!get_rsa ((u_int32_t *)&flashaddr, optarg)) {
				err++;
			}
			flags |= FFLAG; break;
#if notyet
		case 'u':
			flashaddr = (void *)BOOTROMBASE;
			flags |= UFLAG;
			flags |= FFLAG; break;
#endif
		case 'n':
			flags |= NFLAG; break;
		case 'y':
			flags |= YFLAG; break;
		case 'v':
			flags |= VFLAG; break;
		case 'w':
			flags |= WFLAG; break;
		case 'k':
			flags |= KFLAG; break;
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

	if (err || optind < argc - 1) {
		return EXIT_FAILURE;
	}
#if 1
	if (optind < argc) {
		strcpy(path, argv[optind]);
	} 
#else
	if (optind < argc) {
		if ((file = strchr (argv[optind], ':')) != 0) {
			host = argv[optind];
			hostlen = file++ - host;
		} else {
			host = NULL;
			file = argv[optind];
		}
	} else {
		host = file = NULL;
	}

	if (!host || !*host) {
		host = getenv ("bootaddr");
		if (!host) {
			fprintf (stderr, "missing host address and $bootaddr not set\n");
			return EXIT_FAILURE;
		}
		hostlen = strlen (host);
	}

	if (!file || !*file) {
		file = getenv ("bootfile");
		if (!file) {
			fprintf (stderr, "missing file name and $bootfile not set\n");
			return EXIT_FAILURE;
		}
	}

	if (hostlen + strlen(file) + 2 > sizeof(path)) {
		fprintf (stderr, "remote pathname too long\n");
		return EXIT_FAILURE;
	}
	memcpy (path, host, hostlen);
	path[hostlen++] = ':';
	strcpy (&path[hostlen], file);
#endif	
	if ((bootfd = open (path, O_RDONLY | O_NONBLOCK)) < 0) {
		perror (path);
		return EXIT_FAILURE;
	}

	bootpath = path;

	if (flags & FFLAG) {
		tgt_flashinfo (flashaddr, &flashsize);
		if (flashsize == 0) {
			printf ("No FLASH at given address\n");
			return 0;
		}
		/* any loaded program will be trashed... */
		flags &= ~(SFLAG | BFLAG | EFLAG);
		flags |= NFLAG;		/* don't bother with symbols */
		/*
		 * Recalculate any offset given on command line.
		 * Addresses should be 0 based, so a given offset should be
		 * the actual load address of the file.
		 */
		offset = (unsigned long)heaptop - offset;
#if BYTE_ORDER == LITTLE_ENDIAN
		bootbigend = 0;
#else
		bootbigend = 1;
#endif
	}

	dl_initialise (offset, flags);

	fprintf (stderr, "Loading file: %s ", bootpath);
	errno = 0;
	n = 0; 
	for (i = 0; loaders[i]; i++) {
		if ((ep = (*loaders[i]) (bootfd, buf, &n, flags)) != -1) {
			break;
		}
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

	if (!(flags & (FFLAG|YFLAG))) {
		printf ("Entry address is %08x\n", ep);
		flush_cache (DCACHE | ICACHE, NULL);
		md_setpc(ep);
		if (!(flags & SFLAG)) {
		    dl_setloadsyms ();
		}
	}
	if (flags & FFLAG) {
		extern long dl_minaddr;
		extern long dl_maxaddr;
		if (flags & WFLAG)
			bootbigend = !bootbigend;
		tgt_flashprogram ((void *)flashaddr, 	   	/* address */
				dl_maxaddr - dl_minaddr, 	/* size */
				(void *)heaptop,		/* load */
				bootbigend);
	}
	return EXIT_SUCCESS;
}

int
cmd_net_boot (argc, argv)
    int argc; char **argv;
{
    int ret;
    ret = spawn ("boot", net_boot, argc, argv);
    return (ret & ~0xff) ? 1 : (signed char)ret;
}
