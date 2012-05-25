//#include <pmon.h>
//#include <stdio.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pmon.h>
#include <include/fcr.h>
 #include "gcSdk.h"
#include "testImages.h"

#undef TEST_GPU_FILTERBLT

//#define TEST_GPU_FILTERBLT
#define TEST_GPU_STRETCH

#ifdef TEST_GPU_FILTERBLT 
#include "Source_U.h"
#include "Source_V.h"
#include "Source_Y.h"

#define U_BUFFER_ADDR    0x3600000
#define V_BUFFER_ADDR    0x3800000
#define Y_BUFFER_ADDR    0x3A00000
#define MEDI_BUFFER_ADDR 0x4000000
#endif

#define SRC_BUFFER_ADDR 0
#define FRAME_BUFFER_ADDR_2 0

#define PATTERN_BUFFER_ADDR   0xA2300000
#define PATTERN_BUFFER_ADDR_1 0x02300000

#if 0
unsigned int gpu_instructions[]=
{
    0x08010E00,0x1,0x0801049a,0xFF,
    0x0801049C,0x80ff0000,0x0801049D,0x80ff0000,
    0x0801048A,SRC_BUFFER_ADDR,0x0801048B,0x100,
    0x0801048C,0x0,0x0801048D,0x6,
    0x08010498,0x0,0x08010499,0x040040,
    0x08010497,0x200000,0x20000100,0x0,
    0x0,0x040040,0x0801049a,0xff,
    0x0801049C,0x8000ff00,0x0801049D,0x8000ff00,       //32

    0x0801048A,SRC_BUFFER_ADDR,0x0801048B,0x100,
    0x0801048C,0,0x0801048D,0x6,
    0x08010498,0,0x08010499,0x010020,
    0x08010497,0x200000,0x20000100,0x0,          //48

    0,0x010020,0x0801049a,0xff,
    0x0801049C,0x80123456,0x0801049D,0x80123456,
    0x0801048A,FRAME_BUFFER_ADDR_2,0x0801048B,0x100,
    0x0801048C,0,0x0801048D,6,                     //64

    0x08010498,0,0x08010499,0x300040,
    0x08010497,0x200000,0x20000100,0,
    0,0x300040,0x08010E03,8,
    0x08010480,SRC_BUFFER_ADDR,0x08010481,0x100,   //80

    0x08010483,0x10006,0x08010484,0,
    0x08010485,0x040040,0x08010488,0x10000,
    0x08010489,0x8000,0x08010497,0x30cccc,
    0x0801048e,0x400000,0x0801048f,0xd6,         //96

    0x0801048D,0x4006,0x20000100,0,
    0x000000,0x080040,0x08010E03,8,
    0x08010E02,0x701,0x48000000,0x701,
    0x10000000,0x0
};
#else

unsigned int gpu_drawline_instructions[]=
{
0x8010e00,
0x1,
0x801049a,
0xff,
0x801049c,
0xadfdadfd,
0x801049d,
0xadfdadfd,
0x801048a,
FRAME_BUFFER_ADDR_2,
0x801048b,
0x80,
0x801048c,
0x0,
0x801048d,
0x4,
0x8010498,
0x0,
0x8010499,
0x300040,
0x8010497,
0x200000,
0x20000100,
0x0,
0x0,
0x300040,
0x8010e03,
0x8,
0x8010492,
0xffffffff,
0x8010493,
0xffffffff,
0x801048e,
PATTERN_BUFFER_ADDR_1,
0x8010497,
0x30f0f0,
0x801048d,
0x2004,
0x801048f,
0xd2,
0x20000100,
0x0,
0x50005,
0x2b003b,
0x8010e03,
0x8,
0x8010495,
0xff0000,
0x801048d,
0x1004,
0x801048f,
0xea,
0x20000400,
0x0,
0x50005,
0x5003b,
0x5003b,
0x2b003b,
0x2b003b,
0x2b0005,
0x50005,
0x2b0005,
0x08010e03,
0x8,
0x08010e02,
0x701,
0x48000000,
0x701,
0x10000000,
0x0
};

#endif

//#if 1
#ifdef TEST_GPU_FILTERBLT 
unsigned int gpu_filterblt_instructions[]=
{
0x08010e00,
0x00000001,
0x08010488,
0x0000ccbc, 
0x08010489,
0x00010000, 
0x08010600,
0x00000000,
0x08010601,
0x00000000,
0x08010602,
0x00004000,
0x08010603,
0x00000000,
0x08010604,
0x00000000,
0x08010605,
0x00000000,
0x08010606,
0x40000000,
0x08010607,
0x00000000,
0x08010608,
0x00000000,
0x08010609,
0x00000000,
0x0801060a,
0x00000000,
0x0801060b,
0x00004000,
0x0801060c,
0x00000000,
0x0801060d,
0x00000000,
0x0801060e,
0x00000000,
0x0801060f,
0x40000000,
0x08010610,
0x00000000,
0x08010611,
0x00000000,
0x08010612,
0x00000000,
0x08010613,
0x00000000,
0x08010614,
0x00004000,
0x08010615,
0x00000000,
0x08010616,
0x00000000,
0x08010617,
0x00000000,
0x08010618,
0x40000000,
0x08010619,
0x00000000,
0x0801061a,
0x00000000,
0x0801061b,
0x00000000,
0x0801061c,
0x00000000,
0x0801061d,
0x00004000,
0x0801061e,
0x00000000,
0x0801061f,
0x00000000,
0x08010620,
0x00000000,
0x08010621,
0x40000000,
0x08010622,
0x00000000,
0x08010623,
0x00000000,
0x08010624,
0x00000000,
0x08010625,
0x00000000,
0x08010626,
0x00004000,
0x08010627,
0x00000000,
0x08010628,
0x00000000,
0x08010629,
0x00000000,
0x0801062a,
0x40000000,
0x0801062b,
0x00000000,
0x0801062c,
0x00000000,
0x0801062d,
0x00000000,
0x0801062e,
0x00000000,
0x0801062f,
0x00004000,
0x08010630,
0x00000000,
0x08010631,
0x00000000,
0x08010632,
0x00000000,
0x08010633,
0x40000000,
0x08010634,
0x00000000,
0x08010635,
0x00000000,
0x08010636,
0x00000000,
0x08010637,
0x00000000,
0x08010638,
0x00004000,
0x08010639,
0x00000000,
0x0801063a,
0x00000000,
0x0801063b,
0x00000000,
0x0801063c,
0x40000000,
0x0801063d,
0x00000000,
0x0801063e,
0x00000000,
0x0801063f,
0x00000000,
0x08010640,
0x00000000,
0x08010641,
0x00004000,
0x08010642,
0x00000000,
0x08010643,
0x00000000,
0x08010644,
0x00000000,
0x08010645,
0x40000000,
0x08010646,
0x00000000,
0x08010647,
0x00000000,
0x08010648,
0x00000000,
0x08010649,
0x00000000,
0x0801064a,
0x00004000,
0x0801064b,
0x00000000,
0x0801064c,
0x00000000,
0x08010480,
Y_BUFFER_ADDR,
0x080104a1,
U_BUFFER_ADDR, 
0x080104a3,
V_BUFFER_ADDR, 
0x08010481,
0x00000280, //#Y_BUFFER_STRIDE
0x080104a2,
0x00000140, //#U_BUFFER_STRIDE 
0x080104a4,
0x00000140, //#V_BUFFER_STRIDE 
0x08010482,
0x00000000,
0x080104af,
0x00000000,
0x080104ae,
0x000001e0,
0x08010482,
0x00000280,
0x08010483,
0x00000000,
0x08010483,
0x0000000f, //#source piel format is A8R8G8B8 
0x08010486,
0x00000000,
0x0801048a,
MEDI_BUFFER_ADDR,
0x0801048b,
0x00000c80, //#MEDI_BUFFER_STRIDE
0x0801048d,
0x00000006, //#medi destination piel format is A8R8G8B8
0x0801048c,
0x00000000,
0x080104ad,
0x000001e0,
0x0801048c,
0x00000320,
0x08010498,
0x00000000, //#[14:0] is clip left [30:16] is clip top
0x08010499,
0x01e00320, //#[14:0] is clip right [30:16] is clip bottom
0x080104a6,
0x00000000, //#[14:0] is source left   [30:16] is source top
0x080104a7,
0x01e00280, //#[14:0] is source right  [30:16] is source bottom
0x080104a8,
0x00008000,
0x080104a9,
0x00008000,
0x080104aa,
0x00000000, //#[14:0] is dest left   [30:16] is dest top
0x080104ab,
0x01e00320, ///#[14:0] is dest right  [30:16] is dest bottom
0x080104a5,
0xfffffff6,
0x08010600,
0x00000000,
0x08010601,
0x00000000,
0x08010602,
0x00004000,
0x08010603,
0x00000000,
0x08010604,
0x00000000,
0x08010605,
0x00000000,
0x08010606,
0x40000000,
0x08010607,
0x00000000,
0x08010608,
0x00000000,
0x08010609,
0x00000000,
0x0801060a,
0x00000000,
0x0801060b,
0x00004000,
0x0801060c,
0x00000000,
0x0801060d,
0x00000000,
0x0801060e,
0x00000000,
0x0801060f,
0x40000000,
0x08010610,
0x00000000,
0x08010611,
0x00000000,
0x08010612,
0x00000000,
0x08010613,
0x00000000,
0x08010614,
0x00004000,
0x08010615,
0x00000000,
0x08010616,
0x00000000,
0x08010617,
0x00000000,
0x08010618,
0x40000000,
0x08010619,
0x00000000,
0x0801061a,
0x00000000,
0x0801061b,
0x00000000,
0x0801061c,
0x00000000,
0x0801061d,
0x00004000,
0x0801061e,
0x00000000,
0x0801061f,
0x00000000,
0x08010620,
0x00000000,
0x08010621,
0x40000000,
0x08010622,
0x00000000,
0x08010623,
0x00000000,
0x08010624,
0x00000000,
0x08010625,
0x00000000,
0x08010626,
0x00004000,
0x08010627,
0x00000000,
0x08010628,
0x00000000,
0x08010629,
0x00000000,
0x0801062a,
0x40000000,
0x0801062b,
0x00000000,
0x0801062c,
0x00000000,
0x0801062d,
0x00000000,
0x0801062e,
0x00000000,
0x0801062f,
0x00004000,
0x08010630,
0x00000000,
0x08010631,
0x00000000,
0x08010632,
0x00000000,
0x08010633,
0x40000000,
0x08010634,
0x00000000,
0x08010635,
0x00000000,
0x08010636,
0x00000000,
0x08010637,
0x00000000,
0x08010638,
0x00004000,
0x08010639,
0x00000000,
0x0801063a,
0x00000000,
0x0801063b,
0x00000000,
0x0801063c,
0x40000000,
0x0801063d,
0x00000000,
0x0801063e,
0x00000000,
0x0801063f,
0x00000000,
0x08010640,
0x00000000,
0x08010641,
0x00004000,
0x08010642,
0x00000000,
0x08010643,
0x00000000,
0x08010644,
0x00000000,
0x08010645,
0x40000000,
0x08010646,
0x00000000,
0x08010647,
0x00000000,
0x08010648,
0x00000000,
0x08010649,
0x00000000,
0x0801064a,
0x00004000,
0x0801064b,
0x00000000,
0x0801064c,
0x00000000,
0x08010480,
MEDI_BUFFER_ADDR,
0x08010481,
0x00000c80, // #MEDI_BUFFER_STRIDE
0x08010482,
0x00000320,
0x08010483,
0x00000006, //#MEDI buffer piel format A8R8G8B8
0x0801048a,
FRAME_BUFFER_ADDR_2,
0x080104a7,
0x01e00320,  // ##[14:0] is source right  [30:16] is source bottom
0x080104a5,
0xfffffff7,
0x08010e03,
0x00000008,
0x08010e02,
0x00000701,
0x48000000,
0x00000701,
0x10000000,
0x00000000
};
#endif

#if 1
unsigned int gpu_drawline_pattern[]=
{
0x7BA1229C,
0x03E47BA1,
0x7BA17BA1,
0x7BA1229C,
0x7BA1229C,
0x03E40604,
0x7BA10604,
0x229C229C,
0x7BA17BA1,
0x03E403E4,
0x7BA103E4,
0x7BA17BA1,
0x06047BA1,
0x7C0403E4,
0x060403E4,
0x7BA17BA1,
0x03E47BA1,
0x03E47C04,
0x03E403E4,
0x7BA17BA1,
0x03E40604,
0x03E403E4,
0x03E47C04,
0x7BA10604,
0x7BA17BA1,
0x41617BA1,
0x7BA17BA1,
0x7BA17BA1,
0x7BA1229C,
0x7BA17BA1,
0x7BA17BA1,
0x229C229C
};
#endif


#ifdef TEST_GPU_STRETCH 
unsigned int gpu_stretch_instructions[]=
{
0x08020488,
0x00052108,
0x0009ba2e,
0x0ee04000, 
0x08060480,
0x02008000,
0x00000280,
0x00000000, 
0x00000005,
0x00000000,
0x006c00a0,
0x00300800, 
0x08010486,
0xffffffff,
0x08010497,
0x0030cccc, 
0x0804048a,
0x0ee04000,
0x00000a00,
0x00000000, 
0x00004006,
0x01e00280,
0x08020498,
0x00000000, 
0x01e00280,
0x00023d4b,
0x20000100,
0x00d90122, 
0x01180134,
0x01240154,
0x08010e03,
0x00000008, 
0x08010e02,
0x00000701,
0x48000000,
0x00000701, 
0x10000000,
0x00000000,
0x08010497,
0x0030cccc, 
0x0804048a,
0x0ee04000,
0x00000a00,
0x00000000, 
0x00004006,
0xffce0000,
0x08020498,
0x00000000, 
0x01e00280,
0x00000008,
0x20000100,
0x00000701, 
0x01240084,
0x015800cc,
0x08010e03,
0x00000008, 
0x08010e02,
0x00000701,
0x48000000,
0x00000701, 
0x10000000,
0x00000000 
};
#endif
extern unsigned long GPU_fbaddr;

#if 0
int gpu_mystretch(void)
{
    UINT32 my_fbaddr = (GPU_fbaddr & 0x0FFFFFFF );
    UINT32 my_cmdbuf_add = gcCMDBUFADDR;
    int i=0;
    int gpu_cmd_size=sizeof(gpu_instructions);
    
    if(my_cmdbuf_add & 0x80000000)
        my_cmdbuf_add |= 0xA0000000 ;
    for(i=0;i<gpu_cmd_size;i=i+4)
    {
        *(volatile UINT32 *)(my_cmdbuf_add + i) = gpu_instructions[i];
    }

        *(volatile UINT32 *)(my_cmdbuf_add + 9*4 ) = ;
        *(volatile UINT32 *)(my_cmdbuf_add + i) = gpu_instructions[i];
        *(volatile UINT32 *)(my_cmdbuf_add + 57*4) = my_fbaddr;
}
#else

#ifndef UINT32
// typedef unsigned int UINT32 ;
#endif



int gpu_mydrawline(void)
{
    UINT32 my_fbaddr = (GPU_fbaddr & 0x0FFFFFFF );
    //UINT32 my_cmdbuf_add = gcCMDBUFADDR;
    UINT32 my_pattern_add = PATTERN_BUFFER_ADDR;
    int i=0;
    int gpu_cmd_size=sizeof(gpu_drawline_instructions)+16;
    int gpu_pattern_size=sizeof(gpu_drawline_pattern);
    //char *my_cmdbuf_add_1 =(char *)malloc(71) ;
    UINT32 *my_cmdbuf_add_1 =(UINT32 *)malloc(gpu_cmd_size) ;
    UINT32 my_cmdbuf_add = my_cmdbuf_add_1;
    if(my_cmdbuf_add%16)
    {
        my_cmdbuf_add = my_cmdbuf_add+16 ;
        my_cmdbuf_add &= 0xFFFFFFF0;
    }
printf("my_cmdbuf_add: %p , cmd size:%d ,pattern_size: %d\n",my_cmdbuf_add,gpu_cmd_size,gpu_pattern_size);    

    if(!(my_cmdbuf_add & 0x80000000))
        my_cmdbuf_add |= 0xA0000000 ;
   
    gpu_cmd_size=sizeof(gpu_drawline_instructions);

    for(i=0;i<gpu_cmd_size/4;i++)
    {
        //*(volatile UINT32 *)(my_cmdbuf_add + i) = *(UINT32 *)(gpu_drawline_instructions +i);
        *(volatile UINT32 *)(my_cmdbuf_add + i*4) = gpu_drawline_instructions[i];
    //    printf("value : 0x%x , cmd[%d] : 0x%x\n",*(volatile UINT32 *)(my_cmdbuf_add + i*4),i,*(volatile  UINT32 *)(gpu_drawline_instructions + i));
    }

        *(volatile UINT32 *)(my_cmdbuf_add + 9*4 ) = my_fbaddr;

    for(i=0; i<gpu_pattern_size/4; i++)
    {
        //*(volatile UINT32 *)(my_pattern_add + i) = *(UINT32 *)(gpu_drawline_pattern + i);
        *(volatile UINT32 *)(my_pattern_add + i*4) = gpu_drawline_pattern[i];
    }

    printf("Dump the CMD BUF\n");
    for(i=0;i<gpu_cmd_size;i=i+4)
    {
        if((i%16) == 0)
            printf("%p :  ",(my_cmdbuf_add+i));
        printf(" %8x ",*(UINT32 *)(my_cmdbuf_add+i));
        if((i%16) == 12)
            printf("\n");

    }
    printf("\nDump the PATTERN BUF\n");
    for(i=0;i<gpu_pattern_size;i=i+4)
    {
        if((i%16) == 0)
            printf("%p :  ",(my_pattern_add+i));
        printf(" %8x ",*(UINT32 *)(my_pattern_add+i));
        if((i%16) == 12)
            printf("\n");

    }
   
   for(i=0;i<200;i++)
   {
    my_cmdbuf_add &= 0x0FFFFFFF;
    printf("\n my_cmdbuf_add : %p\n",my_cmdbuf_add);
    *(volatile unsigned int *)0xBC200654 = my_cmdbuf_add; 
    *(volatile unsigned int *)0xBC200658 = 0xFFFFFFFF;
    printf("%d done\n");
    }
    printf("Test end\n");
    free(my_cmdbuf_add_1);
    //    *(volatile UINT32 *)(my_cmdbuf_add + i) = gpu_instructions[i];
    //    *(volatile UINT32 *)(my_cmdbuf_add + 57*4) = my_fbaddr;
    return 0;
}
#endif

#ifdef TEST_GPU_FILTERBLT

extern void apSleep( UINT32 msec );
 
int gpu_myfilterblt(void)
{
    UINT32 my_fbaddr_filterblt = (GPU_fbaddr & 0x0FFFFFFF );
    //UINT32 my_cmdbuf_add = gcCMDBUFADDR;
    UINT32 my_source_u_add = U_BUFFER_ADDR;
    UINT32 my_source_v_add = V_BUFFER_ADDR;
    UINT32 my_source_y_add = Y_BUFFER_ADDR;
    UINT32 idle;
    int i=0;
    int j=0;
    int gpu_cmd_size=sizeof(gpu_filterblt_instructions)+16;
    int gpu_source_u_size=sizeof(source_u_array);
    int gpu_source_v_size=sizeof(source_v_array);
    int gpu_source_y_size=sizeof(source_y_array);
    //char *my_cmdbuf_add_1 =(char *)malloc(71) ;
    UINT32 *my_cmdbuf_add_1 =(UINT32 *)malloc(gpu_cmd_size) ;
    UINT32 my_cmdbuf_add = my_cmdbuf_add_1;
    if(my_cmdbuf_add%16)
    {
        my_cmdbuf_add = my_cmdbuf_add+16 ;
        my_cmdbuf_add &= 0xFFFFFFF0;
    }
printf("my_cmdbuf_add: %p , cmd size:%d ,U_size: %d\n",my_cmdbuf_add,gpu_cmd_size,gpu_source_u_size);    

    if(!(my_cmdbuf_add & 0x80000000))
        my_cmdbuf_add |= 0xA0000000 ;
   
    gpu_cmd_size=sizeof(gpu_filterblt_instructions);

    for(i=0;i<gpu_cmd_size/4;i++)
    {
        //*(volatile UINT32 *)(my_cmdbuf_add + i) = *(UINT32 *)(gpu_drawline_instructions +i);
        *(volatile UINT32 *)(my_cmdbuf_add + i*4) = gpu_filterblt_instructions[i];
    //    printf("value : 0x%x , cmd[%d] : 0x%x\n",*(volatile UINT32 *)(my_cmdbuf_add + i*4),i,*(volatile  UINT32 *)(gpu_drawline_instructions + i));
    }

        // *(volatile UINT32 *)(my_cmdbuf_add + 9*4 ) = my_fbaddr;
        *(volatile UINT32 *)(my_cmdbuf_add + 379*4 ) = my_fbaddr_filterblt;

    printf("CMD BUF cp done \n");
    my_source_u_add |= 0xA0000000;
    my_source_v_add |= 0xA0000000;
    my_source_y_add |= 0xA0000000;

    for(i=0; i<gpu_source_u_size/4; i++)
    {
        //*(volatile UINT32 *)(my_pattern_add + i) = *(UINT32 *)(gpu_drawline_pattern + i);
        *(volatile UINT32 *)(my_source_u_add + i*4) = source_u_array[i];
    }
    printf("Source u BUF cp done \n");
    for(i=0; i<gpu_source_v_size/4; i++)
    {
        //*(volatile UINT32 *)(my_pattern_add + i) = *(UINT32 *)(gpu_drawline_pattern + i);
        *(volatile UINT32 *)(my_source_v_add + i*4) = source_v_array[i];
    }
    printf("Source v BUF cp done \n");
    for(i=0; i<gpu_source_y_size/4; i++)
    {
        //*(volatile UINT32 *)(my_pattern_add + i) = *(UINT32 *)(gpu_drawline_pattern + i);
        *(volatile UINT32 *)(my_source_y_add + i*4) = source_y_array[i];
    }
    printf("Source y BUF cp done \n");

    printf("Dump the CMD BUF\n");
    for(i=0;i<gpu_cmd_size;i=i+4)
    {
        if((i%16) == 0)
            printf("%p :  ",(my_cmdbuf_add+i));
        printf(" %8x ",*(UINT32 *)(my_cmdbuf_add+i));
        if((i%16) == 12)
            printf("\n");

    }
#if 0
    printf("\nDump the PATTERN BUF\n");
    for(i=0;i<gpu_pattern_size;i=i+4)
    {
        if((i%16) == 0)
            printf("%p :  ",(my_pattern_add+i));
        printf(" %8x ",*(UINT32 *)(my_pattern_add+i));
        if((i%16) == 12)
            printf("\n");

    }
#endif
   for(i=0;i<200;i++)
   {
    my_cmdbuf_add &= 0x0FFFFFFF;
    printf("\n my_cmdbuf_add : %p\n",my_cmdbuf_add);
    *(volatile unsigned int *)0xBC200654 = my_cmdbuf_add; 
    *(volatile unsigned int *)0xBC200658 = 0xFFFFFFFF; 

    // Wait for idle.
    for (j = 0; j < 5000; j++)
    {
        idle = gcReportIdle(NULL);
        if (!(idle ^ 0x7FFFFFFF)) break;
        apSleep(1);
    }

    if (idle ^ 0x3FFFFFFF)
    {
        printf("gcStart: chip has not become idle: 0x%08X\n", idle);
    }

    printf("%d done\n",i);
    }
    printf("Test end\n");
    free(my_cmdbuf_add_1);
    //    *(volatile UINT32 *)(my_cmdbuf_add + i) = gpu_instructions[i];
    //    *(volatile UINT32 *)(my_cmdbuf_add + 57*4) = my_fbaddr;
    return 0;
}
#endif

#ifdef TEST_GPU_STRETCH

extern void apSleep( UINT32 msec );
 
int gpu_mystretch_1(void)
{
    UINT32 my_fbaddr_stretch = (GPU_fbaddr & 0x0FFFFFFF );
    UINT32 idle;
    int i=0;
    int j=0;
    int gpu_cmd_size=sizeof(gpu_stretch_instructions)+16;
    UINT32 my_cmdbuf_add = 0xA2000000; //  my_cmdbuf_add_1;
    
    if(my_cmdbuf_add%16)
    {
        my_cmdbuf_add = my_cmdbuf_add+16 ;
        my_cmdbuf_add &= 0xFFFFFFF0;
    }
printf("my_cmdbuf_add: %p , cmd size:%d \n",my_cmdbuf_add,gpu_cmd_size);    

    if(!(my_cmdbuf_add & 0x80000000))
        my_cmdbuf_add |= 0xA0000000 ;
   
    gpu_cmd_size=sizeof(gpu_stretch_instructions);

    for(i=0;i<gpu_cmd_size/4;i++)
    {
        //*(volatile UINT32 *)(my_cmdbuf_add + i) = *(UINT32 *)(gpu_drawline_instructions +i);
        *(volatile UINT32 *)(my_cmdbuf_add + i*4) = gpu_stretch_instructions[i];
    //    printf("value : 0x%x , cmd[%d] : 0x%x\n",*(volatile UINT32 *)(my_cmdbuf_add + i*4),i,*(volatile  UINT32 *)(gpu_drawline_instructions + i));
    }

        // *(volatile UINT32 *)(my_cmdbuf_add + 9*4 ) = my_fbaddr;
 //       *(volatile UINT32 *)(my_cmdbuf_add + 379*4 ) = my_fbaddr_stretch;

    printf("CMD BUF cp done \n");

     // Load the image.
    gcLoadImage_3(&exige_0160x108x24);

#if 0
    printf("Dump the CMD BUF\n");
    for(i=0;i<gpu_cmd_size;i=i+4)
    {
        if((i%16) == 0)
            printf("%p :  ",(my_cmdbuf_add+i));
        printf(" %8x ",*(UINT32 *)(my_cmdbuf_add+i));
        if((i%16) == 12)
            printf("\n");

    }
#endif

   for(i=0;i<200;i++)
   {
    my_cmdbuf_add &= 0x0FFFFFFF;
    printf("\n my_cmdbuf_add : %p\n",my_cmdbuf_add);
    *(volatile unsigned int *)0xBC200654 = my_cmdbuf_add; 
    *(volatile unsigned int *)0xBC200658 = 0xFFFFFFFF; 

    // Wait for idle.
    for (j = 0; j < 500000; j++)
    {
        idle = gcReportIdle(NULL);
        if (!(idle ^ 0x000000FF)) break;
        apSleep(1);
    }

    if (idle ^ 0x000000FF)
    {
        printf("gcStart: chip has not become idle: 0x%08X\n", idle);
    }

   // printf("%d done\n",i);
 //   }
#if 1

 //   i=1;
 //  {
    my_cmdbuf_add &= 0x0FFFFFFF;
    printf("\n my_cmdbuf_add : %p\n",my_cmdbuf_add);
    *(volatile unsigned int *)0xBC200654 = my_cmdbuf_add + 0x98; 
    printf("\n after my_cmdbuf_add : %p\n",my_cmdbuf_add + 0x98);
    *(volatile unsigned int *)0xBC200658 = 0xFFFFFFFF; 

    // Wait for idle.
    for (j = 0; j < 500000; j++)
    {
        idle = gcReportIdle(NULL);
        if (!(idle ^ 0x000000FF)) break;
        apSleep(1);
    }

    if (idle ^ 0x000000FF)
    {
        printf("gcStart: chip has not become idle: 0x%08X\n", idle);
    }

    printf("%d done\n",i);
    }
#endif

    printf("Test end\n");
//    free(my_cmdbuf_add_1);
    //    *(volatile UINT32 *)(my_cmdbuf_add + i) = gpu_instructions[i];
    //    *(volatile UINT32 *)(my_cmdbuf_add + 57*4) = my_fbaddr;
    return 0;
}
#endif

static const Cmd Cmds[] =
{
    {"GPU Test"},
    {"gc_drawline1", "", 0, "DUMP GC Draw line",gpu_mydrawline , 0, 99, CMD_REPEAT},
#ifdef TEST_GPU_FILTERBLT 
    {"gc_filterblt", "", 0, "GC Filter Blt",gpu_myfilterblt , 0, 99, CMD_REPEAT},
#endif
#ifdef TEST_GPU_STRETCH 
    {"gc_stretch", "", 0, "GC Stretch",gpu_mystretch_1 , 0, 99, CMD_REPEAT},
#endif
    {0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void init_cmd() {
    cmdlist_expand(Cmds, 1);
}

