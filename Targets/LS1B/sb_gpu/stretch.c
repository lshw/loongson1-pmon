#include <sys/param.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <machine/cpu.h>
#include <machine/pio.h>

#include <termio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include <linux/io.h>

#include <autoconf.h>

#include <machine/cpu.h>
#include <machine/pio.h>

#include <pmon.h>
#include "gcSdk.h"
#include "testStretch.h"
#include "testLine.h"
#include "testFilterBlt.h"
#include "testMono.h"
#if 0
#include "testClear.h"
#include "testLine.h"
#include "testMask.h"
#include "testFilterBlt.h"
#include "testRotation.h"
#endif

#if 0
typedef void (*gcTESTENTRY)(
	gcSURFACEINFO* Target
	);

typedef struct
{
	char* name;
	gcTESTENTRY entry;
}
gcTESTINFO;

// Define test functions.
gcTESTINFO testArray[] =
{
	{"clear",                 &testClear},
	{"random clear",          &testRandomClear},
	{"line",                  &testLine},
	{"random line",           &testRandomLine},
	{"stretch",               &testStretch},
	{"random stretch",        &testRandomStretch},
	{"mono expansion",        &testMonoExpand},
	{"random mono expansion", &testMonoExpandRandom},
	{"random masked blit",    &testMaskedBlitRandom},
	{"filter blit",           &testFilterBlit},
	{"filter blit formats",   &testFilterBlitFormats},
	{"Rotation",   			  &testRotation},
};
#endif

void gcSetMemoryAllocation(
	void
	)
{
	gcCODESIZE  = gcMEGABYTES(5);
	gcHEAPSIZE  = gcMEGABYTES(1);
	gcSTACKSIZE = gcMEGABYTES(1);
}

int stretch(void)
//	int argc,
//	char** argv
//	)
{
#if 0
	UINT32 loopCount = 1000;
    char fname[20];
    UINT32 test ;
    UINT32 saveBmp = 0;
    UINT32 dst_addr;
#endif
	// Init the system.
//    printf("gcAppInit\n");
	gcAppInit();

	// Virtualize target buffer.
#if gcENABLEVIRTUAL
	gcVirtualizeSurface(&target);
#endif

	// Switch to 2D pipe.
//    printf("Switch to 2D pipe\n");
	gcSelect2DPipe();
#if 0
    while(loopCount)
    {
    	// Generate random test.
		test = printSelectMenu();
		//gcReportRegs();
		
	    //test = 10;//gcRand(CountOf(testArray) - 1);
	    /*
	    if (test/2 == 1)
	    	test = 10;
	    else
	        test = 9;
	      */  
		// Run the test.
		printf("Entering: %s test...", testArray[test].name);
		(*testArray[test].entry)(&gcDisplaySurface);
			
		if(saveBmp == 1)
		{
			dst_addr = gcDisplaySurface.address;
	    	sprintf(fname, "result%04d.bmp", test);
			GalSaveDIBitmap(fname, (unsigned char*)dst_addr, 640, 480);	
		}
			   
		// Report idle state.
		gcReportIdle(" done; IDLE=%08X\n");
		//gcReportRegs();
		loopCount--;
    }
#endif
    printf("Entering: Stretch Test...\n");
    testStretch(&gcDisplaySurface);
    printf("Stretch Test over\n");

    return 0;
}

int my_test_gpu_draw_line(void)
{
	// Init the system.
//    printf("gcAppInit\n");
	gcAppInit();

	// Virtualize target buffer.
#if gcENABLEVIRTUAL
	gcVirtualizeSurface(&target);
#endif

	// Switch to 2D pipe.
//    printf("Switch to 2D pipe\n");
	gcSelect2DPipe();

    printf("Entering: Line Test...\n");
    testLine(&gcDisplaySurface);
    printf("Line Test End\n");
    return 0;
}

int my_test_gpu_random_draw_line(void)
{
	// Init the system.
//    printf("gcAppInit\n");
	gcAppInit();

	// Virtualize target buffer.
#if gcENABLEVIRTUAL
	gcVirtualizeSurface(&target);
#endif

	// Switch to 2D pipe.
//    printf("Switch to 2D pipe\n");
	gcSelect2DPipe();

    printf("Entering: RANDOM Line Test...\n");
    testRandomLine(&gcDisplaySurface);
    printf("RANDOM Line Test End\n");
    return 0;
}

int my_test_gpu_random_stretch(void)
{
	// Init the system.
//    printf("gcAppInit\n");
	gcAppInit();

	// Virtualize target buffer.
#if gcENABLEVIRTUAL
	gcVirtualizeSurface(&target);
#endif

	// Switch to 2D pipe.
//    printf("Switch to 2D pipe\n");
	gcSelect2DPipe();

    printf("Entering: RANDOM Stretch Test...\n");
    testRandomStretch(&gcDisplaySurface);
    printf("RANDOM Stretch Test End\n");
    return 0;
}

int my_test_gpu_FilterBlt(void)
{
	// Init the system.
//    printf("gcAppInit\n");
	gcAppInit();

	// Virtualize target buffer.
#if gcENABLEVIRTUAL
	gcVirtualizeSurface(&target);
#endif

	// Switch to 2D pipe.
//    printf("Switch to 2D pipe\n");
	gcSelect2DPipe();

    printf("Entering: FilterBlit Test...\n");
    testFilterBlit(&gcDisplaySurface);
    printf("FilterBlit Test End\n");
    return 0;
}

int my_test_gpu_MonoExpand(void)
{
	// Init the system.
//    printf("gcAppInit\n");
	gcAppInit();

	// Virtualize target buffer.
#if gcENABLEVIRTUAL
	gcVirtualizeSurface(&target);
#endif

	// Switch to 2D pipe.
//    printf("Switch to 2D pipe\n");
	gcSelect2DPipe();

    printf("Entering: MonoExpand Test...\n");
    testMonoExpand(&gcDisplaySurface);
    printf("MonoExpand Test End\n");
    return 0;
}

int my_test_gpu_MonoExpandRandom(void)
{
	// Init the system.
//    printf("gcAppInit\n");
	gcAppInit();

	// Virtualize target buffer.
#if gcENABLEVIRTUAL
	gcVirtualizeSurface(&target);
#endif

	// Switch to 2D pipe.
//    printf("Switch to 2D pipe\n");
	gcSelect2DPipe();

    printf("Entering: Random MonoExpand Test...\n");
    testMonoExpandRandom(&gcDisplaySurface);
    printf("Random MonoExpand Test End\n");
    return 0;
}

int my_test_gpu_Rotation(void)
{
	// Init the system.
//    printf("gcAppInit\n");
	gcAppInit();

	// Virtualize target buffer.
#if gcENABLEVIRTUAL
	gcVirtualizeSurface(&target);
#endif

	// Switch to 2D pipe.
//    printf("Switch to 2D pipe\n");
	gcSelect2DPipe();

    printf("Entering: Rotation Test...\n");
    testRotation(&gcDisplaySurface);
    printf("Rotation Test End\n");
    return 0;
}

int my_test_gpu_MaskedBlitRandom(void)
{
	// Init the system.
//    printf("MaskedBlitRandom gcAppInit\n");
	gcAppInit();

	// Virtualize target buffer.
#if gcENABLEVIRTUAL
	gcVirtualizeSurface(&target);
#endif

	// Switch to 2D pipe.
//    printf("Switch to 2D pipe\n");
	gcSelect2DPipe();

    printf("Entering: MaskedBlitRandom Test...\n");
    testMaskedBlitRandom(&gcDisplaySurface);
    printf("MaskedBlitRandom Test End\n");
    return 0;
}

int my_test_gpu_FilterBlitFormats(void)
{
	// Init the system.
//    printf("gcAppInit\n");
	gcAppInit();

	// Virtualize target buffer.
#if gcENABLEVIRTUAL
	gcVirtualizeSurface(&target);
#endif

	// Switch to 2D pipe.
//    printf("Switch to 2D pipe\n");
	gcSelect2DPipe();

    printf("Entering: FilterBlitFormats Test...\n");
    testFilterBlitFormats(&gcDisplaySurface);
    printf("FilterBlitFormats Test End\n");
    return 0;
}

#define MY_TEST_NUMBER_MAX 1000

extern  void gpu_rand_seed( unsigned int seed1, unsigned int seed2 );
extern  unsigned int gpu_rand(void);

int my_random_test_gpu(int argc,char **argv)
{

    unsigned int i=0;
    int j=0;
    int n=0;
    unsigned int tmp=0;

    if(argc > 1)
        n = strtol(argv[1],0,0);
    else
        n = MY_TEST_NUMBER_MAX;
    
    printf("==================Test loop count is %d================\n",n);
    for( j=0; j<n; j++)
    {
    printf("-----------------%d test begin----------------\n",j);
    //i = CPU_GetCOUNT();
    gpu_rand_seed(1,1);
    tmp=gpu_rand();
    //printf("tmp: %d \n",tmp);
    i=tmp%10;
    printf("tmp : %d , i:%d\n",tmp,i);
    //i = i%10 ;
    switch(i)
    {
        case 0:
            stretch();
        break;

        case 1:
           // my_test_gpu_FilterBlitFormats();
            my_test_gpu_random_stretch();
        break;

        case 2:
            my_test_gpu_draw_line();            
        break;

        case 3:
            my_test_gpu_random_draw_line();
        break;

        case 4:
            my_test_gpu_FilterBlt();            
        break;

        case 5:
            my_test_gpu_MonoExpand();
        break;
        case 6:
            my_test_gpu_MonoExpandRandom();
        break;

        case 7:
            my_test_gpu_Rotation();
        break;

        case 8:
            my_test_gpu_MaskedBlitRandom();
        break;

        case 9:
            my_test_gpu_FilterBlitFormats();
        break;

        default:
            printf("i:%d ,haha,random is error!\n",i);
        break;
    }
    printf("-----------------------%d test over------------------\n",j);
    }
    printf("ALL test is over\n");
}

extern unsigned int my_cycle_count;
int my_choose_cycle_count(int argc,char ** argv)
{
    if(argc != 2)
     printf("choose_cycle_count number\n");
    else
        my_cycle_count = strtoul(argv[1],0,0);
    printf("my_cycle_count:%d\n",my_cycle_count);
}
//static const TestStretch[] = 
static const Cmd Cmds[] =
{
	{"GPU Test"},
//	{"pcs",	"bus dev func", 0, "select pci dev function", mypcs, 0, 99, CMD_REPEAT},
//	{"disks",	"disk", 0, "select disk", mydisks, 0, 99, CMD_REPEAT},
	{"gpu_alltest", "gpu_alltest [test number]", 0, "test GPU all 2D function", my_random_test_gpu, 0, 99, CMD_REPEAT},
	{"choose_cycle_count", "choose_cycle_count  number", 0, "choose the cycle clock", my_choose_cycle_count, 0, 99, CMD_REPEAT},
	{"stretch", "", 0, "test stretch", stretch, 0, 99, CMD_REPEAT},
	{"random_stretch", "", 0, "random test stretch", my_test_gpu_random_stretch, 0, 99, CMD_REPEAT},
	{"drawline", "", 0, "GPU draw line", my_test_gpu_draw_line, 0, 99, CMD_REPEAT},
	{"randomdrawline", "", 0, "GPU draw line", my_test_gpu_random_draw_line, 0, 99, CMD_REPEAT},
	{"FilterBlt", "", 0, "test FilterBlt", my_test_gpu_FilterBlt, 0, 99, CMD_REPEAT},
	{"MonoExpand", "", 0, "test MonoExpand", my_test_gpu_MonoExpand,0,99,CMD_REPEAT},
	{"MonoExpandRandom", "", 0, "test random MonoExpand", my_test_gpu_MonoExpandRandom,0,99,CMD_REPEAT},
	{"Rotation", "", 0, "test Rotation", my_test_gpu_Rotation,0,99,CMD_REPEAT},
	{"MaskedBlitRandom", "", 0, "test random MaskedBlit", my_test_gpu_MaskedBlitRandom,0,99,CMD_REPEAT},
	{"FilterBlitFormats", "", 0, "test FilterBlitFormats", my_test_gpu_FilterBlitFormats,0,99,CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void init_cmd() {
	cmdlist_expand(Cmds, 1);
}
