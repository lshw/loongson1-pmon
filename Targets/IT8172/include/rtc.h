#ifndef __LX_RTC_H__
#define __LX_RTC_H__

/*
 * RTC Register locations
 */
#define RTC_SEC         0x00    /* seconds */
#define RTC_SECALRM     0x01    /* seconds alarm */
#define RTC_MIN         0x02    /* minutes */
#define RTC_MINALRM     0x03    /* minutes alarm */
#define RTC_HRS         0x04    /* hours */
#define RTC_HRSALRM     0x05    /* hours alarm */
#define RTC_WDAY        0x06    /* week day */
#define RTC_DAY         0x07    /* day of month */
#define RTC_MONTH       0x08    /* month of year */
#define RTC_YEAR        0x09    /* month of year */
#define RTC_STATUSA     0x0a    /* status register A */
#define  RTCSA_UIP       0x80   /* update in progress */
#define  RTCSA_DVMASK    0x70   /* divisor select mask (see below) */
#define  RTCSA_RSMASK    0x0f   /* interrupt rate select mask (see below) */
#define RTC_STATUSB     0x0b    /* status register B */
#define  RTCSB_UTI       0x80   /* update transfer inhibit */
#define  RTCSB_PIE       0x40   /* periodic i/u enable */
#define  RTCSB_AIE       0x20   /* alarm i/u enable */
#define  RTCSB_UIE       0x10   /* update cycle i/u enable */
#define  RTCSB_SQWE      0x08   /* square wave enable */
#define  RTCSB_BINARY    0x04   /* data format (1=binary, 0=bcd) */
#define  RTCSB_24HR      0x02   /* hour format (1=24 hour) */
#define  RTCSB_DSE       0x01   /* daylight savings enable! */
#define RTC_INTR        0x0c    /* status register C (R) interrupt source */
#define  RTCIR_INTF      0x80   /* i/u output signal */
#define  RTCIR_PF        0x40   /* periodic i/u */
#define  RTCIR_AF        0x20   /* alarm i/u */
#define  RTCIR_UF        0x10   /* update i/u */
#define  RTCIR_32KE      0x04   /* enable 32kHz output */
#define RTC_STATUSD     0x0d    /* status register D (R) Lost Power */
#define  RTCSD_VRT       0x80   /* clock has valid backup power */
#define RTC_CENTURY     0x0e    /* current century - increment in Dec 99 */

#define RTC_NTODREGS    (RTC_CENTURY+1)

#define RTC_NVSTART     RTC_NTODREGS
#define RTC_NVSIZE      (128-RTC_NTODREGS)
#define RTC_SIZE        (256-RTC_NTODREGS)

/*
 * Time base (divisor select) constants (Control register A)
 */
#define RTC_OSC_ON      0x20    /* 32KHz crystal */
#define RTC_OSC_32KHz   0x30    /* 32KHz crystal; 32KHz square wave */
#define RTC_OSC_NONE    0x60    /* actually, both of these reset */
#define RTC_OSC_RESET   0x70
#define RTC_DV0_OSC_NONE 0x00   /* PC97307: bank0 oscillator disabled */
#define RTC_DV0_OSC_ON  0x20    /* PC97307: bank1 oscillator enabled */
#define RTC_DV1_OSC_ON  0x30    /* PC97307: bank2 oscillator enabled */
#define RTC_DV2_OSC_ON  0x40    /* PC97307: bank3 oscillator enabled */
#define RTC_RATE_MASK   0x0f    /* No periodic interrupt */
#define RTC_RATE_NONE   0x00    /* No periodic interrupt */
#define RTC_RATE_8192Hz 0x03    /* 122.070 us period */
#define RTC_RATE_4096Hz 0x04    /* 244.141 us period */
#define RTC_RATE_2048Hz 0x05    /* 488.281 us period */
#define RTC_RATE_1024Hz 0x06    /* 976.562 us period */
#define RTC_RATE_512Hz  0x07    /* 1.953125 ms period */
#define RTC_RATE_256Hz  0x08    /* 3.90625 ms period */
#define RTC_RATE_128Hz  0x09    /* 7.8125 ms period */
#define RTC_RATE_64Hz   0x0a    /* 15.625 ms period */
#define RTC_RATE_32Hz   0x0b    /* 31.25 ms period */
#define RTC_RATE_16Hz   0x0c    /* 62.5 ms period */
#define RTC_RATE_8Hz    0x0d    /* 125 ms period */
#define RTC_RATE_4Hz    0x0e    /* 250 ms period */
#define RTC_RATE_2Hz    0x0f    /* 500 ms period */

/* PC97037 specific definitions */

#define RTC_BANK1_CENTURY       0x48
#define RTC_BANK1_URADDR        0x50
#define RTC_BANK1_URDATA        0x53

#define RTC_BANK2_APCR1         0x40
#define RTC_BANK2_APCR2         0x41
#define RTC_BANK2_APCSR         0x42
#define RTC_BANK2_WDAY_WAKEUP   0x43
#define RTC_BANK2_DAY_WAKEUP    0x44
#define RTC_BANK2_MONTH_WAKEUP  0x45
#define RTC_BANK2_MONTH_YEAR    0x46
#define RTC_BANK2_RAMLOCK       0x47
#define RTC_BANK2_CENTURY_WAKEUP 0x48

#define APCR1_SWITCHOFFDELAY    0x02
#define APCR1_LEVELPOR          0x04
#define APCR1_CLEARPOR          0x08
#define APCR1_MOAP              0x10
#define APCR1_SOC               0x20
#define APCR1_FAILSAFESTOP      0x40
#define APCR1_POWERFAILURE      0x80

/**********************************************************************
 * register summary
 **********************************************************************/
#define RTC_SECONDS             0
#define RTC_SECONDS_ALARM       1
#define RTC_MINUTES             2
#define RTC_MINUTES_ALARM       3
#define RTC_HOURS               4
#define RTC_HOURS_ALARM         5
/* RTC_*_alarm is always true if 2 MSBs are set */
# define RTC_ALARM_DONT_CARE    0xC0

#define RTC_DAY_OF_WEEK         6
#define RTC_DAY_OF_MONTH        7

/* control registers - Moto names
 */
#define RTC_REG_A               10
#define RTC_REG_B               11
#define RTC_REG_C               12
#define RTC_REG_D               13

/**********************************************************************
 * register details
 **********************************************************************/
#define RTC_FREQ_SELECT RTC_REG_A

/* update-in-progress  - set to "1" 244 microsecs before RTC goes off the bus,
 * reset after update (may take 1.984ms @ 32768Hz RefClock) is complete,
 * totalling to a max high interval of 2.228 ms.
 */
# define RTC_UIP                0x80
# define RTC_DIV_CTL            0x70
   /* divider control: refclock values 4.194 / 1.049 MHz / 32.768 kHz */
#  define RTC_REF_CLCK_4MHZ     0x00
#  define RTC_REF_CLCK_1MHZ     0x10
#  define RTC_REF_CLCK_32KHZ    0x20
   /* 2 values for divider stage reset, others for "testing purposes only" */
#  define RTC_DIV_RESET1        0x60
#  define RTC_DIV_RESET2        0x70
  /* Periodic intr. / Square wave rate select. 0=none, 1=32.8kHz,... 15=2Hz */
# define RTC_RATE_SELECT        0x0F

/**********************************************************************/
#define RTC_CONTROL     RTC_REG_B
# define RTC_SET 0x80           /* disable updates for clock setting */
# define RTC_PIE 0x40           /* periodic interrupt enable */
# define RTC_AIE 0x20           /* alarm interrupt enable */
# define RTC_UIE 0x10           /* update-finished interrupt enable */
# define RTC_SQWE 0x08          /* enable square-wave output */
# define RTC_DM_BINARY 0x04     /* all time/date values are BCD if clear */
# define RTC_24H 0x02           /* 24 hour mode - else hours bit 7 means pm */
# define RTC_DST_EN 0x01        /* auto switch DST - works f. USA only */

/**********************************************************************/
#define RTC_INTR_FLAGS  RTC_REG_C
/* caution - cleared by read */
# define RTC_IRQF 0x80          /* any of the following 3 is active */
# define RTC_PF 0x40
# define RTC_AF 0x20
# define RTC_UF 0x10

/**********************************************************************/
#define RTC_VALID       RTC_REG_D
# define RTC_VRT 0x80           /* valid RAM and time */
/**********************************************************************/

/* example: !(CMOS_READ(RTC_CONTROL) & RTC_DM_BINARY)
 * determines if the following two #defines are needed
 */
#ifndef BCD_TO_BIN
#define BCD_TO_BIN(val) ((val)=((val)&15) + ((val)>>4)*10)
#endif

#ifndef BIN_TO_BCD
#define BIN_TO_BCD(val) ((val)=(((val)/10)<<4) + (val)%10)
#endif

#define READ_RTC_RA(v)  \
do {\
        *((volatile unsigned char *)0xb4014800) = 0xA; \
        (*v) = *((volatile unsigned char *)0xb4014801);\
} while (0);

#define WRITE_RTC_RA(v) \
do {\
        *((volatile unsigned char *)0xb4014800) = 0xA; \
        *((volatile unsigned char *)0xb4014801) = (v);\
} while (0);

#define READ_RTC_RB(v)  \
do {\
        *((volatile unsigned char *)0xb4014800) = 0xB; \
        (*v) = *((volatile unsigned char *)0xb4014801);\
} while (0);

#define WRITE_RTC_RB(v) \
do {\
        *((volatile unsigned char *)0xb4014800) = 0xB; \
        *((volatile unsigned char *)0xb4014801) = (v);\
} while (0);

#define READ_RTC_BANK0(i, v)\
do {\
        *((volatile unsigned char *)0xb4014800) = (i);\
        (*(v)) = *((volatile unsigned char *)0xb4014801);\
} while (0);

#define WRITE_RTC_BANK0(i, v)\
do {\
        *((volatile unsigned char *)0xb4014800) = (i);\
        *((volatile unsigned char *)0xb4014801) = (v);\
} while (0);

#define READ_RTC_BANK1(i, v) \
do {\
        *((volatile unsigned char *)0xb4014802) = (i);\
        (*(v)) = *((volatile unsigned char *)0xb4014803);\
} while (0);

#define WRITE_RTC_BANK1(i, v)\
do {\
        *((volatile unsigned char *)0xb4014802) = (i);\
        *((volatile unsigned char *)0xb4014803) = (v);\
} while (0);

#endif

