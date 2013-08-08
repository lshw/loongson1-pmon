#include <pmon.h>
#include <cpu.h>
#include <string.h>
#include <machine/pio.h>
#include <time.h>
#include "target/fcr.h"

//#define	CAN0_BASE	0xbfe50000
//#define	CAN1_BASE	0xbfe54000

int test_can(int argc,char **argv)
{
	unsigned int i;
	unsigned int CAN0_BASE;
	unsigned int reg_val;
	unsigned char rev_buf[8];
	i = strtoul(argv[1], 0, 0);

	printf ("begin to test can %d !\n", i);
	if (i == 0) {
		CAN0_BASE = 0xbfe50000;		//can0's base address
	}
	else {
		CAN0_BASE = 0xbfe54000;		//can1's base address
	#ifdef	LS1ASOC
		reg_val = *(volatile unsigned int*)0xbfd00420;
		*(volatile unsigned int*)0xbfd00420 &= ~((1<<12) | (1<<13) | (1<<16));
	#endif
	}

	//reset mode
	outb(CAN0_BASE,1);

	//externmode	
	outb(CAN0_BASE+1,0x84);
	//outb(CAN0_BASE+1,0x80);

	//set timing
	outb(CAN0_BASE+6,3+(1<<6)); 	//gc3+(1<<6)   43
	outb(CAN0_BASE+7,0x2f);  	//gc2f  0xf+(0x2<<4)+(0<<7)
	outb(CAN0_BASE+5,0xff);  	//gcff  mask == ff

	outb(CAN0_BASE+16,0x0);
	outb(CAN0_BASE+17,0x0);   	//设置验收帧
	outb(CAN0_BASE+18,0x0);
	outb(CAN0_BASE+19,0x0);
	outb(CAN0_BASE+20,0xff);
	outb(CAN0_BASE+21,0xff);
	outb(CAN0_BASE+22,0xff);
	outb(CAN0_BASE+23,0xff);

//	outb(CAN0_BASE+4,0xff);  	//gc 所有中断 使能

	//work mode
	//outb(CAN0_BASE,0x8); 		//gc04正常工作模式
	outb(CAN0_BASE,0xc); 		//gc04正常工作模式

	//set data
	outb(CAN0_BASE+16,0x88);  	//gcID 
	outb(CAN0_BASE+17,0x12);
	outb(CAN0_BASE+18,0x34);
	outb(CAN0_BASE+19,0x45);
	outb(CAN0_BASE+20,0x56);   	//gc接收时，变0x50

	outb(CAN0_BASE+21,0x1);
	outb(CAN0_BASE+22,0x2);
	outb(CAN0_BASE+23,0x3);
	outb(CAN0_BASE+24,0x4);
	outb(CAN0_BASE+25,0x5);
	outb(CAN0_BASE+26,0x6);
	outb(CAN0_BASE+27,0x7);
	outb(CAN0_BASE+28,0x8);

	//outb(CAN0_BASE+1,0x90);  	//self request ok now
	//outb(CAN0_BASE+1,0x81);  	//gctx_requeste
//	printf ("before to send data ......\n");
//	do_cmd("d4 0x%x 24\n",CAN0_BASE);
	outb(CAN0_BASE+1,0x92);  	//gctx_requeste
	delay(1000);
//	printf("after can bus to self tx and rx......\n");
//	do_cmd("d4 0x%x 24\n",CAN0_BASE);

	for (i = sizeof(rev_buf); i>0; i--) {
		rev_buf[i - 1] = inb(CAN0_BASE + 20 + i);
	}
	
	for (i = 0; i < sizeof(rev_buf); i++) {
		if (rev_buf[i] != (i + 1)) {
			outb(CAN0_BASE+1,0x84); 	//release buffer
			printf ("can selftest error ~_~ !!!\n");
			return 0;
		}
	}

	outb(CAN0_BASE+1,0x84); 	//release buffer
	printf ("can selftest succesful ^_^ !!!\n");

#ifdef	LS1ASOC
	*(volatile unsigned int*)0xbfd00420 = reg_val;
#endif

	return 0;
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"test_can","val", 0, "test can: 0|can0, 1|can1", test_can, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

