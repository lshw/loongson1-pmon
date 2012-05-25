#ifndef _gcFILTERBLIT_H_
#define _gcFILTERBLIT_H_


/*******************************************************************************
** Common definitions.
*/

#define MAX_KERNEL_SIZE		9

#define SUBPIXEL_BITS		5
#define SUBPIXEL_COUNT		(1 << SUBPIXEL_BITS)
#define KERNEL_LOAD_COUNT	(SUBPIXEL_COUNT / 2 + 1)

#define WEIGHT_LOAD_COUNT(KernelSize) \
	(KERNEL_LOAD_COUNT * KernelSize)

#define WEIGHT_PAIR_COUNT(KernelSize) \
	(((WEIGHT_LOAD_COUNT(KernelSize) + 1) & ~1) / 2)

#define WEIGHT_PAIR_MAX_COUNT \
	WEIGHT_PAIR_COUNT(MAX_KERNEL_SIZE)


/*******************************************************************************
** Common types.
*/

typedef struct
{
	short weight[9];
}
gcKERNELWEIGHTS;

typedef struct
{
	gcKERNELWEIGHTS kernel[32];
}
gcKERNELTABLE;

typedef struct
{
	gcPOINT srcSize;
	gcPOINT trgSize;

	UINT32 horFactor;
	UINT32 verFactor;

	gcPOINT horSrcOrigin;
	gcRECT horSrcRect;
	gcRECT horTrgRect;

	gcPOINT verSrcOrigin;
	gcRECT verSrcRect;
	gcRECT verTrgRect;

	UINT32 tempFormat;
	UINT32 tempSurfaceWidth;
	UINT32 tempSurfaceHeight;
}
gcFILTERPARAMETERS;


/*******************************************************************************
** API functions.
*/

UINT32 gcGetStretchFactor(
	UINT32 SrcSize,
	UINT32 DstSize
	);

void gcSetStretchFactor(
	UINT32 HorFactor,
	UINT32 VerFactor
	);

void gcComputeKernelTable(
	UINT32 SrcSize,
	UINT32 DstSize,
	UINT32 KernelSize,
	gcKERNELTABLE* Table
	);

void gcProgramKernelTable(
	gcKERNELTABLE* Table
	);

void gcSetVideoSource(
	gcSURFACEINFO* Surface,
	gcRECT* ImageRect,
	gcPOINT* Origin
	);

void gcSetVideoTarget(
	gcSURFACEINFO* Surface,
	gcRECT* TrgRect
	);

void gcComputeFilterBlit(
	gcSURFACEINFO* Target,
	gcSURFACEINFO* Source,
	gcRECT* TrgRect,
	gcRECT* SrcRect,
	gcRECT* TrgSubRect,
	UINT32 HorKernelSize,
	UINT32 VerKernelSize,
	gcFILTERPARAMETERS* Parameters
	);

void gcHorFilterBlit(
	gcSURFACEINFO* Target,
	gcSURFACEINFO* Source,
	gcRECT* TrgRect,
	gcRECT* SrcRect,
	gcPOINT* SrcOrigin
	);

void gcVerFilterBlit(
	gcSURFACEINFO* Target,
	gcSURFACEINFO* Source,
	gcRECT* TrgRect,
	gcRECT* SrcRect,
	gcPOINT* SrcOrigin
	);

void gcFilterBlit(
	gcSURFACEINFO* Target,
	gcSURFACEINFO* Source,
	gcRECT* TrgRect,
	gcRECT* SrcRect,
	gcRECT* TrgSubRect,
	UINT32 HorKernelSize,
	UINT32 VerKernelSize
	);

void gcStartVR(
	UINT32 Horizontal
	);

#endif // _gcFILTERBLIT_H_
