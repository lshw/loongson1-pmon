#include "gcSdk.h"
//#include <stdio.h>

//#include <stdlib.h>

#include <ctype.h>

#include "gcUtilities.h"

#include "display.h"
#include "utilities.h"
#include "common.h"

#undef MY_POKE_GPU_DEBUG

int _MIN(
	const int n1,
	const int n2
	)
{
	return (n1 < n2) ? n1 : n2;
}

int _MAX(
	const int n1,
	const int n2
	)
{
	return (n1 > n2) ? n1 : n2;
}

// Extract bits from an 8-bit value.
UINT8 GETBITS8(
	const UINT8 Data,
	const int Start,
	const int End
	)
{
	const int _Start = _MIN(Start, End);
	const int _End = _MAX(Start, End);
	const int _Size = _End - _Start + 1;
	const UINT8 _Mask = ((UINT8) ~0) >> ((8 - _Size) & 7);
	return (Data >> _Start) & _Mask;
}

// Extract bits from a 16-bit value.
UINT16 GETBITS16(
	const UINT16 Data,
	const int Start,
	const int End
	)
{
	const int _Start = _MIN(Start, End);
	const int _End = _MAX(Start, End);
	const int _Size = _End - _Start + 1;
	const UINT16 _Mask = ((UINT16) ~0) >> ((16 - _Size) & 15);
	return (Data >> _Start) & _Mask;
}

// Extract bits from a 32-bit value.
UINT32 GETBITS32(
	const UINT32 Data,
	const int Start,
	const int End
	)
{
	const int _Start = _MIN(Start, End);
	const int _End = _MAX(Start, End);
	const int _Size = _End - _Start + 1;
	const UINT32 _Mask = ((UINT32) ~0) >> ((32 - _Size) & 31);
	return (Data >> _Start) & _Mask;
}

// Extract bits from a 64-bit value.
UINT64 GETBITS64(
	const UINT64 Data,
	const int Start,
	const int End
	)
{
	const int _Start = _MIN(Start, End);
	const int _End = _MAX(Start, End);
	const int _Size = _End - _Start + 1;
	const UINT64 _Mask = ((UINT64) ~0) >> ((64 - _Size) & 63);
	return (Data >> _Start) & _Mask;
}

// Set bits in an 8-bit value.
UINT8 SETBITS8(
	UINT8* Data,
	const int Start,
	const int End,
	const UINT8 Value
	)
{
	const int _Start = _MIN(Start, End);
	const int _End = _MAX(Start, End);
	const int _Size = _End - _Start + 1;
	const UINT8 _Mask = ((UINT8) ~0) >> ((8 - _Size) & 7);
	*Data &= ~(_Mask << _Start);
	*Data |= (Value & _Mask) << _Start;
	return *Data;
}

// Set bits in a 16-bit value.
UINT16 SETBITS16(
	UINT16* Data,
	const int Start,
	const int End,
	const UINT16 Value
	)
{
	const int _Start = _MIN(Start, End);
	const int _End = _MAX(Start, End);
	const int _Size = _End - _Start + 1;
	const UINT16 _Mask = ((UINT16) ~0) >> ((16 - _Size) & 15);
	*Data &= ~(_Mask << _Start);
	*Data |= (Value & _Mask) << _Start;
	return *Data;
}

// Set bits in a 32-bit value.
UINT32 SETBITS32(
	UINT32* Data,
	const int Start,
	const int End,
	const UINT32 Value
	)
{
	const int _Start = _MIN(Start, End);
	const int _End = _MAX(Start, End);
	const int _Size = _End - _Start + 1;
	const UINT32 _Mask = ((UINT32) ~0) >> ((32 - _Size) & 31);
	*Data &= ~(_Mask << _Start);
	*Data |= (Value & _Mask) << _Start;
	return *Data;
}

// Set bits in a 64-bit value.
UINT64 SETBITS64(
	UINT64* Data,
	const int Start,
	const int End,
	const UINT64 Value
	)
{
	const int _Start = _MIN(Start, End);
	const int _End = _MAX(Start, End);
	const int _Size = _End - _Start + 1;
	const UINT64 _Mask = ((UINT64) ~0) >> ((64 - _Size) & 63);
	*Data &= ~(_Mask << _Start);
	*Data |= (Value & _Mask) << _Start;
	return *Data;
}

void gcPoke(
	UINT32 Address,
	UINT32 Data
	)
{
    if(!(Address & 0x80000000))
        Address |= 0xA0000000;
    if((Data & 0xFF000000)==0xA2000000)
    {
    #ifdef MY_POKE_GPU_DEBUG
        printf("org data : 0x%x\n",Data);
    #endif
        Data &= 0x0FFFFFFF;
    }
    #ifdef MY_POKE_GPU_DEBUG
    printf("===========poke %p : 0x%8x  ==========\n",Address,Data);
    #endif
	*(PUINT32) Address = Data;
}

UINT32 gcPeek(
	UINT32 Address
	)
{
    if(!(Address & 0x80000000))
        Address |= 0xA0000000;
	return *(PUINT32) Address;
}

UINT32 gcGetNextColor16(
	UINT32 Color
	)
{
	switch (Color)
	{
	case RED16:
		return GREEN16;

	case GREEN16:
		return BLUE16;

	case BLUE16:
		return YELLOW16;

	case YELLOW16:
		return MAGENTA16;

	case MAGENTA16:
		return CYAN16;

	case CYAN16:
		return DEADBEEF16;
	}

	return RED16;
}

extern unsigned int gpu_rand(void);
extern void gpu_rand_seed(unsigned int,unsigned int);

UINT32 gcRand(
	UINT32 Range
	)
{
	// Generate a random in range 0..RAND_MAX.
	//UINT32 value = rand();
    gpu_rand_seed(1,1);
	UINT32 value = gpu_rand();

#if 0
	// Normalize to 0..1 range.
	float normValue = ((float) value) / RAND_MAX;

	// Convert to requested range.
	UINT32 result = (UINT32) (normValue * Range + 0.5);
#else
    UINT32 result = value % Range + 1;
#if 0
    if((result+4) < Range)
        result = (result + 4)&0xFFFFFFFC;
    else
        result = result & 0xFFFFFFFC;
#endif
#endif
    #ifdef MY_POKE_GPU_DEBUG
    printf("value : 0x%x , result : %d\n",value,result);
    #endif
    // return result.
	return result;
}

UINT32 gcRandExclusive(
	UINT32 Range,
	UINT32 ExcludeValue
	)
{
	UINT32 result;

	do
	{
		result = gcRand(Range);
	}
	while (result == ExcludeValue);

	return result;
}

void gcNormalizeRect(
	gcRECT* Rect,
	gcRECT* BoundingRect
	)
{
	if (Rect->left > Rect->right)
	{
		UINT32 temp = Rect->left;
		Rect->left = Rect->right;
		Rect->right = temp;
	}

	if (Rect->top > Rect->bottom)
	{
		UINT32 temp = Rect->top;
		Rect->top = Rect->bottom;
		Rect->bottom = temp;
	}

	if (BoundingRect != NULL)
	{
		UINT32 bndWidth  = BoundingRect->right  - BoundingRect->left;
		UINT32 bndHeight = BoundingRect->bottom - BoundingRect->top;

		if (Rect->right - Rect->left > bndWidth)
		{
			Rect->right = Rect->left + bndWidth;
		}

		if (Rect->bottom - Rect->top > bndHeight)
		{
			Rect->bottom = Rect->top + bndHeight;
		}

		if (Rect->left < BoundingRect->left)
		{
			UINT32 overlap = BoundingRect->left - Rect->left;
			Rect->left  += overlap;
			Rect->right += overlap;
		}

		if (Rect->top < BoundingRect->top)
		{
			UINT32 overlap = BoundingRect->top - Rect->top;
			Rect->top    += overlap;
			Rect->bottom += overlap;
		}

		if (Rect->right > BoundingRect->right)
		{
			UINT32 overlap = Rect->right - BoundingRect->right;
			Rect->left  -= overlap;
			Rect->right -= overlap;
		}

		if (Rect->bottom > BoundingRect->bottom)
		{
			UINT32 overlap = Rect->bottom - BoundingRect->bottom;
			Rect->top    -= overlap;
			Rect->bottom -= overlap;
		}
	}
}

void gcGenerateRect(
	gcRECT* Rect,
	gcRECT* BoundingRect
	)
{
	UINT32 maxWidth  = BoundingRect->right  - BoundingRect->left;
	UINT32 maxHeight = BoundingRect->bottom - BoundingRect->top;

	Rect->left   = gcRand(maxWidth);
	Rect->top    = gcRand(maxHeight);
	Rect->right  = gcRandExclusive(maxWidth,  Rect->left);
	Rect->bottom = gcRandExclusive(maxHeight, Rect->top);

	gcNormalizeRect(Rect, BoundingRect);
}

#if 1
void gcGenerateRect_Stretch(
	gcRECT* Rect,
	gcRECT* BoundingRect
	)
{
	UINT32 maxWidth  = BoundingRect->right  - BoundingRect->left;
	UINT32 maxHeight = BoundingRect->bottom - BoundingRect->top;

	Rect->left   = gcRand(maxWidth);
	Rect->top    = gcRand(maxHeight);
	Rect->right  = gcRandExclusive(maxWidth,  Rect->left);
	//Rect->right  = 3 + Rect->left;
	Rect->bottom = gcRandExclusive(maxHeight, Rect->top);
	//Rect->bottom = 1 + Rect->top;

	gcNormalizeRect(Rect, BoundingRect);
}
#endif

void gcPrintRect(
	char* Message,
	gcRECT* Rect
	)
{
	printf("%s = (%d, %d) %d x %d\n",
		Message,
		Rect->left,
		Rect->top,
		Rect->right  - Rect->left,
		Rect->bottom - Rect->top
		);
}

UINT32 gcSurfaceWidth(
	gcSURFACEINFO* Surface
	)
{
	return Surface->rect.right - Surface->rect.left;
}

UINT32 gcSurfaceHeight(
	gcSURFACEINFO* Surface
	)
{
	return Surface->rect.bottom - Surface->rect.top;
}

void gcAlignSurface(
	PUINT8* Surface,
	UINT32 Size,
	UINT32 Alignment
	)
{
	UINT32 alignmentMask = Alignment - 1;
	UINT32 alignedAddess = (((UINT32) *Surface) + alignmentMask) & ~alignmentMask;
	UINT32 offset = alignedAddess - ((UINT32) *Surface);

	if (offset != 0)
	{
		UINT32 i;

		PUINT8 srcTail = *Surface + Size - 1;
		PUINT8 trgTail = (PUINT8) (alignedAddess + Size - 1);

		for (i = 0; i < Size; i++)
		{
			*trgTail-- = *srcTail--;
		}

		*Surface = (PUINT8) alignedAddess;
	}
}

typedef struct BMPINFOHEADER {                     
    unsigned int   biSize;           
    int            biWidth;          
    int            biHeight;         
    unsigned short biPlanes;         
    unsigned short biBitCount;       
    unsigned int   biCompression;    
    unsigned int   biSizeImage;      
    int            biXPelsPerMeter;  
    int            biYPelsPerMeter;  
    unsigned int   biClrUsed;        
    unsigned int   biClrImportant;   
} BMPINFOHEADER;

typedef union RGB {
	struct{             
    unsigned char   rgbBlue;          
    unsigned char   rgbGreen;         
    unsigned char   rgbRed;           
    unsigned char   rgbReserved;
	} RGB1; 
	unsigned long u;
} RGB;

typedef struct _BMPINFO {                      
    BMPINFOHEADER   bmiHeader;      
    RGB             bmiColors[256]; 
} BMPINFO;

#define BF_TYPE 0x4D42 

static int write_word(FILE *fp, unsigned short w)
{
    putc(w, fp);
    return (putc(w >> 8, fp));
}

/*
 * 'write_dword()' - Write a 32-bit unsigned integer.
 */
static int write_dword(FILE *fp, unsigned int dw)
{
    putc(dw, fp);
    putc(dw >> 8, fp);
    putc(dw >> 16, fp);
    return (putc(dw >> 24, fp));
}

/*
 * 'write_long()' - Write a 32-bit signed integer.
 */
static int write_long(FILE *fp,int  l)
{
    putc(l, fp);
    putc(l >> 8, fp);
    putc(l >> 16, fp);
    return (putc(l >> 24, fp));
}

int GalSaveDIBitmap(const char *filename, unsigned char *bits, int alignedWidth, int alignedHeight)
{
    FILE *fp;
    BMPINFO bitmap;
    BMPINFO *info = &bitmap;                      
    unsigned int    size, infosize, bitsize;                 
    int i, srcStride, dstStride;

    if ((fp = fopen(filename, "wb")) == NULL)
        return (-1);


	
	bitmap.bmiHeader.biSize = 40;
	bitmap.bmiHeader.biWidth = alignedWidth;
	bitmap.bmiHeader.biHeight = -alignedHeight;
	bitmap.bmiHeader.biPlanes = 1;
	bitmap.bmiHeader.biBitCount = 16;
	bitmap.bmiHeader.biCompression = 3;
	bitmap.bmiHeader.biSizeImage = bitsize =  info->bmiHeader.biWidth *
        	        ((info->bmiHeader.biBitCount + 7) / 8) *
		        (-info->bmiHeader.biHeight);;
	bitmap.bmiHeader.biXPelsPerMeter = 0;
	bitmap.bmiHeader.biYPelsPerMeter = 0;
	bitmap.bmiHeader.biClrUsed = 0;
	bitmap.bmiHeader.biClrImportant = 0;
	
	bitmap.bmiColors[0].u           =  0xF800;
    bitmap.bmiColors[1].u           = 0x07E0;
    bitmap.bmiColors[2].u           = 0x001F;
	


    srcStride  =  info->bmiHeader.biWidth * ((info->bmiHeader.biBitCount + 7) / 8);
    dstStride  =  alignedWidth * ((info->bmiHeader.biBitCount + 7) / 8);

    
    infosize = 40+12;

    size = 14 + infosize + bitsize;

    write_word(fp, BF_TYPE);        
    write_dword(fp, size);          
    write_word(fp, 0);              
    write_word(fp, 0);              
    write_dword(fp, 14 + infosize); 

    write_dword(fp, info->bmiHeader.biSize);
    write_long(fp, info->bmiHeader.biWidth);
    write_long(fp, abs(info->bmiHeader.biHeight));
    write_word(fp, info->bmiHeader.biPlanes);
    write_word(fp, info->bmiHeader.biBitCount);
    write_dword(fp, info->bmiHeader.biCompression);
    write_dword(fp, info->bmiHeader.biSizeImage);
    write_long(fp, info->bmiHeader.biXPelsPerMeter);
    write_long(fp, info->bmiHeader.biYPelsPerMeter);
    write_dword(fp, info->bmiHeader.biClrUsed);
    write_dword(fp, info->bmiHeader.biClrImportant);
    write_long(fp, bitmap.bmiColors[0].u);
    write_long(fp, bitmap.bmiColors[1].u);
    write_long(fp, bitmap.bmiColors[2].u);
    printf("GalSaveDIBitmap");
    for (i = 0; i < abs(info->bmiHeader.biHeight); i++){
		if (info->bmiHeader.biHeight > 0)
			fwrite(bits + dstStride * i, 1, srcStride, fp);
		else
			fwrite(bits + dstStride * (abs(info->bmiHeader.biHeight) - 1 - i), 1, srcStride, fp);
    }

    fclose(fp);
    return (0);
}


UINT32 printSelectMenu(void)

{

    UINT32		 choice;

	char         buffer[8];	

	printf("Select which test you want to run:\n");

	

	printf("0  - clear!\n");

	printf("1  - random clear!\n");

	printf("2  - line!\n");

	printf("3  - random line!\n");

	printf("4  - stretch!\n");

	printf("5  - random stretch!\n");

	printf("6  - mono expansion!\n");

	printf("7  - random mono expansion!\n");	

	printf("8  - random masked blit!\n");	

	printf("9  - filter blit! \n");

	printf("10 - filter blit formats! \n");
	
	printf("11 - rotation blit!\n");
	
	printf("99 - random tests! \n");

	do {

    	choice = 99;

    	printf ("\nChoice: ");

	    fgets (buffer, sizeof(buffer), stdin);
	    
	    if(&buffer[8] == "99")	    
	    {
	    	choice = gcRand(10);
	    }
	    else
	    {
	    	sscanf (buffer, "%d", &choice);
	    }  

	}

	while (choice > 38 );

	return choice;

}
