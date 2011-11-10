/*	$Id: smpfork.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

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
 *   This is a simple program that demonstrate the SMP fork
 *   functionality for the purpose of starting up additional CPUs.
 *   The semaphore locking around the cpu2done variable access
 *   is not really necessary since only one thread modifies it.
 *   Instead it's there to show how access to shared variables
 *   can be protected withing the realm of PMON2000. Once CPU 2
 *   has been scheduled the client program can take full control
 *   of the system.
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
	int     *(*smpfork) (int, char *);
	int     *(*semlock) (int);
	int     *(*semunlock) (int);
};

struct callvectors *callvec;

#define	printf (*callvec->printf)
#define	gets   (*callvec->gets)
#define	smpfork (*callvec->smpfork)
#define	semlock (*callvec->semlock)
#define	semunlock (*callvec->semunlock)

volatile int	cpu2done;
char str[256];

int
main(int argc, char **argv, char **env, struct callvectors *cv)
{
	char **ev;
	int i;

	callvec = cv;

	printf("\n\nSimple SMP FORK demo program!\n\n");
	printf("It was invoked with:\n");
	for (i = 0; i < argc; i++) {
		printf("Arg %2d: %s\n", i, argv[i]);
	}
	printf("\nCPU 1 is running\n");

	printf("\nScheduling CPU 2\n");

	if (smpfork(1024, (char *)&argc - 16384) == 0) {
		printf("This is CPU 2!\n");
		printf("Type someting\n");
		gets(str);
		printf("Telling CPU 1 that input is done\n");
		while(!semlock(0));
		cpu2done = 1;
		semunlock(0);
		return(0);
	}


	printf("This is CPU 1 waiting for CPU 2!\n");
	do {
		while(!semlock(0));
		i = cpu2done;
		semunlock(0);
	} while (!i);

	printf("CPU 1 recognized that CPU 2 is done.\n");
	printf("You typed '%s'\n", str);

	return(0);
}
