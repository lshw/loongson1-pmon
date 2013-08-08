#include <pmon.h>
#include <cpu.h>
#include <string.h>
#include <machine/pio.h>
#include <time.h>
#include "target/fcr.h"

#define REG_TOY_READ0 0xbfe6402c
#define REG_TOY_READ1 0xbfe64030
#define REG_TOY_WRITE0 0xbfe64024
#define REG_TOY_WRITE1 0xbfe64028
#define REG_CNTRCTL 0xbfe64040

extern void init_legacy_rtc(void);

time_t get_rtc_time(void)
{
	struct tm tm;
	time_t t;
	unsigned int year,v;

//	if(!clk_invalid) {
		v = inl(REG_TOY_READ0);
		year = inl(REG_TOY_READ1);
		
		tm.tm_sec = (v>>4)&0x3f;
		tm.tm_min = (v>>10)&0x3f;
		tm.tm_hour = (v>>16)&0x1f;
		tm.tm_wday = 0;
		tm.tm_mday = (v>>21)&0x1f;
		tm.tm_mon = ((v>>26)&0xf) - 1;
		tm.tm_year = year - 1900;
		if(tm.tm_year < 50)tm.tm_year += 100;

		tm.tm_isdst = tm.tm_gmtoff = 0;
		t = gmmktime(&tm);
//	}
//	else
//	{
//		t = 957960000;  /* Wed May 10 14:00:00 2000 :-) */
//	}
	return(t);
}

int test_rtc(int argc,char **argv)
{
	struct tm tm;
	time_t t;
	char *v;

#ifdef CONFIG_CHINESE
	printf("RTC实时时钟测试\n说明：\n");
	printf("1.注意显示的时钟秒值是否变化，如果显示的时钟不变，说明RTC工作不正常。\n");
	printf("2.按任意键退出测试程序。\n");
#else
	printf("press any key to out of RTC test\n");
#endif
	printf("\n");
	init_legacy_rtc();
	while(1){
		t = tgt_gettime();
		while(tgt_gettime() == t){
			if (get_uart_char(COM1_BASE_ADDR)){
				printf("\n");
				return 0;
			}
		}
	#ifdef HAVE_TOD
		tm = *localtime (&t);
	#endif
		printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b%04d-%02d-%02d %02d:%02d:%02d",tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour, tm.tm_min, tm.tm_sec);
	}
	return 0;
}


static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"test_rtc","", 0, "test rtc", test_rtc, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}

