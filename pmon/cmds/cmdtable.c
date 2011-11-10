/* $Id: cmdtable.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2001 Opsycon AB  (www.opsycon.se)
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
#include <string.h>

/*
 *  Expandable command list handling
 *  --------------------------------
 *
 *  Now the most elegant solution would be to link commandlists
 *  on a linked list but we must be able to have commandlists in
 *  read only memory and thus they are not writeable.
 */

const Cmd *CmdList[100];

void
cmdlist_expand(newcmdlist, atend)
	const Cmd *newcmdlist;
	int atend;
{
	int i;

	for(i = 0; i < 100; i++) {
		if(CmdList[i] == 0) {
			CmdList[i] = newcmdlist;
			return;
		}

		if (!strcmp(newcmdlist->name, CmdList[i]->name)) {
			break;
		}
	}
	if(atend) {
		while(CmdList[i] && !strcmp(newcmdlist->name,CmdList[i]->name)) {
			i++;
		}
	}
	bcopy((void *)&CmdList[i], (void *)&CmdList[i+1], sizeof(Cmd *) *(99 - i));
	CmdList[i] = newcmdlist;
}
