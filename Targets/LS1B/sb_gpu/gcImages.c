#include "gcSdk.h"
#include <string.h>
#include <stdio.h>
//#include "img_Exige_0160x108x24.h"

#define GPU_IMG_LOAD_ADDR   0xA2500000
#define GPU_IMG_LOAD_ADDR_1 0xA2800000
#define GPU_IMG_LOAD_ADDR_2 0xA2B00000
#define GPU_IMG_LOAD_ADDR_3 0xA2008000

#if __mips >= 3
#define nr_str2addr str2addr
#else
#define nr_str2addr strtoul
#endif

extern unsigned long GPU_fbaddr;

#if 1
void gcLoadImage_1(
	gcIMAGEDESCRIPTOR* ImageInfo
	)
{
	// Allocate surface.
	ImageInfo->surface.address = gcMemAllocate(ImageInfo->size);

#if 1  //zgj

    //zgj ImageInfo->surface.address += 640*240*4 ;
    ImageInfo->surface.address = GPU_IMG_LOAD_ADDR_1 ;
    if(!(ImageInfo->surface.address & 0x80000000))
        ImageInfo->surface.address |= 0xA0000000 ;
     printf("Image addr 1 is : %p\n",ImageInfo->surface.address);
#endif
    // Move the image to the aligned location.
	memcpy((void*) ImageInfo->surface.address, ImageInfo->bits, ImageInfo->size);
}
void gcLoadImage_2(
	gcIMAGEDESCRIPTOR* ImageInfo
	)
{
	// Allocate surface.
	ImageInfo->surface.address = gcMemAllocate(ImageInfo->size);

#if 1  //zgj

    //zgj ImageInfo->surface.address += 640*240*4 ;
    ImageInfo->surface.address = GPU_IMG_LOAD_ADDR_2 ;
    if(!(ImageInfo->surface.address & 0x80000000))
        ImageInfo->surface.address |= 0xA0000000 ;
     printf("Image addr 2 is : %p\n",ImageInfo->surface.address);
#endif
    // Move the image to the aligned location.
	memcpy((void*) ImageInfo->surface.address, ImageInfo->bits, ImageInfo->size);
}

void gcLoadImage_3(
	gcIMAGEDESCRIPTOR* ImageInfo
	)
{
	// Allocate surface.
//	ImageInfo->surface.address = gcMemAllocate(ImageInfo->size);

#if 1  //zgj

    //zgj ImageInfo->surface.address += 640*240*4 ;
    ImageInfo->surface.address = GPU_IMG_LOAD_ADDR_3 ;
    if(!(ImageInfo->surface.address & 0x80000000))
        ImageInfo->surface.address |= 0xA0000000 ;
//     printf("Image addr 3 is : %p\n",ImageInfo->surface.address);
#endif
    // Move the image to the aligned location.
	memcpy((void*) ImageInfo->surface.address, ImageInfo->bits, ImageInfo->size);
}
#endif


void gcLoadImage(
	gcIMAGEDESCRIPTOR* ImageInfo
	)
{
#if gcENABLEVIRTUAL
	UINT32 Physical;

	// Allocate surface.
	ImageInfo->surface.address = gcMemAllocateVirtual(ImageInfo->size);

	// Get the physical address.
	Physical = gcGetPhysicalAddress(ImageInfo->surface.address);

	// Move the image to the aligned location.
	memcpy((void*) Physical, ImageInfo->bits, ImageInfo->size);
#else
	// Allocate surface.
	ImageInfo->surface.address = gcMemAllocate(ImageInfo->size);

#if 1  //zgj

    //zgj ImageInfo->surface.address += 640*240*4 ;
  //zgj-2010-3-22  ImageInfo->surface.address = GPU_IMG_LOAD_ADDR  ;
    if(!(ImageInfo->surface.address & 0x80000000))
        ImageInfo->surface.address |= 0xA0000000 ;
     printf("Image addr is : %p\n",ImageInfo->surface.address);
#endif
    // Move the image to the aligned location.
	memcpy((void*) ImageInfo->surface.address, ImageInfo->bits, ImageInfo->size);
#endif
}

UINT32 gcImageWidth(
	gcIMAGEDESCRIPTOR* Surface
	)
{
	return Surface->surface.rect.right - Surface->surface.rect.left;
}

UINT32 gcImageHeight(
	gcIMAGEDESCRIPTOR* Surface
	)
{
	return Surface->surface.rect.bottom - Surface->surface.rect.top;
}

extern void gcMemReset(void);

#include <pmon.h>

#ifdef GPU_LOAD_IMG
extern gcIMAGEDESCRIPTOR exige_0160x108x24 ;

int gc_load_image_test(void)
{

    printf("Load Image begin\n");
    // Load the image.
    gcAppInit();
    //gcMemReset();
    gcLoadImage(&exige_0160x108x24);

    gcMemFree();
    
    printf("Load Image end\n");
}

#endif

#if 1
int gpu_load_image_test(int argc,char *argv)
{
    unsigned int src,dst,count;
    int ret;
    printf("ac : %d ,argv[1]:%s ,argv[2]:%s\n",argv[1],argv[2]);
    if(argc == 4)
        dst=nr_str2addr(argv[3],0,0);
    else if(argc == 3)
        dst = GPU_fbaddr;
    else 
        return -1;
printf("dst: %x\n",dst);
    src=nr_str2addr(argv[1],0,0);
printf("src: %x\n",dst);
    count=strtol(argv[2],0,0);
    printf("src:%x ,dst:%x ,count: %x \n",src,dst,count);
    ret=memcpy(dst,src,count);
    printf("Load end!\n");
    return ret; 
}

static const Cmd Cmds[] =
{
    {"GPU Test"},
 #ifdef GPU_LOAD_IMG
    {"gc_load_img", "", 0, "Load Image to FB", gc_load_image_test, 0, 99, CMD_REPEAT},
 #endif
    {"gpu_load_img", "gpu_load_img src count [fbaddr] ", 0, "Load Image to FB", gpu_load_image_test, 0, 99, CMD_REPEAT},
    {0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void init_cmd() {
    cmdlist_expand(Cmds, 1);
}

#endif
