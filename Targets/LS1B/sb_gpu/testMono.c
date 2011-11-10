#include "gcSdk.h"
//#include "testImages.h"
#include "img_Mono_Vivante_97x33x1.h"

void testMonoExpand(
	gcSURFACEINFO* Target
	)
{
	// Set source packing.
	UINT32 srcPack = AQDE_SRC_CONFIG_PACK_PACKED8;

	// Set transparency.
	UINT32 transparency = AQDE_SRC_CONFIG_TRANSPARENCY_OPAQUE;

	// Set mono transparency.
	UINT32 monoTransparency = AQDE_SRC_CONFIG_MONO_TRANSPARENCY_BACKGROUND;

	// Set colors.
	UINT32 fgColor = GREEN32;
	UINT32 bgColor = BLUE32;

	// Set image.
	gcIMAGEDESCRIPTOR* Image = &mono_vivante_97x33x1;

	// Determine sizes.
	UINT32 srcWidth  = gcImageWidth(Image);
	UINT32 srcHeight = gcImageHeight(Image);
	UINT32 trgWidth  = gcSurfaceWidth(Target);
	UINT32 trgHeight = gcSurfaceHeight(Target);

	// Determine the loop count.
	UINT32 loopCount = _MIN(srcWidth, srcHeight) / 2 - 1;

	// Define target point.
	gcPOINT point;

	// Save default clipping window.
	gcRECT saveClip = Target->clip;

	// Define current clipping rectangle.
	gcRECT currClip;

	// Load the image.
	gcLoadImage(&mono_vivante_97x33x1);

	// Set target point.
	point.x = (trgWidth  - srcWidth)  / 2;
	point.y = (trgHeight - srcHeight) / 2;

	// Set initial rectangle.
	currClip.left   = point.x;
	currClip.top    = point.y;
	currClip.right  = currClip.left + srcWidth;
	currClip.bottom = currClip.top  + srcHeight;

	// Start the test.
	while (loopCount--)
	{
		// Clear the frame.
		//gcClear(Target, NULL, DEADBEEF16);
		gcClear(Target, NULL, BLACK32);

		// Set clipping.
		Target->clip = currClip;

		// Execute mono blit.
		gcMonoBitBlt(Target, Image, &point,
					 fgColor, bgColor,
					 0xCC, 0xAA,
					 srcPack,
					 NULL,		// No brush.
					 gcTRUE,	// Enable color convert.
					 transparency,
					 monoTransparency);

		// Restore clipping.
		Target->clip = saveClip;

		// Update clipping.
		currClip.left++;
		currClip.top++;
		currClip.right--;
		currClip.bottom--;

		// Start.
		gcFlush2DAndStall();
		gcStart();
		gcFlushDisplay();
	}

	// Free the image.
	gcMemFree();
}

void testMonoExpandRandom(
	gcSURFACEINFO* Target
	)
{
	UINT32 loopCount = 50;

	gcIMAGEDESCRIPTOR* Image;
	gcPOINT point;
	gcRECT clip;

	UINT32 srcWidth;
	UINT32 srcHeight;
	UINT32 trgWidth;
	UINT32 trgHeight;

	// Load the image.
	gcLoadImage(&mono_vivante_97x33x1);

	// Set image.
	Image = &mono_vivante_97x33x1;

	// Clear the frame.
	//gcClear(Target, NULL, DEADBEEF16);
	gcClear(Target, NULL, BLACK32);

	// Determine sizes.
	srcWidth  = gcImageWidth(Image);
	srcHeight = gcImageHeight(Image);
	trgWidth  = gcSurfaceWidth(Target);
	trgHeight = gcSurfaceHeight(Target);

	while (loopCount--)
	{
		// Generate source packing.
		UINT32 srcPack = gcRand(3);

		// Generate transparency.
		UINT32 transparency = gcRand(1);

		// Generate mono transparency.
		UINT32 monoTransparency = gcRand(1);

		// Generate source size.
		UINT32 curWidth  = gcRand(srcWidth  - 1) + 1;
		UINT32 curHeight = gcRand(srcHeight - 1) + 1;

		// Generate colors.
		UINT32 fgColor = gcRand(0x00FFFFFF);
		UINT32 bgColor = gcRand(0x00FFFFFF);

		// Generate the target origin.
		point.x = Target->rect.left + gcRand(trgWidth  - curWidth);
		point.y = Target->rect.top  + gcRand(trgHeight - curHeight);

		// Save current clipping rect and set new one.
		clip = Target->clip;
		Target->clip.left   = point.x;
		Target->clip.top    = point.y;
		Target->clip.right  = Target->clip.left + curWidth;
		Target->clip.bottom = Target->clip.top  + curHeight;

		// Execute mono blit.
		gcMonoBitBlt(Target, Image, &point,
					 fgColor, bgColor,
					 0xCC, 0xAA,
					 srcPack,
					 NULL,		// No brush.
					 gcTRUE,	// Enable color convert.
					 transparency,
					 monoTransparency);

		// Restore clipping.
		Target->clip = clip;

		// Start.
		gcFlush2DAndStall();
		gcStart();
		gcFlushDisplay();
	}

	// Free the image.
	gcMemFree();
}
