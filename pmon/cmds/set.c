/* $Id: set.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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

int	cmd_set __P((int, char *[]));
int	cmd_eset __P((int, char *[]));
int	cmd_unset __P((int, char *[]));

static const Cmd Cmds[] = {
	{"Environment"},
	{"set",		"[[-t] name [value]]",
			0,
			"display/set variable",
			cmd_set, 1, 4, CMD_REPEAT},
	{"eset",	"name ...",
			0,
			"edit variable(s)",
			cmd_eset, 2, 99, CMD_REPEAT},
	{"unset",	"name* ...",
			0,
			"unset variable(s)",
			cmd_unset, 2, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}


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
    {"dlproto", "EtxAck", "none XonXoff EtxAck"},
#ifdef INET
    {"bootp",	"no", "no sec pri save"},
#endif
    {"hostport", "tty1", 0},
    {"inalpha", "hex", "hex symbol"},
    {"inbase", INBASE_DEFAULT, INBASE_STRING},
    {"moresz", "10", 0, chg_moresz},
    {"prompt", "PMON> ", 0},
    {"regstyle", "sw", "hw sw"},
#ifdef HAVE_QUAD
    {"regsize", "32", "32 64"},
#endif
    {"rptcmd", "trace", "off on trace"},
    {"trabort", "^K", 0},
    {"ulcr", "cr", "cr lf crlf"},
    {"uleof", "%", 0},
    {"validpc", "_ftext etext", 0, chg_validpc},
    {"heaptop", SETCLIENTPC, 0, chg_heaptop},
    {"showsym", "yes", "no yes"},
    {"fpfmt", "both", "both double single none"},
    {"fpdis", "yes", "no yes"},
#if defined(TGT_DEFENV)
    TGT_DEFENV,
#endif
    {0}
};

struct envpair {
    char	*name;
    char	*value;
};

#define NVAR	64
static struct envpair envvar[NVAR];

static int printvar __P((char *, char *, int *));
static int _matchval __P((const struct stdenv *, char *));
static const struct stdenv *getstdenv __P((const char *));
static int _setenv __P((char *, char *));


static int 
_matchval (sp, value)
    const struct stdenv *sp;
    char *value;
{
    char *t, wrd[20];
    int j;

    for (j = 0, t = sp->values; ; j++) {
	t = getword (wrd, t);
	if (t == 0)
	    return (-2);
	if (striequ (wrd, value))
	    return (j);
    }
}


static const struct stdenv *
getstdenv (name)
    const char *name;
{
    const struct stdenv *sp;
    for (sp = stdenvtab; sp->name; sp++)
	if (striequ (name, sp->name))
	    return sp;
    return 0;
}


static int
_setenv (name, value)
    char *name, *value;
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

static int
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
		tgt_unsetenv (ep->name);	/* first from non-volatile ram */
		free (ep->name);		/* then from local copy */
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


int
cmd_set(ac, av)
	int ac;
	char *av[];
{
	struct envpair *ep;
	char *s;
	int ln;
	int tempflag = 0;

	if(ac >= 2 && strcmp(av[1], "-t") == 0) {
		tempflag = 1;
	}

	ln = moresz;
	switch (ac - tempflag) {
	case 1:			/* display all variables */
		ioctl (STDIN, CBREAK, NULL);
		for (ep = envvar; ep < &envvar[NVAR]; ep++) {
			if (ep->name && printvar(ep->name, ep->value, &ln)) {
				break;
			}
		}
		break;

	case 2:			/* display specific variable */
		if ((s = getenv (av[1 + tempflag])) != 0) {
			printvar (av[1 + tempflag], s, &ln);
		}
		else {
			printf("%s: not found\n", av[1]);
			return(1);
		}
		break;

	case 3:			/* set specific variable */
		if(!do_setenv(av[1 + tempflag], av[2 + tempflag], tempflag)) {
			printf("%s: cannot set variable\n", av[1 + tempflag]);
			return(1);
		}
		break;

	default:
		return (-1);
	}
	return (0);
}


int
cmd_eset(ac, av)
	int ac;
	char *av[];
{
	char name[LINESZ];
	char val[LINESZ];
	char *s;
	int i;
	int tempflag = 0;

	if(ac >= 2 && strcmp(av[1], "-t") == 0) {
		tempflag = 1;
	}

	for (i = 1 + tempflag; i < ac; i++) {
		strcpy (name, av[i]);
		strtoupper(name);

		s = getenv (av[i]);
		if(!s) {
			printf ("%s: not found\n", name);
			continue;
		}

		printf("%s=", name); fflush (stdout);

		strcpy(val, s);
		get_line(val, 0);

		if (!strequ (val, s)) {
			if (!do_setenv (av[i], val, tempflag)) {
				printf ("%s: cannot set variable\n", name);
				return (1);
			}
		}
	}
	return (0);
}


int
cmd_unset(ac, av)
	int ac;
	char *av[];
{
	int i;

	for (i = 1; i < ac; i++) {
		if (getenv(av[i])) {
			unsetenv(av[i]);
		}
		else {
			printf ("%s: no matching variable\n", av[i]);
			return (1);
		}
	}
	return (0);
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

