/***************************************************************************
 * Name:
 *     smi712.c
 * License:
 *     2003-2007, Copyright by BLX IC Design Co., Ltd.
 * Description:
 *     smi712 driver
 *
 ***************************************************************************/

#include "smi712.h"
#include "smtc2d.h"

//#if     ENABLE_VGA_SMI712
////////////////////////////////
extern void  video_hw_init();

////////////////////////////////
static void smi_set_timing(struct par_info *hw)
{
        int i=0,j=0;
        u32 m_nScreenStride;
//    video_hw_init(); //xuhua    
    for (j=0;j < numVGAModes;j++) {
                if (VGAMode[j].mmSizeX == hw->width &&
                        VGAMode[j].mmSizeY == hw->height &&
                        VGAMode[j].bpp == hw->bits_per_pixel &&
                        VGAMode[j].hz == hw->hz)
                {
                        smi_mmiowb(0x0,0x3c6);

                        smi_seqw(0,0x1);

                        smi_mmiowb(VGAMode[j].Init_MISC,0x3c2);

                        for (i=0;i<SIZE_SR00_SR04;i++)  /* init SEQ register SR00 - SR04 */
                        {
                                smi_seqw(i,VGAMode[j].Init_SR00_SR04[i]);
                        }

                        for (i=0;i<SIZE_SR10_SR24;i++)  /* init SEQ register SR10 - SR24 */
                        {
                                smi_seqw(i+0x10,VGAMode[j].Init_SR10_SR24[i]);
                        }

                        for (i=0;i<SIZE_SR30_SR75;i++)  /* init SEQ register SR30 - SR75 */
                        {
                                if (((i+0x30) != 0x62) && ((i+0x30) != 0x6a) && ((i+0x30) != 0x6b))
                                        smi_seqw(i+0x30,VGAMode[j].Init_SR30_SR75[i]);
                        }
                        for (i=0;i<SIZE_SR80_SR93;i++)  /* init SEQ register SR80 - SR93 */
                        {
                                smi_seqw(i+0x80,VGAMode[j].Init_SR80_SR93[i]);
                        }
                        for (i=0;i<SIZE_SRA0_SRAF;i++)  /* init SEQ register SRA0 - SRAF */
                        {
                                smi_seqw(i+0xa0,VGAMode[j].Init_SRA0_SRAF[i]);
                        }

                        for (i=0;i<SIZE_GR00_GR08;i++)  /* init Graphic register GR00 - GR08 */
                        {
                                smi_grphw(i,VGAMode[j].Init_GR00_GR08[i]);
                        }

                        for (i=0;i<SIZE_AR00_AR14;i++)  /* init Attribute register AR00 - AR14 */
                        {

                                smi_attrw(i,VGAMode[j].Init_AR00_AR14[i]);
                        }

                        for (i=0;i<SIZE_CR00_CR18;i++)  /* init CRTC register CR00 - CR18 */
                        {
                                smi_crtcw(i,VGAMode[j].Init_CR00_CR18[i]);
                        }

                        for (i=0;i<SIZE_CR30_CR4D;i++)  /* init CRTC register CR30 - CR4D */
                        {
                                smi_crtcw(i+0x30,VGAMode[j].Init_CR30_CR4D[i]);
                        }

                        for (i=0;i<SIZE_CR90_CRA7;i++)  /* init CRTC register CR90 - CRA7 */
                        {
                                smi_crtcw(i+0x90,VGAMode[j].Init_CR90_CRA7[i]);
                        }
                }
        }
        smi_mmiowb(0x67,0x3c2);
        
        /* set VPR registers */
        writel(hw->m_pVPR+0x0C, 0x0);
        writel(hw->m_pVPR+0x40, 0x0);
        /* set data width */
        m_nScreenStride = (hw->width * hw->bits_per_pixel) / 64;

#ifdef CONFIG_VIDEO_8BPP_INDEX
			writel(hw->m_pVPR+0x0,0x00000000);
#elif defined(CONFIG_VIDEO_8BPP)
			writel(hw->m_pVPR+0x0,0x00050000);
#elif defined(CONFIG_VIDEO_16BPP)
			writel(hw->m_pVPR+0x0,0x00020000);
#elif defined(CONFIG_VIDEO_24BPP)
			writel(hw->m_pVPR+0x0,0x00040000);
#else
      writel(hw->m_pVPR+0x0,0x00030000);
#endif
                        
        writel(hw->m_pVPR+0x10, (u32)(((m_nScreenStride + 2) << 16) | m_nScreenStride));
}



/***************************************************************************
 * We need to wake up the LynxEM+, and make sure its in linear memory mode.
 ***************************************************************************/
static inline void 
smi_init_hw(void)
{
#if 0
	outb(0x18, 0x3c4);
    outb(0x11, 0x3c5);
#endif
	linux_outb(0x18, 0x3c4);
    linux_outb(0x11, 0x3c5);
}


#define smtc_2DBaseAddress  (SMILFB + 0x00408000)
#define smtc_2Ddataport   (SMILFB + 0x400000)
#define smtc_RegBaseAddress (SMILFB + 0x00700000)

unsigned long regRead32(unsigned long nOffset)
{
    return *(volatile unsigned long *)(smtc_RegBaseAddress+nOffset);
}


void regWrite32(unsigned long nOffset, unsigned long nData)
{
    *(volatile unsigned long *)(smtc_RegBaseAddress+nOffset) = nData;
}

void SMTC_write2Dreg(unsigned long nOffset, unsigned long nData)
{
    *(volatile unsigned long *)(smtc_2DBaseAddress+nOffset) = nData;
}

void SMTC_write2Ddataport(unsigned long nOffset, unsigned long nData)
{
    *(volatile unsigned long *)(smtc_2Ddataport+nOffset) = nData;
}

static inline unsigned int smtc_seqr(int reg)
{
       *(volatile unsigned char *)(smtc_RegBaseAddress+0x3c4)=reg;
        return *(volatile unsigned char *)(smtc_RegBaseAddress+0x3c5);
}

static inline void smtc_seqw(int reg,int val)
{
       *(volatile unsigned char *)(smtc_RegBaseAddress+0x3c4)=reg;
       *(volatile unsigned char *)(smtc_RegBaseAddress+0x3c5)=val;
}
#define CONFIG_FB_SM7XX 1
#include "smtc2d.c"

int  smi712_init(char * fbaddress,char * ioaddress)
{
  
        u32 smem_size;

        
        smi_init_hw();


				
        hw.m_pLFB = SMILFB = fbaddress;
        hw.m_pMMIO = SMIRegs = SMILFB + 0x00700000; /* ioaddress */
        hw.m_pDPR = hw.m_pLFB + 0x00408000;
        hw.m_pVPR = hw.m_pLFB + 0x0040c000;

        /* now we fix the  mode */
#if 0
		hw.width = 800;
        hw.height = 600;
#endif
#if 1
		hw.width = FB_XSIZE;
        hw.height = FB_YSIZE;
#endif
		hw.bits_per_pixel = FB_COLOR_BITS;
        hw.hz = 60;
        
        if (!SMIRegs)
        {
                printf(" unable to map memory mapped IO\n");
                return -1;
        }

		/*xuhua*/
		smi_seqw(0x21,0x00); 
		/*****/
        smi_seqw(0x62,0x7A);
        smi_seqw(0x6a,0x0c);
        smi_seqw(0x6b,0x02);
        smem_size = 0x00400000;

        /* LynxEM+ memory dection */
        *(u32 *)(SMILFB + 4) = 0xAA551133;
        if (*(u32 *)(SMILFB + 4) != 0xAA551133)
        {
                smem_size = 0x00200000;
                /* Program the MCLK to 130 MHz */
                smi_seqw(0x6a,0x12);
                smi_seqw(0x6b,0x02);
                smi_seqw(0x62,0x3e);
        }

        smi_set_timing(&hw);
                
        printf("Silicon Motion, Inc. LynxEM+ Init complete.\n");

		AutodeInit();

        return 0;

}
#include <pmon.h>
#include <machine/pio.h>

#define udelay delay

#define SMI_LUT_MASK            (smtc_RegBaseAddress + 0x03c6)    /* lut mask reg */
#define SMI_LUT_START           (smtc_RegBaseAddress + 0x03c8)    /* lut start index */
#define SMI_LUT_RGB             (smtc_RegBaseAddress + 0x03c9)    /* lut colors auto incr.*/
#define SMI_INDX_ATTR           (smtc_RegBaseAddress + 0x03c0)    /* attributes index reg */

/*******************************************************************************
 *
 * Set a RGB color in the LUT (8 bit index)
 */
void video_set_lut (
        unsigned int index,        /* color number */
        unsigned char r,              /* red */
        unsigned char g,              /* green */
        unsigned char b               /* blue */
        )
{
        outb (SMI_LUT_MASK,  0xff);

        outb (SMI_LUT_START, (char)index);

        outb (SMI_LUT_RGB, r>>2);    /* red */
        udelay (10);
        outb (SMI_LUT_RGB, g>>2);    /* green */
        udelay (10);
        outb (SMI_LUT_RGB, b>>2);    /* blue */
        udelay (10);
}


//----------------------------------
#include <pmon.h>
#if 0
static inline int smi_attrr(int reg)
{
		smi_mmiorb(0x3da);
		smi_mmiowb(reg, 0x3c0);
        return smi_mmiorb(0x3c1);
}
int dumpsmi(int argc,char **argv)
{
unsigned int i,data;

printf("3c2:%02x\n",smi_mmiorb(0x3c2));
printf("3da:%02x\n",linux_inb(0x3da));
printf("3ca:%02x\n",linux_inb(0x3ca));
printf("3c6:%02x\n",smi_mmiorb(0x3c6));


                        for (i=0;i<SIZE_GR00_GR08;i++)  /* init Graphic register GR00 - GR08 */
                        {
                                printf("gr%02x:%02x\n",i,smi_grphr(i));
                        }

                        for (i=0;i<SIZE_AR00_AR14;i++)  /* init Attribute register AR00 - AR14 */
                        {
                                printf("ar%02x:%02x\n",i, smi_attrr(i));
                        }

                        for (i=0;i<SIZE_CR00_CR18;i++)  /* init CRTC register CR00 - CR18 */
                        {
                                printf("cr%02x:%02x\n",i,smi_crtcr(i));
                        }

                        for (i=0;i<SIZE_CR30_CR4D;i++)  /* init CRTC register CR30 - CR4D */
                        {
                                printf("cr%02x:%02x\n",i+0x30,smi_crtcr(i+0x30));
                        }

                        for (i=0;i<SIZE_CR90_CRA7;i++)  /* init CRTC register CR90 - CRA7 */
                        {
                                printf("cr%02x:%02x\n",i+0x90,smi_crtcr(i+0x30));
                        }


printf("index 0x3c4,data 0x3c5\n");
for(i=0;i<256;i++)
{
data=smi_seqr(i);
printf("%02x:%02x\n",i,data);
}
       
{
char str[100];
printf("vpr as bellow:\n");
sprintf(str,"pcs -1;d4 0x%x 64;",(long)(hw.m_pVPR)&0x1fffffff);
do_cmd(str);
printf("dpr as bellow:\n");
sprintf(str,"pcs -1;d4 0x%x 64;",(long)(hw.m_pDPR )&0x1fffffff);
do_cmd(str);
printf("mmio(fb+7M:\n");
sprintf(str,"pcs -1;d1 0x%x 0x400;",(long)(hw.m_pMMIO )&0x1fffffff);
do_cmd(str);
}
return 0;
}

#endif
int info712(int ac,char *av[])
{
printf("smtc_RegBaseAddress=0x%x\nsmtc_2DBaseAddress=0x%x\nsmtc_2Ddataport=0x%x\n",smtc_RegBaseAddress,smtc_2DBaseAddress,smtc_2Ddataport);
return 0;

}
#if 1
static const Cmd Cmds[] =
{
	{"MyCmds"},
//	{"dumpsmi","",0,"sumpsmi",dumpsmi,1,1,CMD_REPEAT},
	{"info712","",0,"info712",info712,1,1,CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
	cmdlist_expand(Cmds, 1);
}
#endif



//#endif

/* ---------------------------------------------------------------------- */
// $Log$
