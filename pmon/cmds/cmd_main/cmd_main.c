/*
	PMON command main
	Designed by LiuXiangYu(¡ıœË”Ó) USTC 04011
*/
#include <stdio.h>
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
#include "window.h"

#include "mod_debugger.h"
#include "mod_symbols.h"

#include "sd.h"
#include "wd.h"

#include <time.h>

#define TM_YEAR_BASE 1900
#include "cmd_hist.h"
#include "cmd_more.h"
extern int             memorysize;
extern int             memorysize_high;
void run(char *cmd)
{
	char t[100];
	strcpy(t,cmd);
	do_cmd(t);
}
struct _daytime
{
	char *name;
	short x,y;
	char buf[8];
	short buflen,base;
}daytime[6]=	
{
	{"Sec",29,6,"",4,0},
	{"Min",16,6,"",4,0},
	{"Hour",1,6,"",4,0},
	{"Day",29,5,"",4,0},
	{"Month",16,5,"",4,1},
	{"Year",1,5,"",6,TM_YEAR_BASE}
};

int	cmd_main __P((int, char *[]));
int cmd_main
(ac, av)
	int ac;
	char *av[];
{
	char *message;//message passing through windows
	char* hint="";
	int oldwindow;//save the previous number of oldwindow
	char tinput[256];//input buffer1
	char line[100];
	char prompt[32];
	char w0[6][50];//buffer of window0"basic"
	char w1[6][50];//buffer of window1"boot"
	char w2[6][50];//buffer of window2"network"
	char sibuf[4][20];
	//daytime modify
	int bp;
	int n = 0;
	int i = 0;//for cycle
	unsigned int a;
	time_t t;//current date and time
	struct tm tm;
	int freq;//cpu freq
	char *maintabs[]={"Main","Boot","Network","Exit"};
	char *f1[]={"wd0","wd1","cd0","fd0",0};
	char *f1b[]={"tty","ttyS0","ttyS1",0};
	char *f2[]={"rtl0","em0","em1","fxp0",0};

	w_init();

	w_setpage(0);

	strcpy(tinput,"");
	strcpy(w1[0],"vmlinux");
	strcpy(w1[1],"/dev/hda1");
	strcpy(w1[2],"10.0.0.3");
	strcpy(w2[0],"192.168.110.176");	
	strcpy(w2[1],"192.168.110.176");

	strcpy(sibuf[0],f1[0]);
	strcpy(sibuf[1],f1b[0]);
	strcpy(sibuf[2],f2[0]);
	strcpy(sibuf[3],f2[0]);

	bp=0;	
	while(1)
	{
		if(w_getpage()>=0 && w_getpage()<4)
		{
			if(w_keydown('[C'))//HOOK  keyboard right
				w_setpage(w_getpage()+1&3);
			if(w_keydown('[D'))//HOOK keyboard left
				w_setpage(w_getpage()-1&3);
		}
		if(w_keydown('`') || w_keydown('~'))//HOOK keyboard ~/`
			w_setpage(101);

		for(i=0;i<4;i++)
		{
			if(w_getpage()==i)
				w_setcolor(0xe0,0,0);
			else 
				w_setcolor(0,0,0);
			w_window(i*20+1,1,18,1,maintabs[i]);
		}
		w_defaultcolor();

	
		if((w_getpage() >=0 && w_getpage()<6) || (w_getpage() == 101)/* && (w_getpage() != 31 && (w_getpage() != -100))*/) {
			w_window(49,3,30,19,"HINT");
			w_bigtext(49,4,30,15,hint);
		}
		
		/* Call the tgt_gettime() funciton every 50 cycles to ensure that the bios time could update in time */
		if(a>800)
		{
			a=0;
			 t = tgt_gettime();
			 tm = *localtime(&t);
		}
		a++;

		switch(w_getpage())
		{
		case 0://Main window.Also display basic information
			oldwindow = 0;
			w_window(1,3,47,19,"Basic Information");
			hint = "This is the basic information of GodSon Computer.";
			if(w_button(21,4,20,"[Modify Time & Date]"))w_setpage(5);
			if(w_focused()) {
				hint = "You can modify Time and Date here.";
			}
			/* Display the current date and time recored in BIOS */
			w_text(1,4,WA_LEFT,"Time and Date:");
			for(i=0;i<6;i++)
			{
				sprintf(line,"%s:%d",daytime[i].name,((int *)(&tm))[i]+daytime[i].base);
				w_text(daytime[i].x,daytime[i].y,WA_LEFT,line);
			}
			/* Display CPU information */
			sprintf(line,"Primary Instruction cache size: %dkb",CpuPrimaryInstCacheSize / 1024);
			w_bigtext(1,9,40,2,line);
			sprintf(line,"Primary Data cache size: %dkb ",CpuPrimaryDataCacheSize / 1024);
			w_bigtext(1,10,40,2,line);
			if(CpuSecondaryCacheSize != 0) {
				sprintf(line,"Secondary cache size: %dkb", CpuSecondaryCacheSize / 1024);
			}
			w_bigtext(1,11,40,2,line);
			if(CpuTertiaryCacheSize != 0) {
				sprintf(line,"Tertiary cache size: %dkb", CpuTertiaryCacheSize / 1024);
			}
			
			sprintf(line,"CPU: %s @ %d MHz",md_cpuname(),tgt_pipefreq()/1000000);
			w_text(1,7,WA_LEFT,line);
			sprintf (line,"Memory size: %3d MB", (memorysize + memorysize_high)>>20);
			w_bigtext(1,8,40,2,line);
			break;
		case 1://Boot related functions
			oldwindow = 1;
			hint = "";
			w_window(1,3,47,19,"Select boot option");
			w_text(2,4,WA_LEFT,"Boot from local");
			w_selectinput(18,5,20,"Set disk:       ",f1,sibuf[0],20);
			if(w_focused()) {
				hint = "Set the disk you want to boot.Use <Enter> to switch, other keys to modify.";
			}
			w_input(18,6,20,"Set kernel path:",w1[0],50);
			if(w_focused()) {
				hint = "Set kernel path. Just input the path in the textbox";
			}
			w_selectinput(18,7,20,"Set console:    ",f1b,sibuf[1],20);
			if(w_focused()) {
				hint = "Set console.Use <Enter> to switch, other keys to modify.";
			}
			w_input(18,8,20,"Set root:       ",w1[1],50);
			if(w_focused()) {
				hint = "Set root path. Just input the path in the textbox.";
			}
			w_input(18,9,20,"Set server ip:  ",w1[2],50);
			if(w_focused()) {
				hint = "Set server ip. Just input the path in the textbox.";
			}
			if(w_button(2,10,10,"[BOOT NOW]")) {
				hint = "";	
				sprintf(line,"set disk %s",sibuf[0]);
				run(line);
				sprintf(line,"set kernelpath %s",w1[0]);
				run(line);
				sprintf(line,"set console %s",sibuf[1]);
				run(line);
				sprintf(line,"set root %s",w1[1]);
				run(line);
				sprintf(line,"set serverip %s",w1[2]);
				run(line);
				run("load /dev/fs/ext2@$disk/$kernelpath;g console=$console root=$root;");
				message = "Boot failed"; 
				w_setpage(100);
			}
		break;
		case 2:
			oldwindow = 2;
			hint = "";
			w_window(1,3,47,19,"Modify the Network configuration");
			w_text(2,5,WA_LEFT,"Set IP for current system");
			w_selectinput(17,6,20,"Select IC      ",f2,sibuf[2],20);
			if(w_focused()) {
				hint = "Option:rtl0,em0,em1,fxp0.Press Enter to Switch, other keys to modify";
			}
			w_input(17,7,20,"New IP Address:",w2[0],50);
			if(w_focused()) {
				hint = "Please input new IP in the textbox.";
			}
			if(w_button(19,8,10,"[Set IP]")){
					ifconfig(sibuf[2],w2[0]);//configure
						sprintf(line,"The device [%s] now has a new IP",sibuf[2]);
						message = line;
						w_setpage(100);
			}
			w_text(2,10,WA_LEFT,"Set IP for current system and save it to CMOS");
			w_selectinput(17,11,20,"Select IC      ",f2,sibuf[3],20);
			if(w_focused()) {
				hint = "Option:rtl0,em0,em1,fxp0.Press Enter to Switch, other keys to modify";
			}
			w_input(17,12,20,"New IP Address:",w2[1],50);
			if(w_focused()) {
				hint = "Please input new IP in the textbox.";
			}
			if(w_button(19,13,10,"[Set IP]")){
						sprintf(line,"set ifconfig %s:%s",sibuf[3],w2[1]);
						printf(line);
						run(line);//configure
						sprintf(line,"The device [%s] now has a new IP",sibuf[3]);
						message = line;
						w_setpage(100);
			}
		break;
		case 3://Save configuration and reboot the system
			oldwindow = 3;
			hint = "";
			w_window(1,3,47,19,"Save configuration and/or Restart the system");
			if(w_button(3,5,20,"[ Restart the system ]")) 
				w_setpage(31);
			if(w_focused())
				hint="<Enter> to restart system.";
			if(w_button(3,6,20,"[ Run Command ]"))
				w_setpage(101);		
			if(w_focused())
				hint="<Enter> to run PMON commond";
			if(w_button(3,7,20,"  [ Return to PMON ]  "))
				{
					w_enterconsole();
					return(0);
				}
			if(w_focused())
				hint="<Enter> to restart system.";

		break;

		case 31:
			w_window(20,8,40,8,"WARRNING");
			w_text(40,10,WA_CENTRE,"Are you sure to restart the system?");
			if(w_button(37,12,10,"[ YES ]"))w_setpage(-4);
			if(w_button(37,14,10,"[ NO ]"))w_setpage(oldwindow);
		break;

		case 5://Modify Time and Date
			oldwindow=0;
			
			w_window(1,3,47,19,"Modify Time and Date");
			//w_input(20,8,20,"New Time & Date: ",w0[0],50);
			if(w_button(14,16,22,"[ Return ]"))w_setpage(oldwindow);
			if(w_focused())
				hint = "<Enter> to return. Up Arrow key to modify";
			for(i=0;i<6;i++)
			{
				if(w_input(daytime[i].x+8,daytime[i].y,6,daytime[i].name,daytime[i].buf,daytime[i].buflen))
				{
					sprintf(line,"date %04s%02s%02s%02s%02s.%02s",
daytime[5].buf,daytime[4].buf,daytime[3].buf,daytime[2].buf,daytime[1].buf,daytime[0].buf);
					run(line);
					w_setfocusid(w_getfocusid()-1);
					
				}
				if(w_focused())
				{
					sprintf(line,"input %s, <Enter> to confirm.",daytime[i].name);
					hint=line;
				}
				else
					sprintf(daytime[i].buf,"%d",((int *)(&tm))[i]+daytime[i].base);
			}
		break;
		case 100:
			w_window(20,7,40,9,"Notice");
			w_text(39,9,WA_CENTRE,message);
			w_text(39,11,WA_CENTRE,"Press Enter to return");
			//delay(1000000);
			if(w_button(30,13,20,"[Return]"))w_setpage(oldwindow);
		break;
		case 101://run command
			w_window(1,3,47,19,"Run Command");
			w_bigtext(49,4,30,15,hint);
			if(w_biginput(11,5,30,10,"Input Command line",tinput,256))w_setpage(-3);
			if(w_focused())
			hint = "Input commad line in the textbox. <Enter> to run";
			if(w_button(18,17,10,"  [ Return ]  "))w_setpage(oldwindow);
			if(w_focused())
			hint = "<Enter> to return. Up Arrow key to Input command line";
			break;
		case -3://run command
			w_enterconsole();
			run(tinput);
			w_hitanykey();
			w_leaveconsole();
			w_setpage(101);
			break;
		case -4:
			printf("Rebooting.....");
			tgt_reboot();
			break;
		}
		w_present();
	}
	return(0);
}
static const Cmd Cmds[] = {
	{"MainCmds"},
	{"main","",0,"Simulates the MAIN BIOS SETUP",cmd_main, 0, 99, CMD_REPEAT},
	{0, 0}
};
static void init_cmd __P((void)) __attribute__ ((constructor));
static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}