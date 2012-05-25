#include "gcSdk.h"
#include "testImages.h"

#undef GC300_SROLLUP_DEBUG

int gc300_init_one = 0;

void gc300_hw_bitblt(unsigned int Bpp,unsigned int winx,unsigned int winy,unsigned int font_height)
{

	gcSURFACEINFO* Target =&gcDisplaySurface;
	gcSURFACEINFO Src ;

    if(!gc300_init_one)
    {
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
    Target->rect.bottom = winy - font_height ;
    Src.rect.bottom = winy ;

#if 0

    *(volatile unsigned int *)(0xA5000000+i) = bpp;
    i += 4;
    *(volatile unsigned int *)(0xA5000000+i) = winx;
    i += 4;
    *(volatile unsigned int *)(0xA5000000+i) = winy;
    i += 4;
    *(volatile unsigned int *)(0xA5000000+i) = font_height;
    i += 4;
    *(volatile unsigned int *)(0xA5000000+i) = Target->rect.left;
    i += 4;
    *(volatile unsigned int *)(0xA5000000+i) = Target->rect.right;
    i += 4;
    *(volatile unsigned int *)(0xA5000000+i) = Target->rect.top;
    i += 4;
    *(volatile unsigned int *)(0xA5000000+i) = Target->rect.bottom;
    i += 4;
    *(volatile unsigned int *)(0xA5000000+i) = Src->rect.left;
    i += 4;
    *(volatile unsigned int *)(0xA5000000+i) = Src->rect.right;
    i += 4;
    *(volatile unsigned int *)(0xA5000000+i) = Src->rect.top;
    i += 4;
    *(volatile unsigned int *)(0xA5000000+i) = Src->rect.bottom;
    i += 4;
    *(volatile unsigned int *)(0xA5000000+i) = Src->address;
    i += 4;
    #endif
    //    printf("%d,%d,%d,%d,%d,%d,%d,%d\n",Target->rect.left,Target->rect.right,Target->rect.top,Target->rect.bottom,Src->rect.left,Src->rect.right,Src->rect.top,Src->rect.bottom);

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

void gc300_hw_rectfill(unsigned int bpp,unsigned int winx,unsigned int winy,unsigned int font_height)
{
    
	gcSURFACEINFO* Target =&gcDisplaySurface;
#if 1
    Target->rect.left   =  0 ;
    Target->rect.right  =  winx ;
    Target->rect.top    =  winy - font_height ;
    Target->rect.bottom =  winy ;
	gcClear(Target, &Target->rect, BLACK32);
#endif
    gcFlush2DAndStall();
    gcStart();      
    gcFlushDisplay();
}


#ifdef GC300_SROLLUP_DEBUG
int test_sc_gc300(void)
{
    gcAppInit();

    // Virtualize target buffer.
#if gcENABLEVIRTUAL
    gcVirtualizeSurface(&target);
#endif

    // Switch to 2D pipe.
    gcSelect2DPipe();

    gc300_hw_bitblt(4,640,480,16);
    return 0;
}

static const Cmd Cmds[] =
{
    {"GPU Test"},
    {"sc_test", "", 0, "random test GC300 scrollup sccreen", test_sc_gc300, 0, 99, CMD_REPEAT},
    {0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void init_cmd() {
    cmdlist_expand(Cmds, 1);
}
#endif
