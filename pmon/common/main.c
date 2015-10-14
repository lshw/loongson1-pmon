/* $Id: main.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2001-2002 Opsycon AB  (www.opsycon.se / www.opsycon.com)
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
 *      This product includes software developed by Opsycon AB, Sweden.
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
 *  This code was created from code released to Public Domain
 *  by LSI Logic and Algorithmics UK.
 */ 

#include <stdio.h>
#include <string.h> 
#include <machine/pio.h>
#include <pmon.h>
#include <termio.h>
#include <endian.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif
#include <pmon.h>
#include <exec.h>
#include <file.h>

#include "mod_debugger.h"
#include "mod_symbols.h"

#include "nand.h"
#include "sd.h"
#include "wd.h"

#include <pflash.h>
#include <flash.h>

extern void    *callvec;
unsigned int show_menu;

#include "cmd_hist.h"		/* Test if command history selected */
#include "cmd_more.h"		/* Test if more command is selected */

jmp_buf         jmpb;		/* non-local goto jump buffer */
char            line[LINESZ + 1];	/* input line */
struct termio	clntterm;	/* client terminal mode */
struct termio	consterm;	/* console terminal mode */
register_t	initial_sr;
int             memorysize;
int             memorysize_high;
char            prnbuf[LINESZ + 8];	/* commonly used print buffer */

int             repeating_cmd;
unsigned int  	moresz = 10;
#ifdef AUTOLOAD
static void autoload __P((char *));
#else
static void autorun __P((char *));
#endif
extern void __init __P((void));
extern void _exit (int retval);
extern void delay __P((int));

#ifdef INET
static void pmon_intr (int dummy)
{
    sigsetmask (0);
    longjmp (jmpb, 1);
}
#endif

/*FILE *logfp; = stdout; */

#if NCMD_HIST == 0
void get_line(char *line, int how)
{
	int i;

	i = read (STDIN, line, LINESZ);
	if(i > 0) {
		i--;
	}
	line[i] = '\0';
}
#endif

static int load_menu_list(void)
{
	char* rootdev = NULL;
	char* path = NULL;


	show_menu=1;
	if (path == NULL) {
		path = malloc(512);
			if (path == NULL) {
			return 0;
		}
	}

	memset(path, 0, 512);
	rootdev = getenv("bootdev");
	if (rootdev == NULL) {
		rootdev = "/dev/fs/ext2@wd0";
	}

	sprintf(path, "%s/boot/boot.cfg", rootdev);
	if (check_config(path) == 1) {
		sprintf(path, "bl -d ide %s/boot/boot.cfg", rootdev);
		if (do_cmd(path) == 0) {
			show_menu = 0;
			//                                      video_cls();
			free(path);
			path = NULL;
			return 1;
		}
	}
	else {
		sprintf(path, "/dev/fs/ext2@wd0/boot/boot.cfg", rootdev);
		if (check_config(path) == 1) {
			sprintf(path, "bl -d ide /dev/fs/ext2@wd0/boot/boot.cfg", rootdev);
			if (do_cmd(path) == 0) {
				show_menu = 0;
				//video_cls();
				free(path);
				path = NULL;
				return 1;
			}
		}
	}
#if 0
	if( check_ide() == 1 )// GOT IDE
	{
		if( do_cmd ("bl -d ide /dev/fs/ext2@wd0/boot.cfg") ==0 ) {
			show_menu=0;
			video_cls();
			return 1;
		}
	}
	else if( check_cdrom () == 1 ) // GOT CDROM
	{
		if( do_cmd ("bl -d cdrom /dev/fs/ext2@wd0/boot.cfg") ==0 ) {
			show_menu=0;
			video_cls();
			return 1;
		}
	}
#endif
	free(path);
	path = NULL;
//	video_cls();
	show_menu=0;
	return 0;
	show_menu=0;
	return 1;
}

int check_user_password(void)
{
	char buf[50];
	struct termio tty;
	int i;
	char c;

	if (!pwd_exist()||!pwd_is_set("user"))
		return 0;

	for (i=0; i<2; i++) {
		ioctl(i,TCGETA,&tty);
		tty.c_lflag &= ~ ECHO;
		ioctl(i,TCSETAW,&tty);
	}
	printf("\nPlease input user password:");

loop0:
	for (i=0; i<50; i++) {
		c=getchar();
		if (c!='\n'&&c!='\r') {	
			printf("*");
			buf[i] = c;
		}
		else {
			buf[i]='\0';
			break;
		}
	}
	
	if (!pwd_cmp("user",buf)) {
		printf("\nPassword error!\n");
		printf("Please input user password:");
		goto loop0;
	}

	for (i=0; i<2; i++) {
		tty.c_lflag |=  ECHO;
		ioctl(i,TCSETAW,&tty);
	}
			
	return 0;
}

int check_admin_password(void)
{
	char buf[50];
	struct termio tty;
	int i;
	char c;

	if (!pwd_exist()||!pwd_is_set("admin"))
		return 0;

	for (i=0; i<2; i++) {
		ioctl(i,TCGETA,&tty);
		tty.c_lflag &= ~ ECHO;
		ioctl(i,TCSETAW,&tty);
	}
	printf("\nPlease input admin password:");

loop1:
	for (i= 0; i<50; i++) {
		c=getchar();
		if (c!='\n'&&c!='\r') {	
			printf("*");
			buf[i] = c;
		}
		else {
			buf[i]='\0';
			break;
		}
	}
	
	if (!pwd_cmp("admin",buf)) {
		printf("\nPassword error!\n");
		printf("Please input admin password:");
		goto loop1;
	}

	for (i=0; i<2; i++) {
		tty.c_lflag |=  ECHO;
		ioctl(i,TCSETAW,&tty);
	}
	
	return 0;
}

int check_sys_password(void)
{
	char buf[50];
	struct termio tty;
	int i;
	char c;
	int count=0;

	if (!pwd_exist()||!pwd_is_set("sys"))
		return 0;

	for (i=0; i<6; i++) {
		ioctl(i,TCGETA,&tty);
		tty.c_lflag &= ~ ECHO;
		ioctl(i,TCSETAW,&tty);
	}
	printf("\nPlease input sys password:");

loop1:
	for (i= 0; i<50; i++) {
		c=getchar();
		if (c!='\n'&&c!='\r') {	
			printf("*");
			buf[i] = c;
		}
		else {
			buf[i]='\0';
			break;
		}
	}
	
	if (!pwd_cmp("sys",buf)) {
		printf("\nPassword error!\n");
		printf("Please input sys password:");
		count++;
		if (count==3)
			return -1;
		goto loop1;
	}

	for (i=0; i<6; i++) {
		tty.c_lflag |=  ECHO;
		ioctl(i,TCSETAW,&tty);
	}

	return 0;
}

/*
 *  Main interactive command loop
 *  -----------------------------
 *
 *  Clean up removing breakpoints etc and enter main command loop.
 *  Read commands from the keyboard and execute. When executing a
 *  command this is where <crtl-c> takes us back.
 */
void __gccmain(void);
void __gccmain(void)
{
}

int main(void)
{
	char prompt[32];

	if (setjmp(jmpb)) {
		/* Bailing out, restore */
		closelst(0);
		ioctl(STDIN, TCSETAF, &consterm);
		printf(" break!\r\n");
	}

#ifdef INET
	signal (SIGINT, pmon_intr);
#else
	ioctl (STDIN, SETINTR, jmpb);
#endif

#if NMOD_DEBUGGER > 0
	rm_bpts();
#endif
	md_setsr(NULL, initial_sr);	/* XXX does this belong here? */

	{
//		check_user_password();
		if(!getenv("al"))
			;
//		load_menu_list();
	}

	{
	static int run = 0;
	char *s;
	int ret = 0;

	if (!run) {

#ifdef BOOT_TEST_NAND_MEM
		s = getenv("boot_test");
		if (!strcmp(s, "yes")) {
			ret = do_cmd("mt -v");
			ret = ret + ls1x_nand_test();
			printf("mt -v %x\n", ret);
			if (ret == 0) {
				ls1x_gpio_direction_output(BOOT_TEST_LED_GREEN, 0);
				gpio_set_value(BOOT_TEST_LED_GREEN, 0);
				setenv("boot_test", "no");
			}
		}
#endif

		s = getenv("update_usb");
		printf("update_usb, %s !\n", s);
		if (!strcmp(s, "yes")) {
			printf("going to update vmlinuz .....\n");
			ret = do_cmd("devcp /dev/fat@usb0/vmlinuz /dev/mtd3");	
			if (ret != 0)
				goto no_update;
			setenv("al", "/dev/mtd3");
			setenv("append", "console=ttyS2,115200 rdinit=/sbin/init");
			setenv("update_usb", "no");
		}
no_update:

	#ifdef FAST_STARTUP
		do_cmd("test");
	#endif
		run = 1;
	#ifdef AUTOLOAD
		s = getenv("al");
		autoload(s);
	#else
		s = getenv("autoboot");
		autorun(s);
	#endif
	}
	}

	while (1) {
		#if 0
		while (1) {
			char c;int i;
			i=term_read(0,&c,1);
			printf("haha:%d,%02x\n",i,c);
		}
		#endif		
		strncpy (prompt, getenv ("prompt"), sizeof(prompt));

	#if NCMD_HIST > 0
		if (strchr(prompt, '!') != 0) {
			char tmp[8], *p;
			p = strchr(prompt, '!');
			strdchr(p);	/* delete the bang */
			sprintf(tmp, "%d", histno);
			stristr(p, tmp);
		}
	#endif
		printf("%s", prompt);
	#if NCMD_HIST > 0
		get_cmd(line);
	#else
		get_line(line, 0);
	#endif
		do_cmd(line);
		console_state(1);
	}
	return(0);
}

#ifdef AUTOLOAD
static void autoload(char *s)
{
	char buf[LINESZ];
	char *pa;
	char *rd;
	unsigned int dly, lastt;
	unsigned int cnt;
	struct termio sav;

	if(s != NULL  && strlen(s) != 0) {
	#ifdef	wait_key
		char *d = getenv ("bootdelay");

		if (!d || !atob (&dly, d, 10) || dly < 0 || dly > 99) {
			dly = 1;
		}

		SBD_DISPLAY ("AUTO", CHKPNT_AUTO);
		printf("Press <Enter> to execute loading image:%s\n",s);
		printf("Press any other key to abort.\n");
		ioctl(STDIN, CBREAK, &sav);
		lastt = 0;
		ioctl(STDIN, FIONREAD, &cnt);

		/* 每次循环为100ms延时，原延时时间为1秒太长 */
		while (dly != 0 && cnt == 0) {
			delay(100000);
			printf ("\b\b%02d", --dly);
			ioctl (STDIN, FIONREAD, &cnt);
		} 

		if (cnt > 0 && strchr("\n\r", getchar())) {
			cnt = 0;
		} else if (cnt > 0 && strchr("u", getchar())) {
			do_cmd("test");
			do_cmd("load /dev/mtd0");
			do_cmd("g console=ttyS2,115200 root=/dev/mtdblock1 rw rootfstype=yaffs2 init=/sbin/init video=ls1bfb:vga1024x768-24@60");
		}

		ioctl(STDIN, TCSETAF, &sav);
		putchar('\n');
	#else
		cnt = 0;
	#endif

		if (cnt == 0) {
			if (getenv("autocmd")) {
				strcpy(buf, getenv("autocmd"));
				do_cmd(buf);
			}
			rd = getenv("rd");
			if (rd != 0) {
				sprintf(buf, "initrd %s", rd);
				if(do_cmd(buf))
					return;
			}

			strcpy(buf, "load ");
			strcat(buf, s);
			if (do_cmd(buf))
				return;
			if ((pa = getenv("append"))) {
				sprintf(buf, "g %s", pa);
			} else if ((pa = getenv("karg"))) {
				sprintf(buf, "g %s", pa);
			} else {
				pa = getenv("dev");
				strcpy(buf, "g root=/dev/");
				if (pa != NULL  && strlen(pa) != 0)
					strcat(buf, pa);
				else
					strcat(buf, "hda1");
				strcat(buf, " console=tty");
			}
			printf("%s\n", buf);
//			delay(100000);
			do_cmd(buf);
		}
	}
}
#else

/*
 *  Handle autoboot execution
 *  -------------------------
 *
 *  Autoboot variable set. Countdown bootdelay to allow manual
 *  intervention. If CR is pressed skip counting. If var bootdelay
 *  is set use the value othervise default to 15 seconds.
 */
static void autorun(char *s)
{
	char buf[LINESZ];
	char *d;
	unsigned int dly, lastt;
	unsigned int cnt;
	struct termio sav;

	if(s != NULL  && strlen(s) != 0) {
		d = getenv ("bootdelay");
		if(!d || !atob (&dly, d, 10) || dly < 0 || dly > 99) {
			dly = 15;
		}

		SBD_DISPLAY ("AUTO", CHKPNT_AUTO);
		printf("Autoboot command: \"%.60s\"\n", s);
		printf("Press <Enter> to execute or any other key to abort.\n");
		ioctl (STDIN, CBREAK, &sav);
		lastt = 0;
		dly++;
		do {
#if defined(HAVE_TOD) && defined(DELAY_INACURATE)
			time_t t;
			t = tgt_gettime ();
			if(t != lastt) {
				printf ("\r%2d", --dly);
				lastt = t;
			}
#else
			delay(1000000);
			printf ("\r%2d", --dly);
#endif
			ioctl (STDIN, FIONREAD, &cnt);
		} while (dly != 0 && cnt == 0);

		if(cnt > 0 && strchr("\n\r", getchar())) {
			cnt = 0;
		}

		ioctl (STDIN, TCSETAF, &sav);
		putchar ('\n');

		if(cnt == 0) {
			strcpy (buf, s);
			do_cmd (buf);
		}
	}
}
#endif

#ifdef FAST_STARTUP
void fast_startup(void)		//lxy
{
#if 0
extern	int vga_available;
#if NMOD_FRAMEBUFFER > 0 
	unsigned long fbaddress;
#endif
#endif

	__init();
	
	CpuOnboardCacheOn = 1;
	CpuExternalCacheOn = 1;	
	CPU_ConfigCache();

	vminit();
	kmeminit();
	init_proc ();
	
#if 0
#if NMOD_FRAMEBUFFER > 0
	printf("begin fb_init\n");
	fbaddress = dc_init();
	fbaddress |= 0xa0000000;
#ifdef GC300
extern unsigned long GPU_fbaddr;
        GPU_fbaddr = fbaddress ;
#endif
	fb_init(fbaddress, 0);
	printf("after fb_init\n");
	vga_available = 1;
#endif
#endif

	printf ("lxy: begin nand_init....\n");
//extern   void norflash_init();
	ls1g_soc_nand_init();
//	printf ("lxy: after norflash_init.....\n");
	do_cmd("load /dev/mtd0");
	printf ("lxy: after load mtd .....\n");
	do_cmd("g console=ttyS2,115200 root=/dev/mtdblock1 rw rootfstype=yaffs2 init=/sbin/init video=ls1bfb:vga1024x768-24@60 quiet");
//	do_cmd("g console=ttyS2,115200 lpj=530432 root=/dev/mtdblock1 rw rootfstype=yaffs2 init=/sbin/init video=ls1bfb:vga1024x768-16@60 quiet");
	printf ("lxy: after cmd_go .....\n");
//	do_cmd("g console=ttyS0,115200 lpj=530432 rdinit=/linuxrc quiet");
//	printf ("lxy: after go .....\n");

}
#endif

/*
 *  PMON2000 entrypoint. Called after initial setup.
 */
void dbginit(char *adr)
{
	int	memsize, freq;
	char	fs[10], *fp;

/*	splhigh();*/

	memsize = memorysize;

	__init();	/* Do all constructor initialisation */

#if NNAND
	ls1x_nand_init();
#endif

	SBD_DISPLAY ("ENVI", CHKPNT_ENVI);
	envinit ();

#if defined(LS1ASOC)
	{
	char buf[10];
	sprintf(buf, "%d", tgt_pipefreq());
	setenv("cpuclock", buf);
	sprintf(buf, "%d", tgt_cpufreq());
	setenv("busclock", buf);
	}
#endif

#if defined(SMP)
	/* Turn on caches unless opted out */
	if (!getenv("nocache"))
		md_cacheon();
#endif

	SBD_DISPLAY ("SBDD", CHKPNT_SBDD);
	tgt_devinit();

//#ifdef INET
	SBD_DISPLAY ("NETI", CHKPNT_NETI);
	init_net (1);
//#endif

#if NCMD_HIST > 0
	SBD_DISPLAY ("HSTI", CHKPNT_HSTI);
	histinit ();
#endif

#if NMOD_SYMBOLS > 0
	SBD_DISPLAY ("SYMI", CHKPNT_SYMI);
	syminit ();
#endif

#ifdef DEMO
	SBD_DISPLAY ("DEMO", CHKPNT_DEMO);
	demoinit ();
#endif

	SBD_DISPLAY ("SBDE", CHKPNT_SBDE);
	initial_sr |= tgt_enable (tgt_getmachtype ());

#ifdef SR_FR
	Status = initial_sr & ~SR_FR; /* don't confuse naive clients */
#endif
	/* Set up initial console terminal state */
	ioctl(STDIN, TCGETA, &consterm);

#ifdef HAVE_LOGO
	tgt_logo();
#else
//	printf ("PMON2000 Professional"); 
#endif
	printf ("\nConfiguration [%s,%s", TARGETNAME,
			BYTE_ORDER == BIG_ENDIAN ? "EB" : "EL");
#ifdef INET
	printf (",NET");
#endif
#if NSD > 0
	printf (",SCSI");
#endif
#if NWD > 0
	printf (",IDE");
#endif
	printf ("]\n%s.\n", vers);
	printf ("Supported loaders [%s]\n", getExecString());
	printf ("Supported filesystems [%s]\n", getFSString());
	printf ("This software may be redistributed under the BSD copyright.\n");

	tgt_machprint();

	freq = tgt_pipefreq ();
	sprintf(fs, "%d", freq);
	fp = fs + strlen(fs) - 6;
	fp[3] = '\0';
	fp[2] = fp[1];
	fp[1] = fp[0];
	fp[0] = '.';
	printf (" %s MHz", fs);

	freq = tgt_cpufreq ();
	sprintf(fs, "%d", freq);
	fp = fs + strlen(fs) - 6;
	fp[3] = '\0';
	fp[2] = fp[1];
	fp[1] = fp[0];
	fp[0] = '.';
	printf (" / Bus @ %s MHz\n", fs);

	printf ("Memory size %3d MB (%3d MB Low memory, %3d MB High memory) .\n", (memsize+memorysize_high)>>20,
		(memsize>>20), (memorysize_high>>20));

	tgt_memprint();
#if defined(SMP)
	tgt_smpstartup();
#endif

	printf ("\n");

	md_clreg(NULL);
	md_setpc(NULL, (int32_t) CLIENTPC);
	md_setsp(NULL, tgt_clienttos ());
}

/*
 *  closelst(lst) -- Handle client state opens and terminal state.
 */
void closelst(int lst)
{
	switch (lst) {
	case 0:
		/* XXX Close all LU's opened by client */
		break;

	case 1:
		break;

	case 2:
		/* reset client terminal state to consterm value */
		clntterm = consterm;
		break;
	}
}

/*
 *  console_state(lst) -- switches between PMON2000 and client tty setting.
 */
void console_state(int lst)
{
	switch (lst) {
	case 1:
		/* save client terminal state and set PMON default */
		ioctl (STDIN, TCGETA, &clntterm);
		ioctl (STDIN, TCSETAW, &consterm);
		break;

	case 2:
		/* restore client terminal state */
		ioctl (STDIN, TCSETAF, &clntterm);
		break;
	}
}

/*************************************************************
 *  dotik(rate,use_ret)
 */
void dotik(int rate, int use_ret)
{
	static	int             tik_cnt;
	static	const char      more_tiks[] = "|/-\\";
	static	const char     *more_tik;

	tik_cnt -= rate;
	if (tik_cnt > 0) {
		return;
	}
	tik_cnt = 256000;
	if (more_tik == 0) {
		more_tik = more_tiks;
	}
	if (*more_tik == 0) {
		more_tik = more_tiks;
	}
	if (use_ret) {
		printf (" %c\r", *more_tik);
	} else {
		printf ("\b%c", *more_tik);
	}
	more_tik++;
}

#if NCMD_MORE == 0
/*
 *  Allow usage of more printout even if more is not compiled in.
 */
int more (char *p, int *cnt, int size)
{ 
	printf("%s\n", p);
	return(0);
}
#endif

/*
 *  Non direct command placeholder. Give info to user.
 */
int no_cmd(int ac, char *av[])
{
    printf("Not a direct command! Use 'h %s' for more information.\n", av[0]);
    return (1);
}

/*
 *  Build argument area on 'clientstack' and set up clients
 *  argument registers in preparation for 'launch'.
 *  arg1 = argc, arg2 = argv, arg3 = envp, arg4 = callvector
 */

void initstack (int ac, char **av, int addenv)
{
	char	**vsp, *ssp;
	int	ec, stringlen, vectorlen, stacklen, i;
	register_t nsp;

	/*
	 *  Calculate the amount of stack space needed to build args.
	 */
	stringlen = 0;
	if (addenv) {
		envsize (&ec, &stringlen);
	}
	else {
		ec = 0;
	}
	for (i = 0; i < ac; i++) {
		stringlen += strlen(av[i]) + 1;
	}
	stringlen = (stringlen + 3) & ~3;	/* Round to words */
	vectorlen = (ac + ec + 2) * sizeof (char *);
	stacklen = ((vectorlen + stringlen) + 7) & ~7;

	/*
	 *  Allocate stack and us md code to set args.
	 */
	nsp = md_adjstack(NULL, 0) - stacklen;
	md_setargs(NULL, ac, nsp, nsp + (ac + 1) * sizeof(char *), (int)callvec);

	/* put $sp below vectors, leaving 32 byte argsave */
	md_adjstack(NULL, nsp - 32);
	memset((void *)((int)nsp - 32), 0, 32);

	/*
	 * Build argument vector and strings on stack.
	 * Vectors start at nsp; strings after vectors.
	 */
	vsp = (char **)(int)nsp;
	ssp = (char *)((int)nsp + vectorlen);

	for (i = 0; i < ac; i++) {
		*vsp++ = ssp;
		strcpy (ssp, av[i]);
		ssp += strlen(av[i]) + 1;
	}
	*vsp++ = (char *)0;

	/* build environment vector on stack */
	if (ec) {
		envbuild (vsp, ssp);
	}
	else {
		*vsp++ = (char *)0;
	}
	/*
	 * Finally set the link register to catch returning programs.
	 */
	md_setlr(NULL, (register_t)_exit);
}

