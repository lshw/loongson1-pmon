/*
 * (C) Copyright 2007
 * Matthias Fuchs, esd gmbh, matthias.fuchs@esd-electronics.com.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Epson RX8025 RTC driver.
 */

#if 0
#include <common.h>
#include <command.h>
#include <rtc.h>
#include <i2c.h>
#endif
#include <pmon.h>
#include <target/i2c.h>

#ifndef CONFIG_RTC_RX8025
#define CONFIG_RTC_RX8025
#endif

#ifndef CONFIG_CMD_DATE
#define CONFIG_CMD_DATE
#endif

#if defined(CONFIG_RTC_RX8025) && defined(CONFIG_CMD_DATE)

/*---------------------------------------------------------------------*/
//#undef DEBUG_RTC

#define DEBUG_RTC

#ifdef DEBUG_RTC
#define DEBUGR(fmt,args...) printf(fmt ,##args)
#else
#define DEBUGR(fmt,args...)
#endif
/*---------------------------------------------------------------------*/

#ifndef CFG_I2C_RTC_ADDR
# define CFG_I2C_RTC_ADDR       0x32
#endif

/*
 * RTC register addresses
 */
#define RTC_SEC_REG_ADDR        0x00
#define RTC_MIN_REG_ADDR        0x01
#define RTC_HR_REG_ADDR         0x02
#define RTC_WEEK_REG_ADDR        0x03
//#define RTC_DAY_REG_ADDR        0x03
#define RTC_DATE_REG_ADDR       0x04
#define RTC_MON_REG_ADDR        0x05
#define RTC_YR_REG_ADDR         0x06

#define RTC_CTL1_REG_ADDR       0x0e
#define RTC_CTL2_REG_ADDR       0x0f

/*
 * Control register 1 bits
 */
#define RTC_CTL1_BIT_2412       0x20

/*
 * Control register 2 bits
 */
#define RTC_CTL2_BIT_PON        0x10
#define RTC_CTL2_BIT_VDET       0x40
#define RTC_CTL2_BIT_XST        0x20
#define RTC_CTL2_BIT_VDSL       0x80

/*
 * Note: the RX8025 I2C RTC requires register
 * reads and write to consist of a single bus
 * cycle. It is not allowed to write the register
 * address in a first cycle that is terminated by
 * a STOP condition. The chips needs a 'restart'
 * sequence (start sequence without a prior stop).
 * This driver has been written for a 4xx board.
 * U-Boot's 4xx i2c driver is currently not capable
 * to generate such cycles to some work arounds
 * are used.
 */

/* static uchar rtc_read (uchar reg); */
//#define rtc_read(reg) buf[((reg) + 1) & 0xf]
/*
 * The struct used to pass data from the generic interface code to
 * the hardware dependend low-level code ande vice versa. Identical
 * to struct rtc_time used by the Linux kernel.
 *
 * Note that there are small but significant differences to the
 * common "struct time":
 *
 *              struct time:            struct rtc_time:
 * tm_mon       0 ... 11                1 ... 12
 * tm_year      years since 1900        years since 0
 */

struct rtc_time {
        int tm_sec;
        int tm_min;
        int tm_hour;
        int tm_mday;
        int tm_mon;
        int tm_year;
        int tm_wday;
        int tm_yday;
        int tm_isdst;
};

void rtc_set (struct rtc_time *tmp);

#ifndef uchar
typedef char uchar ;
#endif
static uchar buf[16];

uchar rtc_read(uchar reg)
{
	uchar a;
	a = buf[((reg) + 1) & 0xf];
	return a;
}


/*
extern void i2c_send(char ctrl,char addr);
extern char i2c_stat();
extern char i2c_recv();
*/

static void rtc_write (uchar reg, uchar val);
static uchar bin2bcd (unsigned int n);
static unsigned int bcd2bin (uchar c);

// static int Ics950220Read(int type,long long addr,union commondata *mydata)
//uchar 8025_i2c_read(uchar chip, uint addr, int alen, uchar *buffer, int len)
//unsigned char 8025_i2c_read(unsigned int addr)
#if 0
unsigned int 8025_i2c_read(unsigned int addr)
{
	unsigned char ret_val;
#if 0
	//i2c_send(I2C_START|I2C_WRITE,CFG_I2C_RTC_ADDR);
	i2c_send(I2C_START|I2C_WRITE,0x32);
	i2c_send(I2C_WRITE,addr);
#if 0
	i2c_send(I2C_START|I2C_WRITE,0xd3);
	count=i2c_recv();
#endif

	i2c_send(I2C_WACK,0);
	ret_val=i2c_recv();
	i2c_send(I2C_STOP|I2C_WRITE,0);
#endif
	ret_val++ ;
//	printf(" %c \n",ret_val);
	return ret_val;
}
#endif

/*
 * Get the current time from the RTC
 */
//void rtc_get (struct rtc_time *tmp)
//void rtc_get (void)
void rtc_8025_read (void)
{
#if 1
        uchar sec, min, hour, mday, wday, mon, year, ctl2;
//        uchar buf[17];
	uchar i;
	struct rtc_time *tmp;
	char value;
	char i2caddr[]={(unsigned char)0x64};
	tmp = malloc(sizeof(struct rtc_time ));
	value = 0x20;
	tgt_i2cwrite(I2C_SINGLE,i2caddr,1,0xe<<4,&value,1);

	value = 0x0;
	tgt_i2cwrite(I2C_SINGLE,i2caddr,1,0xf<<4,&value,1);

	tgt_i2cread(I2C_SINGLE,i2caddr,1,0,buf,16);

        sec = rtc_read(RTC_SEC_REG_ADDR);
        min = rtc_read(RTC_MIN_REG_ADDR);
        hour = rtc_read(RTC_HR_REG_ADDR);
        wday = rtc_read(RTC_WEEK_REG_ADDR);
        mday = rtc_read(RTC_DATE_REG_ADDR);
        mon = rtc_read(RTC_MON_REG_ADDR);
        year = rtc_read(RTC_YR_REG_ADDR);

        DEBUGR ("Get RTC year: %02x mon: %02x mday: %02x wday: %02x "
                "hr: %02x min: %02x sec: %02x\n",
                year, mon, mday, wday, hour, min, sec);

        /* dump status */
        ctl2 = rtc_read(RTC_CTL2_REG_ADDR);
        if (ctl2 & RTC_CTL2_BIT_PON)
                printf("RTC: power-on detected\n");

        if (ctl2 & RTC_CTL2_BIT_VDET)
                printf("RTC: voltage drop detected\n");

        if (!(ctl2 & RTC_CTL2_BIT_XST))
                printf("RTC: oscillator stop detected\n");

        tmp->tm_sec  = bcd2bin (sec & 0x7F);
        tmp->tm_min  = bcd2bin (min & 0x7F);
        tmp->tm_hour = bcd2bin (hour & 0x3F);
        tmp->tm_mday = bcd2bin (mday & 0x3F);
        tmp->tm_mon  = bcd2bin (mon & 0x1F);
        tmp->tm_year = bcd2bin (year) + ( bcd2bin (year) >= 70 ? 1900 : 2000);
        tmp->tm_wday = bcd2bin (wday & 0x07);
        tmp->tm_yday = 0;
        tmp->tm_isdst= 0;

        DEBUGR ("Get DATE: %4d-%02d-%02d (wday=%d)  TIME: %2d:%02d:%02d\n",
                tmp->tm_year, tmp->tm_mon, tmp->tm_mday, tmp->tm_wday,
                tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
	free(tmp);
#endif
}

/*
 * Set the current time to the RTC
 */
void rtc_8025_set (void)
{
#if 1
        uchar sec, min, hour, mday, wday, mon, year, ctl2;
//        uchar buf[17];
	uchar i;
	struct rtc_time *tmp;
	char value;
	char i2caddr[]={(unsigned char)0x64};
	tmp = malloc(sizeof(struct rtc_time ));

	value = 0x20;
	tgt_i2cwrite(I2C_SINGLE,i2caddr,1,0xe<<4,&value,1);

	value = 0x0;
	tgt_i2cwrite(I2C_SINGLE,i2caddr,1,0xf<<4,&value,1);

	memset(tmp,0,sizeof(struct rtc_time));

	rtc_set(tmp);

	free(tmp);
#endif
}

/*
 * Set the RTC
 */
void rtc_set (struct rtc_time *tmp)
{
        DEBUGR ("Set DATE: %4d-%02d-%02d (wday=%d)  TIME: %2d:%02d:%02d\n",
                tmp->tm_year, tmp->tm_mon, tmp->tm_mday, tmp->tm_wday,
                tmp->tm_hour, tmp->tm_min, tmp->tm_sec);

        if (tmp->tm_year < 1970 || tmp->tm_year > 2069)
                printf("WARNING: year should be between 1970 and 2069!\n");

        rtc_write (RTC_YR_REG_ADDR, bin2bcd (tmp->tm_year % 100));
        rtc_write (RTC_MON_REG_ADDR, bin2bcd (tmp->tm_mon));
        rtc_write (RTC_WEEK_REG_ADDR, bin2bcd (tmp->tm_wday));
        rtc_write (RTC_DATE_REG_ADDR, bin2bcd (tmp->tm_mday));
        rtc_write (RTC_HR_REG_ADDR, bin2bcd (tmp->tm_hour));
        rtc_write (RTC_MIN_REG_ADDR, bin2bcd (tmp->tm_min));
        rtc_write (RTC_SEC_REG_ADDR, bin2bcd (tmp->tm_sec));

        rtc_write (RTC_CTL1_REG_ADDR, RTC_CTL1_BIT_2412);
}

/*
 * Reset the RTC. We setting the date back to 1970-01-01.
 */
void rtc_reset (void)
{
        struct rtc_time tmp;
//        uchar buf[16];
        uchar ctl2;

#if 0
        if (i2c_read(CFG_I2C_RTC_ADDR, 0,    0,   buf, 16))
                printf("Error reading from RTC\n");
#endif

        ctl2 = rtc_read(RTC_CTL2_REG_ADDR);
        ctl2 &= ~(RTC_CTL2_BIT_PON | RTC_CTL2_BIT_VDET);
        ctl2 |= RTC_CTL2_BIT_XST | RTC_CTL2_BIT_VDSL;
        rtc_write (RTC_CTL2_REG_ADDR, ctl2);

        tmp.tm_year = 1970;
        tmp.tm_mon = 1;
        tmp.tm_mday= 1;
        tmp.tm_hour = 0;
        tmp.tm_min = 0;
        tmp.tm_sec = 0;

        rtc_set(&tmp);

        printf ( "RTC:   %4d-%02d-%02d %2d:%02d:%02d UTC\n",
                tmp.tm_year, tmp.tm_mon, tmp.tm_mday,
                tmp.tm_hour, tmp.tm_min, tmp.tm_sec);

        return;
}

/*
 * Helper functions
 */
static void rtc_write (uchar reg, uchar val)
{
	char i2caddr[]={(unsigned char)0x64};
	tgt_i2cwrite(I2C_SINGLE,i2caddr,1,reg,&val,1);
}

static unsigned bcd2bin (uchar n)
{
        return ((((n >> 4) & 0x0F) * 10) + (n & 0x0F));
}

static uchar bin2bcd (unsigned int n)
{
        return (((n / 10) << 4) | (n % 10));
}

#endif /* CONFIG_RTC_RX8025 && CONFIG_CMD_DATE */
