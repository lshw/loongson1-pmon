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

#define	ADC_REF			3300
#define	ADC_X_LEN		800
#define	ADC_Y_LEN		480
#define	NOR_MODE			0x1			//normal adc mode
#define	TS_MODE			0x2			//touch screen mode
#define	ADC_SINGLE_MODE	0x1		//adc single test mode.
#define	ADC_CONT_MODE		0x2		//adc continue test mode.
#define	ADC_DMA_DESC_ADDR	0xa0810000
#define	ADC_DMA_RES_ADDR	0xa0820000
#define	RES_LEN			0x4

#define	ADC_S_BUSY		(1 << 31)
#define	ADC_S_SOC		(1 << 4)
#define	ADC_CC_EN		(1 << 7)
#define	ADC_TOUCH_SEL	(1 << 6)
#define	ADC_TOUCH_5		(1 << 5)
#define	ADC_INT_RELEASE	(1 << 1)
#define	ADC_INT_PRESS	(1 << 0)

#define	DMA_BASE		0xbfd01160
#define	ADC_BASE		0xbfe74000
#define	ADC_DMA_DADDR	(ADC_BASE + 0x28)
#define	ADC_CNT			*(volatile unsigned int *)(ADC_BASE + 0x0)
#define	ADC_S_CTL		*(volatile unsigned int *)(ADC_BASE + 0x4)
#define	ADC_C_CTL		*(volatile unsigned int *)(ADC_BASE + 0x8)
#define	ADC_X_RANGE		*(volatile unsigned int *)(ADC_BASE + 0x10)
#define	ADC_Y_RANGE		*(volatile unsigned int *)(ADC_BASE + 0x14)
#define	ADC_WAT_RANGE	*(volatile unsigned int *)(ADC_BASE + 0x18)
#define	ADC_AXIS			*(volatile unsigned int *)(ADC_BASE + 0x1c)
#define	ADC_S_DOUT0		*(volatile unsigned int *)(ADC_BASE + 0x20)
#define	ADC_S_DOUT1		*(volatile unsigned int *)(ADC_BASE + 0x24)
#define	ADC_C_DOUT		*(volatile unsigned int *)(ADC_BASE + 0x28)
#define	ADC_DEB_CNT		*(volatile unsigned int *)(ADC_BASE + 0x2c)
#define	ADC_INTC			*(volatile unsigned int *)(ADC_BASE + 0x30)

static adc_conf_dma()
{
	*(volatile unsigned int *)0xbfd00424 |= (1 << 22);	//enable adc_dma on dma2.
	*(volatile unsigned int *)ADC_DMA_DESC_ADDR = (ADC_DMA_DESC_ADDR + 0x100) & 0x1fffffff;
	*(volatile unsigned int *)(ADC_DMA_DESC_ADDR + 0x4) = ADC_DMA_RES_ADDR & 0x1fffffff;
	*(volatile unsigned int *)(ADC_DMA_DESC_ADDR + 0x8) = ADC_DMA_DADDR & 0x1fffffff;
	*(volatile unsigned int *)(ADC_DMA_DESC_ADDR + 0xc) = (RES_LEN >> 2);
	*(volatile unsigned int *)(ADC_DMA_DESC_ADDR + 0x10) = 0;		//step len.
	*(volatile unsigned int *)(ADC_DMA_DESC_ADDR + 0x14) = 1;		//step time.
	*(volatile unsigned int *)(ADC_DMA_DESC_ADDR + 0x14) = 0;		//dma read.
	*(volatile unsigned int *)DMA_BASE = (ADC_DMA_DESC_ADDR & 0x1fffffff) | 0xa;	//dma2, start.
}

static int ls1c_adc_test(int argc,char **argv)
{
	int flag_channal;	//which channel will be select
	int flag_s_c;		//single or continue mode
	int flag_t_n;		//touch screen or normal adc mode
	unsigned int result;
	unsigned int x_pos, y_pos;

	if (argc == 1) {
		flag_channal = 0;
		flag_s_c = ADC_SINGLE_MODE;
		flag_t_n = NOR_MODE;
		printf("use default mode: channel 0, single, normal adc !\n");
	} else if (argc < 4) {
		printf("please use \"ls1c_adc_test channel, single or continue, ts or normal.\"");
	}
	else {
		flag_channal = (unsigned int)strtoul(argv[1], 0, 0);
		flag_s_c = (unsigned int)strtoul(argv[2], 0, 0);
		flag_t_n = (unsigned int)strtoul(argv[3], 0, 0);
	}

	ADC_S_CTL |= (1 << 6) | (1 << 5);		//power down all adc, and reset controller.
	delay(100);
	ADC_S_CTL = 0x00;
	delay(100);
	ADC_C_CTL &= ~(1 << 7);		//stop all continue adc.
	ADC_CNT = 0x400010;			//set adc_pre and adc_gap.

	if (flag_t_n == TS_MODE) {	//touch screen mode.
		ADC_INTC = 0x1f;	//clear interrupt flag.
		ADC_INTC = 0x60;	//enable press and release interrupt.
		ADC_X_RANGE = 0x3ff0001;
		ADC_Y_RANGE = 0x3ff0001;
//		ADC_X_RANGE = (650 << 16) | 370;
//		ADC_Y_RANGE = (800 << 16) | 380;
		ADC_DEB_CNT = 0x640;
		ADC_C_CTL = ADC_CC_EN | ADC_TOUCH_SEL;
		ADC_S_CTL = ADC_S_SOC;	//start adc convert.
		while (1) {
			if (ADC_INTC & ADC_INT_PRESS) {
				result = ADC_AXIS;
				x_pos = (result >> 16) & 0x3ff;
//				x_pos = (x_pos * ADC_X_LEN) >> 10;
				y_pos = result & 0x3ff;
//				y_pos = (y_pos * ADC_Y_LEN) >> 10;
				printf("%x x_pos = %d, y_pos = %d !\n", result, x_pos, y_pos);
				ADC_INTC &= ADC_INT_PRESS;	//clear press flag.
				break;
			}
		}
	}
	else if (flag_t_n == NOR_MODE) {
		if (flag_s_c == ADC_SINGLE_MODE) {	//adc single test mode.
			ADC_S_CTL = ADC_S_SOC | (1 << flag_channal);
			if (flag_channal < 2) {
				while (ADC_S_DOUT0 & ADC_S_BUSY);
				result = (ADC_S_DOUT0 >> (16 * flag_channal)) & 0x3ff;
			} else {
				while (ADC_S_DOUT1 & ADC_S_BUSY);
				result = (ADC_S_DOUT1 >> (16 * (flag_channal - 2))) & 0x3ff;
			}
			printf("channel %d, single mode, result = 0x%2x !\n", flag_channal, result);
		}
		else {		//adc continue test mode.
			adc_conf_dma();
			ADC_C_CTL = ADC_CC_EN | (1 << flag_channal);
			ADC_S_CTL = ADC_S_SOC;	//start adc convert.
			delay(20000);
			*(volatile unsigned int *)DMA_BASE = 0x12;	//stop dma2;
			result = *(volatile unsigned int *)ADC_DMA_RES_ADDR;
			ADC_S_CTL |= (1 << 6) | (1 << 5);		//power down all adc, and reset controller.
			printf("channel %d, continue result =0x%2x !\n", flag_channal, result);
		}
	}
}

/*  触摸测试  ls1c_adc_test 0 0 2 */
/*  通道测试  ls1c_adc_test 0 1 1 */
static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"ls1c_adc_test", "", 0, "adc [channel|s-c|ts-nor]", ls1c_adc_test, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void init_cmd(void)
{
	cmdlist_expand(Cmds, 1);
}
