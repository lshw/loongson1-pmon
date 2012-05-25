#ifndef _gcSDK_H_
#define _gcSDK_H_

// System headers.
#include <stdio.h>
#include <stdlib.h>

//testStretch
#include <termio.h>
#include <string.h>
#include <setjmp.h>
#include <stdlib.h>
#include <signal.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif
#include <pmon.h>
#include <debugger.h>
//testStretch

// Miscellaneous macros.
#define CountOf(array) \
	sizeof(array) / sizeof(array[0])

#define IN
#define OUT

#define gcFALSE		0
#define gcTRUE		1

// Colors.
#define DEADBEEF32	0xDEADBEEF
#define DEADBEEF16	0xADFDADFD

#define ZGJ32		0x12345678
#define BLACK32		0x00000000
#define WHITE32		0xFFFFFFFF
#define RED32		0x00FF0000
#define GREEN32		0x0000FF00
#define BLUE32		0x000000FF
#define YELLOW32	(RED32 | GREEN32)
#define MAGENTA32	(RED32 | BLUE32)
#define CYAN32		(GREEN32 | BLUE32)

#define BLACK16		0x00000000
#define WHITE16		0xFFFFFFFF
#define RED16		0xF800F800
#define GREEN16		0x07E007E0
#define BLUE16		0x001F001F
#define YELLOW16	(RED16 | GREEN16)
#define MAGENTA16	(RED16 | BLUE16)
#define CYAN16		(GREEN16 | BLUE16)

// Memory allocation defines.
#define gcENABLEVIRTUAL		0

#define gcMEGABYTES(MB) \
	(MB * 1024 * 1024)

// Types.
typedef signed char			INT8;
typedef signed char*		PINT8;

typedef unsigned char		UINT8;
typedef unsigned char*		PUINT8;

typedef signed short		INT16;
typedef signed short*		PINT16;

typedef unsigned short		UINT16;
typedef unsigned short*		PUINT16;

typedef signed int			INT32;
typedef signed int*			PINT32;

typedef unsigned int		UINT32;
typedef unsigned int*		PUINT32;

typedef signed long long	INT64;
typedef signed long long*	PINT64;

typedef unsigned long long	UINT64;
typedef unsigned long long*	PUINT64;

typedef unsigned int		BOOL;
typedef unsigned int*		PBOOL;

typedef void				VOID;
typedef void*				PVOID;

// Our headers.
#include "Reg/AQ.h"
#include "gcImages.h"
#include "gcMeman.h"
#include "gcBrush.h"
#include "gcPrimitives.h"
#include "gcFilterBlit.h"
#include "gcUtilities.h"

// Register base location.
extern UINT32 gcREG_BASE;

// Memory allocation values.
extern UINT32 gcRAMSIZE;
extern UINT32 gcCODESIZE;
extern UINT32 gcHEAPSIZE;
extern UINT32 gcSTACKSIZE;
extern UINT32 gcVIDEOSIZE;

extern UINT32 gcHEAPBASE;
extern UINT32 gcSTACKBASE;
extern UINT32 gcVIDEOBASE;

// Command buffer.
extern UINT32 gcCMDBUFADDR;
extern UINT32 gcCMDBUFSIZE;
extern UINT32 gcCMDBUFCURRADDR;
extern UINT32 gcCMDBUFCURRSIZE;

// Display surface.
extern gcSURFACEINFO gcDisplaySurface;

// API functions.
void gcSetMemoryAllocation(
	void
	);

void gcAppInit(
	void
	);

void gcFlushDisplay(
	void
	);

#endif // _gcSDK_H_
