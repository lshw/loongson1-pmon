#include <pmon.h>
#include <dev/pci/pcivar.h>
#include <target/types.h>
#include <target/lcd.h>
#define HMAX 640    //z 320  384
#define VMAX 480    //z 256
#define HREAL 480   //z 240  384
/* FRAME Buffer */
unsigned char __SD_LCD_BAR_BASE[VMAX*HMAX*2] __attribute__ ((aligned(64)));
static int count = 0;

void init_lcd_regs(void) ;
void init_lcd()
{
	pci_sync_cache(0, (vm_offset_t)__SD_LCD_BAR_BASE, sizeof(__SD_LCD_BAR_BASE), SYNC_R);
    memset(SD_LCD_BAR_BASE, 0x00, sizeof(__SD_LCD_BAR_BASE));
    init_lcd_regs();
}

void init_lcd_regs()
{
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
}
