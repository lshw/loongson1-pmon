/*
 * Copyright (c) 2007 SunWah Hi-tech (www.sw-linux.com.cn)
 * Wing Sun <chunyang.sun@sw-linux.com>
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

static long   load_txt (int fd, char *buf, int *n, int flags);

static long
   load_txt (int fd, char *buf, int *n, int flags)
{
	void *addr = (void *)buf;
	int size = 1;
	int n2;
	char ch='\0';
	int count=0;
	if(!n)
		return -1;
	if(!buf)
		return -1;

	do {
		n2 = read (fd, addr, size);
		ch=*((char*)addr);
		addr = ((char *)addr + n2);
		count+=n2;
	} while (n2 >= size && ch != '\n' && count<*n);

	*n=count;

	return(*n);
}


static ExecType txt_exec =
{
	"txt",
	load_txt,
	EXECFLAGS_NOAUTO,
};


static void init_exec __P((void)) __attribute__ ((constructor));

static void
   init_exec()
{
	exec_init(&txt_exec);
}

