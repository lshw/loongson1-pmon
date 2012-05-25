#include <stdarg.h>
//#include <string.h>
#include "gcSdk.h"

#define Z_DEBUG_GPU_PRI 
//printf("%s - %s : %d\n",__FILE__,__FUNCTION__,__LINE__);
#undef MY_DEBUG_GPU_23
// #define MY_DEBUG_GPU_23

static UINT32 lineX;
static UINT32 lineY;

extern void apSleep(
	UINT32 msec
	);

UINT32 gcGetPixelSize(
	UINT32 Format
	)
{
Z_DEBUG_GPU_PRI
	switch (Format)
	{
	case AQ_DRAWING_ENGINE_FORMAT_FORMAT_MONOCHROME:
		return 1;

	case AQ_DRAWING_ENGINE_FORMAT_FORMAT_INDEX8:
		return 8;

	case AQ_DRAWING_ENGINE_FORMAT_FORMAT_X4R4G4B4:
	case AQ_DRAWING_ENGINE_FORMAT_FORMAT_A4R4G4B4:
	case AQ_DRAWING_ENGINE_FORMAT_FORMAT_X1R5G5B5:
	case AQ_DRAWING_ENGINE_FORMAT_FORMAT_A1R5G5B5:
	case AQ_DRAWING_ENGINE_FORMAT_FORMAT_R5G6B5:
	case AQ_DRAWING_ENGINE_FORMAT_FORMAT_YUY2:
	case AQ_DRAWING_ENGINE_FORMAT_FORMAT_UYVY:
		return 16;

	case AQ_DRAWING_ENGINE_FORMAT_FORMAT_X8R8G8B8:
	case AQ_DRAWING_ENGINE_FORMAT_FORMAT_A8R8G8B8:
		return 32;
	}

	return 0;
}

void gcLoadState(
	UINT32 Address,
	UINT32 Count,
	...
	)
{
	va_list argList;
	UINT32 cmdAddress;

Z_DEBUG_GPU_PRI
#ifdef MY_DEBUG_GPU_23
printf("%s : Address:%p, Count:0x%x  \n",__FUNCTION__,Address,Count);
#endif
Z_DEBUG_GPU_PRI
	// Init the argument list.
	va_start(argList, Count);

	// Allocate space in the buffer.
	cmdAddress = gcAllocateQueueSpace(1 + Count);

	// Construct load state command.
	gcPoke(cmdAddress,
		   SETFIELDVALUE(0, AQ_COMMAND_LOAD_STATE_COMMAND, OPCODE, LOAD_STATE)
		   | SETFIELD(0, AQ_COMMAND_LOAD_STATE_COMMAND, ADDRESS, Address)
		   | SETFIELD(0, AQ_COMMAND_LOAD_STATE_COMMAND, COUNT,   Count));
	cmdAddress += 4;

	// Copy the data.
	while (Count--)
	{
		UINT32 data = va_arg(argList, UINT32);
		gcPoke(cmdAddress, data);
		cmdAddress += 4;
	}
}

void gcLoadStatePtr(
	UINT32 Address,
	UINT32 Count,
	PUINT32 Values
	)
{
	// Allocate space in the buffer.
	UINT32 cmdAddress = gcAllocateQueueSpace(1 + Count);
Z_DEBUG_GPU_PRI
#ifdef MY_DEBUG_GPU_23
printf("%s : Address:%p, Count:0x%x ,Values:0x%x \n",__FUNCTION__,Address,Count,Values);
#endif
Z_DEBUG_GPU_PRI
	// Construct load state command.
	gcPoke(cmdAddress,
		   SETFIELDVALUE(0, AQ_COMMAND_LOAD_STATE_COMMAND, OPCODE, LOAD_STATE)
		   | SETFIELD(0, AQ_COMMAND_LOAD_STATE_COMMAND, ADDRESS, Address)
		   | SETFIELD(0, AQ_COMMAND_LOAD_STATE_COMMAND, COUNT,   Count));
	cmdAddress += 4;

	// Copy the data.
	while (Count--)
	{
		UINT32 data = *Values++;
		gcPoke(cmdAddress, data);
		cmdAddress += 4;
	}
}

void gcFlush2DAndStall(
	void
	)
{
	// Allocate space in the buffer.
	UINT32 cmdAddress = gcAllocateQueueSpace(6);
Z_DEBUG_GPU_PRI

	// Flush 2D.
	gcPoke(cmdAddress,
		   SETFIELDVALUE(0, AQ_COMMAND_LOAD_STATE_COMMAND, OPCODE, LOAD_STATE)
		   | SETFIELD(0, AQ_COMMAND_LOAD_STATE_COMMAND, ADDRESS, AQFlushRegAddrs)
		   | SETFIELD(0, AQ_COMMAND_LOAD_STATE_COMMAND, COUNT,   1));
	cmdAddress += 4;

	gcPoke(cmdAddress,
		   SETFIELDVALUE(0, AQ_FLUSH, PE2D_CACHE, ENABLE));
	cmdAddress += 4;

	// Semaphore.
	gcPoke(cmdAddress,
		   SETFIELDVALUE(0, AQ_COMMAND_LOAD_STATE_COMMAND, OPCODE, LOAD_STATE)
		   | SETFIELD(0, AQ_COMMAND_LOAD_STATE_COMMAND, ADDRESS, AQSemaphoreRegAddrs)
		   | SETFIELD(0, AQ_COMMAND_LOAD_STATE_COMMAND, COUNT,   1));
	cmdAddress += 4;

	gcPoke(cmdAddress,
		   SETFIELDVALUE(0, AQ_SEMAPHORE, SOURCE, FRONT_END) |
		   SETFIELDVALUE(0, AQ_SEMAPHORE, DESTINATION, PIXEL_ENGINE));
	cmdAddress += 4;

	// Stall.
	gcPoke(cmdAddress,
		   SETFIELDVALUE(0, STALL_COMMAND, OPCODE, STALL));
	cmdAddress += 4;

	gcPoke(cmdAddress,
		   SETFIELDVALUE(0, STALL, STALL_SOURCE, FRONT_END) |
		   SETFIELDVALUE(0, STALL, STALL_DESTINATION, PIXEL_ENGINE));
	cmdAddress += 4;
}

void gcSelect2DPipe(
	void
	)
{
Z_DEBUG_GPU_PRI
	gcLoadState(AQPipeSelectRegAddrs, 1,
				SETFIELDVALUE(0, AQ_PIPE_SELECT, PIPE, PIPE2D));
}

void gcStartDE(
	UINT32 RectCount,
	gcRECT* Rect
	)
{
	UINT32 i;

	// Allocate space in the buffer.
	UINT32 cmdAddress = gcAllocateQueueSpace(1 + RectCount * 2);
Z_DEBUG_GPU_PRI

	// Construct start DE command.
	gcPoke(cmdAddress,
		   SETFIELDVALUE(0, AQ_COMMAND_START_DE_COMMAND, OPCODE, START_DE)
		   | SETFIELD(0, AQ_COMMAND_START_DE_COMMAND, COUNT, RectCount)
		   | SETFIELD(0, AQ_COMMAND_START_DE_COMMAND, DATA_COUNT, 0));
	cmdAddress += 8;

	// Copy rectangles.
	for (i = 0; i < RectCount; i++)
	{
		gcPoke(cmdAddress,
			   SETFIELD(0, AQ_COMMAND_TOP_LEFT, X, Rect[i].left)
			   | SETFIELD(0, AQ_COMMAND_TOP_LEFT, Y, Rect[i].top));
		cmdAddress += 4;

		gcPoke(cmdAddress,
			   SETFIELD(0, AQ_COMMAND_BOTTOM_RIGHT, X, Rect[i].right)
			   | SETFIELD(0, AQ_COMMAND_BOTTOM_RIGHT, Y, Rect[i].bottom));
		cmdAddress += 4;
	}
}

void gcStartMonoDE(
	gcIMAGEDESCRIPTOR* Data,
	gcRECT* Rect,
	UINT32 SrcPack
	)
{
	UINT32 cmdAddress;
	UINT32 dataCount;
	UINT32 columnWidth;
	UINT32 columnHeight;
	UINT32 alignedWidth;
	UINT32 alignedHeight;
	UINT32 alignMaskX;
	UINT32 alignMaskY;

	// Get physical address of the source.
#if gcENABLEVIRTUAL
	PUINT32 dataAddress = (PUINT32) gcGetPhysicalAddress(Data->surface.address);
#else
	PUINT32 dataAddress = (PUINT32) Data->surface.address;
#endif

Z_DEBUG_GPU_PRI
	// Stream size.
	UINT32 streamWidth  = Data->surface.rect.right  - Data->surface.rect.left;
	UINT32 streamHeight = Data->surface.rect.bottom - Data->surface.rect.top;

	// Determine the column width in pixels and height in lines.
	switch (SrcPack)
	{
	case AQDE_SRC_CONFIG_PACK_PACKED8:
		columnWidth  = 8;
		columnHeight = 4;
		break;

	case AQDE_SRC_CONFIG_PACK_PACKED16:
		columnWidth  = 16;
		columnHeight = 2;
		break;

	case AQDE_SRC_CONFIG_PACK_PACKED32:
	case AQDE_SRC_CONFIG_PACK_UNPACKED:
		columnWidth  = 32;
		columnHeight = 1;
		break;

	default:
		printf("gcStartMonoDE: invalid source packing specified.\n");
		return;
	}

	// Determine the data count.
	alignMaskX = columnWidth  - 1;
	alignMaskY = columnHeight - 1;

	alignedWidth  = (streamWidth  + alignMaskX) & ~alignMaskX;
	alignedHeight = (streamHeight + alignMaskY) & ~alignMaskY;

	dataCount = alignedWidth * alignedHeight / 32;

	// Allocate space in the buffer.
	cmdAddress = gcAllocateQueueSpace(2 + 2 + dataCount);

	// Construct start DE command.
	gcPoke(cmdAddress,
		   SETFIELDVALUE(0, AQ_COMMAND_START_DE_COMMAND, OPCODE, START_DE)
		   | SETFIELD(0, AQ_COMMAND_START_DE_COMMAND, COUNT, 1)
		   | SETFIELD(0, AQ_COMMAND_START_DE_COMMAND, DATA_COUNT, dataCount));
	cmdAddress += 8;

	// Copy the rectangle.
	gcPoke(cmdAddress,
		   SETFIELD(0, AQ_COMMAND_TOP_LEFT, X, Rect->left)
		   | SETFIELD(0, AQ_COMMAND_TOP_LEFT, Y, Rect->top));
	cmdAddress += 4;

	gcPoke(cmdAddress,
		   SETFIELD(0, AQ_COMMAND_BOTTOM_RIGHT, X, Rect->right)
		   | SETFIELD(0, AQ_COMMAND_BOTTOM_RIGHT, Y, Rect->bottom));
	cmdAddress += 4;

	// Dispatch based on the packing.
	if (SrcPack == AQDE_SRC_CONFIG_PACK_UNPACKED)
	{
		UINT32 currLine;
		PUINT8 srcAddress = (PUINT8) dataAddress;
		UINT32 lineSize   = ((streamWidth + 31) & ~31) / 8;

		for (currLine = 0; currLine < streamHeight; currLine++)
		{
			// Copy one line.
			memcpy((PVOID)cmdAddress, srcAddress, lineSize);

			// Advance to the next.
			cmdAddress += lineSize;
			srcAddress += Data->surface.stride;
		}
	}
	else
	{
		UINT32 currLine;
		UINT32 currColumn;

		for (currColumn = 0; currColumn < streamWidth; currColumn += columnWidth)
		{
			for (currLine = 0; currLine < streamHeight; currLine += columnHeight)
			{
				UINT32 x, y;
				UINT32 streamData = 0;
				UINT32 inputBitBase = 31 - (currColumn & 31);

				for (y = 0; y < columnHeight; y++)
				{
					if (currLine + y == streamHeight)
					{
						break;
					}
					else
					{
						UINT32 outputBitBase = 31 - y * columnWidth;

						for (x = 0; x < columnWidth; x++)
						{
							if (currColumn + x == streamWidth)
							{
								break;
							}
							else
							{
								UINT32 srcOffset
									= (currLine + y) * Data->surface.stride
									+ ((currColumn + (x & ~31)) / 8);

								UINT32 data = dataAddress[srcOffset >> 2];

								UINT32 inputBit  = (inputBitBase  - x) ^ 0x18;
								UINT32 outputBit = (outputBitBase - x) ^ 0x18;

								UINT32 bit = GETBITS32(data, inputBit, inputBit);

								SETBITS32(&streamData, outputBit, outputBit, bit);
							}
						}
					}
				}

				gcPoke(cmdAddress, streamData);
				cmdAddress += 4;
			}
		}
	}
}

#undef Z_DEBUG_CMDBUF 
// #define Z_DEBUG_CMDBUF 1

#ifdef Z_DEBUG_CMDBUF
extern void gc_dump_cmdbuf(void);
#endif

void gcStart(
	void
	)
{
	int i;
	UINT32 idle;
    UINT32 my_gc_cmdbufaddr=gcCMDBUFADDR;

	// Append END command.
	gcAppendEnd();

	// Start execution.
    if(my_gc_cmdbufaddr & 0x80000000)  //zgj
        my_gc_cmdbufaddr &= 0x0FFFFFFF;
	gcWriteReg(AQCmdBufferAddrRegAddrs, my_gc_cmdbufaddr);
	//gcWriteReg(AQCmdBufferAddrRegAddrs, gcCMDBUFADDR);

#ifdef Z_DEBUG_CMDBUF
gc_dump_cmdbuf();
#endif

	gcWriteReg(AQCmdBufferCtrlRegAddrs, 0xFFFFFFFF);

	// Wait for idle.
	//zgj-2010-3-24 for ( i = 0; i < 5000; i++)
	for ( i = 0; i < 500000; i++)
	{
		idle = gcReportIdle(NULL);
        #if 0
        if(idle != 0x000000FF)
            printf("%d:0x%x \n",i,idle);
        #endif
		//zgj if (!(idle ^ 0x7FFFFFFF)) break;
		if (!(idle ^ 0x000000FF)) break;
		apSleep(1);
	}
	
	//zgj if (idle ^ 0x3FFFFFFF)
	if (idle ^ 0x000000FF)
	{
		printf("gcStart: chip has not become idle: 0x%08X\n", idle);
	}

	// Reset the buffer.
	gcCMDBUFCURRADDR = gcCMDBUFADDR;
	gcCMDBUFCURRSIZE = gcCMDBUFSIZE;
}

void gcSetROP4(
	UINT32 FgRop,
	UINT32 BgRop
	)
{
	gcLoadState(AQDERopRegAddrs, 1,
				SETFIELDVALUE(0, AQDE_ROP, TYPE, ROP4)
				| SETFIELD(0, AQDE_ROP, ROP_FG, FgRop)
				| SETFIELD(0, AQDE_ROP, ROP_BG, BgRop));
}

void gcSetSource(
	gcSURFACEINFO* Surface,
	gcRECT* Rect,
	UINT32 Transparency,
	UINT32 TransparencyColor,
	BOOL SrcRelative
	)
{
Z_DEBUG_GPU_PRI
    Surface->address &= 0x0FFFFFFF;  //zgj DMA
	gcLoadState(AQDESrcAddressRegAddrs, 6,

				// AQDESrcAddress.
				Surface->address,

				// AQDESrcStride.
				Surface->stride,

				// AQDESrcRotationConfig.
				SETFIELDVALUE(0, AQDE_SRC_ROTATION_CONFIG, ROTATION, NORMAL),

				// AQDESrcConfig.
				SETFIELDVALUE(0, AQDE_SRC_CONFIG, LOCATION, MEMORY)
				| SETFIELD(0, AQDE_SRC_CONFIG, FORMAT, Surface->format)
				| SETFIELD(0, AQDE_SRC_CONFIG, SRC_RELATIVE, SrcRelative)
				| SETFIELD(0, AQDE_SRC_CONFIG, TRANSPARENCY, Transparency),

				// AQDESrcOrigin.
				SETFIELD(0, AQDE_SRC_ORIGIN, X, Rect->left)
				| SETFIELD(0, AQDE_SRC_ORIGIN, Y, Rect->top),

				// AQDESrcSize.
				SETFIELD(0, AQDE_SRC_SIZE, X, Rect->right - Rect->left)
				| SETFIELD(0, AQDE_SRC_SIZE, Y, Rect->bottom - Rect->top));

	gcLoadState(AQDESrcColorBgRegAddrs, 1,
				TransparencyColor);
}

void gcSetMonoSource(
	gcSURFACEINFO* Surface,
	gcRECT* Rect,
	UINT32 FgColor,
	UINT32 BgColor,
	UINT32 MonoPack,
	UINT32 ColorConvert,
	UINT32 Transparency,
	UINT32 MonoTransparency,
	BOOL SrcRelative
	)
{
Z_DEBUG_GPU_PRI
	gcLoadState(AQDESrcConfigRegAddrs, 3,

				// AQDESrcConfig.
				SETFIELDVALUE(0, AQDE_SRC_CONFIG, LOCATION, STREAM)
				| SETFIELDVALUE(0, AQDE_SRC_CONFIG, FORMAT, MONOCHROME)
				| SETFIELD(0, AQDE_SRC_CONFIG, PACK, MonoPack)
				| SETFIELD(0, AQDE_SRC_CONFIG, COLOR_CONVERT, ColorConvert)
				| SETFIELD(0, AQDE_SRC_CONFIG, MONO_TRANSPARENCY, MonoTransparency)
				| SETFIELD(0, AQDE_SRC_CONFIG, SRC_RELATIVE, SrcRelative)
				| SETFIELD(0, AQDE_SRC_CONFIG, TRANSPARENCY, Transparency),

				// AQDESrcOrigin.
				SETFIELD(0, AQDE_SRC_ORIGIN, X, Rect->left)
				| SETFIELD(0, AQDE_SRC_ORIGIN, Y, Rect->top),

				// AQDESrcSize.
				SETFIELD(0, AQDE_SRC_SIZE, X, Rect->right - Rect->left)
				| SETFIELD(0, AQDE_SRC_SIZE, Y, Rect->bottom - Rect->top));

	gcLoadState(AQDESrcColorBgRegAddrs, 2,

				// AQDESrcColorBg.
				BgColor,

				// AQDESrcColorFg.
				FgColor);
}

void gcSetMaskedSource(
	gcSURFACEINFO* Surface,
	gcRECT* Rect,
	UINT32 MaskPack,
	BOOL SrcRelative
	)
{
Z_DEBUG_GPU_PRI
    Surface->address &= 0x0FFFFFFF;  //zgj DMA
	gcLoadState(AQDESrcAddressRegAddrs, 6,

				// AQDESrcAddress.
				Surface->address,

				// AQDESrcStride.
				Surface->stride,

				// AQDESrcRotationConfig.
				SETFIELDVALUE(0, AQDE_SRC_ROTATION_CONFIG, ROTATION, NORMAL),

				// AQDESrcConfig.
				SETFIELDVALUE(0, AQDE_SRC_CONFIG, LOCATION, STREAM)
				| SETFIELDVALUE(0, AQDE_SRC_CONFIG, TRANSPARENCY, MASKED_MASK)
				| SETFIELD(0, AQDE_SRC_CONFIG, FORMAT, Surface->format)
				| SETFIELD(0, AQDE_SRC_CONFIG, PACK, MaskPack)
				| SETFIELD(0, AQDE_SRC_CONFIG, SRC_RELATIVE, SrcRelative),

				// AQDESrcOrigin.
				SETFIELD(0, AQDE_SRC_ORIGIN, X, Rect->left)
				| SETFIELD(0, AQDE_SRC_ORIGIN, Y, Rect->top),

				// AQDESrcSize.
				SETFIELD(0, AQDE_SRC_SIZE, X, Rect->right - Rect->left)
				| SETFIELD(0, AQDE_SRC_SIZE, Y, Rect->bottom - Rect->top));
}

void gcSetTarget(
	gcSURFACEINFO* Surface,
	UINT32 Command
	)
{
Z_DEBUG_GPU_PRI
    Surface->address &= 0x0FFFFFFF;  //zgj DMA
	gcLoadState(AQDEDestAddressRegAddrs, 4,

				// AQDEDestAddress.
				Surface->address,

				// AQDEDestStride.
				Surface->stride,

				// AQDEDestRotationConfig.
				SETFIELDVALUE(0, AQDE_DEST_ROTATION_CONFIG, ROTATION, NORMAL),

				// AQDEDestConfig.
				SETFIELD(0, AQDE_DEST_CONFIG, COMMAND, Command)
				| SETFIELD(0, AQDE_DEST_CONFIG, FORMAT, Surface->format));

	gcLoadState(AQDEClipTopLeftRegAddrs, 2,

				// AQDEClipTopLeft.
				SETFIELD(0, AQDE_CLIP_TOP_LEFT, X, Surface->clip.left)
				| SETFIELD(0, AQDE_CLIP_TOP_LEFT, Y, Surface->clip.top),

				// AQDEClipBottomRight.
				SETFIELD(0, AQDE_CLIP_BOTTOM_RIGHT, X, Surface->clip.right)
				| SETFIELD(0, AQDE_CLIP_BOTTOM_RIGHT, Y, Surface->clip.bottom));
}

void gcClear(
	gcSURFACEINFO* Target,
	gcRECT* Rect,
	UINT32 Color	
	)
{
Z_DEBUG_GPU_PRI
	// Set to full target if no rect provided.
	if (Rect == NULL)
	{
		Rect = &Target->rect;
	}

Z_DEBUG_GPU_PRI
	// Set destination.
	gcSetTarget(Target, AQDE_DEST_CONFIG_COMMAND_CLEAR);

Z_DEBUG_GPU_PRI
	// Set ROP.
	gcSetROP4(0x00, 0x00);

Z_DEBUG_GPU_PRI
	// Set clear parameters.
	gcLoadState(AQDEClearByteMaskRegAddrs, 1,
				0xFF);

Z_DEBUG_GPU_PRI
	gcLoadState(AQDEClearPixelValueLowRegAddrs, 2,

				// AQDEClearPixelValueLow.
				Color,

				// AQDEClearPixelValueHigh.
				Color);

	// Start.
Z_DEBUG_GPU_PRI
	gcStartDE(1, Rect);
}

void gcLine(
	gcSURFACEINFO* Target,
	gcRECT* Rect,
	gcBRUSH* Brush
	)
{
Z_DEBUG_GPU_PRI
	// Set destination.
	gcSetTarget(Target, AQDE_DEST_CONFIG_COMMAND_LINE);

Z_DEBUG_GPU_PRI
	// Init pattern.
	gcSetBrush(Brush);

Z_DEBUG_GPU_PRI
	// Pattern copy.
	gcSetROP4(0xF0, 0xF0);

Z_DEBUG_GPU_PRI
	// Start.
	gcStartDE(1, Rect);
Z_DEBUG_GPU_PRI
}

void gcLineMoveTo(
	UINT32 X,
	UINT32 Y
	)
{
Z_DEBUG_GPU_PRI
	lineX = X;
	lineY = Y;
}

void gcLineTo(
	gcSURFACEINFO* Target,
	UINT32 X,
	UINT32 Y,
	gcBRUSH* Brush
	)
{
Z_DEBUG_GPU_PRI
	// Construct the rectangle.
	gcRECT lineRect;
	lineRect.left   = lineX;
	lineRect.top    = lineY;
	lineRect.right  = X;
	lineRect.bottom = Y;

	// Draw the line.
	gcLine(Target, &lineRect, Brush);

	// Update the origin.
	gcLineMoveTo(X, Y);
}

void gcRect(
	gcSURFACEINFO* Target,
	gcRECT* Rect,
	gcBRUSH* Brush
	)
{
	gcRECT lineRect;

Z_DEBUG_GPU_PRI
	// Draw top horizontal line.
	lineRect.left   = Rect->left;
	lineRect.top    = Rect->top;
	lineRect.right  = Rect->right;
	lineRect.bottom = Rect->top;
	gcLine(Target, &lineRect, Brush);

	// Draw right vertical line.
	lineRect.left   = Rect->right - 1;
	lineRect.top    = Rect->top   + 1;
	lineRect.right  = Rect->right - 1;
	lineRect.bottom = Rect->bottom;
	gcLine(Target, &lineRect, Brush);

	// Draw bottom horizontal line.
	lineRect.left   = Rect->right  - 1;
	lineRect.top    = Rect->bottom - 1;
	lineRect.right  = Rect->left;
	lineRect.bottom = Rect->bottom - 1;
	gcLine(Target, &lineRect, Brush);

	// Draw left vertical line.
	lineRect.left   = Rect->left;
	lineRect.top    = Rect->bottom - 1;
	lineRect.right  = Rect->left;
	lineRect.bottom = Rect->top;
	gcLine(Target, &lineRect, Brush);
}

#if 1
void gcBitBlt_SC(
	gcSURFACEINFO* Target,
	gcSURFACEINFO* Source,
	gcRECT* TrgRect,
	gcRECT* SrcRect
	)
{

	gcSetSource(Source, SrcRect, AQDE_SRC_CONFIG_TRANSPARENCY_OPAQUE,~0, gcFALSE);
	// Setup ROP.
	gcSetROP4(0xCC,0xCC);

	// Program target registers.
	gcSetTarget(Target, AQDE_DEST_CONFIG_COMMAND_BIT_BLT);

	gcStartDE(1, TrgRect);
}
#endif

void gcBitBlt(
	gcSURFACEINFO* Target,
	gcSURFACEINFO* Source,
	gcRECT* TrgRect,
	gcRECT* SrcRect,
	UINT32 FgRop,
	UINT32 BgRop,
	gcBRUSH* Brush,
	UINT32 Transparency,
	UINT32 TransparencyColor,
	gcIMAGEDESCRIPTOR* Mask,
	UINT32 MaskPack,
	BOOL SrcRelative
	)
{
	// Assume regular blit.
	UINT32 bltCommand = AQDE_DEST_CONFIG_COMMAND_BIT_BLT;

	// Determine whether this is target only operation.
	BOOL targetOnly
		=  ((FgRop == 0x00) && (BgRop == 0x00))
		|| ((FgRop == 0x55) && (BgRop == 0x55))
		|| ((FgRop == 0xAA) && (BgRop == 0xAA))
		|| ((FgRop == 0xFF) && (BgRop == 0xFF));

	// Determine whether we need the brush for the operation.
	BOOL useBrush
		=  (((FgRop >> 4) & 0x0F) != (FgRop & 0x0F))
		|| (((BgRop >> 4) & 0x0F) != (BgRop & 0x0F));

	// Determine whether we need the source for the operation.
	BOOL useSource
		= (targetOnly == 0)
		&& ((((FgRop >> 2) & 0x33) != (FgRop & 0x33))
		||  (((BgRop >> 2) & 0x33) != (BgRop & 0x33)));

Z_DEBUG_GPU_PRI
	// Set to full target if no rect provided.
	if (TrgRect == NULL)
	{
		TrgRect = &Target->rect;
	}

	// Program source.
	if (useSource)
	{
		// Set to full source if no rect provided.
		if (SrcRect == NULL)
		{
			SrcRect = &Source->rect;
		}

		// Program color source without a mask.
		if (Mask == NULL)
		{
			// Compute rectangle sizes.
			UINT32 srcWidth  = SrcRect->right  - SrcRect->left;
			UINT32 srcHeight = SrcRect->bottom - SrcRect->top;
			UINT32 trgWidth  = TrgRect->right  - TrgRect->left;
			UINT32 trgHeight = TrgRect->bottom - TrgRect->top;

			// Check whether this is a stretch/shrink blit.
			if ((SrcRelative == 0) &&
				((srcWidth != trgWidth) || (srcHeight != trgHeight)))
			{
				// Compute stretch factors.
				UINT32 horFactor = gcGetStretchFactor(srcWidth,  trgWidth);
				UINT32 verFactor = gcGetStretchFactor(srcHeight, trgHeight);

				// Program stretch factors.
				gcSetStretchFactor(horFactor, verFactor);

				// Set blit command to stretch.
				bltCommand = AQDE_DEST_CONFIG_COMMAND_STRETCH_BLT;
			}

			// Program source registers.
			gcSetSource(Source, SrcRect, Transparency, TransparencyColor, SrcRelative);
		}

		// Program color source with a mask.
		else
		{
			// Program source registers.
			gcSetMaskedSource(Source, SrcRect, MaskPack, SrcRelative);
		}
	}

	// bltCommand = AQDE_DEST_CONFIG_COMMAND_BIT_BLT; //zgj
Z_DEBUG_GPU_PRI
	// Setup ROP.
	gcSetROP4(FgRop, BgRop);

Z_DEBUG_GPU_PRI
	// Program target registers.
	gcSetTarget(Target, bltCommand);

	// Program brush.
	if (useBrush)
	{
Z_DEBUG_GPU_PRI
		gcSetBrush(Brush);
	}

	// Start.
	if (Mask)
	{
Z_DEBUG_GPU_PRI
		gcStartMonoDE(Mask, TrgRect, MaskPack);
	}
	else
	{
Z_DEBUG_GPU_PRI
		gcStartDE(1, TrgRect);
	}
}

void gcMonoBitBlt(
	gcSURFACEINFO* Target,
	gcIMAGEDESCRIPTOR* Source,
	gcPOINT* TrgPoint,
	UINT32 FgColor,
	UINT32 BgColor,
	UINT32 FgRop,
	UINT32 BgRop,
	UINT32 SrcPack,
	gcBRUSH* Brush,
	BOOL ColorConvert,
	UINT32 Transparency,
	UINT32 MonoTransparency
	)
{
	gcRECT srcRect;
	gcRECT trgRect;

	// Determine whether this is target only operation.
	BOOL targetOnly
		=  ((FgRop == 0x00) && (BgRop == 0x00))
		|| ((FgRop == 0x55) && (BgRop == 0x55))
		|| ((FgRop == 0xAA) && (BgRop == 0xAA))
		|| ((FgRop == 0xFF) && (BgRop == 0xFF));

	// Determine whether we need the brush for the operation.
	BOOL useBrush
		=  (((FgRop >> 4) & 0x0F) != (FgRop & 0x0F))
		|| (((BgRop >> 4) & 0x0F) != (BgRop & 0x0F));

	// Determine whether we need the source for the operation.
	BOOL useSource
		= (targetOnly == 0)
		&& ((((FgRop >> 2) & 0x33) != (FgRop & 0x33))
		||  (((BgRop >> 2) & 0x33) != (BgRop & 0x33)));

Z_DEBUG_GPU_PRI
	// Make sure we have source.
	if (!useSource)
	{
		printf("gcMonoBitBlt: source must be present.\n");
		return;
	}

	// Construct source rectangle.
	srcRect.left   = 0;
	srcRect.top    = 0;
	srcRect.right  = 0;			// Right and bottom are ignored for mono blit.
	srcRect.bottom = 0;

	// Construct target rectangle.
	trgRect.left   = TrgPoint->x;
	trgRect.top    = TrgPoint->y;
	trgRect.right  = trgRect.left + gcImageWidth(Source);
	trgRect.bottom = trgRect.top  + gcImageHeight(Source);

	// Program source registers.
	gcSetMonoSource(&Source->surface, &srcRect, FgColor, BgColor, SrcPack,
					ColorConvert, Transparency, MonoTransparency, gcFALSE);

	// Setup ROP.
	gcSetROP4(FgRop, BgRop);

Z_DEBUG_GPU_PRI
	// Program target registers.
	gcSetTarget(Target, AQDE_DEST_CONFIG_COMMAND_BIT_BLT);

Z_DEBUG_GPU_PRI
	// Program brush.
	if (useBrush)
	{
Z_DEBUG_GPU_PRI
		gcSetBrush(Brush);
	}

Z_DEBUG_GPU_PRI
	// Start.
	gcStartMonoDE(Source, &trgRect, SrcPack);
}

void gcSetSourceRot(
	gcSURFACEINFO* Surface,
	gcRECT* Rect,
	UINT32 Transparency,
	UINT32 TransparencyColor,
	BOOL SrcRelative,
	UINT32 rotAngle,
	UINT32 imgWidth,
	UINT32 imgHeight
	)
{
Z_DEBUG_GPU_PRI
    Surface->address &= 0x0FFFFFFF;  //zgj DMA
	gcLoadState(AQDESrcAddressRegAddrs, 6,

				// AQDESrcAddress.
				Surface->address,

				// AQDESrcStride.
				Surface->stride,

				// AQDESrcRotationConfig.
				SETFIELDVALUE(0, AQDE_SRC_ROTATION_CONFIG, ROTATION, NORMAL),

				// AQDESrcConfig.
				SETFIELDVALUE(0, AQDE_SRC_CONFIG, LOCATION, MEMORY)
				| SETFIELD(0, AQDE_SRC_CONFIG, FORMAT, Surface->format)
				| SETFIELD(0, AQDE_SRC_CONFIG, SRC_RELATIVE, SrcRelative)
				| SETFIELD(0, AQDE_SRC_CONFIG, TRANSPARENCY, Transparency),

				// AQDESrcOrigin.
				SETFIELD(0, AQDE_SRC_ORIGIN, X, Rect->left)
				| SETFIELD(0, AQDE_SRC_ORIGIN, Y, Rect->top),

				// AQDESrcSize.
				SETFIELD(0, AQDE_SRC_SIZE, X, Rect->right - Rect->left)
				| SETFIELD(0, AQDE_SRC_SIZE, Y, Rect->bottom - Rect->top));

Z_DEBUG_GPU_PRI
	gcLoadState(AQDESrcColorBgRegAddrs, 1,
				TransparencyColor);

	//set source width;
	gcLoadState(AQDESrcRotationConfigRegAddrs, 1,
				SETFIELD(0, AQDE_SRC_ROTATION_CONFIG, WIDTH, imgWidth));

	//set source height;
	gcLoadState(AQDESrcRotationHeightRegAddrs, 1,
				imgHeight);

	//set rotation angle;
	gcLoadState(AQDERotAngleRegAddrs, 1,
				rotAngle);



}

void gcSetTargetRot(
	gcSURFACEINFO* Surface,
	UINT32 Command,
	UINT32 rotAngle,
	UINT32 trgWidth,
	UINT32 trgHeight
	)
{
Z_DEBUG_GPU_PRI
    Surface->address &= 0x0FFFFFFF;  //zgj DMA
	gcLoadState(AQDEDestAddressRegAddrs, 4,

				// AQDEDestAddress.
				Surface->address,

				// AQDEDestStride.
				Surface->stride,

				// AQDEDestRotationConfig.
				SETFIELDVALUE(0, AQDE_DEST_ROTATION_CONFIG, ROTATION, NORMAL),

				// AQDEDestConfig.
				SETFIELD(0, AQDE_DEST_CONFIG, COMMAND, Command)
				| SETFIELD(0, AQDE_DEST_CONFIG, FORMAT, Surface->format));

	gcLoadState(AQDEClipTopLeftRegAddrs, 2,

				// AQDEClipTopLeft.
				SETFIELD(0, AQDE_CLIP_TOP_LEFT, X, Surface->clip.left)
				| SETFIELD(0, AQDE_CLIP_TOP_LEFT, Y, Surface->clip.top),

				// AQDEClipBottomRight.
				SETFIELD(0, AQDE_CLIP_BOTTOM_RIGHT, X, Surface->clip.right)
				| SETFIELD(0, AQDE_CLIP_BOTTOM_RIGHT, Y, Surface->clip.bottom));

Z_DEBUG_GPU_PRI
	//set source width;
	gcLoadState(AQDEDestRotationConfigRegAddrs, 1,
				SETFIELD(0, AQDE_DEST_ROTATION_CONFIG, WIDTH, trgWidth));

Z_DEBUG_GPU_PRI
	//set source height;
	gcLoadState(AQDEDstRotationHeightRegAddrs, 1,
				trgHeight);

Z_DEBUG_GPU_PRI
	//set rotation angle;
	gcLoadState(AQDERotAngleRegAddrs, 1,
				rotAngle);

}

void gcBitBltRot(
	gcSURFACEINFO* Target,
	gcSURFACEINFO* Source,
	gcRECT* TrgRect,
	gcRECT* SrcRect,
	UINT32 FgRop,
	UINT32 BgRop,
	gcBRUSH* Brush,
	UINT32 Transparency,
	UINT32 TransparencyColor,
	gcIMAGEDESCRIPTOR* Mask,
	UINT32 MaskPack,
	BOOL SrcRelative,
	UINT32 srcRotAngle,
	UINT32 dstRotAngle,
	UINT32 imgWidth,
	UINT32 imgHeight,
	UINT32 trgWidth,
	UINT32 trgHeight

	)
{
	// Assume regular blit.
	UINT32 bltCommand = AQDE_DEST_CONFIG_COMMAND_BIT_BLT;

	// Determine whether this is target only operation.
	BOOL targetOnly
		=  ((FgRop == 0x00) && (BgRop == 0x00))
		|| ((FgRop == 0x55) && (BgRop == 0x55))
		|| ((FgRop == 0xAA) && (BgRop == 0xAA))
		|| ((FgRop == 0xFF) && (BgRop == 0xFF));

	// Determine whether we need the brush for the operation.
	BOOL useBrush
		=  (((FgRop >> 4) & 0x0F) != (FgRop & 0x0F))
		|| (((BgRop >> 4) & 0x0F) != (BgRop & 0x0F));

	// Determine whether we need the source for the operation.
	BOOL useSource
		= (targetOnly == 0)
		&& ((((FgRop >> 2) & 0x33) != (FgRop & 0x33))
		||  (((BgRop >> 2) & 0x33) != (BgRop & 0x33)));

	// Set to full target if no rect provided.
	if (TrgRect == NULL)
	{
		TrgRect = &Target->rect;
	}

	// Program source.
	if (useSource)
	{
		// Set to full source if no rect provided.
		if (SrcRect == NULL)
		{
			SrcRect = &Source->rect;
		}

		// Program color source without a mask.
		if (Mask == NULL)
		{
			// Compute rectangle sizes.
			UINT32 srcWidth  = SrcRect->right  - SrcRect->left;
			UINT32 srcHeight = SrcRect->bottom - SrcRect->top;
			UINT32 trgWidth  = TrgRect->right  - TrgRect->left;
			UINT32 trgHeight = TrgRect->bottom - TrgRect->top;

Z_DEBUG_GPU_PRI
			// Check whether this is a stretch/shrink blit.
			if ((SrcRelative == 0) &&
				((srcWidth != trgWidth) || (srcHeight != trgHeight)))
			{
				// Compute stretch factors.
				UINT32 horFactor = gcGetStretchFactor(srcWidth,  trgWidth);
				UINT32 verFactor = gcGetStretchFactor(srcHeight, trgHeight);

				// Program stretch factors.
				gcSetStretchFactor(horFactor, verFactor);

				// Set blit command to stretch.
				bltCommand = AQDE_DEST_CONFIG_COMMAND_STRETCH_BLT;
			}

			// Program source registers.
			gcSetSourceRot(Source, SrcRect, Transparency, TransparencyColor, SrcRelative,srcRotAngle,imgWidth,imgHeight);
		}

		// Program color source with a mask.
		else
		{
			// Program source registers.
			gcSetMaskedSource(Source, SrcRect, MaskPack, SrcRelative);
		}
	}

	// Setup ROP.
	gcSetROP4(FgRop, BgRop);

Z_DEBUG_GPU_PRI
	// Program target registers.
	gcSetTargetRot(Target, bltCommand,dstRotAngle,trgWidth,trgHeight);

	// Program brush.
	if (useBrush)
	{
Z_DEBUG_GPU_PRI
		gcSetBrush(Brush);
	}

Z_DEBUG_GPU_PRI
	// Start.
	if (Mask)
	{
Z_DEBUG_GPU_PRI
		//gcStartMonoDE(Mask, TrgRect, MaskPack);
		gcStartDE(1, TrgRect);
	}
	else
	{
Z_DEBUG_GPU_PRI
		gcStartDE(1, TrgRect);
	}
}


