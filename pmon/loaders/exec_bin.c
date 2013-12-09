/* $Id: exec_bin.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2000 Opsycon AB  (www.opsycon.se)
 * Copyright (c) 2002 Patrik Lindergren (www.lindergren.com)
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
 *	This product includes software developed by Patrik Lindergren.
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

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <exec.h>

#include <pmon.h>
#include <pmon/loaders/loadfn.h>

extern long dl_minaddr;
extern long dl_maxaddr;
extern long long dl_loffset;
extern int highmemcpy(long long dst, long long src, long long count);

static long load_bin(int fd, char *buf, int *n, int flags)
{
	unsigned long long addr;
	int size = 2048;
	int n2;
	int count = 0;

	if (flags & OFLAG)
		addr = dl_loffset;
	else
		addr = (void *)dl_offset;

	fprintf (stderr, "(bin)\n");

	dl_minaddr = (long)addr;

	do {
		if (flags & OFLAG) {
			n2 = read(fd, buf, size);
			highmemcpy((long long)addr, (long long)buf, (long long)n2);
		}
		else 
			n2 = read(fd, addr, size);
		addr = (addr + n2);
		count += n2;
	} while (n2 >= size);

	dl_maxaddr = (long)addr;
	printf("\nLoaded %d bytes\n", count);

	return dl_offset;
}


static ExecType bin_exec = {
	"bin",
	load_bin,
	EXECFLAGS_NOAUTO,
};


static void init_exec __P((void)) __attribute__ ((constructor));

static void init_exec(void)
{
	/*
	 * Install ram based file system.
	 */
	exec_init(&bin_exec);
}

