/* $Id: about.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2001 Opsycon AB  (www.opsycon.se)
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

#include <stdio.h>
#ifdef _KERNEL
#undef _KERNEL
#endif

#include <pmon.h>

int cmd_about __P((int, char *[]));

int
cmd_about(argc, argv)
	int	argc;
	char	*argv[];
{
 printf("\nPMON2000 is a derivative work released under the BSD Copyright.\n\n");

 printf("This software is freely redistributable under this generous copyright\n");
 printf("as long as all the preexisting copyrights are retained.\n\n");

 printf("PMON2000 was originally based on PMON from Algorithmics UK, which in\n");
 printf("turn was based on PMON for the MIPS architecure written by Phil Bunce.\n\n");

#if defined(__powerpc__)
 printf("The PowerPC implementation  was created on sub-contract to RTMX Inc,\n");
 printf("(GroupBSD.org) at Opsycon AB, Sweden. The board specific porting was\n");
 printf("done at the request of  SBS Communications  for a variety of VMEbus,\n");
 printf("CompactPCI and PPMC PowerPC based SBCs.\n\n");
#endif

#if defined(__mips__)
 printf("The MIPS implementation was created on sub-contract to PMC-Sierra, Inc,\n");
 printf("at Opsycon AB, Sweden. This implementation was then been ported to\n");
 printf("Galileo Technology evaluation boards on contract to Marvell/Galileo\n\n");
#endif

 printf("Visit the PMON2000 Web Site, http://www.pmon2000.com for on-line\n");
 printf("reference documentation, information and support.\n\n");

 printf("For other related activities / products / services visit.\n\n");
 printf("Opsycon AB, Sweden              http://www.opsycon.com\n");
 printf("GroupBSD Open Source            http://groupbsd.org\n");
 printf("OpenBSD Project                 http://www.openbsd.org\n");
 printf("\n");

 return( 0 );
 }

static const Cmd Cmds[] =
{
        {"Shell"},
	{"about",	"",
			0,
			"about PMON2000",
			cmd_about, 1, 1, NULL},
	{0, 0}
};
			

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}                  
