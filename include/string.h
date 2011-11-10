/*	$Id: string.h,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */

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
#ifndef _STRING_H_
#define _STRING_H_
#include <machine/ansi.h>

#ifdef  _BSD_SIZE_T_
typedef _BSD_SIZE_T_    size_t;
#undef  _BSD_SIZE_T_
#endif

#ifndef NULL
#define NULL 0
#endif

#include <sys/cdefs.h>

__BEGIN_DECLS
void	bzero __P((void *, size_t));
char	*strcat __P((char *, const char *));
char	*strncat __P((char *, const char *, size_t));
char	*strchr __P((const char *, int));
char	*strnchr __P((const char *, char, size_t));
char	*strncpy __P((char *, const char *, size_t));
int	strncmp __P((const char *, const char *, size_t));
char	*strcpy __P((char *, const char *));
char	*strrchr __P((const char *, int));
char	*strpbrk __P((const char *, const char *));
size_t	strlen __P((const char *));
char	*strtok __P((char *, const char *));
char	*strstr __P((const char *, const char *));
char	*strerror __P((int));
int	strcasecmp __P((const char *, const char *));
int	strncasecmp __P((const char *, const char *, size_t)); 
void	*memchr __P((const void *, int, size_t));

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE)
char    *index __P((const char *, int));
char    *rindex __P((const char *, int)); 

char	*strdchr __P((char *));
char	*strichr __P((char *, int));
void	stristr __P((char *, const char *));
char	*strccat __P((char *, int));
void	strmerge __P((char *, const char *));
int	strspn __P((const char *, const char *));
int	strcspn __P((const char *, const char *));
int	strempty __P((const char *));
char	*getword __P((char *, const char *));
int	strnwrd __P((const char *));
int	wordsz __P((const char *));
char	*strset __P((const char *, const char *));
char	*strrset __P((const char *, const char *));
char	*strbalp __P((const char *));
char	*strrpset __P((const char *, const char *));
char	*strrrot __P((char *));
void	strsort __P((char *));
#define strequ(a, b) (strcmp(a,b) ? 1 : 0)
int	striequ __P((const char *, const char *));
int	strbequ __P((const char *, const char *));
int	strpat __P((const char *, const char *));
char	*strposn __P((const char *, const char *));
char	*cc2str __P((char *, int));
int	str2cc __P((const char *));
int	argvize __P((char *[], char *));
int	ffs __P((int));
void	bcopy __P((const char *, char *, size_t));
int     bcmp __P((const void *, const void *, size_t));
void	str_fmt __P((char *, int, int));
void	strtoupper __P((char *));
#endif

__END_DECLS

/* definitions for fmt parameter of str_fmt(p,width,fmt) */
#define FMT_RJUST 0
#define FMT_LJUST 1
#define FMT_RJUST0 2
#define FMT_CENTER 3

#endif /* _STRING_H_ */
