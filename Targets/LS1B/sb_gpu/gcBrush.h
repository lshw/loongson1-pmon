#ifndef _gcBRUSH_H_
#define _gcBRUSH_H_

typedef struct
{
	UINT32 format;
	UINT32 originX;
	UINT32 originY;
	BOOL colorConvert;
	UINT32 fgColor;
	UINT32 bgColor;
	UINT64 monoBits;
	UINT32 colorBits;
	UINT64 mask;
}
gcBRUSH;

void gcSetBrush(
	gcBRUSH* Brush
	);

void gcConstructSingleColorBrush(
	gcBRUSH* Brush,
	UINT32 Color,
	BOOL ColorConvert,
	UINT64 Mask
	);

void gcConstructMonoBrush(
	gcBRUSH* Brush,
	UINT32 Bits,
	UINT32 FgColor,
	UINT32 BgColor,
	BOOL ColorConvert,
	UINT32 OriginX,
	UINT32 OriginY,
	UINT64 Mask
	);

void gcConstructColorBrush(
	gcBRUSH* Brush,
	gcSURFACEINFO* Pattern,
	UINT32 OriginX,
	UINT32 OriginY,
	UINT64 Mask
	);

#endif // _gcBRUSH_H_
