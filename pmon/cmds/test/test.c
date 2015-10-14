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
#define TEST_IR			16384
#define	UPDATE_PMON_NET		(1<<15)
#define	UPDATE_KERNEL_NET	(1<<16)
#define	UPDATE_SYSTEM_NET	(1<<17)	
#define	UPDATE_PMON_USB		(1<<18)
#define	UPDATE_KERNEL_USB	(1<<19)
#define	UPDATE_SYSTEM_USB	(1<<20)	
#define	OTHER_FUNCTON_TEST	(1<<21)	


#include <fb/video_font.h>		//定义了字长字宽
#define INFO_Y  25	//该值要根据菜单的占用的行数修改
#define INFO_W  50
extern void delay1(int);
extern void (*__cprint)(int y, int x,int width,char color, const char *text);

int pause(void)
{
	char cmd[20];
	struct termio tbuf;
	
#ifdef CONFIG_CHINESE
//	__cprint(INFO_Y,0,INFO_W,0x70,"请按""回车""键继续");
	printf("请按\"回车\"键继续\n");
#else
//	__cprint(INFO_Y,0,INFO_W,0x70,"press enter to continue");
	printf("press enter to continue\n");
#endif
//	__cprint(fb_ysize/VIDEO_FONT_HEIGHT - 1, 0, fb_xsize/VIDEO_FONT_WIDTH, 0x70, "press enter to continue");
	ioctl (STDIN, SETNCNE, &tbuf);
	gets(cmd);
	ioctl (STDIN, TCSETAW, &tbuf);
	printf("----------\n");
//	__cprint(INFO_Y,0,INFO_W,0x7,0);
//	__cprint(INFO_Y + 1, 0, INFO_W, 0x7, 0);
	return 0;
}

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
#include "net.c"
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
		{POP_Y+2,POP_X,3,3,TYPE_CMD,	"(2)内存测试(memory test):${?&#mytest 2}=[on=| _or mytest 2||off=| _andn mytest 2]test 2"},
		{POP_Y+3,POP_X,4,4,TYPE_CMD,	"(3)网络测试1(netcard net0):${?&#mytest 4}=[on=| _or mytest 4||off=| _andn mytest 4]test 4"},
		{POP_Y+4,POP_X,5,5,TYPE_CMD,	"(4)网络测试2(netcard net1):${?&#mytest 8}=[on=| _or mytest 8||off=| _andn mytest 8]test 8"},
		{POP_Y+5,POP_X,6,6,TYPE_CMD,	"(5)触摸屏测试(touchscreen test):${?&#mytest 16}=[on=| _or mytest 16||off=| _andn mytest 16]test 16"},
		{POP_Y+6,POP_X,7,7,TYPE_CMD,	"(6)SD卡测试(SD card test):${?&#mytest 32}=[on=| _or mytest 32||off=| _andn mytest 32]test 32"},
		{POP_Y+7,POP_X,8,8,TYPE_CMD,	"(7)显示测试(video test):${?&#mytest 64}=[on=| _or mytest 64||off=| _andn mytest 64]test 64"},
		{POP_Y+8,POP_X,9,9,TYPE_CMD,	"(8)USB测试(USB test):${?&#mytest 128}=[on=| _or mytest 128||off=| _andn mytest 128]test 128"},
		{POP_Y+9,POP_X,10,10,TYPE_CMD,	"(9)按键测试(button test):${?&#mytest 256}=[on=| _or mytest 256||off=| _andn mytest 256]test 256"},
		{POP_Y+10,POP_X,11,11,TYPE_CMD,	"(10)串口测试(serial test):${?&#mytest 512}=[on=| _or mytest 512||off=| _andn mytest 512]test 512"},
		{POP_Y+11,POP_X,12,12,TYPE_CMD,	"(11)AC97音频测试(AC97 test):${?&#mytest 1024}=[on=| _or mytest 1024||off=| _andn mytest 1024]test 1024"},
		{POP_Y+12,POP_X,13,13,TYPE_CMD,	"(12)AD转换测试(MCP3201 AD test):${?&#mytest 2048}=[on=| _or mytest 2048||off=| _andn mytest 2048]test 2048"},
		{POP_Y+13,POP_X,14,14,TYPE_CMD,	"(13)RTC测试(RTC test):${?&#mytest 4096}=[on=| _or mytest 4096||off=| _andn mytest 4096]test 4096"},
		{POP_Y+14,POP_X,15,15,TYPE_CMD,	"(14)NAND闪存测试(NAND Flash test):${?&#mytest 8192}=[on=| _or mytest 8192||off=| _andn mytest 8192]test 8192"},
		{POP_Y+15,POP_X,16,16,TYPE_CMD,	"(15)红外接收测试(Ir test):${?&#mytest 16384}=[on=| _or mytest 16384||off=| _andn mytest 16384]test 16384"},
		{POP_Y+16,POP_X,17,17,TYPE_CMD,	"(16)更新内核(kernel update):${?&#mytest 32768}=[on=| _or mytest 32768||off=| _andn mytest 32768]test 32768"},
		{POP_Y+17,POP_X,18,18,TYPE_CMD,	"(17)更新文件系统(system update):${?&#mytest 65536}=[on=| _or mytest 65536||off=| _andn mytest 65536]test 65536"},
		{POP_Y+18,POP_X,19,19,TYPE_CMD,	"(18)all selected=test ${#mytest}"},
		{POP_Y+19,POP_X,1,1,TYPE_CMD,	"(19)退出=| _quit",0},
		{}
	}
};
/*
static struct setupMenu testmenu1={
	0,POP_W,POP_H,
	(struct setupMenuitem[])
	{
		{POP_Y,POP_X,1,1,TYPE_NONE,		"     开发板测试(board test)"},
		{POP_Y+1,POP_X,2,2,TYPE_CMD,	"(1)CPU测试(CPU test):${?&#mytest 1}                 ${#cputest}=[on=| _or mytest 1||off=| _andn mytest 1]test 1"},
		{POP_Y+2,POP_X,3,3,TYPE_CMD,	"(2)内存测试(memory test):${?&#mytest 2}            ${#memorytest}=[on=| _or mytest 2||off=| _andn mytest 2]test 2"},
		{POP_Y+3,POP_X,4,4,TYPE_CMD,	"(3)网络测试1(netcard net0):${?&#mytest 4}          ${#net0test}=[on=| _or mytest 4||off=| _andn mytest 4]test 4"},
		{POP_Y+4,POP_X,5,5,TYPE_CMD,	"(4)网络测试2(netcard net1):${?&#mytest 8}          ${#net1test}=[on=| _or mytest 8||off=| _andn mytest 8]test 8"},
		{POP_Y+5,POP_X,6,6,TYPE_CMD,	"(5)触摸屏测试(touchscreen test):${?&#mytest 16}     ${#touchscreentest}=[on=| _or mytest 16||off=| _andn mytest 16]test 16"},
		{POP_Y+6,POP_X,7,7,TYPE_CMD,	"(6)SD卡测试(SD card test):${?&#mytest 32}           ${#SDcardtest}=[on=| _or mytest 32||off=| _andn mytest 32]test 32"},
		{POP_Y+7,POP_X,8,8,TYPE_CMD,	"(7)显示测试(video test):${?&#mytest 64}             ${#videotest}=[on=| _or mytest 64||off=| _andn mytest 64]test 64"},
		{POP_Y+8,POP_X,9,9,TYPE_CMD,	"(8)USB测试(USB test):${?&#mytest 128}                ${#USBtest}=[on=| _or mytest 128||off=| _andn mytest 128]test 128"},
		{POP_Y+9,POP_X,10,10,TYPE_CMD,	"(9)按键测试(button test):${?&#mytest 256}            ${#buttontest}=[on=| _or mytest 256||off=| _andn mytest 256]test 256"},
		{POP_Y+10,POP_X,11,11,TYPE_CMD,	"(10)串口测试(serial test):${?&#mytest 512}           ${#UARTtest}=[on=| _or mytest 512||off=| _andn mytest 512]test 512"},
		{POP_Y+11,POP_X,12,12,TYPE_CMD,	"(11)AC97音频测试(AC97 test):${?&#mytest 1024}         ${#AC97test}=[on=| _or mytest 1024||off=| _andn mytest 1024]test 1024"},
		{POP_Y+12,POP_X,13,13,TYPE_CMD,	"(12)AD转换测试(MCP3201 AD test):${?&#mytest 2048}     ${#ADtest}=[on=| _or mytest 2048||off=| _andn mytest 2048]test 2048"},
		{POP_Y+13,POP_X,14,14,TYPE_CMD,	"(13)RTC测试(RTC test):${?&#mytest 4096}                ${#RTCtest}=[on=| _or mytest 4096||off=| _andn mytest 4096]test 4096"},
		{POP_Y+14,POP_X,15,15,TYPE_CMD,	"(14)NAND闪存测试(NAND Flash test):${?&#mytest 8192}    ${#NANDFlashtest}=[on=| _or mytest 8192||off=| _andn mytest 8192]test 8192"},
		{POP_Y+15,POP_X,16,16,TYPE_CMD,	"(15)all selected=test ${#mytest}"},
		{POP_Y+16,POP_X,1,1,TYPE_CMD,	"(16)退出=| _quit",0},
		{}
	}
};
*/
#else
#if 0
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
static struct setupMenu testmenu1={
	0,POP_W,POP_H,
	(struct setupMenuitem[])
	{
		{POP_Y,POP_X,1,1,TYPE_NONE,		"    cloud_board_test (board test)"},
		{POP_Y+1,POP_X,2,2,TYPE_CMD,	"(1)(CPU test):${?&#mytest 1}=[on=| _or mytest 1||off=| _andn mytest 1]test 1"},
		{POP_Y+2,POP_X,3,3,TYPE_CMD,	"(2)(memory test):${?&#mytest 2}=[on=| _or mytest 2||off=| _andn mytest 2]test 2"},
		{POP_Y+3,POP_X,4,4,TYPE_CMD,	"(3)(netcard net0):${?&#mytest 4}=[on=| _or mytest 4||off=| _andn mytest 4]test 4"},
		{POP_Y+4,POP_X,5,5,TYPE_CMD,	"(4)(netcard net1):${?&#mytest 8}=[on=| _or mytest 8||off=| _andn mytest 8]test 8"},
		{POP_Y+5,POP_X,6,6,TYPE_CMD,	"(5)(touchscreen test):${?&#mytest 16}=[on=| _or mytest 16||off=| _andn mytest 16]test 16"},
		{POP_Y+6,POP_X,7,7,TYPE_CMD,	"(6)(SD card test):${?&#mytest 32}=[on=| _or mytest 32||off=| _andn mytest 32]test 32"},
		{POP_Y+7,POP_X,8,8,TYPE_CMD,	"(7)(video test):${?&#mytest 64}=[on=| _or mytest 64||off=| _andn mytest 64]test 64"},
		{POP_Y+8,POP_X,9,9,TYPE_CMD,	"(8)(USB test):${?&#mytest 128}=[on=| _or mytest 128||off=| _andn mytest 128]test 128"},
		{POP_Y+9,POP_X,10,10,TYPE_CMD,	"(9)(button test):${?&#mytest 256}=[on=| _or mytest 256||off=| _andn mytest 256]test 256"},
		{POP_Y+10,POP_X,11,11,TYPE_CMD,	"(10)(serial test):${?&#mytest 512}=[on=| _or mytest 512||off=| _andn mytest 512]test 512"},
		{POP_Y+11,POP_X,12,12,TYPE_CMD,	"(11)(AC97 test):${?&#mytest 1024}=[on=| _or mytest 1024||off=| _andn mytest 1024]test 1024"},
		{POP_Y+12,POP_X,13,13,TYPE_CMD,	"(12)(MCP3201 AD test):${?&#mytest 2048}=[on=| _or mytest 2048||off=| _andn mytest 2048]test 2048"},
		{POP_Y+13,POP_X,14,14,TYPE_CMD,	"(13)(RTC test):${?&#mytest 4096}=[on=| _or mytest 4096||off=| _andn mytest 4096]test 4096"},
		{POP_Y+14,POP_X,15,15,TYPE_CMD,	"(14)(NAND Flash test):${?&#mytest 8192}=[on=| _or mytest 8192||off=| _andn mytest 8192]test 8192"},
		{POP_Y+15,POP_X,16,16,TYPE_CMD,	"(15)(Ir test):${?&#mytest 16384}=[on=| _or mytest 16384||off=| _andn mytest 16384]test 16384"},
		{POP_Y+16,POP_X,17,17,TYPE_CMD,	"(16)(pmon_net update):${?&#mytest 32768}=[on=| _or mytest 32768||off=| _andn mytest 32768]test 32768"},
		{POP_Y+17,POP_X,18,18,TYPE_CMD,	"(17)(kernel_net update):${?&#mytest 65536}=[on=| _or mytest 65536||off=| _andn mytest 65536]test 65536"},
		{POP_Y+18,POP_X,19,19,TYPE_CMD,	"(18)(system_net update):${?&#mytest 131072}=[on=| _or mytest 131072||off=| _andn mytest 131072]test 131072"},
		{POP_Y+19,POP_X,20,20,TYPE_CMD,	"(19)(pmon_usb update):${?&#mytest 262144}=[on=| _or mytest 262144||off=| _andn mytest 262144]test 262144"},
		{POP_Y+20,POP_X,21,21,TYPE_CMD,	"(20)(kernel_usb update):${?&#mytest 524288}=[on=| _or mytest 524288||off=| _andn mytest 524288]test 524288"},
		{POP_Y+21,POP_X,22,22,TYPE_CMD,	"(21)(system_usb update):${?&#mytest 1048576}=[on=| _or mytest 1048576||off=| _andn mytest 1048576]test 1048576"},
		{POP_Y+22,POP_X,23,23,TYPE_CMD,	"(22)(other function test):${?&#mytest 2097152}=[on=| _or mytest 2097152||off=| _andn mytest 2097152]test 2097152"},
		{POP_Y+23,POP_X,24,24,TYPE_CMD,	"(23)all selected=test ${#mytest}"},
		{POP_Y+24,POP_X,1,1,TYPE_CMD,	"(24)quit=| _quit",0},
		{}
	}
};
#endif


static char other_test_menu[] = {
"\n\
************************************************|\n\
[1] PCI Networking test				|\n\
[2] PS2 test					|\n\
[3] CAN1 test					|\n\
[4] acpi test					|\n\
[q] quit					|\n\
please input [1-4] to begin the test		|\n\
input 'q' to black main test list.		|\n\
************************************************|\
\r\n\n\
"
};

void print_main_list(void)
{
	video_set_bg(30, 60, 250);
	printf ("%s", other_test_menu);
}

static int other_fun_test(void)
{
	unsigned char input_char;
	char cmd[200];
	char *clientip2;
	char *serverip;
	int k;

	print_main_list();
	while (1) {
		if (input_char = getchar()) {
			switch (input_char) {
			case '1':
				if(!(clientip2=getenv("clientip2")))
					clientip2="192.168.1.102";
				if(!(serverip=getenv("serverip")))
					serverip="192.168.1.12";
				sprintf(cmd, "ifconfig syn0 remove;ifconfig syn1 remove;\
					ifconfig rtl0 remove; ifconfig rtl0 %s;", clientip2);
				do_cmd(cmd);
				if (myping(serverip)){
				#ifdef CONFIG_CHINESE
					printf("\nPCI网口测试失败：数据包丢失！\n");
//					sprintf(cmd, "set net0test \"%s\"", "测试失败！");
				#else
					printf ("PCI net test error ~_~!!!\n");
				#endif
				}
				else{
				#ifdef CONFIG_CHINESE
					printf("\nPCI网络接口测试通过\n");
				#else
					printf ("PCI net test pass ^_^ !\n");
				#endif
				}
				print_main_list();
				break;
			case '2':
				init_kbd();
				k = kbd_initialize();
				psaux_init();
				kbd_available=1;
				print_main_list();
				break;
			case '3':
				sprintf(cmd, "test_can 1");
				do_cmd(cmd);
				print_main_list();
				break;
			case '4':
				sprintf(cmd, "acpi_test");
				do_cmd(cmd);
				print_main_list();
				break;
			case 'q':
				return 0;
				break;
			default:
				break;
			}
		}
	}
	return 0;
}

static int cmd_test(int ac,char **av)
{
	long tests;
	int i;
	char *serverip;
	char *clientip0;
	char *clientip1;
	char cmd[200];
	int vga_status;

	vga_status = vga_available;

#if 0
	if (vga_status == 1){
		vga_available = 0;
	}
#endif

	vga_available = 1;
	__console_alloc();
	if(ac==1){
		if(getenv("testem")) do_menu(&testmenu);
		else do_menu(&testmenu1);
		if (vga_status == 1){
			vga_available = 1;
		}
		return 0;
	}
	else
		tests=strtoul(av[1],0,0);

	if(!(serverip=getenv("serverip")))
		serverip="192.168.1.12";
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
			#ifdef	LS1ASOC
				ls1a_serialtest();
			#else
				ls1b_serialtest();
			#endif
			break;
			case TEST_AC97:
			#ifdef CONFIG_CHINESE
				printf("AC97音频测试\n说明:\n");
				printf("1.请用耳机和麦克风连接开发板的耳机和麦克风接口\n");
			#else
				printf("Plese plug microphone into the board \n");
			#endif
				pause();
				ac97_test(1, NULL);
			break;
			case TEST_SYN0:
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
				if (myping(serverip)){
				#ifdef CONFIG_CHINESE
					printf("\n网络接口0测试失败：数据包丢失！\n");
//					sprintf(cmd, "set net0test \"%s\"", "测试失败！");
				#else
					printf ("net0 test error ~_~!!!\n");
				#endif
				}
				else{
				#ifdef CONFIG_CHINESE
					printf("\n网络接口0测试通过\n");
				#else
					printf ("net0 test pass ^_^ !\n");
				#endif
//					sprintf(cmd, "set net0test \"%s\"", "测试通过");
				}
//				do_cmd(cmd);
			break;
			case TEST_SYN1:
			#ifdef CONFIG_CHINESE
				printf("网络接口1测试\n说明：\n");
				printf("1.请用网线把开发板的网络接口1和PC机连接起来\n");
				printf("2.同时把PC机的IP地址设置为192.168.1.3\n");
				printf("3.网络接口1测试,需要把JP2插针接上跳线帽\n");
			#else
				printf("Plese plug net wire into syn1\n");
			#endif
				pause();

			#ifdef LS1ASOC
				*((volatile unsigned int*)0xbfd00420) |= 0xc0;	//gmac1 use UART01
			#endif

				sprintf(cmd, "ifconfig syn0 remove;ifconfig syn1 remove;ifconfig syn1 %s;", clientip1);
				do_cmd(cmd);
				if (myping(serverip)){
					printf("\n网络接口1测试失败：数据包丢失！\n");
//					sprintf(cmd, "set net1test \"%s\"", "测试失败！");
				}
				else{
					printf("\n网络接口1测试通过\n");
//					sprintf(cmd, "set net1test \"%s\"", "测试通过");
				}
				do_cmd(cmd);
			#ifdef LS1ASOC
				*((volatile unsigned int*)0xbfd00420) &= ~0xc0; //reset pin to UART01
			#endif
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
			{
				int dev_num;
				sprintf(cmd,"usb info");
				dev_num = do_cmd(cmd);
				sprintf(cmd,"usb tree");
				do_cmd(cmd);
				if (dev_num >= 2){
				#ifdef CONFIG_CHINESE
					printf("USB测试通过\n");
				#else
					printf ("usb test pass ^_^ !\n");
				#endif
				}else{
				#ifdef CONFIG_CHINESE
					printf("USB集线器(HUB)错误！\n");
				#else
					printf ("usb test error ~_~!!!! \n");
				#endif
				}
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
				int ret;
			#ifdef CONFIG_CHINESE
				printf("NAND Flash 测试\n");
			#else
				printf ("NAND Flash test begin :\n");
			#endif
				ret = ls1x_nand_test();
				if (ret == 0) {
				#ifdef CONFIG_CHINESE
					printf("NAND Flash 测试通过\n");
				#else
					printf ("Nandflash test pass ^_^ !!!!!\n");
				#endif
				} else {
				#ifdef CONFIG_CHINESE
					printf("NAND Flash 读写错误！\n");
				#else
					printf ("Nandflash read-write error ~_~!!!\n");
				#endif
				}
				printf("\n");
			}
			break;
			case TEST_IR:
				sprintf(cmd, "test_Ir");
				do_cmd(cmd);
			break;
			case UPDATE_PMON_NET:
				printf ("please set the host-PC's IP to 192.168.1.12 !\n");
				do_cmd("load -r -f bfc00000 tftp://192.168.1.12/gzrom-cloud.bin");
				break;

			case UPDATE_KERNEL_NET:
				printf ("please set the host-PC's IP to 192.168.1.12 !\n");
				do_cmd("devcp tftp://192.168.1.12/vmlinuz-cloud /dev/mtd0");
				setenv("al", "/dev/mtd0");
				setenv("append", "console=ttyS2,115200 root=/dev/mtdblock1 rw rootfstype=yaffs2 init=/sbin/init video=ls1bfb:vga1024x768-24@60 quiet");
//				setenv("append", "console=ttyS2,115200 root=/dev/mtdblock1 rw rootfstype=yaffs2 init=/sbin/init video=ls1bfb:vga1024x768-24@60");
			break;

			case UPDATE_SYSTEM_NET:
				printf ("please set the host-PC's IP to 192.168.1.12 !\n");
				do_cmd("mtd_erase /dev/mtd1");
				do_cmd("devcp tftp://192.168.1.12/cloud.img /dev/mtd1 yaf nw");
			break;

			case UPDATE_PMON_USB:
				printf ("please plug in the USB-disk, file name is \'gzrom-cloud.bin\' !\n");
				do_cmd ("load -r -f bfc00000 /dev/fat@usb0/gzrom-cloud.bin");
				break;

			case UPDATE_KERNEL_USB:
				printf ("please plug in the USB-disk, file name is \'vmlinuz-cloud\' !\n");
				do_cmd ("devcp /dev/fat@usb0/vmlinuz-cloud /dev/mtd0");
				setenv("al", "/dev/mtd0");
				setenv("append", "console=ttyS2,115200 root=/dev/mtdblock1 rw rootfstype=yaffs2 init=/sbin/init video=ls1bfb:vga1024x768-24@60 quiet");
//				setenv("append", "console=ttyS2,115200 root=/dev/mtdblock1 rw rootfstype=yaffs2 init=/sbin/init video=ls1bfb:vga1024x768-24@60");
				break;

			case UPDATE_SYSTEM_USB:
				printf ("please plug in the USB-disk, file name is \'cloud.img\' !\n");
				do_cmd("mtd_erase /dev/mtd1");
				do_cmd ("devcp /dev/fat@usb0/cloud.img /dev/mtd1 yaf nw");
				break;
			case OTHER_FUNCTON_TEST:
			#ifdef	LS1ASOC
				other_fun_test();
			#endif
				break;
		}
		pause();
	}
	if (vga_status == 1){
		vga_available = 1;
	}
	return 0;
}


//-------------------------------------------------------------------------------------------
static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"test","val",0,"hardware test",cmd_test,0,99,CMD_REPEAT},
#if NMOD_VGACON
	{"LCD0","", 0, "LCD0", lcd0, 0, 99, CMD_REPEAT},
	{"LCD1","", 0, "LCD1", lcd1, 0, 99, CMD_REPEAT},
#endif
//	{"serial","val",0,"hardware test",cmd_serial,0,99,CMD_REPEAT},
	{0,0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));
static void init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
