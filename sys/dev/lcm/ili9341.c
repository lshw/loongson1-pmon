#include <pmon.h>
#include <cpu.h>
#include <sys/types.h>
#include <termio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "include/gpio.h"
#include "ili9341.h"
#include "ili9341_lcd_dis.h"

typedef unsigned int  u32;
typedef unsigned short u16;
typedef unsigned char  u8;
typedef signed int  s32;
typedef signed short s16;
typedef signed char  s8;
typedef int bool;
typedef unsigned long dma_addr_t;

#define writeb(val, addr) (*(volatile u8*)(addr) = (val))
#define writew(val, addr) (*(volatile u16*)(addr) = (val))
#define writel(val, addr) (*(volatile u32*)(addr) = (val))
#define readb(addr) (*(volatile u8*)(addr))
#define readw(addr) (*(volatile u16*)(addr))
#define readl(addr) (*(volatile u32*)(addr))

#define   BLACK                0x0000                // 黑色：    0,   0,   0 //
#define   BLUE                 0x001F                // 蓝色：    0,   0, 255 //
#define   GREEN                0x07E0                // 绿色：    0, 255,   0 //
#define   CYAN                 0x07FF                // 青色：    0, 255, 255 //
#define   RED                  0xF800                // 红色：  255,   0,   0 //
#define   MAGENTA              0xF81F                // 品红：  255,   0, 255 //
#define   YELLOW               0xFFE0                // 黄色：  255, 255, 0   //
#define   WHITE                0xFFFF                // 白色：  255, 255, 255 //
#define   NAVY                 0x000F                // 深蓝色：  0,   0, 128 //
#define   DGREEN               0x03E0                // 深绿色：  0, 128,   0 //
#define   DCYAN                0x03EF                // 深青色：  0, 128, 128 //
#define   MAROON               0x7800                // 深红色：128,   0,   0 //
#define   PURPLE               0x780F                // 紫色：  128,   0, 128 //
#define   OLIVE                0x7BE0                // 橄榄绿：128, 128,   0 //
#define   LGRAY                0xC618                // 灰白色：192, 192, 192 //
#define   DGRAY                0x7BEF                // 深灰色：128, 128, 128 //

//=========================LCD接口引脚定义======================
#define LCDCS	29		//片选信号
#define LCDA0	36		//命令信号
#define LCDWR	37		//时钟信号
#define LCDRD	35		//数据信号
#define LCDRES	34		//复位信号

static void gpio_init(void)
{
	u32 ret;

	ls1b_gpio_direction_output(LCDCS, 0);
	ls1b_gpio_direction_output(LCDA0, 1);
	ls1b_gpio_direction_output(LCDWR, 1);
	ls1b_gpio_direction_output(LCDRD, 1);
	ls1b_gpio_direction_output(LCDRES, 1);

	ret = readl(0xbfd010c0);
	ret |= 0xFFFF << 8;
	writel(ret, 0xbfd010c0);
	
	ret = readl(0xbfd010d0);
	ret &= ~(0xFFFF << 8);
	writel(ret, 0xbfd010d0);

}

void write_data16(int dat)
{
	u32 ret;
	ret = readl(0xbfd010f0);
	ret &= ~(0xFFFF << 8);
	writel(ret | (dat<<8), 0xbfd010f0);
	gpio_set_value(LCDWR, 0);
	gpio_set_value(LCDWR, 1);
}

static void write_data8(int dat)
{
	u32 ret;
	
	ret = readl(0xbfd010f0);
	ret &= ~(0xFFFF << 8);
//	tmp1 = (0x1F & data) << 19;	/* 低5bit */
//	tmp2 = ((0xE0 & data) >> 5) << 10;	/* 高3bit ((0xE0 & dat) >> 5) << 10; */
	writel(ret | ((0x1F & dat) << 19) | ((0xE0 & dat) << 5), 0xbfd010f0);
	gpio_set_value(LCDWR, 0);
	gpio_set_value(LCDWR, 1);
}

static void write_command(int command)
{
	gpio_set_value(LCDCS, 0);
	gpio_set_value(LCDA0, 0);
	write_data8(command);
	gpio_set_value(LCDA0, 1);
}

static int read_data16(void)
{
	u32 ret;
	u32 tmp1;

	ret = readl(0xbfd010d0);
	ret |= 0xFFFF << 8;
	writel(ret, 0xbfd010d0);
	
	gpio_set_value(LCDRD, 0);
	tmp1 = ((readl(0xbfd010e0) >> 8) & 0xFFFF);
	gpio_set_value(LCDRD, 1);

	ret = readl(0xbfd010d0);
	ret &= ~(0xFFFF << 8);
	writel(ret, 0xbfd010d0);
	
	return tmp1;
}

static int read_data8(void)
{
	u32 ret;
	u32 tmp1;
	u32 data1, data2;

	ret = readl(0xbfd010d0);
	ret |= 0xFFFF << 8;
	writel(ret, 0xbfd010d0);
	
	gpio_set_value(LCDRD, 0);
	tmp1 = readl(0xbfd010e0);
	data1 = (tmp1 >> 19) & 0x1F;
	data2 = (tmp1 >> 5) & 0xE0;
	tmp1 = data1 | data2;
	gpio_set_value(LCDRD, 1);

	ret = readl(0xbfd010d0);
	ret &= ~(0xFFFF << 8);
	writel(ret, 0xbfd010d0);
	
	return tmp1;
}

void read_id4()
{
	u32 parameter1, parameter2, parameter3, parameter4;
	write_command(0xd3);
	parameter1 = read_data8();
	parameter2 = read_data8();
	parameter3 = read_data8();
	parameter4 = read_data8();
	printf("ID: %x %x %x %x\n", parameter1, parameter2, parameter3, parameter4);
}

//========================================================================
// 函数: void clear_dot_lcd(int x, int y)
// 描述: 清除在LCD的真实坐标系上的X、Y点（清除后该点为黑色）
// 参数: x 		X轴坐标
//		 y 		Y轴坐标
// 返回: 无
// 备注: 
// 版本:
//========================================================================
void clear_dot_lcd(int x, int y)
{
	x = y;//无意义，仅为了不提警告
}

//========================================================================
// 函数: unsigned int get_dot_lcd(int x, int y)
// 描述: 获取在LCD的真实坐标系上的X、Y点上的当前填充色数据
// 参数: x 		X轴坐标
//		 y 		Y轴坐标
// 返回: 该点的颜色
// 备注: 
// 版本:
//========================================================================
unsigned int get_dot_lcd(int x, int y)
{
/*
	unsigned int Read_Data;
	LCD_RegWrite(0x20,x);
	LCD_RegWrite(0x21,y);
	LCD_Reg22();
	Read_Data = LCD_DataRead();
	return Read_Data;
*/
	return 0;
}

//========================================================================
// 函数: void set_dot_addr_lcd(int x, int y)
// 描述: 设置在LCD的真实坐标系上的X、Y点对应的RAM地址
// 参数: x 		X轴坐标
//		 y 		Y轴坐标
// 返回: 无
// 备注: 仅设置当前操作地址，为后面的连续操作作好准备
// 版本:
//========================================================================
void set_dot_addr_lcd(int x, int y)
{
	//列地址 x
	write_command(0x2A);
	write_data8((x>>8) & 0xFF);	//高8位
	write_data8(x & 0xFF);		//低8位
	write_data8(0x00);			//改变xy像素大小时需要修改这里.
	write_data8(0xEF);
	//页地址 y
	write_command(0x2B);
	write_data8((y>>8) & 0xFF);	//高8位
	write_data8(y & 0xFF);		//低8位
	write_data8(0x01);
	write_data8(0x3F);
	
	write_command(0x2c);
}

//========================================================================
// 函数: void fill_dot_lcd(unsigned int color)
// 描述: 填充一个点到LCD显示缓冲RAM当中，而不管当前要填充的地址如何
// 参数: color 		要填充的点的颜色 
// 返回: 无
// 备注: 
// 版本:
//========================================================================
void fill_dot_lcd(unsigned int color)
{
//	write_command(0x2c);
	write_data16(color);
}
//========================================================================
// 函数: void write_dot_lcd(int x,int y,unsigned int i)
// 描述: 在LCD的真实坐标系上的X、Y点绘制填充色为i的点
// 参数: x 		X轴坐标
//		 y 		Y轴坐标
//		 i 		要填充的点的颜色 
// 返回: 无
// 备注: 
// 版本:
//========================================================================
void write_dot_lcd(unsigned int x, unsigned int y, unsigned int i)
{
/*	//列地址
	write_command(0x2A);
	write_data8((x>>8) & 0xFF);	//高8位
	write_data8(x & 0xFF);		//低8位
	write_data8(0x01);
	write_data8(0x3F);
	//页地址
	write_command(0x2B);
	write_data8((y>>8) & 0xFF);	//高8位
	write_data8(y & 0xFF);		//低8位
	write_data8(0x00);
	write_data8(0xEF);
	
	write_command(0x2c);*/
	set_dot_addr_lcd(x, y);
	write_data16(i);
}

//========================================================================
// 函数: void lcd_fill(unsigned int dat)
// 描述: 会屏填充以dat的数据至各点中
// 参数: dat   要填充的颜色数据
// 返回: 无
// 备注: 仅在LCD初始化程序当中调用
// 版本:
//========================================================================
void lcd_fill(unsigned int dat)
{
	unsigned int i;
	unsigned int j;
	set_dot_addr_lcd(0, 0);
	//改变xy像素大小时需要修改这里.
	for(i=0; i<240; i++){
		for(j=0; j<320; j++){
			fill_dot_lcd(dat);
		}
	}
}

//========================================================================
// 函数: void lcd_fill_s(unsigned int number,unsigned int color)
// 描述: 连续填充以color色调的number个点
// 参数: number 填充的数量    color  像素点的颜色  
// 返回:
// 备注:
// 版本:
//========================================================================
void lcd_fill_s(unsigned int number,unsigned int color)
{
//	LCD_Reg22();
	while(number != 0){
		fill_dot_lcd(color);
		number--;
	}
}

//========================液晶显示模块初始化================================= 
void lcd_init() 
{ 
	gpio_init();
	gpio_set_value(LCDRES, 0);       //LCD 复位有效(L) 
	delay(100); // 延时100ms , Datasheet 要求至少大于1us
	gpio_set_value(LCDRES, 1);    //LCD 复位无效(H)
	delay(100); //硬件复位
	read_id4();
	//************* Start Initial Sequence **********//
	write_command(0x11);	//Sleep OUT
	delay(100);
//	write_command(0xCB);
//	write_data8(0x01);

	write_command(0xC0);	//Power Control 1 
	write_data8(0x23);
	write_data8(0x08);


	write_command(0xC1);	//Power Control 2 
	write_data8(0x04);     

	write_command(0xC5);	//VCOM Control 1
	write_data8(0x25);
	write_data8(0x2b);

	write_command(0x36);	//Memory Access Control 内存访问控制
	write_data8(0x48);		//改变xy像素大小时需要修改这里.

	write_command(0xB1);	//Frame Control 帧频控制
	write_data8(0x00);
	write_data8(0x1b);		//OSC 0x16,0x18

	write_command(0xB6);	//显示功能控制
	write_data8(0x0A);
	write_data8(0x82);

	write_command(0xC7);	//VCOM Control 2 
	write_data8(0xBC);

	write_command(0xF2);	//??
	write_data8(0x00);

	write_command(0x26);	//伽玛曲线设置
	write_data8(0x01);

	write_command(0x3a);	//像素格式设置
	write_data8(0x55);		//16 bits / pixel 

	write_command(0x2a);	//列地址设置
	write_data8(0x00);
	write_data8(0x00);
	write_data8(0x00);
	write_data8(0xef);		//239
	write_command(0x2b);	//
	write_data8(0x00);
	write_data8(0x00);
	write_data8(0x01);
	write_data8(0x3f);		//319

	//=======================================
	write_command(0xE0);	//正伽玛校正
	write_data8(0x1f);
	write_data8(0x25);
	write_data8(0x25);
	write_data8(0x0c);
	write_data8(0x11);
	write_data8(0x0a);
	write_data8(0x4e);
	write_data8(0xcb);
	write_data8(0x37);
	write_data8(0x03);
	write_data8(0x00);
	write_data8(0x00);
	write_data8(0x00);
	write_data8(0x00);
	write_data8(0x00);

	write_command(0XE1);	//负伽玛校正
	write_data8(0x00);
	write_data8(0x1a);
	write_data8(0x1c);
	write_data8(0x02);
	write_data8(0x0e);
	write_data8(0x06);
	write_data8(0x31);
	write_data8(0x36);
	write_data8(0x48);
	write_data8(0x0c);
	write_data8(0x1f);
	write_data8(0x1f);
	write_data8(0x3f);
	write_data8(0x3f);
	write_data8(0x1F);

	write_command(0x29);	//DISPON (Display ON) 
	
	delay(80000);
	lcd_fill(LCD_INITIAL_COLOR);
/*	
	lcd_fill(RED);
	delay(1000000);
	lcd_fill(GREEN);
	delay(1000000);
	lcd_fill(WHITE);
	delay(1000000);
	lcd_fill(BLACK);
	delay(1000000);
	lcd_fill(CYAN);
	delay(1000000);
	lcd_fill(YELLOW);
	delay(1000000);
	lcd_fill(PURPLE);
	delay(1000000);
	lcd_fill(OLIVE);
	delay(1000000);
	lcd_fill(MAROON);
	delay(1000000);
	lcd_fill(WHITE);
	delay(1000000);

	write_dot_lcd(0, 0, RED);
	write_dot_lcd(239, 0, RED);
	write_dot_lcd(0, 319, RED);
	write_dot_lcd(239, 319, RED);
	write_dot_lcd(1, 0, RED);
	write_dot_lcd(2, 0, RED);
	write_dot_lcd(100, 100, RED);
	write_dot_lcd(101, 100, RED);
	write_dot_lcd(102, 100, RED);
	write_dot_lcd(103, 100, RED);
*/
}

void ili9341_test(void)
{
	lcd_init();							//LCD初始化
	font_set(1,0xf800);
	put_string(10,10,"Mz Design!");
//	clr_screen();
	font_set(1,0x07e0);
	put_string(10,42,"Mz");
	font_set(2,0x07e0);
	put_char(42,40,0);
	put_char(74,40,1);

	set_paint_mode(0,0x001f);
	put_pixel(10,72);
	put_pixel(12,72);
	put_pixel(14,72);
	line_my(10, 75, 230, 75);
	rectangle(20,80,100,120,1);
	rectangle(18,78,102,122,0);
	circle(60,180,30,1);
	circle(60,180,32,0);
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"ili9341","", 0, "test ili9341", lcd_init, 0, 99, CMD_REPEAT},
	{"ili9341_test","", 0, "test ili9341", ili9341_test, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
