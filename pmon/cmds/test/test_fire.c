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
#define TEST_MEM 1
#define TEST_NET 2
#define TEST_DISK 4
#define TEST_SERIAL 8
#define TEST_PPPORT 16
#define TEST_PCI 32

#define INFO_Y  24
#define INFO_W  80
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

static int printticks(int n,int d)
{
int i;
char c[4]={'\\','|','/','-'};
for(i=0;i<n;i++)
{
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
#include "kbd.c"
#include "video.c"
#endif
#include "pci.c"
#include "pp.c"
#include "fd.c"
#include "net.c"

#include "../setup.h"


struct setupMenu testmenu={
	0,POP_W,POP_H,
	(struct setupMenuitem[])
	{
		{POP_Y,POP_X,1,1,TYPE_NONE,"    board test"},
		{POP_Y+1,POP_X,2,2,TYPE_CMD,"(1)memory:${?&#mytest 1}=[on=| _or mytest 1||off=| _andn mytest 1]test 1"},
		{POP_Y+2,POP_X,3,3,TYPE_CMD,"(2)netcard :${?&#mytest 2}=[on=| _or mytest 2||off=| _andn mytest 2]test 2"},
		{POP_Y+3,POP_X,4,4,TYPE_CMD,"(3)disk:${?&#mytest 4}=[on=| _or mytest 4||off=| _andn mytest 4]test 4"},
		{POP_Y+4,POP_X,5,5,TYPE_CMD,"(4)serial:${?&#mytest 8}=[on=| _or mytest 8||off=| _andn mytest 8]test 8"},
		{POP_Y+5,POP_X,6,6,TYPE_CMD,"(5)parallel:${?&#mytest 16}=[on=| _or mytest 16||off=| _andn mytest 16]test 16"},
		{POP_Y+6,POP_X,7,7,TYPE_CMD,"(6)all selected=test ${#mytest}"},
		{POP_Y+7,POP_X,1,1,TYPE_CMD,"(7)quit=| _quit",0},
		{}
	}
};


static int cmd_test(int ac,char **av)
{
	long tests;
	int i;
	char cmd[200];

	__console_alloc();
	if(ac==1){
		do_menu(&testmenu);
		return 0;
	}
	else
		tests=strtoul(av[1],0,0);

	for(i=0;i<31;i++){
		if(!(tests&(1<<i)))continue;
		switch(1<<i)
		{
			case TEST_MEM:
				strcpy(cmd,"mt -v 0x88000000 0x89000000");
				do_cmd(cmd);
			break;
			case TEST_SERIAL:
				serial_selftest(10);
				serial_selftest(11);
			break;
			case TEST_PPPORT:
				pptest();
			break;
			case TEST_NET:
			//net_looptest();
			break;
			case TEST_DISK:
			//disktest();
			break;
		}
		if(ac==2)	pause();
	}
	return 0;
}

static char test_menu[] = {
"[1] memory test\n\
[2] video test\n\
[3] serial test\n\
[4] net test\n\
[5] ac97 test\n\
[6] button test\n\
[7] touchscreen test\n\
[8] usb test\n\
[9] SD card test\n\
[a] MCP3201 AD test\n\
[b] RTC test\n\
[c] NAND Flash test\n\
[q] quit\n\
please input [1-8] to begin the test\n\
input 'q' to black PMON command line.\n\
"
};

static int test_dev(int ac, char **av)
{
	char cmd[200];
	char *serverip;
	char *clientip;
	char input_char;
	
	printf("%s", test_menu);
	while(1)
	{
		input_char = get_uart_char(0);
		switch(input_char)
		{
			case '1':
				printf("/--------------------memory test------------------/\n");
				strcpy(cmd, "mt -v");
				do_cmd(cmd);
				printf("/------------------memory test done---------------/\n");
				printf("\n%s", test_menu);
			break;
			case '2':
				printf("/--------------------video test-------------------/\n");
				__console_alloc();
				videotest();
				printf("/------------------video test done----------------/\n");
				printf("\n%s", test_menu);
			break;
			case '3':
				printf("/--------------------serial test------------------/\n");
				serial_selftest(4);
				serial_selftest(8);
				serial_selftest(9);
				serial_selftest(10);
				serial_selftest(11);
				printf("/------------------serial test done---------------/\n");
				printf("\n%s", test_menu);
			break;
			case '4':
				printf("/--------------------net test---------------------/\n");
				if(!(serverip=getenv("serverip")))
					serverip="192.168.1.3";
				if(!(clientip=getenv("clientip")))
					clientip="192.168.1.100";
				//sprintf(cmd,"ifconfig syn0 remove;ifconfig syn1 remove;ifconfig syn0 %s",clientip);
				//do_cmd(cmd);
				//printf("Plese plug net wire into syn0\n");
				//pause();
				//while(get_uart_char(0) == 0);
				sprintf(cmd,"ping -c 5 %s",serverip);
				do_cmd(cmd);
				printf("/------------------net test done------------------/\n");
				printf("\n%s", test_menu);
			break;
			case '5':
				printf("/--------------------ac97 test--------------------/\n");
				ac97_read();
				ac97_test();
				printf("/------------------ac97 test done-----------------/\n");
				printf("\n%s", test_menu);
			break;
			case '6':
				printf("/--------------------button test------------------/\n");
				button_test();
				printf("/------------------button test done---------------/\n");
				printf("\n%s", test_menu);
			break;
			case '7':
				printf("/--------------------touchscreen test-------------/\n");
				ads7846_test();
				printf("/------------------touchscreen test done----------/\n");
				printf("\n%s", test_menu);
			break;
			case '8':
				printf("/--------------------usb test---------------------/\n");
				sprintf(cmd,"usb info");
				do_cmd(cmd);
				printf("/------------------usb test done------------------/\n");
				printf("\n%s", test_menu);
			break;
			case '9':
				printf("/--------------------SD card test-----------------/\n");
				sprintf(cmd,"test_sdcard");
				do_cmd(cmd);
				printf("/------------------SD card test done--------------/\n");
				printf("\n%s", test_menu);
			break;
			case 'a':
				printf("/------------------MCP3201 AD test----------------/\n");
				sprintf(cmd,"test_MCP3201");
				do_cmd(cmd);
				printf("/----------------MCP3201 AD test done-------------/\n");
				printf("\n%s", test_menu);
			break;
			case 'b':
				printf("/-------------------- RTC test -------------------/\n");
				sprintf(cmd,"test_rtc");
				do_cmd(cmd);
				printf("/------------------ RTC test done ----------------/\n");
				printf("\n%s", test_menu);
			break;
			case 'c':
			{
				int i, ret;
				unsigned char *temp = (unsigned char *)0xa1000000;
				printf("/----------------- NANA Flash test ---------------/\n");
				sprintf(cmd, "nandreadid");	//读取ID
				ret = do_cmd(cmd);
				if (ret == 0){
					sprintf(cmd, "erase_nand 0 0x1");	//擦除第0块 block
					do_cmd(cmd);
					sprintf(cmd, "read_nand 0xa1000000  0x0  0x800 m");	//读第0页内容
					do_cmd(cmd);
					//打印读取的内容
					for (i=0; i<256; i++){
						if((i%16) == 0)
							printf("\n");
					//	printf("%x ", *(unsigned int *)(ram+(i*4)));
						printf("%02x ", *(temp+i));
					}
					printf("\n");
					//修改
					for (i = 0; i < 0x800; i += 1){
						*(temp + i) = i;
					}
					sprintf(cmd, "write_nand 0xa1000000  0x0  0x800 m");	//写第0页内容
					do_cmd(cmd);
					sprintf(cmd, "read_nand 0xa1000000  0x0  0x800 m");	//读第0页内容
					do_cmd(cmd);
					for (i=0; i<256; i++){
						if((i%16) == 0)
							printf("\n");
					//	printf("%x ", *(unsigned int *)(ram+(i*4)));
						printf("%02x ", *(temp+i));
					}
					printf("\n");
				}
				printf("/--------------- NAND Flash test done -------------/\n");
				printf("\n%s", test_menu);
			}
			break;
			case 'q':
				return 0;
			break;
			default:
			break;
		}
	}

	return 0;
}

//-------------------------------------------------------------------------------------------
static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"test","val",0,"hardware test",cmd_test,0,99,CMD_REPEAT},
//	{"functest","",0,"function test",cmd_functest,0,99,CMD_REPEAT},
	{"testdev","",0,"hardware test",test_dev,0,99,CMD_REPEAT},
//	{"serial","val",0,"hardware test",cmd_serial,0,99,CMD_REPEAT},
	{0,0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));
static void init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
