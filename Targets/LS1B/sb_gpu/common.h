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
 * File:     common.h
 * Release:  Version 2.0
 * ----------------------------------------------------------------
 *
 *  ----------------------------------------
 *  Version and Release Control Information:
 *
 *  File Name              : $RCSfile: common.h,v $
 *  File Revision          : $Revision: 1.1.2.1 $
 *
 *  ----------------------------------------
 *
 *            Common Interface
 *            ================
 */

#ifndef _COMMON_H_
#define _COMMON_H_


// Types
#undef FALSE
#undef TRUE
#define FALSE   0
#define TRUE    1


// For debugging the tests.
#define DEBUG
#ifdef DEBUG
    #define debug(...)      printf(__VA_ARGS__)
#else
    #define debug(...)
#endif  // ifdef DEBUG

// System Control registers
//#define SYS_BASE        0x10000000
#define SYS_BASE        0xBC200000                              //zgj
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

/*
 * Description:
 * Error codes - each module is assigned space for error codes from apERR_XXXX_START to apERR_XXXX_START+255.
 *
 * Implementation:
 * Define an enumerated type apError.  By using this it is posible to avoid the
 * need to specify individual values for the error constants, but can simply append
 * new constants to the list below.
 *
 * Code numbers with the top bit clear are generic errors.
 * Those with the top bit set (negative) are module-specific errors
 */
#ifndef _EXTN_ERROR_H
#define _EXTN_ERROR_H

typedef enum
{
/*---------------Generic errors---------------*/
apERR_NONE          =0,

apERR_BAD_PARAMETER =1,    // Call failed due to a parameter out of range.
apERR_UNSUPPORTED   =2,    // Call failed due to a parameter which was in range,
                           // but unsupported for this variant of the hardware.
apERR_BUSY          =3,    // Resource is already in use

/*--------------Specific errors---------------*/
/* Returns errors of the form 0xFFFFxxxx */
apERR_BUZZER_START  =  apERR_NONE         - 0x100,    /*Module apBUZZER*/
apERR_MMC_START     =  apERR_BUZZER_START - 0x100,    /*Module apMMC*/
apERR_SCI_START     =  apERR_MMC_START    - 0x100,    /*Module apSCI*/
apERR_UART0_START   =  apERR_SCI_START    - 0x100,    /*Module apUART0*/
apERR_UART1_START   =  apERR_UART0_START  - 0x100,    /*Module apUART1*/
apERR_IRDA_START    =  apERR_UART1_START  - 0x100,    /*Module apIRDA*/
apERR_LCD_START     =  apERR_IRDA_START   - 0x100,    /*Module apLCD*/
apERR_TSCI_START    =  apERR_LCD_START    - 0x100,    /*Module apTSCI*/
apERR_LCDBIAS_START =  apERR_TSCI_START   - 0x100,    /*Module apLCDBIAS*/
apERR_AACI_START    =  apERR_LCDBIAS_START- 0x100,    /*Module apAACI*/
apERR_LEDS_START    =  apERR_AACI_START   - 0x100,    /*Module apLEDS*/
apERR_GPIO_START    =  apERR_LEDS_START   - 0x100,    /*Module apGPIO*/
apERR_KYBD_START    =  apERR_GPIO_START   - 0x100,    /*Module apKYBD*/
apERR_MOUSE_START   =  apERR_KYBD_START   - 0x100,    /*Module apMOUSE*/
apERR_LANI_START    =  apERR_MOUSE_START  - 0x100,    /*Module apLANI*/
apERR_CLOCK_START   =  apERR_LANI_START   - 0x100,    /*Module apCLOCK*/
apERR_USB_START     =  apERR_CLOCK_START  - 0x100,    /*Module apUSB*/
apERR_MEM_START     =  apERR_USB_START    - 0x100,    /*Module apMEM*/
apERR_RTC_START 	=  apERR_MEM_START	  - 0x100,	  /*Module apI2C_RTC*/
apERR_CHARLCD_START =  apERR_RTC_START	  - 0x100,	  /*Module apCharLCD*/
apERR_SSP_START		=  apERR_CHARLCD_START- 0x100,    /*Module SSP*/
apERR_DMA_START     =  apERR_SSP_START    - 0x100,    /*Module DMA*/
apERR_PCI_START     =  apERR_DMA_START    - 0x100,    /*Module PCI*/
apERR_TIMER_START   =  apERR_PCI_START    - 0x100,    /*Module Timer*/
/*============================================*/

apERR_END                  // Dummy terminator
} apError;

#endif

#ifndef _EXTN_STATUS_H
#define _EXTN_STATUS_H

// Return status
typedef enum sts
{
    TEST_S_UNKNOWN,
    TEST_S_NOTRUN,
    TEST_S_SUCCESS,
    TEST_S_FAILURE,
    MAX_STATUS
}
Status_t;

#endif

enum Time_Units {
    MICROSECONDS    = 0,
    MILLISECONDS    = 1,
    SECONDS         = 2
};


// Declaration of the global variables
extern unsigned int RunAllTests;    //Used to indicate Run All Tests mode


#define ARM_CMID_PB    0x7000      // PB926EJ-S SYS_ID should be rev 7
#define ARM_CMID_AB    0x8000      // AB926EJ-S SYS_ID should be rev 8
#define ARM_CMID_EB    0x01400000  // EB SYS_ID should be board 0x140 (new ID layout)

typedef enum arm_board {
    board_unknown   = 0,
    board_pb926     = 1,
    board_ab926     = 2,
    board_eb        = 3
} boardtype_t;

boardtype_t detect_board(void);

// Function prototypes
void timer_start(const unsigned int, const enum Time_Units);
int  timeout(const unsigned int, const enum Time_Units);


int ct7tdmi(void);
void turn_icache_on(void);
void turn_icache_off(void);

unsigned int    register_test(volatile unsigned int *addr, int firstbit,int lastbit);
void            apSleep(unsigned int msec);
void            apSleepus(unsigned int usec);
void            Wait_For_Enter (int always);
int             Get_OK(void);

#endif // _COMMON_H_

