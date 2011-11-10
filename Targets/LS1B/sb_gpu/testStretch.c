#include "gcSdk.h"
//#include "testImages.h"
#include "img_Exige_0160x108x24.h"

void testStretch(
	gcSURFACEINFO* Target
	)
{
	UINT32 loopCount = 50;

	gcIMAGEDESCRIPTOR* Image;
	float imgRatio;
	float trgRatio;
	UINT32 imgWidth, imgHeight;
	UINT32 trgWidth, trgHeight;
	gcRECT rect;
	UINT32 xStep;

	// Load the image.
	gcLoadImage(&exige_0160x108x24);

	// Set image.
	Image = &exige_0160x108x24;

	// Compute sizes.
	imgWidth  = gcImageWidth(Image);
	imgHeight = gcImageHeight(Image);

	trgWidth  = gcSurfaceWidth(Target);
	trgHeight = gcSurfaceHeight(Target);

	// Compute ratios.
	imgRatio = (float) imgWidth / imgHeight;
	trgRatio = (float) trgWidth / trgHeight;
//printf("----------------------------------imgWidth:%d ,imgHeight:%d ,trgWidth:%d ,trgHeight:%d ,imgRatio:%f ,trgRatio:%f \n",imgWidth,imgHeight,trgWidth,trgHeight,imgRatio,trgRatio);
	// Compute initial rect.
	if (imgRatio < trgRatio)
	{
		UINT32 width = (UINT32) (trgHeight * imgRatio);
		rect.left   = Target->rect.left + (trgWidth - width) / 2;
		rect.top    = Target->rect.top;
		rect.right  = rect.left + width;
		rect.bottom = Target->rect.bottom;
	}
	else
	{
		UINT32 height = (UINT32) (trgWidth / imgRatio);
		rect.left   = Target->rect.left;
		rect.top    = Target->rect.top + (trgHeight - height) / 2;;
		rect.right  = Target->rect.right;
		rect.bottom = rect.top + height;
	}

	// Determine the step.
	xStep = (trgWidth / 2 - 1) / loopCount;

	// Clear the frame.
	//gcClear(Target, &Target->rect, DEADBEEF16);
	gcClear(Target, &Target->rect, BLACK32);

	while (loopCount--)
	{
		// Blit the image.
		gcBitBlt(Target, &Image->surface,
				 &rect, &Image->surface.rect,
				 0xCC, 0xCC,
				 NULL,			// No brush.
				 AQDE_SRC_CONFIG_TRANSPARENCY_OPAQUE,
				 ~0,			// Transparency color is ignored.
				 NULL,			// No mask.
				 ~0,			// Mask pack is ignored.
				 gcFALSE);		// Absolute coordinates.

		// Update rect.
		{
			UINT32 width  = rect.right - rect.left - xStep * 2;
		    UINT32 height = (UINT32) (width / imgRatio);
			//UINT32 height = 108;

			rect.left   = Target->rect.left + (trgWidth  - width)  / 2;
			rect.top    = Target->rect.top  + (trgHeight - height) / 2;
			rect.right  = rect.left + width;
			rect.bottom = rect.top  + height;
//printf("------++++++++++++-------width:%d ,height:%d ,L:%d ,T:%d ,R:%d ,B:%d ,xStep:%d \n",width,height,rect.left,rect.top,rect.right,rect.bottom,xStep);
		}

		// Start.
		gcFlush2DAndStall();
		gcStart();
		gcFlushDisplay();
	}

	// Free the image.
	gcMemFree();
}

void testRandomStretch(
	gcSURFACEINFO* Target
	)
{
	UINT32 loopCount = 50;

	gcRECT dstRect;
	gcIMAGEDESCRIPTOR* Image;

	// Load the image.
	gcLoadImage(&exige_0160x108x24);

	// Set image.
	Image = &exige_0160x108x24;

	// Clear the frame.
	//gcClear(Target, NULL, DEADBEEF16);
	gcClear(Target, NULL, BLACK32);

	while (loopCount--)
	{
		// Generate destination rect.
		gcGenerateRect(&dstRect, &Target->rect);
		//gcGenerateRect_Stretch(&dstRect, &Target->rect);

		// Blit the image.
		gcBitBlt(Target, &Image->surface,
				 &dstRect, &Image->surface.rect,
				 0xCC, 0xCC,
				 NULL,			// No brush.
				 AQDE_SRC_CONFIG_TRANSPARENCY_OPAQUE,
				 ~0,			// Transparency color is ignored.
				 NULL,			// No mask.
				 ~0,			// Mask pack is ignored.
				 gcFALSE);		// Absolute coordinates.

		// Start.
		gcFlush2DAndStall();
		gcStart();
		gcFlushDisplay();
	}

	// Free the image.
	gcMemFree();
}
