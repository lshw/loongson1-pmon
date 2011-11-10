/*	$Id: kern_misc.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2000 Opsycon AB  (www.opsycon.se)
 * Copyright (c) 2000 Rtmx, Inc   (www.rtmx.com)
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
 *	This product includes software developed for Rtmx, Inc by
 *	Opsycon Open System Consulting AB, Sweden.
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

/*
 *  Miscelaneous code requiered by the kernel code.
 */

#include <sys/param.h>
#include <sys/syslog.h>
#include <sys/queue.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>
#include <sys/buf.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/netisr.h>

#include <machine/stdarg.h>
#include <machine/intr.h>

#include <pmon.h>
#include <linux/io.h>
#undef MYPRINT

#define PORT3fd (mips_io_port_base+0x3fd)
#define PORT3f8 (mips_io_port_base+0x3f8)

int hz = 100;
int securelevel = -1;
int cmask = 0777;
vm_size_t page_mask = (NBPG-1);
struct timeval boottime;
static u_char *kmem;
vm_offset_t kmem_offs;

static void ifpoll __P((void));
#ifdef MYPRINT
void printstr(char *);
void printnum(unsigned long long);
#endif
int
copyin (src, dest, cnt)
	const void *src;
	void *dest;
	size_t cnt;
{
	memcpy (dest, src, cnt);
	return 0;
}


int
copyout(src, dest, size)
	const void *src;
	void *dest;
	size_t size;
{
	memcpy(dest, src, size);
	return(0);
}

int
uiomove(cp, n, uio)
	caddr_t cp;
	int n;
	struct uio *uio;
{
	struct iovec *iov;
	u_int cnt;

	while (n > 0 && uio->uio_resid) {
		iov = uio->uio_iov;
		cnt = iov->iov_len;
		if (cnt == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			continue;
		}
		if (cnt > n) {
			cnt = n;
		}
		if (uio->uio_rw == UIO_READ) {
			memcpy (iov->iov_base, cp, cnt);
		}
		else {
			memcpy (cp, iov->iov_base, cnt);
		}
		iov->iov_base += cnt;
		iov->iov_len -= cnt;
		uio->uio_resid -= cnt;
		uio->uio_offset += cnt;
		cp += cnt;
		n -= cnt;
	}
	return 0;
}

void
panic(msg)
	const char *msg;
{
	printf("PMON PANIC '%s'\n", msg);
	md_do_stacktrace(NULL, -1, -1, 0);
	tgt_reboot();
	while(1);
}

extern int sysloglevel;

static const char *const priname[] = {
	"\nEMERG",
	"\nALERT",
	"\nCRIT",
	"\nERROR",
	"\nWARNING",
	"\nNOTICE",
	"INFO",
	"DEBUG",
};


void
log (int kind, const char *fmt, ...)
{
	va_list arg;
	int pri = kind & LOG_PRIMASK;

	if (pri > sysloglevel)
		return;

	printf ("%s: ", priname[pri]);
	va_start(arg, fmt);
	vprintf (fmt, arg);
	va_end(arg);
}

void
tablefull (name)
	const char *name;
{
	log (LOG_ERR, "%s table is full\n", name);
}

int
min (a, b)
{
	return a < b ? a : b;
}

int
imin (a, b)
{
	return a < b ? a : b;
}

int
max (a, b)
{
	return a > b ? a : b;
}


int
imax (a, b)
{
	return a > b ? a : b;
}

u_int32_t
arc4random()
{
	return(0);
}

/*
 * General routine to allocate a hash table.
 */
void *
hashinit(elements, type, flags, hashmask)
        int elements, type, flags;
        u_long *hashmask;
{
        long hashsize;
        LIST_HEAD(generic, generic) *hashtbl;
        int i;

        if (elements <= 0)
                panic("hashinit: bad cnt");
        for (hashsize = 1; hashsize <= elements; hashsize <<= 1)
                continue;
        hashsize >>= 1;
        hashtbl = malloc((u_long)hashsize * sizeof(*hashtbl), type, flags);
        for (i = 0; i < hashsize; i++)
                LIST_INIT(&hashtbl[i]);
        *hashmask = hashsize - 1;
        return (hashtbl);
}

/*
 * Validate parameters and get old / set new parameters
 * for a structure oriented sysctl function.
 */
int
sysctl_struct(oldp, oldlenp, newp, newlen, sp, len)
        void *oldp;
        size_t *oldlenp;
        void *newp;
        size_t newlen;
        void *sp;
        int len;
{
        int error = 0;

        if (oldp && *oldlenp < len)
                return (ENOMEM);
        if (newp && newlen > len)
                return (EINVAL);
        if (oldp) {
                *oldlenp = len;
                error = copyout(sp, oldp, len);
        }
        if (error == 0 && newp)
                error = copyin(newp, sp, len);
        return (error);
}

/*
 * Validate parameters and get old / set new parameters
 * for an integer-valued sysctl function.
 */
int
sysctl_int(oldp, oldlenp, newp, newlen, valp)
        void *oldp;
        size_t *oldlenp;
        void *newp;
        size_t newlen;
        int *valp;
{
        int error = 0;

        if (oldp && *oldlenp < sizeof(int))
                return (ENOMEM);
        if (newp && newlen != sizeof(int))
                return (EINVAL);
        *oldlenp = sizeof(int);
        if (oldp)
                error = copyout(valp, oldp, sizeof(int));
        if (error == 0 && newp)
                error = copyin(newp, valp, sizeof(int));
        return (error);
}

/*
 * As above, but read-only.
 */
int
   sysctl_rdint(oldp, oldlenp, newp, val)
   void *oldp;
size_t *oldlenp;
void *newp;
int val;
{
   int error = 0;

   if (oldp && *oldlenp < sizeof(int))
      return (ENOMEM);
   if (newp)
      return (EPERM);
   *oldlenp = sizeof(int);
   if (oldp)
      error = copyout((caddr_t)&val, oldp, sizeof(int));
   return (error);
}

void vminit __P((void));
void
vminit ()
{
extern int memorysize;

	if (!kmem) {
		/* grab a chunk at the top of memory */
		if (memorysize < VM_KMEM_SIZE * 2) {
			panic ("not enough memory for network");
		}
		memorysize = (memorysize - VM_KMEM_SIZE) & ~PGOFSET;
#ifdef __mips__
		if ((u_int32_t)&kmem < (u_int32_t)UNCACHED_MEMORY_ADDR) {
			/* if linked for data in kseg0, keep kmem there too */
			kmem = (u_char *) PHYS_TO_CACHED (memorysize);
		}
		else {
			kmem = (u_char *) PHYS_TO_UNCACHED (memorysize);
		}
#else
		kmem = (u_char *)memorysize;
#endif
	}
}

vm_offset_t
kmem_malloc (map, size, canwait)
	vm_map_t map;
	vm_size_t size;
{
	vm_offset_t p;

	if(!kmem) {	/* In case we call this before vminit() */
		vminit();
	}

	size = (size + PGOFSET) & ~PGOFSET;
	if (kmem_offs + size > VM_KMEM_SIZE) {
		log (LOG_DEBUG, "kmem_malloc: request for %d bytes fails\n", size);
		return 0;
	}
	p = (vm_offset_t) &kmem[kmem_offs];
	kmem_offs += size;
	return p;
}


vm_offset_t
kmem_alloc (map, size)
	vm_map_t map;
	vm_size_t size;
{
	return kmem_malloc (map, size, 0);
}


vm_map_t
kmem_suballoc (map, base, lim, size, canwait)
	vm_map_t map;
	vm_offset_t *base, *lim;
	vm_size_t size;
{
	if (size > VM_KMEM_SIZE)
		panic ("kmem_suballoc");
	*base = (vm_offset_t) kmem;
	*lim = (vm_offset_t)  kmem + VM_KMEM_SIZE;
	return map;
}

void
kmem_free (map, addr, size)
	vm_map_t map;
	vm_offset_t addr;
	vm_size_t size;
{
	panic ("kmem_free");
}


/*XXX*/
/*
 *  Shutdown hooks are currently nops. Later we should add something
 *  that shutdown activites like ethernet etc (eg does the hook calls)
 *  so interfearence with loaded and run programs will not occur.
 */
void *
shutdownhook_establish(callee, sc)
	void (*callee) __P((void *));
	void *sc;
{
	return((void *)1);	/* Return non-zero for now */
}


static void
ifpoll()
{
	struct ifnet *ifp;
static int here = 0;

	if(here) {		/* Don't recurse */
		splhigh();
		printf("ifpoll recursed!\n");
		md_do_stacktrace(0, -1, 0, 0);
		while(1);
	}

	here = 1;
	for(ifp = TAILQ_FIRST(&ifnet); ifp; ifp = TAILQ_NEXT(ifp, if_list)) {
		if (ifp->if_ioctl) {
			(*ifp->if_ioctl)(ifp, SIOCPOLL, 0);
		}
	}
	here = 0;
}

/*
 *  XXX The entire SPL code is wrong in the sense that the 'levels'
 *  XXX are not levels but events that should be masked. Note that
 *  XXX some 'levels' masks more than one event anyhow. This should
 *  XXX really be rewritten in the future and especially if we start
 *  XXX using real interrupts.
 */

volatile int spl;	/* Current SPL mask */

int
splhigh()
{
	int old = spl;

	spl = 7;
	return(old);
}

int
splclock()
{
	int old = spl;

	spl = 7;
	return(old);
}

int
spltty()
{
	int old = spl;

	if(old < 5) {
		spl = 5;
	}
	return(old);
}

int
splbio()
{
	int old = spl;

	if(old < 4) {
		spl = 4;
	}
	return(old);
}

int
splimp()
{
	int old = spl;

	if(old < 7) {
		spl = 7;
	}
	return(old);
}

int
splnet()
{
	int old = spl;

	if(old < 3) {
		spl = 3;
	}
	return(old);
}

int
splsoftclock()
{
	int old = spl;

	if(old < 1) {
		spl = 1;
	}
	return(old);
}

int
splsoftnet()
{
	int old = spl;

	if(old < 1) {
		spl = 1;
	}
	return(old);
}

int
spl0()
{
	int s = spl;

	splx(0);
	return(s);
} 
#ifdef MYPRINT
void printstr(char *s)
{
   unsigned long port = PORT3f8; 
   while (*s) {
     while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0); 
     *(unsigned char*)port = *s;
     s++;
   }
}
void printnum(unsigned long long n)
{
  int i,j;
  unsigned char a[40];
  unsigned long port = PORT3f8; 
  i = 0;
  do {
   a[i] = n % 16;
   n = n / 16;
   i++;
  }while(n);
 
  for (j=i-1;j>=0;j--) {
   if (a[j]>=10) {
     while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0); 
     *(unsigned char*)port = 'a' + a[j] - 10;
   }else{
     while (((*(volatile unsigned char*)PORT3fd) & 0x20)==0); 
     *(unsigned char*)port = '0' + a[j];
   }
  }
  printstr("\r\n");
}
#endif

void
splx(newspl)
{
extern void softnet __P((void));
	/* If new means lowering then check pending jobs */
#ifdef MYPRINT
	printstr("in splx");
	printnum(newspl);
#endif
	if (newspl < spl) {
		if (newspl < 7) {
			spl = 7;
			tgt_clkpoll ();
#ifdef MYPRINT
			printstr("clkpoll");
#endif
		}
		if (newspl < 3) {	/* NET + IMP */
			spl = 3;
			ifpoll ();
#ifdef MYPRINT
			printstr("ifpoll");
#endif
		}
		if (newspl < 1 && netisr != 0) {
			spl = 1;
			softnet ();
#ifdef MYPRINT
			printstr("softnet");
#endif
		}
	}
	spl = newspl;
	if (spl == 0) {
		tgt_poll();
#ifdef MYPRINT
		printstr("tgtpoll");
#endif
	}
}

void
setsoftnet ()
{
	/* nothing to do, checked by spl0() */
}

void
setsoftclock ()
{
	/* simulated soft clock */
	schednetisr(NETISR_SCLK);
}

void
minphys(bp)
	struct buf *bp;
{
	if (bp->b_bcount > MAXPHYS)
		bp->b_bcount = MAXPHYS;
}
