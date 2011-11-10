/* $Id: hist.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */
/*
 * Copyright (c) 2001-2003 Opsycon AB  (www.opsycon.se / www.opsycon.com)
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
#include <ctype.h>
#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif

#include <pmon.h>

#define PREV	CNTRL('P')
#define NEXT	CNTRL('N')
#define FORW	CNTRL('F')
#define BACK	CNTRL('B')
#define BEGIN	CNTRL('A')
#define END	CNTRL('E')
#define DELETE	CNTRL('D')
#define DELLINE	CNTRL('K')
#define MARK	CNTRL(' ')
#define KILL	CNTRL('W')

char cvtab[] = { PREV, NEXT, FORW, BACK, 'E', END, 'G', BEGIN };

enum esc_state { NONE, HAVE_ESC, HAVE_LB };

#define HISTMAX 200
char           *hist[HISTMAX + 1];	/* array of old commands */

int             histi;		/* index to hist[], indicates next slot */
int             histo;		/* index to hist[], indicates oldest entry */
int             histr;		/* index to hist[], used while walking back with PREV */
int             histno;		/* current history number */
int             column;		/* current column number */

char		*gethists __P((char *));

int cmd_hi __P((int, char *[]));

static void	addchar __P((char *, int));
static void	backspace __P((int));
static void	right __P((char *, int));
static void	left __P((int));
static void	putstr __P((char *));
static void	addhist __P((char *));
static char	*findbang __P((char *));

/*************************************************************
 *  histinit() initialize history mechanism
 */
void
histinit ()
{

    histno = 1;
}

/*************************************************************
 *  get_line(p) get a line from stdin using lineedit and history
 */
void
get_line (p, usehist)
     char           *p;
     int	    usehist;
{
	enum esc_state esc_state = NONE;
	int c, oc, i, n, mark;
	struct termio tbuf;

	ioctl (STDIN, SETNCNE, &tbuf);

	mark = column = 0;
	histr = histi;

	if (*p) {
		putstr (p);
		left (column);
	}

    for(;;) {
	c = getchar ();
	switch (esc_state) {
	case NONE:
		if (c == 0x1b) {
			esc_state = HAVE_ESC;
			continue;
		}
		break;

	case HAVE_ESC:
		if (c == '[') {
			esc_state = HAVE_LB;
			continue;
		}
		esc_state = NONE;
		break;

	case HAVE_LB:
		if (c >= 'A' && c <= 'H' ) {
			c = cvtab[c - 'A'];
		}
		esc_state = NONE;
		break;

	default:
		esc_state = NONE;
		break;
	}

	if (c == PREV && usehist) {
	    right (p, strlen (p) - column);
	    backspace (column);
	    if (histr != histo)
		histr = decmod (histr, HISTMAX);
	    strcpy (p, hist[histr]);
	    putstr (p);
	    mark = 0;
	} else if (c == NEXT && usehist) {
	    right (p, strlen (p) - column);
	    backspace (column);
	    if (histr != histi)
		histr = incmod (histr, HISTMAX);
	    strcpy (p, hist[histr]);
	    putstr (p);
	    mark = 0;
	} else if (c == FORW)
	    right (p, 1);
	else if (c == BACK)
	    left (1);
	else if (c == BEGIN)
	    left (column);
	else if (c == END)
	    right (p, strlen (&p[column]));
	else if (c == MARK)
	    mark = column;
	else if (c == DELETE) {
	    oc = column;
	    strdchr (&p[column]);
	    putstr (&p[column]);
	    putstr (" ");
	    left (strlen (&p[oc]) + 1);
	    if (mark > oc)
	      mark--;
	} else if (c == DELLINE) {
	    n = strlen (&p[column]);
	    p[column] = '\0';
	    oc = column;
	    for (i = n; i > 0; i--)
	      putstr (" ");
	    left (n);
	    if (mark > oc)
	      mark = oc;
	} else if (c == KILL) {
	    if (column > mark) {
		n = column - mark;
		left (n);
	    } else if (column < mark) {
		n = mark - column;
	    } else
	       continue;
	    for (i = n; i > 0; i--)
	      strdchr (&p[column]);
	    oc = column;
	    putstr (&p[column]);
	    for (i = n; i > 0; i--)
	      putstr (" ");
	    left (strlen (&p[oc]) + n);
	    mark = column;
	} else if (c == '\b' || c == 0x7f) {
	    left (1);
	    oc = column;
	    strdchr (&p[column]);
	    putstr (&p[column]);
	    putstr (" ");
	    left (strlen (&p[oc]) + 1);
	    if (mark >= oc)
	      mark--;
	} else if (c == '\n' || c == '\r') {
	    putstr ("\n");
	    break;
	} else if (isprint (c)) {
	    addchar (p, c);
	} else
	    putchar (CNTRL ('G'));
    }

    ioctl (STDIN, TCSETAW, &tbuf);
}

void
get_line1 (p, usehist)
     char           *p;
     int	    usehist;
{
	enum esc_state esc_state = NONE;
	int c, oc, i, n, mark;
	struct termio tbuf;

	ioctl (STDIN, SETNCNE, &tbuf);

	mark = column = 0;
	histr = histi;

	if (*p) {
		putstr (p);
		left (column);
	}

    for(;;) {
	c = getchar ();
	switch (esc_state) {
	case NONE:
		if (c == 0x1b) {
			esc_state = HAVE_ESC;
			continue;
		}
		break;

	case HAVE_ESC:
		if (c == '[') {
			esc_state = HAVE_LB;
			continue;
		}
		esc_state = NONE;
		break;

	case HAVE_LB:
		if (c >= 'A' && c <= 'H' ) {
			c = cvtab[c - 'A'];
		}
		esc_state = NONE;
		break;

	default:
		esc_state = NONE;
		break;
	}

	if (c == PREV && usehist) {
	    right (p, strlen (p) - column);
	    backspace (column);
	    if (histr != histo)
		histr = decmod (histr, HISTMAX);
	    strcpy (p, hist[histr]);
	    putstr (p);
	    mark = 0;
	} else if (c == NEXT && usehist) {
	    right (p, strlen (p) - column);
	    backspace (column);
	    if (histr != histi)
		histr = incmod (histr, HISTMAX);
	    strcpy (p, hist[histr]);
	    putstr (p);
	    mark = 0;
	} else if (c == FORW)
	    right (p, 1);
	else if (c == BACK)
	    left (1);
	else if (c == BEGIN)
	    left (column);
	else if (c == END)
	    right (p, strlen (&p[column]));
	else if (c == MARK)
	    mark = column;
	else if (c == DELETE) {
	    oc = column;
	    strdchr (&p[column]);
	    putstr (&p[column]);
	    putstr (" ");
	    left (strlen (&p[oc]) + 1);
	    if (mark > oc)
	      mark--;
	} else if (c == DELLINE) {
	    n = strlen (&p[column]);
	    p[column] = '\0';
	    oc = column;
	    for (i = n; i > 0; i--)
	      putstr (" ");
	    left (n);
	    if (mark > oc)
	      mark = oc;
	} else if (c == KILL) {
	    if (column > mark) {
		n = column - mark;
		left (n);
	    } else if (column < mark) {
		n = mark - column;
	    } else
	       continue;
	    for (i = n; i > 0; i--)
	      strdchr (&p[column]);
	    oc = column;
	    putstr (&p[column]);
	    for (i = n; i > 0; i--)
	      putstr (" ");
	    left (strlen (&p[oc]) + n);
	    mark = column;
	} else if (c == '\b' || c == 0x7f) {
	    left (1);
	    oc = column;
	    strdchr (&p[column]);
	    putstr (&p[column]);
	    putstr (" ");
	    left (strlen (&p[oc]) + 1);
	    if (mark >= oc)
	      mark--;
	} else if (c == '\n' || c == '\r') {
	    break;
	} else if (isprint (c)) {
	    addchar (p, c);
	} else
	    putchar (CNTRL ('G'));
    }

    ioctl (STDIN, TCSETAW, &tbuf);
}

void
get_cmd (p)
  char *p;
{
    char *q, buf[20], *t;
    u_int32_t v;
    int i;

    *p = '\0';
    get_line (p, 1);

/* do C-shell history subs */
    if ((q = findbang (p)) != 0) {
	getword (buf, q);
	strdchr (buf);		/* delete the bang */
	if (!strempty (buf)) {
	    for (i = strlen (buf) + 1; i > 0; i--)
		strdchr (q);
	    if (!strcmp(buf, "!")) {	/* repeat last cmd */
		stristr (q, hist[decmod (histi, HISTMAX)]);
		printf ("%s\n", p);
	    } else if (atob (&v, buf, 10)) {	/* hist# */
		if ((t = gethistn (v)) != 0) {
		    stristr (q, t);
		    printf ("%s\n", p);
		} else
		    printf ("%s: Event not found.\n", buf);
	    } else {		/* hist string */
		if ((t = gethists (buf)) != 0) {
		    stristr (q, t);
		    printf ("%s\n", p);
		} else
		    printf ("%s: Event not found.\n", buf);
	    }
	}
    }
    addhist (p);
}

/*************************************************************
 *  static addchar(p,c) add char to line, updates 'column'
 */
static void
addchar (p, c)
     char           *p;
     int             c;
{
    int             oc;

    if (strlen (p) > LINESZ - 40)
	return;
    oc = column;
    strichr (&p[column], c);
    putstr (&p[column]);
    left (strlen (&p[oc]) - 1);
}

/*************************************************************
 *  static backspace(n) distructive backspace, updates 'column'
 */
static void
backspace (n)
     int             n;
{

    for (; n > 0 && column > 0; n--) {
	printf ("\b \b");
	column--;
    }
}

/*************************************************************
 *  static right(p,n) move the cursor n places right, updates global 'column'
 */
static void
right (p, n)
     char           *p;
     int             n;
{
    int             len;

    len = strlen (p);
    for (; n > 0 && column < len; n--) {
	putchar (p[column]);
	column++;
    }
}

/*************************************************************
 *  static left(n) move the cursor n places left, updating global 'column'
 */
static void
left (n)
     int             n;
{

    for (; n > 0 && column > 0; n--) {
	putchar ('\b');
	column--;
    }
}

/*************************************************************
 *  char *gethistn(n) return ptr to hist entry n
 */
char           *
gethistn (n)
     int             n;
{
    int             m, d;

    if (n < (histno - HISTMAX) || n > histno)
	return (0);
    m = histi;
    d = histno - n;
    for (; d > 0; d--)
	m = decmod (m, HISTMAX);
    return (hist[m]);
}

/*************************************************************
 *  char *gethists(p) use strbequ to return ptr to matching hist string
 */
char           *
gethists (p)
     char           *p;
{
    int             i;

    i = decmod (histi, HISTMAX);
    while (i != decmod (histo, HISTMAX)) {
	if (hist[i] == 0)
	    break;
	if (strbequ (hist[i], p))
	    return (hist[i]);
	i = decmod (i, HISTMAX);
    }
    return (0);
}

/*************************************************************
 *  addhist(p) add command line to history
 */
static void
addhist (p)
     char *p;
{
    int             i;

    if (strempty (p))
	return;

    i = decmod (histi, HISTMAX);
    if (hist[i] != 0) {
	if (!strcmp(p, hist[i]))
	    return;
    }
    if (incmod (histi, HISTMAX) == histo) {
	if (hist[histo] != 0)
	    free (hist[histo]);
	hist[histo] = 0;
	histo = incmod (histo, HISTMAX);
    }
    hist[histi] = malloc (strlen (p) + 1);
    if (hist[histi] == 0) {
	printf ("addhist: out of memory\n");
	return;
    }
    strcpy (hist[histi], p);
    histi = incmod (histi, HISTMAX);
    histno++;
}

/*************************************************************
 *  static putstr(p) print string and update global 'column'
 */
static void
putstr (p)
     char *p;
{
    int             i;

    for (i = 0; p[i] != 0; i++) {
	putchar (p[i]);
	if (p[i] == '\n')
	    column = 0;
	else
	    column++;
    }
}

/*************************************************************
 *  char *findbang(p)
 */
static char           *
findbang (p)
     char           *p;
{
    int             quote;

    for (; *p; p++) {
	if (*p == '\'' || *p == '"') {
	    quote = *p;
	    p++;
	    while (*p && *p != quote)
		p++;
	} else if (*p == '!')
	    return (p);
    }
    return (0);
}

/*************************************************************
 *  cmd_hi() hi command
 */
int 
cmd_hi (ac, av)
     int             ac;
     char           *av[];
{
    int             i, n, l;
    u_int32_t	    siz;

    if (ac == 2) {
	if (!get_rsa (&siz, av[1]))
	    return (-1);
    } else {
	siz = moresz;
    }

    ioctl (STDIN, CBREAK, NULL);
    l = siz;
    i = decmod (histi, HISTMAX);
    n = histno - 1;
    while (i != decmod (histo, HISTMAX)) {
	sprintf (prnbuf, "%5d  %s", n, hist[i]);
	if (more (prnbuf, &l, (ac > 1) ? 0 : siz))
	    break;
	i = decmod (i, HISTMAX);
	n--;
    }
    return (0);
}

static const Cmd Cmds[] =
{
	{"Shell"},
	{"hi",          "[cnt]",
			0,
			"display command history",
			cmd_hi, 1, 2, 0},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 0);
}
