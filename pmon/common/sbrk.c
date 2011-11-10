/* $Id: sbrk.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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
#include <pmon.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>


extern char     end[];
char           *allocp1 = end;
char           *heaptop = end + 65536;

int
chg_heaptop (name, value)
	char *name, *value;
{
	u_int32_t top;

	if (atob (&top, value, 16)) {
#ifdef HEAP_ALLOC_UPWORD
		if(top < (u_int32_t)allocp1) {
			printf ("%x: heap is already above this point\n", top);
			return 0;
		}
#else
		if(top <heaptop) {
			printf ("%x:  memory between %x-%x  is already been allocated,heap is already above this point\n", top,allocp1,heaptop);
			return 0;
		}
#endif
		heaptop = (char *) top;
		return 1;
	}
	printf ("%s: invalid address\n", value);
	return 0;
}


char *
sbrk (n)
	int n;
{
	char *top;

	top = heaptop;
	if (!top) {
		top = (char *) CLIENTPC;
	}
#ifdef HEAP_ALLOC_UPWORD
	if ((allocp1 + n) <= top) {
		allocp1 += n;
		return (allocp1 - n);
	}
#else
		allocp1 -= n;
		return allocp1;
#endif

	return (0);
}
static void init_heaptop __P((void)) __attribute__ ((constructor));

void init_heaptop()
{
#ifndef OLDSBRK
		if (memorysize >= 0x4000000) {
			allocp1 = 0x82000000;
			heaptop = 0x83000000;
		} else 
#endif
    heaptop = (unsigned int)(end + 65536)<CLIENTPC?CLIENTPC:(end + 65536);
#ifndef HEAP_ALLOC_UPWORD
			allocp1=heaptop;
#endif
}

