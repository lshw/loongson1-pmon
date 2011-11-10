/*	$Id: termio.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2002 Opsycon AB  (www.opsycon.se)
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
#include <sys/param.h>
#include <stdlib.h>
#include <queue.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <termio.h>
#include <unistd.h>
#include <setjmp.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <file.h>
#include <pmon.h>
#include "mod_vgacon.h"
#include "mod_usb_kbd.h"

int term_open __P((int, const char *, int, int));
int term_close __P((int));
int term_read __P((int, void *, size_t));
int term_write __P((int, const void *, size_t));
int term_ioctl (int fd, unsigned long op, ...);
static void chwrite __P((DevEntry *, char));

extern void    gsignal __P((struct jmp_buf *, int sig));
extern ConfigEntry ConfigTable[];
extern int errno;
extern void kbd_poll(void);

DevEntry        DevTable[DEV_MAX];

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

struct TermDev {
      int dev;
};

#ifdef INPUT_FROM_BOTH
int input_from_both=1;
#else
int input_from_both=0;
#endif
#ifdef OUTPUT_TO_BOTH
int output_to_both=1;
#else
int output_to_both=0;
#endif


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

extern void dbglednum(int);

int
term_write (int fd, const void *buf, size_t nchar)
{
	DevEntry       *p;
	struct TermDev *devp;
	char *buf2 = (char *)buf;
	int i, n;
	int dsel;
	int count;
	
	devp = (struct TermDev *)_file[fd].data;
	p = &DevTable[devp->dev];
	n = nchar;
	dsel=devp->dev;

do
{
	p = &DevTable[dsel];
	buf2 = (char *)buf;
	n = nchar;

	while (n > 0) {
		/* << LOCK >> */
		while(!tgt_smplock());

		i = Qspace (p->txq);
		while (i > 2 && n > 0) {
			if ((p->t.c_oflag & ONLCR) && *buf2 == '\n') {
				Qput(p->txq, '\r');
				i--;
			}
			Qput(p->txq, *buf2++);
			n--;
			i--;
		}
		tgt_smpunlock();
		/* << UNLOCK >> */

		while (Qused(p->txq)) {
			scandevs();
		}
	}
dsel=DevTable[dsel+1].handler?dsel+1:0;
}while(dsel!=devp->dev && output_to_both);

	return (nchar);
}

/*
 * High priority write. Just get the char out!
 */
static void
chwrite (DevEntry *p, char ch)
{
	while (!(*p->handler) (OP_TXRDY, p, NULL, NULL));
	(*p->handler) (OP_TX, p, NULL, ch);
}
/* yh
 * */
int dummy; 
void
scandevs ()
{
	int		s, c, n;
	DevEntry	*p;


	for (p = DevTable; p->rxq; p++) {
		/* << LOCK >> */
		while(!tgt_smplock());

		/* Read queue */
		while ((*p->handler) (OP_RXRDY, p, NULL, NULL)) {
			c = (*p->handler) (OP_RX, p, NULL, NULL);
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
					(*p->handler) (OP_RXSTOP, p, NULL, p->rxoff = 1);
					if (p->t.c_iflag & IXOFF)
						chwrite (p, CNTRL ('S'));
				}
			}
			else break;
		}

		/* Write queue */
		n = Qused (p->txq);
		while (n > 0 && !p->txoff &&
		  (*p->handler)(OP_TXRDY, p, NULL, NULL)) {
			char c = Qget(p->txq);
			(*p->handler) (OP_TX, p, NULL, c);
			n--;
		}

		tgt_smpunlock();
		/* << UNLOCK >> */
	}

#if defined(SMP)
	if (tgt_smpwhoami() != 0)
		return;		/* Only CPU 1 is allowed to run kernel */
#endif
#if 1	//delay 10 micro seconds	
	{
		int i;
		for(dummy=1,i=0; i<200; i++){
			dummy*=i;
		}
	}	
#endif
	s = splhigh();		/* Trigg the polled interrupt system */
	splx(s);

#ifdef NETIO
	tgt_netpoll ();		/* XXX this one should die! */
#endif
#if NMOD_VGACON >0
#if NMOD_USB_KBD >0
	if (usb_kbd_available)
		usb_kbd_poll();
#endif
	if (kbd_available)
		kbd_poll();
#endif
}

/** ioctl(fd,op,argp) perform control operation on fd */

int
term_ioctl (int fd, unsigned long op, ...)
{
	DevEntry *p;
	struct termio *at;
	int i;
	void *argp;
	va_list ap;
	struct TermDev	       *devp;
	
	devp = (struct TermDev *)_file[fd].data;

	va_start(ap, op);
	argp = va_arg(ap, void *);
	va_end(ap);

	if (devp->dev < 0)
		return -1;
	p = &DevTable[devp->dev];

	switch (op) {
		case TCGETA:
			*(struct termio *)argp = p->t;
			break;
		case TCSETAF:		/* after flush of input queue */
			while (!Qempty (p->rxq))
				Qget (p->rxq);
			(*p->handler) (OP_FLUSH, p, NULL, 1);
			if (p->rxoff) {
				(*p->handler) (OP_RXSTOP, p, NULL, p->rxoff = 0);
				if (p->t.c_iflag & IXOFF)
					chwrite (p, CNTRL ('Q'));
			}
		case TCSETAW:		/* after write */
			/* no txq, so no delay needed */
			at = (struct termio *)argp;
			if (p->t.c_ispeed != at->c_ispeed) {
				if ((*p->handler) (OP_BAUD, p, NULL, at->c_ispeed))
					return -1;
			}
			p->t = *at;
			break;
		case FIONREAD:
			scandevs ();
	if(input_from_both)
	{
	int dsel,total=0;
	for(dsel=0;;dsel++)
	{
	p = &DevTable[dsel];
	if(!p->handler)break;
	total+= Qused (p->rxq);
	}
			*(int *)argp = total;
	}
	else
			*(int *)argp = Qused (p->rxq);
			break;
		case SETINTR:
			p->intr = (struct jmp_buf *) argp;
			break;
		case GETINTR:
			*(struct jmp_buf **) argp = p->intr;
			break;
		case SETSANE:
			(*p->handler) (OP_RESET, p, NULL, 0);
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
		default:
			break;
	}
	return 0;
}

int
devinit (void)
{
	int i, brate;
	ConfigEntry *q;
	DevEntry *p;
	char dname[10];
	char *s;
	
	strcpy(dname, "tty_baud");
	for (i = 0; ConfigTable[i].devinfo && i < DEV_MAX; i++) {
		q = &ConfigTable[i];
		p = &DevTable[i];

		p->txoff = 0;
		p->qsize = q->rxqsize;
		p->sio = q->devinfo;
		p->chan = q->chan;
		p->rxoff = 0;
		p->handler = q->handler;
		p->tfunc = 0;
		p->freq = q->freq;
		p->nopen = 0;
		
		if (p->chan == 0)
			(*p->handler) (OP_INIT, p, NULL, q->rxqsize);

		p->rxq = Qcreate (p->qsize);
		p->txq = Qcreate (p->qsize);
		if (p->rxq == 0 || p->txq == 0)
			return (-1);

		/*
		 * program requested baud rate, but fall back to default
		 * if there is a problem
		 */
/* XXX don't work. env init is not called yet. has to be solved */
		dname[3] = (i < 10) ? i + '0' : i - 10 + 'a';
		if ((s = getenv(dname)) == 0 || (brate = getbaudrate(s)) == 0)
			brate = q->brate;

		if ((brate != q->brate)||(q->flag&1)) {
			if ((*p->handler)(OP_BAUD, p, NULL, brate)) {
				brate = q->brate;
				(void)(*p->handler)(OP_BAUD, p, NULL, brate);
			}
		}

		p->t.c_ispeed = brate;
		p->t.c_ospeed = brate;
	}
	return (0);
}

/* term_open(fname,mode,perms) return fd for fname */
int
term_open(int fd, const char *fname, int mode, int perms)
{
	int c, dev;
	char *dname;
	struct TermDev *devp;
	DevEntry *p;

	dname = (char *)fname;
	if (strncmp (dname, "/dev/", 5) == 0)
		dname += 5;

	if (strlen (dname) == 4 && strncmp (dname, "tty", 3) == 0) {
		c = dname[3];
		if (c >= 'a' && c <= 'z')
			dev = c - 'a' + 10;
		else if (c >= 'A' && c <= 'Z')
			dev = c - 'A' + 10;
		else if (c >= '0' && c <= '9')
			dev = c - '0';
		if (dev >= DEV_MAX || DevTable[dev].rxq == 0)
			return  -1;

		devp = (struct TermDev *)malloc(sizeof(struct TermDev));
		if (devp == NULL) {
			errno = ENOMEM;
			return -1;
		}
		devp->dev = dev;
		_file[fd].data = (void *)devp;
		p = &DevTable[dev];
		if(p->handler){
		(*p->handler) (OP_OPEN, p, NULL, 0);
		if (p->nopen++ == 0)
			term_ioctl (fd, SETSANE);
			}
	}
	else {
		return -1;
	}

	return fd;
}

/** term_close(fd) close fd */
int
term_close(int fd)
{
	DevEntry *p = &DevTable[(((struct TermDev *)_file[fd].data))->dev];

	(*p->handler) (OP_CLOSE, p, NULL, 0);
	if (p->nopen)
		p->nopen--;
	free(_file[fd].data);
	return 0;
}
/** term_read(fd,buf,n) read n bytes into buf from fd */
int
term_read (fd, buf, n)
     int        fd;
     size_t	n;
     void       *buf;
{
	int i, used;
	DevEntry *p;
	char ch;
	struct TermDev *devp;
	char *buf2 = buf;
	int dsel;
	DevEntry *p0;

	devp = (struct TermDev *)_file[fd].data;
	p0 = p = &DevTable[devp->dev];
	dsel=devp->dev;


	for (i = 0; i < n;) {
		scandevs ();
	if(input_from_both)
	{
	p = &DevTable[dsel];
	if(!p->handler){dsel=0;continue;}
	else dsel++;
	}

		/* << LOCK >> */
		while(!tgt_smplock());
		if ((used = Qused (p->rxq)) == 0) {
			tgt_smpunlock();
			continue;
		}
		if (used < 20 && p->rxoff) {
			(*p->handler) (OP_RXSTOP, p, NULL, p->rxoff = 0);
			if (p->t.c_iflag & IXOFF)
				chwrite (p, CNTRL ('Q'));
		}
		ch = Qget (p->rxq);
		tgt_smpunlock();
		/* << UNLOCK >> */

		if (p->t.c_iflag & ICRNL && ch == '\r')
			ch = '\n';
		if (p0->t.c_lflag & ICANON) {
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
			if (p0->t.c_lflag & ECHO)
				write (fd, &ch, 1);
			buf2[i++] = ch;
			if (ch == p->t.c_cc[VEOL] || ch == p->t.c_cc[VEOL2])
				break;
		} else {
			buf2[i++] = ch;
		}
	}


	return i;
}


static FileSystem termfs =
{
	"tty", FS_TTY,
	term_open,
	term_read,
	term_write,
	NULL,
	term_close,
	term_ioctl
};

static void init_fs __P((void)) __attribute__ ((constructor));

static void
   init_fs()
{
	SBD_DISPLAY ("DEVI", CHKPNT_DEVI);
	devinit ();

	/*
	 * Install terminal based file system.
	 */
	filefs_init(&termfs);

	/*
	 * Create the standard i/o files the proper way
	 */
	_file[0].valid = 1;
	_file[0].fs = &termfs;
	_file[1].valid = 1;
	_file[1].fs = &termfs;
	_file[2].valid = 1;
	_file[2].fs = &termfs;
	_file[3].valid = 1;
	_file[3].fs = &termfs;
	_file[4].valid = 1;
	_file[4].fs = &termfs;
	_file[5].valid = 1;
	_file[5].fs = &termfs;
	term_open(0, "/dev/tty0", 0, 0); /* stdin */
	term_open(1, "/dev/tty0", 0, 0); /* stdout */
	term_open(2, "/dev/tty0", 0, 0); /* stderr */
	term_open(3, "/dev/tty1", 0, 0); /* kbdin */
	term_open(4, "/dev/tty1", 0, 0); /* vgaout */
	term_open(5, "/dev/tty2", 0, 0); /* vgaout */
}

void *restdout(int  (*newwrite) (int fd, const void *buf, size_t n))
{
int  (*oldwrite) (int fd, const void *buf, size_t n);
 oldwrite=_file[stdout->fd].fs->write;
 _file[stdout->fd].fs->write=newwrite;
return oldwrite;
}

#define MAXLEN 1024
char *regets(char *buf,int len)
{
static char str[MAXLEN+1]="";
static char line[MAXLEN+1]="";
char *p;
if(len)
{
int n,oldlen,newlen;
oldlen=strlen(str);
n=min(MAXLEN-oldlen,len);
newlen=oldlen+n;
memcpy(&str[oldlen],buf,n);
str[newlen]=0;
}

if((p=strchr(str,'\n')))
{
strncpy(line,str,p-str+1);
line[p-str+1]=0;
strcpy(str,p+1);
return line;
}
else return 0;
}

static int (*oldwrite)(int fd,char *buf,int len)=0;
int  (*filter)(char *s)=0;
static int newwrite(int fd,char *buf,int len)
{
char *s;
s=regets(buf,len);
while(s)
{
filter(s);
s=regets(0,0);
}
return oldwrite(fd,buf,len);
}

void *filterstdout(int  (*f)(char *s))
{
filter=f;
if(oldwrite) {restdout(oldwrite);oldwrite=0;}
else {oldwrite=restdout(newwrite);}
}
