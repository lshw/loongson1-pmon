#include <stdio.h>
#include <termio.h>
#include <string.h>
#include <setjmp.h>
#include <sys/endian.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif

#include <machine/cpu.h>

#include <pmon.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <flash.h>
#include <time.h>

/* header files */
#include <linux/zlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "setup.h"

extern void delay1(int);
extern void (*__cprint)(int y, int x,int width,char color, const char *text);
static int pause()
{
char cmd[20];
struct termio tbuf;
printf("\n");
__cprint(INFO_Y,0,INFO_W,0x70,"press enter to continue");
ioctl (STDIN, SETNCNE, &tbuf);
gets(cmd);
ioctl (STDIN, TCSETAW, &tbuf);
__cprint(INFO_Y,0,INFO_W,0x7,0);
return 0;
}


extern void (*__msgbox)(int yy,int xx,int height,int width,char *msg);
static int cpuinfo(){
	int	memsize, freq;
	char	fs[10], *fp;
	char	*s;
printf("cpu info:\n");
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

	//tgt_memprint();
}


static int meminfo(){
printf("mem info:\n");
	printf ("Memory size 512 MB .\n");
return 0;
}

static int uartinfo()
{
printf("uart info:\n");
printf("serial max baud 115200\n");
//	__msgbox(MSG_Y,MSG_X,MSG_H,MSG_W,menu[item].arg);
return 0;
}

static int (*oldwrite)(int fd,char *buf,int len)=0;
static char *buffer;
static int total;
void *restdout(int  (*newwrite) (int fd, const void *buf, size_t n));
static int newwrite(int fd,char *buf,int len)
{
memcpy(buffer+total,buf,len);
total+=len;
return len;
}


static int netinfo()
{
char cmd[100];
#if 0
oldwrite=restdout(newwrite);
total=0;
buffer=heaptop+0x100000;
#endif
printf("net info:\n");
printf("82546 em0 info:\n");
strcpy(cmd,"ifconfig em0;ifconfig em0 status;");
do_cmd(cmd);
printf("82546 em1 info:\n");
strcpy(cmd,"ifconfig em1;ifconfig em1 status;");
do_cmd(cmd);
printf("82559 fxp0 info:\n");
strcpy(cmd,"ifconfig fxp0;ifconfig fxp0 status");
do_cmd(cmd);
printf("link speed up to 100 Mbps\n");
#if 0
restdout(oldwrite);
buffer[total]='\n';
buffer[total+1]=0;
__msgbox(0,0,24,80,buffer);
#endif
//	__msgbox(MSG_Y,MSG_X,MSG_H,MSG_W,menu[item].arg);
}

static int cmd_test(int ac,char **av)
{
__console_alloc();
if(ac==1){
cpuinfo();
meminfo();
uartinfo();
netinfo();
}
else if(!strcmp(av[1],"cpu")) cpuinfo();
else if(!strcmp(av[1],"mem")) meminfo();
else if(!strcmp(av[1],"uart")) uartinfo();
else if(!strcmp(av[1],"net")) netinfo();
			pause();
}

static struct setupMenu testmenu={
0,POP_W,POP_H,
(struct setupMenuitem[])
{
{POP_Y,POP_X,1,1,TYPE_NONE,"    board test"},
{POP_Y+1,POP_X,2,2,TYPE_CMD,"(1)cpu info=sysinfo cpu"},
{POP_Y+2,POP_X,3,3,TYPE_CMD,"(2)memory info=sysinfo mem"},
{POP_Y+3,POP_X,4,4,TYPE_CMD,"(3)uart info=sysinfo uart"},
{POP_Y+4,POP_X,5,5,TYPE_CMD,"(4)net info=sysinfo net"},
{POP_Y+5,POP_X,1,1,TYPE_CMD,"(5)quit=| _quit"},
{}
}
};

static int cmd_setup(int ac,char **av)
{
__console_alloc();
do_menu(&testmenu);
__console_free();
return 0;
}


static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"sysinfo","[cpu|mem|uart|net]",0,"hardware test",cmd_test,0,99,CMD_REPEAT},
	{"info","",0,"hardware test",cmd_setup,0,99,CMD_REPEAT},
	{0,0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));
static void init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
