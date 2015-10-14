/* 
 * Display Controller for Loongson1
 */

#include <pmon.h>
#include <pmon/dev/ns16550.h>

#include <machine/pio.h>
#include <target/regs-clk.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/malloc.h>

#define DC_BASE_ADDR0 0xbc301240
#define DC_BASE_ADDR1 0xbc301250

#define PAGE_SIZE	(1 << 12)
#define PAGE_ALIGN(addr) ALIGN(addr, PAGE_SIZE)
#define ALIGN(x, a)		__ALIGN_KERNEL((x), (a))
#define __ALIGN_KERNEL(x, a)		__ALIGN_KERNEL_MASK(x, (typeof(x))(a) - 1)
#define __ALIGN_KERNEL_MASK(x, mask)	(((x) + (mask)) & ~(mask))

static int fb_xsize, fb_ysize, frame_rate;
#if defined(LS1ASOC) || defined(LS1BSOC)
#define BURST_SIZE	0xff
static char *addr_cursor = NULL;
static char *mem_ptr = NULL;
#elif defined(LS1CSOC)
#define BURST_SIZE	0x7f
static char *addr_cursor = NULL;
static char *mem_ptr = NULL;
#endif

unsigned long fb_addr;

static struct vga_struc {
	unsigned int pclk, refresh;
	unsigned int hr, hss, hse, hfl;
	unsigned int vr, vss, vse, vfl;
	unsigned int pan_config;		/* 注意 不同的lcd面板该值可能不同 */
	unsigned int hvsync_polarity;	/* 注意 不同的lcd面板该值可能不同 */
} vgamode[] = {
	{/*"240x320_70.00"*/	6429,	70,	240,	250,	260,	280,	320,	324,	326,	328,	0x00000301, 0x40000000},/* ili9341 DE mode */
	{/*"320x240_60.00"*/	7154,	60,	320,	332,	364,	432,	240,	248,	254,	276,	0x00000103, 0xc0000000},/* HX8238-D控制器 */
//	{/*"320x240_60.00"*/	6438,	60,	320,	336,	337,	408,	240,	250,	251,	263,	0x80001311, 0xc0000000},/* NT39016D控制器 */
	{/*"320x480_60.00"*/	12908,	60,	320,	360,	364,	432,	480,	488,	490,	498,	0x00000101, 0xc0000000},/* NT35310控制器 */
//	{/*"480x272_60.00"*/	9009,	60,	480,	0,	0,	525,	272,	0,	0,	288,	0x00000101, 0x00000000},/* AT043TN13 DE mode */
	{/*"480x272_60.00"*/	9009,	60,	480,	488,	489,	531,	272,	276,	282,	288,	0x00000101, 0xc0000000},/* LT043A-02AT */
	{/*"480x640_60.00"*/	20217,	60,	480,    488,    496,    520,    640,    642,    644,    648,	0x00000101, 0xc0000000},/* jbt6k74控制器 */
//	{/*"640x480_60.00"*/	25200,	60,	640,	656,	666,	800,	480,	512,    514,    525,	0x00000101, 0xc0000000},/* AT056TN52 */
	{/*"640x480_60.00"*/	25200,	60,	640,	656,	666,	800,	480,	512,    514,    525,	0x00000301, 0xc0000000},/* AT056TN53 */
	{/*"640x640_60.00"*/	33100,	60,	640,	672,	736,	832,	640,	641,	644,	663,	0x00000101, 0xc0000000},
	{/*"640x768_60.00"*/	39690,	60,	640,	672,	736,	832,	768,	769,	772,	795,	0x00000101, 0xc0000000},
	{/*"640x800_60.00"*/	42130,	60,	640,	680,	744,	848,	800,	801,	804,	828,	0x00000101, 0xc0000000},
	{/*"800x480_60.00"*/	29232,	60,	800,	0,	0,	928,	480,	0,    0,    525,	0x00000101, 0x00000000},/* AT070TN93 HX8264 DE mode */
//	{/*"800x480_60.00"*/	29232,	60,	800,	840,	888,	928,	480,	493,    496,    525,	0x00000100, 0xc0000000},/* HX8264 HV mode */

//	{/*"800x600_60.00"*/	39790,	60,	800,	840,	968,	1056,	600,	601,	605,	628,	0x00000101, 0xc0000000},/* VESA */
	{/*"800x600_75.00"*/	49500,	75,	800,	816,	896,	1056,	600,	601,	604,	625,	0x00000101, 0xc0000000},/* VESA */
	{/*"800x640_60.00"*/	40730,	60,	800,	832,	912,	1024,	640,	641,	644,	663,	0x00000101, 0xc0000000},
	{/*"832x600_60.00"*/	40010,	60,	832,	864,	952,	1072,	600,	601,	604,	622,	0x00000101, 0xc0000000},
	{/*"832x608_60.00"*/	40520,	60,	832,	864,	952,	1072,	608,	609,	612,	630,	0x00000101, 0xc0000000},
	{/*"1024x480_60.00"*/	38170,	60,	1024,	1048,	1152,	1280,	480,	481,	484,	497,	0x00000101, 0xc0000000},
	{/*"1024x600_60.00"*/	51206,	60,	1024,	0,	0,	1344,	600,	0,	0,	635,	0x00000101, 0x00000000}, /* HDBO101XLE-21 DE mode */
//	{/*"1024x600_60.00"*/	51206,	60,	1024,	1184,	1185,	1344,	600,	612,	613,	635,	0x00000101, 0xc0000000}, /* HDBO101XLE-21 HV mode */
	{/*"1024x640_60.00"*/	52830,	60,	1024,	1072,	1176,	1328,	640,	641,	644,	663,	0x00000101, 0xc0000000},
	{/*"1024x768_60.00"*/	65000,	60,	1024,	1048,	1184,	1344,	768,	771,	777,	806,	0x00000101, 0xc0000000},/* VESA */
	{/*"1152x764_60.00"*/	71380,	60,	1152,	1208,	1328,	1504,	764,	765,    768,    791,	0x00000101, 0xc0000000},
	{/*"1280x800_60.00"*/	83460,	60,	1280,	1344,	1480,	1680,	800,	801,    804,    828,	0x00000101, 0xc0000000},
//	{/*"1280x1024_60.00"*/	108000,	60,	1280,	1328,	1440,	1688,	1024,	1025,   1028,   1066,	0x00000101, 0xc0000000},/* VESA */
	{/*"1280x1024_75.00"*/	135000,	75,	1280,	1296,	1440,	1688,	1024,	1025,   1028,   1066,	0x00000101, 0xc0000000},/* VESA */
	{/*"1360x768_60.00"*/	85500,	60,	1360,	1424,	1536,	1792,	768,	771,    777,    795,	0x00000101, 0xc0000000},/* VESA */
	{/*"1440x1050_60.00"*/	121750,	60,	1440,	1528,	1672,	1904,	1050,	1053,	1057,	1089,	0x00000101, 0xc0000000},/* VESA */
//	{/*"1440x900_60.00"*/	106500,	60,	1440,	1520,	1672,	1904,	900,    903,    909,    934,	0x00000101, 0xc0000000},/* VESA */
	{/*"1440x900_75.00"*/	136750,	75,	1440,	1536,	1688,	1936,	900,	903,	909,	942,	0x00000101, 0xc0000000},/* VESA */
	{/*"1920x1080_60.00"*/	148500,	60,	1920,	2008,	2052,	2200,	1080,	1084,	1089,	1125,	0x00000101, 0xc0000000},/* VESA */
};

#ifdef CONFIG_VGA_MODEM	/* 只用于1B的外接VGA */
struct ls1b_vga {
	unsigned int xres;
	unsigned int yres;
	unsigned int refresh;
	unsigned int ls1b_pll_freq;
	unsigned int ls1b_pll_div;
};

static struct ls1b_vga ls1b_vga_modes[] = {
	{
		.xres = 800,
		.yres = 600,
		.refresh = 75,
	#if APB_CLK == 25000000
		.ls1b_pll_freq = 0x21813,
		.ls1b_pll_div = 0x8a28ea00,
	#else	//APB_CLK == 33000000
		.ls1b_pll_freq = 0x0080c,
		.ls1b_pll_div = 0x8a28ea00,
	#endif
	},
	{
		.xres = 1024,
		.yres = 768,
		.refresh = 60,
	#if APB_CLK == 25000000
		.ls1b_pll_freq = 0x2181d,
		.ls1b_pll_div = 0x8a28ea00,
	#else	//APB_CLK == 33000000
		.ls1b_pll_freq = 0x21813,
		.ls1b_pll_div = 0x8a28ea00,
	#endif
	},
	{
		.xres = 1280,
		.yres = 1024,
		.refresh = 75,
	#if APB_CLK == 25000000
		.ls1b_pll_freq = 0x3af1e,
		.ls1b_pll_div = 0x8628ea00,
	#else	//APB_CLK == 33000000
		.ls1b_pll_freq = 0x3af14,
		.ls1b_pll_div = 0x8628ea00,
	#endif
	},
	{
		.xres = 1440,
		.yres = 900,
		.refresh = 75,
	#if APB_CLK == 25000000
		.ls1b_pll_freq = 0x3af1f,
		.ls1b_pll_div = 0x8628ea00,
	#else	//APB_CLK == 33000000
		.ls1b_pll_freq = 0x3af14,
		.ls1b_pll_div = 0x8628ea00,
	#endif
	},
	{
		.xres = 1920,
		.yres = 1080,
		.refresh = 60,
	#if APB_CLK == 25000000
		.ls1b_pll_freq = 0x3af23,
		.ls1b_pll_div = 0x86392a00,
	#else	//APB_CLK == 33000000
		.ls1b_pll_freq = 0x3af17,
		.ls1b_pll_div = 0x86392a00,
	#endif
	},
	{},
};
#endif	//#ifdef CONFIG_VGA_MODEM

enum {
	OF_BUF_CONFIG = 0,
	OF_BUF_ADDR0 = 0x20,
	OF_BUF_STRIDE = 0x40,
	OF_BUF_ORIG = 0x60,
	OF_DITHER_CONFIG = 0x120,
	OF_DITHER_TABLE_LOW = 0x140,
	OF_DITHER_TABLE_HIGH = 0x160,
	OF_PAN_CONFIG = 0x180,
	OF_PAN_TIMING = 0x1a0,
	OF_HDISPLAY = 0x1c0,
	OF_HSYNC = 0x1e0,
	OF_VDISPLAY = 0x240,
	OF_VSYNC = 0x260,
	OF_BUF_ADDR1 = 0x340,
};

#if defined(LS1ASOC)
static unsigned int caclulatefreq(unsigned int sys_clk, unsigned int pclk)
{
	/* N值和OD值选择不正确会造成系统死机，莫名其妙。OD=2^PIX12 */
	unsigned int N = 4, PIX12 = 2, OD = 4;
	unsigned int M = 0, FRAC = 0;
	unsigned long tmp1, tmp2;

	while (1) {
		tmp2 = pclk * N * OD;
		M = tmp2 / sys_clk;
		if (M <= 1) {
			N++;
		} else {
			tmp1 = sys_clk * M;
			if (tmp2 < tmp1) {
				unsigned int tmp3;
				tmp3 = tmp1; tmp1 = tmp2; tmp2 = tmp3;
			}
			if ((tmp2 - tmp1) > 16384) {
				if (N < 15 ) {
					N++;
				} else {
					N = 15; PIX12++; OD *= 2;
					if (PIX12 > 3) {
						tgt_printf("Warning: \
								clock source is out of range.\n");
						break;
					}
				}
			}
			else {
				FRAC = ((tmp2 - tmp1) * 262144) / sys_clk;
				break;
			}
		}
	}
//	printf("tmp2-tmp1=%d FRAC=%d\n", tmp2 - tmp1, FRAC);
//	printf("PIX12=%d N=%d M=%d\n", PIX12, N, M);
	return ((FRAC<<14) + (PIX12<<12) + (N<<8) + M);
}
#elif defined(LS1BSOC)
extern void _probe_frequencies(void);
static void caclulatefreq(unsigned int ls1b_pll_freq, unsigned int ls1b_pll_div)
{
	unsigned int pll, ctrl;
	unsigned int x, divisor;
	unsigned int i;
	int ls1b_uart_base[] = {
		0xbfe40000, 0xbfe41000, 0xbfe42000,
		0xbfe43000, 0xbfe44000, 0xbfe45000,
		0xbfe46000, 0xbfe47000, 0xbfe48000,
		0xbfe4c000, 0xbfe6c000, 0xbfe7c000
	};
	#define PORT(id, offset)	(u8 *)(ls1b_uart_base[id] + offset)
	
	#define DIV_DDR				(0x1f << 14)
	#define DIV_DDR_SHIFT		14

	/* 计算ddr频率，更新串口分频 */
	pll = ls1b_pll_freq;
	ctrl = ls1b_pll_div & DIV_DDR;
	divisor = (12 + (pll & 0x3f)) * APB_CLK / 2
			+ ((pll >> 8) & 0x3ff) * APB_CLK / 1024 / 2;
	divisor = divisor / (ctrl >> DIV_DDR_SHIFT);
	divisor = divisor / 2 / (16*115200);

	for (i=0; i<12; i++) {
		x = readb(PORT(i, NS16550_CFCR));
		writeb(x | CFCR_DLAB, PORT(i, NS16550_CFCR));

		writeb(divisor & 0xff, PORT(i, NS16550_DATA));	/* DLL */
		writeb((divisor>>8) & 0xff, PORT(i, NS16550_IER));	/* DLM */

		writeb(x & ~CFCR_DLAB, PORT(i, NS16550_CFCR));
	}
	/* 更新系统的cpu和总线频率 */
	_probe_frequencies();
}
#endif

#ifdef DC_CURSOR
static int config_cursor(void)
{
	int i;
	
	for (i=0; i<0x1000; i+=4)
		*(volatile unsigned int *)(addr_cursor + i) = 0x88f31f4f;

	addr_cursor = (unsigned int)addr_cursor & 0x0fffffff;

	writel(0x00020200, 0xbc301520+0x00);
	writel(addr_cursor, 0xbc301530+0x00);
	writel(0x00060122, 0xbc301540+0x00);
	writel(0x00eeeeee, 0xbc301550+0x00);
	writel(0x00aaaaaa, 0xbc301560+0x00);
	return 0;
}
#endif

static int config_fb(unsigned int base)
{
	int i, mode = -1;
	u32 val;

	for (i=0; i<sizeof(vgamode)/sizeof(struct vga_struc); i++) {
		if (vgamode[i].hr == fb_xsize && vgamode[i].vr == fb_ysize) {
//			&& vgamode[i].refresh == frame_rate) {
			mode = i;
		#if defined(LS1ASOC)
		{
			int out;
			out = caclulatefreq(APB_CLK/1000, vgamode[mode].pclk);
			/*output pix1 clock  pll ctrl */
			#ifdef DC_FB0
			*(volatile int *)0xbfd00410 = out;
			delay(1000);
			#endif
			/*output pix2 clock pll ctrl */
			#ifdef DC_FB1
			*(volatile int *)0xbfd00424 = out;
			delay(1000);
			#endif
			/*inner gpu dc logic fifo pll ctrl,must large then outclk*/
			out = caclulatefreq(APB_CLK/1000, 200000);
			*(volatile int *)0xbfd00414 = out;
			delay(1000);
		}
//		#elif defined(CONFIG_FB_DYN)	//LS1BSOC
		#elif defined(LS1BSOC)
		#ifdef CONFIG_VGA_MODEM	/* 只用于1B的外接VGA */
		{
			struct ls1b_vga *input_vga;

			for (input_vga=ls1b_vga_modes; input_vga->ls1b_pll_freq!=0; ++input_vga) {
		//		if((input_vga->xres == fb_xsize) && (input_vga->yres == fb_ysize) && 
		//			(input_vga->refresh == frame_rate)) {
				if ((input_vga->xres == fb_xsize) && (input_vga->yres == fb_ysize)) {
					break;
				}
			}
			if (input_vga->ls1b_pll_freq) {
				writel(input_vga->ls1b_pll_freq, LS1X_CLK_PLL_FREQ);
				writel(input_vga->ls1b_pll_div, LS1X_CLK_PLL_DIV);
				delay(100);
				caclulatefreq(input_vga->ls1b_pll_freq, input_vga->ls1b_pll_div);
			}
		}
		#else
		{
			u32 divider_int, div_reg;

			divider_int = tgt_pllfreq() / (vgamode[mode].pclk * 1000) / 4;
			/* check whether divisor is too small. */
			if (divider_int < 1) {
				printf("Warning: clock source is too slow."
						"Try smaller resolution\n");
				divider_int = 1;
			}
			else if (divider_int > 15) {
				printf("Warning: clock source is too fast."
						"Try smaller resolution\n");
				divider_int = 15;
			}
			/* Set setting to reg. */
			div_reg = readl(LS1X_CLK_PLL_DIV);
			div_reg |= 0x00003000;	//dc_bypass 置1
			div_reg &= ~0x00000030;	//dc_rst 置0
			div_reg &= ~(0x1f<<26);	//dc_div 清零
			div_reg |= divider_int << 26;
			writel(div_reg, LS1X_CLK_PLL_DIV);
			div_reg &= ~0x00001000;	//dc_bypass 置0
			writel(div_reg, LS1X_CLK_PLL_DIV);
		}
		#endif //CONFIG_VGA_MODEM
		#elif defined(LS1CSOC)
		{
			u32 lcd_div, div_reg;

			lcd_div = tgt_pllfreq() / (vgamode[mode].pclk * 1000);
			div_reg = readl(LS1X_CLK_PLL_DIV);
			/* 注意：首先需要把分频使能位清零 */
			writel(div_reg & ~DIV_DC_EN, LS1X_CLK_PLL_DIV);
			div_reg |= DIV_DC_EN | DIV_DC_SEL_EN | DIV_DC_SEL;
			div_reg &= ~DIV_DC;
			div_reg |= lcd_div << DIV_DC_SHIFT;
			writel(div_reg, LS1X_CLK_PLL_DIV);
		}
		#endif //#ifdef LS1ASOC
		break;
		}
	}

	if (mode < 0) {
		printf("\n\n\nunsupported framebuffer resolution,choose from bellow:\n");
		for (i=0; i<sizeof(vgamode)/sizeof(struct vga_struc); i++)
			printf("%dx%d, ", vgamode[i].hr, vgamode[i].vr);
		printf("\n");
		return -1;
	}

	/* Disable the panel 0 */
	writel((unsigned int)mem_ptr & 0x0fffffff, base+OF_BUF_ADDR0);
	writel((unsigned int)mem_ptr & 0x0fffffff, base+OF_BUF_ADDR1);
//	writel(0x00000000, base+OF_DITHER_CONFIG);
//	writel(0x00000000, base+OF_DITHER_TABLE_LOW);
//	writel(0x00000000, base+OF_DITHER_TABLE_HIGH);
	/* PAN_CONFIG寄存器最高位需要置1，否则lcd时钟延时一段时间才会有输出 */
	#ifdef CONFIG_VGA_MODEM
	writel(0x80001311, base+OF_PAN_CONFIG);
	#else
	writel(0x80001111 | vgamode[mode].pan_config, base+OF_PAN_CONFIG);
	#endif
//	writel(0x00000000, base+OF_PAN_TIMING);

	writel((vgamode[mode].hfl<<16) | vgamode[mode].hr, base + OF_HDISPLAY);
	writel(vgamode[mode].hvsync_polarity | (vgamode[mode].hse<<16) | vgamode[mode].hss, base + OF_HSYNC);
	writel((vgamode[mode].vfl<<16) | vgamode[mode].vr, base + OF_VDISPLAY);
	writel(vgamode[mode].hvsync_polarity | (vgamode[mode].vse<<16) | vgamode[mode].vss, base + OF_VSYNC);

#if defined(CONFIG_VIDEO_32BPP)
	writel(0x00100104, base+OF_BUF_CONFIG);
	writel((fb_xsize*4+BURST_SIZE)&~BURST_SIZE, base+OF_BUF_STRIDE);
	#ifdef LS1BSOC
	*(volatile int *)0xbfd00420 &= ~0x08;
	*(volatile int *)0xbfd00420 |= 0x05;
	#endif
#elif defined(CONFIG_VIDEO_24BPP)
	writel(0x00100104, base+OF_BUF_CONFIG);
	writel((fb_xsize*3+BURST_SIZE)&~BURST_SIZE, base+OF_BUF_STRIDE);
	#ifdef LS1BSOC
	*(volatile int *)0xbfd00420 &= ~0x08;
	*(volatile int *)0xbfd00420 |= 0x05;
	#endif
#elif defined(CONFIG_VIDEO_16BPP)
	writel(0x00100103, base+OF_BUF_CONFIG);
	writel((fb_xsize*2+BURST_SIZE)&~BURST_SIZE, base+OF_BUF_STRIDE);
	#ifdef LS1BSOC
	*(volatile int *)0xbfd00420 &= ~0x07;
	#endif
#elif defined(CONFIG_VIDEO_15BPP)
	writel(0x00100102, base+OF_BUF_CONFIG);
	writel((fb_xsize*2+BURST_SIZE)&~BURST_SIZE, base+OF_BUF_STRIDE);
	#ifdef LS1BSOC
	*(volatile int *)0xbfd00420 &= ~0x07;
	#endif
#elif defined(CONFIG_VIDEO_12BPP)
	writel(0x00100101, base+OF_BUF_CONFIG);
	writel((fb_xsize*2+BURST_SIZE)&~BURST_SIZE, base+OF_BUF_STRIDE);
	#ifdef LS1BSOC
	*(volatile int *)0xbfd00420 &= ~0x07;
	#endif
#else	/* 16bpp */
	writel(0x00100103, base+OF_BUF_CONFIG);
	writel((fb_xsize*2+BURST_SIZE)&~BURST_SIZE, base+OF_BUF_STRIDE);
	#ifdef LS1BSOC
	*(volatile int *)0xbfd00420 &= ~0x07;
	#endif
#endif	//#if defined(CONFIG_VIDEO_32BPP)
	writel(0, base+OF_BUF_ORIG);

	{	/* 显示数据输出使能 */
		int timeout = 204800;
		val = readl((base+OF_BUF_CONFIG));
		do {
			writel(val|0x100, base+OF_BUF_CONFIG);
			val = readl(base+OF_BUF_CONFIG);
		} while (((val & 0x100) == 0) && (timeout-- > 0));
	}
	return 0;
}

unsigned long dc_init(void)
{
	unsigned int smem_len;
	/* Driver IC需要在ls1x lcd控制器初始化前现进行初始化，否则有可能出现没显示的现象 */
#if defined(CONFIG_JBT6K74)
//	printf("JBT6K74 TFT LCD Driver IC\n");
	jbt6k74_init();
#endif
#if defined(CONFIG_ILI9341)
//	printf("ILI9341 TFT LCD Driver IC\n");
	ili9341_hw_init();
#endif
#if defined(CONFIG_NT35310)
//	printf("NT35310 TFT LCD Driver IC\n");
	nt35310_init();
#endif
#if defined(CONFIG_ST7565)
//	printf("ST7565 LCD Driver IC\n");
	st7565_init();
#endif

	fb_xsize  = getenv("xres") ? strtoul(getenv("xres"),0,0) : FB_XSIZE;
	fb_ysize  = getenv("yres") ? strtoul(getenv("yres"),0,0) : FB_YSIZE;
	frame_rate  = getenv("frame_rate") ? strtoul(getenv("frame_rate"),0,0) : 60;

	smem_len = PAGE_ALIGN(fb_xsize * fb_ysize * 4);
	mem_ptr = (char *)malloc(smem_len, M_DEVBUF, M_WAITOK); /* 高位地在开始分配内存 */
	if (!mem_ptr) {
		printf("Unable to allocate memory for lcd.\n");
		return -ENOMEM;
	}
	memset(mem_ptr, 0, smem_len);
	mem_ptr = (char *)((unsigned int)mem_ptr | 0xa0000000); /* 需要转换为umap ucache的地址 */
	fb_addr = (unsigned long)mem_ptr;

#ifdef DC_FB0
	config_fb(DC_BASE_ADDR0);
#endif
#ifdef DC_FB1
	config_fb(DC_BASE_ADDR1);
#endif
#ifdef DC_CURSOR
	config_cursor();
#endif

	return (unsigned long)mem_ptr;
}

