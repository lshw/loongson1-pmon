#include "gcSdk.h"

#define gcSCREENADDR	CLCD_FRAME_BASE1
#define gcSCREENWIDTH	XPIXELS
#define gcSCREENHEIGHT	YPIXELS
#if defined(CONFIG_VIDEO_16BPP)
#define gcSCREENFORMAT	AQ_DRAWING_ENGINE_FORMAT_FORMAT_R5G6B5
#elif defined(CONFIG_VIDEO_32BPP)
#define gcSCREENFORMAT	AQ_DRAWING_ENGINE_FORMAT_FORMAT_A8R8G8B8
#elif defined(CONFIG_VIDEO_15BPP)
#define gcSCREENFORMAT	AQ_DRAWING_ENGINE_FORMAT_FORMAT_X1R5G5B5
#elif defined(CONFIG_VIDEO_12BPP)
#define gcSCREENFORMAT	AQ_DRAWING_ENGINE_FORMAT_FORMAT_X4R4G4B4
#else
#define gcSCREENFORMAT	AQ_DRAWING_ENGINE_FORMAT_FORMAT_A8R8G8B8
#endif

UINT32 gcRAMSIZE = gcMEGABYTES(256);

gcSURFACEINFO gcDisplaySurface;

#if 0
__value_in_regs struct __initial_stackheap __user_initial_stackheap(
	unsigned R0,
	unsigned SP,
	unsigned R2,
	unsigned SL
	)
{
	struct __initial_stackheap config;

	gcSetMemoryAllocation();
	gcHEAPBASE  = gcCODESIZE;
	gcSTACKBASE = gcHEAPBASE  + gcHEAPSIZE;
	gcVIDEOBASE = gcSTACKBASE + gcSTACKSIZE;
	gcVIDEOSIZE = gcRAMSIZE - gcVIDEOBASE;

	config.heap_base   = gcHEAPBASE;
	config.stack_base  = gcSTACKBASE;
	config.heap_limit  = gcHEAPSIZE;
	config.stack_limit = gcSTACKSIZE;

	return config;
}
#endif

unsigned long GPU_fbaddr;

void gcAppInit(void)
{
	unsigned int xres, yres;
	// Init display.
//	apCLCD_Init(1, 1);

	// Set register base address.
	gcREG_BASE = 0xbc200000;

#if 1  //zgj-2010-3-22
    gcVIDEOBASE = 0xa2800000;
    gcVIDEOSIZE = 0x02000000;
#endif

	// Init memory.
	gcMemReset();

	xres = getenv("xres")? strtoul(getenv("xres"),0,0):FB_XSIZE;
    yres = getenv("yres")? strtoul(getenv("yres"),0,0):FB_YSIZE;
	// Init target surface.
//	gcDisplaySurface.address = gcSCREENADDR;
	gcDisplaySurface.address = GPU_fbaddr;
	gcDisplaySurface.stride  = xres * gcGetPixelSize(gcSCREENFORMAT) / 8;
	gcDisplaySurface.format  = gcSCREENFORMAT;

	// Init coordinates.
	gcDisplaySurface.rect.left   = 0;
	gcDisplaySurface.rect.top    = 0;
	gcDisplaySurface.rect.right  = xres; 
	gcDisplaySurface.rect.bottom = yres;

	// Init clipping.
	gcDisplaySurface.clip.left   = 0;
	gcDisplaySurface.clip.top    = 0;
	gcDisplaySurface.clip.right  = xres; 
	gcDisplaySurface.clip.bottom = yres;
}

void gcFlushDisplay(void)
{
}
