#include "gcSdk.h"
//#include "testImages.h"
#include "img_F430_171x128x1.h"
#include "img_F430_171x128x24.h"
#include "img_F430_Babe_171x128x1.h"

void testMaskedBlitRandom(
	gcSURFACEINFO* Target
	)
{
	UINT32 loopCount = 10;

	gcRECT saveClip = Target->clip;
	gcRECT outerRect;
	gcRECT blitRect;
	gcRECT dstRect;

	gcBRUSH brush;

	gcIMAGEDESCRIPTOR* Image = &f430_171x128x24;
	gcIMAGEDESCRIPTOR* Mask[] =
	{
		&f430_171x128x1,
		&f430_babe_171x128x1
	};

	// Compute sizes.
	UINT32 imgWidth  = gcImageWidth(Image);
	UINT32 imgHeight = gcImageHeight(Image);

	UINT32 trgWidth  = gcSurfaceWidth(Target);
	UINT32 trgHeight = gcSurfaceHeight(Target);

	// Load the images.
	gcLoadImage(&f430_171x128x1);
	gcLoadImage_1(&f430_babe_171x128x1);
	gcLoadImage_2(&f430_171x128x24);

	// Init the brush.
	gcConstructSingleColorBrush(&brush, RED32, gcTRUE, ~0);

	// Clear the frame.
	//gcClear(Target, NULL, DEADBEEF16);
	gcClear(Target, NULL, BLACK32);

	while (loopCount--)
	{
		// Pick a mask.
		gcIMAGEDESCRIPTOR* mask = Mask[gcRand(CountOf(Mask) - 1)];

		// Generate packing.
		UINT32 maskPack = gcRand(3);

		// Generate destination rect.
		dstRect.left   = Target->rect.left + gcRand(trgWidth  - imgWidth);
		dstRect.top    = Target->rect.top  + gcRand(trgHeight - imgHeight);
		dstRect.right  = dstRect.left + imgWidth;
		dstRect.bottom = dstRect.top  + imgHeight;

		// Generate clipping.
		blitRect = dstRect;

		if (gcRand(1))
		{
			blitRect.left   += gcRand(imgWidth  / 2 - 1);
			blitRect.top    += gcRand(imgHeight / 2 - 1);
			blitRect.right  -= gcRand(imgWidth  / 2 - 1);
			blitRect.bottom -= gcRand(imgHeight / 2 - 1);
		}

		// Clear under the image.
		outerRect = blitRect;
		outerRect.left--;
		outerRect.top--;
		outerRect.right++;
		outerRect.bottom++;

		gcClear(Target, &outerRect, DEADBEEF16);
		gcRect(Target, &outerRect, &brush);

		// Flush before bitblit. gcRect draws a border, which will allocate
		// cache lines for the border pixels. The chip does not fetch the
		// destination anymore unless needed. When the bitblit comes in,
		// there will be cache hits on the lines allocated for the border,
		// but the pixels needed by the bitblit will not be there because
		// the destination is not fetched for them. This will cause some
		// garbage to show up. To avoid that, flush 2D cache here.
		gcLoadState(
			AQFlushRegAddrs, 1,
			SETFIELDVALUE(0, AQ_FLUSH, PE2D_CACHE, ENABLE)
			);

		// Set clipping.
		Target->clip = blitRect;

		// Blit the image.
		gcBitBlt(Target, &Image->surface,
				 &dstRect, &Image->surface.rect,
				 0xCC, 0xAA,
				 NULL,			// No brush.
				 ~0,			// Transparency is forced to mask.
				 ~0,			// Transparency color is ignored.
				 mask,
				 maskPack,
				 gcFALSE);		// Absolute coordinates.

		// Restore clipping.
		Target->clip = saveClip;

		// Start.
		gcFlush2DAndStall();
		gcStart();
		gcFlushDisplay();
	}

	// Free the images.
	gcMemFree();
	gcMemFree();
	gcMemFree();
}
