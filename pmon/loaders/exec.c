/* $Id: exec.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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
 *	This product includes software developed by Opsycon AB.
 *	This product includes software developed by Patrik Lindergren.
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
#include <string.h>
#include <exec.h>

SLIST_HEAD(ExecTypes, ExecType) ExecTypes = SLIST_HEAD_INITIALIZER(ExecTypes);

int
exec_init(ExecType *exec)
{
	SLIST_INSERT_HEAD(&ExecTypes, exec, i_next);
	return(0);
}

/*
 *  Try to load the input stream. If id is NULL probe and try
 *  all autodetectable formats else use the specified format.
 *
 *  buf should be a pointer to a DLREC+1 size buffer.
 *  n   should be a pointer to an int saying how much we have
 *      read from the input stream so far.
 */
long
exec (ExecId id, int fd, char *buf, int *n, int flags)
{
	ExecType *p;
	long ep;

	if (id == NULL) {
		SLIST_FOREACH(p, &ExecTypes, i_next) {
			if (p->flags != EXECFLAGS_NOAUTO) {
				if ((ep = (*p->loader) (fd, buf, n, flags)) != -1) {
					break;
				}
			}
		}
	} else {
		SLIST_FOREACH(p, &ExecTypes, i_next) {
			if (((ExecType *)id) == p) {
				ep = (*p->loader) (fd, buf, n, flags);
				break;
			}
		}	
	}
	return (ep);
}

/*
 * Return an Id to the asked for exec type
 */
ExecId getExec(char *execname)
{
	ExecType *p;
	ExecId id = NULL;

	SLIST_FOREACH(p, &ExecTypes, i_next) {
		if(strcmp(p->execname, execname) == 0) {
			id = (ExecId)p;
		}
	}
	return (id);
}


/*
 * Return a string with supported executable types
 * Example "elf, srec, gz"
 */
const char *getExecString()
{
	static char buffer[80];
	ExecType *p;
	int i = 0;

	buffer[0] = '\000';
   
	SLIST_FOREACH(p, &ExecTypes, i_next) {
		if (i != 0) {
			strcat(buffer, ", ");
		}
		strcat(buffer, p->execname);
		i++;
	}
	return(buffer);
}
