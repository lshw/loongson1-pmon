#ifndef _gcPRIMITIVES_H_
#define _gcPRIMITIVES_H_

UINT32 gcGetPixelSize(
	UINT32 Format
	);

void gcLoadState(
	UINT32 Address,
	UINT32 Count,
	...
	);

void gcLoadStatePtr(
	UINT32 Address,
	UINT32 Count,
	PUINT32 Values
	);

void gcFlush2DAndStall(
	void
	);

void gcSelect2DPipe(
	void
	);

void gcStartDE(
	UINT32 RectCount,
	gcRECT* Rect
	);

void gcStartMonoDE(
	gcIMAGEDESCRIPTOR* Data,
	gcRECT* Rect,
	UINT32 SrcPack
	);

void gcStart(
	void
	);

void gcSetROP4(
	UINT32 FgRop,
	UINT32 BgRop
	);

void gcSetSource(
	gcSURFACEINFO* Surface,
	gcRECT* Rect,
	UINT32 Transparency,
	UINT32 TransparencyColor,
	BOOL SrcRelative
	);

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
	);

void gcSetMaskedSource(
	gcSURFACEINFO* Surface,
	gcRECT* Rect,
	UINT32 MaskPack,
	BOOL SrcRelative
	);

void gcSetTarget(
	gcSURFACEINFO* Surface,
	UINT32 Command
	);

void gcClear(
	gcSURFACEINFO* Target,
	gcRECT* Rect,
	UINT32 Color	
	);

void gcLine(
	gcSURFACEINFO* Target,
	gcRECT* Rect,
	gcBRUSH* Brush
	);

void gcLineMoveTo(
	UINT32 X,
	UINT32 Y
	);

void gcLineTo(
	gcSURFACEINFO* Target,
	UINT32 X,
	UINT32 Y,
	gcBRUSH* Brush
	);

void gcRect(
	gcSURFACEINFO* Target,
	gcRECT* Rect,
	gcBRUSH* Brush
	);

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
	);

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
	);

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
	);


void gcBitBlt_SC(
	gcSURFACEINFO* Target,
	gcSURFACEINFO* Source,
	gcRECT* TrgRect,
	gcRECT* SrcRect
    );  //zgj

#endif // _gcPRIMITIVES_H_
