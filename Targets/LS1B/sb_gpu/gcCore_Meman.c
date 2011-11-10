#include "gcSdk.h"
#include <target/lcd.h>

extern unsigned long GPU_fbaddr;

typedef struct
{
	UINT32 addr;
	UINT32 size;
#if gcENABLEVIRTUAL
	UINT32 index;
#endif
}
gcALLOCHISTORY;

#if gcENABLEVIRTUAL
// Maximum entries in the page table.
#define PAGE_TABLE_ENTRIES	1024
#define PAGE_SIZE			4096

// Page table.
static BOOL g_gcPageTableFlushed;
static UINT32 g_gcPageTableAddr;
static UINT32 g_gcPageTableSize;
static UINT32 g_gcPageTableIndex;
#endif

// The address of the next block of memory to be allocated.
static UINT32 g_gcCurrAddr;
static UINT32 g_gcCurrSize;

// Command buffer.
UINT32 gcCMDBUFADDR;
UINT32 gcCMDBUFSIZE;
UINT32 gcCMDBUFCURRADDR;
UINT32 gcCMDBUFCURRSIZE;

// Allocation history.
static gcALLOCHISTORY g_gcHistory[128];
static UINT32 g_gcHistoryIndex;

// General allocation.
UINT32 gcCODESIZE;
UINT32 gcHEAPSIZE;
UINT32 gcSTACKSIZE;
UINT32 gcVIDEOSIZE;

UINT32 gcHEAPBASE;
UINT32 gcSTACKBASE;
UINT32 gcVIDEOBASE;

#if gcENABLEVIRTUAL
UINT32 gcGetVirtualAddress(
	UINT32 Index,
	UINT32 Offset
	)
{
	UINT32 result
		= SETFIELDVALUE(0, AQ_MEMORY_ADDRESS, TYPE,    VIRTUAL_SYSTEM)
		| SETFIELD     (0, AQ_MEMORY_ADDRESS, ADDRESS, Offset | (Index << 12));

	return result;
}

UINT32 gcGetPhysicalAddress(
	UINT32 VirtualAddress
	)
{
	// Retrive the index and the offset.
	UINT32 Index  = GETFIELD(VirtualAddress, AQ_MEMORY_ADDRESS, ADDRESS) >> 12;
	UINT32 Offset = GETFIELD(VirtualAddress, AQ_MEMORY_ADDRESS, ADDRESS) & (PAGE_SIZE - 1);

	// Read back the physical address.
	UINT32 Physical = gcPeek(g_gcPageTableAddr + Index * sizeof(UINT32)) | Offset;

	// Return result.
	return Physical;
}
#endif

#undef MY_GPU_DEBUG_MEMRESET 

void gcMemReset(
	void
	)
{
	// Reset current buffer info.
	// g_gcCurrAddr = gcVIDEOBASE = __SD_LCD_BAR_BASE;
	g_gcCurrAddr = gcVIDEOBASE ; //zgj = GPU_fbaddr & 0x0FFFFFFF ;
	g_gcCurrSize = gcVIDEOSIZE ; //zgj-2010-3-22 = 680 * 480;

#ifdef MY_GPU_DEBUG_MEMRESET
    printf("Video memory  : 0x%08X to 0x%08X\n", 
		g_gcCurrAddr, g_gcCurrAddr + g_gcCurrSize);
#endif

	// Reset history.
	g_gcHistoryIndex = 0;

	// Allocate command buffer.
	gcCMDBUFSIZE = gcCMDBUFCURRSIZE = 32 * 1024;
#if 0  //zgj-2010-3-22
    gcCMDBUFCURRADDR = (UINT32 *)malloc(gcCMDBUFSIZE);
    gcCMDBUFCURRADDR &= 0x0FFFFFFF ;  //zgj
    //gcCMDBUFCURRADDR |= 0xA0000000 ;

#if 1 //zgj
    if(gcCMDBUFCURRADDR % 16)
    gcCMDBUFCURRADDR = (gcCMDBUFCURRADDR + 16) & 0x0ffffff0;
#endif

	gcCMDBUFADDR = gcCMDBUFCURRADDR ; 
#endif

	gcCMDBUFADDR = gcCMDBUFCURRADDR = gcMemAllocate(gcCMDBUFSIZE);
    #ifdef MY_GPU_DEBUG_MEMRESET
    printf("Command buffer: 0x%08X to 0x%08X\n", 
		gcCMDBUFADDR, gcCMDBUFADDR + gcCMDBUFSIZE);
    #endif

#if gcENABLEVIRTUAL
	// Allocate page table.
	g_gcPageTableFlushed = TRUE;
	g_gcPageTableSize    = PAGE_TABLE_ENTRIES * sizeof(UINT32);
	g_gcPageTableAddr    = gcMemAllocate(g_gcPageTableSize);
	g_gcPageTableIndex   = 0;
	printf("Page table    : 0x%08X to 0x%08X\n",
		g_gcPageTableAddr, g_gcPageTableAddr + g_gcPageTableSize);

	// Set the base address of the page table.
	gcWriteReg(AQMemoryFePageTableRegAddrs,  g_gcPageTableAddr);
	gcWriteReg(AQMemoryTxPageTableRegAddrs,  g_gcPageTableAddr);
	gcWriteReg(AQMemoryPePageTableRegAddrs,  g_gcPageTableAddr);
	gcWriteReg(AQMemoryPezPageTableRegAddrs, g_gcPageTableAddr);
	gcWriteReg(AQMemoryRaPageTableRegAddrs,  g_gcPageTableAddr);
#endif
}

UINT32 gcMemAllocate(
	UINT32 Size
	)
{
	UINT32 result = NULL;

	// Align.
	Size = (Size + 63) & ~63;

	// Do we have enough memory?
	if (Size <= g_gcCurrSize)
	{
		// Set the result.
		result = g_gcCurrAddr;

		// Store current pointer and size.
		g_gcHistory[g_gcHistoryIndex].addr  = g_gcCurrAddr;
		g_gcHistory[g_gcHistoryIndex].size  = g_gcCurrSize;
#if gcENABLEVIRTUAL
		g_gcHistory[g_gcHistoryIndex].index = g_gcPageTableIndex;
#endif
		g_gcHistoryIndex++;

		// Update current pointers.
		g_gcCurrAddr += Size;
		g_gcCurrSize -= Size;
	}

	// Return result.
	return result;
}

#if gcENABLEVIRTUAL
static UINT32 gcInitPageTable(
	UINT32 Address,
	UINT32 Size
	)
{
	UINT32 pageIndex;
	UINT32 pageCount;
	UINT32 pageOffset;

	// Store current page table index.
	pageIndex = g_gcPageTableIndex;

	// Align.
	Size = (Size + 63) & ~63;

	// Calculate the number of pages to allocate.
	pageCount = (Size + PAGE_SIZE - 1) / PAGE_SIZE;

	// Separate the offset.
	pageOffset = Address & (PAGE_SIZE - 1);
	Address &= ~(PAGE_SIZE - 1);

	// Init pages.
	while (pageCount--)
	{
		if (g_gcPageTableIndex < PAGE_TABLE_ENTRIES)
		{
			// Set the physical address to the entry.
			gcPoke(g_gcPageTableAddr + g_gcPageTableIndex * sizeof(UINT32),
				   Address);

			// Advance page table index.
			g_gcPageTableIndex += 1;

			// Advance physical address.
			Address += PAGE_SIZE;
		}
		else
		{
			printf("Page table entries are exhausted!\n");
			break;
		}
	}

	// Flush if necessary.
	if (!g_gcPageTableFlushed)
	{
		g_gcPageTableFlushed = TRUE;

		gcWriteReg(AQMMUFlushRegAddrs,
				   SETFIELDVALUE(0, AQMMU_FLUSH, FEMMU,  ENABLE) |
				   SETFIELDVALUE(0, AQMMU_FLUSH, RAMMU,  ENABLE) |
				   SETFIELDVALUE(0, AQMMU_FLUSH, TXMMU,  ENABLE) |
				   SETFIELDVALUE(0, AQMMU_FLUSH, PEMMU,  ENABLE) |
				   SETFIELDVALUE(0, AQMMU_FLUSH, PEZMMU, ENABLE));
	}

	// Determine virtual address.
	return gcGetVirtualAddress(pageIndex, pageOffset);
}

UINT32 gcMemAllocateVirtual(
	UINT32 Size
	)
{
	UINT32 result = gcMemAllocate(Size);

	if (result)
	{
		result = gcInitPageTable(result, Size);
	}

	return result;
}

void gcVirtualizeSurface(
	gcSURFACEINFO* Surface
	)
{
	// Determine the size of the surface.
	unsigned long height = (Surface->rect.bottom + 7) & ~7;
	unsigned long size   =  Surface->stride * height;

	// Set new virtual address.
	Surface->address = gcInitPageTable(Surface->address, size);
}
#endif

void gcMemFree(
	void
	)
{
	if (g_gcHistoryIndex)
	{
		g_gcHistoryIndex--;
		g_gcCurrAddr = g_gcHistory[g_gcHistoryIndex].addr;
		g_gcCurrSize = g_gcHistory[g_gcHistoryIndex].size;
#if gcENABLEVIRTUAL
		g_gcPageTableIndex = g_gcHistory[g_gcHistoryIndex].index;
		g_gcPageTableFlushed = FALSE;
#endif
	}
}

void gcAppendEnd(
	void
	)
{
	gcPoke(gcCMDBUFCURRADDR,
		   SETFIELDVALUE(0, AQ_COMMAND_END_COMMAND, OPCODE, END));
	gcCMDBUFCURRADDR += 4;

	gcPoke(gcCMDBUFCURRADDR,
		   0);
	gcCMDBUFCURRADDR += 4;
}

void gcInitSurface(
	gcSURFACEINFO* Surface,
	UINT32 Width,
	UINT32 Height,
	UINT32 Format
	)
{
	// Initialize the surface rectangle.
	Surface->rect.left   = 0;
	Surface->rect.top    = 0;
	Surface->rect.right  = (Width  + 7) & ~7;
	Surface->rect.bottom = (Height + 7) & ~7;

	// Set initial clipping.
	Surface->clip = Surface->rect;

	// Set the format.
	Surface->format = Format;

	// Compute the stride.
	Surface->stride = Surface->rect.right * gcGetPixelSize(Format) / 8;
}

void gcAllocateSurface(
	gcSURFACEINFO* Surface,
	UINT32 Width,
	UINT32 Height,
	UINT32 Format
	)
{
	UINT32 surfaceSize;

	// Init surface parameters.
	gcInitSurface(Surface, Width, Height, Format);

	// Compute the size of the surface.
	surfaceSize = Surface->stride * Surface->rect.bottom;

	// Allocate the temporary surface.
#if gcENABLEVIRTUAL
	Surface->address = gcMemAllocateVirtual(surfaceSize);
#else
	Surface->address = gcMemAllocate(surfaceSize);
#endif
}

UINT32 gcAllocateQueueSpace(
	UINT32 Count
	)
{
	UINT32 result;
	UINT32 size = ((Count + 1) & ~1) * sizeof(UINT32);

	// Queue is full?
	if (size > gcCMDBUFCURRSIZE - 8)
	{
		gcStart();
	}

	// Allocate the space.
	result = gcCMDBUFCURRADDR;
	gcCMDBUFCURRADDR += size;
	gcCMDBUFCURRSIZE -= size;

	// Return result.
	return result;
}

int gc_dump_cmdbuf(void)
{
    int i=0;
    UINT32 dump_addr=gcCMDBUFADDR;
    //UINT32 dump_addr=gcCMDBUFCURRADDR;
    printf("Dump GC CMD buffer\n");
    if(!(dump_addr & 0x80000000))
        dump_addr |= 0xA0000000;
    for(i=0;i<512; i=i+4)
    //for(i=0;i<gcCMDBUFCURRSIZE; i=i+4)
    {
        if((i%16) == 0)
            printf("%p :  ",(dump_addr+i));
        printf(" %8x ",*(UINT32 *)(dump_addr+i));
        if((i%16) == 12)
            printf("\n");
    }

    return 0;
}
#if 0
int gc_dump_cmdbuf_toram(void)
{
    int i=0;
    //UINT32 dump_addr=gcCMDBUFADDR;
    UINT32 dump_addr=gcCMDBUFCURRADDR;
    printf("Dump GC CMD buffer\n");
    if(!(dump_addr & 0x80000000))
        dump_addr |= 0xA0000000;
    //for(i=0;i<512; i=i+4)
    for(i=0;i<gcCMDBUFCURRSIZE; i=i+4)
    {
        if((i%16) == 0)
            printf("%p :  ",(dump_addr+i));
        printf(" %8x ",*(UINT32 *)(dump_addr+i));
        if((i%16) == 12)
            printf("\n");
    }

    return 0;
}
#endif

static const Cmd Cmds[] =
{
    {"GPU Test"},
    {"gc_dump", "", 0, "DUMP GC CMD BUF", gc_dump_cmdbuf, 0, 99, CMD_REPEAT},
    {0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void init_cmd() {
    cmdlist_expand(Cmds, 1);
}

