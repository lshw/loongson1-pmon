#include "gcSdk.h"
//#include <rt_misc.h>
#include "display.h"
//#include "lcd.h"

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

extern unsigned long GPU_fbaddr;

void gcAppInit(
	void
	)
{
	// Init display.
//	apCLCD_Init(1, 1);

	// Set register base address.
	gcREG_BASE = 0xBC200000;

#if 1  //zgj-2010-3-22
    gcVIDEOBASE = 0xA2000000;
    gcVIDEOSIZE = 0x2000000;
#endif

	// Init memory.
	gcMemReset();

	// Init target surface.
	//gcDisplaySurface.address = gcSCREENADDR;
	gcDisplaySurface.address = GPU_fbaddr;
	gcDisplaySurface.stride  = gcSCREENWIDTH * gcGetPixelSize(gcSCREENFORMAT) / 8;
	gcDisplaySurface.format  = gcSCREENFORMAT;

	// Init coordinates.
	gcDisplaySurface.rect.left   = 0;
	gcDisplaySurface.rect.top    = 0;
	gcDisplaySurface.rect.right  = gcSCREENWIDTH; 
	gcDisplaySurface.rect.bottom = gcSCREENHEIGHT;

	// Init clipping.
	gcDisplaySurface.clip.left   = 0;
	gcDisplaySurface.clip.top    = 0;
	gcDisplaySurface.clip.right  = gcSCREENWIDTH; 
	gcDisplaySurface.clip.bottom = gcSCREENHEIGHT;
}

void gcFlushDisplay(
	void
	)
{
}
