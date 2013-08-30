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
#define TEST_CPU		1
#define TEST_MEM		2
#define TEST_SYN0		4
#define TEST_SYN1		8
#define TEST_TS		16
#define TEST_SDCARD	32
#define TEST_VIDEO	64
#define TEST_USB		128
#define TEST_KBD		256
#define TEST_SERIAL	512
#define TEST_AC97		1024
#define TEST_AD		2048
#define TEST_RTC		4096
#define TEST_NAND		8192

#include <fb/video_font.h>		//定义了字长字宽
#define INFO_Y  19	//该值要根据菜单的占用的行数修改
#define INFO_W  50
extern void delay1(int);
extern void (*__cprint)(int y, int x, int width, char color, const char *text);
extern int pause(void);

static int printticks(int n,int d)
{
	int i;
	char c[4]={'\\','|','/','-'};
	
	for(i=0;i<n;i++){
		printf("%c",c[i%4]);
		delay1(d);	
		printf("\b");
	}
	return 0;
}

#include "cpu.c"
#include "mem.c"
//#include "serial.c"
//#include "hd.c"
#if NMOD_VGACON
//#include "kbd.c"
#include "video.c"
#endif
//#include "pci.c"
//#include "pp.c"
//#include "fd.c"

#include "../setup.h"

extern int ls1b_serialtest(void);
extern int ac97_test(int argc, char **argv);
extern void ads7846_test(void);
extern void button_test(void);

/*
struct setupMenu nextmenu={
	0,10,3,
	(struct setupMenuitem[])
	{
		{20,60,1,1,TYPE_CMD,"GPIO Setup",0,10},
		{21,60,2,2,TYPE_CMD,"(1)uart1"},
		{22,60,0,0,TYPE_CMD,"(2)uart2"},
		{}
	}
};
*/

static struct setupMenu videomenu={
	0,POP_W,POP_H,
	(struct setupMenuitem [])
	{
		{POP_Y,POP_X,1,1,TYPE_NONE,"    显示测试"},
		{POP_Y+1,POP_X,2,2,TYPE_CMD,"(1)uart1:${!gpio uart1}=[232=| _gpio 1 ||422=| _gpio 2||485=| _gpio 3||none=| _gpio 4]"},
		{POP_Y+2,POP_X,3,3,TYPE_CMD,"(2)uart2:${!gpio uart2}=[232=| _gpio 1 ||422=| _gpio 2||485=| _gpio 3||none=| _gpio 4]"},
		{POP_Y+3,POP_X,4,4,TYPE_CMD,"(3)lan1:${!gpio lan1}=[on=| _gpio 1||off=| _gpio 2]"},
		{POP_Y+4,POP_X,5,5,TYPE_CMD,"(4)lan2A:${!gpio lan2A}=[on=| _gpio 1||off=| _gpio 2]"},
		{POP_Y+5,POP_X,6,6,TYPE_CMD,"(5)lan2B:${!gpio lan2B}=[on=| _gpio 1||off=| _gpio 2]"},
//		{POP_Y+6,POP_X,7,7,TYPE_CMD,"(6)save=set gpiocfg ${*0x%08x}",0,&gpiodata},
		{POP_Y+7,POP_X,1,1,TYPE_NONE,"(7)quit",setup_quit},
		{}
	}
};

static struct setupMenu testmenu={
	0,POP_W,POP_H,
	(struct setupMenuitem[])
	{
		{POP_Y,POP_X,1,1,TYPE_NONE,"    board test"},
		{POP_Y+1,POP_X,2,2,TYPE_CMD,"(1)cpu:${?&#mytest 1}=[on=| _or mytest 1||off=| _andn mytest 1]test 1"},
		{POP_Y+2,POP_X,3,3,TYPE_CMD,"(2)memory:${?&#mytest 2}=[on=| _or mytest 2||off=| _andn mytest 2]test 2"},
		{POP_Y+3,POP_X,4,4,TYPE_CMD,"(3)netcard net0:${?&#mytest 4}=[on=| _or mytest 4||off=| _andn mytest 4]test 4"},
		{POP_Y+4,POP_X,5,5,TYPE_CMD,"(4)netcard net1:${?&#mytest 8}=[on=| _or mytest 8||off=| _andn mytest 8]test 8"},
		{POP_Y+5,POP_X,6,6,TYPE_CMD,"(5)netcard net2:${?&#mytest 16}=[on=| _or mytest 16||off=| _andn mytest 16]test 16"},
		{POP_Y+6,POP_X,7,7,TYPE_CMD,"(5)pci:${?&#mytest 32}=[on=| _or mytest 32||off=| _andn mytest 32]test 32"},
		{POP_Y+7,POP_X,8,8,TYPE_CMD,"(6)video:${?&#mytest 64}=[on=| _or mytest 64||off=| _andn mytest 64]test 64"},
		{POP_Y+8,POP_X,9,9,TYPE_CMD,"(7)harddisk:${?&#mytest 128}=[on=| _or mytest 128||off=| _andn mytest 128]test 128"},
		{POP_Y+9,POP_X,10,10,TYPE_CMD,"(8)keyboard:${?&#mytest 256}=[on=| _or mytest 256||off=| _andn mytest 256]test 256"},
		{POP_Y+10,POP_X,11,11,TYPE_CMD,"(9)serial:${?&#mytest 512}=[on=| _or mytest 512||off=| _andn mytest 512]test 512"},
		{POP_Y+11,POP_X,12,12,TYPE_CMD,"(10)parallel:${?&#mytest 1024}=[on=| _or mytest 1024||off=| _andn mytest 1024]test 1024"},
		{POP_Y+12,POP_X,13,13,TYPE_CMD,"(11)all selected=test ${#mytest}"},
		{POP_Y+13,POP_X,1,1,TYPE_CMD,"(12)quit=| _quit",0},
		{}
	}
};

#ifdef CONFIG_CHINESE
static struct setupMenu testmenu1={
	0,POP_W,POP_H,
	(struct setupMenuitem[])
	{
		{POP_Y,POP_X,1,1,TYPE_NONE,		"     开发板测试(board test)"},
		{POP_Y+1,POP_X,2,2,TYPE_CMD,	"(1)CPU测试(CPU test):${?&#mytest 1}=[on=| _or mytest 1||off=| _andn mytest 1]test 1"},
		{POP_Y+2,POP_X,3,3,TYPE_CMD,	"(2)内存测试(memory tets):${?&#mytest 2}=[on=| _or mytest 2||off=| _andn mytest 2]test 2"},
		{POP_Y+3,POP_X,4,4,TYPE_CMD,	"(3)网络测试1(netcard net0):${?&#mytest 4}=[on=| _or mytest 4||off=| _andn mytest 4]test 4"},
		{POP_Y+4,POP_X,5,5,TYPE_CMD,	"(4)网络测试2(netcard net1):${?&#mytest 8}=[on=| _or mytest 8||off=| _andn mytest 8]test 8"},
		{POP_Y+5,POP_X,6,6,TYPE_CMD,	"(5)触摸屏测试(touchscreen test):${?&#mytest 16}=[on=| _or mytest 16||off=| _andn mytest 16]test 16"},
		{POP_Y+6,POP_X,7,7,TYPE_CMD,	"(6)SD卡测试(SD card test):${?&#mytest 32}=[on=| _or mytest 32||off=| _andn mytest 32]test 32"},
		{POP_Y+7,POP_X,8,8,TYPE_CMD,	"(7)显示测试(video test):${?&#mytest 64}=[on=| _or mytest 64||off=| _andn mytest 64]test 64"},
//		{POP_Y+5,POP_X,6,6,TYPE_MENU,	"(5)显示测试(video test) >", 0, &videomenu},
		{POP_Y+8,POP_X,9,9,TYPE_CMD,	"(8)USB测试(USB test):${?&#mytest 128}=[on=| _or mytest 128||off=| _andn mytest 128]test 128"},
		{POP_Y+9,POP_X,10,10,TYPE_CMD,	"(9)按键和蜂鸣器测试(button and buzzer test):${?&#mytest 256}=[on=| _or mytest 256||off=| _andn mytest 256]test 256"},
		{POP_Y+10,POP_X,11,11,TYPE_CMD,	"(10)串口测试(serial test):${?&#mytest 512}=[on=| _or mytest 512||off=| _andn mytest 512]test 512"},
		{POP_Y+11,POP_X,12,12,TYPE_CMD,	"(11)AC97音频测试(AC97 test):${?&#mytest 1024}=[on=| _or mytest 1024||off=| _andn mytest 1024]test 1024"},
		{POP_Y+12,POP_X,13,13,TYPE_CMD,	"(12)AD转换测试(MCP3201 AD test):${?&#mytest 2048}=[on=| _or mytest 2048||off=| _andn mytest 2048]test 2048"},
		{POP_Y+13,POP_X,14,14,TYPE_CMD,	"(13)RTC测试(RTC test):${?&#mytest 4096}=[on=| _or mytest 4096||off=| _andn mytest 4096]test 4096"},
		{POP_Y+14,POP_X,15,15,TYPE_CMD,	"(14)NAND闪存测试(NAND Flash test):${?&#mytest 8192}=[on=| _or mytest 8192||off=| _andn mytest 8192]test 8192"},
		{POP_Y+15,POP_X,16,16,TYPE_CMD,	"(15)all selected=test ${#mytest}"},
		{POP_Y+16,POP_X,1,1,TYPE_CMD,	"(16)退出=| _quit",0},
		{}
	}
};
#else
static struct setupMenu testmenu1={
	0,POP_W,POP_H,
	(struct setupMenuitem[])
	{
		{POP_Y,POP_X,1,1,TYPE_NONE,"    board test"},
		{POP_Y+1,POP_X,2,2,TYPE_CMD,"(1)cpu:${?&#mytest 1}=[on=| _or mytest 1||off=| _andn mytest 1]test 1"},
		{POP_Y+2,POP_X,3,3,TYPE_CMD,"(2)memory:${?&#mytest 2}=[on=| _or mytest 2||off=| _andn mytest 2]test 2"},
		{POP_Y+3,POP_X,4,4,TYPE_CMD,"(3)netcard net0:${?&#mytest 4}=[on=| _or mytest 4||off=| _andn mytest 4]test 4"},
		{POP_Y+4,POP_X,5,5,TYPE_CMD,"(4)pci:${?&#mytest 32}=[on=| _or mytest 32||off=| _andn mytest 32]test 32"},
		{POP_Y+5,POP_X,6,6,TYPE_CMD,"(5)video:${?&#mytest 64}=[on=| _or mytest 64||off=| _andn mytest 64]test 64"},
		{POP_Y+6,POP_X,7,7,TYPE_CMD,"(6)harddisk:${?&#mytest 128}=[on=| _or mytest 128||off=| _andn mytest 128]test 128"},
		{POP_Y+7,POP_X,8,8,TYPE_CMD,"(7)keyboard:${?&#mytest 256}=[on=| _or mytest 256||off=| _andn mytest 256]test 256"},
		{POP_Y+8,POP_X,9,9,TYPE_CMD,"(8)serial:${?&#mytest 512}=[on=| _or mytest 512||off=| _andn mytest 512]test 512"},
		{POP_Y+9,POP_X,10,10,TYPE_CMD,"(9)parallel:${?&#mytest 1024}=[on=| _or mytest 1024||off=| _andn mytest 1024]test 1024"},
		{POP_Y+10,POP_X,11,11,TYPE_CMD,"(10)all selected=test ${#mytest}"},
		{POP_Y+11,POP_X,1,1,TYPE_CMD,"(11)quit=| _quit",0},
		{}
	}
};
#endif

static int cmd_test_vga(int ac,char **av)
{
	long tests;
	int i;
	char *serverip;
	char *clientip0;
	char *clientip1;
	char cmd[200];
	int vga_status;

	vga_status = vga_available;
	if (vga_status == 0){
		vga_available = 1;
	}
	__console_alloc();
	if(ac==1){
		if(getenv("testem")) do_menu(&testmenu);
		else do_menu(&testmenu1);
		if (vga_status == 0){
			vga_available = 0;
		}
		return 0;
	}
	else
		tests=strtoul(av[1],0,0);

	if(!(serverip=getenv("serverip")))
		serverip="192.168.1.3";
	if(!(clientip0=getenv("clientip0")))
		clientip0="192.168.1.100";
	if(!(clientip1=getenv("clientip1")))
		clientip1="192.168.1.101";

	for(i=0;i<31;i++){
		if(!(tests&(1<<i)))continue;
		switch(1<<i)
		{
			case TEST_CPU:
				cputest();
			break;
			case TEST_MEM:
				memtest();
			break;
			case TEST_SERIAL:
				ls1b_serialtest();
			break;
			case TEST_AC97:
			#ifdef CONFIG_CHINESE
				printf("AC97音频测试\n说明:\n");
				printf("1.请用耳机和麦克风连接开发板的耳机和麦克风接口\n");
			#else
				printf("Plese plug net wire into syn0\n");
			#endif
				pause();
				ac97_test(1, NULL);
			break;
			case TEST_SYN0:
			//	sprintf(cmd,"ifconfig em0 remove;ifconfig em1 remove;ifconfig fxp0 remove;ifconfig fxp0 %s;",clientip);
			//	do_cmd(cmd);
			#ifdef CONFIG_CHINESE
				printf("网络接口0测试\n说明：\n");
				printf("1.请用网线把开发板的网络接口0和PC机连接起来\n");
				printf("2.同时把PC机的IP地址设置为192.168.1.3\n");
			#else
				printf("Plese plug net wire into syn0\n");
			#endif
				pause();
				sprintf(cmd, "ifconfig syn0 remove;ifconfig syn1 remove;ifconfig syn0 %s;", clientip0);
				do_cmd(cmd);
				sprintf(cmd,"ping -c 3 %s",serverip);
				do_cmd(cmd);
			break;
			case TEST_SYN1:
			//	sprintf(cmd,"ifconfig em0 remove;ifconfig em1 remove;ifconfig fxp0 remove;ifconfig em0 %s",clientip);
			//	do_cmd(cmd);
			#ifdef CONFIG_CHINESE
				printf("网络接口1测试\n说明：\n");
				printf("1.请用网线把开发板的网络接口1和PC机连接起来\n");
				printf("2.同时把PC机的IP地址设置为192.168.1.3\n");
			#else
				printf("Plese plug net wire into syn1\n");
			#endif
				pause();
				sprintf(cmd, "ifconfig syn0 remove;ifconfig syn1 remove;ifconfig syn1 %s;", clientip1);
				do_cmd(cmd);
				sprintf(cmd,"ping -c 3 %s",serverip);
				do_cmd(cmd);
			break;
			case TEST_TS:
				ads7846_test();
			break;
		#if NMOD_VGACON
			case TEST_VIDEO:
				videotest();
			break;
		#endif
			case TEST_USB:
			#ifdef CONFIG_CHINESE
				printf("USB 设备信息：\n");
			#else
				printf("USB devices information:\n");
			#endif
				sprintf(cmd,"usb info");
				if (do_cmd(cmd) >= 2){
					printf("USB测试通过\n");
				}else{
					printf("USB集线器(HUB)错误！\n");
				}
			break;
		#if NMOD_VGACON
			case TEST_KBD:
				button_test();
			break;
		#endif
			case TEST_SDCARD:
				printf("SD卡测试\n");
				sprintf(cmd,"test_sdcard");
				do_cmd(cmd);
			break;
			case TEST_AD:
				sprintf(cmd,"test_MCP3201");
				do_cmd(cmd);
			break;
			case TEST_RTC:
				sprintf(cmd,"test_rtc");
				do_cmd(cmd);
			break;
			case TEST_NAND:
			{
				int i, cont, ret;
				unsigned char *temp1 = (unsigned char *)0xa1000000;
				unsigned char *temp2 = (unsigned char *)0xa2000000;
				printf("NAND Flash 测试\n");
//				printf("/----------------- NAND Flash test ---------------/\n");
				sprintf(cmd, "nandreadid");	//读取ID
				ret = do_cmd(cmd);
				if (ret == 0){
					sprintf(cmd, "erase_nand 0 0x1");	//擦除第0块 block
					do_cmd(cmd);
					sprintf(cmd, "read_nand 0xa1000000  0x0  0x800 m");	//读第0页内容
					do_cmd(cmd);
					//打印读取的内容
					for (i=0, cont=0; i<0x800; i++){
						if(*(temp1+i) != 0xff){
							printf("NAND Flash 擦除错误！\n");
							cont++;
						}
//						if((i%16) == 0)
//							printf("\n");
//						printf("%02x ", *(temp1+i));
					}
//					printf("\n");
					//修改
					for (i=0; i<0x800; i++){
						*(temp1 + i) = i;
					}
					sprintf(cmd, "write_nand 0xa1000000  0x0  0x800 m");	//写第0页内容
					do_cmd(cmd);
					sprintf(cmd, "read_nand 0xa2000000  0x0  0x800 m");	//读第0页内容
					do_cmd(cmd);
					for (i=0, cont=0; i<0x800; i++){
						if (*(temp1 + i) != *(temp2 + i)){
							printf("NAND Flash 读写错误！\n");
							cont++;
						}
//						if((i%16) == 0)
//							printf("\n");
//						printf("%02x ", *(temp2+i));
					}
					if (cont == 0){
						printf("NAND Flash 测试通过\n");
					}
					printf("\n");
				}
//				printf("/--------------- NAND Flash test done -------------/\n");
			}
			break;
		}
		pause();
	}
	if (vga_status == 0){
		vga_available = 0;
	}
	return 0;
}

//-------------------------------------------------------------------------------------------
static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"test_vga","val",0,"hardware test",cmd_test_vga,0,99,CMD_REPEAT},
//	{"serial","val",0,"hardware test",cmd_serial,0,99,CMD_REPEAT},
	{0,0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));
static void init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
