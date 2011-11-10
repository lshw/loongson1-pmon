/*
 * Copyright: 
 * ----------------------------------------------------------------
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 *   (C) COPYRIGHT 2003,2004 ARM Limited
 *       ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 * ----------------------------------------------------------------
 * File:     apclcd.h
 * Release:  Version 2.0
 * ----------------------------------------------------------------
 * 
 *  ----------------------------------------
 *  Version and Release Control Information:
 * 
 *  File Name              : $RCSfile: display.h,v $
 *  File Revision          : $Revision: 1.1.2.1 $
 * 
 *  ----------------------------------------
 *  
 *            CLCD Support
 *            ============
 */

// System Control registers
#define SYS_BASE        0xBC200000
#define SYS_ID          ((volatile unsigned int *)(SYS_BASE + 0x00))
#define SYS_SW          ((volatile unsigned int *)(SYS_BASE + 0x04))
#define SYS_LED         ((volatile unsigned int *)(SYS_BASE + 0x08))
#define SYS_OSC0        ((volatile unsigned int *)(SYS_BASE + 0x0C))
#define SYS_OSC1        ((volatile unsigned int *)(SYS_BASE + 0x10))
#define SYS_OSC2        ((volatile unsigned int *)(SYS_BASE + 0x14))
#define SYS_OSC3        ((volatile unsigned int *)(SYS_BASE + 0x18))
#define SYS_OSC4        ((volatile unsigned int *)(SYS_BASE + 0x1C))
#define SYS_LOCK        ((volatile unsigned int *)(SYS_BASE + 0x20))
#define SYS_100HZ       ((volatile unsigned int *)(SYS_BASE + 0x24))
//#define SYS_FLAGS       ((volatile unsigned int *)(SYS_BASE + 0x30)) /* Has not been mapped as of 08/07 */
//#define SYS_FLAGSSET    ((volatile unsigned int *)(SYS_BASE + 0x30)) /* Has not been mapped as of 08/07 */
//#define SYS_FLAGSCLR    ((volatile unsigned int *)(SYS_BASE + 0x34)) /* Has not been mapped as of 08/07 */
#define SYS_PCICTL      ((volatile unsigned int *)(SYS_BASE + 0x44))
#define SYS_FLASH       ((volatile unsigned int *)(SYS_BASE + 0x4c))
#define SYS_CLCD        ((volatile unsigned int *)(SYS_BASE + 0x50))
#define SYS_CLCDSER     ((volatile unsigned int *)(SYS_BASE + 0x54))
#define SYS_24MHZ       ((volatile unsigned int *)(SYS_BASE + 0x5c))
#define SYS_DMAPSR0     ((volatile unsigned int *)(SYS_BASE + 0x64))
#define SYS_DMAPSR1     ((volatile unsigned int *)(SYS_BASE + 0x68))
#define SYS_DMAPSR2     ((volatile unsigned int *)(SYS_BASE + 0x6c))
#define SYS_IOSEL       ((volatile unsigned int *)(SYS_BASE + 0x70))
#define SYS_BUSID       ((volatile unsigned int *)(SYS_BASE + 0x80))
#define SYS_PROCID0     ((volatile unsigned int *)(SYS_BASE + 0x84))
#define SYS_PROCID1     ((volatile unsigned int *)(SYS_BASE + 0x88))

#define SYS_TEST_OSC0   ((volatile unsigned int *)(SYS_BASE + 0xC0))
#define SYS_TEST_OSC1   ((volatile unsigned int *)(SYS_BASE + 0xC4))
#define SYS_TEST_OSC2   ((volatile unsigned int *)(SYS_BASE + 0xC8))
#define SYS_TEST_OSC3   ((volatile unsigned int *)(SYS_BASE + 0xCC))
#define SYS_TEST_OSC4   ((volatile unsigned int *)(SYS_BASE + 0xD0))
#define SYS_RTC         ((volatile unsigned int *)(SYS_BASE + 0x17000))

// DevChip TIMCLK control
#define TIMCLK_CTRL     ((volatile unsigned int *)(SYS_BASE + 0x1000))

// Timers 0/1
#define TIMER01_BASE    (SYS_BASE + 0x11000)
#define TIMER0_LOAD     ((volatile unsigned int *)(TIMER01_BASE + 0x00))
#define TIMER0_VALUE    ((volatile unsigned int *)(TIMER01_BASE + 0x04))
#define TIMER0_CTRL     ((volatile unsigned int *)(TIMER01_BASE + 0x08))
#define TIMER0_CLR      ((volatile unsigned int *)(TIMER01_BASE + 0x0C))

#define TIMER1_LOAD     ((volatile unsigned int *)(TIMER01_BASE + 0x20))
#define TIMER1_VALUE    ((volatile unsigned int *)(TIMER01_BASE + 0x24))
#define TIMER1_CTRL     ((volatile unsigned int *)(TIMER01_BASE + 0x28))
#define TIMER1_CLR      ((volatile unsigned int *)(TIMER01_BASE + 0x2C))




// SYS_MCI values on GPIO3
#define SYS_MCI_CARDIN0 0x01    // MMCI card detect 0
#define SYS_MCI_WPROT0  0x02    // MMCI write protect 0


#define SYS_CLCD_EN     0x14    // Enable CLCD and VGA

// Misc defines
#define SLEEP_TIMEOUT   1000    // Maximum time for timer to be loaded
#define BOARD_REV   0x140       // Realview/EB SYS_ID should be rev 8

#define SYS_LOCK_UNLOCK     0x0000A05F     
#define SYS_LOCK_LOCK       0x00000000
#define XTALCLK_MIN         15              // XTAL clock minimum value
#define XTALCLK_MINTYP      10              // XTAL clock typical minimum value
#define XTALCLK_DFLT        25              // XTAL clock default value
#define XTALCLK_MAX         35              // XTAL clock maximum value
#define XTALCLK_MAXTYP      35              // XTAL clock typical maximum value
#define XTALFIN             24              // Oscillator clock

#define CLCD_BASE           (SYS_BASE + 0x20000)
#define CLCD_FRAME_BASE1     (0x80000000 - 640*480*4)  // Frame buffer is at the end 
#define CLCD_FRAME_BASE2     (0x80000000 - 2*640*480*4)  // Frame buffer is at the end of SDRAM


// CLCD Controller Internal Register addresses
#define CLCD_TIM0           ((volatile int *)(CLCD_BASE + 0x000))
#define CLCD_TIM1           ((volatile int *)(CLCD_BASE + 0x004))
#define CLCD_TIM2           ((volatile int *)(CLCD_BASE + 0x008))
#define CLCD_TIM3           ((volatile int *)(CLCD_BASE + 0x00C))
#define CLCD_UBAS           ((volatile int *)(CLCD_BASE + 0x010))
#define CLCD_LBAS           ((volatile int *)(CLCD_BASE + 0x014))
#define CLCD_CNTL           ((volatile int *)(CLCD_BASE + 0x018))
#define CLCD_IENB           ((volatile int *)(CLCD_BASE + 0x01C))
#define CLCD_STAT           ((volatile int *)(CLCD_BASE + 0x020))
#define CLCD_INTR           ((volatile int *)(CLCD_BASE + 0x024))
#define CLCD_ICLR           ((volatile int *)(CLCD_BASE + 0x028))
#define CLCD_UCUR           ((volatile int *)(CLCD_BASE + 0x02C))
#define CLCD_LCUR           ((volatile int *)(CLCD_BASE + 0x030))
#define CLCD_PALETTE        ((volatile int *)(CLCD_BASE + 0x200))
#define CLCD_FRAME1          ((volatile int *)(CLCD_FRAME_BASE1))
#define CLCD_FRAME2          ((volatile int *)(CLCD_FRAME_BASE2))

// Cursor control
#define CLCD_CRSR           ((volatile int *)(CLCD_BASE + 0x800))
#define CLCD_CRSR_Cntl      ((volatile int *)(CLCD_BASE + 0xC00))
#define CLCD_CRSR_Config    ((volatile int *)(CLCD_BASE + 0xC04))
#define CLCD_CRSR_Palet0    ((volatile int *)(CLCD_BASE + 0xC08))
#define CLCD_CRSR_Palet1    ((volatile int *)(CLCD_BASE + 0xC0C))
#define CLCD_CRSR_XY        ((volatile int *)(CLCD_BASE + 0xC10))
#define CLCD_CRSR_Clip      ((volatile int *)(CLCD_BASE + 0xC14))
#define CLCD_CRSR_IENB      ((volatile int *)(CLCD_BASE + 0xC20))
#define CLCD_CRSR_ICLR      ((volatile int *)(CLCD_BASE + 0xC24))
#define CLCD_CRSR_INTR      ((volatile int *)(CLCD_BASE + 0xC28))
#define CLCD_CRSR_STAT      ((volatile int *)(CLCD_BASE + 0xC2C))

// Peripheral ID
#define CLCD_PeriphID0      ((volatile int *)(CLCD_BASE + 0xFE0))
// ...

// CLCD Timing Reg 0 (8.4", 3.8" and 2.2" displays) 
#define CLCD_PPL84          ((640/16)-1)            // Pixels per line
#define CLCD_HSW84          63                      // Pulse width for Horiz Sync
#define CLCD_HFP84          31                      // clk delay after line transmission
#define CLCD_HBP84          63                      // clk delay before starting next line
#define CLCD_PPL38          ((320/16)-1)
#define CLCD_HSW38          5
#define CLCD_HFP38          5
#define CLCD_HBP38          5
#define CLCD_PPL22          ((176/16)-1)
#define CLCD_HSW22          1
#define CLCD_HFP22          0
#define CLCD_HBP22          0

// CLCD Timing Reg 1 (8.4", 3.8" and 2.2" displays) 
#define CLCD_LPP84          (480-1)     // Lines per panel
#define CLCD_VSW84          24          // Vertical sync pulse width
#define CLCD_VFP84          11          // # of line clocks delay after every frame transmission
#define CLCD_VBP84          9           // clk delay before starting frame
#define CLCD_LPP38          (240-1)
#define CLCD_VSW38          5
#define CLCD_VFP38          5
#define CLCD_VBP38          5
#define CLCD_LPP22          (220-1)
#define CLCD_VSW22          0
#define CLCD_VFP22          0
#define CLCD_VBP22          0

// CLCD Timing Reg 2 (8.4", 3.8" and 2.2" displays) 
#define CLCD_PCD            0       // Pixel clk divider, to get Panel Clk Freq by CLCP = CLCDCLK/(CLD+2)
#define CLCD_CLKSEL         0
#define CLCD_ACB            0
#define CLCD_IVS            1
#define CLCD_IHS            1
#define CLCD_IPC            1       // Changed for Boulder
#define CLCD_IOE            0
#define CLCD_CPL84          (640-1) // Clocks per line
#define CLCD_CPL38          (320-1)
#define CLCD_CPL22          (176-1)
#define CLCD_BCD            1       // Bool to bypass PCD above


// Timing Reg 3
#define CLCD_LED            0       // Line-end signal delay period
#define CLCD_LEE            0       // Line-end signal enable

// Control register
#define CLCD_LCDEN          1
#define CLCD_LCDBPP4        2       // 2 = 4bpp
#define CLCD_LCDBPP8        3       // 3 = 8bpp
#define CLCD_LCDBPP24       5       // 5 = 24bpp
#define CLCD_LCDBW          0
#define CLCD_LCDTFT         1
#define CLCD_LCDMONO8       0
#define CLCD_LCDDUAL        0
#define CLCD_BGR            0
#define CLCD_RGB			1
#define CLCD_BGR22          1
#define CLCD_BEBO           0
#define CLCD_BEPO           0
#define CLCD_LCDPWR         1
#define CLCD_LCDVCOMP       1       // Interrupt on start of back porch
#define CLCD_LDMAFIFOTME    0
#define CLCD_WATERMARK      0

#define CLCD_24BITMODE      (0x5 << 1)
#define CLCD_5551MODE       (0x4 << 1)
#define CLCD_565MODE		(0x6 << 1)
#define CLCD_MODEMASK       0xE

// Interrupt enable register
#define FUFINTRENB          0x02
#define LNBUINTRENB         0x04
#define VCOMPINTRENB        0x08
#define MBERRINTRENB        0x10

// Upper and lower panel frame base addresses
#define LCDUPBASE1           CLCD_FRAME_BASE1
#define LCDUPBASE2           CLCD_FRAME_BASE2
#define LCDLPBASE            0x00000000




// Other screen defaults
#define PALETTE_WORDS       128     // Number of words in palette
#define XPIXELS             FB_XSIZE  // 640     // Number of pixel columns (VGA and 8.4")
#define YPIXELS             FB_YSIZE  // 480     // Number of pixel rows    (VGA and 8.4")
#define XPIXELS38           320     // Number of pixel columns (3.8")
#define YPIXELS38           240     // Number of pixel rows    (3.8")
#define XPIXELS22           176     // Number of pixel columns (2.2")
#define YPIXELS22           220     // Number of pixel rows    (2.2")
#define CLCDIDMASK          0x1F00  // CLCDID 5 bit mask
#define CLCDID84            0x0100  // CLCDID for Sharp 8.4" CLCD
#define CLCDID38            0x0000  // CLCDID for Sanyo 3.8" CLCD
#define CLCDID22            0x0200  // CLCDID for Epson 2.2" CLCD

/*  Function: apCLCD_Init(void)
 *   Purpose: Initiate the CLCD screen, set the palette and clear the frame buffer
 *
 * Arguments: mode24bit, 8bit or 24bit mode select
 *   Returns: TRUE if OK, FALSE if failed
 */
int apCLCD_Init(int mode24bit, int clear);

void CopyFrameBufferToDisplay(unsigned long addr);

