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
#include <sys/device.h>

#include <target/regs-wdt.h>

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

int autoexec(const char *dev) {
  FILE	   *fp;
  char *cmd,*buf,*ver;
  const char *old_ver;
  int ret=1,i,m,have=0;
  struct device *deva;
  m=strlen(dev);
  for(i=4;i<m;i++){
    if(dev[i]=='@')break;
  }
  i++;  //找到设备名 &dev[i]
  if(strncmp(dev,"/dev/",5)==0) {
    for (deva  = TAILQ_FIRST(&alldevs); deva != NULL; deva = TAILQ_NEXT(deva,dv_list)) {
      if(strcmp(deva->dv_xname,&dev[i])==0) {
	have=1;
	break;
      }
    }
    if(have==0)
      return ret;
  }

  printf("%s ",dev);
  buf=malloc(2048);
  int filelen,cmdlen;
  cmd=malloc(1024);
  ver=malloc(30);
#ifdef LS1BSOC
  sprintf(buf,"%s/autoexec.1b",dev); //1b开发板
#else   
  sprintf(buf,"%s/autoexec.bat",dev); //开龙
#endif
  fp=fopen(buf,"r");
  if(fp){
#ifdef LS1BSOC
    printf("\nrun autoexec.1b from %s\n",dev);
#else   
    printf("\nrun autoexec.bat from %s\n",dev);
#endif
    fgets(buf,300,fp);
    for(i=0;i<20;i++) { //第一行是版本号，
      if(buf[i]==13) break;
      if(buf[i]==10) break;
      if(buf[i]==' ') break; //空格截断
      if(buf[i]=='\t') break;
      ver[i]=buf[i];
      ver[i+1]=0;
    }
    old_ver=getenv("autoexecVer");
    if(!old_ver)  //getenv函数有个问题， 会返回0值，造成strcmp崩溃，所以先要判断下
      old_ver="";
#ifdef UPDATE_KEY
    ls1x_gpio_direction_input(UPDATE_KEY);
    if(gpio_get_value(UPDATE_KEY)==0)
      old_ver="";
#endif
    printf("old_ver=%s,new_ver=%s\n",old_ver,ver);
    if( ver && strcmp(ver,old_ver) != 0) 
    {
      strncpy(buf,dev,100);
      setenv("autoexecDev",buf); //可以在autoexec.bat中用${autoexecDev}调用
      filelen=fread (buf, 1, 2048, fp); //一次读入
      fclose(fp);
      cmdlen=0;
      memset(cmd,0,1024);
      if(filelen>0) 
	for(i=0;i<filelen;i++) {
	  switch(buf[i]) {
	    case 0:
	    case 13:
	    case 10:
              if(strncmp(cmd,"[end]",5)==0) break;
	      if(cmdlen>0 && cmd[0]!='#') { //不是"[end]"
		printf("%s\r\n",cmd);	
		do_cmd(cmd);
	      }
	      memset(cmd,0,1024);
	      cmdlen=0;
	      break;
	    default:
	      cmd[cmdlen]=buf[i];
	      cmdlen++;
	      break;
	  }
          if(strncmp(cmd,"[end]",5)==0) break;
	}
    }
    printf("OK\r\n");
    unsetenv("autoexecDev");
    ret=0; //返回完成， 就不会再查其他的位置的autoexec.bat
    if(ver[0]!='#') {  //第一行的第一个字母是#,则不会更新环境变量autoexecVer, 就可以每次都自动执行。
      setenv("autoexecVer",ver);
#ifdef UPDATE_KEY
      while(gpio_get_value(UPDATE_KEY)==0) ; //等待释放key2,再继续
#endif
      if(strcmp(ver,old_ver)!=0)
	do_cmd("reboot"); //版本有变化才reboot
    }
  }
  free(ver);
  free(buf);
  free(cmd);
  return ret;
}

/* autoexec */
void load_autoexec() {
  char *s;
  s=getenv("autoexec");
  if(!s) s="";

#ifdef UPDATE_KEY
  ls1x_gpio_direction_input(UPDATE_KEY);
  if (gpio_get_value(UPDATE_KEY)==0){
    printf("key2 is down, update...\n");
    s="yes";
  }
#endif

  if(strcmp(s,"yes") == 0){
    if(autoexec("/dev/fat@usb0") == 1
	&& autoexec("/dev/ext2@usb0") == 1)
      if(sdcard_init() == 0xff00)
	if(autoexec("/dev/fat@sdcard0") == 1)
	  autoexec("/dev/ext2@sdcard0");
  }
}

void watchdog() {
  char *s;
  s=getenv("watchdog");
  if(strcmp(s,"yes") == 0) {
    printf("watchdog = yes, set timeout to 34 sec \n");
    __raw_writel(1, LS1X_WDT_EN);
    __raw_writel(0xffffffff, LS1X_WDT_TIMER); //34s
    __raw_writel(1, LS1X_WDT_SET);
  }
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

	char *s;
	s = getenv("al");
	if(s) autoload(s);
	while (1) {
	if(getenv("prompt"))
		strncpy (prompt, getenv ("prompt"), sizeof(prompt));
	else
		strcpy (prompt,"PMON> ");

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
		char *d;
		if(getenv("bootdelay"))
			d = getenv ("bootdelay");
		else
			d = "8";

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
			delay(200000);
			printf ("\b\b%02d", --dly);
			ioctl (STDIN, FIONREAD, &cnt);
		} 

		if (cnt > 0 && strchr("\n\r", getchar())) {
			cnt = 0;
		} else if (cnt > 0 && strchr("u", getchar())) {
			do_cmd("test");
			do_cmd("load /dev/mtd0");
			do_cmd("g console=ttyS2,115200 root=/dev/mtdblock1 rw rootfstype=yaffs2 video=ls1bfb:vga1024x768-24@60 panic=10 noswap");
		}

		ioctl(STDIN, TCSETAF, &sav);
		putchar('\n');
	#else
		cnt = 0;
	#endif

		if (cnt == 0) {
		  	load_autoexec();
			if(getenv("watchdog"))
				watchdog();
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
			do_cmd(buf);
		}
	}
}

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

#ifdef MTDPARTS
int mtdpart_setup_real(char *s);
    mtdpart_setup_real(getenv("mtdparts"));
#endif

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

