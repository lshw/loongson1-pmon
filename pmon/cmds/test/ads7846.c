#include <pmon.h>
#include <cpu.h>
#include <string.h>
#include "target/fcr.h"
/*
#define K1BASE 0xa0000000
#define KSEG1(addr) (K1BASE | (unsigned int)(addr))

#define KSEG1_STORE8(addr, value) *(volatile unsigned char *)(KSEG1(addr)) = (value)
#define KSEG1_STORE16(addr, value) *(volatile unsigned short *)(KSEG1(addr)) = (value)
#define KSEG1_STORE32(addr, value) *(volatile unsigned int *)(KSEG1(addr)) = (value)

#define KSEG1_LOAD8(addr) *(volatile unsigned char *)(KSEG1(addr))
#define KSEG1_LOAD16(addr) *(volatile unsigned short *)(KSEG1(addr))
#define KSEG1_LOAD32(addr) *(volatile unsigned int *)(KSEG1(addr))*/

#define SPI_BASE  0x1fe80000

#define SPCR      0x0
#define SPSR      0x1
#define TXFIFO    0x2
#define RXFIFO    0x2
#define SPER      0x3
#define PARAM     0x4
#define SOFTCS    0x5
#define PARAM2    0x6

#define RFEMPTY	1
#define TFFULL	8

#define SET_SPI(addr,val)        KSEG1_STORE8(SPI_BASE+addr,val)
#define GET_SPI(addr)            KSEG1_LOAD8(SPI_BASE+addr)
#define dly	do{int i =100; while (i--);}while(0)


/* The ADS7846 has touchscreen and other sensors.
 * Earlier ads784x chips are somewhat compatible.
 */
#define	ADS_START		(1 << 7)
#define	ADS_A2A1A0_d_y		(1 << 4)	/* differential */
#define	ADS_A2A1A0_d_z1		(3 << 4)	/* differential */
#define	ADS_A2A1A0_d_z2		(4 << 4)	/* differential */
#define	ADS_A2A1A0_d_x		(5 << 4)	/* differential */
#define	ADS_A2A1A0_temp0	(0 << 4)	/* non-differential */
#define	ADS_A2A1A0_vbatt	(2 << 4)	/* non-differential */
#define	ADS_A2A1A0_vaux		(6 << 4)	/* non-differential */
#define	ADS_A2A1A0_temp1	(7 << 4)	/* non-differential */
#define	ADS_8_BIT		(1 << 3)
#define	ADS_12_BIT		(0 << 3)
#define	ADS_SER			(1 << 2)	/* non-differential */
#define	ADS_DFR			(0 << 2)	/* differential */
#define	ADS_PD10_PDOWN		(0 << 0)	/* lowpower mode + penirq */
#define	ADS_PD10_ADC_ON		(1 << 0)	/* ADC on */
#define	ADS_PD10_REF_ON		(2 << 0)	/* vREF on + penirq */
#define	ADS_PD10_ALL_ON		(3 << 0)	/* ADC + vREF on */

/* leave ADC powered up (disables penirq) between differential samples */
#define	READ_12BIT_DFR(x, adc, vref) (ADS_START | ADS_A2A1A0_d_ ## x \
	| ADS_12_BIT | ADS_DFR | \
	(adc ? ADS_PD10_ADC_ON : 0) | (vref ? ADS_PD10_REF_ON : 0))

#define	READ_Y(vref)	(READ_12BIT_DFR(y,  1, vref))
#define	READ_Z1(vref)	(READ_12BIT_DFR(z1, 1, vref))
#define	READ_Z2(vref)	(READ_12BIT_DFR(z2, 1, vref))

#define	READ_X(vref)	(READ_12BIT_DFR(x,  1, vref))
#define	PWRDOWN		(READ_12BIT_DFR(y,  0, 0))	/* LAST */

/* single-ended samples need to first power up reference voltage;
 * we leave both ADC and VREF powered
 */
#define	READ_12BIT_SER(x) (ADS_START | ADS_A2A1A0_ ## x \
	| ADS_12_BIT | ADS_SER)

#define	REF_ON	(READ_12BIT_DFR(x, 1, 1))
#define	REF_OFF	(READ_12BIT_DFR(y, 0, 0))

#define ADS7846_FILTER_OK		0
#define ADS7846_FILTER_REPEAT	1
#define ADS7846_FILTER_IGNORE	2


#define REG_GPIO_CFG0		0x1fd010c0		//GPIO 配置寄存器 0
#define REG_GPIO_CFG1		0x1fd010c4		//GPIO 配置寄存器 1
#define REG_GPIO_OE0			0x1fd010d0		//GPIO 配置寄存器输入使能 0
#define REG_GPIO_OE1			0x1fd010d4		//GPIO 配置寄存器输入使能 1
#define REG_GPIO_IN0			0x1fd010e0		//GPIO 配置寄存器输入寄存器 0
#define REG_GPIO_IN1			0x1fd010e4		//GPIO 配置寄存器输入寄存器 1
#define REG_GPIO_OUT0		0x1fd010f0		//GPIO 配置寄存器输出寄存器 0
#define REG_GPIO_OUT1		0x1fd010f4		//GPIO 配置寄存器输出寄存器 1

#define SB2F_BOARD_INTREG_BASE 0x1fd01040

struct sb2f_board_intc_regs
{
	volatile unsigned int int_isr;
	volatile unsigned int int_en;
	volatile unsigned int int_set;
	volatile unsigned int int_clr;		/* offset 0x10*/
	volatile unsigned int int_pol;
   	volatile unsigned int int_edge;		/* offset 0 */
}; 

#define GPIO_IRQ 60
static struct sb2f_board_intc_regs volatile *sb2f_board_hw0_icregs
	= (struct sb2f_board_intc_regs volatile *)(_KSEG1(SB2F_BOARD_INTREG_BASE));

int ads7846_pendown_state(void)
{
	unsigned int ret;
	ret = KSEG1_LOAD32(REG_GPIO_IN1); //读回的数值是反码？
	ret = ((ret >> (GPIO_IRQ & 0x1f)) & 0x01);
	return !ret;
}

void ads7846_detect_penirq(void)
{
	unsigned int ret;
	//配置GPIO0
	ret = KSEG1_LOAD32(REG_GPIO_CFG1); //GPIO0	0xbfd010c0 使能GPIO
	ret |= (1 << (GPIO_IRQ & 0x1f)); //GPIO50
	KSEG1_STORE32(REG_GPIO_CFG1, ret);
	
	ret = KSEG1_LOAD32(REG_GPIO_OE1);//GPIO0 设置GPIO输入使能
	ret |= (1 << (GPIO_IRQ & 0x1f));
	KSEG1_STORE32(REG_GPIO_OE1, ret);
	(sb2f_board_hw0_icregs + 3) -> int_edge		&= ~(1 << (GPIO_IRQ & 0x1f));
	(sb2f_board_hw0_icregs + 3) -> int_pol		&= ~(1 << (GPIO_IRQ & 0x1f));
	(sb2f_board_hw0_icregs + 3) -> int_clr		|= (1 << (GPIO_IRQ & 0x1f));
	(sb2f_board_hw0_icregs + 3) -> int_set		&= ~(1 << (GPIO_IRQ & 0x1f));
	(sb2f_board_hw0_icregs + 3) -> int_en		|= (1 << (GPIO_IRQ & 0x1f));
}


struct ads7846 {
	int			read_cnt;
	int			read_rep;
	unsigned short			last_read;

	unsigned short			debounce_max;
	unsigned short			debounce_tol;
	unsigned short			debounce_rep;
};

static int ads7846_debounce(struct ads7846 *ads, int data_idx, int *val)
{
	struct ads7846		*ts = ads;

	if (!ts->read_cnt /*|| (abs(ts->last_read - *val) > ts->debounce_tol)*/) {
		/* Start over collecting consistent readings. */
		ts->read_rep = 0;
		/* Repeat it, if this was the first read or the read
		 * wasn't consistent enough. */
		if (ts->read_cnt < ts->debounce_max) {
			ts->last_read = *val;
			ts->read_cnt++;
			return ADS7846_FILTER_REPEAT;
		} else {
			/* Maximum number of debouncing reached and still
			 * not enough number of consistent readings. Abort
			 * the whole sample, repeat it in the next sampling
			 * period.
			 */
			ts->read_cnt = 0;
			return ADS7846_FILTER_IGNORE;
		}
	} else {
		if (++ts->read_rep > ts->debounce_rep) {
			/* Got a good reading for this coordinate,
			 * go for the next one. */
			ts->read_cnt = 0;
			ts->read_rep = 0;
			return ADS7846_FILTER_OK;
		} else {
			/* Read more values that are consistent. */
			ts->read_cnt++;
			return ADS7846_FILTER_REPEAT;
		}
	}
}

void ads7846_test(void)
{
	unsigned char val;
	unsigned short y, x, z1, z2;
	int y_ok = 0;
	int cont = 3;
	struct ads7846 *ts;

#ifdef CONFIG_CHINESE
	printf("触摸屏测试\n说明：\n");
	printf("1.触摸屏幕会返回坐标值。\n");
	printf("2.如果触摸屏幕坐标值不变，或者没有坐标显示，说明触摸屏工作不正常。\n");
	printf("3.按任意键退出测试程序。\n");
#else
	printf("press any key to out of touchscreen test\n");
#endif
	ts = malloc(sizeof(struct ads7846));
	memset(ts, 0, sizeof(struct ads7846));
	ts->debounce_rep = 3;
	ts->debounce_max = 10;
	ts->debounce_tol = 50;
	
	ads7846_detect_penirq();
	//初始化Loongson 1B的SPI控制器
	SET_SPI(SPSR, 0xC0);
	val = GET_SPI(PARAM);
	val &= 0xfe;
	SET_SPI(PARAM, val);
	SET_SPI(SPCR, 0x50);
	SET_SPI(SPER, 0x05);
//	SET_SPI(SPER, 0x04);	//快
	SET_SPI(SOFTCS,0xDF);	//片选
	
	while(1){
		while(ads7846_pendown_state() && cont){
			SET_SPI(TXFIFO, READ_Y(0));
			while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY);
			GET_SPI(RXFIFO);
			delay(100);	//需要适当的延时
			SET_SPI(TXFIFO,0x00);
			while((GET_SPI(SPSR)) & RFEMPTY);
			y = GET_SPI(RXFIFO);
			delay(100);	//需要适当的延时
			SET_SPI(TXFIFO,0x00);
			while((GET_SPI(SPSR)) & RFEMPTY);
			y = (y << 4) | (GET_SPI(RXFIFO) >> 4);
	
			SET_SPI(TXFIFO, READ_X(0));
			while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY);
			GET_SPI(RXFIFO);
			delay(20);	//需要适当的延时
			SET_SPI(TXFIFO,0x00);
			while((GET_SPI(SPSR)) & RFEMPTY);
			x = GET_SPI(RXFIFO);
			delay(20);	//需要适当的延时
			SET_SPI(TXFIFO,0x00);
			while((GET_SPI(SPSR)) & RFEMPTY);
			x = (x << 4) | (GET_SPI(RXFIFO) >> 4);
	//		if (ads7846_debounce(ts, 0, &x) != ADS7846_FILTER_OK)
	//			continue;
	
	
			SET_SPI(TXFIFO, PWRDOWN);
			while(((GET_SPI(SPSR)) & RFEMPTY) == RFEMPTY);
			GET_SPI(RXFIFO);
			delay(20);	//需要适当的延时
			SET_SPI(TXFIFO,0x00);
			while((GET_SPI(SPSR)) & RFEMPTY);
			val = GET_SPI(RXFIFO);
			delay(20);	//需要适当的延时
			SET_SPI(TXFIFO,0x00);
			while((GET_SPI(SPSR)) & RFEMPTY);
			val = (val << 4) | (GET_SPI(RXFIFO) >> 4);
			cont--;
			delay(20);	//需要适当的延时
		}
		if (cont == 0){
			printf("y = %x   x = %x  \n", y, x);
			while(ads7846_pendown_state());
			cont = 3;
		}
		if (get_uart_char(COM1_BASE_ADDR)){
		#ifdef CONFIG_CHINESE
			printf("\n退出触摸屏测试程序\n");
		#else
			printf("\nout of touchscreen test\n");
		#endif
			break;
		}
	}
	SET_SPI(SOFTCS,0xFF);	//片选
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"ads7846","", 0, "test ads7846", ads7846_test, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
