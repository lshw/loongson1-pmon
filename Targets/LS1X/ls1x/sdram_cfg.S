//#define	SD_FREQ	(6 * PLL_M) / (2 * SDRAM_PARAM_DIV_NUM)
#define	SD_FREQ	(((APB_CLK / 4) * (PLL_MULT / CPU_DIV)) / SDRAM_PARAM_DIV_NUM)
#define	ROW_WIDTH	13
/* 颗粒行数 */
#define	ROW_1K		0x7
#define	ROW_2K		0x0
#define	ROW_4K		0x1
#define	ROW_8K		0x2
#define	ROW_16K	0x3
/* 颗粒列数 */
#define	COL_256	0x7
#define	COL_512	0x0
#define	COL_1K		0x1
#define	COL_2K		0x2
#define	COL_4K		0x3
/* 颗粒位宽 */
#define	WIDTH_8	0x0
#define	WIDTH_16	0x1
#define	WIDTH_32	0x2

/*
#define	TIME_SET(x)	(x * SD_FREQ +999)/1000
#define	TRCD		TIME_SET(20)
#define	TCL			(SD_FREQ / 75 + 2)
#define	TRP			TIME_SET(20)
#define	TRFC		TIME_SET(63)
#define	TRAS		TIME_SET(63)
#define	TREF		((64000 * SD_FREQ)>>ROW_WIDTH - 8)
#define	TWR			(SD_FREQ/75 + 1)
*/

#define	TRCD		3
#define	TCL			3
#define	TRP			3
#define	TRFC		9
#define	TRAS		6
#define	TREF		0xfff	//8530
#define	TWR			2

#define	DEF_SEL	0x1
#define	DEF_SEL_N	0x0
#define	HANG_UP	0x1
#define	HANG_UP_N	0x0
#define	CFG_VALID	0x1

#define	SD_PARA0	(0x7f<<25 | \
					(TRAS << 21) | \
					(TRFC << 17) | (TRP << 14) | (TCL << 11) | \
					(TRCD << 8) | (WIDTH_16 << 6) | (COL_512 << 3) | \
					ROW_8K)

#define	SD_PARA1	((CFG_VALID << 9) | (HANG_UP_N << 8) | \
					(DEF_SEL_N << 7) | (TWR << 5) | (TREF >> 7))
