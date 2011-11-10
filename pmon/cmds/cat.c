/*
 * Copyright (c) 2007 SUNWAH HI-TECH  (www.sw-linux.com.cn)
 * Wing Sun	<chunyang.sun@sw-linux.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/endian.h>

#include <pmon.h>
#include <exec.h>
#include <pmon/loaders/loadfn.h>

#include <termio.h>
#include <endian.h>

#include <signal.h>
#include <setjmp.h>
#include <ctype.h>

#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif

#ifdef __mips__
#include <machine/cpu.h>
#endif

#define LINE_MAX 8192

int cmd_cat (int ac, char *av[])
{
	char path[256] = {0};
//	char s[LINE_MAX+1] = {0};
	char* s = NULL;
	char* buf = NULL;
	int flags = 0;
	int bootfd;
	int n = LINE_MAX;
	ExecId id;

	int siz = 0;
	int ln = 0;
	int i, j;
	int m;
	if (ac != 2)
		return -1;

	
	flags |= RFLAG;
	strcpy(path, av[1]);
	
	if ((bootfd = open (path, O_RDONLY | O_NONBLOCK)) < 0) {
		perror (path);
		return EXIT_FAILURE;
	}

	s = malloc(LINE_MAX + 1);
	if (s == NULL)
	{
		perror("malloc");
		return EXIT_FAILURE;
	}

	memset(s, 0, LINE_MAX + 1);
		
	siz = moresz;
	ioctl(STDIN, CBREAK, NULL);
	ln = siz;
	i = 0;
	j = 0;
	
	buf = malloc(LINESZ + 8);
	if (buf == NULL)
	{
		perror("malloc");
		free(s);
		return EXIT_FAILURE;
	}

	id = getExec("txt");
	if (id != NULL) {
		do
		{
			n = LINE_MAX;
			exec(id, bootfd, s, &n, flags);
			if (n <= 0)
			{
				break;
			}
			s[n] = '\0';

			j += sprintf(buf + j, "%s", s);
			if ((i % 2) != 0)
			{
				if (more(buf, &ln, siz))
				{
					free(buf);
					free(s);
					close(bootfd);
					return 0;
				}
				j = 0;
			}
			i++;
		} while (n > 0);

		if (i % 2 != 0)
		{
			printf("%s", buf);
		}
#if 0
		do
		{
			exec(id, bootfd, s, &n, flags);
			s[n]='\0';
			j += sprintf(buf + j, "%s", s);
			if (s[0] == '\n')
			{
				if ((i % 2) != 0)
				{
					more(buf, &ln, siz);
					j = 0;
				}
				/*
				else
				{
					j += sprintf(buf + j, "\n");
				}
				*/
				i++;
			}
		}while(n>0 );
#endif
	}else{
		printf("[error] this pmon can't read file!");
		free(s);
		free(buf);
		close(bootfd);
		return -2;
	}
	close(bootfd);


	free(buf);
	free(s);
	return 0;
}

/*
 *  Command table registration
 *  ==========================
 */

static const Cmd CatCmd[] =
{
	{"RAYS Commands for PMON 2000"},
	{"cat",	"filename to display ",
			0,
			"Display file content.",
			cmd_cat, 2, 2, 0},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(CatCmd, 1);
}

