//#define	SD_FREQ	(6 * PLL_M) / (2 * SDRAM_PARAM_DIV_NUM)
#define	SD_FREQ	(((APB_CLK / 4) * (PLL_MULT / CPU_DIV)) / SDRAM_PARAM_DIV_NUM)
/* 颗粒行数 */
#define	ROW_1K		0x7
#define	ROW_2K		0x0
#define	ROW_4K		0x1
#define	ROW_8K		0x2
#define	ROW_16K		0x3
/* 颗粒列数 */
#define	COL_256		0x7
#define	COL_512		0x0
#define	COL_1K		0x1
#define	COL_2K		0x2
#define	COL_4K		0x3
/* 颗粒位宽 */
#define	WIDTH_8		0x0
#define	WIDTH_16	0x1
#define	WIDTH_32	0x2

#define	TRCD		3
#define	TCL			3
#define	TRP			3
#define	TRFC		8
#define	TRAS		6
#define	TREF		0x818
#define	TWR			2

#define	DEF_SEL		0x1
#define	DEF_SEL_N	0x0
#define	HANG_UP		0x1
#define	HANG_UP_N	0x0
#define	CFG_VALID	0x1

#define	SD_PARA0	(0x7f<<25 | \
					(TRAS << 21) | \
					(TRFC << 17) | (TRP << 14) | (TCL << 11) | \
					(TRCD << 8) | (WIDTH_16 << 6) | (COL_512 << 3) | \
					ROW_8K)

#define	SD_PARA1	((HANG_UP_N << 8) | (DEF_SEL_N << 7) | (TWR << 5) | (TREF >> 7))

#define	SD_PARA1_EN	((CFG_VALID << 9) | (HANG_UP_N << 8) | \
					(DEF_SEL_N << 7) | (TWR << 5) | (TREF >> 7))
