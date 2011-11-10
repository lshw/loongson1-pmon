/* $Id: getservent.c,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */

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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/endian.h>

struct servtab {
    const char	*name;
    const char	*alias;
    short	port;
    short	proto;
};

#define TCP	0
#define UDP	1

static const struct servtab servtab[] = {
    {"echo",	0,	7,	TCP},
    {"echo",	0,	7,	UDP},
    {"discard", "sink",	9,	TCP},
    {"discard",	"sink",	9,	UDP},
    {"daytime", 0,	13,	TCP},
    {"daytime",	0,	13,	UDP},
    {"ftp",	0,	21,	TCP},
    {"telnet",	0,	23,	TCP},
    {"time", 	0,	37,	TCP},
    {"time",	0,	37,	UDP},
    {"bootps",	0,	67,	UDP},
    {"bootpc",	0,	68,	UDP},
    {"tftp",	0,	69,	UDP},
    {"sunrpc",	0,	111,	TCP},
    {"sunrpc",	0,	111,	UDP},
    /* Unix specials */
    {"exec",	0,	512,	TCP},
    {"login",	0,	513,	TCP},
    {"shell",	"cmd",	514,	TCP},
    {"talk",	0,	518,	UDP},
    {"bfs",	0,	2201,	UDP},	/* MIPS boot file server */
    {"gdb",	"rdbg",	2202,	UDP},	/* GDB remote debugging */
    {0}
};

static int servidx;
int _serv_stayopen;

void setservent(x)
{
	servidx = 0;
}

void endservent()
{
	servidx = -1;
}    

struct servent *
getservent()
{
static struct servent sv;
static char *aliases[2];
	const struct servtab *st;

	if (servidx < 0)
	      return (0);

	st = &servtab[servidx];
	if (!st->name)
		return (0);

	sv.s_name = (char *)st->name;
	aliases[0] = (char *)st->alias; aliases[1] = 0;
	sv.s_aliases = aliases;
	sv.s_port = htons(st->port);
	sv.s_proto = (st->proto == TCP) ? "tcp" : "udp";

	servidx++;
	return (&sv);
}
