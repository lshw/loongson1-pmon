#include "gcSdk.h"

void gcSetStretchFactor(
	UINT32 HorFactor,
	UINT32 VerFactor
	)
{
	gcLoadState(AQDEStretchFactorLowRegAddrs, 2,

				// AQDEClipTopLeft.
				SETFIELD(0, AQDE_STRETCH_FACTOR_LOW, X, HorFactor),

				// AQDEClipBottomRight.
				SETFIELD(0, AQDE_STRETCH_FACTOR_HIGH, Y, VerFactor));
}

void gcSetVideoSource(
	gcSURFACEINFO* Surface,
	gcRECT* SrcRect,
	gcPOINT* Origin
	)
{
	gcLoadState(AQDESrcAddressRegAddrs, 2,

				// AQDESrcAddress.
				Surface->address,

				// AQDESrcStride.
				Surface->stride);

	gcLoadState(AQDESrcConfigRegAddrs, 1,
				SETFIELD(0, AQDE_SRC_CONFIG, FORMAT, Surface->format));

	gcLoadState(UPlaneAddressRegAddrs, 4,

				// UPlaneAddress
				Surface->addressU,

				// UPlaneStride
				Surface->strideU,

				// VPlaneAddress
				Surface->addressV,

				// VPlaneStride
				Surface->strideV);

	gcLoadState(AQVRSourceImageLowRegAddrs, 4,

				// AQVRSourceImageLow
				SETFIELD(0, AQVR_SOURCE_IMAGE_LOW, LEFT, SrcRect->left)
				| SETFIELD(0, AQVR_SOURCE_IMAGE_LOW, TOP, SrcRect->top),

				// AQVRSourceImageHigh
				SETFIELD(0, AQVR_SOURCE_IMAGE_HIGH, RIGHT, SrcRect->right)
				| SETFIELD(0, AQVR_SOURCE_IMAGE_HIGH, BOTTOM, SrcRect->bottom),

				// AQVRSourceOriginLow
				SETFIELD(0, AQVR_SOURCE_ORIGIN_LOW, X, Origin->x),

				// AQVRSourceOriginHigh
				SETFIELD(0, AQVR_SOURCE_ORIGIN_HIGH, Y, Origin->y));
}

void gcSetVideoTarget(
	gcSURFACEINFO* Surface,
	gcRECT* TrgRect
	)
{
	gcLoadState(AQDEDestAddressRegAddrs, 2,

				// AQDEDestAddress.
				Surface->address,

				// AQDEDestStride.
				Surface->stride);

	gcLoadState(AQDEDestConfigRegAddrs, 1,

				// AQDEDestConfig.
				SETFIELD(0, AQDE_DEST_CONFIG, FORMAT, Surface->format));

	gcLoadState(AQVRTargetWindowLowRegAddrs, 2,

				// AQVRTargetWindowLow
				SETFIELD(0, AQVR_TARGET_WINDOW_LOW, LEFT, TrgRect->left)
				| SETFIELD(0, AQVR_TARGET_WINDOW_LOW, TOP, TrgRect->top),

				// AQVRTargetWindowHigh
				SETFIELD(0, AQVR_TARGET_WINDOW_HIGH, RIGHT, TrgRect->right)
				| SETFIELD(0, AQVR_TARGET_WINDOW_HIGH, BOTTOM, TrgRect->bottom));
}

void gcSetTargetFetch(
	UINT32 Default
	)
{
	UINT32 fetchOverride = Default
		? AQPE_CONFIG_DESTINATION_FETCH_DEFAULT
		: AQPE_CONFIG_DESTINATION_FETCH_DISABLE;

	gcLoadState(AQPEConfigRegAddrs, 1,
				SETFIELDVALUE(~0, AQPE_CONFIG, MASK_DESTINATION_FETCH, ENABLED) &
				SETFIELD     (~0, AQPE_CONFIG,      DESTINATION_FETCH, fetchOverride));
}

void gcProgramKernelTable(
	gcKERNELTABLE* Table
	)
{
	// Define kernel weight state array.
	UINT32 weightArray[WEIGHT_PAIR_MAX_COUNT];

	// Define kernel pair pointer.
	UINT32* weightPair = weightArray;

	// Odd/even flag.
	int odd = 0;

	// Loop variables.
	int kernelIndex, subpixelPos;

	// Copy weights into the linear state buffer.
	for (subpixelPos = 0; subpixelPos < KERNEL_LOAD_COUNT; subpixelPos++)
	{
		for (kernelIndex = 0; kernelIndex < MAX_KERNEL_SIZE; kernelIndex++)
		{
			// Determine the start bit.
			UINT32 start = odd? 16 : 0;

			// Set the current weight.
			SETBITS32(
				weightPair,
				start,
				start + 15,
				Table->kernel[subpixelPos].weight[kernelIndex]
				);

			// Update pair pointer as needed.
			if (odd)
			{
				weightPair++;
			}
			else
			{
				SETBITS32(weightPair, 16, 31, 0);
			}

			// Update even/odd flag.
			odd = (odd + 1) & 1;
		}
	}

	gcLoadStatePtr(
		AQDEFilterKernelRegAddrs,
		WEIGHT_PAIR_MAX_COUNT,
		weightArray
		);
}

void gcHorFilterBlit(
	gcSURFACEINFO* Target,
	gcSURFACEINFO* Source,
	gcRECT* TrgRect,
	gcRECT* SrcRect,
	gcPOINT* SrcOrigin
	)
{
	gcPOINT srcOrigin;

	// Set to full surface if no rects provided.
	if (TrgRect == NULL)
	{
		TrgRect = &Target->rect;
	}

	if (SrcRect == NULL)
	{
		SrcRect = &Source->rect;
	}

	if (SrcOrigin == NULL)
	{
		srcOrigin.x = (SrcRect->left << 16) + 0x8000;
		srcOrigin.y = (SrcRect->top  << 16) + 0x8000;

		SrcOrigin = &srcOrigin;
	}

	// Set source.
	gcSetVideoSource(Source, SrcRect, SrcOrigin);

	// Set destination.
	gcSetVideoTarget(Target, TrgRect);

	// We can disable the destination fetch since we are rendering
	// into a temporaty buffer and don't care about the content around
	// the rendering area.
	gcSetTargetFetch(gcFALSE);

	// Start the horizontal blit.
	gcStartVR(gcTRUE);

	// Restore default destination fetch.
	gcSetTargetFetch(gcTRUE);
}

void gcVerFilterBlit(
	gcSURFACEINFO* Target,
	gcSURFACEINFO* Source,
	gcRECT* TrgRect,
	gcRECT* SrcRect,
	gcPOINT* SrcOrigin
	)
{
	gcPOINT srcOrigin;

	// Set to full surface if no rects provided.
	if (TrgRect == NULL)
	{
		TrgRect = &Target->rect;
	}

	if (SrcRect == NULL)
	{
		SrcRect = &Source->rect;
	}

	if (SrcOrigin == NULL)
	{
		srcOrigin.x = (SrcRect->left << 16) + 0x8000;
		srcOrigin.y = (SrcRect->top  << 16) + 0x8000;

		SrcOrigin = &srcOrigin;
	}

	// Set source.
	gcSetVideoSource(Source, SrcRect, SrcOrigin);

	// Set destination.
	gcSetVideoTarget(Target, TrgRect);

	// Start the vertical blit.
	gcStartVR(gcFALSE);
}

void gcStartVR(
	UINT32 Horizontal
	)
{
	UINT32 blitType = Horizontal
		? AQVR_CONFIG_START_HORIZONTAL_BLIT
		: AQVR_CONFIG_START_VERTICAL_BLIT;

	gcLoadState(AQVRConfigRegAddrs, 1,
				SETFIELDVALUE(~0, AQVR_CONFIG, MASK_START, ENABLED) &
				SETFIELD     (~0, AQVR_CONFIG,      START, blitType)
				);
}
