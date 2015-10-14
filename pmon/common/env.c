/* $Id: env.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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
#include <stdio.h>
#include <termio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif
#include <pmon.h>
#include <pmonenv.h>

#include "cmd_more.h"

static int	envinited = 0;

static struct stdenv {
    char           *name;
    char           *init;
    char           *values;
    int		   (*chgfunc) __P((char *, char *));
} stdenvtab[] = {
    {"brkcmd", "l -r @cpc 1", 0},
#ifdef HAVE_QUAD
    {"datasize", "-b", "-b -h -w -d"},
#else
    {"datasize", "-b", "-b -h -w"},
#endif
    {"dlecho", "off", "off on lfeed"},
    {"dlproto", "none", "none XonXoff EtxAck"},
#ifdef INET
    {"bootp",	"no", "no sec pri save"},
#endif
    {"hostport", "tty0", 0},
    {"inalpha", "hex", "hex symbol"},
    {"inbase", INBASE_DEFAULT, INBASE_STRING},
#if NCMD_MORE > 0
    {"moresz", "10", 0, chg_moresz},
#endif
    {"prompt", "PMON> ", 0},
    {"regstyle", "sw", "hw sw"},
#ifdef HAVE_QUAD
    {"regsize", "32", "32 64"},
#endif
    {"rptcmd", "trace", "off on trace"},
    {"trabort", "^K", 0},
    {"ulcr", "cr", "cr lf crlf"},
    {"uleof", "%", 0},
#ifdef PMON_DEBUGGER /* XXX: Patrik temp */
    {"validpc", "_ftext etext", 0, chg_validpc},
#endif
    {"heaptop", SETCLIENTPC, 0, chg_heaptop},
    {"showsym", "yes", "no yes"},
    {"fpfmt", "both", "both double single none"},
    {"fpdis", "yes", "no yes"},
	{"TZ",	"UTC8",	0},		//lxy
	{"ifconfig", "syn0:192.168.1.2", 0},
	{"update_usb", "no", "no yes"},	//lxy
//	{"novga", "1", 0},	//当设置为novga=1时表示不使用vga lcd显示屏输出信息 信息只输出到串口 这样可以把菜单定位到串口，因为菜单默认定位到显示屏。
	{"bootdelay", "8", 0},
#ifdef BOOT_TEST_NAND_MEM
	{"boot_test", "yes", "no yes"},
#endif
	/* 用于记录测试结果 */
//	{"cputest", "等待测试", 0},
//	{"memorytest", "等待测试", 0},
//	{"net0test", "等待测试", 0},
//	{"net1test", "等待测试", 0},
//	{"touchscreentest", "等待测试", 0},
//	{"SDcardtest", "等待测试", 0},
//	{"videotest", "等待测试", 0},
//	{"USBtest", "等待测试", 0},
//	{"buttontest", "等待测试", 0},
//	{"UARTtest", "等待测试", 0},
//	{"AC97test", "等待测试", 0},
//	{"ADtest", "等待测试", 0},
//	{"RTCtest", "等待测试", 0},
//	{"NANDFlashtest", "等待测试", 0},
#if defined(TGT_DEFENV)
    TGT_DEFENV,
#endif
    {0}
};

struct envpair envvar[NVAR];

static int _matchval __P((const struct stdenv *, char *));
static const struct stdenv *getstdenv __P((const char *));
static int _setenv __P((char *, char *));

static int _matchval(const struct stdenv *sp, char *value)
{
	char *t, wrd[20];
	int j;

	for (j=0, t=sp->values; ; j++) {
		t = getword(wrd, t);
		if (t == 0)
			return (-2);
		if (striequ(wrd, value))
			return (j);
	}
}


static const struct stdenv *getstdenv(const char *name)
{
	const struct stdenv *sp;
	for (sp=stdenvtab; sp->name; sp++)
		if (striequ(name, sp->name))
			return sp;
	return 0;
}

static int _setenv (char *name, char *value)
{
    struct envpair *ep;
    struct envpair *bp = 0;
    const struct stdenv *sp;

    if ((sp = getstdenv (name)) != 0) {
	if (sp-> chgfunc && !(*sp->chgfunc) (name, value))
	    return 0;
	if (sp->values && _matchval (sp, value) < 0 && envinited) {
	    printf ("%s: bad %s value, try [%s]\n", value, name, sp->values);
	    return 0;
	}
    }

    for (ep = envvar; ep < &envvar[NVAR]; ep++) {
	if (!ep->name && !bp)
	  bp = ep;
	else if (ep->name && striequ (name, ep->name))
	  break;
    }
    
    if (ep < &envvar[NVAR]) {
	/* must have got a match, free old value */
	if (ep->value) {
	    free (ep->value); ep->value = 0;
	}
    } else if (bp) {
	/* new entry */
	ep = bp;
	if (!(ep->name = malloc (strlen (name) + 1)))
	  return 0;
	strcpy (ep->name, name);
    } else {
	return 0;
    }

    if (value) {
	if (!(ep->value = malloc (strlen (value) + 1))) {
	    free (ep->name); ep->name = 0;
	    return 0;
	}
	strcpy (ep->value, value);
    }

    return 1;
}

int
printvar(name, value, cntp)
	char *name;
	char *value;
	int  *cntp;
{
	const struct stdenv *ep;
	char buf[300];
	char *b;

	if (!value) {
		sprintf (buf, "%10s", name);
	}
	else if(strchr (value, ' ')) {
		sprintf (buf, "%10s = \"%s\"", name, value);
	}
	else {
		sprintf (buf, "%10s = %-10s", name, value);
	}

	b = buf + strlen(buf);
	if ((ep = getstdenv (name)) && ep->values) {
		sprintf (b, "  [%s]", ep->values);
	}
	return(more(buf, cntp, moresz));
}

int
setenv (name, value)
    char *name, *value;
{
	return(do_setenv (name, value, 0));
}

int
do_setenv (name, value, temp)
	char *name, *value;
	int temp;
{
	if (_setenv (name, value)) {
#ifdef HAVE_NVENV
		const struct stdenv *sp;
		if ((sp = getstdenv (name)) && striequ (value, sp->init)) {
			/* set to default: remove from non-volatile ram */
			return tgt_unsetenv (name);
		}
		else if(!temp) {
			/* new value: save in non-volatile ram */
			return tgt_setenv (name, value);
		}
		else {
			return(1);
		}
#endif
	}
	return 0;
}

char *
getenv (name)
    const char *name;
{

    if (envinited) {
	struct envpair *ep;
	for (ep = envvar; ep < &envvar[NVAR]; ep++)
	  if (ep->name && striequ (name, ep->name))
	    return ep->value ? ep->value : "";
    } else {
	const struct stdenv *sp;
	if ((sp = getstdenv (name)) != 0)
	    return sp->init;
    }
    return 0;
}


void
mapenv (pfunc)
    void (*pfunc) __P((char *, char *));
{
    struct envpair *ep;

    for (ep = envvar; ep < &envvar[NVAR]; ep++) {
	if (ep->name)
	  (*pfunc) (ep->name, ep->value);
    }
}
 
void
unsetenv (pat)
    const char *pat;
{
	const struct stdenv *sp;
	struct envpair *ep;
	int ndone = 0;

	for (ep = envvar; ep < &envvar[NVAR]; ep++) {
		if (ep->name && strpat (ep->name, pat)) {
			sp = getstdenv (ep->name);
			if (sp) {
				/* unsetenving a standard variable resets it to initial value! */
				if (!setenv (sp->name, sp->init))
					return;
			} else {
				/* normal user variable: delete it */
#ifdef HAVE_NVENV				
				tgt_unsetenv (ep->name);
#endif
				free (ep->name);
				if (ep->value)
					free (ep->value);
				ep->name = ep->value = 0;
			}
			ndone++;
		}
	}
}

void
envinit ()
{
	int i;

    SBD_DISPLAY ("MAPV", CHKPNT_MAPV);

    /* extract nvram variables into local copy */
    bzero (envvar, sizeof(envvar));
    tgt_mapenv (_setenv);
    envinited = 1;

    SBD_DISPLAY ("STDV", CHKPNT_STDV);
    /* set defaults (only if not set at all) */
    for (i = 0; stdenvtab[i].name; i++) {
	if (!getenv (stdenvtab[i].name)) {
	  setenv (stdenvtab[i].name, stdenvtab[i].init);
	}
    }
}

/** matchenv(name) returns integer corresponding to current value */
int
matchenv (name)
     char           *name;
{
    const struct stdenv *ep;
    char           *value;

    if (!(ep = getstdenv (name)))
	return (-1);

    if (!(value = getenv (name)))
      value = "";

    return _matchval (ep, value);
}

/* Two helpers to build a traditional environment for clients */

void
envsize (int *ec, int *slen)
{
	struct envpair *ep;

	*ec = 0;
	*slen = 0;
	for (ep = envvar; ep < &envvar[NVAR]; ep++) {
		if (ep->name) {
			*ec += 1;
			*slen += strlen (ep->name) + 2; /* = and NULL */
			if (ep->value) {
				*slen += strlen(ep->value);
			}
		}
	}
}

void
envbuild (char **vsp, char *ssp)
{
	struct envpair *ep;

	for(ep = envvar; ep < &envvar[NVAR]; ep++) {
		if (ep->name) {
			*vsp++ = ssp;
			ssp += sprintf (ssp, "%s=", ep->name) + 1;
			if (ep->value) {
				ssp += sprintf(ssp - 1, "%s", ep->value);
			}
		}
	}
	*vsp++ = (char *)0;
}

