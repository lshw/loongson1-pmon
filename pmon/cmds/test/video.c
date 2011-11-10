#include <fb/video_font.h>
#include <string.h>

extern void video_fill(int color);
extern void video_drawstring (int xx, int yy, unsigned char *s);
extern char *vgabh;
extern char *heaptop;
char *color[]={"Blue","Green","Cyan","Red","Magenta","Brown","White","Gray","Light Blue","Light Green","Light cyan","Light red","Light Magenta","yellow","Bright White"};
void cprintf(int y, int x,int width,char color,const char *fmt,...);

static void blacklight_test(void)
{
	int i;
	
	*(volatile int *)0xbfd010c0 |= 0x01;		//GPIO0使能
	*(volatile int *)0xbfd010d0 &= ~(0x01);	//GPIO0输出使能
	*(volatile int *)0xbfd010f0 |= 0x01;		//GPIO0输出1 使能LCD背光
	
	for(i=0; i<6; i++){
		if(*(volatile int *)0xbfd010f0 & 0x01)
			*(volatile int *)0xbfd010f0 &= ~(0x01);
		else
			*(volatile int *)0xbfd010f0 |= 0x01;
		delay(300000);
	}
	*(volatile int *)0xbfd010f0 |= 0x01;		//GPIO0输出1 使能LCD背光
}

static int videotest(void)
{
//	for(i=0; i<0xffff; i++)
#if defined(CONFIG_VIDEO_16BPP)
		//RGB565
		video_fill(0xFFFFFFFF);
//		video_drawstring(8, 16, "White  White  White");
	#ifdef CONFIG_CHINESE
		printf("白色\n");
	#else
		printf("White\n");
	#endif
		delay(3000000);
		video_fill(0x001F001F);
//		video_drawstring(8, 16, "Blue  Blue  Blue");
	#ifdef CONFIG_CHINESE
		printf("蓝色\n");
	#else
		printf("Blue\n");
	#endif
		delay(3000000);
		video_fill(0x07E007E0);
//		video_drawstring(8, 16, "Green  Green  Green");
	#ifdef CONFIG_CHINESE
		printf("绿色\n");
	#else
		printf("Green\n");
	#endif
		delay(3000000);
		video_fill(0xF800F800);
//		video_drawstring(8, 16, "Red  Red  Red");
	#ifdef CONFIG_CHINESE
		printf("红色\n");
	#else
		printf("Red\n");
	#endif
		delay(3000000);
		video_fill(0x07FF07FF);
//		video_drawstring(8, 16, "cyan  cyan  cyan");
	#ifdef CONFIG_CHINESE
		printf("青色\n");
	#else
		printf("cyan\n");
	#endif
		delay(3000000);
		video_fill(0xF81FF81F);
//		video_drawstring(8, 16, "purple  purple  purple");
	#ifdef CONFIG_CHINESE
		printf("紫色\n");
	#else
		printf("purple\n");
	#endif
		delay(3000000);
		video_fill(0xFFE0FFE0);
//		video_drawstring(8, 16, "yellow  yellow  yellow");
	#ifdef CONFIG_CHINESE
		printf("黄色\n");
	#else
		printf("yellow\n");
	#endif
		delay(3000000);
//		video_display_bitmap(0xbfc80000, 0, 0);
//		video_drawstring(0, 0, "1234567890 abcdefghijklmnopqrstuvwxyz ~!@#$%^&*()_+{}|:""<>?,./;'[]\-= Loongson");
#elif defined(CONFIG_VIDEO_32BPP)
		//RGB888
		video_fill(0xFFFFFFFF);
//		video_drawstring(8, 16, "White  White  White");
	#ifdef CONFIG_CHINESE
		printf("白色\n");
	#else
		printf("White\n");
	#endif
		delay(3000000);
		video_fill(0xFF0000FF);
//		video_drawstring(8, 16, "Blue  Blue  Blue");
	#ifdef CONFIG_CHINESE
		printf("蓝色\n");
	#else
		printf("Blue\n");
	#endif
		delay(3000000);
		video_fill(0x0000FF00);
//		video_drawstring(8, 16, "Green  Green  Green");
	#ifdef CONFIG_CHINESE
		printf("绿色\n");
	#else
		printf("Green\n");
	#endif
		delay(3000000);
		video_fill(0x00FF0000);
//		video_drawstring(8, 16, "Red  Red  Red");
	#ifdef CONFIG_CHINESE
		printf("红色\n");
	#else
		printf("Red\n");
	#endif
		delay(3000000);
		video_fill(0x0000FFFF);
//		video_drawstring(8, 16, "cyan  cyan  cyan");
	#ifdef CONFIG_CHINESE
		printf("青色\n");
	#else
		printf("cyan\n");
	#endif
		delay(3000000);
		video_fill(0x00FF00FF);
//		video_drawstring(8, 16, "purple  purple  purple");
	#ifdef CONFIG_CHINESE
		printf("紫色\n");
	#else
		printf("purple\n");
	#endif
		delay(3000000);
		video_fill(0x00FFFF00);
//		video_drawstring(8, 16, "yellow  yellow  yellow");
	#ifdef CONFIG_CHINESE
		printf("黄色\n");
	#else
		printf("yellow\n");
	#endif
		delay(3000000);
#else
#endif
		video_fill(0x00000000);
		video_drawstring(0, 0, "1234567890 abcdefghijklmnopqrstuvwxyz");
		video_drawstring(0, 16, "~!@#$%^&*()_+{}|:""<>?,./;'[]= Loongson");
		delay(3000000);
	#if (CONFIG_COMMANDS & CFG_CMD_BMP) || defined(CONFIG_SPLASH_SCREEN)
		video_display_bitmap(0xbfc80000, 0, 0);
		delay(3000000);
	#endif
		//背光控制测试
		blacklight_test();
		
	return 0;
}

