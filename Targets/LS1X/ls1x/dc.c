//Created by xiexin for Display Controller pmon test 
//Oct 6th,2009
#include <pmon.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef CONFIG_LS1B01
#define EXTRA_DIV 1
#else
#define EXTRA_DIV 4
#endif
typedef unsigned long  u32;
typedef unsigned short u16;
typedef unsigned char  u8;
typedef signed long  s32;
typedef signed short s16;
typedef signed char  s8;
typedef int bool;
typedef unsigned long dma_addr_t;

//#define TEST_800x600 1
#undef TEST_800x600
#ifdef LS1ASOC
#define DC_FB1 1
#undef DC_FB0
#else
#define DC_FB0 1
#undef DC_FB1
#endif

#define writeb(val, addr) (*(volatile u8*)(addr) = (val))
#define writew(val, addr) (*(volatile u16*)(addr) = (val))
#define writel(val, addr) (*(volatile u32*)(addr) = (val))
#define readb(addr) (*(volatile u8*)(addr))
#define readw(addr) (*(volatile u16*)(addr))
#define readl(addr) (*(volatile u32*)(addr))

#define write_reg(addr,val) writel(val,addr)

#define EXTRA_PIXEL  0
#define DC_BASE_ADDR 0xbc301240
#define DC_BASE_ADDR_1 0xbc301250

#define RANDOM_HEIGHT_Z 37

static char *ADDR_CURSOR = 0xA6000000;
static char *MEM_ptr = 0xA2000000;
static int MEM_ADDR =0;

struct vga_struc{
	long pclk;
	int hr,hss,hse,hfl;
	int vr,vss,vse,vfl;
	int pan_config;
	int pll_reg0, pll_reg1;
}

#ifdef CONFIG_VGA_MODEM
vgamode[] = {
	{/*"320x240_60.00"*/	5260,	320,	304,	336,	352,	240,	241,	244,	249,	0x80001311},
	{/*"640x480_70.00"*/	28560,	640,	664,	728,	816,	480,	481,    484,    500,	0x80001311},
	{/*"640x640_60.00"*/	33100,	640,	672,	736,	832,	640,	641,	644,	663,	0x80001311},
	{/*"640x768_60.00"*/	39690,	640,	672,	736,	832,	768,	769,	772,	795,	0x80001311},
	{/*"640x800_60.00"*/	42130,	640,	680,	744,	848,	800,	801,	804,	828,	0x80001311},
	{/*"800x480_70.00"*/	35840,	800,	832,	912,	1024,	480,	481,    484,    500,	0x80001311},
	{/*"800x600_75.00"*/	49500,	800,	816,	896,	1056,	600,	601,	604,	625,	0x80001311, 0x0080c, 0x8a28ea00},/*******/
	{/*"800x640_60.00"*/	40730,	800,	832,	912,	1024,	640,	641,	644,	663,	0x80001311},
	{/*"832x600_60.00"*/	40010,	832,	864,	952,	1072,	600,	601,	604,	622,	0x80001311},
	{/*"832x608_60.00"*/	40520,	832,	864,	952,	1072,	608,	609,	612,	630,	0x80001311},
	{/*"1024x480_60.00"*/	38170,	1024,	1048,	1152,	1280,	480,	481,	484,	497,	0x80001311},
	{/*"1024x600_60.00"*/	48960,	1024,	1064,	1168,	1312,	600,	601,	604,	622,	0x80001311},
	{/*"1024x640_60.00"*/	52830,	1024,	1072,	1176,	1328,	640,	641,	644,	663,	0x80001311},
	{/*"1024x768_60.00"*/	65000,	1024,	1048,	1184,	1344,	768,	771,	777,	806,	0x80001311, 0x21813, 0x8a28ea00},/*******/
	{/*"1152x764_60.00"*/	71380,	1152,	1208,	1328,	1504,	764,	765,    768,    791,	0x80001311},
	{/*"1280x800_60.00"*/	83460,	1280,	1344,	1480,	1680,	800,	801,    804,    828,	0x80001311},
//	{/*"1280x1024_60.00"*/	10888,	1280,	1328,	1440,	1688,	1024,	1025,   1028,   1066,	0x80001311, 0x0080e, 0x86292a00},/*******/
	{/*"1280x1024_75.00"*/	135000,	1280,	1296,	1440,	1688,	1024,	1025,   1028,   1066,	0x80001311, 0x3af14, 0x8628ea00},/*******/
	{/*"1368x768_60.00"*/	85860,	1368,	1440,	1584,	1800,	768,	769,    772,    795,	0x80001311},
	{/*"1440x800_60.00"*/	93800,	1440,	1512,	1664,	1888,	800,	801,    804,    828,	0x80001311},
//	{/*"1440x900_60.00"*/	120280,	1440,	1520,	1672,	1904,	900,    903,    909,    934,	0x80001311, 0x0080e, 0x86292a00},/*******/
	{/*"1440x900_75.00"*/	136750,	1440,	1536,	1688,	1936,	900,    903,    909,    942,	0x80001311, 0x3af14, 0x8628ea00},/*******/
};
#else
vgamode[] = {
	{/*"320x240_70.00"*/	4000,	320,	332,	364,	432,	240,	248,	254,	276,	0x00000103},/* HX8238-D控制器 */
//	{/*"320x240_70.00"*/	4000,	320,	336,	337,	408,	240,	250,	251,	263,	0x80001311},/* NT39016D控制器 */
	{/*"480x272_70.00"*/	6500,	480,	481,	482,	525,	272,	273,	274,	288,	0x00000101},//THF
	{/*"640x480_70.00"*/    28560,  640,    664,    728,    816,    480,    481,    484,    500,    0x00000101},
	{/*"640x640_60.00"*/	33100,	640,	672,	736,	832,	640,	641,	644,	663,	0x00000101},
	{/*"640x768_60.00"*/	39690,	640,	672,	736,	832,	768,	769,	772,	795,	0x00000101},
	{/*"640x800_60.00"*/	42130,	640,	680,	744,	848,	800,	801,	804,	828,	0x00000101},
	{/*"800x480_70.00"*/    35840,  800,    832,    912,    1024,   480,    481,    484,    500,    0x00000101},
//	{/*"800x600_60.00"*/	38220,	800,	832,	912,	1024,	600,	601,	604,	622,	0x00000101},
	{/*"800x600_60.00"*/	38220,	800,	832,	912,	1024,	600,	601,	604,	622,	0x80001311},
	{/*"800x640_60.00"*/	40730,	800,	832,	912,	1024,	640,	641,	644,	663,	0x00000101},
	{/*"832x600_60.00"*/	40010,	832,	864,	952,	1072,	600,	601,	604,	622,	0x00000101},
	{/*"832x608_60.00"*/	40520,	832,	864,	952,	1072,	608,	609,	612,	630,	0x00000101},
	{/*"1024x480_60.00"*/	38170,	1024,	1048,	1152,	1280,	480,	481,	484,	497,	0x00000101},
	{/*"1024x600_60.00"*/	48960,	1024,	1064,	1168,	1312,	600,	601,	604,	622,	0x00000101},
	{/*"1024x640_60.00"*/	52830,	1024,	1072,	1176,	1328,	640,	641,	644,	663,	0x00000101},
	{/*"1024x768_60.00"*/	64110,	1024,	1080,	1184,	1344,	768,	769,	772,	795,	0x00000101},
	{/*"1152x764_60.00"*/   71380,  1152,   1208,   1328,   1504,   764,    765,    768,    791,    0x00000101},
	{/*"1280x800_60.00"*/   83460,  1280,   1344,   1480,   1680,   800,    801,    804,    828,    0x00000101},
	{/*"1280x1024_55.00"*/  98600,  1280,   1352,   1488,   1696,   1024,   1025,   1028,   1057,   0x00000101},
	{/*"1440x800_60.00"*/   93800,  1440,   1512,   1664,   1888,   800,    801,    804,    828,    0x00000101},
//	{/*"1440x900_67.00"*/   120280, 1440,   1528,   1680,   1920,   900,    901,    904,    935,    0x00000101},
	{/*"1440x900_67.00"*/   40000, 1440,   1528,   1680,   1920,   900,    901,    904,    935,    0x80001311},
};
#endif

enum{
	OF_BUF_CONFIG=0,
	OF_BUF_ADDR=0x20,
	OF_BUF_STRIDE=0x40,
	OF_BUF_ORIG=0x60,
	OF_DITHER_CONFIG=0x120,
	OF_DITHER_TABLE_LOW=0x140,
	OF_DITHER_TABLE_HIGH=0x160,
	OF_PAN_CONFIG=0x180,
	OF_PAN_TIMING=0x1a0,
	OF_HDISPLAY=0x1c0,
	OF_HSYNC=0x1e0,
	OF_VDISPLAY=0x240,
	OF_VSYNC=0x260,
	OF_DBLBUF=0x340,
};

#define MYDBG printf(":%d\n",__LINE__);
#define PLL_FREQ_REG(x) *(volatile unsigned int *)(0xbfe78030+x)

#ifdef LS1ASOC
int caclulatefreq(long long XIN, long long PCLK)
{
	long N=4, NO=4, OD=2, M, FRAC;
	int flag = 0;
	long  out;
	long long MF;
//	printf("PCLK=%lld\n",PCLK);

	while(flag == 0) {
		flag = 1;
//		printf("N=%lld\n",N);
		if(XIN/N<5000) {N--; flag=0;}
		if(XIN/N>50000) {N++; flag=0;}
	}
	flag = 0;
	while(flag == 0) {
		flag = 1;
		if(PCLK*NO<200000) {NO*=2; OD++; flag=0;}
		if(PCLK*NO>700000) {NO/=2; OD--; flag=0;}
	}
	MF = PCLK*N*NO*262144/XIN;
	MF %= 262144;
	M = PCLK*N*NO/XIN;
	FRAC = (int)(MF);
	out = (FRAC<<14)+(OD<<12)+(N<<8)+M;

//	printf("in this case, M=%llx ,N=%llx, OD=%llx, FRAC=%llx\n",M,N,OD,FRAC);
	return out;
}
#else
#define abs(x) ((x<0)?(-x):x)
#define min(a,b) ((a<b)?a:b)
int caclulatefreq(long long XIN, long long PCLK)
{
#if 0
	int i;
	long long clk,clk1;
	int start,end;
	int mi;
	int pll,ctrl,div,div1,frac;
	pll=PLL_FREQ_REG(0);
	ctrl=PLL_FREQ_REG(4);
	printf("pll=0x%x,ctrl=0x%x\n",pll,ctrl);
	printf("cpu freq is %d\n",tgt_pipefreq());
	start=pll?-1:0;
	end=1;
/*ls1b02:dc clk divided 4 */
	PCLK *= EXTRA_DIV;

	for(i=start;i<=end;i++)
	{
	clk=(12+i+(pll&0x3f))*APB_CLK/2;
	div=clk/(long)PCLK/1000;
	clk1=(12+i+1+(pll&0x3f))*APB_CLK/2;
	div1=clk1/(long)PCLK/1000;
	if(div!=div1)break;
	}

	if(div!=div1)
	{
	frac=((PCLK*1000*div1)*2*1024/APB_CLK - (12+i+(pll&0x3f))*1024)&0x3ff;
	pll = (pll & ~0x3ff3f)|(frac<<8)|((pll&0x3f)+i);
	ctrl = ctrl&~(0x1f<<26)|(div1<<26)|(1<<31);
	}
	else
	{
	clk=(12+start+(pll&0x3f))*APB_CLK/2;
	clk1=(12+end+(pll&0x3f))*APB_CLK/2;
	if(abs((long)clk/div/1000-PCLK)<abs((long)clk1/(div+1)/1000-PCLK))
	{
	pll = (pll & ~0x3ff3f)|((pll&0x3f)+start);
	ctrl = ctrl&~(0x1f<<26)|(div<<26)|(1<<31);
	}
	else
	{
	pll = (pll & ~0x3ff3f)|((pll&0x3f)+end);
	ctrl = ctrl&~(0x1f<<26)|((div+1)<<26)|(1<<31);
	}
	}

	printf("new pll=%x: ctrl=%x\n",pll,ctrl);
	ctrl |= 0x2a00;
	PLL_FREQ_REG(4) = ctrl; 
	PLL_FREQ_REG(0) = pll;
	delay(1000);
#endif
	initserial(0);
	_probe_frequencies();
	printf("cpu freq is %d\n",tgt_pipefreq());
	return 0;
}

#endif

int config_cursor(void)
{
	write_reg((0xbc301520+0x00), 0x00020200);
	write_reg((0xbc301530+0x00), ADDR_CURSOR);
	write_reg((0xbc301540+0x00), 0x00060122);
	write_reg((0xbc301550+0x00), 0x00eeeeee);
	write_reg((0xbc301560+0x00), 0x00aaaaaa);
}

static int fb_xsize, fb_ysize, frame_rate;

int config_fb(unsigned long base)
{
	int i, mode=-1;

	for(i=0; i<sizeof(vgamode)/sizeof(struct vga_struc); i++) {
		int out;
		if(vgamode[i].hr == fb_xsize && vgamode[i].vr == fb_ysize){
			mode = i;
		#ifdef LS1ASOC
			out = caclulatefreq(APB_CLK/1000, vgamode[i].pclk);
			/*inner gpu dc logic fifo pll ctrl,must large then outclk*/
//			*(volatile int *)0xbfd00414 = out + 1;
			*(volatile int *)0xbfd00414 = out;
			/*output pix1 clock  pll ctrl */
		#ifdef DC_FB0
			*(volatile int *)0xbfd00410 = out;
		#endif
			/*output pix2 clock pll ctrl */
		#ifdef DC_FB1
			*(volatile int *)0xbfd00424 = out;
		#endif

		#elif defined(CONFIG_FB_DYN)	//LS1BSOC
		#ifdef CONFIG_VGA_MODEM
			PLL_FREQ_REG(4) = vgamode[i].pll_reg1;
			PLL_FREQ_REG(0) = vgamode[i].pll_reg0;
			delay(1000);
			caclulatefreq(APB_CLK/1000,vgamode[i].pclk);
		#else
			{
			#define PLL_FREQ_REG(x) *(volatile unsigned int *)(0xbfe78030+x)
			unsigned int pll, ctrl, clk, div, tmp;
			vgamode[i].pclk = frame_rate * vgamode[i].hfl * vgamode[i].vfl;
			/* 设定LCD工作频率 */
			pll = PLL_FREQ_REG(0);
			ctrl = PLL_FREQ_REG(4);
			clk = (12 + (pll & 0x3f)) * APB_CLK / 2;
			for (tmp=1; tmp<17; tmp++) {
				//参考longson1B的数据手册 LCD分频需要再除以4
				frame_rate = clk / (vgamode[i].hfl * vgamode[i].vfl) / 4 / tmp;
				if((50<=frame_rate) && (frame_rate<=75)) {
					div = tmp;
					break;
				}
			}
			ctrl = (ctrl & ~(0x1f<<26)) | (div<<26) | (1<<31);
			PLL_FREQ_REG(4) = ctrl;
			}
		#endif /* CONFIG_VGA_MODEM */
		#endif /* #ifdef LS1ASOC */
			break;
		}
	}

	if(mode < 0) {
		printf("\n\n\nunsupported framebuffer resolution,choose from bellow:\n");
		for(i=0;i<sizeof(vgamode)/sizeof(struct vga_struc);i++)
			printf("%dx%d, ",vgamode[i].hr,vgamode[i].vr);
		printf("\n");
		return -1;
	}

	//  Disable the panel 0
	write_reg((base+OF_BUF_CONFIG),0x00000000);
	// framebuffer configuration RGB565
	write_reg((base+OF_BUF_CONFIG),0x00000003);
	write_reg((base+OF_BUF_ADDR),MEM_ADDR);
	write_reg(base+OF_DBLBUF,MEM_ADDR );
	write_reg((base+OF_DITHER_CONFIG),0x00000000);
	write_reg((base+OF_DITHER_TABLE_LOW),0x00000000);
	write_reg((base+OF_DITHER_TABLE_HIGH),0x00000000);
	write_reg((base+OF_PAN_CONFIG), vgamode[mode].pan_config);
	write_reg((base+OF_PAN_TIMING),0x00000000);

	write_reg((base+OF_HDISPLAY),(vgamode[mode].hfl<<16)|vgamode[mode].hr);
	write_reg((base+OF_HSYNC),0x40000000|(vgamode[mode].hse<<16)|vgamode[mode].hss);
	write_reg((base+OF_VDISPLAY),(vgamode[mode].vfl<<16)|vgamode[mode].vr);
	write_reg((base+OF_VSYNC),0x40000000|(vgamode[mode].vse<<16)|vgamode[mode].vss);

#if defined(CONFIG_VIDEO_32BPP)
	write_reg((base+OF_BUF_CONFIG),0x00100004);
	write_reg((base+OF_BUF_STRIDE),(fb_xsize*4+255)&~255); //1024
	#ifdef LS1BSOC
	*(volatile int *)0xbfd00420 &= ~0x18;
	*(volatile int *)0xbfd00420 |= 0x07;
	#endif
#elif defined(CONFIG_VIDEO_24BPP)
	write_reg((base+OF_BUF_CONFIG),0x00100004);
	write_reg((base+OF_BUF_STRIDE),(fb_xsize*3+255)&~255); //1024
	#ifdef LS1BSOC
	*(volatile int *)0xbfd00420 &= ~0x18;
	*(volatile int *)0xbfd00420 |= 0x07;
	#endif
#elif defined(CONFIG_VIDEO_16BPP)
	write_reg((base+OF_BUF_CONFIG),0x00100003);
	write_reg((base+OF_BUF_STRIDE),(fb_xsize*2+255)&~255); //1024
#elif defined(CONFIG_VIDEO_15BPP)
	write_reg((base+OF_BUF_CONFIG),0x00100002);
	write_reg((base+OF_BUF_STRIDE),(fb_xsize*2+255)&~255); //1024
#elif defined(CONFIG_VIDEO_12BPP)
	write_reg((base+OF_BUF_CONFIG),0x00100001);
	write_reg((base+OF_BUF_STRIDE),(fb_xsize*2+255)&~255); //1024
#else  //640x480-32Bits
	write_reg((base+OF_BUF_CONFIG),0x00100004);
	write_reg((base+OF_BUF_STRIDE),(fb_xsize*4+255)&~255); //640
#endif //32Bits

	{
	int val;
	val = readl((base+OF_BUF_CONFIG));
#ifdef LS1BSOC
	/*fix ls1g dc
	*first switch to tile mode
	*change origin register to 0
	*goback nomal mode
	*/
	write_reg((base+OF_BUF_CONFIG), val|0x10);
	write_reg((base+OF_BUF_ORIG), 0);
	readl((base+OF_BUF_ORIG));
	delay(40000);
	readl((base+OF_BUF_CONFIG));
#endif
	write_reg((base+OF_BUF_CONFIG), val|0x100);
	}
}

int dc_init(void)
{
	int ii=0;

	fb_xsize  = getenv("xres")? strtoul(getenv("xres"),0,0):FB_XSIZE;
	fb_ysize  = getenv("yres")? strtoul(getenv("yres"),0,0):FB_YSIZE;
	frame_rate  = getenv("frame_rate")? strtoul(getenv("frame_rate"),0,0):60;

	MEM_ADDR = (long)MEM_ptr&0x0fffffff;

	if(MEM_ptr == NULL) {
		printf("frame buffer memory malloc failed!\n ");
		exit(0);
	}

	for(ii=0; ii<0x1000; ii+=4)
		*(volatile unsigned int *)(ADDR_CURSOR + ii) = 0x88f31f4f;

	ADDR_CURSOR = (long)ADDR_CURSOR & 0x0fffffff;

#ifdef DC_FB0
	config_fb(DC_BASE_ADDR);
#endif
#ifdef DC_FB1
	config_fb(DC_BASE_ADDR_1);
#endif
	config_cursor();

	return MEM_ptr;
}



static int cmd_dc_freq(int argc,char **argv)
{
	int out;
	long sysclk;
	long pclk;
	if(argc<2)return -1;
	pclk=strtoul(argv[1],0,0);
	if(argc>2) sysclk=strtoul(argv[2],0,0);
	else sysclk=APB_CLK;
	out = caclulatefreq(sysclk,pclk);
	printf("out=%x\n",out);
	/*inner gpu dc logic fifo pll ctrl,must large then outclk*/
	*(volatile int *)0xbfd00414 = out+1;
	/*output pix1 clock  pll ctrl*/
	*(volatile int *)0xbfd00410 = out;
	/*output pix2 clock pll ctrl */
	*(volatile int *)0xbfd00424 = out;

	return 0;
}

static int cmd_caclfreq(int argc,char **argv)
{
	_probe_frequencies();
	printf("freq is %d\n",tgt_pipefreq());
	return 0;
}

static int cmd_initserial(int argc,char **argv)
{
	initserial(argc>1?strtoul(argv[1],0,0):0);
	return 0;
}


#ifdef LS1BSOC
struct xmode
{
	struct xmode *next;
	int i,j,ks[3],key;
};

static int cmd_xrandr(int argc,char **argv)
{
	int i,j,k,l;
	int mode=-1;
	struct xmode *head=0,*pnode,**p;
	int length=0;
	int xres,yres;
	int val,freq;
	int cpu,ddr,dc;
	int gclk;
	unsigned int r8030,r8034;
	int ks[3];
	int idx[3];
	int area[3][3];
	int order;
	int key;

	if(argc<6) return -1;

	xres=strtoul(argv[1],0,0);
	yres=strtoul(argv[2],0,0);

	for(i=4;i<=6;i++) {
		char *p;
		if(strncmp("cpu:",argv[i],4) == 0) {
			idx[0] = i-4;
			p = argv[i]+4;
		}
		else if(strncmp("ddr:",argv[i],4) == 0) {
			idx[1] = i-4;
			p = argv[i]+4;
		}
		else if(strncmp("dc:",argv[i],3) == 0) {
			idx[2] = i-4;
			p = argv[i]+3;
		}
		else
			return -1;

		area[i-4][0]=strtol(p,&p,0);
		p++;
		area[i-4][1]=strtol(p,&p,0);
		area[i-4][2]= 1;
	}

	area[idx[2]][2] = EXTRA_DIV;

	for(i=0;i<sizeof(vgamode)/sizeof(struct vga_struc);i++) {
		int out;
		if(vgamode[i].hr == xres && vgamode[i].vr == yres) {
			mode=i;
			freq = vgamode[i].pclk;
			break;
		}
	}

	if(mode<0) {
		printf("\n\n\nunsupported framebuffer resolution,choose from bellow:\n");
		for(i=0;i<sizeof(vgamode)/sizeof(struct vga_struc);i++)
			printf("%dx%d, ",vgamode[i].hr,vgamode[i].vr);
		printf("\n");
		return;
	}

	area[idx[2]][0] += freq;
	area[idx[2]][1] += freq;

	for(i=0;i<=0x3f;i++) {
		for(j=0;j<=1023;j++) {
			gclk=(APB_CLK*(12+i)+APB_CLK*j/1024)/2;
			if(gclk>660000)  continue;

			for(order=0;order<=2;order++) {	
				for(k=1;k<=31;k++) {
					val=(APB_CLK*(12+i)+APB_CLK*j/1024)/2/area[order][2]/k;
					if(val>=area[order][0] && val<=area[order][1]){
						ks[order] = k;
						break;
					}
				}
				if(k==32) break;
			}

			if(order<3) continue;

			if(idx[2] == 0)
				key = abs(gclk/area[0][2]/ks[0] - freq);
			else
				key = gclk/area[0][2]/ks[0];

			/*sort first cpu,then diff*/
			for(p=&head;*p && (*p)->key>key;p=&(*p)->next);
				pnode=malloc(sizeof(struct xmode));
			if(pnode) {
				pnode->next = *p;
				*p=pnode;
				length++;
			}
			else
				pnode=*p;
			if(!pnode)
				continue;
			pnode->i = i;
			pnode->j = j;
			pnode->ks[0] = ks[0];
			pnode->ks[1] = ks[1];
			pnode->ks[2] = ks[2];
			pnode->key = key;
		}
	}

	printf("i,\tj,\tpll,\tcpu,\tddr,\tdc,\tdcdiff\t,8030,\t,8034\n");
	for(pnode=head,l=0;pnode;pnode=pnode->next,l++) {
		i=pnode->i;
		j=pnode->j;
		ks[0]=pnode->ks[0];
		ks[1]=pnode->ks[1];
		ks[2]=pnode->ks[2];

		gclk=(APB_CLK*(12+i)+APB_CLK*j/1024)/2;

		cpu=gclk/area[idx[0]][2]/ks[idx[0]];
		ddr=gclk/area[idx[1]][2]/ks[idx[1]];
		dc=gclk/area[idx[2]][2]/ks[idx[2]];

		r8030=i|(j<<8);
		r8034=(1<<31)|(ks[2]<<26)|(1<<25)|(ks[0]<<20)|(1<<19)|(ks[1]<<14);

		printf("%d:%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%x,\t,%x\n",l,i,j,gclk,cpu,ddr,dc,dc-freq, r8030, r8034);
		if(l%10==9||!pnode->next) {
			char c,buf[10];
			printf("select which one [num|q|enter]?");
			i=read(0,buf,9);
			buf[i] = 0;
			c=buf[0];
			if(c =='\n'|| c=='\r') continue;
			if(c == 'q') return 0;
			l=strtoul(buf,0,0);
			break;
		}
	}

	if(!pnode) return 0;

	for(pnode=head;pnode && l;pnode=pnode->next,l--);

	i=pnode->i;
	j=pnode->j;
	ks[0]=pnode->ks[0];
	ks[1]=pnode->ks[1];
	ks[2]=pnode->ks[2];
	
	r8030=i|(j<<8);
	r8034=(1<<31)|(ks[2]<<26)|(1<<25)|(ks[0]<<20)|(1<<19)|(ks[1]<<14);

	{
	char str[256];
	sprintf(str,"set pll_reg0 0x%x;set pll_reg1 0x%x;set xres %d;set yres %d",r8030,r8034,xres,yres);
	do_cmd(str);
	}

	for(pnode=head;pnode;pnode=head) {
		head=pnode->next;
		free(pnode);
	}

	return 0;
}
#endif


static const Cmd Cmds[] =
{
	{"MyCmds"},
	{"dc_freq"," pclk sysclk", 0, "config dc clk(khz)",cmd_dc_freq, 0, 99, CMD_REPEAT},
	{"caclfreq","", 0, "cacl freq",cmd_caclfreq, 0, 99, CMD_REPEAT},
#ifdef LS1BSOC
	{"xrandr","xres yres hsync cpu:from-to ddr:from-to dc:from-to", 0, "xrandr xres yres hsync cpufreq(KHZ) ddrfreq(KHZ) dcdiff(khz)", cmd_xrandr, 0, 99, CMD_REPEAT},
#endif
	{"initserial","[ddrclk]", 0, "cacl freq",cmd_initserial, 0, 99, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}


