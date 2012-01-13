/*
setup syntax
aaa=[bbb=sss||sss=wsd||mlk=wsw]
aaa:$a=[set=set a 1||unset=unset a]
aaa:$#a=[set=set a 1||unset=unset a]
$a:local env
${#a}:global env
${?a}:bool of $a
${?#a}:bool of $a
${?&#a val}:bool of $a&val
${*b}:bool of *(unsigned long *)menu[item].arg
${*}:*(unsigned long *)menu[item].arg
${%}:format of menu[item].arg
${!width: num}
*/
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
#include "mod_vgacon.h"

/* header files */
#include <linux/zlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "setup.h"
#define PREV	CNTRL('P')
#define NEXT	CNTRL('N')
#define FORW	CNTRL('F')
#define BACK	CNTRL('B')
#define BEGIN	CNTRL('A')
#define END	CNTRL('E')
#define DELETE	CNTRL('D')
#define DELLINE	CNTRL('K')
#define MARK	CNTRL(' ')
#define KILL	CNTRL('W')
#define CLEAN	CNTRL('L')
#define UPDATE  0x100
#define UPDATE1  0x101
#define TIMER   0x102
#define MBEGIN 0x103
#define MEND 	0x104
#define RUN -1

void get_line1 (char *p,int usehist);


void cprintfb(int y, int x,int width,char color, const char *buf);
static void popupS(int y, int x,int height,int width);
static void popdownS(int y, int x,int height,int width);
static void msgboxS(int yy,int xx,int height,int width,char *msg);
static void cprintS(int y, int x,int width,char color, const char *text);
static void set_cursorS(unsigned char x,unsigned char y);
extern void set_cursor(unsigned char x,unsigned char y);
extern void set_cursor_fb(unsigned char x,unsigned char y);
static void scr_clearS();
extern void (*__popup)(int y, int x,int height,int width);
extern void (*__popdown)(int y, int x,int height,int width);
extern void (*__cprint)(int y, int x,int width,char color, const char *text);
extern void (*__msgbox)(int yy,int xx,int height,int width,char *msg);
extern void (*__set_cursor)(unsigned char x,unsigned char y);
extern void (*__scr_clear)();
static char *myline;
static char *expline;
static char *ItemName;
//---------------------------------------------------------------------------
/* 前景色 */

#define BLK "\e[30m" /* 黑色 */
#define RED "\e[31m" /* 红色 */
#define GRN "\e[32m" /* 绿色 */
#define YEL "\e[33m" /* 黄色 */
#define BLU "\e[34m" /* 蓝色 */
#define MAG "\e[35m" /* 紫色 */
#define CYN "\e[36m" /* 青色 */
#define WHT "\e[37m" /* 白色 */


/* 背景色 */

#define BBLK "\e[40m" /*黑色 */
#define BRED "\e[41m" /*红色 */
#define BGRN "\e[42m" /*绿色 */
#define BYEL "\e[43m" /* 黄色 */
#define BBLU "\e[44m" /*蓝色 */
#define BMAG "\e[45m" /*紫色 */
#define BCYN "\e[46m" /*青色 */
#define BWHT "\e[47m" /* 白色 */


/* 新增的Ansi颜色定义字符。由 Gothic april 23,1993 */
/* 注意：这些操作符是为VT100终端设计的。 */

#define NOR "\e[0m" /* 返回原色 */
#define BOLD "\e[1m" /* 加亮或打开粗体 */
#define U "\e[4m" /* 下划线模式 */
#define BLINK "\e[5m" /* 不断闪亮模式 */
#define REV "\e[7m" /* 打开反白模式 */
#define CLR "\e[2J" /* 清屏 */
#define HOME "\e[H" /* 发送光标到原处 */
#define REF CLR+HOME /* 清屏和清除光标 */
#define BIGTOP "\e#3" /* Dbl height characters, top half */
#define BIGBOT "\e#4" /* Dbl height characters, bottem half */
#define SAVEC "\e[s" /* Save cursor position */
#define REST "\e[u" /* Restore cursor to saved position */
#define REVINDEX "\eM" /* Scroll screen in opposite direction */
#define SINGW "\e#5" /* Normal, single-width characters */
#define DBL "\e#6" /* Creates double-width characters */
#define FRTOP "\e[2;25r" /*设置2-25行滚动,其他行冻结*/
#define UNFR "\e[r" /*恢复全部滚动*/
/*
注意 "\e[0m\e[37;44m"的等价于" "\e[0;37;44m"
以上的含义可以组合
*/
#define SIZE_OF_EXPLINE 512
//------------------------------------------------------------------------

struct setupMenu *Menu;
#define menu Menu->items
#define popwidth Menu->width
#define popheight Menu->height
static int item=0;
int setup_flag=0;
//----------------------------------------------------------------------------------------


int setup_quit(int);
int default_action(int msg);
int gpio_action(int msg);
static unsigned int gpiodata=0;

#ifdef BONITOEL_CPCI
#include <target/sbd.h>
static char *expand_gpio(char *var) //!tm
{
	  switch(item)
	{
	   case 1: return gpiodata&UART1_232?"232":gpiodata&UART1_422?"422":gpiodata&UART1_485?"485":"none";break;
	   case 2: return gpiodata&UART2_232?"232":gpiodata&UART2_422?"422":gpiodata&UART2_485?"485":"none";break;
	   case 3: return gpiodata&LAN1_EN?"on":"off";break;
	   case 4: return gpiodata&LAN2A_EN?"on":"off";break;
	   case 5: return gpiodata&LAN2B_EN?"on":"off";break;
	   default:return "";
	}
return "";
}

unsigned int _gpio(int argc,char **argv)
{
 int or1[]={0,UART1_232,UART1_422,UART1_485,0};
 int or2[]={0,UART2_232,UART2_422,UART2_485,0};
int msg=argv[1][0];
 switch(item)
 {
  case 1:gpiodata=gpiodata&~(UART1_232|UART1_422|UART1_485);gpiodata|=or1[msg-'0'];break;
  case 2:gpiodata=gpiodata&~(UART2_232|UART2_422|UART2_485);gpiodata|=or2[msg-'0'];break;
  case 3:if(msg=='1')gpiodata=gpiodata|LAN1_EN;
         else if(msg=='2')gpiodata=gpiodata&~LAN1_EN;
		 break;
  case 4:if(msg=='1')gpiodata=gpiodata|LAN2A_EN;
         else if(msg=='2')gpiodata=gpiodata&~LAN2A_EN;
		 break;
  case 5:if(msg=='1')gpiodata=gpiodata|LAN2B_EN;
         else if(msg=='2')gpiodata=gpiodata&~LAN2B_EN;
		 break;
 }
 return gpiodata;
}

int gpio_action(int msg)
{
	if(msg==MBEGIN) gpiodata=*(volatile unsigned int *)PHYS_TO_UNCACHED(PCI_IO_SPACE+GPO_REG);
	return 0;
}
#else
unsigned int _gpio(int msg)
{
return 0;
}

int gpio_action(int msg)
{
return 0;
}
#endif

struct setupMenu gpiomenu={
gpio_action,POP_W,POP_H,
(struct setupMenuitem [])
{
{POP_Y,POP_X,1,1,TYPE_NONE,"    GPIO Setup"},
{POP_Y+1,POP_X,2,2,TYPE_CMD,"(1)uart1:${!gpio uart1}=[232=| _gpio 1 ||422=| _gpio 2||485=| _gpio 3||none=| _gpio 4]"},
{POP_Y+2,POP_X,3,3,TYPE_CMD,"(2)uart2:${!gpio uart2}=[232=| _gpio 1 ||422=| _gpio 2||485=| _gpio 3||none=| _gpio 4]"},
{POP_Y+3,POP_X,4,4,TYPE_CMD,"(3)lan1:${!gpio lan1}=[on=| _gpio 1||off=| _gpio 2]"},
{POP_Y+4,POP_X,5,5,TYPE_CMD,"(4)lan2A:${!gpio lan2A}=[on=| _gpio 1||off=| _gpio 2]"},
{POP_Y+5,POP_X,6,6,TYPE_CMD,"(5)lan2B:${!gpio lan2B}=[on=| _gpio 1||off=| _gpio 2]"},
{POP_Y+6,POP_X,7,7,TYPE_CMD,"(6)save=set gpiocfg ${*0x%08x}",0,&gpiodata},
{POP_Y+7,POP_X,1,1,TYPE_NONE,"(7)quit",setup_quit},
{}
}
};

struct setupMenu netcardmenu={
0,POP_W,POP_H,
(struct setupMenuitem[])
{
{POP_Y,POP_X,1,1,TYPE_CMD,"    NIC Setup"},
{POP_Y+1,POP_X,2,2,TYPE_EDITMENU,"(1)set ip=ic=[em0||em1||fxp0] ip=172.16.21.66;;set ip=ifaddr $ic $ip"},
{POP_Y+2,POP_X,3,3,TYPE_EDITMENU,"(2)ifconfig=ic=[em0||em1||fxp0] ip=172.16.21.66;;set ifconfig=set ifconfig $ic:$ip"},
{POP_Y+3,POP_X,4,4,TYPE_EDITMENU,"(3)serverip for test=ip=${#serverip};;set serverip=set serverip $ip"},
{POP_Y+4,POP_X,5,5,TYPE_EDITMENU,"(4)clientip for test=ip=${#clientip};;set clientip=set clientip $ip"},
{POP_Y+5,POP_X,1,1,TYPE_NONE,"(5)quit",setup_quit},
{}
}
};

struct setupMenu lsmenu={
0,POP_W,POP_H,
(struct setupMenuitem[])
{
{POP_Y,POP_X,1,1,TYPE_NONE,"    List Dirs"},
{POP_Y+1,POP_X,2,2,TYPE_CMD,"(1)list hd's root dirs=| load /dev/fs/ext2@wd0/;| _pause",0},
{POP_Y+2,POP_X,3,3,TYPE_CMD,"(2)list hd's boot dirs=| load /dev/fs/ext2@wd0/boot/;| _pause",0},
{POP_Y+3,POP_X,4,4,TYPE_CMD,"(3)list cd's root dirs=| load /dev/iso9660@cd0/boot/;| _pause",0},
{POP_Y+4,POP_X,5,5,TYPE_CMD,"(4)list fd's root dirs=| load /dev/fs/ext2@fd0/;| _pause",0},
{POP_Y+5,POP_X,6,6,TYPE_CMD,"(5)list hd's dirs of part2=| load /dev/fs/ext2@wd0b/;| _pause",0},
{POP_Y+6,POP_X,7,7,TYPE_CMD,"(6)list hd's dirs of part3=| load /dev/fs/ext2@wd0c/;| _pause",0},
{POP_Y+7,POP_X,8,8,TYPE_CMD,"(7)fdisk=| fdisk wd0;| _pause",0},
{POP_Y+8,POP_X,1,1,TYPE_NONE,"(8)quit",setup_quit},
{}
}
};

struct setupMenu almenu={
0,POP_W,POP_H,
(struct setupMenuitem[])
{
{POP_Y,POP_X,1,1,TYPE_NONE,"    auto load setup"},
{POP_Y+1,POP_X,2,2,TYPE_EDITMENU,"(1)autoboot from hd/fd>=disk=[wd0||fd0] path=boot/vmlinux;;boot=set al /dev/fs/ext2@$disk/$path"},
{POP_Y+2,POP_X,3,3,TYPE_EDITMENU,"(2)autoboot from cdrom>=disk=cd0 path=boot/vmlinux;;boot=set al /dev/iso9660@$disk/$path"},
{POP_Y+3,POP_X,4,4,TYPE_EDITMENU,"(3)autoboot from tftp>=ip=172.16.21.66 path=vmlinux;;boot=set al tftp://$ip/$path"},
{POP_Y+4,POP_X,5,5,TYPE_EDITMENU,"(4)autoboot parameters>=console=tty root=/dev/hda1;;boot=set append 'console=$console root=$root'"},
{POP_Y+5,POP_X,6,6,TYPE_EDITMENU,"(5)autoboot parameters(nfs)>=console=[tty||ttyS0||ttyS1] ip=172.16.21.66 ic=eth0 nfsroot=172.16.21.66:/mnt/sdb2/newdebian;;set append=set append 'console=$console ip=$ip:::::$ic nfsroot=$nfsroot'"},
{POP_Y+6,POP_X,7,7,TYPE_EDITMENU,"(6)autoboot delay >=delay=15;;set delay=set bootdelay $delay"},
{POP_Y+7,POP_X,8,8,TYPE_CMD,"(7)unset autoboot=unset al"},
{POP_Y+8,POP_X,1,1,TYPE_NONE,"(8)quit",setup_quit},
{}
}
};

struct setupMenu dbgmenu={
0,POP_W,POP_H,
(struct setupMenuitem[])
{
{POP_Y,POP_X,1,1,TYPE_NONE,"    debug setup"},
{POP_Y+1,POP_X,2,2,TYPE_CMD,"(1)novga:${?#novga}=[on=| _set novga 1||off=| _unset novga]"},
{POP_Y+2,POP_X,3,3,TYPE_CMD,"(2)nokbd:${?#nokbd}=[on=| _set nokbd 1||off=| _unset nokbd]"},
{POP_Y+3,POP_X,4,4,TYPE_CMD,"(3)old pci cfg:${?#oldcfg}=[on=| _set oldcfg 1||off=| _unset oldcfg]"},
{POP_Y+4,POP_X,5,5,TYPE_CMD,"(4)pci select:${?#pcistep}=[on=| set pcistep 1||off=| _unset pcistep]"},
{POP_Y+5,POP_X,6,6,TYPE_CMD,"(5)print to vga/serial:${*b}=[on=| _m4 ${%p} 1||off=| _m4 ${%p} 0]",0,&vga_available},
{POP_Y+6,POP_X,7,7,TYPE_CMD,"(6)input form kbd/serial:${*b}=[on=| _m4 ${%p} 1||off=| _m4 ${%p} 0]",0,&kbd_available},
{POP_Y+7,POP_X,1,1,TYPE_NONE,"(7)quit",setup_quit},
{}
}
};


char helpmsg[]=
"        other cmds;\n" \
"d1:dump somthing in 1 byte read;\n" \
"d2:dump somthing in 2 byte read;\n" \
"d4:dump somthing in 4 byte read;\n" \
"d8:dump somthing in 8 byte read;\n" \
"m1:modify somthing in 1 byte write;\n" \
"m2:modify somthing in 2 byte write;\n" \
"m4:modify somthing in 4 byte write;\n" \
"m8:modify somthing in 8 byte write;\n" \
"   here something can be memory,pci configure space,disk device;\n" \
"pcs: pcs bus dev func ;\n" \
"   set d1-d4,m1-m4 access pci configure space;\n" \
"disks:disks fd0,disks wd0,disks cd0;\n" \
"   set d1-d4,m1-m4 access disks device;\n" \
"pcs -1;\n" \
"disks -1;\n" \
"   set d1-d4,m1-m4 access normal memory;\n" \
"pnpr: popr LDN index;\n" \
"popw :pnpw LDN index data;\n" \
"   read/write superio configure register;\n" \
"watchdog:;\n" \
"   watchdog test;\n" \
"tpp:;\n" \
"   parallel port test;\n" \
;

struct setupMenu othersmenu={
0,POP_W,POP_H,
(struct setupMenuitem[])
{
{POP_Y,POP_X,1,1,TYPE_NONE,"    others setup"},
{POP_Y+1,POP_X,2,2,TYPE_EDITMENU,"(1)reload pmon form tftp=ip=172.16.21.66 file=gzram;;reload pmon=load tftp://$ip/$file;g"},
{POP_Y+2,POP_X,3,3,TYPE_EDITMENU,"(2)update pmon from disk=disk=[wd0||wd1||fd0] file=gzrom.bin;;update pmon form hd/fd=load -f 0xbfc00000 -r /dev/fs/ext2@$disk/$file;;update pmon from cd=load -f 0xbfc00000 -r /dev/fs/ext2@cd0/$file"},
{POP_Y+3,POP_X,4,4,TYPE_EDITMENU,"(3)update pmon from tftp=ip=172.16.21.66 file=gzrom.bin;;update pmon=load -f 0xbfc00000 -r tftp://$ip/$file"},
{POP_Y+4,POP_X,5,5,TYPE_EDITMENU,"(4)heaptop=heaptop=[80100000||80300000];;set heaptop=set heaptop $heaptop"},
{POP_Y+5,POP_X,6,6,TYPE_MSG,"(5)other info",0,helpmsg},
{POP_Y+6,POP_X,7,7,TYPE_EDITMENU,"(6)${!tm}=zone=${#TZ} year=${!tm_year} month=${!tm_mon} day=${!tm_mday} hour=${!tm_hour} min=${!tm_min} sec=${!tm_sec};;set date=date ${year}${month}${day}${hour}${min}.${sec};;set zone=set TZ ${zone}"},
{POP_Y+7,POP_X,1,1,TYPE_NONE,"(7)quit",setup_quit},
{}
}
};

struct setupMenu mainmenu={
0,POP_W,POP_H,
(struct setupMenuitem[])
{
{POP_Y,POP_X,1,1,TYPE_NONE,"    Main Menu"},
{POP_Y+1,POP_X,2,2,TYPE_EDITMENU,"(1) boot from local>=disk=[wd0||wd1||cd0||fd0] kernelpath=vmlinux console=[tty||ttyS0||ttyS1] root=/dev/hda1 serverip=172.16.21.66;;kernel on hd/fd=load /dev/fs/ext2@$disk/$kernelpath;g console=$console root=$root;;kernel on cd=load /dev/fs/iso9660@$disk/$kernelpath;g console=$console root=$root;;kernel on tftp=load tftp://$serverip/$kernelpath;g console=$console root=$root"},
{POP_Y+2,POP_X,3,3,TYPE_EDITMENU,"(2) boot from nfs  >=disk=[wd0||wd1||cd0||fd0] kernelpath=vmlinux console=[tty||ttyS0||ttyS1]  clientip=172.16.21.66 serverip=172.16.21.66  ic=eth0  root=/mnt/sdb2/newdebian;;kernel on hd/fd=load /dev/fs/ext2@$disk/$kernelpath;g console=$console ip=$clientip:::::$ic nfsroot=$serverip:$root;;kernel on cd=load /dev/fs/iso9660@$disk/$kernelpath;g console=$console ip=$clientip:::::$ic nfsroot=$serverip:$root;;kernel on tftp=load tftp://$serverip/$kernelpath;g console=$console ip=$clientip:::::$ic nfsroot=$serverip:$root"},
{POP_Y+3,POP_X,4,4,TYPE_MENU,    "(3) IP setup       >",0,&netcardmenu},
{POP_Y+4,POP_X,5,5,TYPE_MENU,    "(4) GPIO setup     >",0,&gpiomenu},
{POP_Y+5,POP_X,6,6,TYPE_MENU,    "(5) List DIRS      >",0,&lsmenu},
{POP_Y+6,POP_X,7,7,TYPE_MENU,    "(6) autoload setup >",0,&almenu},
{POP_Y+7,POP_X,8,8,TYPE_MENU,    "(7) debug  setup   >",0,&dbgmenu},
{POP_Y+8,POP_X,9,9,TYPE_MENU,    "(8) others setup   >",0,&othersmenu},
{POP_Y+9,POP_X,10,10,TYPE_CMD,   "(9) test           >=test",0,0},
{POP_Y+10,POP_X,11,11,TYPE_CMD,  "(10)reboot=reboot"},
{POP_Y+11,POP_X,1,1,TYPE_NONE,   "(11)quit",setup_quit},
{}
}
};

//------------------------------------------------------------------
#define MAX_INIT_ENVS 20
#define MAX_INIT_ARGS 20
static char *envp_local[MAX_INIT_ENVS];
static char *argv_local[MAX_INIT_ARGS];
static	int args_local, envs_local;

static void parse_options(char *line)
{
	char *next,*quote,*p;

	if (!*line)
		return;
	args_local = 0;
	envs_local = 0;	/* TERM is set to 'linux' by default */
	next = line;
	while ((line = next) != NULL) {
                quote = strchr(line,'"');
                next = strchr(line, ' ');
                while (next != NULL && quote != NULL && quote < next) {
                        /* we found a left quote before the next blank
                         * now we have to find the matching right quote
                         */
                        next = strchr(quote+1, '"');
                        if (next != NULL) {
                                quote = strchr(next+1, '"');
                                next = strchr(next+1, ' ');
                        }
                }
                if (next != NULL)
                        *next++ = 0;
		
		/*
		 * Then check if it's an environment variable or
		 * an option.
		 */
		if ((p=strchr(line,'='))) {
			if (envs_local >= MAX_INIT_ENVS)
				break;
			if(*p=='"')
			{
			 char *p1=line+strlen(line)-1;
			if(*p1=='"'){ *p1=0; strcpy(p,p+1); }
			}
			envp_local[envs_local++] = line;
		} else {
			if (args_local >= MAX_INIT_ARGS)
				break;
			if (*line) {
				argv_local[args_local++] = line;
			}
		}
	}
	argv_local[args_local+1] = NULL;
	envp_local[envs_local+1] = NULL;
}

char *getenv_local(char *var)
{
 char name[100];
 int len,i;
 strcpy(name,var);
 strcat(name,"=");
 len=strlen(name);
 for(i=0;i<envs_local;i++)
 if(!strncmp(name,envp_local[i],len))return envp_local[i]+len;
 return 0;
}

typedef struct myExpand {
	const char     *name;
    int len;
	char * (*func) __P((char *));
} myExpand;

static char *expand_questionAnd(char *var) //?
{
		char *env;
		char *p=strchr(var,' ');
		if(p)*p++=0;
		else p="-1";
        if(var[2]=='#') env=getenv(var+3);
		else env=getenv_local(var+2);
		if(!env)env="";
		return strtoul(env,0,0)&strtoul(p,0,0)?"on":"off";
}

static char *expand_question(char *var) //?
{
          if(var[1]=='#') return getenv(var+2)?"on":"off";
          else return getenv_local(var+1)?"on":"off";
}

static char *expand_number(char *var) //#
{
return getenv (var+1);
}

static char *expand_star(char *var) //*
{
         char *v=var+20;
         if(var[1]=='b') sprintf(v,"%s",*(unsigned long *)menu[item].arg?"on":"off");
         else sprintf(v,var+1,*(unsigned long *)menu[item].arg);
         return v;
}

static char *expand_percent(char *var) //%
{
        char *v=var+10; 
         sprintf(v,var,menu[item].arg);
        return v;
}

static char *expand_tm(char *var) //!tm
{
struct tm tm;
time_t t;
char *v;

t = tgt_gettime ();
#ifdef HAVE_TOD
tm = *localtime (&t);
#endif
 v=var+20;
 if(!strcmp(var,"!tm"))
 sprintf(v,"%04d-%02d-%02d %02d:%02d:%02d",tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour, tm.tm_min, tm.tm_sec);
 else if(!strcmp(var,"!tm_year"))sprintf(v,"%04d",tm.tm_year+1900);
 else if(!strcmp(var,"!tm_mon"))sprintf(v,"%02d",tm.tm_mon+1);
 else if(!strcmp(var,"!tm_mday"))sprintf(v,"%02d",tm.tm_mday);
 else if(!strcmp(var,"!tm_hour"))sprintf(v,"%02d",tm.tm_hour);
 else if(!strcmp(var,"!tm_min"))sprintf(v,"%02d",tm.tm_min);
 else if(!strcmp(var,"!tm_sec"))sprintf(v,"%02d",tm.tm_sec);
 else v=var-1;
 return v;
}

static char *expand_width(char *var) //!width:
{
        char *v=var+7; 
		popwidth=strtoul(v,0,0);
		return "";
}

static char *expand_height(char *var) //!width:
{
        char *v=var+8; 
		popheight=strtoul(v,0,0);
		return "";
}

myExpand myExpands[]=
{
	{"?&",2,expand_questionAnd},
	{"?",1,expand_question},
	{"#",1,expand_number},
	{"*",1,expand_star},
	{"%",1,expand_percent},
	{"!tm",3,expand_tm},
	{"!width:",7,expand_width},
	{"!height:",8,expand_height},
#ifdef BONITOEL_CPCI
	{"!gpio",5,expand_gpio},
#endif
	{}
};

static char *expand(char *cmdline)
{
	char *ip, *op, *v;
	char _var[256];
	char *var=_var+1;
	int i;
	_var[0]='$';

	if(!strchr (cmdline, '$')) {
	strcpy(expline,cmdline);
		return expline;
	}

	ip = cmdline;
	op = expline;
	while (*ip) {
		if(op >= &expline[SIZE_OF_EXPLINE - 1]) {
			printf ("Line too long after expansion\n");
			return (0);
		}

		if(*ip != '$') {
			*op++ = *ip++;
			continue;
		}

		ip++;
		if(*ip == '$') {
			*op++ = '$';
			ip++;
			continue;
		}

		/* get variable name */
		v = var;
		if(*ip == '{') {
			/* allow ${xxx} */
			ip++;
			while (*ip && *ip != '}') {
				*v++ = *ip++;
			}
			if(*ip && *ip != '}') {
				printf ("Variable syntax\n");
				return (0);
			}
			ip++;
		} else {
			/* look for $[A-Za-z0-9]* */
			while (isalpha(*ip) || isdigit(*ip)) {
				*v++ = *ip++;
			}
		}

		*v = 0;
		for(i=0,v=0;myExpands[i].name;i++)
        {
         if(!strncmp(var,myExpands[i].name,myExpands[i].len))
         {
          v=myExpands[i].func(var);
		 break;
         }
        }
		
        if(!v) v=getenv_local(var);
        if(!v)v=_var;

		if(op + strlen(v) >= &expline[SIZE_OF_EXPLINE - 1]) {
			printf ("Line expansion ovf.\n");
			return (0);
		}

		while (*v) {
			*op++ = *v++;
		}
	}
	*op = '\0';
	return (expline);
}



static char *getItemCmd(char *cmd)
{
 char *p,*next;
 int len;
 p=strchr(cmd,'=');
 if(p){
   next=p+1;
   len=p-cmd;
  }
 else
 {
  next=cmd;
  len=strlen(cmd);
 }
 strncpy(ItemName,cmd,len);
 ItemName[len]=0;
 return next;
}

static char *getItemName(char *cmd)
{
getItemCmd(cmd);
return ItemName;
}

static int ischoice(char *env)
{
 return strstr(env,"=[")?1:0;
}

static char *getChoiceCmd(char *env)
{
char *p;
p=strstr(env,"=[");
if(p)p=strchr(p,']');
if(!p||!p[1])return 0;
else return p+1;
}

static char *getchoice(char *env,int idx)
{
 int i;
 static char chstr[100];
 char *start,*end;
 start=strstr(env,"=[");
 if(!start)return 0;
 start+=2;
 for(i=1;i<=9;i++)
{
 end=strstr(start,"||");
 if(!end){
 end=strstr(start,"]");
 if(!end || i!=idx)break;
 }
 if(i==idx){
 strncpy(chstr,start,end-start);
 chstr[end-start]=0;
 return chstr;
 }
 start=end+2;
}
return 0;
}
//-------------------------------------------------------------------
void selectitem(void)
{
	do{
	if(menu[item].action && menu[item].action(UPDATE1))continue;
	if(Menu->action && Menu->action(UPDATE1))continue;
	default_action(UPDATE1);
	}while(0);
}

void deselectitem(void)
{
	do{
	if(menu[item].action && menu[item].action(UPDATE))continue;
	if(Menu->action && Menu->action(UPDATE))continue;
	default_action(UPDATE);
	}while(0);
}

void nextitem(void)
{
	item=menu[item].dnext;
}

void previtem(void)
{
int i;
for(i=item;menu[i].dnext!=item;i=menu[i].dnext);
item=i;
}

void rightitem(void)
{
	item=menu[item].rnext;
}

void leftitem(void)
{
int i;
for(i=item;menu[i].rnext!=item;i=menu[i].dnext);
item=i;
}
//----------------------------------------------------------------

void do_menu(struct setupMenu *newmenu);
static void do_editmenu(void);

static void printmenu(void)
{
int olditem=item;
	for(item=0;menu[item].msg;item++)
	{
	if(item==olditem)continue;
	if(menu[item].action && menu[item].action(UPDATE))continue;
	if(Menu->action && Menu->action(UPDATE))continue;
	default_action(UPDATE);
	}
item=olditem;
	selectitem();
}


static char* edit_cmdline(char *cmd)
{
__set_cursor(0,INFO_Y);
get_line(cmd,0);
__set_cursor(0,INFO_Y);
return cmd;
}

static char* edit_cmdline1(char *cmd)
{
int x=0;
char *pcmd=cmd;
pcmd=strchr(cmd,'=');
if(pcmd){pcmd++;x=pcmd-cmd;}
else pcmd=cmd;
__set_cursor(x,INFO_Y);
get_line1(pcmd,0);
__set_cursor(0,INFO_Y);
return pcmd;
}

static char* edit_cmdline2(char *cmd)
{
__set_cursor(0,INFO_Y);
get_line1(cmd,0);
__set_cursor(0,INFO_Y);
return cmd;
}

static int do_ok(int msg)
{
char *cmd;
	__popdown(POP_Y,POP_X,POP_H,popwidth);
	switch(menu[item].type)
	{
	case TYPE_MENU:do_menu(menu[item].arg);break;
	case TYPE_CMD:
    if(ischoice(menu[item].msg)){
	cmd=getChoiceCmd(menu[item].msg);
	if(!cmd)break;
	}
	else
	{
    cmd=getItemCmd(menu[item].msg);
    cmd=getItemCmd(menu[item].msg);
	}
    cmd=expand(cmd);
	if(msg=='e'){strcpy(myline,cmd);do_cmd(edit_cmdline(myline));}
	else do_cmd(cmd);
	break;
	case TYPE_MSG:__msgbox(MSG_Y,MSG_X,MSG_H,MSG_W,menu[item].arg);break;
	case TYPE_EDITMENU:do_editmenu();break;
	}
	__popup(POP_Y,POP_X,POP_H,popwidth);
	printmenu();
return 0;
}

static int default_update(void)
{
	switch(menu[item].type)
	{
	case TYPE_CMD:
	   getItemCmd(menu[item].msg);
	   __cprint(menu[item].y,menu[item].x,popwidth,0x17,expand(ItemName));
	 default:
	   __cprint(menu[item].y,menu[item].x,popwidth,0x17,expand(getItemName(menu[item].msg)));break;
	}
	return 1;
}

static int default_update1(void)
{
char *str=myline;
char *cmd;
	switch(menu[item].type)
	{
	 case TYPE_CMD:
	   cmd=getItemCmd(menu[item].msg);
	   __cprint(menu[item].y,menu[item].x,popwidth,0x70,expand(ItemName));
       if(ischoice(menu[item].msg))
       {
		int i;
		str=myline;
		for(i=1;i<=9;i++)
		{
		cmd=getchoice(menu[item].msg,i);
		if(!cmd)break;
		getItemCmd(cmd);
		str +=sprintf(str,"%d:%s ",i,expand(ItemName));
		}
		str=myline;
	   __cprint(INFO_Y,0,INFO_W,0x70,str);
       }
       else
       {
		cmd=getItemCmd(cmd);
	   __cprint(INFO_Y,0,INFO_W,0x70,expand(ItemName));
       }
      break;

	 default:
	   __cprint(menu[item].y,menu[item].x,popwidth,0x70,expand(getItemName(menu[item].msg)));
	   __cprint(INFO_Y,0,INFO_W,0x70,0);
        break;
	}

	return 1;
}

static int default_digital(int msg)
{
char *p,*cmd;

 switch(menu[item].type)
 {
 case TYPE_CMD:
    if(!ischoice(menu[item].msg))return 1;
	p=getchoice(menu[item].msg,msg-'0');
    if(!p)return 1;
    cmd=expand(getItemCmd(p));
    do_cmd(cmd);
    selectitem();
	break;
 default:return 0;
}
return 1;
}

int default_action(int msg)
{
	switch(msg)
	{
	  case NEXT:deselectitem();nextitem();selectitem();break;
	  case PREV:deselectitem();previtem();selectitem();break;
	  case FORW:deselectitem();rightitem();selectitem();break;
	  case BACK:deselectitem();leftitem();selectitem();break;
	  case 'e':
	  case '\n':
	  case '\r':
			do_ok(msg);break;
	  case 0x1b:deselectitem();setup_flag=-1;break;
	  case UPDATE: default_update();break;
	  case UPDATE1: default_update1();break;
	  case TIMER:
	  case CLEAN:
	  	   	printmenu();break;
	  default:
	  if(msg>='1' && msg <= '9')default_digital(msg);
	  else return 0;
	  break;
	}
	return 1;
}



int setup_quit(int msg)
{
if(msg=='\n'|| msg=='\r'){setup_flag=-1;return 1;}
return 0;
}

#define MAXITEM sizeof(menu)/sizeof(struct setupMenuitem)
#define MINIITEM 1


void do_menu(struct setupMenu *newmenu)
{
	struct setupMenu *savedmenu = Menu;
	int saveditem = item;
	Menu = newmenu;
	if(!ItemName||!expline||!myline) __console_alloc();
	item = menu[0].dnext;
	popwidth = Menu->width;
	popheight = Menu->height;
	if(!Menu->action||!Menu->action(MBEGIN))
		default_action(MBEGIN);
	setup_flag = 0;
	__popup(menu[0].y,menu[0].x,popheight,popwidth);
	printmenu();

	while(!setup_flag) {
		int msg = getch();
		if(menu[item].action && menu[item].action(msg))continue;
		if(Menu->action && Menu->action(msg))continue;
		default_action(msg);
	}
	__popdown(menu[0].y,menu[0].x,popheight,popwidth);
	if(!Menu->action||!Menu->action(MEND))
		default_action(MEND);
	setup_flag = 0;
	Menu = savedmenu;
	if(Menu){
		popwidth = Menu->width;
		popheight = Menu->height;
	}
	item = saveditem;
}

static int cmd_setup(int ac,char **av)
{
	__console_alloc();
	do_menu(&mainmenu);
	__console_free();
	return 0;
}

//-------------------------------------------------------------------------------------
static int _pause(int ac,char **av)
{
printf("\npress any key to continue\n");
while(getch()>=0x100);
return 0;
}

typedef struct myCmd {
	const char     *name;
	int	       (*func) __P((int, char *[]));
} myCmd;

static int _m4(int ac,char **av)
{
unsigned long *addr;
addr=(unsigned long *)strtoul(av[1],0,0);
*addr=strtoul(av[2],0,0);
return 0;
}

static int _set(int ac,char **av)
{
setenv(av[1],av[2]);
return 0;
}

static int _unset(int ac,char **av)
{
unsetenv(av[1]);
return 0;
}

static int _or(int ac,char **av)
{
char str[16];
char *env;
unsigned long x,y;
env=getenv(av[1]);
if(env)x=strtoul(env,0,0);
else x=0;
y=strtoul(av[2],0,0);
sprintf(str,"%d",x|y);
setenv(av[1],str);
return 0;
}

static int _andn(int ac,char **av)
{
char str[16];
char *env;
unsigned long x,y;
env=getenv(av[1]);
if(env)x=strtoul(env,0,0);
else x=0;
y=strtoul(av[2],0,0);
sprintf(str,"%d",x&~y);
setenv(av[1],str);
return 0;
}

static int _quit(int ac,char **av)
{
	setup_flag=-1;
return 0;
}

static const myCmd myCmds[] =
{
	{"_pause",_pause},
	{"_m4",_m4},
	{"_set",_set},
	{"_unset",_unset},
	{"_or",_or},
	{"_andn",_andn},
	{"_quit",_quit},
	{"_gpio",_gpio},
	{0, 0}
};


static int cmd_or(int ac,char **av)
{
int i;
int needfree=0;
if(ac<2)return -1;
for(i=0;myCmds[i].name;i++)
{
if(!strcmp(av[1],myCmds[i].name)){
myCmds[i].func(ac-1,&av[1]);
return 0;
}
}
if(!myline){myline=malloc(1000);needfree=1;}
myline[0]=0;
for(i=1;i<ac;i++)
{
strcat(myline,av[i]);
strcat(myline," ");
}
do_cmd(myline);
if(needfree){free(myline); myline=0;}
return 0;
}

//-------------------------------------------------------------------------------------------
static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"setup","",0,"setup boot loader",cmd_setup,0,99,CMD_REPEAT},
	{"|","cmd",0,"run cmd and return 0",cmd_or,0,99,CMD_REPEAT},
	{0, 0}
};


static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
//--------------------------------------------------------------------------
static int items_edit;
#define menu_edit_items menu_edit.items
static int editmenu_action(int msg);
struct setupMenu menu_edit={
editmenu_action,POP_W,POP_H,
(struct setupMenuitem [POP_H]){}
};

static int edit_digital(int msg)
{
char *p;

 if(ischoice(menu[item].msg))
 {
	p=getchoice(menu[item].msg,msg-'0');
	if(p)strcpy(strchr(envp_local[item-1],'=')+1,p);
    __cprint(menu[item].y,menu[item].x,popwidth,0x70,envp_local[item-1]);
 }
return 1;
}

static int edit_update(void)
{
	   if(item>0 && item<=envs_local)
		{
			 __cprint(menu[item].y,menu[item].x,popwidth,0x17,envp_local[item-1]);
		}
		else if(item>envs_local && item<items_edit)
		{
		 char *p;
		 p=strchr(menu[item].msg,'=');
		 *p=0;
	     __cprint(menu[item].y,menu[item].x,popwidth,0x17,expand(menu[item].msg));
		 *p='=';
		}
	   else __cprint(menu[item].y,menu[item].x,popwidth,0x17,expand(getItemName(menu[item].msg)));
return 1;
}


static int edit_update1(void)
{
char *p;
char *penv=0;
	if(item>0 && item<=envs_local)
	{
			 __cprint(menu[item].y,menu[item].x,popwidth,0x70,envp_local[item-1]);
	}
	else if(item<items_edit)
	{
	 char *p;
	 p=strchr(menu[item].msg,'=');
	 *p=0;
	 __cprint(menu[item].y,menu[item].x,popwidth,0x70,expand(menu[item].msg));
	 *p='=';
	}
	else __cprint(menu[item].y,menu[item].x,popwidth,0x70,menu[item].msg);
	
	if(menu[item].type==TYPE_CHOICE)
	{
	int i;
	penv=myline;
	for(i=1;i<=9;i++)
	{
	p=getchoice(menu[item].msg,i);
	if(!p)break;
	penv +=sprintf(penv,"%d:%s ",i,p);
	}
	penv=myline;
	}
	else if(item<=envs_local) penv=envp_local[item-1];
	else if(item<items_edit) penv=expand(menu[item].arg);
	else penv=0;
	 __cprint(INFO_Y,0,INFO_W,0x70,penv);
	return 1;
}

static int quit_edit(int msg)
{
	int i;
if(msg=='\n'|| msg=='\r'){
	for(i=1;i<items_edit;i++)
	if(menu[i].arg)
	{
		free(menu[i].arg);
		menu[i].arg=0;
	}
 	setup_flag=-1;	
	return 1;
  }
return 0;
}

static void edit_ok(int msg)
{
	if(item<=envs_local)
	{
	strcpy(myline,envp_local[item-1]);
	if(menu[item].arg)free(menu[item].arg);
	__cprint(INFO_Y,0,INFO_W,0x7,myline);
	edit_cmdline1(myline);
	__cprint(INFO_Y,0,INFO_W,0x70,myline);
	envp_local[item-1]=menu[item].arg=malloc(strlen(myline)+1);
	strcpy(menu[item].arg,myline);
	selectitem();
	}
	else if(item<items_edit) //run
	{
	 if(msg=='e')
	 {
	strcpy(myline,menu[item].arg);
	free(menu[item].arg);
	__cprint(INFO_Y,0,INFO_W,0x7,myline);
	edit_cmdline2(myline);
	menu[item].arg=malloc(strlen(myline)+1);
	strcpy(menu[item].arg,myline);
	__cprint(INFO_Y,0,INFO_W,0x70,expand(menu[item].arg));
	 }
	 else
	 {
	__popdown(POP_Y,POP_X,POP_H,popwidth);
		do_cmd(expand(menu[item].arg));
	__popup(POP_Y,POP_X,POP_H,popwidth);
	printmenu();
	 }
	}
}

static int editmenu_action(int msg)
{
	switch(msg)
	{
	  case 'e':
	  case '\n':
	  case '\r':
			edit_ok(msg);break;
	  case 0x1b:deselectitem();quit_edit('\n');break;
	  case UPDATE: edit_update();break;
	  case UPDATE1: edit_update1();break;
	  case CLEAN:
	  	   	printmenu();break;
	  default:
	  if(msg>='1' && msg <= '9')edit_digital(msg);
	  else	return 0;break;
	}
	return 1;
}

static void makeup_menu(char *text,char *env,char *cmdline)
{
int i;
parse_options(env); 

for(i=0;i<=envs_local;i++)
{
menu_edit_items[i].y=POP_Y+i;
menu_edit_items[i].x=POP_X;
menu_edit_items[i].dnext=i+1;
menu_edit_items[i].rnext=i+1;
if(i==0)menu_edit_items[i].msg=text;
else menu_edit_items[i].msg=envp_local[i-1];
menu_edit_items[i].arg=0;
menu_edit_items[i].action=0;
if(i<=envs_local && strstr(envp_local[i-1],"=["))
{
menu_edit_items[i].type=TYPE_CHOICE;
menu_edit_items[i].arg=envp_local[i-1]=malloc(strlen(menu_edit_items[i].msg));
strcpy(envp_local[i-1],menu_edit_items[i].msg);
strcpy(strchr(envp_local[i-1],'=')+1,getchoice(menu_edit_items[i].msg,1));
}
else menu_edit_items[i].type=TYPE_NONE;
}
for(i=envs_local+1;cmdline;i++)
{
char *p,*p1;
int len;
p=cmdline;
menu_edit_items[i].msg=p;
p1=strchr(p,'=')+1;
cmdline=strstr(cmdline,";;");
if(cmdline)cmdline+=2;
len=cmdline?cmdline-p1-2:strlen(p1);
p=menu_edit_items[i].arg=malloc(len+1);
strncpy(p,p1,len);
p[len]=0;
menu_edit_items[i].y=POP_Y+i;
menu_edit_items[i].x=POP_X;
menu_edit_items[i].dnext=i+1;
menu_edit_items[i].rnext=i+1;
menu_edit_items[i].type=TYPE_NONE;
menu_edit_items[i].action=menu[item].action;
}
items_edit=i;
menu_edit_items[0].dnext=envs_local+1;
menu_edit_items[0].action=editmenu_action;
menu_edit_items[i].y=POP_Y+i;
menu_edit_items[i].x=POP_X;
menu_edit_items[i].dnext=1;
menu_edit_items[i].rnext=1;
menu_edit_items[items_edit].action=quit_edit;
menu_edit_items[items_edit].msg="quit";
menu_edit_items[items_edit+1].msg=0;
}

static void do_editmenu(void)
{
char *env,*p,*p1;
int len;
char *cmd;
cmd=getItemCmd(menu[item].msg);
p=strstr(cmd,";;");
p[0]=0;
p1=expand(cmd);
len=strlen(p1);
env=malloc(len+1);
strcpy(env,p1);
p[0]=';';
  makeup_menu(menu[item].msg,env,p+2);
  do_menu(&menu_edit);
envs_local=0;
args_local=0;
free(env);
}

void __console_alloc(void)
{
 __console_init();
if(!myline)myline=malloc(1000);
if(!expline)expline=malloc(SIZE_OF_EXPLINE);
if(!ItemName)ItemName=malloc(100);
}
void __console_free(void)
{
free(ItemName); ItemName=0;
free(expline); expline=0;
free(myline); myline=0;
}
