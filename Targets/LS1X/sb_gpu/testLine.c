#include "gcSdk.h"

#define ONETHIRD	(RAND_MAX / 3)
#define TWOTHIRDS	(ONETHIRD * 2)

#define Z_DEBUG_TESTLINE 
//printf("++++++++++++%s : %d \n",__FUNCTION__,__LINE__);

void testLine(
	gcSURFACEINFO* Target
	)
{
	UINT32 loopCount = 10; //50;

	gcRECT vLine, hLine;
	UINT32 color = ~0;
	gcBRUSH brush;
	UINT32 x, xRange;
	UINT32 y, yRange;
	UINT32 currXrange;
	UINT32 xDelta;

	// Determine the size of the window.
Z_DEBUG_TESTLINE
	UINT32 trgHeight = gcSurfaceHeight(Target);

Z_DEBUG_TESTLINE
	// Make sure the count is valid.
	if (loopCount == 0)
	{
		return;
	}

	// Clear the frame.
Z_DEBUG_TESTLINE
	//gcClear(Target, NULL, DEADBEEF16);
	gcClear(Target, NULL, BLACK32);

	// Draw coordinate system.
Z_DEBUG_TESTLINE
	gcConstructSingleColorBrush(&brush, ~0, gcFALSE, ~0);
Z_DEBUG_TESTLINE

	vLine.left   = Target->rect.left   + 10;
	vLine.top    = Target->rect.top    + 10;
	vLine.right  = Target->rect.left   + 10;
	vLine.bottom = Target->rect.bottom - 10;
	gcLine(Target, &vLine, &brush);

	hLine.left   = vLine.left;
	hLine.top    = Target->rect.top   + 10 + trgHeight / 2;
	hLine.right  = Target->rect.right - 10;
	hLine.bottom = hLine.top;
	gcLine(Target, &hLine, &brush);

	// Set initial point.
	x = hLine.left;
	y = hLine.top;
Z_DEBUG_TESTLINE
	gcLineMoveTo(x, y);
Z_DEBUG_TESTLINE

	// Determine random ranges.
	currXrange = xRange = (hLine.right  - hLine.left - 10) / loopCount;
	yRange = (vLine.bottom - vLine.top) / 2 - 10;

//printf("x:%d , y:%d , xRange: %d ,yRange: %d\n",x,y,xRange,yRange);

	while (loopCount--)
	{
		// Construct brush.
Z_DEBUG_TESTLINE
		color = gcGetNextColor16(color);
Z_DEBUG_TESTLINE
		gcConstructSingleColorBrush(&brush, color, gcFALSE, ~0);
Z_DEBUG_TESTLINE

		// Generate next coordinates.
		xDelta = gcRand(currXrange - 2) + 2;
Z_DEBUG_TESTLINE
		x += xDelta;

		if ((loopCount & 1) == 0)
		{
			y = hLine.top - gcRand(yRange);
		}
		else
		{
			y = hLine.top + gcRand(yRange);
		}

		// Draw the next line.
Z_DEBUG_TESTLINE
		gcLineTo(Target, x, y, &brush);
Z_DEBUG_TESTLINE

		// Update x range.
		currXrange += xRange - xDelta;

		// Start.
		gcFlush2DAndStall();
Z_DEBUG_TESTLINE
		gcStart();
Z_DEBUG_TESTLINE
		gcFlushDisplay();
Z_DEBUG_TESTLINE
	}
}

void testRandomLine(
	gcSURFACEINFO* Target
	)
{
	UINT32 loopCount = 10; // 100;

	gcRECT rect;
	UINT32 color = ~0;
	gcBRUSH brush;

	// Clear the frame.
	// gcClear(Target, NULL, DEADBEEF16);
	gcClear(Target, NULL, BLACK32);

//    printf("right:%d ,bottom:%d \n",Target->rect.right,Target->rect.bottom);
#if 0
    if((Target->rect.right-1) < 1)
        Target->rect.right = 640 ;
    if((Target->rect.bottom-1) < 1)
        Target->rect.bottom = 480;
    printf("After- right:%d ,bottom:%d \n",Target->rect.right,Target->rect.bottom);
#endif
	while (loopCount--)
	{
		// Generate the "path".
		UINT32 path = rand();

#if 0
		if (path < ONETHIRD)
		{
			// Generate horizontal line.
Z_DEBUG_TESTLINE
			rect.left   = gcRand(Target->rect.right - 1);
			rect.right  = gcRand(Target->rect.right - 1);
			rect.top    =
			rect.bottom = gcRand(Target->rect.bottom - 1);
		}
		else if (path < TWOTHIRDS)
		{
			// Generate vertical line.
Z_DEBUG_TESTLINE
			rect.left   =
			rect.right  = gcRand(Target->rect.right - 1);
			rect.top    = gcRand(Target->rect.bottom - 1);
			rect.bottom = gcRand(Target->rect.bottom - 1);
		}
		else
#endif
		{
			// Generate random line.
Z_DEBUG_TESTLINE
			rect.left   = gcRand(Target->rect.right - 1);
			rect.right  = gcRand(Target->rect.right - 1);
			rect.top    = gcRand(Target->rect.bottom - 1);
			rect.bottom = gcRand(Target->rect.bottom - 1);
		}

      //  printf("------------------------path : %x,L : %d ,R : %d , T : %d ,B : %d \n",path,rect.left,rect.right,rect.top,rect.bottom);
		color = gcGetNextColor16(color);
      //  printf("color : %x\n",color);
        gcConstructSingleColorBrush(&brush, color, gcFALSE, ~0);
		gcLine(Target, &rect, &brush);

		// Start.
		gcFlush2DAndStall();
		gcStart();
		gcFlushDisplay();
	}
}
