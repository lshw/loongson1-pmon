#include <pmon.h>
#include <cpu.h>
#include <string.h>

#include "target/fcr.h"

#define SPI_REG_BASE 0x1fe80000
#define FCR_SPCR        0x00		//控制寄存器
#define FCR_SPSR        0x01		//状态寄存器
#define FCR_SPDR        0x02		//数据传输寄存器
#define FCR_SPER        0x03		//外部寄存器
#define FCR_PARAM			0x04		//SPI Flash参数控制寄存器
#define FCR_SOFTCS		0x05		//SPI Flash片选控制寄存器
#define FCR_TIMING		0x06		//SPI Flash时序控制寄存器

#define SET_SPI(idx,value) KSEG1_STORE8(SPI_REG_BASE+idx, value)
#define GET_SPI(idx)    KSEG1_LOAD8(SPI_REG_BASE+idx)

static void spi_init(void)
{ 
	SET_SPI(FCR_SPSR, 0xc0);
	//SPI Flash参数控制寄存器
	SET_SPI(0x4, 0x00);
	//#define SPER      0x3	//外部寄存器
	//spre:01 [2]mode spi接口模式控制 1:采样与发送时机错开半周期  [1:0]spre 与Spr一起设定分频的比率
	SET_SPI(FCR_SPER, 0x05);
	//SPI Flash片选控制寄存器
//	SET_SPI(0x5, 0x01);			//softcs
	SET_SPI(0x5, 0xFF);			//softcs
	SET_SPI(FCR_SPCR, 0x5c);
}

static inline void set_cs(int bit)
{
if(bit) 
	SET_SPI(0x5, 0xFF);		//cs high
else 
	SET_SPI(0x5, 0x7F);		//cs low
}

static unsigned char  flash_writeb_cmd(unsigned char value)
{
	unsigned char status, ret;

	SET_SPI(FCR_SPDR, value);
	ret = GET_SPI(FCR_SPSR);

	//sw: make sure the rf is not empty!      
	while ((*(volatile unsigned char *)(0xbfe80001))&0x01);

	ret = GET_SPI(FCR_SPSR);

	ret = GET_SPI(FCR_SPDR);
#ifdef SDCARD_DBG
	printf("==treg ret value: %08b\n",ret);
#endif		
	return ret;
}

//********************************************
unsigned int read_tow_byte(void)//Simulating SPI IO
{
	unsigned int Buffer;
	Buffer = flash_writeb_cmd(0xff);	//Provide 8 extra clock同时读取总线返回到SPI控制器的数据
	Buffer = (Buffer<<8) | flash_writeb_cmd(0xff);
	
	return Buffer;
}

int test_MCP3201(int argc,char **argv)
{
	unsigned short temp;
	spi_init();

#ifdef CONFIG_CHINESE
	printf("MCP3201 AD 转换测试\n说明：\n");
	printf("1.调整MCP3201输入的测量电压，转换值应该会发生变化。\n");
	printf("2.按任意键退出测试程序。\n");
#else
	printf("press any key to out of MCP3201 test\n");
#endif
	while (1){
		set_cs(0);
		temp = read_tow_byte();
		set_cs(1);
		delay(100);
		set_cs(0);
		if (read_tow_byte() != temp)
			printf("\b\b\b\b\b\b\b\b\b\b0x%x  ", temp);
//		printf("0x%x\n", temp);
		set_cs(1);
		delay(100);
		if (get_uart_char(COM1_BASE_ADDR)){
		#ifdef CONFIG_CHINESE
			printf("\n退出MCP3201 AD 转换测试程序\n");
		#else
			printf("\nout of MCP3201 test\n");
		#endif
			break;
		}
	}
/*	
	while (1){
		set_cs(0);
		temp1 = read_tow_byte();
		set_cs(1);
		printf("\b\b\b\b\b\b\b\b\b\b0x%x  \n", temp1);
		while(1){
			delay(10000);
			set_cs(0);
			temp2 = read_tow_byte();
			if (temp1 != temp2){
				break;
			}
			set_cs(1);
			if (get_uart_char(COM1_BASE_ADDR)){
				return 0;
			}
		}
	}
*/	
	return 0;
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"test_MCP3201","", 0, "test MCP3201", test_MCP3201, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
