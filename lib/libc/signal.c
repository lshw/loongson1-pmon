/* $Id: signal.c,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */

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
#include <termio.h>
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/ioctl.h>

/*
 * This is a somewhat skeletal implementation of signal, it is
 * only intended to make basic functionality available.
 */

sig_t	_sigintfunc;
jmp_buf _sigintbuf;
jmp_buf _savedintr;

sig_t
signal(int op, sig_t func)
{
	if (op == SIGINT) {
		if (func == SIG_IGN || func == SIG_DFL) {
			errno = EINVAL;
			return (SIG_ERR);
		}
		else {
			ioctl (STDIN, GETINTR, _savedintr);
			_sigintfunc = func;
			if (setjmp (_sigintbuf)) {	/* when INTR occurs */
				ioctl (STDIN, SETINTR, _savedintr);
				(*_sigintfunc) (SIGINT);
			}
			else {
				ioctl (STDIN, SETINTR, _sigintbuf);
				return (SIG_DFL);
			}
		}
	}
	errno = EINVAL;
	return(SIG_ERR);
}
