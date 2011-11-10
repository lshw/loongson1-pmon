/* $Id: termio.c,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */

#include <sys/types.h>
#include <stdlib.h>
#include <queue.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <termio.h>
#include <unistd.h>
#include <setjmp.h>
#include <sys/ioctl.h>
#include <pmon/netio/netio.h>
#include <pmon.h>

int _read __P((int, char *, int));
int _write __P((int, char *, int));
int  _open __P((char *, int, int));
int _close __P((int));
static void _chwrite __P((DevEntry *, char));

extern void    gsignal __P((struct jmp_buf *, int sig));
extern ConfigEntry ConfigTable[];

DevEntry        DevTable[DEV_MAX];

File            _file[OPEN_MAX] =
{
#ifdef NETIO
    {0, 1, -1},
    {0, 1, -1},
    {0, 1, -1}
#else
    {0, 1},
    {0, 1},
    {0, 1}
#endif
};

typedef int tFunc __P((int, int, int, int));

struct TermEntry {
    char           *name;
    tFunc          *func;
};

struct TermEntry TermTable[] =
{
#if 0
    {"tvi920", tvi920},
#endif
    {"vt100", vt100},
    {0}};

int            *curlst;		/* list of files open in the current context */

static void
setsane(DevEntry *p)
{
	p->t.c_iflag |= (ISTRIP | ICRNL | IXON);
	p->t.c_oflag = (ONLCR);
	p->t.c_lflag = (ICANON | ISIG | ECHO | ECHOE);
	p->t.c_cc[VINTR] = CNTRL ('c');
	p->t.c_cc[VEOL] = '\n';
	p->t.c_cc[VEOL2] = CNTRL ('c');
	p->t.c_cc[VERASE] = CNTRL ('h');
	p->t.c_cc[V_STOP] = CNTRL ('s');
	p->t.c_cc[V_START] = CNTRL ('q');
}

void
reschedule (p)
     char           *p;
{
    scandevs ();
}

#define reschedule(p)    scandevs()


/** _write(fd,buf,n) write n bytes from buf to fd */
int
_write (fd, buf, n)
     int             fd, n;
     char           *buf;
{
    int             i;
    DevEntry       *p;


    if (!_file[fd].valid)
        return (-1);

#ifdef NETIO
    if (_file[fd].netfd >= 0)
      return netwrite (_file[fd].netfd, buf, n);
#endif

    p = &DevTable[_file[fd].dev];
    for (i = 0; i < n; i++) {
	if (p->t.c_oflag & ONLCR && buf[i] == '\n') {
	    _chwrite (p, '\r');
	}
	_chwrite (p, buf[i]);
	scandevs ();
    }
    return (i);
}

static void
_chwrite (p, ch)
     DevEntry       *p;
     char            ch;
{

    while (p->txoff)
	reschedule ("write txoff");
    if (p->handler) {
	while (!(*p->handler) (OP_TXRDY, p->sio, p->chan, NULL))
	  reschedule ("write txrdy");
	(*p->handler) (OP_TX, p->sio, p->chan, ch);
#if 0
    } else {
	/* early error? use main console (and hope someone has programmed it!) */
	ConfigEntry *q = &ConfigTable[0];
	while (!(*q->handler) (OP_TXRDY, q->devinfo, q->chan))
	  continue;
	(*q->handler) (OP_TX, q->devinfo, q->chan, ch);
#endif
    }
}

void
scandevs ()
{
    int             c, n;
    DevEntry       *p;

    for (p = DevTable; p->rxq; p++) {
	while ((*p->handler) (OP_RXRDY, p->sio, p->chan, NULL)) {
	    c = (*p->handler) (OP_RX, p->sio, p->chan, NULL);
	    if (p->t.c_iflag & ISTRIP)
		c &= 0x7f;
	    if (p->t.c_lflag & ISIG) {
		if (c == p->t.c_cc[VINTR]) {
		    gsignal (p->intr/*XXX*/, 2 /*SIGINT*/);
		    continue;
		}
	    }
	    if (p->t.c_iflag & IXON) {
		if (p->t.c_iflag & IXANY && p->txoff) {
		    p->txoff = 0;
		    continue;
		}
		if (c == p->t.c_cc[V_STOP]) {
		    p->txoff = 1;
		    continue;
		}
		if (c == p->t.c_cc[V_START]) {
		    p->txoff = 0;
		    continue;
		}
	    }

	    n = Qspace (p->rxq);
	    if (n > 0) {
		Qput (p->rxq, c);
		if (n < 20 && !p->rxoff) {
		    (*p->handler) (OP_RXSTOP, p->sio, p->chan, p->rxoff = 1);
		    if (p->t.c_iflag & IXOFF)
			_chwrite (p, CNTRL ('S'));
		}
	    }
	}
    }
    tgt_poll ();
#ifdef NETIO
    tgt_netpoll ();
#endif
}

/** ioctl(fd,op,argp) perform control operation on fd */

int
ioctl (int fd, unsigned long op, ...)
{
    DevEntry *p;
    struct termio *at;
    int i;
    void *argp;
    va_list ap;

    va_start(ap, op);
    argp = va_arg(ap, void *);
    va_end(ap);
    if (!_file[fd].valid)
        return (-1);

#ifdef NETIO
    if (_file[fd].netfd >= 0)
      return netioctl (_file[fd].netfd, op, argp);
#endif

    if (_file[fd].dev < 0)
	return (-1);
    p = &DevTable[_file[fd].dev];

    switch (op) {
    case TCGETA:
	*(struct termio *)argp = p->t;
	break;
    case TCSETAF:		/* after flush of input queue */
	while (!Qempty (p->rxq))
	    Qget (p->rxq);
	(*p->handler) (OP_FLUSH, p->sio, p->chan, 1);
	if (p->rxoff) {
	    (*p->handler) (OP_RXSTOP, p->sio, p->chan, p->rxoff = 0);
	    if (p->t.c_iflag & IXOFF)
		_chwrite (p, CNTRL ('Q'));
	}
    case TCSETAW:		/* after write */
	/* no txq, so no delay needed */
	at = (struct termio *)argp;
	if (p->t.c_ispeed != at->c_ispeed) {	/* XXX FIX Split speed */
		if ((*p->handler) (OP_BAUD, p->sio, p->freq, at->c_ispeed))
		    return (-1);
	}
	p->t = *at;
	break;
    case FIONREAD:
	scandevs ();
	*(int *)argp = Qused (p->rxq);
	break;
    case SETINTR:
	p->intr = (struct jmp_buf *) argp;
	break;
    case GETINTR:
	*(struct jmp_buf **) argp = p->intr;
	break;
    case SETSANE:
	(*p->handler) (OP_RESET, p->sio, p->chan, 0);
	setsane(p);
	break;
    case SETNCNE:
	if (argp)
	  *(struct termio *)argp = p->t;
	p->t.c_lflag &= ~(ICANON | ECHO | ECHOE);
	p->t.c_cc[4] = 1;
	break;
    case CBREAK:
	if (argp)
	  *(struct termio *)argp = p->t;
	p->t.c_lflag &= ~(ICANON | ECHO);
	p->t.c_cc[4] = 1;
	break;
    case GETTERM:
	*(int *)argp = 0;
	if (p->tfunc == 0)
	    return (-1);
	strcpy ((char *)argp, p->tname);
	break;
    case SETTERM:
	for (i = 0; TermTable[i].name; i++) {
	    if (!strcmp(argp, TermTable[i].name))
		break;
	}
	if (TermTable[i].name == 0)
	    return (-1);
	p->tname = TermTable[i].name;
	p->tfunc = TermTable[i].func;
	break;
    case TERMTYPE:
	if (TermTable[fd].name == 0)
	    return (-1);
	strcpy ((char *)argp, TermTable[fd].name);
	return (fd + 1);
    default:
	return (-1);
    }
    return (0);
}

int
devinit ()
{
    int             i, brate;
    ConfigEntry    *q;
    DevEntry       *p;
    char	   dname[5];
    char	   *s;

    strcpy (dname, "ttyx");
    for (i = 0; ConfigTable[i].devinfo && i < DEV_MAX; i++) {
	q = &ConfigTable[i];
	p = &DevTable[i];
	p->txoff = 0;
	p->rxoff = 0;
	if (q->chan == 0)
	    (*q->handler) (OP_INIT, q->devinfo, 0, q->rxqsize);
	p->qsize = q->rxqsize;
	p->rxq = Qcreate (p->qsize);
	if (p->rxq == 0)
	    return (-1);
	dname[3] = (i < 10) ? i + '0' : i - 10 + 'a';

	if (!(s = getenv (dname)) || (brate = getbaudrate (s)) == 0)
	    brate = q->brate;

	/* set default baudrate in case anything goes wrong */
	(void) (*q->handler) (OP_BAUD, q->devinfo, q->freq, q->brate);

	/*
	 * program requested baud rate, but fall back to default
	 * if there is a problem
	 */
	if (brate != q->brate) {
	    if ((*q->handler) (OP_BAUD, q->devinfo, q->freq, brate))
		brate = q->brate;
	}

	p->sio = q->devinfo;
	p->chan = q->chan;
	p->handler = q->handler;
	p->intr = 0;
	p->tfunc = 0;
	p->t.c_cflag = 0;
	p->t.c_ispeed = brate;
	p->t.c_ospeed = brate;
	setsane(p);
	_chwrite (p, CNTRL ('Q'));
    }
    return (0);
}

#if 0
/** ttctl(fd,op,a1,a2) perform terminal specific operation */
static int
ttctl (fd, op, a1, a2)
     int             fd, op, a1, a2;
{
    DevEntry       *p;
    int             r;

    if (!_file[fd].valid || _file[fd].dev < 0)
        return (-1);

    p = &DevTable[_file[fd].dev];
    if (p->tfunc == 0)
	return (-1);
    r = (*p->tfunc) (fd, op, a1, a2);
    return (r);
}
#endif

/** _open(fname,mode,perms) return fd for fname */
int
_open (fname, mode, perms)
     char           *fname;
     int             mode, perms;
{
    int             i, c, dev;
    char            *dname;

    for (i = 0; i < OPEN_MAX && _file[i].valid; i++);
    if (i == OPEN_MAX)
	return (-1);

    dname = fname;
    if (strncmp (dname, "/dev/", 5) == 0)
      dname += 5;

    if (strlen (dname) == 4 && strncmp (dname, "tty", 3) == 0) {
	c = dname[3];
	if (c >= 'a' && c <= 'z')
	  dev = c - 'a';
	else if (c >= 'A' && c <= 'Z')
	  dev = c - 'A';
	else if (c >= '0' && c <= '9')
	  dev = c - '0';
	if (dev >= DEV_MAX || DevTable[dev].rxq == 0)
	  return (-1);
	_file[i].dev = dev;
#ifdef NETIO
	_file[i].netfd = -1;
#endif
	_file[i].valid = 1;
	{
	    DevEntry *p = &DevTable[dev];
	    (*p->handler) (OP_OPEN, p->sio, p->chan, 0);
	}
	ioctl (i, SETSANE);
    } else {
#ifdef NETIO
	if ((_file[i].netfd = netopen (fname, mode)) < 0)
	  return (-1);
	_file[i].dev = -1;
	_file[i].valid = 1;
#else
	return (-1);
#endif
    }
    if (curlst)
	*curlst |= (1 << i);
    return (i);
}

/** _close(fd) close fd */
int
_close (fd)
     int             fd;
{
    if (_file[fd].valid) {
#ifdef NETIO
	if (_file[fd].netfd >= 0) {
	    netclose (_file[fd].netfd);
	    _file[fd].netfd = -1;
	}
	else
#endif
	{
	    DevEntry *p = &DevTable[_file[fd].dev];
	    (*p->handler) (OP_CLOSE, p->sio, p->chan, 0);
	}
	_file[fd].valid = 0;
	if (curlst)
	  *curlst &= ~(1 << fd);
    }
    return(0);
}

/** _read(fd,buf,n) read n bytes into buf from fd */
int
_read (fd, buf, n)
     int             fd, n;
     char           *buf;
{
    int             i, used;
    DevEntry       *p;
    char            ch;

    if (!_file[fd].valid)
        return (-1);

#ifdef NETIO
    if (_file[fd].netfd >= 0)
      return netread (_file[fd].netfd, buf, n);
#endif

    p = &DevTable[_file[fd].dev];
    for (i = 0; i < n;) {
	scandevs ();
	while ((used = Qused (p->rxq)) == 0)
	    reschedule ("read Qempty");
	if (used < 20 && p->rxoff) {
	    (*p->handler) (OP_RXSTOP, p->sio, p->chan, p->rxoff = 0);
	    if (p->t.c_iflag & IXOFF)
		_chwrite (p, CNTRL ('Q'));
	}
	ch = Qget (p->rxq);
	if (p->t.c_iflag & ICRNL && ch == '\r')
	    ch = '\n';
	if (p->t.c_lflag & ICANON) {
	    if (ch == p->t.c_cc[VERASE]) {
		if (i > 0) {
		    i--;
		    if (p->t.c_lflag & ECHOE)
		      write (fd, "\b \b", 3);
		    else if (p->t.c_lflag & ECHO)
		      write (fd, "\b", 1);
		}
		continue;
	    }
	    if (p->t.c_lflag & ECHO)
	      write (fd, &ch, 1);
	    buf[i++] = ch;
	    if (ch == p->t.c_cc[VEOL] || ch == p->t.c_cc[VEOL2])
	      break;
	} else {
	    buf[i++] = ch;
	}
    }
    return (i);
}
