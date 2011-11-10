/*	$Id: pmon_target.h,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2001 Opsycon AB  (www.opsycon.se)
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
 *	This product includes software developed by Opsycon AB, Sweden.
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

#include <target/ev64420.h>
#include <pmon/dev/gt64420reg.h>
#define SBD_DEBUG 1
#ifndef SBD_DEBUG
#define SBD_DISPLAY(text, code)		/* No display function */
#else
void hexserial __P((int));
#define SBD_DISPLAY(text, code)	tgt_display1(text, code)
#endif

/*
 *  Name of envvar that has to be set to enable expert mode.
 */
#define EXPERT  "galileoexpert"

/*
 *  Target arch specifics
 */
#if __mips >= 3
#define	HAVE_QUAD			/* Native 64 bit integers */
#endif

/*
 *  Boot loader parameters.
 */

#define	TGT_BOOT_ADR	0x80400000	/* Load 4 meg up. */
#define	TGT_BOOT_SIZ	0x00002000	/* Suck in 8k */
#define	TGT_BOOT_OFF	0x00000400	/* Start reading from byte 1024 */

/*
 *  Target dependent CLIENTPC settings
 */
#define CLIENTPC 0x80100000
#define SETCLIENTPC "80100000"
#define TGT_DEFENV {"ifconfig","rtl0:10.0.0.12",0}
extern char *heaptop;
