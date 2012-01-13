//Created by xiexin for Display Controller pmon test 
//Oct 6th,2009
#include <pmon.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <cpu.h>

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

//#define FB_XSIZE 320	//THF
//#define FB_YSIZE 240	//THF

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

extern void _probe_frequencies(void);
extern int initserial(int line);

static int ADDR_CURSOR = 0xA6000000;
static int MEM_ptr = 0xA2000000;
static int MEM_ADDR = 0;

struct vga_struc{
	long pclk;
	int hr,hss,hse,hfl;
	int vr,vss,vse,vfl;
	int pan_config;
}

vgamode[] =
{
{/*"320x240_70.00"*/	4000,	320,	332,	364,	432,	240,	248,	254,	276,	0x00000103},//THF
{/*"480x272_70.00"*/	6500,	480,	481,	482,	525,	272,	273,	274,	288,	0x00000101},//THF
{/*"640x480_70.00"*/    28560,  640,    664,    728,    816,    480,    481,    484,    500,    0x00000101},
{/*"640x640_60.00"*/	33100,	640,	672,	736,	832,	640,	641,	644,	663,	0x00000101},
{/*"640x768_60.00"*/	39690,	640,	672,	736,	832,	768,	769,	772,	795,	0x00000101},
{/*"640x800_60.00"*/	42130,	640,	680,	744,	848,	800,	801,	804,	828,	0x00000101},
{/*"800x480_70.00"*/    35840,  800,    832,    912,    1024,   480,    481,    484,    500,    0x00000101},
//{/*"800x600_60.00"*/	38220,	800,	832,	912,	1024,	600,	601,	604,	622,	0x00000101},
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
//{/*"1440x900_67.00"*/   120280, 1440,   1528,   1680,   1920,   900,    901,    904,    935,    0x00000101},
{/*"1440x900_67.00"*/   40000, 1440,   1528,   1680,   1920,   900,    901,    904,    935,    0x80001311},
};

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
int caclulatefreq(long XIN,long PCLK)
{
	long N=4,NO=4,OD=2,M,FRAC;
	int flag=0;
	long out;
	long MF;
	
#ifdef DC_DEBUG
	printf("PCLK=%lld\n",PCLK);
#endif

	while(flag==0){
		flag=1;
	#ifdef DC_DEBUG
		printf("N=%lld\n",N);
	#endif
		if(XIN/N<5000) {N--;flag=0;}
		if(XIN/N>50000) {N++;flag=0;}
	}
	flag=0;
	while(flag==0){
		flag=1;
		if(PCLK*NO<200000) {NO*=2;OD++;flag=0;}
		if(PCLK*NO>700000) {NO/=2;OD--;flag=0;}
	}
	MF=PCLK*N*NO*262144/XIN;
	MF %= 262144;
	M=PCLK*N*NO/XIN;
	FRAC=(int)(MF);
	out = (FRAC<<14)+(OD<<12)+(N<<8)+M;
#ifdef DC_DEBUG
	printf("in this case, M=%llx ,N=%llx, OD=%llx, FRAC=%llx\n",M,N,OD,FRAC);
#endif
	return out;
}

#else

#define abs(x) ((x<0)?(-x):x)
#define min(a,b) ((a<b)?a:b)

int caclulatefreq(long XIN,long PCLK)
{
	int i;
	int start,end;
	long clk,clk1;
	long pll,ctrl,div,div1,frac;
	
	pll = PLL_FREQ_REG(0);
	ctrl = PLL_FREQ_REG(4);
#ifdef DC_DEBUG
	printf("pll=0x%x,ctrl=0x%x\n",pll,ctrl);
	printf("cpu freq is %d\n",tgt_pipefreq());
#endif
	start = pll?-1:0;
	end = 1;
/*ls1b02:dc clk divided 4 */
	PCLK *= EXTRA_DIV;

	for(i=start; i<=end; i++)
	{
		clk = (12+i+(pll&0x3f))*33333333/2;
		div = clk/(long)PCLK/1000;
		clk1 = (12+i+1+(pll&0x3f))*33333333/2;
		div1 = clk1/(long)PCLK/1000;
		if(div!=div1)break;
	}

	if(div!=div1){
		frac=((PCLK*1000*div1)*2*1024/33333333 - (12+i+(pll&0x3f))*1024)&0x3ff;
		pll = (pll & ~0x3ff3f)|(frac<<8)|((pll&0x3f)+i);
		ctrl = (ctrl & ~(0x1f<<26)) | (div1<<26) | (1<<31);
	}
	else{
		clk=(12+start+(pll&0x3f))*33333333/2;
		clk1=(12+end+(pll&0x3f))*33333333/2;
		if(abs((long)clk/div/1000-PCLK)<abs((long)clk1/(div+1)/1000-PCLK)){
			pll = (pll & ~0x3ff3f)|((pll&0x3f)+start);
			ctrl = (ctrl & ~(0x1f<<26)) | (div<<26) | (1<<31);
		}
		else{
			pll = (pll & ~0x3ff3f)|((pll&0x3f)+end);
			ctrl = (ctrl & ~(0x1f<<26)) | ((div+1)<<26) | (1<<31);
		}
	}

#ifdef DC_DEBUG
	printf("new pll=%x: ctrl=%x\n",pll,ctrl);
#endif
	ctrl |= 0x2a00;
	PLL_FREQ_REG(4) = ctrl; 
	PLL_FREQ_REG(0) = pll;
	delay(1000);
	initserial(0);
#ifdef DC_DEBUG
	_probe_frequencies();
	printf("cpu freq is %d\n",tgt_pipefreq());
#endif
	return 0;
}

#endif

static void config_cursor(void)
{
	write_reg((0xbc301520 +0x00), 0x00020200);
	write_reg((0xbc301530 +0x00), ADDR_CURSOR);
	write_reg((0xbc301540 +0x00), 0x00060122);
	write_reg((0xbc301550 +0x00), 0x00eeeeee);
	write_reg((0xbc301560 +0x00), 0x00aaaaaa);
#ifdef DC_DEBUG
	printf("framebuffer Cursor Configuration\n");
	printf("framebuffer Cursor Address\n");
	printf("framebuffer Cursor Location\n");
	printf("framebuffer Cursor Background\n");
	printf("what hell is this register for ?\n");
#endif
}

static int fb_xsize, fb_ysize;

int config_fb(unsigned long base)
{
	int i,mode=-1;
	long tmp_pclk;
	int tmp;

	for(i=0;i<sizeof(vgamode)/sizeof(struct vga_struc);i++){
		if(vgamode[i].hr == fb_xsize && vgamode[i].vr == fb_ysize){
			mode=i;
		#ifdef LS1ASOC
			tmp_pclk = getenv("pclk")? strtoul(getenv("pclk"),0,0):vgamode[i].pclk;
			out = caclulatefreq(APB_CLK/1000, tmp_pclk);
		#ifdef DC_DEBUG
			printf("out=%x\n",out);
		#endif
			/*inner gpu dc logic fifo pll ctrl,must large then outclk*/
			*(volatile int *)0xbfd00414 = out + 1;
			/*output pix1 clock  pll ctrl*/
			*(volatile int *)0xbfd00410 = out;
			/*output pix2 clock pll ctrl */
			*(volatile int *)0xbfd00424 = out;
			tmp = *(volatile int *)0xbfd00424;
			tmp |= 0x110000;
			*(volatile int *)0xbfd00424 = tmp;
//		#elif !defined(CONFIG_FB_DYN)	//THF
		#else
//		#if !defined(CONFIG_FB_DYN)
			tmp_pclk = getenv("pclk")? strtoul(getenv("pclk"),0,0):vgamode[i].pclk;
		#ifdef DC_DEBUG
			printf("tmp_pclk = %x\n", tmp_pclk);
		#endif
			caclulatefreq(APB_CLK/1000, tmp_pclk);
//		#endif /*LS1ASOC*/
		#endif
			break;
		}
	}

	if(mode<0){
		printf("\n\n\nunsupported framebuffer resolution\n\n\n");
		return -1;
	}


	//  Disable the panel 0
	write_reg((base+OF_BUF_CONFIG),0x00000000);
	// framebuffer configuration RGB565
	write_reg((base+OF_BUF_CONFIG),0x00000003);
	write_reg((base+OF_BUF_ADDR), MEM_ADDR);
	write_reg((base+OF_DBLBUF), MEM_ADDR);
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
	write_reg((base+OF_BUF_CONFIG),0x00100104);
	write_reg((base+OF_BUF_STRIDE),(fb_xsize*4+255)&~255); //1024
#elif defined(CONFIG_VIDEO_16BPP)
	write_reg((base+OF_BUF_CONFIG),0x00100103);
	write_reg((base+OF_BUF_STRIDE),(fb_xsize*2+255)&~255); //1024
#elif defined(CONFIG_VIDEO_15BPP)
	write_reg((base+OF_BUF_CONFIG),0x00100102);
	write_reg((base+OF_BUF_STRIDE),(fb_xsize*2+255)&~255); //1024
#elif defined(CONFIG_VIDEO_12BPP)
	write_reg((base+OF_BUF_CONFIG),0x00100101);
	write_reg((base+OF_BUF_STRIDE),(fb_xsize*2+255)&~255); //1024
#else  //640x480-32Bits
	write_reg((base+OF_BUF_CONFIG),0x00100104);
	write_reg((base+OF_BUF_STRIDE),(fb_xsize*4+255)&~255); //640
#endif //32Bits

#ifdef LS1BSOC
	/*fix ls1b dc
	*first switch to tile mode
	*change origin register to 0
	*goback nomal mode
	*/
	{
		int val;
		val = readl((base+OF_BUF_CONFIG));
		write_reg((base+OF_BUF_CONFIG),val|0x10);
		write_reg((base+OF_BUF_ORIG),0);
//		readl((base+OF_BUF_ORIG));
		delay(40000);
//		readl((base+OF_BUF_CONFIG));
		write_reg((base+OF_BUF_CONFIG),val);
	}
#endif
	return 0;
}

int dc_init(void)
{
	int ii = 0;
	
	printf("enter dc_init...\n");

	fb_xsize  = getenv("xres")? strtoul(getenv("xres"),0,0):FB_XSIZE;
	fb_ysize  = getenv("yres")? strtoul(getenv("yres"),0,0):FB_YSIZE;

	MEM_ADDR = (long)MEM_ptr & 0x0fffffff;

	if(MEM_ptr == NULL){
		printf("frame buffer memory malloc failed!\n ");
		exit(0);
	}

	for(ii=0; ii<0x1000; ii+=4)
		*(volatile unsigned int *)(ADDR_CURSOR + ii) = 0x88f31f4f;

	ADDR_CURSOR = (long)ADDR_CURSOR & 0x0fffffff;

	printf("video_mode: %dx%d\nframe buffer addr: %x \ncursor addr: %x \n", fb_xsize, fb_ysize, MEM_ADDR, ADDR_CURSOR);
//	printf("frame buffer addr: %x \n", MEM_ADDR);
//	printf("cursor addr: %x \n", ADDR_CURSOR);

#ifdef DC_FB0
	config_fb(DC_BASE_ADDR);
#endif
#ifdef DC_FB1 
	config_fb(DC_BASE_ADDR_1);
#endif
	config_cursor();

	printf("display controller reg config complete!\n");
	
	//背光使能
	*(volatile int *)0xbfd010c0 |= 0x01;		//GPIO0使能
	*(volatile int *)0xbfd010d0 &= ~(0x01);	//GPIO0输出使能
	*(volatile int *)0xbfd010f0 |= 0x01;		//GPIO0输出1 使能LCD背光

	return MEM_ptr;
}

#ifdef DC_DEBUG
static int cmd_dc_freq(int argc,char **argv)
{
	int out;
	long sysclk;
	long pclk;
	int tmp;
	
	if(argc<2)return -1;
	pclk=strtoul(argv[1],0,0);
	if(argc>2) sysclk=strtoul(argv[2],0,0);
	else sysclk=33333;
	out = caclulatefreq(sysclk,pclk);
	printf("out=%x\n",out);
	/*inner gpu dc logic fifo pll ctrl,must large then outclk*/
	*(volatile int *)0xbfd00414 = out+1;
	/*output pix1 clock  pll ctrl*/
	*(volatile int *)0xbfd00410 = out;
	/*output pix2 clock pll ctrl */
	*(volatile int *)0xbfd00424 = out;
	tmp = *(volatile int *)0xbfd00424;
	tmp |= 0x110000;
	*(volatile int *)0xbfd00424 = tmp;

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
	int i,j,k,diff,cpu;
};

static int cmd_xrandr(int argc,char **argv)
{
	unsigned int i,j,k,l;
	int mode=-1;
	struct xmode *head=0,*pnode,**p;
	unsigned int length=0;
	unsigned int xres,yres;
	unsigned int val,diff,freq;
	unsigned int cpudiv,ddrdiv;
	unsigned int cpu,ddr;
	unsigned int cpu_arg,ddr_arg;
	unsigned int gclk;
	unsigned int r8030,r8034;

	if(argc<5) return -1;

	xres=strtoul(argv[1],0,0);
	yres=strtoul(argv[2],0,0);
	cpu_arg = strtoul(argv[4],0,0);
	ddr_arg = strtoul(argv[5],0,0);



	for(i=0;i<sizeof(vgamode)/sizeof(struct vga_struc);i++){
		//	  int out;
		if(vgamode[i].hr == xres && vgamode[i].vr == yres){
			mode=i;
			freq = vgamode[i].pclk;
			break;
		}
	}

	if(mode<0){
		printf("\n\n\nunsupported framebuffer resolution\n\n\n");
		return -1;
	}


	for(i=0;i<=0x3f;i++){
		for(j=0;j<=1023;j++){
			for(k=1;k<=15;k++){
				gclk=(33333*(12+i)+33333*j/1024)/2;
				val=gclk/EXTRA_DIV/k;
				if(gclk>660000 || (diff=abs(val-freq))>=1000) continue;

				cpudiv=gclk>cpu_arg?gclk/cpu_arg:1;
				while(gclk/cpudiv>cpu_arg){
					cpudiv++;
				}
				
				cpu = gclk/cpudiv;

				/*sort first cpu,then diff*/
				for(p=&head;*p && (*p)->cpu>cpu;p=&(*p)->next);
				for(;*p && (*p)->cpu == cpu && (*p)->diff < diff;p=&(*p)->next);
				pnode=malloc(sizeof(struct xmode));
				if(pnode){
					pnode->next = *p;
					*p=pnode;
					length++;
				}
				else pnode=*p;
				if(!pnode) continue;
				
				pnode->i = i;
				pnode->j = j;
				pnode->k = k;
				pnode->cpu = cpu;
				pnode->diff = diff;
			}
		}
	}

	printf("i,\tj,\tk,\tval,\tdiff,\tpll,\tcpu\tddr\t8030,\t,8034\n");
	
	for(pnode=head,l=0;pnode;pnode=pnode->next,l++){
		i=pnode->i;
		j=pnode->j;
		k=pnode->k;
		cpu = pnode->cpu;
		diff = pnode->diff;

		gclk=(33333*(12+i)+33333*j/1024)/2;
		val=gclk/EXTRA_DIV/k;

		cpudiv=gclk/cpu;

		ddrdiv=gclk>ddr_arg?gclk/ddr_arg:1;
		while(gclk/ddrdiv>ddr_arg){
			ddrdiv++;
		}
	
		ddr=gclk/ddrdiv;

		r8030=i|(j<<8);
		r8034=(1<<31)|(k<<26)|(1<<25)|(cpudiv<<20)|(1<<19)|(ddrdiv<<14);

		printf("%d:%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%d,\t%x,\t,%x\n",l,i,j,k,val,diff,33333*(12+i+j/1024)/2 ,cpu,ddr, r8030, r8034);
		
		if(l%10==9||!pnode->next){
			char c,buf[10];
			printf("select which one [num|q|enter]?");
			i = read(0, buf, 9);
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
	k=pnode->k;
	cpu = pnode->cpu;
	
	gclk=(33333*(12+i)+33333*j/1024)/2;

	cpudiv=gclk/cpu;

	ddrdiv=gclk>ddr_arg?gclk/ddr_arg:1;
	while(gclk/ddrdiv>ddr_arg){
		ddrdiv++;
	}
	
	ddr=gclk/ddrdiv;

	r8030=i|(j<<8);
	r8034=(1<<31)|(k<<26)|(1<<25)|(cpudiv<<20)|(1<<19)|(ddrdiv<<14);

	{
		char str[256];
		sprintf(str,"set pll_reg0 0x%x;set pll_reg1 0x%x;set xres %d;set yres %d",r8030,r8034,xres,yres);
		do_cmd(str);
	}

	for(pnode=head;pnode;pnode=head){
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
	{"xrandr","xres yres hsync cpufreq(KHZ) ddrfreq(KHZ)", 0, "xrandr xres yres hsync cpufreq(KHZ) ddrfreq(KHZ)", cmd_xrandr, 0, 99, CMD_REPEAT},
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
#endif /*DC_DEBUG*/

