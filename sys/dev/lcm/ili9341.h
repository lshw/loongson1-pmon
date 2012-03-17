#ifndef	__ILI9341_h__
#define	__ILI9341_h__
//	write your header here
#define LCD_X_MAX			240-1			//屏幕的X轴的物理宽度
#define LCD_Y_MAX			320-1			//屏幕的Y轴的物理宽度
			
#define LCD_XY_SWITCH		0				//显示时X轴和Y由交换
#define LCD_X_REV			0				//显示时X轴反转
#define LCD_Y_REV			0				//显示时Y轴反转

#if LCD_XY_SWITCH == 0
	#define DIS_X_MAX		LCD_X_MAX
	#define DIS_Y_MAX		LCD_Y_MAX	
#endif

#if LCD_XY_SWITCH == 1
	#define DIS_X_MAX		LCD_Y_MAX
	#define DIS_Y_MAX		LCD_X_MAX	
#endif

#define LCD_INITIAL_COLOR	0x0000			//定义LCD屏初始化时的背景色

#endif
