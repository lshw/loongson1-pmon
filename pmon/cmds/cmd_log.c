/*	$Id: cmd_log.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2002 Patrik Lindergren  (www.lindergren.com)
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
 *	This product includes software developed by
 *	Patrik Lindergren, Sweden.
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
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pmon.h>
#include <termio.h>

#define VFLAG 1

/*
 * Prototypes
 */
int log_cmd __P((int, char **av));

static const Optdesc log_opts[] =
{
   {"-v", "verbose messages"},
   {"-c", "clear log messages"},
    {0}};

#define LOGFILE_BASE 0x3000
#define LOGFILE_END  0x4000
    
int
log_cmd (ac, av)
    int             ac;
    char           *av[];
{
	int flags = 0;
	char	*buffer = (char *)LOGFILE_BASE;
	int c;
	
	optind = 0;
	while ((c = getopt (ac, av, "vc")) != EOF)
		switch (c) {
		   case 'v':
		      flags |= VFLAG; break;
		   case 'c':
		      while(buffer < (char *)LOGFILE_END) {
			 *buffer = '\000';
			 buffer++;
		      }
		      break;
		default:
			return (-1);
		}
	while(buffer < (char *)LOGFILE_END) {
	   if(isprint(*buffer) || *buffer == '\n')
		putc(*buffer, stdout);
	   buffer++;
	   if(*buffer == '\000')
	      break;
	}
	return (0);
}

static const Cmd Cmds[] =
{
	{"Misc"},
	{"log",		"[-vc]",
			log_opts,
			"log buffer handling",
			log_cmd, 1, 2, 0},
	{0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
      cmdlist_expand(Cmds, 1);
      logfp = fopen ("logger", "r+");
      if (!logfp) {
	 printf ("can't open logger\n");
	 return;
      }
}

