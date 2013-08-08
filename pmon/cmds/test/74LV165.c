#include <pmon.h>
#include <cpu.h>
#include <string.h>
#include "target/fcr.h"
/*
#define K1BASE 0xa0000000
#define KSEG1(addr) ((void *)(K1BASE | (unsigned int)(addr)))

#define KSEG1_STORE32(addr, value) *(volatile unsigned int *)(KSEG1(addr)) = (value)
#define KSEG1_LOAD32(addr) *(volatile unsigned int *)(KSEG1(addr))*/

#define REG_GPIO_CFG0		0x1fd010c0		//GPIO 配置寄存器 0
#define REG_GPIO_CFG1		0x1fd010c4		//GPIO 配置寄存器 1
#define REG_GPIO_OE0			0x1fd010d0		//GPIO 配置寄存器输入使能 0
#define REG_GPIO_OE1			0x1fd010d4		//GPIO 配置寄存器输入使能 1
#define REG_GPIO_IN0			0x1fd010e0		//GPIO 配置寄存器输入寄存器 0
#define REG_GPIO_IN1			0x1fd010e4		//GPIO 配置寄存器输入寄存器 1
#define REG_GPIO_OUT0		0x1fd010f0		//GPIO 配置寄存器输出寄存器 0
#define REG_GPIO_OUT1		0x1fd010f4		//GPIO 配置寄存器输出寄存器 1

#define REG_GPIO_CFG			REG_GPIO_CFG1
#define REG_GPIO_OE			REG_GPIO_OE1
#define REG_GPIO_IN			REG_GPIO_IN1
#define REG_GPIO_OUT			REG_GPIO_OUT1

#define GPIOS		0x07
#define OFFSET	0x00

#define GPIO0		(1<<0)
#define GPIO1		(1<<1)
#define GPIO2		(1<<2)

#define GPIO38	(1<<(38-32))	/* 注意需要修改 */
#define GPIO39	(1<<(39-32))	/* 注意需要修改 */
#define GPIO41	(1<<(41-32))	/* 注意需要修改 */

//#define KEY_DATA	GPIO38
//#define KEY_EN		GPIO39
//#define KEY_CLK		GPIO41

#define KEY_DATA	GPIO41
#define KEY_CLK	GPIO39
#define KEY_EN	GPIO38

/* 定义蜂鸣器(buzzer)使用的GPIO */
#define BUZZER	(1<<(40-32))

//根据输入参数i得到第一个为1位的位置
static int find_first_bit(unsigned int i)
{
    unsigned int index = i;
    //将第一个为1位的低位都置1，其它位都置0
    index = (index-1)  &  (~index);
    //得到有多少为1的位
    index = (index & 0x55555555) + ((index>>1) & 0x55555555);  
    index = (index & 0x33333333) + ((index>>2) & 0x33333333);
    index = (index & 0x0F0F0F0F) + ((index>>4) & 0x0F0F0F0F);
    index = (index & 0xFF) + ((index & 0xFF00) >> 8) + ((index & 0xFF0000) >> 16) + ((index & 0xFF000000) >> 24);
    //得到位数,如果为32则表示全0
    return (int)(index);
}

static void gpio_init_74LV165(int flag)
{
	unsigned int reg = 0;
	
	/* 使能引脚GPIO方式 */
	if (flag){
		/* 配置GPIO */
		reg = KSEG1_LOAD32(REG_GPIO_CFG);
		reg |= (KEY_DATA | KEY_EN | KEY_CLK);
		KSEG1_STORE32(REG_GPIO_CFG, reg);
		/* GPIO输入输出方式 */
		reg = KSEG1_LOAD32(REG_GPIO_OE);
		reg |= KEY_DATA;
		reg &= ~(KEY_EN | KEY_CLK);
		KSEG1_STORE32(REG_GPIO_OE, reg);
		/* 设置输出电平 */
		reg = KSEG1_LOAD32(REG_GPIO_OUT);
		reg &= ~(KEY_EN | KEY_CLK);
		KSEG1_STORE32(REG_GPIO_OUT, reg);
	}
	/* 取消引脚GPIO方式 */
	else{
		reg = KSEG1_LOAD32(REG_GPIO_CFG);
		reg &= ~(KEY_DATA | KEY_EN | KEY_CLK);
		KSEG1_STORE32(REG_GPIO_CFG, reg);
	}
}

static unsigned int soc_74LV165_read(void)
{
	unsigned int time;
	unsigned int val = 0,reg = 0;
	unsigned int key_val = 0;
	
	/* CLK =0 */
	reg = KSEG1_LOAD32(REG_GPIO_OUT); //GPIO0	0xbfd010c0 使能GPIO
	KSEG1_STORE32(REG_GPIO_OUT, reg & (~KEY_CLK));	//CLK置0 时钟低变高触发
	/* PL = 0 */
	reg = KSEG1_LOAD32(REG_GPIO_OUT);
	KSEG1_STORE32(REG_GPIO_OUT, reg & (~KEY_EN));	//PL置0
	/* PL = 1 */
	reg = KSEG1_LOAD32(REG_GPIO_OUT);
	KSEG1_STORE32(REG_GPIO_OUT, reg | KEY_EN);	//PL置1 准备读取串行数据
	
	reg = KSEG1_LOAD32(REG_GPIO_IN);//还没有跳变沿时 DATA引脚为D7值
	val = ((~reg) & KEY_DATA) >> (41-32); //注意这里KEY_DATA变了这里也要变
	key_val = val;
	/* scanf keyboard */
	for(time = 1; time < 16; time ++)
	{
		/* CLK = 1 */
		reg = KSEG1_LOAD32(REG_GPIO_OUT);
		KSEG1_STORE32(REG_GPIO_OUT, reg | KEY_CLK);//CLK置1 时钟低变高触发
		
		reg = KSEG1_LOAD32(REG_GPIO_IN);
		val = ((~reg) & KEY_DATA) >> (41-32); //注意这里KEY_DATA变了这里也要变
		key_val |= (val << time);
		/* CLK = 0 */
		reg = KSEG1_LOAD32(REG_GPIO_OUT);
		KSEG1_STORE32(REG_GPIO_OUT, reg & (~KEY_CLK));//CLK置0 时钟低变高触发
	}
	if(key_val == 0x0000ffff){
		return key_val;
	}
	val = find_first_bit(key_val);
	return val;
}


void buzzer_init(int flag)
{
	unsigned int reg = 0;
	
	/* 使能引脚GPIO方式 */
	if (flag){
		/* 配置GPIO */
		reg = KSEG1_LOAD32(REG_GPIO_CFG1);
		reg |= BUZZER;
		KSEG1_STORE32(REG_GPIO_CFG1, reg);
		/* GPIO输入输出方式 */
		reg = KSEG1_LOAD32(REG_GPIO_OE1);
		reg &= ~(BUZZER);
		KSEG1_STORE32(REG_GPIO_OE1, reg);
		/* 设置输出电平 关闭蜂鸣器 */
		reg = KSEG1_LOAD32(REG_GPIO_OUT1);
		reg &= ~(BUZZER);
		KSEG1_STORE32(REG_GPIO_OUT1, reg);
	}
	/* 取消引脚GPIO方式 */
	else{
		reg = KSEG1_LOAD32(REG_GPIO_OUT1);
		reg &= ~(BUZZER);
		KSEG1_STORE32(REG_GPIO_OUT1, reg);
		
		reg = KSEG1_LOAD32(REG_GPIO_CFG1);
		reg &= ~(BUZZER);
		KSEG1_STORE32(REG_GPIO_CFG1, reg);
	}
}

void buzzer_ctrl(int cmd)
{
	unsigned int reg = 0;
	
	/* 激活蜂鸣器 */
	if (cmd){
		reg = KSEG1_LOAD32(REG_GPIO_OUT1);
		reg |= BUZZER;
		KSEG1_STORE32(REG_GPIO_OUT1, reg);
	}
	/* 禁止蜂鸣器 */
	else{
		reg = KSEG1_LOAD32(REG_GPIO_OUT1);
		reg &= ~(BUZZER);
		KSEG1_STORE32(REG_GPIO_OUT1, reg);
	}
}

void button_test(void)
{
	int val;
	int key_status = 0;
	int count = 0;
#ifdef CONFIG_CHINESE
	printf("按键测试\n说明：\n");
	printf("1.如果有SD插在开发板的SD卡插槽，请把SD卡退出。\n");
	printf("2.没有按键按下时\"计数值\"不变，当按键按下时\"计数值\"改变。\n");
	printf("3.如果没有按键按下而\"计数值\"不断改变，说明该按键短路。\n");
	printf("4.如果按键按下而\"计数值\"不变，说明该按键开路。\n");
	printf("5.按任意键退出测试程序。\n");
#else
	printf("press any key to out of button_test\n");
#endif
	gpio_init_74LV165(1);
	buzzer_init(1);
	while(1){
		val = soc_74LV165_read();
		delay(20000);	//延时去抖
		if (soc_74LV165_read() != val)
			continue;
		switch(val){
			case 0:
			#ifdef CONFIG_CHINESE
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b按键 %d    计数 %d", val, count++);
			#else
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bbutton %d    count %d", val, count++);
			#endif
				key_status |= (1<<val);
				buzzer_ctrl(1);
				//delay(2000000);
				//while(soc_74LV165_read() == val);
			break;
			case 1:
				//printf("button 1 OK\n");
			#ifdef CONFIG_CHINESE
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b按键 %d    计数 %d", val, count++);
			#else
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bbutton %d    count %d", val, count++);
			#endif
				key_status |= (1<<val);
				buzzer_ctrl(1);
				//delay(2000000);
				//while(soc_74LV165_read() == val);
			break;
			case 2:
				//printf("button 2 OK\n");
			#ifdef CONFIG_CHINESE
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b按键 %d    计数 %d", val, count++);
			#else
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bbutton %d    count %d", val, count++);
			#endif
				key_status |= (1<<val);
				buzzer_ctrl(1);
				//delay(2000000);
				//while(soc_74LV165_read() == val);
			break;
			case 3:
				//printf("button 3 OK\n");
			#ifdef CONFIG_CHINESE
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b按键 %d    计数 %d", val, count++);
			#else
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bbutton %d    count %d", val, count++);
			#endif
				key_status |= (1<<val);
				buzzer_ctrl(1);
				//delay(2000000);
				//while(soc_74LV165_read() == val);
			break;
			case 4:
				//printf("button 4 OK\n");
			#ifdef CONFIG_CHINESE
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b按键 %d    计数 %d", val, count++);
			#else
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bbutton %d    count %d", val, count++);
			#endif
				key_status |= (1<<val);
				buzzer_ctrl(1);
				//delay(2000000);
				//while(soc_74LV165_read() == val);
			break;
			case 5:
				//printf("button 5 OK\n");
			#ifdef CONFIG_CHINESE
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b按键 %d    计数 %d", val, count++);
			#else
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bbutton %d    count %d", val, count++);
			#endif
				key_status |= (1<<val);
				buzzer_ctrl(1);
				//delay(2000000);
				//while(soc_74LV165_read() == val);
			break;
			case 6:
				//printf("button 6 OK\n");
			#ifdef CONFIG_CHINESE
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b按键 %d    计数 %d", val, count++);
			#else
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bbutton %d    count %d", val, count++);
			#endif
				key_status |= (1<<val);
				buzzer_ctrl(1);
				//delay(2000000);
				//while(soc_74LV165_read() == val);
			break;
			case 7:
				//printf("button 7 OK\n");
			#ifdef CONFIG_CHINESE
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b按键 %d    计数 %d", val, count++);
			#else
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bbutton %d    count %d", val, count++);
			#endif
				key_status |= (1<<val);
				buzzer_ctrl(1);
				//delay(2000000);
				//while(soc_74LV165_read() == val);
			break;
			case 8:
				//printf("button 8 OK\n");
			#ifdef CONFIG_CHINESE
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b按键 %d    计数 %d", val, count++);
			#else
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bbutton %d    count %d", val, count++);
			#endif
				key_status |= (1<<val);
				buzzer_ctrl(1);
				//delay(2000000);
				//while(soc_74LV165_read() == val);
			break;
			case 9:
				//printf("button 9 OK\n");
			#ifdef CONFIG_CHINESE
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b按键 %d    计数 %d", val, count++);
			#else
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bbutton %d    count %d", val, count++);
			#endif
				key_status |= (1<<val);
				buzzer_ctrl(1);
				//delay(2000000);
				//while(soc_74LV165_read() == val);
			break;
			case 10:
				//printf("button 10 OK\n");
			#ifdef CONFIG_CHINESE
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b按键 %d    计数 %d", val, count++);
			#else
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bbutton %d    count %d", val, count++);
			#endif
				key_status |= (1<<val);
				buzzer_ctrl(1);
				//delay(2000000);
				//while(soc_74LV165_read() == val);
			break;
			case 11:
				//printf("button 11 OK\n");
			#ifdef CONFIG_CHINESE
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b按键 %d    计数 %d", val, count++);
			#else
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bbutton %d    count %d", val, count++);
			#endif
				key_status |= (1<<val);
				buzzer_ctrl(1);
				//delay(2000000);
				//while(soc_74LV165_read() == val);
			break;
			case 12:
				//printf("button 12 OK\n");
			#ifdef CONFIG_CHINESE
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b按键 %d    计数 %d", val, count++);
			#else
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bbutton %d    count %d", val, count++);
			#endif
				key_status |= (1<<val);
				buzzer_ctrl(1);
				//delay(2000000);
				//while(soc_74LV165_read() == val);
			break;
			case 13:
				//printf("button 13 OK\n");
			#ifdef CONFIG_CHINESE
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b按键 %d    计数 %d", val, count++);
			#else
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bbutton %d    count %d", val, count++);
			#endif
				key_status |= (1<<val);
				buzzer_ctrl(1);
				//delay(2000000);
				//while(soc_74LV165_read() == val);
			break;
			case 14:
				//printf("button 14 OK\n");
			#ifdef CONFIG_CHINESE
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b按键 %d    计数 %d", val, count++);
			#else
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bbutton %d    count %d", val, count++);
			#endif
				key_status |= (1<<val);
				buzzer_ctrl(1);
				//delay(2000000);
				//while(soc_74LV165_read() == val);
			break;
			case 15:
				//printf("button 15 OK\n");
			#ifdef CONFIG_CHINESE
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b按键 %d    计数 %d", val, count++);
			#else
				printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bbutton %d    count %d", val, count++);
			#endif
				key_status |= (1<<val);
				buzzer_ctrl(1);
				//delay(2000000);
				//while(soc_74LV165_read() == val);
			break;
			default:
				if(val == 0x0000ffff){
				#ifdef CONFIG_CHINESE
					printf("\n74LV165芯片错误，没有探测到芯片\n");
				#else
					printf("\n74LV165 Error\n");
				#endif
					gpio_init_74LV165(0);
					buzzer_init(0);
					return;
				}
				buzzer_ctrl(0);
			break;
		}
		if(key_status == 0xFFFF){
		#ifdef CONFIG_CHINESE
			printf("\n所有按键测试完成\n");
		#else
			printf("\nAll button is OK\n");
		#endif
			break;
		}
		if (get_uart_char(COM1_BASE_ADDR)){
		#ifdef CONFIG_CHINESE
			printf("\n退出按键测试程序\n");
		#else
			printf("\nout of button test\n");
		#endif
			break;
		}
	}
	gpio_init_74LV165(0);
	buzzer_ctrl(0);
//	buzzer_init(0);
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"button_test","", 0, "button_test", button_test, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
