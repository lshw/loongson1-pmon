/* $Id: rsa.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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
#include <ctype.h>
#include <stdlib.h>

#include <pmon.h>

#include "mod_debugger.h"
#include "mod_symbols.h"

/*
 *  Scan and input a value.
 *
 *  Note: The vp arg (dest) must be able to receive a value
 *        of 'u_int32_t' size. Assumption here is that this
 *        type will be the largets we deal with for non
 *	  register type values.
 */
int
get_rsa(u_int32_t *vp, char *p)
{
	register_t val;
    
	if(get_rsa_reg (&val, p)) {
		*vp = val;
		return (1);
	}
	return (0);
}

/*
 *  Scan and input a value of largest size (processor register width).
 *
 *  Note: The vp arg (dest) must be able to receive a value
 *        of 'regsiter_t' size. Assumption here is that this
 *        type will be the largets we deal with.
 */
int
get_rsa_reg(register_t *vp, char *p)
{
	int r, inbase;
	register_t v1, v2;
	char *q, subexpr[LINESZ];

	/* strip enclosing parens */
	while (*p == '(' && strbalp (p) == p + strlen (p) - 1) {
		strdchr (p);
		p[strlen (p) - 1] = 0;
	}

	if ((q = strrpset (p, "+-")) != 0) {	/* is compound */
		strncpy (subexpr, p, q - p);
		subexpr[q - p] = '\0';
		r = get_rsa_reg (&v1, subexpr);
		if (r == 0) {
			return (r);
		}
		r = get_rsa_reg (&v2, q + 1);
		if (r == 0) {
			return (r);
		}
		if (*q == '+') {
			*vp = v1 + v2;
		}
		else {
			*vp = v1 - v2;
		}
		return (1);
	}

	if ((q = strrpset (p, "*/")) != 0) {
		strncpy (subexpr, p, q - p);
		subexpr[q - p] = '\0';
		r = get_rsa_reg (&v1, subexpr);
		if (r == 0) {
			return (r);
		}
		r = get_rsa_reg (&v2, q + 1);
		if (r == 0) {
			return (r);
		}
		if (*q == '*') {
			*vp = v1 * v2;
		}
		else {
			if (v2 == 0) {
				printf ("divide by zero\n");
				return (0);
			}
			*vp = v1 / v2;
		}
		return (1);
	}

	if (*p == '^') {
		r = get_rsa_reg (&v2, &p[1]);
		if (r == 0) {
			printf ("%s: bad indirect address\n", &p[1]);
		}
		else {
			*vp = load_word ((int32_t)v2);
		}
#if NMOD_DEBUGGER > 0
	} else if (*p == '@') {
		r = md_getreg (vp, &p[1]);
		if (r == 0) {
			printf ("%s: bad register name\n", &p[1]);
		}
	} else if (strcmp (p, ".") == 0) {
		r = md_getreg (vp, "cpc");
#endif
#if NMOD_SYMBOLS > 0
	} else if (*p == '&') {
		register_t adr;

		r = sym2adr (&adr, &p[1]);
		if (r == 0) {
			printf ("%s: bad symbol name\n", &p[1]);
		}
		else {
			*vp = adr;
		}
#endif
	} else if (isdigit (*p)) {
		inbase = matchenv ("inbase");

		switch (inbase) {
		case INBASE_DEC:
			r = md_ator (vp, p, 10);
			break;

		case INBASE_HEX:
			r = md_ator (vp, p, 16);
			break;

		case INBASE_OCT:
			r = md_ator (vp, p, 8);
			break;

		case INBASE_AUTO:
			r = md_ator (vp, p, 0);
			break;

		default:
			printf ("%s:%d bad inbase value\n", getenv("inbase"), inbase);
			return (0);
		}

		if (r == 0) {
			r = md_ator (vp, p, 0);
			if (r == 0) {
				printf ("%s: bad base %s value\n", p, getenv ("inbase"));
			}
		}
#if NMOD_SYMBOLS > 0
	} else if (isxdigit (*p)) {
		int inalpha;
		register_t adr;

		inalpha = matchenv ("inalpha");
		if (inalpha == FALSE) {
			r = md_ator (vp, p, 16);
			if (r == 0) {
				r = sym2adr (&adr, p);
				if (r == 0) {
					printf ("%s: neither hex value nor symbol\n", p);
				}
				else {
					*vp = adr;
				}
			}
		} else if (inalpha == TRUE) {
			r = sym2adr (&adr, p);
			if (r == 0) {
				r = md_ator (vp, p, 16);
				if (r == 0) {
					printf ("%s: neither hex value nor symbol\n", p);
				}
				else {
					*vp = adr;
				}
			}
		}
		else {
			printf ("%s: bad inalpha value\n", getenv ("inalpha"));
			return (0);
		}
	}
	else {
		register_t adr;

		r = sym2adr (&adr, p);
		if (r == 0) {
			printf ("%s: bad symbol name\n", p);
		}
		else {
			*vp = adr;
		}
#endif
	}
	return (r);
}


void
store_word (adr, v)
     void	* adr;
     int32_t	v;
{
    *(int32_t *)adr = v;
    flush_cache (IADDR, adr);
}

void
store_half (adr, v)
     void	*adr;
     int16_t	v;
{
    *(int16_t *)adr = v;
    flush_cache (IADDR, adr);
}

void
store_byte (adr, v)
     void	*adr;
     int8_t	v;
{
    *(int8_t *)adr = v;
    flush_cache (IADDR, adr);
}
