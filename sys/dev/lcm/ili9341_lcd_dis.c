//========================================================================
// 文件名: LCD_Dis.c
// 作  者: Xinqiang Zhang(email: xinqiang@mzdesign.com.cn)
// 日  期: 2006/10/18
// 描  述: 通用版LCD用户接口层程序集
//
// 参  考: 请参考具体显示器的硬件描述说明书或者是DataSheet,并以此来编写加速
//			版接口程序,本程序集适用于8位MCU
// 版  本:
//      2006/10/18      First version    Mz Design
//========================================================================
//#include "LCD_Driver_User.h"			//LCD底层驱动声明头文件
#include "ili9341.h"					//LCD的配置文件，比如坐标轴是否倒置等的定义
#include "ili9341_lcd_dis.h"

#define dis_zero  0


//extern unsigned int Asii8[];		//6X8的ASII字符库
extern unsigned char Asii1529[];	//15X29的ASII字符库
extern unsigned char GB32[];		//32*32自定义的汉字库

int x_witch;				//字符写入时的宽度
int y_witch;				//字符写入时的高度
int font_wrod;				//字体的大
unsigned char *char_tab;	//字库指针
int plot_mode;				//绘图模式
unsigned int bmp_color;
unsigned int char_color;

void pos_switch(unsigned int * x,unsigned int * y);
void writ_dot(int x,int y,unsigned int color);
//========================================================================
// 函数: void font_set(int font_num,unsigned int color)
// 描述: 文本字体设置
// 参数: font_num 字体选择,以驱动所带的字库为准
//		 color  文本颜色,仅作用于自带字库  
// 返回: 无
// 备注: 
// 版本:
//========================================================================
void font_set(int font_num,unsigned int color)
{
	switch(font_num)
	{
/*		case 0: font_wrod = 3;	//ASII字符A
				x_witch = 6;
				y_witch = 1;
				char_color = color;
//				char_tab = (Asii8 - 32*3);
		break;*/
		case 1: font_wrod = 58;	//ASII字符B
				x_witch = 15;
				y_witch = 29;
				char_color = color;
				char_tab = (unsigned char *)(Asii1529 - (32*58));
		break;
		case 2: font_wrod = 128;	//汉字A
				x_witch = 32;
				y_witch = 32;
				char_color = color;
				char_tab = (unsigned char *)GB32;
		break;
/*		case 3: font_wrod = 16;	//汉字B
				x_witch = 16;
				y_witch = 2;
				char_color = color;
				char_tab = GB16;
		break;*/
		default: break;
	}
}
//========================================================================
// 函数: void put_char(int x,int y,unsigned int a)
// 描述: 写入一个标准字符
// 参数: x  X轴坐标     y  Y轴坐标
//		 a  要显示的字符在字库中的偏移量  
// 返回: 无
// 备注: ASCII字符可直接输入ASCII码即可
// 版本:
//========================================================================
void put_char(int x,int y,unsigned int a)
{
	int i,j;//,K;		//数据暂存
	unsigned char *p_data;
	unsigned char temp;
	p_data = char_tab + a*font_wrod;	//要写字符的首地址
	j = 0;
	while((j ++) < y_witch){
		if(y > DIS_Y_MAX) break;
		i = 0;
		while(i < x_witch){
			if((i&0x07) == 0){
				temp = *(p_data ++);
			}
//			K = 0;
			if((x+i) > DIS_X_MAX) break;
			if((temp & 0x80) > 0) writ_dot(x+i,y,char_color);
			temp = temp << 1;
			i++;
		}
		y ++;
	}
}

//========================================================================
// 函数: void put_string(int x,int y,char *p)
// 描述: 在x、y为起始坐标处写入一串标准字符
// 参数: x  X轴坐标     y  Y轴坐标
//		 p  要显示的字符串  
// 返回: 无
// 备注: 仅能用于自带的ASCII字符串显示
// 版本:
//========================================================================
void put_string(int x,int y,char *p)
{
	while(*p!=0){
		put_char(x,y,*p);
		x += x_witch;
		if((x + x_witch) > DIS_X_MAX){
			x = dis_zero;
			if((DIS_Y_MAX - y) < y_witch) break;
			else y += y_witch;
		}
		p++;
	}
}
/*
//========================================================================
// 函数: void bit_map(unsigned int *p,int x,int y)
// 描述: 写入一个BMP图片,起点为(x,y)
// 参数:   
// 返回: 无
// 备注:
// 版本:
//========================================================================
void bit_map(unsigned int *p,int x,int y)
{
	int temp_with,temp_High,i,j;	//数据暂存
	temp_High = *(p ++);
	temp_with = *(p ++);			//图片宽度
	i = 0;
	while((i ++) < temp_High){
		j = 0;
		while((j ++) < temp_with){
			if(i < DIS_X_MAX) writ_dot(x+j,y+i,*p);
			else break;
		}
	}
}
*/
//========================================================================
// 函数: void set_paint_mode(int mode, unsigned int color)
// 描述: 绘图模式设置
// 参数: mode 绘图模式    color  像素点的颜色,相当于前景色  
// 返回: 无
// 备注: Mode无效
// 版本:
//      2006/10/15      First version
//========================================================================
void set_paint_mode(int mode, unsigned int color)
{
	plot_mode = mode;
	bmp_color = color;
}
//========================================================================
// 函数: void put_pixel(int x, int y)
// 描述: 在x、y点上绘制一个前景色的点
// 参数: x  X轴坐标     y  Y轴坐标
// 返回: 无
// 备注: 使用前景色
// 版本:
//========================================================================
void put_pixel(int x, int y)
{
	writ_dot(x, y, bmp_color);
}
//========================================================================
// 函数: void line_my(int s_x, int s_y, int e_x, int e_y)
// 描述: 在s_x、s_y为起始坐标，e_x、e_y为结束坐标绘制一条直线
// 参数: x  X轴坐标     y  Y轴坐标
// 返回: 无
// 备注: 使用前景色
// 版本:
//========================================================================
void line_my(int s_x, int s_y, int e_x, int e_y)
{  
	int offset_x,offset_y,offset_k = 0;
	int err_d = 1;
	if(s_y > e_y){
		offset_x = s_x;
		s_x = e_x;
		e_x = offset_x;
		offset_x = s_y;
		s_y = e_y;
		e_y = offset_x;
	}
	offset_x = e_x-s_x;
	offset_y = e_y-s_y;
	writ_dot(s_x, s_y, bmp_color);
	if(offset_x <= 0){
		offset_x = s_x-e_x;
		err_d = -1;
	}
	if(offset_x > offset_y){
		while(s_x != e_x){
			if(offset_k > 0){
				s_y += 1;
				offset_k += (offset_y-offset_x);
			}
			else offset_k += offset_y; 
			s_x += err_d;
			if(s_x>LCD_X_MAX || s_y>LCD_Y_MAX) break;
			writ_dot(s_x, s_y, bmp_color);
		}	
	}
	else{
		while(s_y != e_y){
			if(offset_k > 0){
				s_x += err_d;
				offset_k += (offset_x-offset_y);
			}
			else offset_k += offset_x;
			s_y += 1;
			if(s_x>=LCD_X_MAX || s_y>=LCD_Y_MAX) break;
			writ_dot(s_x, s_y, bmp_color);
		}
	}    
}
/*
//========================================================================
// 函数: void w_db_line(int *p)
// 描述: 画一个任意多边形
// 参数: p
// 返回: 该函数无效
// 备注: 使用前景色
// 版本:
//========================================================================
void w_db_line(int *p)
{
     int dot_sun,i;
     dot_sun = *p++;
     i = 0;
     while((i ++) < dot_sun)
     {
        W_line((*p >> 8)&0xff,*(p + 1)&0xff,(*(p + 1)>>8)&0xff,*(p + 1)&0xff);
        p += 1;
     }
}*/

//========================================================================
// 函数: void w_red_dot(int x,int y,int a,int b,int mode)
// 描述: 绘制圆的各个像限中的点和线
// 参数: 
// 返回: 无
// 备注: 该函数对用户不可见，使用前景色
// 版本:
//========================================================================
void line_f(int s_x,int s_y,int e_x);//,int e_y);
void w_red_dot(int x,int y,int a,int b,int mode)
{
	if(mode > 0){
		line_f(x+a, y+b, x-a);//,y+b);
		line_f(x+a, y-b, x-a);//,y-b);
	}
	else{
		put_pixel(x+a, y+b);
		put_pixel(x-a, y+b);
		put_pixel(x+a, y-b);
		put_pixel(x-a, y-b);
	}
}
//========================================================================
// 函数: void w_red_err(int *a,int *b,int *r)
// 描述: 画圆误差计算
// 参数: 
// 返回: 无
// 备注: 该函数对用户不可见
// 版本:
//========================================================================
void w_red_err(int *a,int *b,int *r)
{
	int r_error;
	unsigned int uitemp;
	
	r_error = (*a+1)*(*a+1);
	uitemp = (*b)*(*b);
	r_error += uitemp;
	uitemp = (*r)*(*r);
	r_error -= uitemp;
	if(r_error >= 0){
		r_error = r_error-*b;
		if(r_error >= 0) *b = *b-1;
	}
	*a = *a+1;
}
//========================================================================
// 函数: void circle(int x, int y, int r, int mode)
// 描述: 以x,y为圆心R为半径画一个圆(mode = 0) or 圆面(mode = 1)
// 参数: 
// 返回: 无
// 备注: 画圆函数执行较慢，如果MCU有看门狗，请作好清狗的操作
// 版本:
//      2006/10/16      First version
//========================================================================
void circle(unsigned int x, unsigned int y, int r, int mode)
{
	int arx1=0, ary1, arx2, ary2=0;
	
	pos_switch(&x, &y);						//坐标变换
	x += 4;
	ary1 = r;
	arx2 = r;
	while(1){
        w_red_dot(x, y, arx1, ary1, mode);
        w_red_err(&arx1, &ary1, &r);
		if(arx1 == arx2){
			w_red_dot(x, y, arx1, ary1, mode);
			break; 
		}
		w_red_dot(x, y, arx2, ary2, mode);
		w_red_err(&ary2, &arx2, &r);
		if(arx1 == arx2) {
			w_red_dot(x, y, arx2, ary2, mode);
			break;
		}
	}
}
//========================================================================
// 函数: void rectangle(unsigned left, unsigned top, unsigned right, 
//		 			 unsigned bottom, unsigned mode)
// 描述: 以x,y为圆心R为半径画一个圆(mode = 0) or 圆面(mode = 1)
// 参数: left - 矩形的左上角横坐标，范围0到118
//		 top - 矩形的左上角纵坐标，范围0到50
//		 right - 矩形的右下角横坐标，范围1到119
//		 bottom - 矩形的右下角纵坐标，范围1到51
//		 mode - 绘制模式，可以是下列数值之一：
//				0:	矩形框（空心矩形）
//				1:	矩形面（实心矩形）
// 返回: 无
// 备注: 画圆函数执行较慢，如果MCU有看门狗，请作好清狗的操作
// 版本:
//========================================================================
void rectangle(unsigned left, unsigned top, unsigned right, unsigned bottom, unsigned mode)
{
	unsigned uitemp;
	
	pos_switch(&left, &top);						//坐标变换
	pos_switch(&right, &bottom);					//坐标变换
	if(left > right){
		uitemp = left;
		left = right;
		right = uitemp;
	}
	if(top > bottom){
		uitemp = top;
		top = bottom;
		bottom = uitemp;
	}
	if(mode == 0){
		line_my(left, top, left, bottom);
		line_my(left, top, right, top);
		line_my(right, bottom, left+1, bottom);
		line_my(right, bottom, right, top+1);
	}
	else{
		for(uitemp=top; uitemp<=bottom; uitemp++){
			line_f(left, uitemp, right);//,uitemp);
		}
	}
}
//========================================================================
// 函数: void writ_dot(int x,int y,unsigned int color)
// 描述: 填充以x,y为坐标的象素
// 参数: x  X轴坐标     y  Y轴坐标      color  像素颜色 
// 返回: 无
// 备注: 这里以及之前的所有x和y坐标系都是用户层的，并不是实际LCD的坐标体系
//		 本函数提供可进行坐标变换的接口
// 版本:
//========================================================================
void writ_dot(int x, int y, unsigned int color)
{
#if	LCD_XY_SWITCH == 0
	#if (LCD_X_REV == 0)&&(LCD_Y_REV == 0)
		write_dot_lcd(x,y,color);
	#endif
	#if (LCD_X_REV == 1)&&(LCD_Y_REV == 0)
		write_dot_lcd(LCD_X_MAX - x,y,color);
	#endif
	#if (LCD_X_REV == 0)&&(LCD_Y_REV == 1)
		write_dot_lcd(x,LCD_Y_MAX - y,color);
	#endif
	#if (LCD_X_REV == 1)&&(LCD_Y_REV == 1)
		write_dot_lcd(LCD_X_MAX - x,LCD_Y_MAX - y,color);
	#endif
#endif
#if	LCD_XY_SWITCH == 1
	#if (LCD_X_REV == 0)&&(LCD_Y_REV == 0)
		write_dot_lcd(y,x,color);
	#endif
	#if (LCD_X_REV == 1)&&(LCD_Y_REV == 0)
		write_dot_lcd(y,LCD_Y_MAX - x,color);
	#endif
	#if (LCD_X_REV == 0)&&(LCD_Y_REV == 1)
		write_dot_lcd(LCD_X_MAX - y,x,color);
	#endif
	#if (LCD_X_REV == 1)&&(LCD_Y_REV == 1)
		write_dot_lcd(LCD_X_MAX - y,LCD_Y_MAX - x,color);
	#endif
#endif	
}
//========================================================================
// 函数: unsigned int get_dot(int x,int y)
// 描述: 获取x,y为坐标的象素
// 参数: x  X轴坐标     y  Y轴坐标      
// 返回: color  像素颜色 
// 备注: 这里以及之前的所有x和y坐标系都是用户层的，并不是实际LCD的坐标体系
//		 本函数提供可进行坐标变换的接口
// 版本:
//========================================================================
unsigned int get_dot(int x,int y)
{
#if	LCD_XY_SWITCH == 0
	#if (LCD_X_REV == 0)&&(LCD_Y_REV == 0)
		return get_dot_lcd(x,y);
	#endif
	#if (LCD_X_REV == 1)&&(LCD_Y_REV == 0)
		return get_dot_lcd(LCD_X_MAX - x,y);
	#endif
	#if (LCD_X_REV == 0)&&(LCD_Y_REV == 1)
		return get_dot_lcd(x,LCD_Y_MAX - y);
	#endif
	#if (LCD_X_REV == 1)&&(LCD_Y_REV == 1)
		return get_dot_lcd(LCD_X_MAX - x,LCD_Y_MAX - y);
	#endif
#endif
#if	LCD_XY_SWITCH == 1
	#if (LCD_X_REV == 0)&&(LCD_Y_REV == 0)
		return get_dot_lcd(y,x);
	#endif
	#if (LCD_X_REV == 1)&&(LCD_Y_REV == 0)
		return get_dot_lcd(y,LCD_Y_MAX - x);
	#endif
	#if (LCD_X_REV == 0)&&(LCD_Y_REV == 1)
		return get_dot_lcd(LCD_X_MAX - y,x);
	#endif
	#if (LCD_X_REV == 1)&&(LCD_Y_REV == 1)
		return get_dot_lcd(LCD_X_MAX - y,LCD_Y_MAX - x);
	#endif
#endif	
}

//========================================================================
// 函数: void clear_dot(int x,int y)
// 描述: 清除以x,y为坐标的象素
// 参数: x  X轴坐标     y  Y轴坐标      
// 返回: 无 
// 备注: 这里以及之前的所有x和y坐标系都是用户层的，并不是实际LCD的坐标体系
//		 本函数提供可进行坐标变换的接口
// 版本:
//========================================================================
void clear_dot(int x,int y)
{
#if	LCD_XY_SWITCH == 0
	#if (LCD_X_REV == 0)&&(LCD_Y_REV == 0)
		clear_dot_lcd(x,y);
	#endif
	#if (LCD_X_REV == 1)&&(LCD_Y_REV == 0)
		clear_dot_lcd(LCD_X_MAX - x,y);
	#endif
	#if (LCD_X_REV == 0)&&(LCD_Y_REV == 1)
		clear_dot_lcd(x,LCD_Y_MAX - y);
	#endif
	#if (LCD_X_REV == 1)&&(LCD_Y_REV == 1)
		clear_dot_lcd(LCD_X_MAX - x,LCD_Y_MAX - y);
	#endif
#endif
#if	LCD_XY_SWITCH == 1
	#if (LCD_X_REV == 0)&&(LCD_Y_REV == 0)
		clear_dot_lcd(y,x);
	#endif
	#if (LCD_X_REV == 1)&&(LCD_Y_REV == 0)
		clear_dot_lcd(y,LCD_Y_MAX - x);
	#endif
	#if (LCD_X_REV == 0)&&(LCD_Y_REV == 1)
		clear_dot_lcd(LCD_X_MAX - y,x);
	#endif
	#if (LCD_X_REV == 1)&&(LCD_Y_REV == 1)
		clear_dot_lcd(LCD_X_MAX - y,LCD_Y_MAX - x);
	#endif
#endif	
}
//========================================================================
// 函数: void set_dot_addr(int x,int y)
// 描述: 设置当前需要操作的象素地址
// 参数: x  X轴坐标     y  Y轴坐标      
// 返回: 无 
// 备注: 这里以及之前的所有x和y坐标系都是用户层的，并不是实际LCD的坐标体系
//		 本函数提供可进行坐标变换的接口
// 版本:
//========================================================================
void set_dot_addr(int x,int y)
{
#if	LCD_XY_SWITCH == 0
	#if (LCD_X_REV == 0)&&(LCD_Y_REV == 0)
		set_dot_addr_lcd(x,y);
	#endif
	#if (LCD_X_REV == 1)&&(LCD_Y_REV == 0)
		set_dot_addr_lcd(LCD_X_MAX - x,y);
	#endif
	#if (LCD_X_REV == 0)&&(LCD_Y_REV == 1)
		set_dot_addr_lcd(x,LCD_Y_MAX - y);
	#endif
	#if (LCD_X_REV == 1)&&(LCD_Y_REV == 1)
		set_dot_addr_lcd(LCD_X_MAX - x,LCD_Y_MAX - y);
	#endif
#endif
#if	LCD_XY_SWITCH == 1
	#if (LCD_X_REV == 0)&&(LCD_Y_REV == 0)
		set_dot_addr_lcd(y,x);
	#endif
	#if (LCD_X_REV == 1)&&(LCD_Y_REV == 0)
		set_dot_addr_lcd(y,LCD_Y_MAX - x);
	#endif
	#if (LCD_X_REV == 0)&&(LCD_Y_REV == 1)
		set_dot_addr_lcd(LCD_X_MAX - y,x);
	#endif
	#if (LCD_X_REV == 1)&&(LCD_Y_REV == 1)
		set_dot_addr_lcd(LCD_X_MAX - y,LCD_Y_MAX - x);
	#endif
#endif		
}
//========================================================================
// 函数: void pos_switch(unsigned int * x,unsigned int * y)
// 描述: 将画面的坐标变换为实际LCD的坐标体系，以便于快速画圆形以及矩形
// 参数: x  X轴坐标     y  Y轴坐标      
// 返回: 无 
// 备注: 这里以及之前的所有x和y坐标系都是用户层的，并不是实际LCD的坐标体系
//		 本函数提供可进行坐标变换的接口
// 版本:
//========================================================================
void pos_switch(unsigned int * x,unsigned int * y)
{
	*x = *x;
	*y = *y;
#if	LCD_XY_SWITCH == 0
	#if (LCD_X_REV == 0)&&(LCD_Y_REV == 0)
	#endif
	#if (LCD_X_REV == 1)&&(LCD_Y_REV == 0)
		*x = LCD_X_MAX-*x;
	#endif
	#if (LCD_X_REV == 0)&&(LCD_Y_REV == 1)
		*y = LCD_Y_MAX-*y;
	#endif
	#if (LCD_X_REV == 1)&&(LCD_Y_REV == 1)
		*x = LCD_X_MAX-*x;
		*y = LCD_Y_MAX-*y;
	#endif
#endif
#if	LCD_XY_SWITCH == 1
	unsigned int uitemp;
	#if (LCD_X_REV == 0)&&(LCD_Y_REV == 0)
		uitemp = *x;
		*x = y;
		*y = uitemp;
	#endif
	#if (LCD_X_REV == 1)&&(LCD_Y_REV == 0)
		uitemp = LCD_Y_MAX-*x;
		*x = *y;
		*y = uitemp;
	#endif
	#if (LCD_X_REV == 0)&&(LCD_Y_REV == 1)
		uitemp = *x;
		*x = LCD_X_MAX-*y;
		*y = uitemp;
	#endif
	#if (LCD_X_REV == 1)&&(LCD_Y_REV == 1)
		uitemp = LCD_Y_MAX - *x;
		*x = LCD_X_MAX-*y;
		*y = uitemp;
	#endif
#endif
}
//========================================================================
// 函数: void line_f(int s_x,int s_y,int e_x,int e_y,int mode)
// 描述: 以s_x,s_y为起点,e_x,e_y为终点连续填充一条直线上的点.用于画矩形、圆
// 参数: x  X轴坐标     y  Y轴坐标      
// 返回: 无 
// 备注: 以实际的LCD坐标体系为准
// 版本:
//      2006/10/16      First version
//========================================================================
void line_f(int s_x,int s_y,int e_x)//,int e_y)
{  
	unsigned int uitemp;
	if(s_x>e_x) 
	{
		uitemp = s_x;
		s_x = e_x;
		e_x = uitemp;
	}
	write_dot_lcd(s_x++,s_y,bmp_color);
	for(;s_x<=e_x;s_x++)
		fill_dot_lcd(bmp_color);
}
//========================================================================
// 函数: void clr_screen(void)
// 描述: 全屏以初始化屏幕色的颜色进行清屏
// 参数: 无      
// 返回: 无 
// 备注: 无
// 版本:
//      2007/01/16      First version
//========================================================================
void clr_screen(void)
{  
	set_dot_addr_lcd(0,0);
	lcd_fill(LCD_INITIAL_COLOR);
}
