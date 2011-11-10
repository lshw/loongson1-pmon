/*	$Id: callvec.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2000-2002 Opsycon AB  (www.opsycon.se)
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

#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pmon.h>
unsigned long _pci_make_tag (int, int, int);
void	 _pci_conf_writen(unsigned long, int , unsigned int, int);
unsigned int _pci_conf_readn (unsigned long, int, int);

void cacheflush (void);
int errinval (void);

struct callvectors {
	int	(*open) (const char *, int, ...);
	int	(*close) (int);
	int	(*read) (int, void *, size_t);
	ssize_t	(*write) (int, const void *, size_t);
	off_t	(*lseek) (int, off_t, int);
	int	(*printf) (const char *, ...);
	void	(*flushcache) (void);
	char	*(*gets) (char *);
#if defined(SMP)
	int	(*smpfork) (size_t, char *);
	int	(*semlock) (int);
	void	(*semunlock) (int);
#else
	int	(*smpfork) (void);
	int	(*semlock) (void);
	int	(*semunlock) (void);
#endif
	unsigned int 	(*_pci_conf_readn)(unsigned long, int, int);
	void	(*_pci_conf_writen)(unsigned long, int, unsigned int, int);
	unsigned long 	(*_pci_make_tag) (int, int, int); 
} callvectors = {
	open,
	close,
	read,
	write,
	lseek,
	printf,
	cacheflush,
	gets,
#if defined(SMP)
	tgt_smpfork,
	tgt_semlock,
	tgt_semunlock,
#else
	errinval,
	errinval,
	errinval,
#endif
#ifndef NOPCI
	_pci_conf_readn,
	_pci_conf_writen,
	_pci_make_tag,
#endif
};

struct callvectors *callvec = &callvectors;

/*
 *  Flush cache function to aid program loading.
 */
void
cacheflush()
{
	flush_cache(DCACHE|ICACHE, NULL);
}

/*
 *  Invalid call
 */

int
errinval()
{
	return(-1);
}
