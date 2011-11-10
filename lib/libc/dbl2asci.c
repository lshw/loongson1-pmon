/* $Id: dbl2asci.c,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */

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

#ifdef FLOATINGPT

dbl_to_ascii (in, dst)
     double         *in;
     char            dst[];
{
    int             i;
    char            tmp1[64], tmp2[64];
    long            exp;
    char            sign;

    double          d;

/* dtoa clobbers 'in' so copy it first */
    d = *in;
    dtoa (&d, tmp1, &sign, &exp);

    *dst = 0;
    if (sign == '-')
	strcat (dst, "-");

/* mantissa */
    if (!strcmp(tmp1, "0"))
	sprintf (tmp2, "0.%s", tmp1);
    else {
	sprintf (tmp2, "%c.%s", tmp1[0], &tmp1[1]);
	exp--;
    }
    for (i = strlen (tmp2); i < 25; i++)
	tmp2[i] = '0';
    tmp2[i] = 0;
    strcat (dst, tmp2);

/* exponent */
    sprintf (tmp1, "%02d", exp);
    if (tmp1[0] == '-')
	strcat (dst, "e");
    else
	strcat (dst, "e+");
    strcat (dst, tmp1);
}
#endif /* FLOATINGPT */
