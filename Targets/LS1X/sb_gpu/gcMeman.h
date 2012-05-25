#ifndef _gcMEMAN_H_
#define _gcMEMAN_H_

#if gcENABLEVIRTUAL
UINT32 gcGetVirtualAddress(
	UINT32 Index,
	UINT32 Offset
	);

UINT32 gcGetPhysicalAddress(
	UINT32 VirtualAddress
	);
#endif

void gcMemReset(
	void
	);

UINT32 gcMemAllocate(
	UINT32 Size
	);

#if gcENABLEVIRTUAL
UINT32 gcMemAllocateVirtual(
	UINT32 Size
	);

void gcVirtualizeSurface(
	gcSURFACEINFO* Surface
	);
#endif

void gcMemFree(
	void
	);

void gcAppendEnd(
	void
	);

void gcInitSurface(
	gcSURFACEINFO* Surface,
	UINT32 Width,
	UINT32 Height,
	UINT32 Format
	);

void gcAllocateSurface(
	gcSURFACEINFO* Surface,
	UINT32 Width,
	UINT32 Height,
	UINT32 Format
	);

UINT32 gcAllocateQueueSpace(
	UINT32 Count
	);

#endif // _MEMAN_H_
