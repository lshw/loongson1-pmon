#include "gcSdk.h"
#include "testImages.h"
//#include "img_Exige_1_0160x108x24.h"
#include "img_Poppy_480x325_UYVY.h"
#include "img_Poppy_480x325_YUY2.h"

static void GenerateRandomTargetRect(
	gcSURFACEINFO* Target,
	gcRECT* TrgRect
	)
{
	// Compute sizes.
	UINT32 trgWidth  = gcSurfaceWidth(Target);
	UINT32 trgHeight = gcSurfaceHeight(Target);

    
	UINT32 dstWidth  = gcRand(trgWidth  - 1) + 1;
	UINT32 dstHeight = gcRand(trgHeight - 1) + 1;

	// Compute rectangle.
	TrgRect->left   = Target->rect.left + (trgWidth  - dstWidth)  / 2;
	TrgRect->top    = Target->rect.top  + (trgHeight - dstHeight) / 2;
	TrgRect->right  = TrgRect->left + dstWidth;
	TrgRect->bottom = TrgRect->top  + dstHeight;
}

void testFilterBlit(
	gcSURFACEINFO* Target
	)
{
	UINT32 loopCount = 10;
	gcRECT targetRect;
	UINT32 horKernelSize;
	UINT32 verKernelSize;
    gcIMAGEDESCRIPTOR* Image;


	// Load the image.
	//zgj gcLoadImage(&vasiliy_476x320x24);
	gcLoadImage(&exige_0160x108x24);

    // Set image.
//    Image = &exige_0160x108x24;


	// Start the test.
	while (loopCount--)
	{
		// Generate random destination rectangle.
		GenerateRandomTargetRect(Target, &targetRect);

		// Clear the frame.
		//gcClear(Target, NULL, DEADBEEF16);
		gcClear(Target, NULL, BLACK32);

		// Generate random kernel size.
		horKernelSize = gcRand(9);
		verKernelSize = gcRand(9);

		// Generate a random filter blit.
#if 0
        gcFilterBlit(
			Target, &vasiliy_476x320x24.surface,
			&targetRect, &vasiliy_476x320x24.surface.rect, NULL,
			horKernelSize, verKernelSize
			);
        /*gcFilterBlit(
			Target, &Image->surface,
			&targetRect, &Image->surface.rect, NULL,
			horKernelSize, verKernelSize
			);*/

#else
        gcFilterBlit(
			Target, &exige_0160x108x24.surface,
			&targetRect, &exige_0160x108x24.surface.rect, NULL,
			horKernelSize, verKernelSize
			);

#endif
		gcFlushDisplay();
	}

	// Free the image.
	gcMemFree();
}

#if 1
void testFilterBlitFormats(
	gcSURFACEINFO* Target
	)
{
	UINT32 loopCount, idxImage, idxFormat;
	UINT32 srcWidth, srcHeight;

	gcSURFACEINFO temporary1;
	gcSURFACEINFO temporary2;

	gcRECT rect1;
	gcRECT rect2;

	gcSURFACEINFO* srcSurface;
	gcSURFACEINFO* trgSurface;

	gcRECT* srcRect;
	gcRECT* trgRect;

	static gcIMAGEDESCRIPTOR* SrcImages[] =
	{
		&poppy_480x325_YUY2,
		&poppy_480x325_UYVY,
	};

	static UINT32 Formats[] =
	{
		AQ_DRAWING_ENGINE_FORMAT_FORMAT_A8R8G8B8,
		AQ_DRAWING_ENGINE_FORMAT_FORMAT_X8R8G8B8,
		AQ_DRAWING_ENGINE_FORMAT_FORMAT_R5G6B5,
		AQ_DRAWING_ENGINE_FORMAT_FORMAT_A1R5G5B5,
		AQ_DRAWING_ENGINE_FORMAT_FORMAT_X1R5G5B5,
		AQ_DRAWING_ENGINE_FORMAT_FORMAT_A4R4G4B4,
		AQ_DRAWING_ENGINE_FORMAT_FORMAT_X4R4G4B4,
	};

	// Load images.
	gcLoadImage(&poppy_480x325_YUY2);
	gcLoadImage(&poppy_480x325_UYVY);

	

	// Allocate temporary surfaces with largest pixels.
	gcAllocateSurface(&temporary1,
	                  gcDisplaySurface.rect.right ,/// 2,
	                  gcDisplaySurface.rect.bottom,
	                  AQ_DRAWING_ENGINE_FORMAT_FORMAT_A8R8G8B8);

	gcAllocateSurface(&temporary2,
	                  gcDisplaySurface.rect.right ,/// 2,
	                  gcDisplaySurface.rect.bottom,
	                  AQ_DRAWING_ENGINE_FORMAT_FORMAT_A8R8G8B8);

	// Start the test.
	for (loopCount = 0; loopCount < 10; loopCount++)
	{
		for (idxImage = 0; idxImage < CountOf(SrcImages); idxImage++)
		//for (idxImage = 0; idxImage < 1; idxImage++)
		{
			// Init the source.
			srcRect    = &SrcImages[idxImage]->surface.rect;
			srcSurface = &SrcImages[idxImage]->surface;	
		
			// Init the target.
			trgRect    = &rect1;
			trgSurface = &temporary1;

			for (idxFormat = 0; idxFormat < CountOf(Formats); idxFormat++)
			{
				// Init the target surface with the current format.
				gcInitSurface(trgSurface,
				              trgSurface->rect.right,
				              trgSurface->rect.bottom,
				              Formats[idxFormat]);

				// Generate random destination rectangle.
				GenerateRandomTargetRect(trgSurface, trgRect);

				// Generate a random filter blit.
				gcFilterBlit(
					trgSurface, srcSurface,
					trgRect, srcRect, NULL,
					9, 9
					);

				// Exchange source with destination.
				{
					gcRECT* tempRect;
					gcSURFACEINFO* tempSurface;

					if (idxFormat == 0)
					{
						srcRect = trgRect;
						trgRect = &rect2;

						srcSurface = trgSurface;
						trgSurface = &temporary2;
					}
					else
					{
						tempRect = srcRect;
						srcRect = trgRect;
						trgRect = tempRect;

						tempSurface = srcSurface;
						srcSurface = trgSurface;
						trgSurface = tempSurface;
					}
				}
			}

			// Clear the frame.
			//gcClear(Target, NULL, DEADBEEF16);
			gcClear(Target, NULL, BLACK32);

			// Center the rectangle.
			srcWidth  = srcRect->right  - srcRect->left;
			srcHeight = srcRect->bottom - srcRect->top;

			trgRect->left = (Target->rect.right  - srcWidth)  / 2;
			trgRect->top  = (Target->rect.bottom - srcHeight) / 2;
			trgRect->right  = trgRect->left + srcWidth;
			trgRect->bottom = trgRect->top  + srcHeight;

			// Copy the final result to the frame buffer.
			gcBitBlt(Target, srcSurface,
					 trgRect, srcRect,
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
	}

	// Free temporary buffers and images.
	gcMemFree();
	gcMemFree();
	gcMemFree();
	gcMemFree();
}

#endif
