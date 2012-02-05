/*	$Id: hello.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2002 Opsycon AB  (www.opsycon.se)
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

/*
 *   This is a simple program that demonstrate the linkage
 *   between PMON2000 and a loaded program.
 */

typedef long long off_t;

struct callvectors {
	int     (*open) (char *, int, int);
	int     (*close) (int);
	int     (*read) (int, void *, int);
	int     (*write) (int, void *, int);
	off_t   (*lseek) (int, off_t, int);
	int     (*printf) (const char *, ...);
	void    (*cacheflush) (void);
	char    *(*gets) (char *);
};

struct callvectors *callvec;

#define	printf (*callvec->printf)
#define	gets   (*callvec->gets)

void __gccmain(void);
void __gccmain(void){}
int
main(int argc, char **argv, char **env, struct callvectors *cv)
{
	char str[256];
	char **ev;
	int i;
	unsigned stat;

	callvec = cv;

	printf("\n\nHello! This is the 'hello' program!\n\n");
	printf("It was invoked with:\n");
	for (i = 0; i < argc; i++) {
		printf("Arg %2d: %s\n", i, argv[i]);
	}
	printf("\nEnvironment setup = \n");
	ev = env;
	while(*ev) {
		printf("\t%s\n", *ev);
		ev++;
	}
	printf("\n\n");

	printf("Type '<ctl>C' to stop the program\n");
	gets(str);
	printf("Ehhh.. you typed '%s'\n", str);
	return(0);
}
