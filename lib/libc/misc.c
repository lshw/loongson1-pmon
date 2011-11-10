/* $Id: misc.c,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */

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

#undef _KERNEL
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/errno.h>

#define ULONG_MAX 	4294967295UL
#define LONG_MAX 	2147483647L
#define LONG_MIN 	(-LONG_MAX-1)

unsigned long
strtoul(const char *nptr,char **endptr,int base)
{
    int c;
    unsigned long result = 0L;
    unsigned long limit;
    int negative = 0;
    int overflow = 0;
    int digit;

    while ((c = *nptr) && isspace(c)) /* skip leading white space */
      nptr++;

    if ((c = *nptr) == '+' || c == '-') { /* handle signs */
	negative = (c == '-');
	nptr++;
    }

    if (base == 0) {		/* determine base if unknown */
	base = 10;
	if (*nptr == '0') {
	    base = 8;
	    nptr++;
	    if ((c = *nptr) == 'x' || c == 'X') {
		base = 16;
		nptr++;
	    }
	}
    } else if (base == 16 && *nptr == '0') {	
	/* discard 0x/0X prefix if hex */
	nptr++;
	if ((c = *nptr == 'x') || c == 'X')
	  nptr++;
    }

    limit = ULONG_MAX / base;	/* ensure no overflow */

    nptr--;			/* convert the number */
    while ((c = *++nptr) != 0) {
	if (isdigit(c))
	  digit = c - '0';
	else if(isalpha(c))
	  digit = c - (isupper(c) ? 'A' : 'a') + 10;
	else
	  break;
	if (digit < 0 || digit >= base)
	  break;
	if (result > limit)
	  overflow = 1;
	if (!overflow) {
	    result = base * result;
	    if (digit > ULONG_MAX - result)
	      overflow = 1;
	    else	
	      result += digit;
	}
    }
    if (negative && !overflow)	/* BIZARRE, but ANSI says we should do this! */
      result = 0L - result;
    if (overflow) {
	extern int errno;
	errno = ERANGE;
	result = ULONG_MAX;
    }

    if (endptr != NULL)		/* point at tail */
      *endptr = (char *)nptr;
    return result;
}


long
strtol(const char *nptr,char **endptr,int base)
{
    int c;
    long result = 0L;
    long limit;
    int negative = 0;
    int overflow = 0;
    int digit;

    while ((c = *nptr) && isspace(c)) /* skip leading white space */
      nptr++;
    if ((c = *nptr) == '+' || c == '-') { /* handle signs */
	negative = (c == '-');
	nptr++;
    }
    if (base == 0) {		/* determine base if unknown */
	base = 10;
	if (*nptr == '0') {
	    base = 8;
	    nptr++;
	    if ((c = *nptr) == 'x' || c == 'X') {
		base = 16;
		nptr++;
	    }
	}
    }
    else
      if (base == 16 && *nptr == '0') {	/* discard 0x/0X prefix if hex */
	  nptr++;
	  if ((c = *nptr == 'x') || c == 'X')
	    nptr++;
      }

    limit = LONG_MAX / base;	/* ensure no overflow */

    nptr--;			/* convert the number */
    while ((c = *++nptr) != 0) {
	if (isdigit(c))
	  digit = c - '0';
	else if(isalpha(c))
	  digit = c - (isupper(c) ? 'A' : 'a') + 10;
	else
	  break;
	if (digit < 0 || digit >= base)
	  break;
	if (result > limit)
	  overflow = 1;
	if (!overflow) {
	    result = base * result;
	    if (digit > LONG_MAX - result)
	      overflow = 1;
	    else	
	      result += digit;
	}
    }
    if (negative && !overflow)
      result = 0L - result;
    if (overflow) {
	extern int errno;
	errno = ERANGE;
	if (negative)
	  result = LONG_MIN;
	else
	  result = LONG_MAX;
    }

    if (endptr != NULL)		/* set up return values */
      *endptr = (char *)nptr;
    return result;
}


char *sys_errlist[] = {
	"Undefined error: 0",			/*  0 - ENOERROR */
	"Operation not permitted",		/*  1 - EPERM */
	"No such file or directory",		/*  2 - ENOENT */
	"No such process",			/*  3 - ESRCH */
	"Interrupted system call",		/*  4 - EINTR */
	"Input/output error",			/*  5 - EIO */
	"Device not configured",		/*  6 - ENXIO */
	"Argument list too long",		/*  7 - E2BIG */
	"Exec format error",			/*  8 - ENOEXEC */
	"Bad file descriptor",			/*  9 - EBADF */
	"No child processes",			/* 10 - ECHILD */
	"Resource deadlock avoided",		/* 11 - EDEADLK */
	"Cannot allocate memory",		/* 12 - ENOMEM */
	"Permission denied",			/* 13 - EACCES */
	"Bad address",				/* 14 - EFAULT */
	"Block device required",		/* 15 - ENOTBLK */
	"Device busy",				/* 16 - EBUSY */
	"File exists",				/* 17 - EEXIST */
	"Cross-device link",			/* 18 - EXDEV */
	"Operation not supported by device",	/* 19 - ENODEV */
	"Not a directory",			/* 20 - ENOTDIR */
	"Is a directory",			/* 21 - EISDIR */
	"Invalid argument",			/* 22 - EINVAL */
	"Too many open files in system",	/* 23 - ENFILE */
	"Too many open files",			/* 24 - EMFILE */
	"Inappropriate ioctl for device",	/* 25 - ENOTTY */
	"Text file busy",			/* 26 - ETXTBSY */
	"File too large",			/* 27 - EFBIG */
	"No space left on device",		/* 28 - ENOSPC */
	"Illegal seek",				/* 29 - ESPIPE */
	"Read-only file system",		/* 30 - EROFS */
	"Too many links",			/* 31 - EMLINK */
	"Broken pipe",				/* 32 - EPIPE */

/* math software */
	"Numerical argument out of domain",	/* 33 - EDOM */
	"Result too large",			/* 34 - ERANGE */

/* non-blocking and interrupt i/o */
	"Resource temporarily unavailable",	/* 35 - EAGAIN */
						/* 35 - EWOULDBLOCK */
	"Operation now in progress",		/* 36 - EINPROGRESS */
	"Operation already in progress",	/* 37 - EALREADY */

/* ipc/network software -- argument errors */
	"Socket operation on non-socket",	/* 38 - ENOTSOCK */
	"Destination address required",		/* 39 - EDESTADDRREQ */
	"Message too long",			/* 40 - EMSGSIZE */
	"Protocol wrong type for socket",	/* 41 - EPROTOTYPE */
	"Protocol not available",		/* 42 - ENOPROTOOPT */
	"Protocol not supported",		/* 43 - EPROTONOSUPPORT */
	"Socket type not supported",		/* 44 - ESOCKTNOSUPPORT */
	"Operation not supported",		/* 45 - EOPNOTSUPP */
	"Protocol family not supported",	/* 46 - EPFNOSUPPORT */
						/* 47 - EAFNOSUPPORT */
	"Address family not supported by protocol family",
	"Address already in use",		/* 48 - EADDRINUSE */
	"Can't assign requested address",	/* 49 - EADDRNOTAVAIL */

/* ipc/network software -- operational errors */
	"Network is down",			/* 50 - ENETDOWN */
	"Network is unreachable",		/* 51 - ENETUNREACH */
	"Network dropped connection on reset",	/* 52 - ENETRESET */
	"Software caused connection abort",	/* 53 - ECONNABORTED */
	"Connection reset by peer",		/* 54 - ECONNRESET */
	"No buffer space available",		/* 55 - ENOBUFS */
	"Socket is already connected",		/* 56 - EISCONN */
	"Socket is not connected",		/* 57 - ENOTCONN */
	"Can't send after socket shutdown",	/* 58 - ESHUTDOWN */
	"Too many references: can't splice",	/* 59 - ETOOMANYREFS */
	"Connection timed out",			/* 60 - ETIMEDOUT */
	"Connection refused",			/* 61 - ECONNREFUSED */

	"Too many levels of symbolic links",	/* 62 - ELOOP */
	"File name too long",			/* 63 - ENAMETOOLONG */

/* should be rearranged */
	"Host is down",				/* 64 - EHOSTDOWN */
	"No route to host",			/* 65 - EHOSTUNREACH */
#if 0
	"Directory not empty",			/* 66 - ENOTEMPTY */

/* quotas & mush */
	"Too many processes",			/* 67 - EPROCLIM */
	"Too many users",			/* 68 - EUSERS */
	"Disc quota exceeded",			/* 69 - EDQUOT */

/* Network File System */
	"Stale NFS file handle",		/* 70 - ESTALE */
	"Too many levels of remote in path",	/* 71 - EREMOTE */
	"RPC struct is bad",			/* 72 - EBADRPC */
	"RPC version wrong",			/* 73 - ERPCMISMATCH */
	"RPC prog. not avail",			/* 74 - EPROGUNAVAIL */
	"Program version wrong",		/* 75 - EPROGMISMATCH */
	"Bad procedure for program",		/* 76 - EPROCUNAVAIL */

	"No locks available",			/* 77 - ENOLCK */
	"Function not implemented",		/* 78 - ENOSYS */
	"Inappropriate file type or format",	/* 79 - EFTYPE */
#endif
};

int errno;
int sys_nerr;

char *
strerror(num)
	int num;
{
	register unsigned int errnum;

	if (sys_nerr == 0)
	  sys_nerr = sizeof (sys_errlist)/sizeof (sys_errlist[0]);

	errnum = num;				/* convert to unsigned */
	if (errnum < sys_nerr)
		return(sys_errlist[errnum]);
	return "Unknown error";
}

void
perror (s)
    const char *s;
{
    extern int errno;
    if (s)
      fprintf (stderr, "%s: ", s);
    fprintf (stderr, "%s\n", strerror(errno));
}


unsigned int
alarm(secs)
	unsigned int secs;
{
	struct itimerval it, oitv;
	register struct itimerval *itp = &it;

	timerclear(&itp->it_interval);
	itp->it_value.tv_sec = secs;
	itp->it_value.tv_usec = 0;
	if (setitimer(ITIMER_REAL, itp, &oitv) < 0)
		return (-1);
	if (oitv.it_value.tv_usec)
		oitv.it_value.tv_sec++;
	return (oitv.it_value.tv_sec);
}
