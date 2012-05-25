
/* LCD Register define */
#define LCD_REG_BASE	0x1f001000
#define REG_LCD_CTRL	0x00
#define REG_LCD_STAT	0x04
#define REG_LCD_HTIM	0x08
#define REG_LCD_VTIM	0x0c
#define REG_LCD_HVLEN	0x10
#define REG_LCD_VBARa	0x14
#define REG_LCD_VBARb	0x18
#define REG_LCD_C0XY	0x30
#define REG_LCD_C0BAR	0x34
#define REG_LCD_C0CR	0x40
#define REG_LCD_C1XY	0x70
#define REG_LCD_C1BAR	0x74
#define REG_LCD_CICR	0x80
#define REG_LCD_PCLT	0x800

/* LCD relative Address */
//#define SD_LCD_BARA_BASE	0x60000 
#define SD_LCD_BARA_SIZE	0x4b000
//#define SD_LCD_CLUT_BASE	0xab000
#define SD_LCD_CLUT_SIZE	0x08000
//#define SD_LCD_BARB_BASE	0xb0000
#define SD_LCD_BARB_SIZE	0x4b000

/* REG_LCD_STAT */
#define LCD_NO_DATA_FOLLOW	0x02
#define LCD_FRAME_SWITCH	0x40
#define LCD_FRAME_A		0x10000

/* REG_LCD_CTRL */
#define LCD_INT_RESET		0x20

/* Common define */
#define LCD_SET32(idx, value) KSEG1_STORE32(LCD_REG_BASE + idx, value)
#define LCD_GET32(idx) KSEG1_LOAD32(LCD_REG_BASE + idx)

#define SD_LCD_BAR_BASE KSEG1(__SD_LCD_BAR_BASE)
#define SD_LCD_BAR_BASE_2 KSEG1(SD_LCD_BAR_dum_BASE)
extern unsigned char __SD_LCD_BAR_BASE[];
extern unsigned char SD_LCD_BAR_dum_BASE[];
