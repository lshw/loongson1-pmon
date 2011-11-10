/*	$Id: fl_nvram.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2002 Patrik Lindergren (www.lindergren.com)
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
 *	This product includes software developed by Patrik Lindergren.
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

#include <sys/types.h>
#include <stdio.h>
#include <setjmp.h>
#include <termio.h>
#include <string.h>

#include <include/pflash.h>
#include <pmon.h>

static int cksum __P((void *, size_t, int));
static void nvram_enable __P((void));
static void nvram_disable __P((void));

static int nvram_invalid = 0;
extern int md_pipefreq;
extern int md_cpufreq;

/*
 *   --- NVRAM handling code ---
 */

/*
 *  Special copy that does it bytewise, guaranteed!
 */
#ifdef VXWORKS
static void bytecopy __P((char *, char *, size_t));
static void
bytecopy(char *src, char *dst, size_t size)
{
	if(src == NULL) {
		*dst = '\0';
	}
	else if (dst >= src && dst < src + size) {
		src += size;
		dst += size;
		while (size--) {
			*--dst = *--src;
		}
	}
	else while (size--) {
		*dst++ = *src++;
	}
}
#endif /* VXWORKS */

static void
nvram_enable()
{
}

static void
nvram_disable()
{
}

/*
 *  Read in environment from NV-ram and set.
 */
void
tgt_mapenv(int (*func) __P((char *, char *)))
{
	char *ep;
	char env[256];
        char *nvram;
	int i;

        nvram = (char *)NVRAM_BASE;
	nvram += NVRAM_OFFS;
        
	/*
	 *  Check integrity of the NVRAM env area. If not in order
	 *  initialize it to empty.
	 */
	if(cksum((void *)nvram, NVRAM_SIZE, 0) != 0) {
		nvram_invalid = 1;
		
		SBD_DISPLAY ("ENVC", CHKPNT_MAPV);
	} else {
		ep = nvram + 2;

		while(*ep != 0) {
			char *val = 0, *p = env;
			i = 0;
			while((*p++ = *ep++) != 0 && (ep < nvram + NVRAM_SIZE) && i++ < 255) {
				if((*(p - 1) == '=') && (val == NULL)) {
					*(p - 1) = '\0';
					val = p;
				}
			}
			if(ep < nvram + NVRAM_SIZE && i < 255) {
				(*func)(env, val);
			}
			else {
				nvram_invalid = 2;
				break;
			}
		}
		SBD_DISPLAY ("ENVV", CHKPNT_MAPV);
#ifdef VXWORKS
		bytecopy((char *)NVRAM_VXWORKS, env, sizeof(env));
		for(i = 0; isprint(env[i]) && i < 255; i++ );
		env[i] = '\0';	
		(*func)("vxWorks", env);
#endif /* VXWORKS */
	}
	
	sprintf(env, "%d", memorysize / (1024 * 1024));
	(*func)("memsize", env);
	
	sprintf(env, "%d", md_pipefreq);
	(*func)("cpuclock", env);
	
	sprintf(env, "%d", md_cpufreq);
	(*func)("busclock", env);
	
	(*func)("systype", SYSTYPE);
	(*func)("targetname", TARGETNAME);

	SBD_DISPLAY ("ENVM", CHKPNT_MAPV);
}

int
tgt_unsetenv(char *name)
{
	char *ep;
	char *np;
	char *sp;
	char *nvram;
	char nvrambuf[NVRAM_SIZE];

	if(nvram_invalid) {
		return(0);
	}
        nvram = (char *)NVRAM_BASE;
	nvram += NVRAM_OFFS;
	memcpy(nvrambuf, nvram, sizeof(nvrambuf));
	ep = &nvrambuf[2];

	while((*ep != '\0') && (ep < nvrambuf + sizeof(nvrambuf))) {
		np = name;
		sp = ep;

		while((*ep == *np) && (*ep != '=') && (*np != '\0')) {
			ep++;
			np++;
		}
		if((*np == '\0') && ((*ep == '\0') || (*ep == '='))) {
			while(*ep++);
			while(ep <= nvrambuf + sizeof(nvrambuf)) {
				*sp++ = *ep++;
			}
			if(nvrambuf[2] == '\0') {
				nvrambuf[3] = '\0';
			}
			cksum(nvrambuf, sizeof(nvrambuf), 1);
			if(fl_erase_device(nvram, NVRAM_SIZE, FALSE)) {
				return(-1);
			}

			if(fl_program_device(nvram, nvrambuf, NVRAM_SIZE, FALSE)) {
				return(-1);
			}
			return(1);
		}
		else if(*ep != '\0') {
			while(*ep++ != '\0');
		}
	}
	return(0);
}

int
tgt_setenv(char *name, char *value)
{
	char *ep;
	int envlen;
	char *nvram;
	char nvrambuf[NVRAM_SIZE];
	
	/* Non permanent vars. */
	if(strcmp(EXPERT, name) == 0) {
		return(1);
	}
	
	/* Calculate total env mem size requiered */
	envlen = strlen(name);
	if(envlen == 0) {
		return(0);
	}
	if(value != NULL) {
		envlen += strlen(value);
	}
	envlen += 2;	/* '=' + null byte */
	if(envlen > 255) {
		return(0);	/* Are you crazy!? */
	}
	
	nvram = (char *)NVRAM_BASE;
	nvram += NVRAM_OFFS;
	memcpy(nvrambuf, nvram, sizeof(nvrambuf));
	
	/* If NVRAM is found to be uninitialized, reinit it. */
	if(nvram_invalid) {
		nvram_enable();
		memset(nvrambuf, -1, NVRAM_SIZE);
		nvrambuf[2] = '\0';
		nvrambuf[3] = '\0';
		cksum((void *)nvrambuf, sizeof(nvrambuf), 1);

		if(fl_erase_device(nvram, NVRAM_SIZE, FALSE)) {
			return(-1);
		}
	
		if(fl_program_device(nvram, nvrambuf, NVRAM_SIZE, FALSE)) {
			return(-1);
		}
		nvram_invalid = 0;
#ifdef VXWORKS
		bytecopy(NVRAM_VXWORKS_DEFAULT, (char *)NVRAM_VXWORKS,
			 sizeof(NVRAM_VXWORKS_DEFAULT));
#endif /* VXWORKS */
		nvram_disable();
	}

#ifdef VXWORKS
	if(strcmp("vxWorks", name) == 0) {
		nvram_enable();
		bytecopy(value, (char *)NVRAM_VXWORKS, envlen);
		nvram_disable();
		return(1);
	}
#endif /* VXWORKS */
	
	/* Remove any current setting */
	tgt_unsetenv(name);
	
	/* Find end of evironment strings */
	memcpy(nvrambuf, nvram, sizeof(nvrambuf));
	ep = &nvrambuf[2];
	if(*ep != '\0') {
		do {
			while(*ep++ != '\0');
		} while(*ep++ != '\0');
		ep--;
	}
	if(((int)ep + sizeof(nvrambuf) - (int)ep) < (envlen + 1)) {
		return(0);	/* Bummer! */ 
	}
	
	/*
	 *  Special case heaptop must always be first since it
	 *  can change how memory allocation works.
	 */
	nvram_enable();
	if(strcmp("heaptop", name) == 0) {

		bcopy(&nvrambuf[2], &nvrambuf[2] + envlen,
			 ep - &nvrambuf[1]);

		ep = &nvrambuf[2];
		while(*name != '\0') {
			*ep++ = *name++;
		}
		if(value != NULL) {
			*ep++ = '=';
			while((*ep++ = *value++) != '\0');
		}
		else {
			*ep++ = '\0';
		}
	}
	else {
		while(*name != '\0') {
			*ep++ = *name++;
		}
		if(value != NULL) {
			*ep++ = '=';
			while((*ep++ = *value++) != '\0');
		}
		else {
			*ep++ = '\0';
		}
		*ep++ = '\0';	/* End of env strings */
	}
	cksum(nvrambuf, sizeof(nvrambuf), 1);
	if(fl_erase_device(nvram, NVRAM_SIZE, FALSE)) {
		return(0);
	}
	if(fl_program_device(nvram, nvrambuf, NVRAM_SIZE, FALSE)) {
		return(0);
	}
	nvram_disable();
	return(1);
}


/*
 *  Calculate checksum. If 'set' checksum is calculated and set.
 */
static int
cksum(void *p, size_t s, int set)
{
	u_int16_t sum = 0;
	u_int8_t *sp = p;
	int sz = s / 2;

	if(set) {
		*sp = 0;	/* Clear checksum */
		*(sp+1) = 0;	/* Clear checksum */
	}
	while(sz--) {
		sum += (*sp++) << 8;
		sum += *sp++;
	}
	if(set) {
		sum = -sum;
		*(u_int8_t *)p = sum >> 8;
		*((u_int8_t *)p+1) = sum;
	}
	return(sum);
}
