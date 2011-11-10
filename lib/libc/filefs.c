/* $Id: filefs.c,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */
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
#include <file.h>

File _file[OPEN_MAX] =
{
	{1}, /* stdin */
	{1}, /* stdout */
	{1},  /* stderr */
	{1},  /* kbdin */
	{1}  /* vgaout */
};

SLIST_HEAD(FileSystems, FileSystem) FileSystems = SLIST_HEAD_INITIALIZER(FileSystems);

int
filefs_init(FileSystem *fs)
{
	SLIST_INSERT_HEAD(&FileSystems, fs, i_next);
	return(0);
}

/*
 * Return a string with supported file formats
 * Example "ram, net, disk"
 */
const char *
getFSString()
{
static char buffer[80];
	FileSystem *p;
	int i = 0;

	buffer[0] = '\000';

	SLIST_FOREACH(p, &FileSystems, i_next) {
		if (i != 0) {
			strcat(buffer, ", ");
		}
		strcat(buffer, p->devname);
		i++;
	}
	return(buffer);
}
