#ifndef _gcIMAGES_H_
#define _gcIMAGES_H_


////////////////////////////////////////////////////////////////////////////////
// Coordinate structures.

typedef struct
{
	UINT32 x;
	UINT32 y;
}
gcPOINT;

typedef struct
{
	UINT32 left;
	UINT32 top;
	UINT32 right;
	UINT32 bottom;
}
gcRECT;


////////////////////////////////////////////////////////////////////////////////
// 2D surface.

typedef struct
{
	UINT32 address;
	UINT32 stride;
	UINT32 addressU;
	UINT32 strideU;
	UINT32 addressV;
	UINT32 strideV;
	UINT32 format;
	gcRECT rect;
	gcRECT clip;
}
gcSURFACEINFO;


////////////////////////////////////////////////////////////////////////////////
// 3D texture.

typedef struct
{
	UINT32 width;
	UINT32 height;
	UINT32 depth;
	UINT32 totalSize;
	UINT32 alignment;
	UINT32 format;
	UINT32 lodCount;
	PUINT8 data;
}
gcTEXTURE;


////////////////////////////////////////////////////////////////////////////////
// Image descriptor structure.

typedef struct
{
	gcSURFACEINFO surface;
	PUINT8 bits;
	UINT32 size;
}
gcIMAGEDESCRIPTOR;


////////////////////////////////////////////////////////////////////////////////
// YV12 image structures.

typedef struct
{
	PUINT8 yPlane;
	PUINT8 uPlane;
	PUINT8 vPlane;
}
gcYV12FRAMEHEADER;

typedef struct
{
	UINT32 width;
	UINT32 height;
	UINT32 yPlaneSize;
	UINT32 uPlaneSize;
	UINT32 vPlaneSize;
	UINT32 frameCount;
	gcYV12FRAMEHEADER* frames;
}
gcYV12SEQUENCEHEADER;


////////////////////////////////////////////////////////////////////////////////
// Declare image functions.

void gcLoadImage(
	gcIMAGEDESCRIPTOR* ImageInfo
	);

UINT32 gcImageWidth(
	gcIMAGEDESCRIPTOR* Surface
	);

UINT32 gcImageHeight(
	gcIMAGEDESCRIPTOR* Surface
	);

#endif
