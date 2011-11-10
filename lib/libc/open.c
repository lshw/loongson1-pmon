/* $Id: open.c,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */

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

#include <stdio.h>
#include <string.h>
#include <file.h>
#include <fcntl.h>
#include <errno.h>

extern SLIST_HEAD(FileSystems, FileSystem) FileSystems;

static int __try_open(const char *, int, char *, int, int);

int
open(filename, mode)
	const char *filename;
	int mode;
{
	char 	*fname;
	char    *dname;
	int     lu, i;
	int 	fnamelen;
	
	fnamelen = strlen(filename);
	fname = malloc(fnamelen+1);
	memcpy(fname, filename, fnamelen+1);

	for (lu = 0; lu < OPEN_MAX && _file[lu].valid; lu++);

	if (lu == OPEN_MAX) {
		errno = EMFILE;
		free(fname);
		return -1;
	}
		_file[lu].valid = 1;
	
	dname = (char *)fname;
	if (strncmp (dname, "/dev/", 5) == 0) {
		dname += 5;
		i = __try_open(fname, mode, dname, lu, 0);
		free(fname);
		if(i==-1)_file[lu].valid = 0;
		return i;
#ifdef never
		if (i < 0) {
			dname = "disk";
			i = __try_open(fname, mode, dname, lu, 0);
		}
		if (i < 0) {
			i = __try_open(fname, mode, NULL, lu, FS_FILE);
		}
#endif
	}
	else if (strpat(dname, "*ftp://*")) {
		i = __try_open(fname, mode, "net", lu, 0);
	}
	else if (strpat(dname, "file://*")) {
		dname += 6;
		i = __try_open(dname, mode, NULL, lu, FS_FILE);
	}
	else if(strpat(dname, "http://*"))
	{
		i = __try_open(dname, mode, "net", lu, 0);
	}
    else if(strpat(dname, "nfs://*"))
    {
        i = __try_open(dname, mode, "net", lu, 0);
    }
	else {
		i = __try_open(fname, mode, dname, lu, 0);
	}
	free(fname);
	if(i==-1)_file[lu].valid = 0;
	return i;
}

static int
__try_open(const char *fname, int mode, char *dname, int lu, int fstype)
{
	FileSystem *fsp;

	SLIST_FOREACH(fsp, &FileSystems, i_next) {
		if (dname && strbequ(dname, fsp->devname)) {
			if(fsp->open && (*fsp->open)(lu, fname, mode, 0) != lu)
				return -1;	/* Error? */
			break;
		}
		else if (fstype != FS_NULL && fsp->fstype == fstype) {
			if(fsp->open && (*fsp->open)(lu, fname, mode, 0) == lu)
				break;
		}
	}

	if (fsp) {
		/* Opened */
		_file[lu].valid = 1;
		_file[lu].posn = 0;
		_file[lu].fs = fsp;
		return(lu);
	}
	else {
		errno = ENOENT;
		return -1;
	}
}
