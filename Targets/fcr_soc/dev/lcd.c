#include <pmon.h>
#include <dev/pci/pcivar.h>
#include <target/types.h>
#include <target/lcd.h>
#define HMAX 640    //z 320  384
#define VMAX 480    //z 256
#define HREAL 480   //z 240  384
/* FRAME Buffer */
unsigned char __SD_LCD_BAR_BASE[VMAX*HMAX*2] __attribute__ ((aligned(64),section(".bss.align64")));
static int count = 0;

void init_lcd_regs();
void init_lcd()
{
	pci_sync_cache(0, (vm_offset_t)__SD_LCD_BAR_BASE, sizeof(__SD_LCD_BAR_BASE), SYNC_R);
    memset(SD_LCD_BAR_BASE, 0x00, sizeof(__SD_LCD_BAR_BASE));
    init_lcd_regs();
}

static void setdisplaymode(unsigned long arg);
void init_lcd_regs()
{
#if 0
 int i=0;
 int thsync=0x5f;
 int thdel=0xf;
 int tvsync=0x1;
 int tvdel=0x9;
 int burst=2; //8:7 VBL(video burst length) 11=64cycle, 10=8cycle, 01=4cycle 00=1cycle
 int pseudocolor=0;
 int colordepth=3; //10:9 CD(color depth) 11=16bit, 10=4b, 01=2b, 00=1b
 int xres=640,yres=480;
    LCD_SET32(REG_LCD_VBARa, PHY(SD_LCD_BAR_BASE));
    LCD_SET32(REG_LCD_VBARb, PHY(SD_LCD_BAR_BASE));
	LCD_SET32(REG_LCD_CTRL,0);
    LCD_SET32(REG_LCD_HTIM,(thsync<<24)|(thdel<<16)|(xres-1));
    LCD_SET32(REG_LCD_VTIM,(tvsync<<24)|(tvdel<<16)|(yres-1));
    LCD_SET32(REG_LCD_HVLEN, ((xres+thsync+thdel+0x31)<<16)|(yres+tvsync+tvdel+0x22));
#if 0 // CMC STN 320x240
    LCD_SET32(REG_LCD_CTRL, 0x00000009|(burst<<7)|(pseudocolor<<11)|(colordepth<<9));
#else // PrimeView TFT 800x600
    LCD_SET32(REG_LCD_CTRL, 0x00008009|(burst<<7)|(pseudocolor<<11)|(colordepth<<9));
#endif
    for(i=0;i<0x800;i=i+4) // fill clut
        LCD_SET32((REG_LCD_PCLT+i), 0x0003ffff);
#else
    LCD_SET32(REG_LCD_VBARa, PHY(SD_LCD_BAR_BASE));
    LCD_SET32(REG_LCD_VBARb, PHY(SD_LCD_BAR_BASE));
setdisplaymode(DISPLAY_VGA_1024X768);
#endif
}


static  const  struct gc_videomode xwinmode[] = {
	{
		/* LCD  truly 320x240 @  Hz   0*/
		/*name,refresh,xres,yres,pclk, thsync,tvsync,thdel,tvdel,thfp,tvfp, flag*/
		NULL, 60,    320, 240, 8 ,    95,    1,     15,   9,    19,  3,  0x00007009
	},{
		/* LCD  sp  320x240 @  Hz  1 */
		/*name,refresh,xres,yres,pclk, thsync,tvsync,thdel,tvdel,thfp,tvfp, flag*/
		NULL, 60,    320, 240, 5 ,     95,   1,     15,   9,    49,  34,  0x00008009
	}, {
		/* BYD   320x240 @  Hz      2*/
		/*name,refresh,xres,yres,pclk, thsync,tvsync,thdel,tvdel,thfp,tvfp, flag*/
		NULL, 60,    320, 240, 8 ,    95,     0,    1,   15,     9,    3,  0x00007009
			//	NULL, 60,    320, 240, 8 ,    95,     0,    67,   17,     19,    3,  0x00007009
	},{
		/* lcd  640x480 @ 60 Hz  3*/
		/*name,refresh,xres,yres,pclk, thsync,tvsync,thdel,tvdel,thfp,tvfp, flag*/
		NULL, 60,    640, 480, 25,       95,   1,       47,    30,    15,   10,   0x0000c009
	},{
		/* vga  640x480 @ 60 Hz  4*/
	//	NULL, 60,    640, 480, 25,       95,   1,       47,    30,    15,   10,   0x0000c009
		/*name,refresh,xres,yres,pclk, thsync,tvsync,thdel,tvdel,thfp,tvfp, flag*/
		NULL, 60,    640, 480,   25,    135,   5,    1,    28,    23,   2,   0x0000c009
	}, {
		/* LCD  800x600 @ 60 Hz, 5*/
		/*name,refresh,xres,yres,pclk, thsync,tvsync,thdel,tvdel,thfp,tvfp, flag*/
		NULL, 60,    800, 600, 40,    135,   5,    50,    28,    23,   2,   0x0000c009
			//NULL, 60,    800, 600, 40,    127,   3,     87,    22,  39,  0,   0x0000c009
	}, {
		/* vga  800x600 @ 60 Hz      6 */
		/*name,refresh,xres,yres,pclk, thsync,tvsync,thdel,tvdel,thfp,tvfp, flag*/
		NULL, 60,    800, 600, 40,    135,   5,    50,    28,    23,   2,   0x0000c009
			//NULL, 60,    800, 600, 40,    127,   3,     87,    22,  39,  0,   0x0000c009
	}, { 
		/* vga  1024x768 @ 60 Hz,    7*/
		/*name,refresh,xres,yres,pclk, thsync,tvsync,thdel,tvdel,thfp,tvfp, flag*/
		NULL, 60,    1024, 768, 62.5,    145,   5,   100,   28,  23,   2,   0x0000c009
	},  { 
		/* 1024x768 @ 60 Hz,         8*/
		NULL, 60,    1024, 768, 50,    135,   5,    159,    28,    23,   2,   0x0000c009
	},  {
		/*  @ 60 Hz, 31.5 kHz hsync, 4:3 aspect ratio  9 */
		/*name,refresh,xres,yres,pclk, thsync,tvsync,thdel,tvdel,thfp,tvfp, flag*/
		NULL, 60,    1280, 960,   83.33,  89,   1,    55,    1,    17,   1,   0x0000c009
			//NULL, 60,    800, 600, 40,    127,   3,     87,    22,  39,  0,   0x0000c009
	},{
		/* 1280x800 @ 60 Hz, 31.5 kHz hsync, 4:3 aspect ratio a  */
		/*name,refresh, xres,yres,pclk,   thsync,tvsync,thdel,tvdel,thfp,tvfp, flag*/
	//	NULL, 60,    1280, 800,   83.33,    135,   5,    159,   35,    23,   2,   0x0000c009
		NULL, 60,    1280, 800,   83.33,    135,   15,    159,   35,    23,   19,   0x0000c009
	//	NULL, 60,    1280, 720,   83.33,    29,   5,    15,    4,    23,   2,   0x0000c009
	//	NULL, 60,    1280, 800,   83.33,    49,   3,    15,    4,    23,   2,   0x0000c009
			//NULL, 60,    800, 600, 40,    127,   3,     87,    22,  39,  0,   0x0000c009
	},{
			/* 1280x1024 @ 60 Hz, 31.5 kHz hsync, 4:3 aspect ratio b  */
		/*name,refresh, xres,yres,pclk,   thsync,tvsync,thdel,tvdel,thfp,tvfp, flag*/
		NULL, 60,    1280, 720,   62.5,   69,   5,      49,    28,    23,   2,   0x0000c009
	//	NULL, 60,    1280, 800,   83.33,    49,   3,    15,    4,    23,   2,   0x0000c009
			//NULL, 60,    800, 600, 40,    127,   3,     87,    22,  39,  0,   0x0000c009
	},{
			/* 1280x960 @ 60 Hz, 31.5 kHz hsync, 4:3 aspect ratio c  */
		/*name,refresh, xres,yres,pclk,   thsync,tvsync,thdel,tvdel,thfp,tvfp, flag*/
		NULL, 60,    1280, 760,   62.5,   69,   5,      49,    28,    23,   2,   0x0000c009
	//	NULL, 60,    1280, 800,   83.33,    49,   3,    15,    4,    23,   2,   0x0000c009
			//NULL, 60,    800, 600, 40,    127,   3,     87,    22,  39,  0,   0x0000c009
	},{
			/* 800x480 @ 60 Hz, 31.5 kHz hsync, 4:3 aspect ratio d  */
		/*name,refresh, xres,yres,pclk,   thsync,tvsync,thdel,tvdel,thfp,tvfp, flag*/
		NULL, 60,    800, 480,   40,       1,   1,       44,    21,   209,   131,   0x00008009
	//	NULL, 60,    1280, 800,   83.33,    49,   3,    15,    4,    23,   2,   0x0000c009
			//NULL, 60,    800, 600, 40,    127,   3,     87,    22,  39,  0,   0x0000c009
	},  {
			/* 480x272 @ 60 Hz, 31.5 kHz hsync, 4:3 aspect ratio e  */
		/*name,refresh, xres,yres,pclk,   thsync,tvsync,thdel,tvdel,thfp,tvfp, flag*/
		NULL, 60,    480, 272,   10,       40,   9,       1,    1,   3,   3,   0x0000c009
	//	NULL, 60,    480, 272,   10,       41,   10,       2,    2,   1,   1,   0x00007009
	//	NULL, 60,    1280, 800,   83.33,    49,   3,    15,    4,    23,   2,   0x0000c009
			//NULL, 60,    800, 600, 40,    127,   3,     87,    22,  39,  0,   0x0000c009
	},


	
	{

	}
};

static void setdisplaymode(unsigned long arg)
{
	int burst=2; //8:7 VBL(video burst length) 11=16cycle, 10=8cycle, 01=4cycle 00=1cycle
	int pseudocolor=0;
	int colordepth=3; //10:9 CD(color depth) 11=16bit, 10=4b, 01=2b, 00=1b
	//close
#if 0
	DPRINTK("MODE=0x%x\n",arg);
	prom_printf("xwinmode[arg].pixclock=%d\n",xwinmode[arg].pixclock);
	prom_printf("xwinmode[arg].thsync=%d\n",xwinmode[arg].thsync);
	prom_printf("xwinmode[arg].tvsync=%d\n",xwinmode[arg].tvsync);
	prom_printf("xwinmode[arg].xres=%d\n",xwinmode[arg].xres);
	prom_printf("xwinmode[arg].yres=%d\n",xwinmode[arg].yres);

	prom_printf("REG_LCD_CTRL=0x%x\n", LCD_GET32(REG_LCD_CTRL));
	prom_printf("REG_LCD_HTIM=0x%x\n", LCD_GET32(REG_LCD_HTIM));
	prom_printf("REG_LCD_VTIM=0x%x\n", LCD_GET32(REG_LCD_VTIM));
	prom_printf("REG_LCD_HVLEN=0x%x\n", LCD_GET32(REG_LCD_HVLEN));
	prom_printf("LCD_CLK=0x%x\n",  LCD_CLK_GET32(0));
#endif

	if(CPU_CLK>=200000000 && CPU_CLK<=250000000)
	LCD_CLK_SET32(0, ((((250000000/1000000/xwinmode[arg].pixclock)-1)<<9)+1)|(LCD_CLK_GET32(0)&(~(0x7f<<9))));
	else 
	LCD_CLK_SET32(0, ((((CPU_CLK/1000000/xwinmode[arg].pixclock)-1)<<9)+1)|(LCD_CLK_GET32(0)&(~(0x7f<<9))));
	//LCD_CLK_SET32(0, ((3<<9)+1)|(LCD_CLK_GET32(0)&(~(0x7f<<9))));
	LCD_SET32(REG_LCD_CTRL, (LCD_GET32(REG_LCD_CTRL) & 0xFFFE));//hwm  ven=0
	LCD_SET32(REG_LCD_HTIM,(xwinmode[arg].thsync<<24)|(xwinmode[arg].thdel<<16)|(xwinmode[arg].xres-1));  
	LCD_SET32(REG_LCD_VTIM,(xwinmode[arg].tvsync<<24)|(xwinmode[arg].tvdel<<16)|(xwinmode[arg].yres-1));  
	LCD_SET32(REG_LCD_HVLEN, ((xwinmode[arg].xres+xwinmode[arg].thsync+xwinmode[arg].thdel+xwinmode[arg].thfp)<<16)|(xwinmode[arg].yres+xwinmode[arg].tvsync+xwinmode[arg].tvdel+xwinmode[arg].tvfp)); 
	// LCD_SET32(REG_LCD_CTRL, 0x00008009|(burst<<7)|(pseudocolor<<11)|(colordepth<<9)); 
	// LCD_SET32(REG_LCD_CTRL, (LCD_GET32(REG_LCD_CTRL)|0x1));//hwm  ven=1
	LCD_SET32(REG_LCD_CTRL, (xwinmode[arg].flag)|(burst<<7)|(pseudocolor<<11)|(colordepth<<9));

#if 0
	prom_printf("REG_LCD_CTRL=0x%x\n", LCD_GET32(REG_LCD_CTRL));
	prom_printf("REG_LCD_HTIM=0x%x\n", LCD_GET32(REG_LCD_HTIM));
	prom_printf("REG_LCD_VTIM=0x%x\n", LCD_GET32(REG_LCD_VTIM));
	prom_printf("REG_LCD_HVLEN=0x%x\n", LCD_GET32(REG_LCD_HVLEN));
	prom_printf("LCD_CLK=0x%x\n",  LCD_CLK_GET32(0));
#endif
}

static int setdisplay(int argc,char **argv)
{
 setdisplaymode(strtoul(argv[1],0,0));
 return 0;
}

static int setdisplayaddr(int argc,char **argv)
{
 unsigned long addr=strtoul(argv[1],0,0);
    LCD_SET32(REG_LCD_VBARa, /*PHY(SD_LCD_BAR_BASE)*/addr);
    LCD_SET32(REG_LCD_VBARb, /*PHY(SD_LCD_BAR_BASE)*/addr);
 return 0;
}

static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"setdisplay","",0,"setdisplay",setdisplay,0,99,CMD_REPEAT},
	{"setdisplayaddr","",0,"setdisplayaddr",setdisplayaddr,0,99,CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

