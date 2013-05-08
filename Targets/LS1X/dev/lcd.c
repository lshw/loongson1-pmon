/* 
 * Display Controller for Loongson1
 */

#include <pmon.h>
#include <pmon/dev/ns16550.h>

#include <target/types.h>
#include <target/iorw.h>
#include <target/regs-clk.h>

#include <stdlib.h>
#include <stdio.h>

#define DC_BASE_ADDR0 0xbc301240
#define DC_BASE_ADDR1 0xbc301250

static int fb_xsize, fb_ysize, frame_rate;
#if defined(LS1ASOC) || defined(LS1BSOC)
#define BURST_SIZE	0xff
static char *addr_cursor = 0xa3900000;
static char *mem_ptr = 0xa3200000;
#elif defined(LS1CSOC)
#define BURST_SIZE	0x7f
static char *addr_cursor = 0xa1900000;
static char *mem_ptr = 0xa1200000;
#endif

static struct vga_struc {
	unsigned int pclk, refresh;
	int hr, hss, hse, hfl;
	int vr, vss, vse, vfl;
	int pan_config;
} vgamode[] = {
		{/*"240x320_70.00"*/	6429,	60,	240,	250,	260,	280,	320,	324,	326,	328,	0x00000301},
		{/*"320x240_60.00"*/	7154,	60,	320,	332,	364,	432,	240,	248,	254,	276,	0x00000103},/* HX8238-D控制器 */
//		{/*"320x240_60.00"*/	6438,	60,	320,	336,	337,	408,	240,	250,	251,	263,	0x80001311},/* NT39016D控制器 */
		{/*"480x272_60.00"*/	12072,	60,	480,	481,	482,	525,	272,	273,	274,	288,	0x00000101},/* AT043TN24 */
		{/*"480x640_60.00"*/	20217,	60,	480,    488,    496,    520,    640,    642,    644,    648,	0x00000101},/* jbt6k74控制器 */
		{/*"640x480_60.00"*/	24480,	60,	640,	664,	728,	816,	480,	481,    484,    500,	0x00000101},/* AT056TN52 */
		{/*"640x640_60.00"*/	33100,	60,	640,	672,	736,	832,	640,	641,	644,	663,	0x00000101},
		{/*"640x768_60.00"*/	39690,	60,	640,	672,	736,	832,	768,	769,	772,	795,	0x00000101},
		{/*"640x800_60.00"*/	42130,	60,	640,	680,	744,	848,	800,	801,	804,	828,	0x00000101},
		{/*"800x480_60.00"*/	30720,	60,	800,	832,	912,	1024,	480,	481,    484,    500,	0x00000101},/* AT070TN93 */
//		{/*"800x600_60.00"*/	39790,	60,	800,	840,	968,	1056,	600,	601,	605,	628,	0x00000101},/* VESA */
		{/*"800x600_75.00"*/	49500,	75,	800,	816,	896,	1056,	600,	601,	604,	625,	0x00000101},/* VESA */
		{/*"800x640_60.00"*/	40730,	60,	800,	832,	912,	1024,	640,	641,	644,	663,	0x00000101},
		{/*"832x600_60.00"*/	40010,	60,	832,	864,	952,	1072,	600,	601,	604,	622,	0x00000101},
		{/*"832x608_60.00"*/	40520,	60,	832,	864,	952,	1072,	608,	609,	612,	630,	0x00000101},
		{/*"1024x480_60.00"*/	38170,	60,	1024,	1048,	1152,	1280,	480,	481,	484,	497,	0x00000101},
		{/*"1024x600_60.00"*/	48960,	60,	1024,	1064,	1168,	1312,	600,	601,	604,	622,	0x00000101},
		{/*"1024x640_60.00"*/	52830,	60,	1024,	1072,	1176,	1328,	640,	641,	644,	663,	0x00000101},
		{/*"1024x768_60.00"*/	65000,	60,	1024,	1048,	1184,	1344,	768,	771,	777,	806,	0x00000101},/* VESA */
		{/*"1152x764_60.00"*/	71380,	60,	1152,	1208,	1328,	1504,	764,	765,    768,    791,	0x00000101},
		{/*"1280x800_60.00"*/	83460,	60,	1280,	1344,	1480,	1680,	800,	801,    804,    828,	0x00000101},
//		{/*"1280x1024_60.00"*/	108000,	60,	1280,	1328,	1440,	1688,	1024,	1025,   1028,   1066,	0x00000101},/* VESA */
		{/*"1280x1024_75.00"*/	135000,	75,	1280,	1296,	1440,	1688,	1024,	1025,   1028,   1066,	0x00000101},/* VESA */
		{/*"1360x768_60.00"*/	85500,	60,	1360,	1424,	1536,	1792,	768,	771,    777,    795,	0x00000101},/* VESA */
		{/*"1440x1050_60.00"*/	121750,	60,	1440,	1528,	1672,	1904,	1050,	1053,	1057,	1089,	0x00000101},/* VESA */
//		{/*"1440x900_60.00"*/	106500,	60,	1440,	1520,	1672,	1904,	900,    903,    909,    934,	0x00000101},/* VESA */
		{/*"1440x900_75.00"*/	136750,	75,	1440,	1536,	1688,	1936,	900,	903,	909,	942,	0x00000101},/* VESA */
		{/*"1920x1080_60.00"*/	148500,	60,	1920,	2008,	2052,	2200,	1080,	1084,	1089,	1125,	0x00000101},/* VESA */
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
	int ii;
	
	for(ii=0; ii<0x1000; ii+=4)
		*(volatile unsigned int *)(addr_cursor + ii) = 0x88f31f4f;

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
				weitel(input_vga->ls1b_pll_freq, LS1X_CLK_PLL_FREQ);
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
	writel(0x00000000, base+OF_BUF_CONFIG);
	writel((unsigned int)mem_ptr & 0x0fffffff, base+OF_BUF_ADDR0);
	writel((unsigned int)mem_ptr & 0x0fffffff, base+OF_BUF_ADDR1);
	writel(0x00000000, base+OF_DITHER_CONFIG);
	writel(0x00000000, base+OF_DITHER_TABLE_LOW);
	writel(0x00000000, base+OF_DITHER_TABLE_HIGH);
	#ifdef CONFIG_VGA_MODEM
	writel(0x80001311, base+OF_PAN_CONFIG);
	#else
	writel(vgamode[mode].pan_config, base+OF_PAN_CONFIG);
	#endif
	writel(0x00000000, base+OF_PAN_TIMING);

	writel((vgamode[mode].hfl<<16)|vgamode[mode].hr, base+OF_HDISPLAY);
	writel(0x40000000|(vgamode[mode].hse<<16)|vgamode[mode].hss, base+OF_HSYNC);
	writel((vgamode[mode].vfl<<16)|vgamode[mode].vr, base+OF_VDISPLAY);
	writel(0x40000000|(vgamode[mode].vse<<16)|vgamode[mode].vss, base+OF_VSYNC);

#if defined(CONFIG_VIDEO_32BPP)
	writel(0x00100004, base+OF_BUF_CONFIG);
	writel((fb_xsize*4+BURST_SIZE)&~BURST_SIZE, base+OF_BUF_STRIDE);
	#ifdef LS1BSOC
	*(volatile int *)0xbfd00420 &= ~0x08;
	*(volatile int *)0xbfd00420 |= 0x05;
	#endif
#elif defined(CONFIG_VIDEO_24BPP)
	writel(0x00100004, base+OF_BUF_CONFIG);
	writel((fb_xsize*3+BURST_SIZE)&~BURST_SIZE, base+OF_BUF_STRIDE);
	#ifdef LS1BSOC
	*(volatile int *)0xbfd00420 &= ~0x08;
	*(volatile int *)0xbfd00420 |= 0x05;
	#endif
#elif defined(CONFIG_VIDEO_16BPP)
	writel(0x00100003, base+OF_BUF_CONFIG);
	writel((fb_xsize*2+BURST_SIZE)&~BURST_SIZE, base+OF_BUF_STRIDE);
	#ifdef LS1BSOC
	*(volatile int *)0xbfd00420 &= ~0x07;
	#endif
#elif defined(CONFIG_VIDEO_15BPP)
	writel(0x00100002, base+OF_BUF_CONFIG);
	writel((fb_xsize*2+BURST_SIZE)&~BURST_SIZE, base+OF_BUF_STRIDE);
	#ifdef LS1BSOC
	*(volatile int *)0xbfd00420 &= ~0x07;
	#endif
#elif defined(CONFIG_VIDEO_12BPP)
	writel(0x00100001, base+OF_BUF_CONFIG);
	writel((fb_xsize*2+BURST_SIZE)&~BURST_SIZE, base+OF_BUF_STRIDE);
	#ifdef LS1BSOC
	*(volatile int *)0xbfd00420 &= ~0x07;
	#endif
#else	/* 16bpp */
	writel(0x00100003, base+OF_BUF_CONFIG);
	writel((fb_xsize*2+BURST_SIZE)&~BURST_SIZE, base+OF_BUF_STRIDE);
	#ifdef LS1BSOC
	*(volatile int *)0xbfd00420 &= ~0x07;
	#endif
#endif	//#if defined(CONFIG_VIDEO_32BPP)
	writel(0, base+OF_BUF_ORIG);

	{	/* 显示数据输出使能 */
		int timeout = 204800;
		u32 val;
		val = readl((base+OF_BUF_CONFIG));
		do {
			writel(val|0x100, base+OF_BUF_CONFIG);
			val = readl(base+OF_BUF_CONFIG);
		} while (((val & 0x100) == 0) && (timeout-- > 0));
	}
	return 0;
}

int dc_init(void)
{
	fb_xsize  = getenv("xres") ? strtoul(getenv("xres"),0,0) : FB_XSIZE;
	fb_ysize  = getenv("yres") ? strtoul(getenv("yres"),0,0) : FB_YSIZE;
	frame_rate  = getenv("frame_rate") ? strtoul(getenv("frame_rate"),0,0) : 60;

#ifdef DC_FB0
	config_fb(DC_BASE_ADDR0);
#endif
#ifdef DC_FB1
	config_fb(DC_BASE_ADDR1);
#endif
#ifdef DC_CURSOR
	config_cursor();
#endif

#if defined(CONFIG_JBT6K74)
	jbt6k74_init();
#endif

#if defined(CONFIG_ILI9341)
	ili9341_hw_init();
#endif

	return mem_ptr;
}

