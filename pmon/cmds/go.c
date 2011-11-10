/* $Id: go.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

#include <stdio.h>
#include <termio.h>
#include <string.h>
#include <setjmp.h>
#include <stdlib.h>

#include <signal.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif

#include <pmon.h>

extern struct trapframe DBGREG;
extern struct trapframe TRPREG;

extern void _go __P((void));

int             trace_mode;
unsigned long   trace_count;
int             trace_verbose;
int             trace_invalid;
int             trace_over;
int             trace_bflag;
int             trace_cflag;

char            clientcmd[LINESZ];
char           *clientav[MAX_AC];
int		clientac;

extern int      clkdat;
#if defined(SCSI) || defined(IDE)
extern void    *callvec;
#else
#define callvec NULL
#endif

#define STOPMAX 10
Stopentry       stopval[STOPMAX];

#define PCHISTSZ  200
unsigned long   pchist_d[PCHISTSZ + 1];
int             pchist_ip, pchist_op;

jmp_buf	gobuf;

void exception __P((struct trapframe *));

static int	setTrcbp __P((int32_t, int));
static int	is_break_point __P((int32_t));
static int	is_validpc __P((int32_t));
static void	addpchist __P((int));
static void	clrpchist __P((void));
static void store_trace_breakpoint __P((void));
static void remove_trace_breakpoint __P((void));
static void store_breakpoint __P((void));
void sstep __P((void));
static void compute_validpc __P((void));
void stop __P((char *));
void rm_bpts __P((void));
void dspbpts __P((void));
int addstop __P((u_int32_t, u_int32_t, char *, char));
void initstack __P((int, char **, int));

/*
 *  Build argument area on 'clientstack' and set up clients
 *  argument registers in preparation for 'launch'.
 *  arg1 = argc, arg2 = argv, arg3 = envp, arg4 = callvector
 */

void
initstack (ac, av, addenv)
    int ac;
    char **av;
    int addenv;
{
	char	**vsp, *ssp;
	int	ec, stringlen, vectorlen, stacklen, i;
	register_t nsp;

	/*
	 *  Calculate the ammount of stack space needed to build args.
	 */
	stringlen = 0;
	for (i = 0; i < ac; i++) {
		stringlen += strlen(av[i]) + 1;
	}
	if (addenv) {
		envsize (&ec, &stringlen);
	}
	else {
		ec = 0;
	}
	stringlen = (stringlen + 3) & ~3;	/* Round to words */
	vectorlen = (ac + ec + 2) * sizeof (char *);
	stacklen = ((vectorlen + stringlen) + 7) & ~7;

	/*
	 *  Allocate stack and us md code to set args.
	 */
	nsp = md_adjstack(0) - stacklen;
	md_setargs(ac, nsp, nsp + (ac + 1) * sizeof(char *), (int)callvec);

	/* put $sp below vectors, leaving 32 byte argsave */
	md_adjstack(nsp - 32);
	memset((void *)(nsp - 32), 0, 32);

	/*
	 * Build argument vector and strings on stack.
	 * Vectors start at nsp; strings after vectors.
	 */
	vsp = (char **) nsp;
	ssp = (char *) (nsp + vectorlen);

	for (i = 0; i < ac; i++) {
		*vsp++ = ssp;
		strcpy (ssp, av[i]);
		ssp += strlen(av[i]) + 1;
	}
	*vsp++ = (char *)0;

	/* build environment vector on stack */
	if (ec) {
		envbuild (vsp, ssp);
	}
	else {
		*vsp++ = (char *)0;
	}
}


const Optdesc         g_opts[] = {
	{"-s", "don't set client sp"},
	{"-t", "time execution"},
	{"-e <adr>", "start address"},
	{"-b <bptadr>", "temporary breakpoint"},
	{"-- <args>", "args to be passed to client"},
	{0}
};

/*************************************************************
 *  go(ac,av), the 'g' command
 */
int
go (ac, av)
	int	ac;
	char	*av[];
{
	int32_t	adr;
	int	sflag;
	int	c;
extern int	optind;
extern char	*optarg;

	sflag = 0;
	BptTmp.addr = NO_BPT;
	BptTrc.addr = NO_BPT;
	BptTrcb.addr = NO_BPT;

	strcpy (clientcmd, av[0]);
	strcat (clientcmd, " ");

	optind = 0;
	while ((c = getopt (ac, av, "b:e:st")) != EOF) {
		switch (c) {
		case 's':
			sflag = 1; 
			break;
		case 't':
			strcpy (clientcmd, "time ");
			break;
		case 'b':
			if (!get_rsa (&adr, optarg)) {
				return (-1);
			}
			BptTmp.addr = adr;
			break;
		case 'e':
			if (!get_rsa (&adr, optarg)) {
				return (-1);
			}
			md_setentry(adr);
			break;
		default:
			return (-1);
		}
	}

	while (optind < ac) {
		strcat (clientcmd, av[optind++]);
		strcat (clientcmd, " ");
	}

	if (!sflag) {
		md_adjstack(tgt_clienttos ());
	}

	clientac = argvize (clientav, clientcmd);
	initstack (clientac, clientav, 1);

	clrhndlrs ();
	closelst (2);
	md_setsr(initial_sr);
	tgt_enable (tgt_getmachtype ()); /* set up i/u hardware */
#ifdef FLOATINGPT
	Fcr = 0;		/* clear any outstanding exceptions / enables */
#endif

#ifdef __powerpc__
	strcpy ((void *)0x4200, getenv ("vxWorks"));
#endif

	if (setjmp (gobuf) == 0) {
		goclient ();
	}
	swlst(1);
	return 0;
}

/*************************************************************
 *  cont(ac,av) the continue command
 */
int
cont (ac, av)
	int	ac;
	char	*av[];
{
	int32_t	adr;

	BptTmp.addr = NO_BPT;
	BptTrc.addr = NO_BPT;
	BptTrcb.addr = NO_BPT;
	if (ac > 1) {
		if (!get_rsa (&adr, av[1])) {
			return (-1);
		}
		BptTmp.addr = adr;
	}
	goclient ();
	return(0); /* Shut up gcc */
}

/*************************************************************
 *  trace(ac,av) the 't' (single-step) command
 */
const Optdesc         t_opts[] = {
	{"-v", "verbose, list each step"},
	{"-b", "capture only branches"},
	{"-c", "capture only calls"},
	{"-i", "stop on pc invalid"},
	{"-m adr val", "stop on mem equal"},
	{"-M adr val", "stop on mem not equal"},
	{"-r reg val", "stop on reg equal"},
	{"-R reg val", "stop on reg not equal"},
	{0}
};

int
trace (ac, av)
	int ac;
	char *av[];
{
	int multi, i, j, n;
	register_t *reg;
	register_t adr, val;
	struct trapframe *frame = &DBGREG;

	trace_over = 0;
	if (strequ (av[0], "to")) {
		trace_over = 1;
	}

	n = 0;
	multi = 0;
	trace_verbose = 0;
	trace_invalid = 0;
	trace_bflag = 0;
	trace_cflag = 0;

	for (i = 0; i < STOPMAX; i++) {
		stopval[i].addr = 0;
	}
	for (i = 1; i < ac; i++) {
		if (av[i][0] == '-') {
			for (j = 1; av[i][j] != 0; j++) {
				if (av[i][j] == 'v') {
					trace_verbose = 1;
					trace_count = 0;
					multi = 1;
				}
				else if (av[i][j] == 'm' || av[i][j] == 'M') {
					if (i + 2 >= ac) {
						printf ("bad arg count\n");
						return (-1);
					}
					if (!get_rsa (&adr, av[i + 1])) {
						return (-1);
					}
					if (!get_rsa (&val, av[i + 2])) {
						return (-1);
					}
					if (!addstop (adr, val, "MEMORY", av[i][j])) {
						return (1);
					}
					trace_count = 0;
					multi = 1;
					i += 2;
					break;
				}
				else if (av[i][j] == 'r' || av[i][j] == 'R') {
					if (i + 2 >= ac) {
						printf ("bad arg count\n");
						return (-1);
					}
					if (!md_getregaddr (&reg, av[i + 1])) {
						printf ("%s: bad reg name\n", av[i + 1]);
						return (-1);
					}
					if (!get_rsa (&val, av[i + 2])) {
						return (-1);
					}
					if (!addstop (*reg, val, av[i + 1], av[i][j])) {
						return (1);
					}
					trace_count = 0;
					multi = 1;
					i += 2;
					break;
				}
				else if (av[i][j] == 'b') {
					trace_bflag = 1;
				}
				else if (av[i][j] == 'c') {
					trace_cflag = 1;
				}
				else if (av[i][j] == 'i') {
					trace_invalid = 1;
					trace_count = 0;
					multi = 1;
				}
				else {
					printf ("%c: unrecognized option\n", av[i][j]);
					return (-1);
				}
			}
		}
		else {
			if (n == 0) {
				if (!get_rsa ((u_int32_t *)&trace_count, av[i])) {
					return (-1);
				}
				multi = 1;
			}
			else {
				printf ("%s: unrecognized argument\n", av[i]);
				return (-1);
			}
			n++;
		}
	}

	if (setTrcbp (md_get_excpc(frame), trace_over))
		return (1);
	clrpchist ();
	if (multi)
		trace_mode = TRACE_TN;
	else
		trace_mode = TRACE_TB;
	store_trace_breakpoint ();
	printf("before go!\n");
	_go ();
	return(0); /* Shut up gcc */
}

/*************************************************************
 *  addstop(adr,val,name,sense)
 */
int
addstop (adr, val, name, sense)
	u_int32_t adr, val;
	char *name, sense;
{
	int i;

	for (i = 0; i < STOPMAX; i++) {
		if (stopval[i].addr == 0) {
			break;
		}
	}
	if (i >= STOPMAX) {
		printf ("stopval table full\n");
		return (0);
	}
	stopval[i].addr = adr;
	stopval[i].value = val;
	strcpy (stopval[i].name, name);
	if (sense == 'M' || sense == 'R') {
		stopval[i].sense = 1;
	}
	else {
		stopval[i].sense = 0;
	}
	return (1);
}

/*************************************************************
 *  setbp(ac,av) the 'b' (set breakpoint) command
 */
const Optdesc         b_opts[] = {
#ifdef R4000
	{"-d", "hw bpt for data access"},
	{"-r", "hw bpt for data read only"},
	{"-w", "hw bpt for data write only"},
#endif
	{"-s", "command string"},
	{0}
};

int
setbp (ac, av)
	int	ac;
	char	*av[];
{
	int32_t	adr, i, j, w, x;
	char	*str;
	int	flag = 0;

	if (ac == 1) {
		dspbpts ();
		return (0);
	}
	w = 0;
	str = 0;
	for (i = 1; i < ac; i++) {
		if (av[i][0] == '-') {
			x = 0;
			for (j = 1; av[i][j] != 0; j++) {
				if (av[i][j] == 's') {
					i++;
					if (i >= ac) {
						printf ("bad arg count\n");
						return (-1);
					}
					str = av[i];
					break;
				}
#ifdef R4000
		else if (av[i][j] == 'd')
		    w |= WATCH_R | WATCH_W;
		else if (av[i][j] == 'r')
		    w |= WATCH_R;
		else if (av[i][j] == 'w')
		    w |= WATCH_W;
#endif
				else {
					printf ("%c: unrecognized option\n", av[i][j]);
					return (-1);
				}
			}
		}
		else {
			flag = 1;
			if (!get_rsa (&adr, av[i])) {
				return (-1);
			}
		}
	}


	/*
	 *  Find a free breakpoint but reuse the same slot
	 *  if a break gets parameters changed.
	 */
	for (j = 0, i = MAX_BPT; j < MAX_BPT; j++) {
		if (Bpt[j].addr == adr) {
			Bpt[j].addr = NO_BPT;
			if (Bpt[j].cmdstr) {
				free (Bpt[j].cmdstr);
			}
			i = j;
		}
		if(i == MAX_BPT && Bpt[j].addr == NO_BPT) {
			i = j;
		}
	}
	if (MAX_BPT <= i) {
		printf ("too many breakpoints\n");
		return (1);
	}

	Bpt[i].addr = adr;
	printf ("Bpt %2d = %08x", i, adr);
	if (adr & 3L) {
		printf (" -> ??");
	}
	if (str != 0) {
		Bpt[i].cmdstr = malloc (strlen (str) + 1);
		strcpy (Bpt[i].cmdstr, str);
		str = 0;
		printf (" \"%s\"", Bpt[i].cmdstr);
	}
	else {
		Bpt[i].cmdstr = 0;
	}

	putchar ('\n');

	if (!flag) {
		printf ("break address not specified\n");
	}
	return (0);
}

/*************************************************************
 *  dspbpts() display all breakpoints
 */
void
dspbpts ()
{
	int	i, ln, siz;
	char	tmp[64], buf[100];

	siz = moresz;
	ioctl (STDIN, CBREAK, NULL);
	ln = siz;
	for (i = 0; i < MAX_BPT; i++)
		if (Bpt[i].addr != NO_BPT) {
			sprintf (buf, "Bpt %2d = %08x ", i, Bpt[i].addr);
			if (adr2symoff (tmp, Bpt[i].addr, 0)) {
				strcat (buf, tmp);
			}
			if (Bpt[i].addr & 3L) {
				strcat (buf, " -> ??");
			}
			if (Bpt[i].cmdstr) {
				sprintf (tmp, " \"%s\"", Bpt[i].cmdstr);
				strcat (buf, tmp);
			}
			if (more (buf, &ln, siz)) {
				break;
			}
		}
#ifdef R4000
	if (WatchLo & (WATCH_R | WATCH_W)) {
		register_t pa = WatchLo & WATCH_PA;
		printf ("Bpt %2d = %08x", MAX_BPT, pa);
		if (adr2symoff (tmp, pa, 0)) {
			printf (" %s", tmp);
		}
		printf (" [");
		if (WatchLo & WATCH_R) {
			printf (" read");
		}
		if (WatchLo & WATCH_W) {
			printf (" write");
		}
		printf (" ]\n");
	}
#endif
}

/*************************************************************
 *  clrbp(ac,av)
 *      The 'db' command
 */
int
clrbp (ac, av)
	int	ac;
	char	*av[];
{
	int32_t	i, j;

	if (ac > 1) {
		for (i = j = 0; j < ac - 1; j++) {
			if (strequ (av[1 + j], "*")) {
				clrbpts ();
				continue;
			}
			if (!atob (&i, av[1 + j], 10)) {
				printf ("%s: decimal number expected\n", av[1 + j]);
				return (-1);
			}
			if (i < MAX_BPT) {
				if (Bpt[i].addr == NO_BPT) {
					printf ("Bpt %2d not set\n", i);
				}
				else {
					Bpt[i].addr = NO_BPT;
					if (Bpt[i].cmdstr) {
						free (Bpt[i].cmdstr);
					}
				}
			}
#ifdef R4000
			else if (i == MAX_BPT) {
				if (WatchLo & (WATCH_R | WATCH_W)) {
					WatchLo = 0;
				}
				else {
					printf ("Bpt %2d not set\n", i);
				}

			}
#endif
			else {
				printf ("%d: breakpoint number too large\n", i);
			}
		}
	}
	else {
		dspbpts ();
	}
	return (0);
}

/*************************************************************
 *  rm_bpts()
 */
void
rm_bpts ()
{
	int	i;

	if (BptTmp.addr != NO_BPT && load_word (BptTmp.addr) == BPT_CODE) {
		store_word ((void *)BptTmp.addr, BptTmp.value);
	}

	for(i = 0; i < MAX_BPT; i++) {
		if(Bpt[i].addr != NO_BPT && load_word(Bpt[i].addr) == BPT_CODE) {
			store_word((void *)Bpt[i].addr, Bpt[i].value);
		}
	}
	remove_trace_breakpoint();
}

/*************************************************************
 *  clrbpts()
 */
void
clrbpts ()
{
	int	i;

	for (i = 0; i < MAX_BPT; i++) {
		Bpt[i].addr = NO_BPT;
		if (Bpt[i].cmdstr)
		free (Bpt[i].cmdstr);
	}
#if defined(R4000)
	WatchLo = 0;
#endif
}

/*************************************************************
 *  goclient()
 */
int
goclient ()
{
	if (is_break_point (md_getpc())) {
		if (setTrcbp (md_getpc(), 0)) {
			return (1);
		}
		trace_mode = TRACE_TG;
		store_trace_breakpoint ();
	}
	else {
		trace_mode = TRACE_GB;
		store_breakpoint ();
	}
	_go ();
	return(0); /* Never really gets here if _go gets called */
}

/*************************************************************
 * sstep()
 */
void
sstep ()
{

    if (setTrcbp (md_getpc(), 0))
	return;
    trace_mode = TRACE_DS;
    store_trace_breakpoint ();
    _go ();
}

/*************************************************************
 *  pmexception(cpc,cause)
 *      An (fatal) exception has been generated within PMON
 */

#if 0 /*XXX*/
pmexception (cpc, cause, ra, badva)
    unsigned int cpc, cause, ra, badva;
{
    extern char    *sbdexception();
    char 	   *exc;
    extern int     *curlst;
    int 	   i;

#ifdef INET
    /* lock out all "interrupts" */
    (void) splhigh();
#endif

    /* display exception on alpha-display before attempting to print */
    exc = 0;
    for (i = (curlst ? 1 : 5); i != 0; i--)
      exc = sbdexception (cpc, cause, ra, badva, exc);

    if (!curlst)
	tgt_reboot ();

    if (exc)
      printf ("\r\n%s\r\n", exc);
    else 
      printf ("\r\nPMON %s exception\r\n", md_getexcname (frame));

#ifdef R4000
    if (cause == EXC_CP2) {
	printf ("ErrPC:    %08x\r\n", cpc);
	printf ("CacheErr: %08x\r\n", badva);
	printf ("ECC:      %08x\r\n", get_ecc());
	tgt_reboot ();
    }
#endif

    printf ("CPC:      %08x\r\n", cpc);
    printf ("RA:       %08x\r\n", ra);
    printf ("Cause:    %08x\r\n", cause);
    switch (cause & CAUSE_EXCMASK) {
    case EXC_MOD:
    case EXC_TLBL:
    case EXC_TLBS:
    case EXC_ADEL:
    case EXC_ADES:
	printf ("BadVaddr: %08x\r\n", badva);
    }

#ifdef INET
    /* XXX something is going wrong when we try to reset the sonic interface */
    /* XXX for now we'll just completely reinitialise from scratch */
    /*reset_net ();*/
    tgt_reboot ();
#endif
    closelst (0);
    curlst = 0;
    main ();
printf("PMON EXCEPTION\n");
}
#endif


/*************************************************************
 *  exception()
 *      An exception has been generated within the client
 */
void
exception(frame)
	struct trapframe *frame;
{
	int exc_type;
	int i, flag;
	char tmp[80], *p = 0;
extern char    *sbddbgintr(unsigned int);

	TRPREG = *frame;	/* Copy and give up frame on stack */
	DBGREG = *frame;
	frame = &TRPREG;

	exc_type = md_exc_type(frame);

#if 0
    if (exc_type == EXC_INT && (p = sbddbgintr (Cause & Status)))
      printf ("\r\n%s Interrupt\r\n", p);

    if (trace_mode == TRACE_DC || trace_mode == TRACE_DS) {
	/* pass all exceptions to remote debugger */
	remove_trace_breakpoint ();
	dbgmode ();
    }
#endif

	if(exc_type != EXC_BPT && exc_type != EXC_TRC && exc_type != EXC_WTCH) {
		if (!p) {
			printf ("\r\nException Cause=%s, SR=0x%08x\r\n",
				md_getexcname(frame)), md_getsr();
		}
		stop (0);
	}
	else if (trace_mode == TRACE_NO) {	/* no bpts set */
		printf ("\r\nBreakpoint reached while not in trace mode!\r\n");
		stop (0);
	}
	else if (trace_mode == TRACE_GB) {	/* go & break */
		if (BptTmp.addr != NO_BPT) {
			store_word ((void *)BptTmp.addr, BptTmp.value);
			BptTmp.addr = NO_BPT;
		}
		for (i = 0; i < MAX_BPT; i++) {
			if (Bpt[i].addr != NO_BPT) {
				store_word ((void *)Bpt[i].addr, Bpt[i].value);
			}
		}
		if (exc_type == EXC_WTCH) {
			printf ("\r\nStopped at HW Bpt %d\r\n", MAX_BPT);
		}
		else {
			for (i = 0; i < MAX_BPT; i++) {
				if (Bpt[i].addr == md_get_excpc(frame)) {
					printf ("\r\nStopped at Bpt %d\n", i);
					stop (Bpt[i].cmdstr);
				}
			}
		}
		stop (0);
	}
    
	remove_trace_breakpoint ();
	if (trace_mode == TRACE_TB) {
		stop (0);		/* trace & break */
	}
	else if (trace_mode == TRACE_TN) {
		for (i = 0; i < MAX_BPT; i++) {
			if (Bpt[i].addr == md_get_excpc(frame)) {
				printf ("\r\nStopped at Bpt %d\r\n", i);
				stop (Bpt[i].cmdstr);
			}
		}
		if (trace_invalid && !is_validpc (md_get_excpc(frame))) {
			printf ("\r\nStopped: Invalid PC value\r\n");
			stop (0);
		}
		for (i = 0; i < STOPMAX; i++) {
			if (stopval[i].addr == 0) {
				continue;
			}
			if ((stopval[i].sense == 0 &&
				load_word (stopval[i].addr) == stopval[i].value)
				|| (stopval[i].sense == 1 &&
				load_word (stopval[i].addr) != stopval[i].value)) {
				if (stopval[i].sense == 0) {
					p = " == ";
				}
				else {
					p = " != ";
				}
				if (strequ (stopval[i].name, "MEMORY")) {
					printf ("\r\nStopped: 0x%08x%s0x%08x\r\n",
						stopval[i].addr, p, stopval[i].value);
				}
				else {
					printf ("\r\nStopped: %s%s0x%08x\r\n", stopval[i].name,
						p, stopval[i].value);
				}
				stop (0);
			}
		}
		flag = 1;
		if (trace_bflag || trace_cflag) {
			if (trace_bflag && md_is_branch ((int *)md_get_excpc(frame))) {
				flag = 1;
			}
			else if (trace_cflag && md_is_call ((int *)md_get_excpc(frame))) {
				flag = 1;
			}
			else {
				flag = 0;
			}
		}
		if (flag) {
			addpchist (md_get_excpc(frame));
			if (trace_verbose) {
				md_disasm (tmp, (void *)md_get_excpc(frame));
				printf ("%s\r\n", tmp);
#ifdef HAVE_DLYSLOT
				if (md_is_branch((void *)md_get_excpc(frame))) {
				/* print the branch delay slot too */
					md_disasm(tmp, (void *)md_get_excpc(frame) + 4);
					printf ("%s\r\n", tmp);
				}
#endif
			}
			else {
				dotik (256, 1);
			}
		}
		else {
			dotik (256, 1);
		}
		if (trace_count) {
			trace_count--;
		}
		if (trace_count == 1) {
			trace_mode = TRACE_TB;
		}
		if (setTrcbp (md_get_excpc(frame), trace_over)) {
			stop (0);
		}
		store_trace_breakpoint ();
		_go ();
	}
/* else TRACE_TG  trace & go, set on g or c if starting at bpt */

	trace_mode = TRACE_GB;	/* go & break */
	store_breakpoint ();
	_go ();
}

/*************************************************************
 *  stop(cmdstr)
 */
void
stop (cmdstr)
	char *cmdstr;
{
	char cmd[LINESZ];

	swlst (1);
	trace_mode = TRACE_NO;
	if (cmdstr) {
		strcpy (cmd, cmdstr);
	}
	else {
		strcpy (cmd, getenv ("brkcmd"));
	}
	do_cmd (cmd);
	main ();
}

/*************************************************************
 *  store_breakpoint()
 */
static void
store_breakpoint ()
{
    int             i;

	/*
	 *  Verify that a trace breakpoint won't
	 *  clash with a 'real' breakpoint. If
	 *  that is the case simply remove trace.
	 */
	for (i = 0; i < MAX_BPT; i++) {
		if (BptTmp.addr == Bpt[i].addr) {
			BptTmp.addr = NO_BPT;
		}
		if (BptTrc.addr == Bpt[i].addr) {
			BptTrc.addr = NO_BPT;
		}
		if (BptTrcb.addr == Bpt[i].addr) {
			BptTrcb.addr = NO_BPT;
		}
	}

	/*
	 *  Now do the same check with the trace breaks.
	 */
	if (BptTrc.addr == BptTmp.addr) {
		BptTrc.addr = NO_BPT;
	}
	if (BptTrcb.addr == BptTmp.addr || BptTrcb.addr == BptTrc.addr) {
		BptTrcb.addr = NO_BPT;
	}

	/*
	 *  Activate the 'real' breakpoints.
	 */
	for (i = 0; i < MAX_BPT; i++) {
		if (Bpt[i].addr != NO_BPT) {
			Bpt[i].value = load_word (Bpt[i].addr);
			store_word ((void *)Bpt[i].addr, BPT_CODE);
		}
	}
	if (BptTmp.addr != NO_BPT) {
		BptTmp.value = load_word (BptTmp.addr);
		store_word ((void *)BptTmp.addr, BPT_CODE);
	}
	store_trace_breakpoint ();
}

/*
 *  When doing single step tracing we need to deal with different
 *  aproaches. Some arches have a trace function while others don't
 *  have HW support for it. The following two functions either embed
 *  the traced instruction with a breakpoint or uses the HW trace.
 *  
 *  store_trace_breakpoint()
 *  remove_trace_breakpoint()
 */
static void
store_trace_breakpoint ()
{
#ifdef HAVE_TRACE
	if (BptTrc.addr != NO_BPT) {
		md_settrace();
	}
#else
	if (BptTrc.addr != NO_BPT) {
		BptTrc.value = load_word (BptTrc.addr);
		store_word ((void *)BptTrc.addr, BPT_CODE);
	}
	if (BptTrcb.addr != NO_BPT) {
		BptTrcb.value = load_word (BptTrcb.addr);
		store_word ((void *)BptTrcb.addr, BPT_CODE);
	}
#endif
}

static void
remove_trace_breakpoint ()
{
#ifndef HAVE_TRACE
	if (BptTrc.addr != NO_BPT && load_word (BptTrc.addr) == BPT_CODE) {
		store_word ((void *)BptTrc.addr, BptTrc.value);
	}
	if (BptTrcb.addr != NO_BPT && load_word (BptTrcb.addr) == BPT_CODE) {
		store_word ((void *)BptTrcb.addr, BptTrcb.value);
	}

	BptTrc.addr = NO_BPT;
	BptTrcb.addr = NO_BPT;
#endif
}

/*************************************************************
 *  int is_break_point(adr)
 */
static int 
is_break_point (adr)
     int32_t            adr;
{
    int             i;

    for (i = 0; i < MAX_BPT; i++)
	if (Bpt[i].addr == adr)
	    return (1);
    if (BptTmp.addr == adr)
	return (1);
    return (0);
}

#define NVALIDPC	10
static unsigned long	validpc[NVALIDPC];
static int		nvalidpc = -1;


void
flush_validpc ()
{
    nvalidpc = -1;
}


/* chg_validpc: called if variable is changed */
int
chg_validpc (name, value)
    char *name, *value;
{
    char           *av[NVALIDPC], tmp[80];

    strcpy (tmp, value);
    if (argvize (av, tmp) % 2 != 0) {
	printf ("validpc variable must have even number of values\n");
	return (0);
    }
    /* don't check the values here, symbols may not be loaded */
    flush_validpc ();
    return (1);
}


static void
compute_validpc ()
{
    char           *av[NVALIDPC], tmp[80];
    int		    ac, i;

    strcpy (tmp, getenv ("validpc"));
    ac = argvize (av, tmp);
    nvalidpc = 0;
    
    for (i = 0; i < ac; i += 2) {
	if (!get_rsa ((u_int32_t *)&validpc[nvalidpc], av[i]))
	  continue;
	if (!get_rsa ((u_int32_t *)&validpc[nvalidpc+1], av[i+1]))
	  continue;
	nvalidpc += 2;
    }
}


/*************************************************************
 *  is_validpc(adr)
 */
static int
is_validpc (adr)
    int32_t adr;
{
    int         i;

    if (nvalidpc < 0)
      compute_validpc ();

    for (i = 0; i < nvalidpc; i += 2)
      if ((u_int32_t)adr >= validpc[i] && (u_int32_t)adr < validpc[i+1])
	return (1);

    return (0);
}


/*************************************************************
 *  addpchist(adr)
 */
static void
addpchist (adr)
    int   adr;
{
    pchist_d[pchist_ip] = adr;
    pchist_ip = incmod (pchist_ip, PCHISTSZ);
    if (pchist_ip == pchist_op)
	pchist_op = incmod (pchist_op, PCHISTSZ);
}

/*************************************************************
 *  clrpchist()
 */
static void
clrpchist ()
{
    pchist_ip = pchist_op = 0;
}

/*************************************************************
 *  unsigned long getpchist(n)
 */
u_int32_t 
getpchist (n)
     int             n;
{
    int             i;

    i = pchist_ip - n - 1;
    if (i < 0)
	i += PCHISTSZ + 1;
    if (incmod (i, PCHISTSZ) == pchist_op)
	return (0);
    return (pchist_d[i]);
}

/*************************************************************
 *  setTrcbp(adr,stepover)
 */
static int
setTrcbp (adr, stepover)
     int32_t	     adr;
     int             stepover;
{

	BptTrc.addr = NO_BPT;
	BptTrcb.addr = NO_BPT;
#ifdef HAVE_TRACE
	BptTrc.addr = adr + 4;
#else
	if (md_is_branch((void *)adr)) {
		unsigned long   target;
#if 0
		if (md_is_branch ((void *)adr + 4)) {
			printf ("branch in delay slot\n");
			return (1);
		}
#endif
		target = md_branch_target((void *)adr);
		if (target == adr)
			target = adr + 8;	/* skip self branches */
		if(md_is_cond_branch((void *)adr) && target != adr + 8)
			BptTrc.addr = adr + 8;
		if(md_is_call((void *)adr) && stepover)
			BptTrc.addr = adr + 8;
		else if(md_is_jr((void *)adr) && !md_is_writeable((void *)target))
			BptTrc.addr = md_getlink();
		else
			BptTrcb.addr = target;
	}
	else {
		BptTrc.addr = adr + 4;
	}
#endif /* HAVE_TRACE */
    return (0);
}

