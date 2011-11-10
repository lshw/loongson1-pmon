/*                               -*- Mode: C -*- 
 * 
 * File            : bootkernel.c
 * Program/Library : RAYS Packages Manager
 * Description     : 
 * Created: Thu Apr  3 15:48:18 2008
 * Author: WeiPing Zhu
 * Mail            : weiping.zhu@sw-linux.com
 * Last Modified By : WeiPing Zhu
 * Last Modified On : Mon Apr 14 15:04:01 2008 (28800 CST)
 * Update Count : 51
 * 
 *    Copyright 2007 Sun Wah Linux Limited
 *    addr: New World Center, No.88 Zhujiang Road Nanjing, Jiangsu Province, 210018, China
 *    tel : 
 *    fax : 
 * 
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/endian.h>

#include <pmon.h>
#include <exec.h>
#include <pmon/loaders/loadfn.h>

#ifdef __mips__
#include <machine/cpu.h>
#endif

extern int errno;                       /* global error number */
extern char *heaptop;

int boot_kernel(const char* path, int flags, void* flashaddr, unsigned int offset)
{
	int	bootfd;
	int	bootbigend;

	char buf[DLREC+1];
	long ep;
	int n;
#ifdef HAVE_FLASH
	size_t	    flashsize;
#endif

	if ((bootfd = open (path, O_RDONLY | O_NONBLOCK)) < 0) {
		perror (path);
		return -1;
	}

#ifdef HAVE_FLASH
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
#endif
	dl_initialise (offset, flags);

	fprintf (stderr, "Loading file: %s ", path);
	errno = 0;
	n = 0;

	if (flags & RFLAG) {
	   ExecId id;

	   id = getExec("bin");
	   if (id != NULL) {
		   ep = exec (id, bootfd, buf, &n, flags);
	   }
	} else {
		ep = exec (NULL, bootfd, buf, &n, flags);
	}

	close (bootfd);
	putc ('\n', stderr);

	if (ep == -1) {
		fprintf (stderr, "%s: boot failed\n", path);
		return -3;
	}

	if (ep == -2) {
		fprintf (stderr, "%s: invalid file format\n", path);
		return -4;
	}

	if (!(flags & (FFLAG|YFLAG))) {
		printf ("Entry address is %08x\n", ep);
		/* Flush caches if they are enabled */
		if (md_cachestat())
			flush_cache (DCACHE | ICACHE, NULL);
		md_setpc(NULL, ep);
		if (!(flags & SFLAG)) {
		    dl_setloadsyms ();
		}
	}
#ifdef HAVE_FLASH
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
#endif
   	return 0;
}

static	unsigned int rd_start;
static	unsigned int rd_size;
static	int execed;

int boot_initrd(const char* path, int rdstart,int flags)
{
	char buf[DLREC+1] = {0};
	int bootfd;
	int n = 0;
	ExecId id;
	
	rd_start = rdstart;
	rd_size = 0;
	
	printf("Loading initrd image %s", path);
	
	if ((bootfd = open (path, O_RDONLY | O_NONBLOCK)) < 0) {
		perror (path);
		return -1;
	}

	dl_initialise (rd_start, flags);
	
	id = getExec("bin");
	if (id != NULL) {
		exec (id, bootfd, buf, &n, flags);
		rd_size = (dl_maxaddr - dl_minaddr);
		execed = 1;
	}else{
		printf("[error] this pmon can't load bin file!");
		return -2;
	}
	close(bootfd);
#ifdef INITRD_DEBUG
	printf("rd_start %x, rd_size %x\n", rd_start, rd_size);
#endif
	return 0;
}

int initrd_execed(void)
{
	return execed;
}

unsigned int get_initrd_start(void)
{
	return 	rd_start;
}

unsigned int get_initrd_size(void)
{
	return rd_size;
}

