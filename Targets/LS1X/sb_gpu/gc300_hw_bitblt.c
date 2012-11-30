#include "gcSdk.h"
#include "testImages.h"

#undef GC300_SROLLUP_DEBUG

int gc300_init_one = 0;

void gc300_hw_bitblt(unsigned int Bpp, unsigned int winx, unsigned int winy, unsigned int font_height)
{
	gcSURFACEINFO* Target = &gcDisplaySurface;
	gcSURFACEINFO Src;

	if(!gc300_init_one) {
		*((volatile unsigned int*)0xbfd00420) &= ~0x00100000;	/* 使能GPU */
		gcAppInit();
		gcSelect2DPipe();
		gc300_init_one = 1;
	}

	// Init target surface.
	Src.address = Target->address; //+ 16*winx*Bpp; // font_height*winx*Bpp;
	Src.stride  = Target->stride; // gcSCREENWIDTH * gcGetPixelSize(gcSCREENFORMAT) / 8;
	Src.format  = Target->format;

	// Init coordinates.
	Src.rect.left   = 0;
	Src.rect.top    = 0;
	Src.rect.right  = winx;
	Src.rect.bottom = winy;

	// Init clipping.
	Src.clip.left   = 0;
	Src.clip.top    = 0; // font_height; //zgj
	Src.clip.right  = winx;
	Src.clip.bottom = winy;

	// Compute initial rect.
	Src.rect.left   = Target->rect.left = 0;
	Src.rect.right  = Target->rect.right = winx;
	Target->rect.top = 0;
	Src.rect.top    = Target->rect.top + font_height;
	Target->rect.bottom = winy - font_height;
	Src.rect.bottom = winy;

	// Blit the image.
	gcBitBlt_SC(Target, &Src,&Target->rect, &Src.rect);
#if 0
    gcBitBlt(Target, &Src,
				&Target->rect, &Src.rect,
				 0xCC, 0xCC,
				 NULL,			// No brush.
				 AQDE_SRC_CONFIG_TRANSPARENCY_OPAQUE,
				 ~0,			// Transparency color is ignored.
				 NULL,			// No mask.
				 ~0,			// Mask pack is ignored.
			//	 gcTRUE);		// Relative  coordinates.
				 gcFALSE);		// Absolute coordinates.

#endif

#if 0
    Target->rect.top    =  winy - font_height ;
    Target->rect.bottom = winy ;
	gcClear(Target, &Target->rect, BLACK32);
#endif

	// Start.
	gcFlush2DAndStall();
	gcStart();
	gcFlushDisplay();

	// Free the image.
	gcMemFree();
}

void gc300_hw_rectfill(unsigned int bpp, unsigned int winx, unsigned int winy, unsigned int font_height)
{
	gcSURFACEINFO* Target = &gcDisplaySurface;
#if 1
	Target->rect.left   =  0;
	Target->rect.right  =  winx;
	Target->rect.top    =  winy - font_height;
	Target->rect.bottom =  winy;
	gcClear(Target, &Target->rect, BLACK32);
#endif
	gcFlush2DAndStall();
	gcStart();      
	gcFlushDisplay();
}

#ifdef GC300_SROLLUP_DEBUG
void gc300_hw_bitblt_t(unsigned int winx, unsigned int winy)
{
	gcSURFACEINFO* Target = &gcDisplaySurface;
	gcSURFACEINFO Src;

	// Init target surface.
	Src.address = Target->address;
	Src.stride  = Target->stride; // gcSCREENWIDTH * gcGetPixelSize(gcSCREENFORMAT) / 8;
	Src.format  = Target->format;

	// Init clipping.
	Src.clip.left   = 0;
	Src.clip.top    = 0; // font_height; //zgj
	Src.clip.right  = winx;
	Src.clip.bottom = winy;

	// Compute initial rect.
	Src.rect.left   = 0;
	Src.rect.right  = winx;
	Target->rect.left = winx;
	Target->rect.right = winx + winx;
	Src.rect.top = 0;
	Src.rect.bottom = winy;
	Target->rect.top = winy;
	Target->rect.bottom = winy + winy;

	// Blit the image.
//	gcBitBlt_SC(Target, &Src, &Target->rect, &Src.rect);
	/* 把一块内存的数据传送到令一块 必须0xCC 0xCC 设置透明度 Relative或Absolute(会被拉伸或压缩) */
	gcBitBlt(Target, &Src, &Target->rect, &Src.rect, 0xCC, 0xCC, NULL, 
		AQDE_SRC_CONFIG_TRANSPARENCY_OPAQUE, ~0, NULL, ~0, 0);
//	1);		// Relative coordinates.
//	0);		// Absolute coordinates.

	/* 旋转180 */
/*	gcBitBltRot(Target, &Src, &Target->rect, &Src.rect,
				0xCC, 0xCC,
				NULL,			// No brush.
				AQDE_SRC_CONFIG_TRANSPARENCY_OPAQUE,
				~0,			// Transparency color is ignored.
				NULL,			// No mask.
				~0,			// Mask pack is ignored.
				0,		    // Absolute coordinates.
				0,   // source rotation angle
				5,   // dst rotation angle
				winx,		// src surface width;  
				winy,   	// src surface height;
				winx,		// dest surface width;
				winy);	// dest surface height;
*/
	// Start.
	gcFlush2DAndStall();
	gcStart();
	gcFlushDisplay();

	// Free the image.
	gcMemFree();
}

void testRotation_t(unsigned int winx, unsigned int winy)
{
	UINT32 imgWidth, imgHeight;
	UINT32 trgWidth, trgHeight;
	UINT32 srcRotAngle;
	UINT32 dstRotAngle;
	UINT32 i,j;

	gcSURFACEINFO* Target = &gcDisplaySurface;
	gcSURFACEINFO Src;

	// Init target surface.
	Src.address = Target->address;
	Src.stride  = Target->stride; // gcSCREENWIDTH * gcGetPixelSize(gcSCREENFORMAT) / 8;
	Src.format  = Target->format;

	// Init clipping.
	Src.clip.left   = 0;
	Src.clip.top    = 0; // font_height; //zgj
	Src.clip.right  = winx;
	Src.clip.bottom = winy;

	// Compute initial rect.
	Src.rect.left   = 0;
	Src.rect.right  = winx;
	Target->rect.left = winx;
	Target->rect.right = winx + winx;
	Src.rect.top = 0;
	Src.rect.bottom = winy;
	Target->rect.top = winy;
	Target->rect.bottom = winy + winy;

	for (i=0; i<10; i++) {
		//src/dest rotate 90 degree;
		srcRotAngle = 0x4 + i%3;
		dstRotAngle = 0x6 - i%3;
		
		// Compute sizes.
		// source window;
	    if (srcRotAngle == 0x04 | srcRotAngle == 0x06) {
	    	imgWidth  = winy;
			imgHeight = winx;
	    } else {
			imgWidth  = winx;
			imgHeight = winy;
		}

		// destination window;
		trgWidth  = gcSurfaceWidth(Target);
		trgHeight = gcSurfaceHeight(Target);

		// Clear the frame.
		//gcClear(Target, &Target->rect, DEADBEEF16);

		// Blit the image.
		
		gcBitBltRot(Target, &Src, &Target->rect, &Src.rect,
				 0xCC, 0xCC,
				 NULL,			// No brush.
				 AQDE_SRC_CONFIG_TRANSPARENCY_OPAQUE,
				 ~0,			// Transparency color is ignored.
				 NULL,			// No mask.
				 ~0,			// Mask pack is ignored.
				 0,		    // Absolute coordinates. 拉伸或压缩
				 srcRotAngle,   // source rotation angle
	 			 dstRotAngle,   // dst rotation angle
				 imgWidth,		// src surface width;  
				 imgHeight,   	// src surface height;
				 trgWidth,		// dest surface width;
				 trgHeight);	// dest surface height;

		// Start.
		gcFlush2DAndStall();
		gcStart();
		gcFlushDisplay();
		for (j=1500;j>1;j--) {
			printf("");
		}
	}
/*
	gcBitBltRot(Target, &Src, &Target->rect, &Src.rect,
				 0xCC, 0xCC,
				 NULL,			// No brush.
				 AQDE_SRC_CONFIG_TRANSPARENCY_OPAQUE,
				 ~0,			// Transparency color is ignored.
				 NULL,			// No mask.
				 ~0,			// Mask pack is ignored.
				 0,		    // Absolute coordinates.
				 0,   // source rotation angle
	 			 0,   // dst rotation angle
				 0,		// src surface width;  
				 0,   	// src surface height;
				 0,		// dest surface width;
				 0);	// dest surface height;

	// Start.
	gcFlush2DAndStall();
	gcStart();
	gcFlushDisplay();*/
	// Free the image.
	gcMemFree();
}

void gc300_hw_rectfill_t(unsigned int winx, unsigned int winy, 
		unsigned int width, unsigned int height, int color)
{
	gcSURFACEINFO* Target = &gcDisplaySurface;

	Target->rect.left   =  winx;
	Target->rect.right  =  winx + width;
	Target->rect.top    =  winy;
	Target->rect.bottom =  winy + height;

	gcClear(Target, &Target->rect, color);

	gcFlush2DAndStall();
	gcStart();
	gcFlushDisplay();
}

int test_sc_gc300(void)
{
	gcAppInit();

	// Virtualize target buffer.
#if gcENABLEVIRTUAL
	gcVirtualizeSurface(&target);
#endif

	// Switch to 2D pipe.
	gcSelect2DPipe();

	gc300_hw_bitblt_t(350, 350);
//	testRotation_t(250, 350);
	gc300_hw_rectfill_t(50, 50, 160, 160, YELLOW32);
	gc300_hw_rectfill_t(530, 150, 260, 220, GREEN32);
	return 0;
}

static const Cmd Cmds[] =
{
    {"GPU Test"},
    {"gc_test", "", 0, "test GC300 bitblt Rotation and rectfill sccreen", test_sc_gc300, 0, 99, CMD_REPEAT},
    {0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void init_cmd() {
    cmdlist_expand(Cmds, 1);
}
#endif
