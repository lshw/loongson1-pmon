#ifndef	__ILI9341_LCD_DIS_h__
#define	__ILI9341_LCD_DIS_h__
//	write your header here
//LCD 初始化函数，一般用于LCD屏的寄存器初始化
//void LCD_Init(void);	//在LCD_Driver_User.c中定义的函数，如有必要该函数还会包含端口初始化、复位等操作

//画直线函数，s_x、s_y为起始点，e_x、e_y为结束点
void line_my(int s_x, int s_y, int e_x, int e_y);
//绘图模式、当前绘图颜色设置
void set_paint_mode(int mode, unsigned int color);
//标准字符设置，包括两型号ASCII码的大小尺寸，以及字体颜色的设置
void font_set(int font_num, unsigned int color);
//于x、y的坐标上写入一个标准字符
void put_char(int x, int y, unsigned int a);
//于x、y的坐标为起始写入一串标准字符串
void put_string(int x, int y, char *p);
//于x、y的坐标为中心，绘制一个圆边或实心圆
void put_pixel(int x, int y);
void circle(unsigned int x, unsigned int y, int r, int mode);
//绘制一个以left、top和right、bottom为两个对角的矩形框或者实心矩形
void rectangle(unsigned left, unsigned top, unsigned right, unsigned bottom, unsigned mode);
//全屏以初始化屏幕色的颜色进行清屏,初始化屏幕色在LCD_Config.h当中定义“LCD_INITIAL_COLOR”
void clr_screen(void);

//以下函数以及变量的声明一般建议用户不要调用，仅供高级用户在自行编写特殊显示效果以及特性应用程序时使用
//
//extern unsigned char code Asii16[];		//8X16的ASII字符库
//extern unsigned char code GB32[];		//自定义的32X29汉字库
//extern unsigned char code GB48[];		//自定义的48X55汉字库

// int x_witch;					//字符写入时的宽度
// int y_witch;					//字符写入时的高度
// int font_wrod;				//字体的大
// unsigned char *char_tab;		//字库指针
// int plot_mode;				//绘图模式
// unsigned int bmp_color;
// unsigned int char_color;

void pos_switch(unsigned int * x, unsigned int * y);
void writ_dot(int x, int y, unsigned int color);
void line_f(int s_x, int s_y, int e_x);//,int e_y);
unsigned int get_dot(int x, int y);
void clear_dot(int x, int y);
void set_dot_addr(int x, int y);

#endif
