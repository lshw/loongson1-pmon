/* $Id: strpat.c,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */

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
#include <string.h>

#define BANCHOR		(0x80|'^')
#define EANCHOR		(0x80|'$')

/** int strpat(p,pat) return 1 if pat matches p, else 0; wildcards * and ? */

int 
strpat(const char *s1, const char *s2)
{
	char *p, *pat;
	char *t, tmp[MAXLN];
	char src1[MAXLN], src2[MAXLN];

	if (!s1 || !s2)
		return (0);

	p = src1;
	pat = src2;
	*p++ = BANCHOR;
	while (*s1)
		*p++ = *s1++;
	*p++ = EANCHOR;
	*p = 0;
	*pat++ = BANCHOR;
	while (*s2)
		*pat++ = *s2++;
	*pat++ = EANCHOR;
	*pat = 0;

	p = src1;
	pat = src2;
	for (; *p && *pat;) {
		if (*pat == '*') {
			pat++;
			for (t = pat; *t && *t != '*' && *t != '?'; t++);
			strncpy (tmp, pat, t - pat);
			tmp[t - pat] = '\0';
			pat = t;
			t = strposn (p, tmp);
			if (t == 0)
				return (0);
			p = t + strlen (tmp);
		}
		else if (*pat == '?' || *pat == *p) {
			pat++;
			p++;
		}
		else
			return (0);
	}
	if (!*p && !*pat)
		return (1);
	if (!*p && *pat == '*' && !*(pat + 1))
		return (1);
	return (0);
}
