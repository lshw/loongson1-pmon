/* Initialize radeon 7000 mobility from scratch
 * Some bits copied from linux kernel framebuffer driver.
 * Author: Fuxin Zhang, zhangfx@lemote.com
 * Copyright 2006, Lemote incorporation.
 */
#ifdef RADEON7000

#include <stdio.h>
#include <stdlib.h>

#include <dev/pci/pcivar.h>
#include "xf86int10.h"
#include "xf86x86emu.h"
#include "linux/io.h"

#include "radeon_reg.h"
#include "mod_framebuffer.h"

static unsigned long mmbase = 0;
static unsigned long iobase = 0;
extern struct pci_device *vga_dev;

#define MMINB(addr)  *(volatile unsigned char*)(mmbase + (addr))
#define MMOUTB(addr,v)  (*(volatile unsigned char*)(mmbase + (addr)) = (v))
#define MMINH(addr)  *(volatile unsigned short*)(mmbase + (addr))
#define MMOUTH(addr,v)  (*(volatile unsigned short*)(mmbase + (addr)) = (v))
#define MMINL(addr)  *(volatile unsigned int*)(mmbase + (addr))
#define MMOUTL(addr,v)  (*(volatile unsigned int*)(mmbase + (addr)) = (v))
#define OUTL(addr,v)  (*(volatile unsigned int*)(iobase + (addr)) = (v))
#define OUTW(addr,v)  (*(volatile unsigned short*)(iobase + (addr)) = (v))
#define OUTB(addr,v)  (*(volatile unsigned char*)(iobase + (addr)) = (v))

extern void delay(int msec);
#define mdelay  delay

static unsigned int
INPLL (unsigned int addr)
{
  unsigned int v;
  MMOUTB (0x8, (addr & 0x3f));
  mdelay (5);
  v = MMINL (0xc);
  mdelay (5);
  return v;
}

static void
OUTPLL (unsigned int addr, unsigned int v)
{
  MMOUTB (0x8, (addr & 0x3f) | 0x80);
  mdelay (5);
  MMOUTL (0xc, v);
  MMOUTB (0x8, 0);
  //mdelay (5);
}

#define OUTPLLP(addr, val, mask)                                 \
	do {                                                                    \
		    CARD32 tmp_ = INPLL(addr);                                   \
		    tmp_ &= (mask);                                                     \
		    tmp_ |= ((val) & ~(mask));                                          \
		    OUTPLL(addr, tmp_);                                          \
	} while (0)

#define OUTREGP(addr, val, mask)   \
	do {                           \
		CARD32 tmp = MMINL(addr);  \
		tmp &= (mask);             \
		tmp |= ((val) & ~(mask));  \
		MMOUTL(addr, tmp);         \
	} while (0)

static unsigned int
INMC (unsigned int addr)
{
  MMOUTL (0x1f8, (addr & 0xff));
  mdelay (5);
  return MMINL (0x1fc);
}


static void
OUTMC (unsigned int addr, unsigned int v)
{
  MMOUTL (0x1f8, (addr & 0xff) | 0x100);
  mdelay (5);
  MMOUTL (0x1fc, v);
  mdelay (5);
}

//#define DEBUG
#ifdef DEBUG
typedef enum
{ R, W, RW } reg_type;
struct ati_reg
{
  char *name;
  reg_type type;
  int width;
  int mmaddr;
  int ioaddr;
  int pciaddr;
};

struct ati_reg reg_list[] = {
  {"AGP_BASE", RW, 32, 0x170},
  {"DEVICE_ID", R, 16, 0xF02},
  {"VENDOR_ID", R, 16, 0xF00},
  {"COMMAND", RW, 16, 0xF04},
  {"STATUS", RW, 16, 0xF06},
  {"REVISION_ID", R, 8, 0xF08},
  {"IO_BASE", RW, 32, 0xF14},
  {"REG_BASE", RW, 32, 0xF18},
  {"MEM_BASE", RW, 32, 0xF10},
  {"ADAPTER_ID_W", RW, 32, 0xF4C},
  {"BASE_CODE", R, 8, 0xF0B},
  {"ADAPTER_ID", R, 32, 0xF2C},
  {"BIOS_ROM", RW, 32, 0xF30},
  {"SUB_CLASS", R, 8, 0xF0A},
  {"BIST", R, 8, 0xF0F},
  {"CAPABILITIES_PTR", R, 32, 0xF34},
  {"CONFIG_CNTL", RW, 32, 0xE0},
  {"CONFIG_MEMSIZE", RW, 32, 0xF8},
  {"CONFIG_APER_0_BASE", R, 32, 0x100},
  {"CONFIG_APER_1_BASE", R, 32, 0x104},
  {"CONFIG_APER_SIZE", R, 32, 0x108},
  {"CONFIG_REG_1_BASE", R, 32, 0x10C},
  {"CONFIG_REG_APER_SIZE", R, 32, 0x110},
  {"HEADER", R, 8, 0xF0E},
  {"INTERRUPT_LINE", RW, 8, 0xF3C},
  {"INTERRUPT_PIN", R, 8, 0xF3D},
  {"LATENCY", RW, 8, 0xF0D},
  {"MAX_LATENCY", R, 8, 0xF3F},
  {"REGPROG_INF", R, 8, 0xF09},
  {"CACHE_LINE", RW, 8, 0xF0C},
  {"MIN_GRANT", R, 8, 0xF3E},
  {"BUS_CNTL", RW, 32, 0x30},
  {"BUS_CNTL1", RW, 32, 0x34},
  {"HI_STAT", RW, 32, 0x4C},
  {"BM_STATUS", R, 32, 0x160},
  {"AGP_COMMAND", RW, 32, 0xF60},
  {"AGP_CNTL", RW, 32, 0x174},
  {"AGP_CAP_ID", R, 32, 0xF58},
  {"AGP_STATUS", R, 32, 0xF5C},
  {"MM_INDEX", RW, 32, 0x0},
  {"MM_DATA", RW, 32, 0x4},
  {"PAD_CTLR_STRENGTH", RW, 32, 0x168},
  {"PAD_CTLR_UPDATE", RW, 32, 0x16C},
  {"PAD_AGPINPUT_DELAY", RW, 32, 0x164},
  {"AIC_CTRL", RW, 32, 0x1D0},
  {"AIC_STAT", R, 32, 0x1D4},
  {"AIC_PT_BASE", RW, 32, 0x1D8},
  {"AIC_LO_ADDR", RW, 32, 0x1DC},
  {"AIC_HI_ADDR", RW, 32, 0x1E0},
  {"AIC_TLB_ADDR", R, 32, 0x1E4},
  {"AIC_TLB_DATA", R, 32, 0x1E8},
  {"PMI_CAP_ID", R, 8, 0xF50},
  {"PMI_NXT_CAP_PTR", R, 8, 0xF51},
  {"PM_STATUS", RW, 16, 0xF54},
  {"PMI_PMC_REG", R, 16, 0xF52},
  {"PMI_DATA", R, 8, 0xF57},
  {"CLOCK_CNTL_INDEX", RW, 32, 0x8},
  {"CLOCK_CNTL_DATA", RW, 32, 0xC},
  {"AGP_BASE", RW, 32, 0x170},
  {"MEM_CNTL", RW, 32, 0x140},
  {"EXT_MEM_CNTL", RW, 32, 0x144},
  {"MC_FB_LOCATION", RW, 32, 0x148},
  {"MC_AGP_LOCATION", RW, 32, 0x14C},
  {"MEM_INIT_LATENCY_TIMER", RW, 32, 0x154},
  {"MEM_SDRAM_MODE_REG", RW, 32, 0x158},
  {"MEM_IO_CNTL_A0", RW, 32, 0x178},
  {"MEM_IO_CNTL_A1", RW, 32, 0x17C},
  {"MEM_IO_OE_CNTL", RW, 32, 0x18C},
  {"MC_DEBUG", RW, 32, 0x188},
  {"MC_STATUS", R, 32, 0x150},
  {"RBBM_CNTL", RW, 32, 0xE44},
  {"RBBM_SOFT_RESET", RW, 32, 0xE48},
  {"RBBM_STATUS", R, 32, 0x1740},
  {"ISYNC_CNTL", RW, 32, 0x1724},
  {"RBBM_GUICNTL", RW, 32, 0x172C},
  {"RBBM_CMDFIFO_ADDR", W, 32, 0xE70},
  {"RBBM_CMDFIFO_DATAL", R, 32, 0xE74},
  {"RBBM_CMDFIFO_DATAH", R, 32, 0xE78},
  {"RBBM_CMDFIFO_STAT", R, 32, 0xE7C},
  {"WAIT_UNTIL", RW, 32, 0x1720},
  {"NQWAIT_UNTIL", W, 32, 0xE50},
  {"RBBM_DEBUG", RW, 32, 0xE6C},
  {"GEN_INT_CNTL", RW, 32, 0x40},
  {"GEN_INT_STATUS", RW, 32, 0x44},
  {"DST_OFFSET", RW, 32, 0x1404},
  {"DST_PITCH", RW, 32, 0x1408},
  {"DST_WIDTH", RW, 32, 0x140C},
  {"DST_HEIGHT", RW, 32, 0x1410},
  {"SRC_X", RW, 32, 0x1414},
  {"SRC_Y", RW, 32, 0x1418},
  {"DST_X", RW, 32, 0x141C},
  {"DST_Y", RW, 32, 0x1420},
  {"SRC_PITCH_OFFSET", W, 32, 0x1428},
  {"DST_PITCH_OFFSET", W, 32, 0x142C},
  {"DEFAULT_PITCH_OFFSET", RW, 32, 0x16E0},
  {"DEFAULT2_PITCH_OFFSET", RW, 32, 0x16F8},
  {"SRC_Y_X", W, 32, 0x1434},
  {"DST_Y_X", W, 32, 0x1438},
  {"DST_HEIGHT_WIDTH", W, 32, 0x143C},
  {"DP_GUI_MASTER_CNTL", W, 32, 0x146C},
  {"BRUSH_Y_X", RW, 32, 0x1474},
  {"DP_BRUSH_BKGD_CLR", RW, 32, 0x1478},
  {"DP_BRUSH_FRGD_CLR", RW, 32, 0x147C},
  {"DP_CNTL_XDIR_YDIR_YMAJOR", W, 32, 0x16D0},
  {"BRUSH_DATA0", W, 32, 0x1480},
  {"BRUSH_DATA1", W, 32, 0x1484},
  {"BRUSH_DATA2", W, 32, 0x1488},
  {"BRUSH_DATA3", W, 32, 0x148C},
  {"BRUSH_DATA4", W, 32, 0x1490},
  {"BRUSH_DATA5", W, 32, 0x1494},
  {"BRUSH_DATA6", W, 32, 0x1498},
  {"BRUSH_DATA7", W, 32, 0x149C},
  {"BRUSH_DATA8", W, 32, 0x14A0},
  {"BRUSH_DATA9", W, 32, 0x14A4},
  {"BRUSH_DATA10", W, 32, 0x14A8},
  {"BRUSH_DATA11", W, 32, 0x14AC},
  {"BRUSH_DATA12", W, 32, 0x14B0},
  {"BRUSH_DATA13", W, 32, 0x14B4},
  {"BRUSH_DATA14", W, 32, 0x14B8},
  {"BRUSH_DATA15", W, 32, 0x14BC},
  {"BRUSH_DATA16", W, 32, 0x14C0},
  {"BRUSH_DATA17", W, 32, 0x14C4},
  {"BRUSH_DATA18", W, 32, 0x14C8},
  {"BRUSH_DATA19", W, 32, 0x14CC},
  {"BRUSH_DATA20", W, 32, 0x14D0},
  {"BRUSH_DATA21", W, 32, 0x14D4},
  {"BRUSH_DATA22", W, 32, 0x14D8},
  {"BRUSH_DATA23", W, 32, 0x14DC},
  {"BRUSH_DATA24", W, 32, 0x14E0},
  {"BRUSH_DATA25", W, 32, 0x14E4},
  {"BRUSH_DATA26", W, 32, 0x14E8},
  {"BRUSH_DATA27", W, 32, 0x14EC},
  {"BRUSH_DATA28", W, 32, 0x14F0},
  {"BRUSH_DATA29", W, 32, 0x14F4},
  {"BRUSH_DATA30", W, 32, 0x14F8},
  {"BRUSH_DATA31", W, 32, 0x14FC},
  {"BRUSH_DATA32", W, 32, 0x1500},
  {"BRUSH_DATA33", W, 32, 0x1504},
  {"BRUSH_DATA34", W, 32, 0x1508},
  {"BRUSH_DATA35", W, 32, 0x150C},
  {"BRUSH_DATA36", W, 32, 0x1510},
  {"BRUSH_DATA37", W, 32, 0x1514},
  {"BRUSH_DATA38", W, 32, 0x1518},
  {"BRUSH_DATA39", W, 32, 0x151C},
  {"BRUSH_DATA40", W, 32, 0x1520},
  {"BRUSH_DATA41", W, 32, 0x1524},
  {"BRUSH_DATA42", W, 32, 0x1528},
  {"BRUSH_DATA43", W, 32, 0x152C},
  {"BRUSH_DATA44", W, 32, 0x1530},
  {"BRUSH_DATA45", W, 32, 0x1534},
  {"BRUSH_DATA46", W, 32, 0x1538},
  {"BRUSH_DATA47", W, 32, 0x153C},
  {"BRUSH_DATA48", W, 32, 0x1540},
  {"BRUSH_DATA49", W, 32, 0x1544},
  {"BRUSH_DATA50", W, 32, 0x1548},
  {"BRUSH_DATA51", W, 32, 0x154C},
  {"BRUSH_DATA52", W, 32, 0x1550},
  {"BRUSH_DATA53", W, 32, 0x1554},
  {"BRUSH_DATA54", W, 32, 0x1558},
  {"BRUSH_DATA55", W, 32, 0x155C},
  {"BRUSH_DATA56", W, 32, 0x1560},
  {"BRUSH_DATA57", W, 32, 0x1564},
  {"BRUSH_DATA58", W, 32, 0x1568},
  {"BRUSH_DATA59", W, 32, 0x156C},
  {"BRUSH_DATA60", W, 32, 0x1570},
  {"BRUSH_DATA61", W, 32, 0x1574},
  {"BRUSH_DATA62", W, 32, 0x1578},
  {"BRUSH_DATA63", W, 32, 0x157C},
  {"DST_WIDTH_X", W, 32, 0x1588},
  {"DST_HEIGHT_WIDTH_8", W, 32, 0x158C},
  {"SRC_X_Y", W, 32, 0x1590},
  {"DST_X_Y", W, 32, 0x1594},
  {"DST_WIDTH_HEIGHT", W, 32, 0x1598},
  {"DST_WIDTH_X_INCY", W, 32, 0x159C},
  {"DST_HEIGHT_Y", W, 32, 0x15A0},
  {"SRC_OFFSET", RW, 32, 0x15AC},
  {"SRC_PITCH", RW, 32, 0x15B0},
  {"CLR_CMP_CNTL", W, 32, 0x15C0},
  {"CLR_CMP_CLR_SRC", W, 32, 0x15C4},
  {"CLR_CMP_CLR_DST", W, 32, 0x15C8},
  {"CLR_CMP_MSK", W, 32, 0x15CC},
  {"DP_DST_ENDIAN", W, 32, 0x15D0},
  {"DP_SRC_ENDIAN", W, 32, 0x15D4},
  {"DP_SRC_FRGD_CLR", RW, 32, 0x15D8},
  {"DP_SRC_BKGD_CLR", RW, 32, 0x15DC},
  {"DST_LINE_START", RW, 32, 0x1600},
  {"DST_LINE_END", RW, 32, 0x1604},
  {"DST_LINE_PATCOUNT", RW, 32, 0x1608},
  {"SC_LEFT", RW, 32, 0x1640},
  {"SC_RIGHT", RW, 32, 0x1644},
  {"SC_TOP", RW, 32, 0x1648},
  {"SC_BOTTOM", RW, 32, 0x164C},
  {"SRC_SC_RIGHT", RW, 32, 0x1654},
  {"SRC_SC_BOTTOM", RW, 32, 0x165C},
  {"DP_CNTL", RW, 32, 0x16C0},
  {"DP_DATATYPE", RW, 32, 0x16C4},
  {"DP_MIX", RW, 32, 0x16C8},
  {"DP_WRITE_MSK", W, 32, 0x16CC},
  {"SC_TOP_LEFT", W, 32, 0x16EC},
  {"SC_BOTTOM_RIGHT", W, 32, 0x16F0},
  {"DEFAULT_SC_BOTTOM_RIGHT", RW, 32, 0x16E8},
  {"DEFAULT2_SC_BOTTOM_RIGHT", RW, 32, 0x16DC},
  {"SRC_SC_BOTTOM_RIGHT", W, 32, 0x16F4},
  {"DST_TILE", RW, 32, 0x1700},
  {"SRC_TILE", RW, 32, 0x1704},
  {"SRC_CLUT_ADDRESS", RW, 32, 0x1780},
  {"SRC_CLUT_DATA", W, 32, 0x1784},
  {"SRC_CLUT_DATA_RD", R, 32, 0x1788},
  {"HOST_DATA0", RW, 32, 0x17C0},
  {"HOST_DATA1", W, 32, 0x17C4},
  {"HOST_DATA2", W, 32, 0x17C8},
  {"HOST_DATA3", W, 32, 0x17CC},
  {"HOST_DATA4", W, 32, 0x17D0},
  {"HOST_DATA5", W, 32, 0x17D4},
  {"HOST_DATA6", W, 32, 0x17D8},
  {"HOST_DATA7", W, 32, 0x17DC},
  {"HOST_DATA_LAST", W, 32, 0x17E0},
  {"DP_XOP", W, 32, 0x17F8},
  {"DSTCACHE_MODE", W, 32, 0x1710},
  {"DSTCACHE_CTLSTAT", W, 32, 0x1714},
  {"PD2_DATA", W, 32, 0x1718},
  {"CP_RB_CNTL", RW, 32, 0x704},
  {"CP_RB_BASE", RW, 32, 0x700},
  {"CP_RB_RPTR_ADDR", RW, 32, 0x70C},
  {"CP_RB_RPTR", R, 32, 0x710},
  {"CP_RB_RPTR_WR", RW, 32, 0x71C},
  {"CP_RB_WPTR", RW, 32, 0x714},
  {"CP_RB_WPTR_DELAY", RW, 32, 0x718},
  {"CP_IB_BASE", RW, 32, 0x738},
  {"CP_IB_BUFSZ", RW, 32, 0x73C},
  {"CP_CSQ_CNTL", RW, 32, 0x740},
  {"SCRATCH_UMSK", RW, 32, 0x770},
  {"SCRATCH_ADDR", RW, 32, 0x774},
  {"CP_ME_CNTL", RW, 32, 0x7D0},
  {"CP_ME_RAM_ADDR", RW, 32, 0x7D4},
  {"CP_ME_RAM_RADDR", W, 32, 0x7D8},
  {"CP_ME_RAM_DATAH", RW, 32, 0x7DC},
  {"CP_ME_RAM_DATAL", RW, 32, 0x7E0},
  {"CP_CSQ_ADDR", W, 32, 0x7F0},
  {"CP_CSQ_DATA", R, 32, 0x7F4},
  {"CP_CSQ_STAT", R, 32, 0x7F8},
  {"SCRATCH_REG0", RW, 32, 0x15E0},
  {"SCRATCH_REG1", RW, 32, 0x15E4},
  {"SCRATCH_REG2", RW, 32, 0x15E8},
  {"SCRATCH_REG3", RW, 32, 0x15EC},
  {"SCRATCH_REG4", RW, 32, 0x15F0},
  {"SCRATCH_REG5", RW, 32, 0x15F4},
  {"CP_CSQ_APER_PRIMARY", RW, 32, 0x11FC},
  {"CP_CSQ_APER_INDIRECT", RW, 32, 0x13FC},
  {"CP_DEBUG", RW, 32, 0x7EC},
  {"CP_STAT", R, 32, 0x7C0},
  {"DMA_GUI_TABLE_ADDR", W, 32, 0x780},
  {"DMA_GUI_SRC_ADDR", R, 32, 0x784},
  {"DMA_GUI_DST_ADDR", R, 32, 0x788},
  {"DMA_GUI_COMMAND", R, 32, 0x78C},
  {"DMA_GUI_STATUS", RW, 32, 0x790},
  {"DMA_GUI_ACT_DSCRPTR", R, 32, 0x794},
  {"DISPLAY_BASE_ADDR", RW, 32, 0x23C},
  {"DISP_OUTPUT_CNTL", RW, 32, 0xD64},
  {"DISP_MERGE_CNTL", RW, 32, 0xD60},
  {"DISP2_MERGE_CNTL", RW, 32, 0xD68},
  {"DISP_MISC_CNTL", RW, 32, 0xD00},
  {"DISP_PWR_MAN", RW, 32, 0xD08},
  {"DISP_TEST_DEBUG_CNTL", RW, 32, 0xD10},
  {"DISP_HW_DEBUG", RW, 32, 0xD14},
  {"DAC_CNTL", RW, 32, 0x58},
  {"DAC_EXT_CNTL", RW, 32, 0x280},
  {"DAC_CRC_SIG1", R, 32, 0xD18},
  {"DAC_CRC_SIG2", R, 32, 0xD1C},
  {"DAC_CRC2_SIG1", R, 32, 0xD70},
  {"DAC_CRC2_SIG2", R, 32, 0xD74},
  {"DAC_CNTL2", RW, 32, 0x7C},
  {"DAC_MACRO_CNTL", RW, 32, 0xD04},
  {"GRPH_BUFFER_CNTL", RW, 32, 0x2F0},
  {"VGA_BUFFER_CNTL", RW, 32, 0x2F4},
  {"GRPH2_BUFFER_CNTL", RW, 32, 0x3F0},
  {"CRTC_H_TOTAL_DISP", RW, 32, 0x200},
  {"CRTC_H_SYNC_STRT_WID", RW, 32, 0x204},
  {"CRTC_V_TOTAL_DISP", RW, 32, 0x208},
  {"CRTC_V_SYNC_STRT_WID", RW, 32, 0x20C},
  {"CRTC_VLINE_CRNT_VLINE", RW, 32, 0x210},
  {"CRTC_CRNT_FRAME", R, 32, 0x214},
  {"CRTC_GUI_TRIG_VLINE", RW, 32, 0x218},
  {"CRTC_DEBUG", RW, 32, 0x21C},
  {"CRTC_OFFSET_RIGHT", RW, 32, 0x220},
  {"CRTC_OFFSET", RW, 32, 0x224},
  {"CRTC_OFFSET_CNTL", RW, 32, 0x228},
  {"CRTC_PITCH", RW, 32, 0x22C},
  {"CRT_CRTC_H_SYNC_STRT_WID", RW, 32, 0x258},
  {"CRT_CRTC_V_SYNC_STRT_WID", RW, 32, 0x25C},
  {"CRTC_MORE_CNTL", RW, 32, 0x27C},
  {"CRTC_GEN_CNTL", RW, 32, 0x50},
  {"CRTC_EXT_CNTL", RW, 32, 0x54},
  {"CRTC2_H_TOTAL_DISP", RW, 32, 0x300},
  {"CRTC2_H_SYNC_STRT_WID", RW, 32, 0x304},
  {"CRTC2_V_TOTAL_DISP", RW, 32, 0x308},
  {"CRTC2_V_SYNC_STRT_WID", RW, 32, 0x30C},
  {"CRTC2_VLINE_CRNT_VLINE", RW, 32, 0x310},
  {"CRTC2_CRNT_FRAME", R, 32, 0x314},
  {"CRTC2_GUI_TRIG_VLINE", RW, 32, 0x318},
  {"CRTC2_DEBUG", RW, 32, 0x31C},
  {"CRTC2_OFFSET", RW, 32, 0x324},
  {"CRTC2_OFFSET_CNTL", RW, 32, 0x328},
  {"CRTC2_PITCH", RW, 32, 0x32C},
  {"CRTC2_DISPLAY_BASE_ADDR", RW, 32, 0x33C},
  {"CRTC2_GEN_CNTL", RW, 32, 0x3F8},
  {"CRTC2_STATUS", RW, 32, 0x3FC},
  {"CRTC_STATUS", RW, 32, 0x5C},
  {"GPIO_VGA_DDC", RW, 32, 0x60},
  {"GPIO_DVI_DDC", RW, 32, 0x64},
  {"GPIO_MONID", RW, 32, 0x68},
  {"GPIO_CRT2_DDC", RW, 32, 0x6C},
  {"OV0_Y_X_START", RW, 32, 0x400},
  {"OV0_Y_X_END", RW, 32, 0x404},
  {"OV0_PIPELINE_CNTL", RW, 32, 0x408},
  {"OV0_REG_LOAD_CNTL", RW, 32, 0x410},
  {"OV0_SCALE_CNTL", RW, 32, 0x420},
  {"OV0_V_INC", RW, 32, 0x424},
  {"OV0_P1_V_ACCUM_INIT", RW, 32, 0x428},
  {"OV0_P23_V_ACCUM_INIT", RW, 32, 0x42C},
  {"OV0_P1_BLANK_LINES_AT_TOP", RW, 32, 0x430},
  {"OV0_P23_BLANK_LINES_AT_TOP", RW, 32, 0x434},
  {"OV0_BASE_ADDR", RW, 32, 0x43C},
  {"OV0_VID_BUF0_BASE_ADRS", RW, 32, 0x440},
  {"OV0_VID_BUF1_BASE_ADRS", RW, 32, 0x444},
  {"OV0_VID_BUF2_BASE_ADRS", RW, 32, 0x448},
  {"OV0_VID_BUF3_BASE_ADRS", RW, 32, 0x44C},
  {"OV0_VID_BUF4_BASE_ADRS", RW, 32, 0x450},
  {"OV0_VID_BUF5_BASE_ADRS", RW, 32, 0x454},
  {"OV0_VID_BUF_PITCH0_VALUE", RW, 32, 0x460},
  {"OV0_VID_BUF_PITCH1_VALUE", RW, 32, 0x464},
  {"OV0_AUTO_FLIP_CNTRL", RW, 32, 0x470},
  {"OV0_DEINTERLACE_PATTERN", RW, 32, 0x474},
  {"OV0_SUBMIT_HISTORY", R, 32, 0x478},
  {"OV0_H_INC", RW, 32, 0x480},
  {"OV0_STEP_BY", RW, 32, 0x484},
  {"OV0_P1_H_ACCUM_INIT", RW, 32, 0x488},
  {"OV0_P23_H_ACCUM_INIT", RW, 32, 0x48C},
  {"OV0_P1_X_START_END", RW, 32, 0x494},
  {"OV0_P2_X_START_END", RW, 32, 0x498},
  {"OV0_P3_X_START_END", RW, 32, 0x49C},
  {"OV0_FILTER_CNTL", RW, 32, 0x4A0},
  {"OV0_FOUR_TAP_COEF_0", RW, 32, 0x4B0},
  {"OV0_FOUR_TAP_COEF_1", RW, 32, 0x4B4},
  {"OV0_FOUR_TAP_COEF_2", RW, 32, 0x4B8},
  {"OV0_FOUR_TAP_COEF_3", RW, 32, 0x4BC},
  {"OV0_FOUR_TAP_COEF_4", RW, 32, 0x4C0},
  {"OV0_FLAG_CNTRL", RW, 32, 0x4DC},
  {"OV0_SLICE_CNTL", RW, 32, 0x4E0},
  {"OV0_VID_KEY_CLR_LOW", RW, 32, 0x4E4},
  {"OV0_VID_KEY_CLR_HIGH", RW, 32, 0x4E8},
  {"OV0_GRPH_KEY_CLR_LOW", RW, 32, 0x4EC},
  {"OV0_GRPH_KEY_CLR_HIGH", RW, 32, 0x4F0},
  {"OV0_KEY_CNTL", RW, 32, 0x4F4},
  {"OV0_TEST", RW, 32, 0x4F8},
  {"OV0_LIN_TRANS_A", RW, 32, 0xD20},
  {"OV0_LIN_TRANS_B", RW, 32, 0xD24},
  {"OV0_LIN_TRANS_C", RW, 32, 0xD28},
  {"OV0_LIN_TRANS_D", RW, 32, 0xD2C},
  {"OV0_LIN_TRANS_E", RW, 32, 0xD30},
  {"OV0_LIN_TRANS_F", RW, 32, 0xD34},
  {"OV0_GAMMA_0_F", RW, 32, 0xD40},
  {"OV0_GAMMA_10_1F", RW, 32, 0xD44},
  {"OV0_GAMMA_20_3F", RW, 32, 0xD48},
  {"OV0_GAMMA_40_7F", RW, 32, 0xD4C},
  {"OV0_GAMMA_380_3BF", RW, 32, 0xD50},
  {"OV0_GAMMA_3C0_3FF", RW, 32, 0xD54},
  {"OV1_Y_X_START", RW, 32, 0x600},
  {"OV1_Y_X_END", RW, 32, 0x604},
  {"OV1_PIPELINE_CNTL", RW, 32, 0x608},
  {"CUR_OFFSET", RW, 32, 0x260},
  {"CUR_HORZ_VERT_POSN", RW, 32, 0x264},
  {"CUR_HORZ_VERT_OFF", RW, 32, 0x268},
  {"CUR_CLR0", RW, 32, 0x26C},
  {"CUR_CLR1", RW, 32, 0x270},
  {"CUR2_OFFSET", RW, 32, 0x360},
  {"CUR2_HORZ_VERT_POSN", RW, 32, 0x364},
  {"CUR2_HORZ_VERT_OFF", RW, 32, 0x368},
  {"CUR2_CLR0", RW, 32, 0x36C},
  {"CUR2_CLR1", RW, 32, 0x370},
  {"OVR2_CLR", RW, 32, 0x330},
  {"OVR2_WID_LEFT_RIGHT", RW, 32, 0x334},
  {"OVR2_WID_TOP_BOTTOM", RW, 32, 0x338},
  {"OVR_CLR", RW, 32, 0x230},
  {"OVR_WID_LEFT_RIGHT", RW, 32, 0x234},
  {"OVR_WID_TOP_BOTTOM", RW, 32, 0x238},
  {"TMDS_CNTL", RW, 32, 0x294},
  {"TMDS_SYNC_CHAR_SETA", RW, 32, 0x298},
  {"TMDS_SYNC_CHAR_SETB", RW, 32, 0x29C},
  {"TMDS_CRC", R, 32, 0x2A0},
  {"TMDS_TRANSMITTER_CNTL", RW, 32, 0x2A4},
  {"TMDS_PLL_CNTL", RW, 32, 0x2A8},
  {"TMDS_PATTERN_GEN_SEED", RW, 32, 0x2AC},
  {"DVI_I2C_CNTL_0", RW, 32, 0x2E0},
  {"DVI_I2C_CNTL_1", RW, 32, 0x2E4},
  {"DVI_I2C_DATA", RW, 32, 0x2E8},
  {"FP_H2_SYNC_STRT_WID", RW, 32, 0x3C4},
  {"FP_V2_SYNC_STRT_WID", RW, 32, 0x3C8},
  {"FP_GEN_CNTL", RW, 32, 0x284},
  {"FP2_GEN_CNTL", RW, 32, 0x288},
  {"FP_HORZ_STRETCH", RW, 32, 0x28C},
  {"FP_VERT_STRETCH", RW, 32, 0x290},
  {"FP_H_SYNC_STRT_WID", RW, 32, 0x2C4},
  {"FP_V_SYNC_STRT_WID", RW, 32, 0x2C8},
  {"FP_HORZ_VERT_ACTIVE", RW, 32, 0x278},
  {"FP_CRTC_H_TOTAL_DISP", RW, 32, 0x250},
  {"FP_CRTC_V_TOTAL_DISP", RW, 32, 0x254},
  {"PALETTE_INDEX", RW, 32, 0xB0},
  {"PALETTE_DATA", RW, 32, 0xB4},
  {"PALETTE_30_DATA", RW, 32, 0xB8},
  {"RMX_HORZ_PHASE", RW, 32, 0xDBC},
  {"AUX_WINDOW_HORZ_CNTL", RW, 32, 0x2D8},
  {"AUX_WINDOW_VERT_CNTL", RW, 32, 0x2DC},
  {"SNAPSHOT_VH_COUNTS", R, 32, 0x240},
  {"SNAPSHOT_F_COUNT", R, 32, 0x244},
  {"N_VIF_COUNT", RW, 32, 0x248},
  {"SNAPSHOT_VIF_COUNT", RW, 32, 0x24C},
  {"SNAPSHOT2_VH_COUNTS", R, 32, 0x340},
  {"SNAPSHOT2_F_COUNT", R, 32, 0x344},
  {"N_VIF2_COUNT", RW, 32, 0x348},
  {"SNAPSHOT2_VIF_COUNT", RW, 32, 0x34C},
  {"TV_MASTER_CNTL", RW, 32, 0x800},
  {"TV_RGB_CNTL", RW, 32, 0x804},
  {"TV_SYNC_CNTL", RW, 32, 0x808},
  {"TV_HTOTAL", RW, 32, 0x80C},
  {"TV_HDISP", RW, 32, 0x810},
  {"TV_HSTART", RW, 32, 0x818},
  {"TV_HCOUNT", R, 32, 0x81C},
  {"TV_VTOTAL", RW, 32, 0x820},
  {"TV_VDISP", RW, 32, 0x824},
  {"TV_VCOUNT", R, 32, 0x828},
  {"TV_FTOTAL", RW, 32, 0x82C},
  {"TV_FCOUNT", R, 32, 0x830},
  {"TV_FRESTART", RW, 32, 0x834},
  {"TV_HRESTART", RW, 32, 0x838},
  {"TV_VRESTART", RW, 32, 0x83C},
  {"TV_HOST_READ_DATA", R, 32, 0x840},
  {"TV_HOST_WRITE_DATA", RW, 32, 0x844},
  {"TV_HOST_RD_WT_CNTL", RW, 32, 0x848},
  {"TV_VSCALER_CNTL1", RW, 32, 0x84C},
  {"TV_TIMING_CNTL", RW, 32, 0x850},
  {"TV_VSCALER_CNTL2", RW, 32, 0x854},
  {"TV_Y_FALL_CNTL", RW, 32, 0x858},
  {"TV_Y_RISE_CNTL", RW, 32, 0x85C},
  {"TV_Y_SAW_TOOTH_CNTL", RW, 32, 0x860},
  {"TV_UPSAMP_AND_GAIN_CNTL", RW, 32, 0x864},
  {"TV_GAIN_LIMIT_SETTINGS", RW, 32, 0x868},
  {"TV_LINEAR_GAIN_SETTINGS", RW, 32, 0x86C},
  {"TV_MODULATOR_CNTL1", RW, 32, 0x870},
  {"TV_MODULATOR_CNTL2", RW, 32, 0x874},
  {"TV_PRE_DAC_MUX_CNTL", RW, 32, 0x888},
  {"TV_DAC_CNTL", RW, 32, 0x88C},
  {"TV_CRC_CNTL", RW, 32, 0x890},
  {"TV_VIDEO_PORT_SIG", R, 32, 0x894},
  {"TV_VBI_CC_CNTL", RW, 32, 0x898},
  {"TV_VBI_EDS_CNTL", RW, 32, 0x89C},
  {"TV_VBI_20BIT_CNTL", RW, 32, 0x8A0},
  {"TV_VBI_DTO_CNTL", RW, 32, 0x8A4},
  {"TV_VBI_LEVEL_CNTL", RW, 32, 0x8A8},
  {"TV_UV_ADR", RW, 32, 0x8AC},
  {"TV_VSYNC_DIFF_CNTL", RW, 32, 0x8F4},
  {"TV_VSYNC_DIFF_LIMITS", RW, 32, 0x8F8},
  {"TV_VSYNC_DIFF_RD_DATA", R, 32, 0x8FC},
  {"HOST_PATH_CNTL", RW, 32, 0x130},
  {"MEM_VGA_WP_SEL", RW, 32, 0x38},
  {"MEM_VGA_RP_SEL", RW, 32, 0x3C},
  {"SW_SEMAPHORE", RW, 32, 0x13C},
  {"HDP_DEBUG", RW, 32, 0x138},
  {"CONFIG_XSTRAP", R, 32, 0xE4},
  {"GPIOPAD_MASK", RW, 32, 0x198},
  {"GPIOPAD_A", RW, 32, 0x19C},
  {"GPIOPAD_EN", RW, 32, 0x1A0},
  {"GPIOPAD_Y", R, 32, 0x1A4},
  {"LCDPAD_STRENGTH", RW, 32, 0x194},
  {"LCDPAD_MASK", RW, 32, 0x1A8},
  {"LCDPAD_A", RW, 32, 0x1AC},
  {"LCDPAD_EN", RW, 32, 0x1B0},
  {"LCDPAD_Y", R, 32, 0x1B4},
  {"BIOS_0_SCRATCH", RW, 32, 0x10},
  {"BIOS_1_SCRATCH", RW, 32, 0x14},
  {"BIOS_2_SCRATCH", RW, 32, 0x18},
  {"BIOS_3_SCRATCH", RW, 32, 0x1C},
  {"BIOS_4_SCRATCH", RW, 32, 0x20},
  {"BIOS_5_SCRATCH", RW, 32, 0x24},
  {"BIOS_6_SCRATCH", RW, 32, 0x28},
  {"BIOS_7_SCRATCH", RW, 32, 0x2C},
  {"MEDIA_0_SCRATCH", RW, 32, 0x1F0},
  {"MEDIA_1_SCRATCH", RW, 32, 0x1F4},
  {"TEST_DEBUG_CNTL", RW, 32, 0x120},
  {"TEST_DEBUG_MUX", RW, 32, 0x124},
  {"TEST_DEBUG_OUT", R, 32, 0x12C},
  {"SEPROM_CNTL1", RW, 32, 0x1C0},
  {"SEPROM_CNTL2", RW, 32, 0x1C4},
  {"END", R, 32, 0}
};

struct ati_reg_clk
{
  char *name;
  reg_type type;
  int width;
  int addr;
};

struct ati_reg_clk reg_list_clk[] = {
  {"CLK_PWRMGT_CNTL", RW, 32, 0x14},
  {"PLL_PWRMGT_CNTL", RW, 32, 0x15},
  {"CLK_PIN_CNTL", RW, 32, 0x1},
  {"PPLL_CNTL", RW, 32, 0x2},
  {"PPLL_REF_DIV", RW, 32, 0x3},
  {"SPLL_CNTL", RW, 32, 0xC},
  {"SCLK_CNTL", RW, 32, 0xD},
  {"AGP_PLL_CNTL", RW, 32, 0xB},
  {"TV_PLL_FINE_CNTL", RW, 32, 0x20},
  {"TV_PLL_CNTL", RW, 32, 0x21},
  {"TV_PLL_CNTL1", RW, 32, 0x22},
  {"TV_DTO_INCREMENTS", RW, 32, 0x23},
  {"P2PLL_CNTL", RW, 32, 0x2A},
  {"P2PLL_REF_DIV", RW, 32, 0x2B},
  {"PPLL_DIV_0", RW, 32, 0x4},
  {"PPLL_DIV_1", RW, 32, 0x5},
  {"PPLL_DIV_2", RW, 32, 0x6},
  {"PPLL_DIV_3", RW, 32, 0x7},
  {"PLL_TEST_CNTL", RW, 32, 0x13},
  {"P2PLL_DIV_0", RW, 32, 0x2C},
  {"MPLL_CNTL", RW, 32, 0xE},
  {"MDLL_CKO", RW, 32, 0xF},
  {"MDLL_RDCKA", RW, 32, 0x10},
  {"MCLK_CNTL", RW, 32, 0x12},
  {"MCLK_MISC", RW, 32, 0x1F},
  {"CG_TEST_MACRO_RW_WRITE", RW, 32, 0x16},
  {"CG_TEST_MACRO_RW_READ", RW, 32, 0x17},
  {"CG_TEST_MACRO_RW_DATA", R, 32, 0x18},
  {"CG_TEST_MACRO_RW_CNTL", RW, 32, 0x19},
  {"M_SPLL_REF_FB_DIV", RW, 32, 0xA},
  {"VCLK_ECP_CNTL", RW, 32, 0x8},
  {"PIXCLKS_CNTL", RW, 32, 0x2D},
  {"HTOTAL_CNTL", RW, 32, 0x9},
  {"HTOTAL2_CNTL", RW, 32, 0x2E},
  {"DISP_TEST_MACRO_RW_WRITE", RW, 32, 0x1A},
  {"DISP_TEST_MACRO_RW_READ", RW, 32, 0x1B},
  {"DISP_TEST_MACRO_RW_DATA", R, 32, 0x1C},
  {"DISP_TEST_MACRO_RW_CNTL", RW, 32, 0x1D},
  {"END", RW, 32, 0}
};
#endif

void
radeon_dump_regs (void)
{
#ifdef DEBUG
  int i;
  unsigned int value;

  i = 0;
  while (strcmp (reg_list[i].name, "END") != 0)
    {
      switch (reg_list[i].width)
	{
	case 8:
	  value = MMINB (reg_list[i].mmaddr);
	  break;
	case 16:
	  value = MMINH (reg_list[i].mmaddr);
	  break;
	case 32:
	  value = MMINL (reg_list[i].mmaddr);
	  break;
	default:
	  printf ("unknown width %d for reg %s\n", reg_list[i].width,
		  reg_list[i].name);
	  break;
	}


      switch (reg_list[i].type)
	{
	case R:
	  printf ("reg %03x = 0x%08lx, width %d,read only\n",
		  reg_list[i].mmaddr, value, reg_list[i].width);
	  break;
	case RW:
	  printf ("reg %03x = 0x%08lx, width %d,read/write\n",
		  reg_list[i].mmaddr, value, reg_list[i].width);
	  break;
	case W:
	  printf ("reg %03x = 0x%08lx, width %d,write only\n",
		  reg_list[i].mmaddr, value, reg_list[i].width);
	  break;
	}
      i++;

    }

  printf ("done1\n");

  i = 0;
  while (strcmp (reg_list_clk[i].name, "END") != 0)
    {
      value = INPLL (reg_list_clk[i].addr);

      switch (reg_list_clk[i].type)
	{
	case R:
	  printf ("reg %03x = 0x%08lx, width %d,read only\n",
		  reg_list_clk[i].addr, value, reg_list_clk[i].width);
	  break;
	case RW:
	  printf ("reg %03x = 0x%08lx, width %d,read/write\n",
		  reg_list_clk[i].addr, value, reg_list_clk[i].width);
	  break;
	case W:
	  printf ("reg %03x = 0x%08lx, width %d,write only\n",
		  reg_list_clk[i].addr, value, reg_list_clk[i].width);
	  break;
	}
      i++;

    }

  printf ("done2\n");
#endif
  return 0;
}


static void
program_mode_reg (int value)
{
  /* program */
  MMOUTL (0x158, 0x40320000 | value);
  mdelay (1);
  MMOUTL (0x158, 0xc0320000 | value);
  mdelay (1);
  MMOUTL (0x158, 0x40320000 | value);
  mdelay (1);

  printf ("mc_status=%x\n", MMINL (0x150));
}

static void
enable_dll (void)
{
  unsigned int cko, cka, ckb;

  cko = INPLL (0xf) | 0x3;
  cka = INPLL (0x10) | 0x30003;
  ckb = INPLL (0x11) | 0x30003;

  OUTPLL (0xf, cko);
  OUTPLL (0x10, cka);
  OUTPLL (0x11, ckb);

  mdelay (10);

  cko &= ~(0x10001);
  OUTPLL (0xf, cko);
  mdelay (1);
  cko &= ~(0x20002);
  OUTPLL (0xf, cko);
  mdelay (1);

  cka &= ~(0x10001);
  OUTPLL (0x10, cka);
  mdelay (1);
  cka &= ~(0x20002);
  OUTPLL (0x10, cka);
  mdelay (1);

  ckb &= ~(0x10001);
  OUTPLL (0x11, ckb);
  mdelay (1);
  ckb &= ~(0x20002);
  OUTPLL (0x11, ckb);
  mdelay (1);
}

static void
sync_clk (void)
{
  unsigned int v1, v2;

  v1 = INMC (0xc) & ~0x03000000;
  v2 = INMC (0xE) & ~0x03000000;

  OUTMC (0xC, v1 | (1 << 0x18));
  OUTMC (0xE, v2 | (1 << 0x18));

  OUTMC (0xC, v1);
  OUTMC (0xE, v2);

  mdelay (1);
}

static void
mc_init (void)
{
  unsigned int v, v1;

  unsigned int crtcGenCntl, crtcGenCntl2, memRefreshCntl, crtc_more_cntl,
    fp_gen_cntl, fp2_gen_cntl;

  crtcGenCntl = MMINL (0x50 /*CRTC_GEN_CNTL */ );
  crtcGenCntl2 = MMINL (0x3f8 /*CRTC2_GEN_CNTL */ );

  crtc_more_cntl = MMINL (0x27c /*CRTC_MORE_CNTL */ );
  fp_gen_cntl = MMINL (0x284 /*FP_GEN_CNTL */ );
  fp2_gen_cntl = MMINL (0x288 /*FP2_GEN_CNTL */ );


  MMOUTL (0x27c, 0);
  MMOUTL (0x284, 0);
  MMOUTL (0x288, 0);

  MMOUTL (0x50, (crtcGenCntl | 0x04000000));
  MMOUTL (0x3f8, (crtcGenCntl2 | 0x04000000));

  /* Disable refresh,EXT_MEM_CNTL */
  v = MMINL (0x144) & ~(1 << 20);
  MMOUTL (0x144, v | (1 << 20));

  /* Reset memory, SDRAM_MODE_REG */
  MMOUTL (0x158, MMINL (0x158) & ~0x10000000);

  /* DLL */
  enable_dll ();

  /* sync */
  sync_clk ();

  program_mode_reg (0x2001);
  program_mode_reg (0x2002);
  program_mode_reg (0x0132);
  program_mode_reg (0x0032);

  /* complete */
  MMOUTL (0x158, MMINL (0x158) | 0x10000000);

  /* enable refresh */
  MMOUTL (0x144, v & ~(1 << 20));

  MMOUTL (0x50, crtcGenCntl);
  MMOUTL (0x3f8 /*CRTC2_GEN_CNTL */ , crtcGenCntl2);
  MMOUTL (0x284 /*FP_GEN_CNTL */ , fp_gen_cntl);
  MMOUTL (0x288 /*FP2_GEN_CNTL */ , fp2_gen_cntl);

  MMOUTL (0x27c /*CRTC_MORE_CNTL */ , crtc_more_cntl);

  mdelay (150);

  /* fixups */

#if 0

  /* reg 0x14c is MC_AGP_LOCATION */
  MMOUTL (0x14c, 0x14ff1500);

  /* reg 0x148 is MC_FB_LOCATION */
  MMOUTL (0x148, 0x14ff1400);

  /* reg 0x170 is */
  MMOUTL (0x170, 0xb4c00000);
#endif

#if 0
  /* size 0xf8 */
  MMOUTL (0xF8, 0x01000000);
  /* reg 0x140 is MEM_CNTL */
  MMOUTL (0x140, 0x2d08);
  /* reg 0x170 is */
  MMOUTL (0x170, 0xb4c00000);
  /* CONFIG_CNTL 0xe0 */
  MMOUTL (0xe0, 0x00010100);
#endif

  MMOUTL (0x17c, 0xfe4fbfff);
  MMOUTL (0xd10, 0x11000000);
  MMOUTL (0x2f0, 0x20065c5c);
  MMOUTL (0x2f4, 0x00003434);
  MMOUTL (0x210, 0x025903ff);
  MMOUTL (0x324, 0x00000000);
  MMOUTL (0x328, 0x10000000);
  MMOUTL (0x5c,  0x2);
  MMOUTL (0x2a8, 0xa27);
  MMOUTL (0x10, 0x0000000c);
  MMOUTL (0x14, 0x0030c000);
  MMOUTL (0x18, 0x04040400);
  MMOUTL (0x20, 0x02000002);
  MMOUTL (0x24, 0x02000002);

  OUTPLL (0x14, 0x00013000);
  OUTPLL (0x15, 0x0007c000);
  OUTPLL (0x01, 0x00018015);
  OUTPLL (0x02, 0x00008f00);
  OUTPLL (0x0c, 0x0400bc30);
  OUTPLL (0x04, 0x0007029f);
  OUTPLL (0x05, 0x000702f3);
  OUTPLL (0x13, 0x41000200);
  OUTPLL (0x0e, 0x0400fc30);
  OUTPLL (0x0f, 0x0000053c);
  OUTPLL (0x10, 0x08800880);
  OUTPLL (0x2d, 0x0000f8c0);

  /* size 0xf8 */
  MMOUTL (0xF8, 0x01000000);
  /* reg 0x140 is MEM_CNTL */
  //MMOUTL (0x140, 0x2d08);
  MMOUTL (0x188, 0x7400);

  OUTL(0x0,0x140);
  OUTB(0x4,0x08);
  mdelay(1500);
  OUTB(0x5,0x2d);
  mdelay(1500);
  /* reg 0x170 is */
  MMOUTL (0x170, 0xb4c00000);
  /* CONFIG_CNTL 0xe0 */
  MMOUTL (0xe0, 0x00010100);
}

static
CARD8
RADEONComputePLLGain(
                    CARD16 reference_freq,
                    CARD16 ref_div,
                    CARD16 fb_div
                    )
{
  unsigned vcoFreq;

  vcoFreq = ((unsigned)reference_freq * fb_div) / ref_div;

  /*
   * This is orribly crude: the VCO frequency range is divided into
   * 3 parts, each part having a fixed PLL gain value.
   */
  if (vcoFreq >= 30000)
    /*
     * [300..max] MHz : 7
     */
    return 7;
  else if (vcoFreq >= 18000)
    /*
     * [180..300) MHz : 4
     */
    return 4;
  else
    /*
     * [0..180) MHz : 1
     */
    return 1;
}



static void RADEONPLLWaitForReadUpdateComplete(void)
{
	int i = 0;

	/* FIXME: Certain revisions of R300 can't recover here.  Not sure of
	 *        the cause yet, but this workaround will mask the problem for now.
	 *               Other chips usually will pass at the very first test, so the
	 *                      workaround shouldn't have any effect on them. */
	for (i = 0;
			(i < 10000 &&
			 INPLL( RADEON_PPLL_REF_DIV) & RADEON_PPLL_ATOMIC_UPDATE_R);
			i++);
}

static void RADEONPLLWriteUpdate(void)
{
	while (INPLL(RADEON_PPLL_REF_DIV) & RADEON_PPLL_ATOMIC_UPDATE_R);

	OUTPLLP(RADEON_PPLL_REF_DIV,
			RADEON_PPLL_ATOMIC_UPDATE_W,
			~(RADEON_PPLL_ATOMIC_UPDATE_W));
}


/* Write PLL registers */
static void RADEONRestorePLLRegisters(void)
{
    CARD8 pllGain;
#ifdef VGA_NOTEBOOK
    int reference_freq = 2700;
    int ppll_ref_div = 0x43;
    int ppll_div_3 = 0x10160;
    int htotal_cntl = 0;
    int vclk_ecp_cntl = 0xc3;
#else
    int reference_freq = 2700;
    int ppll_ref_div = 0x43;
    int ppll_div_3 = 0x301f4;
    int htotal_cntl = 0;
    int vclk_ecp_cntl = 0xc3;
#endif

    OUTPLLP(RADEON_VCLK_ECP_CNTL,
	    RADEON_VCLK_SRC_SEL_CPUCLK,
	    ~(RADEON_VCLK_SRC_SEL_MASK));

    pllGain = RADEONComputePLLGain(reference_freq,
                                  ppll_ref_div & RADEON_PPLL_REF_DIV_MASK,
                                  ppll_div_3 & RADEON_PPLL_FB3_DIV_MASK);

    OUTPLLP( 
	    RADEON_PPLL_CNTL,
	    RADEON_PPLL_RESET
	    | RADEON_PPLL_ATOMIC_UPDATE_EN
	    | RADEON_PPLL_VGA_ATOMIC_UPDATE_EN
	    | ((CARD32)pllGain << RADEON_PPLL_PVG_SHIFT),
	    ~(RADEON_PPLL_RESET
	      | RADEON_PPLL_ATOMIC_UPDATE_EN
	      | RADEON_PPLL_VGA_ATOMIC_UPDATE_EN
	      | RADEON_PPLL_PVG_MASK));

    OUTREGP(RADEON_CLOCK_CNTL_INDEX,
	    RADEON_PLL_DIV_SEL,
	    ~(RADEON_PLL_DIV_SEL));

    OUTPLLP(RADEON_PPLL_REF_DIV,
		    ppll_ref_div,
		    ~RADEON_PPLL_REF_DIV_MASK);

    OUTPLLP(RADEON_PPLL_DIV_3,
	    ppll_div_3,
	    ~RADEON_PPLL_FB3_DIV_MASK);

    OUTPLLP(RADEON_PPLL_DIV_3,
	    ppll_div_3,
	    ~RADEON_PPLL_POST3_DIV_MASK);

    RADEONPLLWriteUpdate();
    RADEONPLLWaitForReadUpdateComplete();

    OUTPLL(RADEON_HTOTAL_CNTL, htotal_cntl);

    OUTPLLP(RADEON_PPLL_CNTL,
	    0,
	    ~(RADEON_PPLL_RESET
	      | RADEON_PPLL_SLEEP
	      | RADEON_PPLL_ATOMIC_UPDATE_EN
	      | RADEON_PPLL_VGA_ATOMIC_UPDATE_EN));

    printf("Wrote: 0x%08x 0x%08x 0x%08x (0x%08x)\n",
	       ppll_ref_div,
	       ppll_div_3,
	       htotal_cntl,
	       INPLL(RADEON_PPLL_CNTL));
    printf("Wrote: rd=%d, fd=%d, pd=%d\n",
	       ppll_ref_div & RADEON_PPLL_REF_DIV_MASK,
	       ppll_div_3 & RADEON_PPLL_FB3_DIV_MASK,
	       (ppll_div_3 & RADEON_PPLL_POST3_DIV_MASK) >> 16);

    mdelay(50000); /* Let the clock to lock */

    OUTPLL(RADEON_VCLK_ECP_CNTL , vclk_ecp_cntl);

    printf("VCLK_ECP_CNTL = %08X\n" ,vclk_ecp_cntl);
}


#define CRT_C   24              /* 24 CRT Controller Registers */
#define ATT_C   21              /* 21 Attribute Controller Registers */
#define GRA_C   9               /* 9  Graphics Controller Registers */
#define SEQ_C   5               /* 5  Sequencer Registers */
#define MIS_C   1               /* 1  Misc Output Register */
                                                                                                                  
/* VGA registers saving indexes */
#define CRT     0               /* CRT Controller Registers start */
#define ATT     (CRT+CRT_C)     /* Attribute Controller Registers start */
#define GRA     (ATT+ATT_C)     /* Graphics Controller Registers start */
#define SEQ     (GRA+GRA_C)     /* Sequencer Registers */
#define MIS     (SEQ+SEQ_C)     /* General Registers */
#define EXT     (MIS+MIS_C)     /* SVGA Extended Registers */

static void vgadelay(void)
{
  int i;
  for(i=0;i<10;i++);
}

static unsigned char regs[60] = {
    0x5F,0x4F,0x50,0x02,0x55,0x81,0xBF,0x1F,      /* CR00-CR18 */
    0x00,0x4F,0x0D,0x0E,0x0,0x0,0x0,0x0,
    0x9C,0x00,0x8F,0x28,0x1F,0x96,0xB9,0xA3,
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,      /* AR00-AR15 */
    0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
    0x0C,0x00,0x0F,0x08,0x00,
    0x00,0x00,0x00,0x00,0x00,0x10,0x0E,0x00,      /* GR00-GR05 */
    0xFF,
    0x03,0x00,0x03,0x00,0x02,                     /* SR00-SR05 */
    0x67,                                         /* MISC_OUT  */
};
                                                                                                                  
static void outseq(int index,unsigned char val)
{
  int v;
  v=((int)val<<8)+index;
  linux_outw(v,0x3c4);
}
                                                                                                                  
static void outcrtc(int index,unsigned char val)
{
  int v;
  v=((int)val<<8)+index;
  linux_outw(v,0x3d4);
}
static unsigned char incrtc(int index)
{
  linux_outb(index,0x3d4);
  return linux_inb(0x3d5);
}
                                                                                                                  
static void setregs(const unsigned char *regs)
{
  int i;
  linux_outb(regs[MIS],0x3c2);
  outseq(0x0,0x1);
  outseq(0x01,regs[SEQ+1]|0x20);
  linux_outb(0x1,0x3c4);
  linux_outb(regs[SEQ+1]|0x20,0x3c5);
  for(i=2;i<SEQ_C;i++)
  {
    outseq(i,regs[SEQ+i]);
  }
  outseq(0x0,0x3);
  outcrtc(0x11,incrtc(0x11)&0x7f);
                                                                                                                  
  for(i=0;i<CRT_C;i++)
  {
    outcrtc(i,regs[CRT+i]);
  }
                                                                                                                  
  for(i=0;i<GRA_C;i++)
  {
    linux_outb(i,0x3ce);
    linux_outb(regs[GRA+i],0x3cf);
  }
                                                                                                                  
  for(i=0;i<ATT_C;i++)
  {
    linux_inb(0x3da);
    vgadelay();
    linux_outb(i,0x3c0);
    vgadelay();
    linux_outb(regs[ATT+i],0x3c0);
    vgadelay();
  }
  outseq(0x01,regs[SEQ+1]&0xDF);
  linux_inb(0x3da);
  vgadelay();
  linux_outb(0x20,0x3c0);

}


void radeon_init_mode(void)
{
  unsigned int v;

  /* initialize to given mode */

  MMOUTL(0x0230,0);
  MMOUTL(0x0234,0);
  MMOUTL(0x0238,0);
  MMOUTL(0x0420,0);
  MMOUTL(0x0540,0);
  MMOUTL(0x0c40,0);
  MMOUTL(0x094,0);
  MMOUTL(0x040,0);
  MMOUTL(0x950,0);


  /* blank display */
  v = MMINL(RADEON_CRTC_EXT_CNTL);
  MMOUTL(RADEON_CRTC_EXT_CNTL,v | 0x700); 
  v = MMINL(RADEON_FP_GEN_CNTL);
  MMOUTL(RADEON_FP_GEN_CNTL,v & ~0x5); 

#ifdef VGA_NOTEBOOK
  /* 1280x800-71 */
  MMOUTL(RADEON_CRTC_GEN_CNTL,0x3000400); 
  MMOUTL(RADEON_CRTC_EXT_CNTL,0x8748); 
  MMOUTL(RADEON_CRTC_MORE_CNTL,0x0); 
  MMOUTL(RADEON_DAC_CNTL, 0xff002102);
  MMOUTL(RADEON_CRTC_H_TOTAL_DISP,0x9f00b3);
  MMOUTL(RADEON_CRTC_H_SYNC_STRT_WID,0x8a053d);
  MMOUTL(RADEON_CRTC_V_TOTAL_DISP,0x31f0336);
  MMOUTL(RADEON_CRTC_V_SYNC_STRT_WID,0x30320);
  MMOUTL(RADEON_CRTC_OFFSET,0x0);
  MMOUTL(RADEON_CRTC_OFFSET_CNTL,0x0);
  MMOUTL(RADEON_SURFACE_CNTL,0x0);
  MMOUTL(RADEON_CRTC_PITCH,0xa000a0);

  MMOUTL(RADEON_DEFAULT_PITCH,0xa000000);
  MMOUTL(RADEON_CRTC_OFFSET,0xa000000);
  MMOUTL(RADEON_CRTC_OFFSET,0xa000000);
#else
  /* 640x480-60 */
  MMOUTL(RADEON_CRTC_GEN_CNTL,0x3000400); 
  MMOUTL(RADEON_CRTC_EXT_CNTL,0x8748); 
  MMOUTL(RADEON_CRTC_MORE_CNTL,0x0); 
  MMOUTL(RADEON_DAC_CNTL, 0xff002102);
  MMOUTL(RADEON_CRTC_H_TOTAL_DISP,0x4f0063);
  MMOUTL(RADEON_CRTC_H_SYNC_STRT_WID,0x8c029a);
  MMOUTL(RADEON_CRTC_V_TOTAL_DISP,0x1df020c);
  MMOUTL(RADEON_CRTC_V_SYNC_STRT_WID,0x8201e9);
  MMOUTL(RADEON_CRTC_OFFSET,0x0);
  MMOUTL(RADEON_CRTC_OFFSET_CNTL,0x0);
  MMOUTL(RADEON_SURFACE_CNTL,0x0);
  MMOUTL(RADEON_CRTC_PITCH,0x500050);

  MMOUTL(RADEON_DEFAULT_PITCH,0x5000000);
  MMOUTL(RADEON_CRTC_OFFSET,0x5000000);
  MMOUTL(RADEON_CRTC_OFFSET,0x5000000);
#endif

  RADEONRestorePLLRegisters();

#ifdef VGA_NOTEBOOK
  MMOUTL(RADEON_FP_CRTC_H_TOTAL_DISP,0x9f0014);
  MMOUTL(RADEON_FP_H_SYNC_STRT_WID,0x8a003c);
  MMOUTL(RADEON_FP_CRTC_V_TOTAL_DISP,0x31f0017);
  MMOUTL(RADEON_FP_V_SYNC_STRT_WID,0x30001);
  MMOUTL(RADEON_FP_GEN_CNTL,0x30080); 
  MMOUTL(RADEON_FP_HORZ_STRETCH,0x9f0000); 
  MMOUTL(RADEON_FP_VERT_STRETCH,0x31f000); 
  MMOUTL(0x2a0,0x0); 
  MMOUTL(RADEON_TMDS_TRANSMITTER_CNTL,0x10000081); 
#else
  MMOUTL(RADEON_FP_CRTC_H_TOTAL_DISP,0x9f0014);
  MMOUTL(RADEON_FP_H_SYNC_STRT_WID,0x8a003c);
  MMOUTL(RADEON_FP_CRTC_V_TOTAL_DISP,0x31f0017);
  MMOUTL(RADEON_FP_V_SYNC_STRT_WID,0x30001);
  MMOUTL(RADEON_FP_GEN_CNTL,0x30080); 
  MMOUTL(RADEON_FP_HORZ_STRETCH,0x9f0000); 
  MMOUTL(RADEON_FP_VERT_STRETCH,0x31f000); 
  MMOUTL(0x2a0,0x0); 
  MMOUTL(RADEON_TMDS_TRANSMITTER_CNTL,0x10000081); 
#endif

  MMOUTL(0x146c,0xf030d0);
  MMOUTL(0x147c,0);
  MMOUTL(0x16cc,0xffffffff);
  MMOUTL(0x16c0,3);
  MMOUTL(0x1438,0x500000);
  MMOUTL(0x1598,180010);
  MMOUTL(0x342c,0xf);

  /* unblank */
  MMOUTL(RADEON_CRTC_EXT_CNTL,0x8048); 
#ifdef VGA_NOTEBOOK
  MMOUTL(RADEON_FP_GEN_CNTL,0x30085); 
#endif

}

void radeon_init_regbase(void)
{
  iobase  = (_pci_conf_read(vga_dev->pa.pa_tag,0x14) & ~0x3) + 0xbfd00000;
  mmbase  =_pci_conf_read(vga_dev->pa.pa_tag,0x18) | 0xb0000000;
  printf("iobase=%lx,mmbase=%lx\n",iobase,mmbase);
}

int radeon_engine_init (void);
int
radeon_init (void)
{
  unsigned int v;
  int i;
  /* BIOS Header at 0x10c
     first initialization block at 0x1dd
     PLL information block at 0x2ef
     second initialization block at 0x25e
     second initialization block at 0x3bb */

  printf ("starting radeon init...\n");
  
  radeon_init_regbase();

  /* first initialization */

  /* reg 0x1c0 is SEPROM_CNTL1 */
  v = MMINL (0x1c0);
  v = (v & 0xffffff) | 0x4000000;
  MMOUTL (0x1c0, v);

  /* reg 0x30 is BUS_CNTL */
  OUTL (0x30, 0x5133a3b0);

  MMOUTL (0xec, 0x4443);

  /* reg 0x1d0 is AIC_CTRL */
  v = MMINL (0x1d0);
  v = (v & 0xfffffffd) | 0x2;
  MMOUTL (0x1d0, v);

  /* reg 0x50 is CRTC_GEN_CNTL */
  OUTL (0x50, 0x4000000);

  /* reg 0x58 is DAC_CNTL */
  OUTL (0x58, 0xff604102);

  /* reg 0x168 is PAD_CTLR_STRENGTH */
  v = MMINL (0x168);
  v = (v & 0xfffeffff) | 0x1200;
  MMOUTL (0x168, v);

  /* reg 0x178 is MEM_IO_CNTL_A0 */
  MMOUTL (0x178, 0xff1f7fff);

  /* reg 0x17c is MEM_IO_CNTL_A1 */
  MMOUTL (0x17c, 0xfecfbfff);

  /* reg 0x188 is MC_DEBUG */
  v = MMINL (0x188);
  v = (v & 0xffffffff) | 0x8007c00;
  MMOUTL (0x188, v);

  /* reg 0xd00 is DISP_MISC_CNTL */
  v = MMINL (0xd00);
  v = (v & 0xffffff) | 0x5b000000;
  MMOUTL (0xd00, v);

  /* reg 0x88c is TV_DAC_CNTL */
  v = MMINL (0x88c);
  v = (v & 0xf800fcef) | 0x7480000;
  MMOUTL (0x88c, v);

  /* reg 0xd04 is DAC_MACRO_CNTL */
  v = MMINL (0xd04);
  v = (v & 0xfffffff0) | 0x6;
  MMOUTL (0xd04, v);

  /* reg 0x284 is FP_GEN_CNTL */
  v = MMINL (0x284);
  v = (v & 0xffffffff) | 0x8;
  MMOUTL (0x284, v);

  /* reg 0x30 is BUS_CNTL */
  v = MMINL (0x30);
  v = (v & 0xffffffef) | 0x0;
  MMOUTL (0x30, v);

  linux_outb (0x0,0xa108);
  linux_outb (0x5,0x3c2);
  linux_outb (0x0,0x3c0);
  linux_outw (0x2001,0x3c4);

  /* PLL initialization */

  /* PLL reg 0xd is SCLK_CNTL */
  OUTPLL (0xd, 0xfffffff8);

  /* PLL reg 0x12 is MCLK_CNTL */
  OUTPLL (0x12, 0xa350000);

  /* PLL reg 0x8 is VCLK_ECP_CNTL */
  OUTPLL (0x8, 0x0);

  /* PLL reg 0x2d is PIXCLKS_CNTL */
  OUTPLL (0x2d, 0x0);

  linux_outb (0x0,0xa108);

  /*special command 0x2 */
  mdelay (150);

  /* PLL reg 0x3 is PPLL_REF_DIV */
  OUTPLL (0x3, 0x3c);

  /* PLL reg 0xa is M_SPLL_REF_FB_DIV */
  OUTPLL (0xa, 0x14a4a0c);

  /* PLL reg 0xe is MPLL_CNTL */
  OUTPLL (0xe, 0x400fc33);

  /* PLL reg 0xc is SPLL_CNTL */
  OUTPLL (0xc, 0x400bc33);

  /* PLL reg 0x2 is PPLL_CNTL */
  OUTPLL (0x2, 0xa703);

  /* PLL reg 0xf is MDLL_CKO */
  OUTPLL (0xf, 0x53f);

  /* PLL reg 0x10 is MDLL_RDCKA */
  OUTPLL (0x10, 0x8830883);

  /*special command 0x2 */
  mdelay (150);

  /* PLL reg 0xe is MPLL_CNTL */
  /* AND/OR */
  v = INPLL (0xe);
  OUTPLL (0xe, ((v & 0xfd) | 0x0));

  /*special command 0x2 */
  mdelay (150);

  /* PLL reg 0xe is MPLL_CNTL */
  /* AND/OR */
  v = INPLL (0xe);
  OUTPLL (0xe, ((v & 0xfe) | 0x0));

  /*special command 0x2 */
  mdelay (150);

  /* PLL reg 0x12 is MCLK_CNTL */
  OUTPLL (0x12, 0xa350012);

  /*special command 0x2 */
  mdelay (150);

  /* PLL reg 0xf is MDLL_CKO */
  /* AND/OR */
  v = INPLL (0xf);
  OUTPLL (0xf, ((v & 0xfe) | 0x0));

  /*special command 0x1 */
  mdelay (1500);

  /*special command 0x2 */
  mdelay (150);

  /* PLL reg 0x10 is MDLL_RDCKA */
  /* AND/OR */
  v = INPLL (0x10);
  OUTPLL (0x10, ((v & 0xfe) | 0x0));

  /*special command 0x2 */
  mdelay (150);

  /* PLL reg 0x10 is MDLL_RDCKA */
  /* AND/OR */
  v = INPLL (0x10);
  OUTPLL (0x10, ((v & 0xfe0000) | 0x0));

  /*special command 0x1 */
  mdelay (1500);

  /*special command 0x2 */
  mdelay (150);

  /* PLL reg 0xf is MDLL_CKO */
  /* AND/OR */
  v = INPLL (0xf);
  OUTPLL (0xf, ((v & 0xfd) | 0x0));

  /*special command 0x2 */
  mdelay (150);

  /* PLL reg 0x10 is MDLL_RDCKA */
  /* AND/OR */
  v = INPLL (0x10);
  OUTPLL (0x10, ((v & 0xfd) | 0x0));

  /*special command 0x2 */
  mdelay (150);

  /* PLL reg 0x10 is MDLL_RDCKA */
  /* AND/OR */
  v = INPLL (0x10);
  OUTPLL (0x10, ((v & 0xfd0000) | 0x0));

  /*special command 0x2 */
  mdelay (150);

  /*special command 0x2 */
  mdelay (150);

  /* PLL reg 0xc is SPLL_CNTL */
  /* AND/OR */
  v = INPLL (0xc);
  OUTPLL (0xc, ((v & 0xfe) | 0x0));

  /*special command 0x1 */
  mdelay (1500);

  /*special command 0x2 */
  mdelay (150);

  /* PLL reg 0xc is SPLL_CNTL */
  /* AND/OR */
  v = INPLL (0xc);
  OUTPLL (0xc, ((v & 0xfd) | 0x0));

  /*special command 0x2 */
  mdelay (150);

  /* PLL reg 0xd is SCLK_CNTL */
  OUTPLL (0xd, 0xfffffffa);

  /*special command 0x2 */
  mdelay (150);

  /* PLL reg 0x8 is VCLK_ECP_CNTL */
  /* AND/OR */
  v = INPLL (0x8);
  OUTPLL (0x8, ((v & 0x3c) | 0x0));

  /* PLL reg 0x2 is PPLL_CNTL */
  /* AND/OR */
  v = INPLL (0x2);
  OUTPLL (0x2, ((v & 0xff) | 0x3));

  /* PLL reg 0x4 is PPLL_DIV_0 */
  OUTPLL (0x4, 0x381c0);

  /* PLL reg 0x5 is PPLL_DIV_1 */
  OUTPLL (0x5, 0x381f7);

  /* PLL reg 0x6 is PPLL_DIV_2 */
  OUTPLL (0x6, 0x381c0);

  /* PLL reg 0x7 is PPLL_DIV_3 */
  OUTPLL (0x7, 0x381f7);

  /* PLL reg 0x2 is PPLL_CNTL */
  /* AND/OR */
  v = INPLL (0x2);
  OUTPLL (0x2, ((v & 0xfd) | 0x0));

  /*special command 0x1 */
  mdelay (1500);

  /*special command 0x2 */
  mdelay (150);

  /* PLL reg 0x2 is PPLL_CNTL */
  /* AND/OR */
  v = INPLL (0x2);
  OUTPLL (0x2, ((v & 0xfe) | 0x0));

  /*special command 0x2 */
  mdelay (150);

  /* PLL reg 0x8 is VCLK_ECP_CNTL */
  /* AND/OR */
  v = INPLL (0x8);
  OUTPLL (0x8, ((v & 0x3c) | 0x3));

  /*special command 0x2 */
  mdelay (150);

  /* PLL reg 0x1 is CLK_PIN_CNTL */
  /* AND/OR */
  v = INPLL (0x1);
  OUTPLL (0x1, ((v & 0xff) | 0x10));

  /* second initialization */

  /* reg 0x140 is MEM_CNTL */
  v = MMINL (0x140);
  v = (v & 0xffffffff) | 0x20002004;
  MMOUTL (0x140, v);

  /* reg 0x158 is MEM_SDRAM_MODE_REG */
  v = MMINL (0x158);
  v = (v & 0xf0000000) | 0x403a0000;
  MMOUTL (0x158, v);

  /* reg 0x144 is EXT_MEM_CNTL */
  MMOUTL (0x144, 0x1405356b);

  /* reg 0x14c is MC_AGP_LOCATION */
  MMOUTL (0x14c, 0xffff0);

  /* reg 0x148 is MC_FB_LOCATION */
  MMOUTL (0x148, 0xffff0000);

  /* reg 0x154 is MEM_INIT_LATENCY_TIMER */
  MMOUTL (0x154, 0x77777777);

  /* reg 0x18c is MEM_IO_OE_CNTL */
  MMOUTL (0x18c, 0x16666);

  MMOUTL (0x910, 0x4);

  /* reg 0x10 is BIOS_0_SCRATCH */
  v = MMINL (0x10);
  v = (v & 0xfffffffb) | 0x4;
  MMOUTL (0x10, v);

  /* reg 0xd64 is DISP_OUTPUT_CNTL */
  v = MMINL (0xd64);
  v = (v & 0xfffffbff) | 0x0;
  MMOUTL (0xd64, v);

  /* reg 0x2a8 is TMDS_PLL_CNTL */
  MMOUTL (0x2a8, 0xa1b);

  /* reg 0xd64 is DISP_OUTPUT_CNTL */
  v = MMINL (0xd64);
  v = (v & 0xfffffbff) | 0x222;
  MMOUTL (0xd64, v);

  /* reg 0x800 is TV_MASTER_CNTL */
  v = MMINL (0x800);
  v = (v & 0xbfffffff) | 0x40000000;
  MMOUTL (0x800, v);

  /* reg 0xd10 is DISP_TEST_DEBUG_CNTL */
  v = MMINL (0xd10);
  v = (v & 0xefffffff) | 0x10000000;
  MMOUTL (0xd10, v);

  /* reg 0x4dc is OV0_FLAG_CNTRL */
  v = MMINL (0x4dc);
  v = (v & 0xfffffeff) | 0x100;
  MMOUTL (0x4dc, v);

  /* reg 0x34 is BUS_CNTL1 */
  v = MMINL (0x34);
  v = (v & 0x73ffffff) | 0x84000000;
  MMOUTL (0x34, v);

  /* reg 0x174 is AGP_CNTL */
  v = MMINL (0x174);
  v = (v & 0xffefff00) | 0x1e0000;
  MMOUTL (0x174, v);

  /* memory controller initialization */
  mc_init ();

  MMOUTL (0xd64, 0);
  /* 8bit DAC */
  MMOUTL (0x058, 0xff604002);

  MMOUTL (0x38, 0x00010000);
  MMOUTL (0x3C, 0x00010000);

  setregs(&regs);

  /* install palette */
  OUTL(0xb0, 0);
  for (i=0;i<0xff;i++) {
    OUTL(0xb4, (i<<16) | (i<<8) | i );
  }

  radeon_init_mode();

  radeon_engine_init();
  radeon_dump_regs();

  printf ("radeon init done\n");

  return 1;
}

/*
 *  * 2D engine routines
 *   */

static __inline__ void radeon_engine_flush (void)
{
	int i;

	/* initiate flush */
	OUTREGP(RADEON_RB2D_DSTCACHE_CTLSTAT, RADEON_RB2D_DC_FLUSH_ALL,
			~RADEON_RB2D_DC_FLUSH_ALL);

	for (i=0; i < 2000000; i++) {
		if (!(MMINL(RADEON_RB2D_DSTCACHE_CTLSTAT) & RADEON_RB2D_DC_BUSY))
			break;
	}
}


static __inline__ void radeon_fifo_wait (int entries)
{
	int i;

	for (i=0; i<2000000; i++)
		if ((MMINL(RADEON_RBBM_STATUS) & 0x7f) >= entries)
			return;
}

static __inline__ void radeon_engine_idle (void)
{
	int i;

	/* ensure FIFO is empty before waiting for idle */
	radeon_fifo_wait (64);

	for (i=0; i<2000000; i++) {
		if (((MMINL(RADEON_RBBM_STATUS) & RADEON_RBBM_ACTIVE)) == 0) {
			radeon_engine_flush ();
			return;
		}
	}
}

#define DST_1BPP                0
#define DST_4BPP                1
#define DST_8BPP                2
#define DST_15BPP               3
#define DST_16BPP               4
#define DST_32BPP               6
static __inline__ unsigned int radeon_get_dstbpp(int depth)
{
	switch (depth) {
		case 8:
			return DST_8BPP;
		case 15:
			return DST_15BPP;
		case 16:
			return DST_16BPP;
		case 32:
			return DST_32BPP;
		default:
			return 0;
	}
}

static void radeon_engine_reset(void)
{
	unsigned int clock_cntl_index, mclk_cntl, rbbm_soft_reset;
	unsigned int host_path_cntl;

	radeon_engine_flush ();

	{
		unsigned int tmp;

		tmp = INPLL(RADEON_SCLK_CNTL);
		OUTPLL(RADEON_SCLK_CNTL, ((tmp & ~RADEON_DYN_STOP_LAT_MASK) |
				   RADEON_CP_MAX_DYN_STOP_LAT |
				   RADEON_SCLK_FORCEON_MASK));

	}

	clock_cntl_index = MMINL(RADEON_CLOCK_CNTL_INDEX);
	mclk_cntl = INPLL(RADEON_MCLK_CNTL);

	OUTPLL(RADEON_MCLK_CNTL, (mclk_cntl |
			   RADEON_FORCEON_MCLKA |
			   RADEON_FORCEON_MCLKB |
			   RADEON_FORCEON_YCLKA |
			   RADEON_FORCEON_YCLKB |
			   RADEON_FORCEON_MC |
			   RADEON_FORCEON_AIC));

	host_path_cntl = MMINL(RADEON_HOST_PATH_CNTL);
	rbbm_soft_reset = MMINL(RADEON_RBBM_SOFT_RESET);

	MMOUTL(RADEON_RBBM_SOFT_RESET, rbbm_soft_reset |
			RADEON_SOFT_RESET_CP |
			RADEON_SOFT_RESET_HI |
			RADEON_SOFT_RESET_SE |
			RADEON_SOFT_RESET_RE |
			RADEON_SOFT_RESET_PP |
			RADEON_SOFT_RESET_E2 |
			RADEON_SOFT_RESET_RB);
	MMINL(RADEON_RBBM_SOFT_RESET);
	MMOUTL(RADEON_RBBM_SOFT_RESET, rbbm_soft_reset & (u32)
			~(RADEON_SOFT_RESET_CP |
				RADEON_SOFT_RESET_HI |
				RADEON_SOFT_RESET_SE |
				RADEON_SOFT_RESET_RE |
				RADEON_SOFT_RESET_PP |
				RADEON_SOFT_RESET_E2 |
				RADEON_SOFT_RESET_RB));
	MMINL(RADEON_RBBM_SOFT_RESET);

	MMOUTL(RADEON_HOST_PATH_CNTL, host_path_cntl | RADEON_HDP_SOFT_RESET);
	MMINL(RADEON_HOST_PATH_CNTL);
	MMOUTL(RADEON_HOST_PATH_CNTL, host_path_cntl);

	MMOUTL(RADEON_RBBM_SOFT_RESET, rbbm_soft_reset);

	MMOUTL(RADEON_CLOCK_CNTL_INDEX, clock_cntl_index);
	OUTPLL(RADEON_MCLK_CNTL, mclk_cntl);

	return;
}

int radeon_engine_init (void)
{
	unsigned long temp;
	unsigned long dp_gui_master_cntl;

	/* disable 3D engine */
	MMOUTL(RADEON_RB3D_CNTL, 0);

	radeon_engine_reset ();

	radeon_fifo_wait (1);
	MMOUTL(RADEON_RB2D_DSTCACHE_MODE, 0);

	radeon_fifo_wait (3);
#ifdef VGA_NOTEBOOK
	MMOUTL(RADEON_DEFAULT_OFFSET, 0xa000000);
	MMOUTL(RADEON_DST_PITCH_OFFSET, 0xa000000);
	MMOUTL(RADEON_SRC_PITCH_OFFSET, 0xa000000);
#else
	MMOUTL(RADEON_DEFAULT_OFFSET, 0x5000000);
	MMOUTL(RADEON_DST_PITCH_OFFSET, 0x5000000);
	MMOUTL(RADEON_SRC_PITCH_OFFSET, 0x5000000);
#endif

	radeon_fifo_wait (1);
	OUTREGP(RADEON_DP_DATATYPE, 0, ~RADEON_HOST_BIG_ENDIAN_EN);

	radeon_fifo_wait (1);
	MMOUTL(RADEON_DEFAULT_SC_BOTTOM_RIGHT, (RADEON_DEFAULT_SC_RIGHT_MAX |
					 RADEON_DEFAULT_SC_BOTTOM_MAX));

	temp = radeon_get_dstbpp(2);
	dp_gui_master_cntl = ((temp << 8) | RADEON_GMC_CLR_CMP_CNTL_DIS);

	radeon_fifo_wait (1);
	MMOUTL(RADEON_DP_GUI_MASTER_CNTL, (dp_gui_master_cntl |
				    RADEON_GMC_BRUSH_SOLID_COLOR |
				    RADEON_GMC_SRC_DATATYPE_COLOR));

	radeon_fifo_wait (7);

	/* clear line drawing regs */
	MMOUTL(RADEON_DST_LINE_START, 0);
	MMOUTL(RADEON_DST_LINE_END, 0);

	/* set brush color regs */
	MMOUTL(RADEON_DP_BRUSH_FRGD_CLR, 0xffffffff);
	MMOUTL(RADEON_DP_BRUSH_BKGD_CLR, 0x00000000);

	/* set source color regs */
	MMOUTL(RADEON_DP_SRC_FRGD_CLR, 0xffffffff);
	MMOUTL(RADEON_DP_SRC_BKGD_CLR, 0x00000000);

	/* default write mask */
	MMOUTL(RADEON_DP_WRITE_MASK, 0xffffffff);

	radeon_engine_idle ();

	return 0;
}


extern void prom_printf(char *fmt, ...);
void video_hw_bitblt(int bpp, int sx, int sy, int dx, int dy, int w, int h)
{
	int val;
	int xdir,ydir;

	xdir = sx - dx;
	ydir = sy - dy;

	if ( xdir < 0 ) { sx += w-1; dx += w-1; }
	if ( ydir < 0 ) { sy += h-1; dy += h-1; }

	val = (radeon_get_dstbpp(bpp) << 8) | RADEON_GMC_CLR_CMP_CNTL_DIS;
	radeon_fifo_wait(3);
	MMOUTL(RADEON_DP_GUI_MASTER_CNTL,
			val
			| RADEON_GMC_BRUSH_NONE
			| RADEON_GMC_SRC_DATATYPE_COLOR
			| RADEON_ROP3_S
			| RADEON_DP_SRC_SOURCE_MEMORY );
	MMOUTL(RADEON_DP_WRITE_MASK, 0xffffffff);
	MMOUTL(RADEON_DP_CNTL, (xdir>=0 ? RADEON_DST_X_LEFT_TO_RIGHT : 0)
			| (ydir>=0 ? RADEON_DST_Y_TOP_TO_BOTTOM : 0));

	radeon_fifo_wait(3);
	MMOUTL(RADEON_SRC_Y_X, (sy << 16) | sx);
	MMOUTL(RADEON_DST_Y_X, (dy << 16) | dx);
	MMOUTL(RADEON_DST_HEIGHT_WIDTH, (h << 16) | w);


	radeon_engine_idle();
}

#endif /* RADEON 7000 */
