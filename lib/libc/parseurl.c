/* $Id: parseurl.c,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */
/*
 * Copyright (c) 2001-2002 Opsycon AB  (www.opsycon.se / www.opsycon.com)
 * Copyright (c) 2001-2002 Patrik Lindergren  (www.lindergren.com)
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
 *	This product includes software developed by Patrik Lindergren, Sweden.
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
#include <stdlib.h>
#include <string.h>
#include <url.h>

/*
 * Parse an url like:
 * "tftp://192.168.4.5/pmon"
 * "ftp://patrik@192.168.4.5/pmon"
 * "ftp://patrik:deadbeaf@sirus.pmon200.org/pmon"
 */
int parseUrl(const char *path, struct Url *url)
{
	char *dname;
	char *buf;
#ifdef __not_yet
	char *buf2;
#endif
	
	dname = (char *)path;

	/*
	 * Parse protocol field
	 */
	if ((buf = strchr (dname, ':')) != NULL) {
		int len = buf - dname;
		if (len + 1 > PROTOCOLSIZE) {
			return (-1);
		}
		strncpy (url->protocol, dname, len);
		url->protocol[len] = 0;
		dname = buf;
	} else {
		return(-1);
	}
	if (strncmp (dname, "://", 3) == 0) {
		dname += 3;
	} else {
		return (-1);
	}

#if 1 //def __not_yet
	/*
	 * Parse username field
	 */
	if ((buf = strchr (dname, '@')) != NULL) {
		int passwdlen;
		int unamelen;
		char *buf2;
		/*
		 * Check if we also provide a password
		 */
		if ((buf2 = strchr (dname, ':')) != 0) {
			passwdlen = buf2 - dname;
			if (passwdlen + 1 > PASSWORDSIZE) {
				return (-1);
			}
			bcopy (url->password, buf2, passwdlen);
			url->password[passwdlen] = 0;
		} else {
			url->password[0] = 0;
			passwdlen = 0;
		}

		unamelen = buf - dname - passwdlen;
		if (unamelen + 1 > USERNAMESIZE) {
			return (-1);
		}
		bcopy (url->username, dname, unamelen);
		url->username[unamelen] = 0;
		if (passwdlen)
			dname += passwdlen + 1;
		dname += unamelen + 1;
		
	} else {
		url->username[0] = 0;
	}
#endif	
	/*
	 * Parse hostname field
	 */
	url->port=0;
	if ((buf = strchr (dname, ':')) != NULL)
	{
		int len = buf - dname;
		if (len + 1 > HOSTNAMESIZE) {
			return (-1);
		}
		strncpy (url->hostname, dname, len);
		url->hostname[len] = 0;
		dname = buf + 1;
	if ((buf = strchr (dname, '/')) != NULL) {
		url->port=strtoul(dname,0,0);
		dname= buf + 1;
	 }
	else return -1;		
	}
	else
	if ((buf = strchr (dname, '/')) != NULL) {
		int len = buf - dname;
		if (len + 1 > HOSTNAMESIZE) {
			return (-1);
		}
		strncpy (url->hostname, dname, len);
		url->hostname[len] = 0;
		dname = buf + 1;
	} 

	/*
	 * Parse Filename field
	 */
	strncpy(url->filename, dname, FILENAMESIZE);
	
	return (0);
}

