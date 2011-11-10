#include "gcSdk.h"

static void gcInitColorPattern(
	gcBRUSH* Brush
	)
{
	gcLoadState(AQDEPatternAddressRegAddrs, 1,
				Brush->colorBits);

	gcLoadState(AQDEPatternMaskLowRegAddrs, 2,

				// AQDEPatternMaskLow.
				(UINT32) (Brush->mask),

				// AQDEPatternMaskHigh.
				(UINT32) (Brush->mask >> 32));

	gcLoadState(AQDEPatternConfigRegAddrs, 1,
				SETFIELD(0, AQDE_PATTERN_CONFIG, FORMAT, Brush->format)
				| SETFIELD(0, AQDE_PATTERN_CONFIG, ORIGIN_X, Brush->originX)
				| SETFIELD(0, AQDE_PATTERN_CONFIG, ORIGIN_Y, Brush->originY)
				| SETFIELDVALUE(0, AQDE_PATTERN_CONFIG, TYPE, PATTERN)
				| SETFIELDVALUE(0, AQDE_PATTERN_CONFIG, INIT_TRIGGER, INIT_ALL));
}

static void gcInitMonoPattern(
	gcBRUSH* Brush
	)
{
	gcLoadState(AQDEPatternLowRegAddrs, 6,

				// AQDEPatternLow.
				(UINT32) (Brush->monoBits),

				// AQDEPatternHigh.
				(UINT32) (Brush->monoBits >> 32),

				// AQDEPatternMaskLow.
				(UINT32) (Brush->mask),

				// AQDEPatternMaskHigh.
				(UINT32) (Brush->mask >> 32),

				// AQDEPatternBgColor.
				Brush->bgColor,

				// AQDEPatternFgColor.
				Brush->fgColor);

	gcLoadState(AQDEPatternConfigRegAddrs, 1,
				SETFIELD(0, AQDE_PATTERN_CONFIG, COLOR_CONVERT, Brush->colorConvert)
				| SETFIELD(0, AQDE_PATTERN_CONFIG, ORIGIN_X, Brush->originX)
				| SETFIELD(0, AQDE_PATTERN_CONFIG, ORIGIN_Y, Brush->originY)
				| SETFIELDVALUE(0, AQDE_PATTERN_CONFIG, FORMAT, MONOCHROME)
				| SETFIELDVALUE(0, AQDE_PATTERN_CONFIG, TYPE, PATTERN)
				| SETFIELDVALUE(0, AQDE_PATTERN_CONFIG, INIT_TRIGGER, INIT_ALL));
}

static void gcInitSolidPattern(
	gcBRUSH* Brush
	)
{
	gcLoadState(AQDEPatternMaskLowRegAddrs, 4,

				// AQDEPatternMaskLow.
				(UINT32) (Brush->mask),

				// AQDEPatternMaskHigh.
				(UINT32) (Brush->mask >> 32),

				// AQDEPatternBgColor.
				Brush->bgColor,

				// AQDEPatternFgColor.
				Brush->fgColor);

	gcLoadState(AQDEPatternConfigRegAddrs, 1,
				SETFIELD(0, AQDE_PATTERN_CONFIG, COLOR_CONVERT, Brush->colorConvert)
				| SETFIELD(0, AQDE_PATTERN_CONFIG, FORMAT, Brush->format)
				| SETFIELDVALUE(0, AQDE_PATTERN_CONFIG, TYPE, SOLID_COLOR)
				| SETFIELDVALUE(0, AQDE_PATTERN_CONFIG, INIT_TRIGGER, INIT_ALL));
}

void gcSetBrush(
	gcBRUSH* Brush
	)
{
	// Hardware uses 2D PE cache for a brief period of time for pattern init.
	// Flush the cache here to preserve whatever results there might be left
	// after the previous blit.
	gcFlush2DAndStall();

	// Color brush?
	if (Brush->colorBits != ~0)
	{
		gcInitColorPattern(Brush);
	}

	// Mono brush.
	else if ((Brush->originX != ~0) && (Brush->originY != ~0))
	{
		gcInitMonoPattern(Brush);
	}

	// Solid color brush.
	else
	{
		gcInitSolidPattern(Brush);
	}
}

void gcConstructSingleColorBrush(
	gcBRUSH* Brush,
	UINT32 Color,
	BOOL ColorConvert,
	UINT64 Mask
	)
{
	Brush->format       = ~0;
	Brush->originX      = ~0;
	Brush->originY      = ~0;
	Brush->fgColor      = Color;
	Brush->bgColor      = Color;
	Brush->monoBits     = 0;
	Brush->colorBits    = ~0;
	Brush->mask         = Mask;
	Brush->colorConvert = ColorConvert
		? AQDE_PATTERN_CONFIG_COLOR_CONVERT_ON
		: AQDE_PATTERN_CONFIG_COLOR_CONVERT_OFF;
}

void gcConstructMonoBrush(
	gcBRUSH* Brush,
	UINT32 Bits,
	UINT32 FgColor,
	UINT32 BgColor,
	BOOL ColorConvert,
	UINT32 OriginX,
	UINT32 OriginY,
	UINT64 Mask
	)
{
	Brush->format       = ~0;
	Brush->originX      = OriginX;
	Brush->originY      = OriginY;
	Brush->fgColor      = FgColor;
	Brush->bgColor      = BgColor;
	Brush->monoBits     = Bits;
	Brush->colorBits    = ~0;
	Brush->mask         = Mask;
	Brush->colorConvert = ColorConvert
		? AQDE_PATTERN_CONFIG_COLOR_CONVERT_ON
		: AQDE_PATTERN_CONFIG_COLOR_CONVERT_OFF;
}

void gcConstructColorBrush(
	gcBRUSH* Brush,
	gcSURFACEINFO* Pattern,
	UINT32 OriginX,
	UINT32 OriginY,
	UINT64 Mask
	)
{
	Brush->format       = Pattern->format;
	Brush->originX      = OriginX;
	Brush->originY      = OriginY;
	Brush->fgColor      = ~0;
	Brush->bgColor      = ~0;
	Brush->monoBits     = ~0;
	Brush->colorBits    = Pattern->address;
	Brush->mask         = Mask;
	Brush->colorConvert = AQDE_PATTERN_CONFIG_COLOR_CONVERT_OFF;
}
