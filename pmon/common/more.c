/* $Id: more.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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
#include <termio.h>
#include <string.h>

#include <pmon.h>

#define BEL 007

char            more_pat[LINESZ];
const char * const more_msg = "more... ";

static int getinp __P((int *, int));

int
chg_moresz (name, value)
    char *name, *value;
{
	u_int32_t v;

	if (atob (&v, value, 10) && v > 2 && v < 100) {
		moresz = v;
		return 1;
	}
	printf ("%s: invalid moresz value\n", value);
	return 0;
}


const Optdesc         more_opts[] =
{
    {"/str", "search for str"},
    {"n", "repeat last search"},
    {"SPACE", "next page"},
    {"CR", "next line"},
    {"q", "quit"},
    {0}};

/*************************************************************
 *  more(p,cnt,size)
 *      The 'more' paginator    
 */
int
more (p, cnt, size)
     char           *p;
     int            *cnt, size;
{
    int             r;

    if (*cnt == 0) {
	if (size == 0)
	    return (1);		/* if cnt=0 and size=0 we're done */
	while ((r = getinp (cnt, size)) == -1);
	if (r) {		/* quit */
	    *p = 0;
	    return (1);
	}
    }
    if (*cnt == -1) {		/* search in progress */
	if (!strposn (p, more_pat)) {
	    dotik (256, 0);
	    return (0);		/* not found yet */
	} else {		/* found */
	    *cnt = size;
	    printf ("\b \n");
	}
    }
    printf ("%s\n", p);
    *p = 0;
    (*cnt)--;
    return (0);
}

/*************************************************************
 *  getinp(cnt,size)
 */
static int
getinp (cnt, size)
     int            *cnt, size;
{
    int             i, c;
    const char *s;

    printf ("%s", more_msg);
    for(;;) {
	c = getchar ();
	if (strchr ("nq/ \n", c))
	    break;
	putchar (BEL);
    }
    s = more_msg;
    while(*s++) {
	printf("\b \b");
    }
    if (c == 'q')
	return (1);
    switch (c) {
    case ' ':
	*cnt = size;
	break;
    case '\n':
	*cnt = 1;
	break;
    case '/':
	/* get pattern */
	putchar ('/');
	for (i = 0;;) {
	    c = getchar ();
	    if (c == '\n')
		break;
	    if (c == '\b') {
		if (i > 0) {
		    putchar (c);
		    i--;
		} else {
		    putchar ('\b');
		    return (-1);
		}
	    } else {
		putchar (c);
		more_pat[i++] = c;
	    }
	}
	more_pat[i] = 0;
	printf ("  ");
	*cnt = -1;		/* enter search mode */
	break;
    case 'n':
	printf ("/%s  ", more_pat);
	*cnt = -1;		/* enter search mode */
	break;
    }
    return (0);
}

static const Cmd Cmds[] =
{
	{"Shell"},
	{"more",        "",
			more_opts,
			"paginator",
			no_cmd, 1, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
        cmdlist_expand(Cmds, 0);
}
