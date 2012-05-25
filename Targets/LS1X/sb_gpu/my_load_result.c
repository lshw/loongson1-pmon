
#include <pmon.h>
#include "result_filterblt.h"

extern long GPU_fbaddr; 

int gpu_flblt_result_test(void)
{
    unsigned int my_fbaddr = GPU_fbaddr ;
    //UINT32 my_cmdbuf_add = gcCMDBUFADDR;
//    unsigned int my_pattern_add = PATTERN_BUFFER_ADDR;
    int i=0;
    int gpu_cmd_size=sizeof(gpu_fliterblt_result);

#if 0
    if(my_cmdbuf_add%16)
    {
        my_cmdbuf_add = my_cmdbuf_add+16 ;
        my_cmdbuf_add &= 0xFFFFFFF0;
    }
printf("my_cmdbuf_add: %p , cmd size:%d ,pattern_size: %d\n",my_cmdbuf_add,gpu_cmd_size,gpu_pattern_size);
    if(!(my_cmdbuf_add & 0x80000000))
        my_cmdbuf_add |= 0xA0000000 ;
#endif

    my_fbaddr |= 0xA0000000;

    for(i=0;i<gpu_cmd_size/4;i++)
    {
        //*(volatile UINT32 *)(my_cmdbuf_add + i) = *(UINT32 *)(gpu_drawline_instructions +i);
        *(volatile unsigned int *)(my_fbaddr + i*4) = gpu_fliterblt_result[i];
    //    printf("value : 0x%x , cmd[%d] : 0x%x\n",*(volatile UINT32 *)(my_cmdbuf_add + i*4),i,*(volatile  UINT32 *)(gpu_drawline_instructions + i));
    }

    printf("Test end\n");
    //    *(volatile UINT32 *)(my_cmdbuf_add + i) = gpu_instructions[i];
    //    *(volatile UINT32 *)(my_cmdbuf_add + 57*4) = my_fbaddr;
    return 0;
}

static const Cmd Cmds[] =
{
    {"GPU Test"},
    {"gc_flblt_result", "", 0, "GPU  Filterblt FUNC",gpu_flblt_result_test , 0, 99, CMD_REPEAT},
    {0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void init_cmd() {
    cmdlist_expand(Cmds, 1);
}

