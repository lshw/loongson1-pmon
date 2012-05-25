#include "gcSdk.h"
#include <math.h>
#include "display.h"

UINT32 gcGetStretchFactor(
	UINT32 SrcSize,
	UINT32 DstSize
	)
{
	if (DstSize > 1)
	{
	
		return ((SrcSize - 1) << 16) / (DstSize - 1);
	}
	else
	{
		return 0;
	}
}

void gcComputeKernelTable(
	UINT32 SrcSize,
	UINT32 DstSize,
	UINT32 KernelSize,
	gcKERNELTABLE* Table
	)
{
	double scale, fscale;
	double subpixelStep;
	double subpixelOffset;

	int i, subpixelPos;
	int kernelHalf;

	// Compute kernel sizes.
	KernelSize |= 1;
	if (KernelSize > MAX_KERNEL_SIZE)
		KernelSize = MAX_KERNEL_SIZE;
	kernelHalf = KernelSize >> 1;

	// Horizontal scale factor.
	scale = ((double)DstSize) / SrcSize;

	// Compute scale adustment factor.
	fscale = (scale < 1.0)
		? scale				// Minification.
		: 1.0;				// Magnification.

	subpixelStep   = 1.0 / SUBPIXEL_COUNT;
	subpixelOffset = 0.5;

	for (subpixelPos = 0; subpixelPos < SUBPIXEL_COUNT; subpixelPos++)
	{
		// Compute weights.
		double weightSum = 0;

		// Current kernel array.
		double kernel[MAX_KERNEL_SIZE];

		// Determine the padding.
		int padding = (MAX_KERNEL_SIZE - KernelSize) / 2;

		// Compute the kernel.
		for (i = 0; i < MAX_KERNEL_SIZE; i++)
		{
			// Compute the kernel index.
			int index = i - padding;

			// Are we within range?
			if ((index >= 0) && (index < KernelSize))
			{
				double x = (index - kernelHalf + subpixelOffset) * fscale;

				if (KernelSize == 1)
				{
					kernel[i] = 1.0;
				}
				else
				{
					if (fabs(x) > kernelHalf)
					{
						kernel[i] = 0.0;
					}
					else if (x == 0)
					{
						kernel[i] = 1.0;
					}
					else
					{
						double pit = 3.14159265358979323846 * x;
						double pitd = pit / kernelHalf;

						double f1 = sin(pit) / pit;
						double f2 = sin(pitd) / pitd;

						kernel[i] = f1 * f2;
					}
				}

				// Update the sum.
				weightSum += kernel[i];
			}

			// Out of range.
			else
			{
				kernel[i] = 0;
			}
		}

		// Adjust weights so that the sum will be 1.0.
		for (i = 0; i < MAX_KERNEL_SIZE; i++)
		{
			// Compute the final current weight.
			double dblWeight = kernel[i] / weightSum;

			// Convert to fixed point.
			short fxdWeight;

			if (dblWeight == 0)
			{
				fxdWeight = (short) 0x0000;
			}
			else if (dblWeight >= 2.0)
			{
				fxdWeight = (short) 0x7FFF;
			}
			else if (dblWeight < -2.0)
			{
				fxdWeight = (short) 0x8000;
			}
			else
			{
				fxdWeight = (short) (dblWeight * (1 << 14));
			}

			// Set the current table entry.
			Table->kernel[subpixelPos].weight[i] = fxdWeight;
		}

		// Advance to the next subpixel.
		subpixelOffset -= subpixelStep;
	}
}

void gcComputeFilterBlit(
	gcSURFACEINFO* Target,
	gcSURFACEINFO* Source,
	gcRECT* TrgRect,
	gcRECT* SrcRect,
	gcRECT* TrgSubRect,
	UINT32 HorKernelSize,
	UINT32 VerKernelSize,
	gcFILTERPARAMETERS* Parameters
	)
{
	gcRECT srcSubRect;
	UINT32 tmpWidth, tmpHeight;

	UINT32 bytesPerSrcPixel;
	UINT32 bytesPerTrgPixel;

	UINT32 tempHorCoordMask;
	UINT32 tmpTopExtra, tmpBotExtra;

	////////////////////////////////////////////////////////////////////////////
	// Set rectangles if not specified.

	// Set the destination rectangle.
	if (TrgRect == NULL)
	{
		TrgRect = &Target->rect;
	}

	// Set the source rectangle.
	if (SrcRect == NULL)
	{
		SrcRect = &Source->rect;
	}

	// Set the destination sub rectangle.
	if (TrgSubRect == NULL)
	{
		static gcRECT rect;

		rect.left   = 0;
		rect.top    = 0;
		rect.right  = TrgRect->right  - TrgRect->left;
		rect.bottom = TrgRect->bottom - TrgRect->top;

		TrgSubRect = &rect;
	}

	////////////////////////////////////////////////////////////////////////////
	// Compute sizes.

	Parameters->srcSize.x = SrcRect->right  - SrcRect->left;
	Parameters->srcSize.y = SrcRect->bottom - SrcRect->top;

	Parameters->trgSize.x = TrgRect->right  - TrgRect->left;
	Parameters->trgSize.y = TrgRect->bottom - TrgRect->top;

	////////////////////////////////////////////////////////////////////////////
	// Compute and program the stretch factors.

	Parameters->horFactor = gcGetStretchFactor(
		Parameters->srcSize.x,
		Parameters->trgSize.x
		);

	Parameters->verFactor = gcGetStretchFactor(
		Parameters->srcSize.y,
		Parameters->trgSize.y
		);

	////////////////////////////////////////////////////////////////////////////
	// Determine temporary surface format.

	bytesPerSrcPixel = gcGetPixelSize(Source->format) / 8;
	bytesPerTrgPixel = gcGetPixelSize(Target->format) / 8;

	if (bytesPerSrcPixel > bytesPerTrgPixel)
	{
		Parameters->tempFormat = Source->format;
		tempHorCoordMask = (64 / bytesPerSrcPixel) - 1;
	}
	else
	{
		Parameters->tempFormat = Target->format;
		tempHorCoordMask = (64 / bytesPerTrgPixel) - 1;
	}

	////////////////////////////////////////////////////////////////////////////
	// Determine the source sub rectangle.

	// Compute the source sub rectangle that exactly represents
	// the destination sub rectangle.
	srcSubRect.left   =  TrgSubRect->left        * Parameters->horFactor;
	srcSubRect.top    =  TrgSubRect->top         * Parameters->verFactor;
	srcSubRect.right  = (TrgSubRect->right  - 1) * Parameters->horFactor + (1 << 16);
	srcSubRect.bottom = (TrgSubRect->bottom - 1) * Parameters->verFactor + (1 << 16);

	// Before rendering each destination pixel, the HW will select the
	// corresponding source center pixel to apply the kernel around.
	// To make this process precise we need to add 0.5 to source initial
	// coordinates here; this will make HW pick the next source pixel if
	// the fraction is equal or greater then 0.5.
	srcSubRect.left   += 0x00008000;
	srcSubRect.top    += 0x00008000;
	srcSubRect.right  += 0x00008000;
	srcSubRect.bottom += 0x00008000;

	////////////////////////////////////////////////////////////////////////////
	// Horizontal pass has to render more vertically so that vertical pass has
	// its necessary kernel information on the edges of the image.

	{
		UINT32 verKernelHalf = VerKernelSize >> 1;

		tmpTopExtra = srcSubRect.top >> 16;
		tmpBotExtra = Parameters->srcSize.y - (srcSubRect.bottom >> 16);

		if (tmpTopExtra > verKernelHalf)
			tmpTopExtra = verKernelHalf;

		if (tmpBotExtra > verKernelHalf)
			tmpBotExtra = verKernelHalf;
	}

	////////////////////////////////////////////////////////////////////////////
	// Determine the size of the temporary image.

	tmpWidth
		= TrgSubRect->right
		- TrgSubRect->left;

	tmpHeight
		= tmpTopExtra
		+ ((srcSubRect.bottom >> 16) - (srcSubRect.top >> 16))
		+ tmpBotExtra;

	////////////////////////////////////////////////////////////////////////////
	// Compute coordinates for the horizontal pass.

	// Determine the source image coordinates.
	Parameters->horSrcRect = *SrcRect;

	// Determine the source origin.
	Parameters->horSrcOrigin.x
		= (SrcRect->left << 16)
		+ srcSubRect.left;

	Parameters->horSrcOrigin.y
		= ((SrcRect->top - tmpTopExtra) << 16)
		+ srcSubRect.top;

	// Determine the temporary window coordinates.
	Parameters->horTrgRect.left   = (TrgRect->left + TrgSubRect->left) & tempHorCoordMask;
	Parameters->horTrgRect.top    = 0;
	Parameters->horTrgRect.right  = Parameters->horTrgRect.left + tmpWidth;
	Parameters->horTrgRect.bottom = Parameters->horTrgRect.top  + tmpHeight;

	////////////////////////////////////////////////////////////////////////////
	// Compute coordinates for the vertical pass.

	// Determine the source image coordinates.
	Parameters->verSrcRect = Parameters->horTrgRect;

	// Determine the source origin.
	Parameters->verSrcOrigin.x
		= (Parameters->verSrcRect.left << 16)
		+ (srcSubRect.left & 0xFFFF);

	Parameters->verSrcOrigin.y
		= ((Parameters->verSrcRect.top + tmpTopExtra) << 16)
		+ (srcSubRect.top  & 0xFFFF);

	// Compute final destination subrectangle.
	Parameters->verTrgRect.left   = TrgRect->left + TrgSubRect->left;
	Parameters->verTrgRect.top    = TrgRect->top  + TrgSubRect->top;
	Parameters->verTrgRect.right  = TrgRect->left + TrgSubRect->right;
	Parameters->verTrgRect.bottom = TrgRect->top  + TrgSubRect->bottom;

	////////////////////////////////////////////////////////////////////////////
	// Allocate temporary buffer.

	// Determine the size of the temporaty surface.
	Parameters->tempSurfaceWidth
		= (Parameters->horTrgRect.right + tempHorCoordMask) & ~tempHorCoordMask;

	Parameters->tempSurfaceHeight
		 = Parameters->horTrgRect.bottom;
}

void gcFilterBlit(
	gcSURFACEINFO* Target,
	gcSURFACEINFO* Source,
	gcRECT* TrgRect,
	gcRECT* SrcRect,
	gcRECT* TrgSubRect,
	UINT32 HorKernelSize,
	UINT32 VerKernelSize
	)
{
	gcFILTERPARAMETERS parameters;
	gcSURFACEINFO tempSurface;
	gcKERNELTABLE kernelTable;

    
	// Compute filter blit parameters.
	gcComputeFilterBlit(
		Target,
		Source,
		TrgRect,
		SrcRect,
		TrgSubRect,
		HorKernelSize,
		VerKernelSize,
		&parameters
		);

	// Allocate temporary buffer.
	gcAllocateSurface(
		&tempSurface,
		parameters.tempSurfaceWidth,
		parameters.tempSurfaceHeight,
		parameters.tempFormat
		);

	// Program stretch factors.
	gcSetStretchFactor(
		parameters.horFactor,
		parameters.verFactor
		);

	// Compute the kernel table.
	gcComputeKernelTable(
		parameters.srcSize.x,
		parameters.trgSize.x,
		HorKernelSize,
		&kernelTable
		);

	// Program the kernel table.
	gcProgramKernelTable(
		&kernelTable
		);

	// Horizontal pass.
	gcHorFilterBlit(
		&tempSurface, Source,
		&parameters.horTrgRect,
		&parameters.horSrcRect,
		&parameters.horSrcOrigin
		);

	// Compute the kernel table.
	gcComputeKernelTable(
		parameters.srcSize.y,
		parameters.trgSize.y,
		VerKernelSize,
		&kernelTable
		);

	// Program the kernel table.
	gcProgramKernelTable(
		&kernelTable
		);

	// Vertical pass.
	gcVerFilterBlit(
		Target, &tempSurface,
		&parameters.verTrgRect,
		&parameters.verSrcRect,
		&parameters.verSrcOrigin
		);


	// Start.
	gcFlush2DAndStall();
	gcStart();

	// Free the temporary buffer.
	gcMemFree();
}
