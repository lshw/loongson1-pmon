/* concatenation of following two 16-bit multiply with carry generators */
/* x(n)=a*x(n-1)+carry mod 2^16 and y(n)=b*y(n-1)+carry mod 2^16, */
/* number and carry packed within the same 32 bit integer.        */
/******************************************************************/

#include <sys/param.h>
#include <sys/syslog.h>
#include <machine/endian.h>
#include <sys/device.h>
#include <machine/cpu.h>
#include <machine/pio.h>
#include <machine/intr.h>
#include <dev/pci/pcivar.h>
#include <sys/types.h>
#include <termio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include <dev/ic/mc146818reg.h>
#include <linux/io.h>

#include <autoconf.h>

#include <machine/cpu.h>
#include <machine/pio.h>
#include "pflash.h"
#include "dev/pflash_tgt.h"

#include "include/fcr.h"
#include <pmon/dev/gt64240reg.h>
#include <pmon/dev/ns16550.h>
#include <target/types.h>
#include <target/lcd.h>

#include <pmon.h>


unsigned int gpu_rand( void );           /* returns a random 32-bit integer */
void  gpu_rand_seed( unsigned int, unsigned int );      /* seed the generator */

/* return a random float >= 0 and < 1 */
#define gpu_rand_float          ((double)Rand() / 4294967296.0)

unsigned int GPU_RAN_SEED_X = 521288629;
unsigned int GPU_RAN_SEED_Y = 362436069;


unsigned int gpu_rand()
   {
   static unsigned int a = 18000, b = 30903;

   GPU_RAN_SEED_X = a*(GPU_RAN_SEED_X&65535) + (GPU_RAN_SEED_X>>16);
   GPU_RAN_SEED_Y = b*(GPU_RAN_SEED_Y&65535) + (GPU_RAN_SEED_Y>>16);

   return ((GPU_RAN_SEED_X<<16) + (GPU_RAN_SEED_Y&65535));
   }


#if 0
void gpu_rand_seed( unsigned int seed1, unsigned int seed2 )
   {
   #if 1
   if (seed1) GPU_RAN_SEED_X = seed1;   /* use default seeds if parameter is 0 */
   if (seed2) GPU_RAN_SEED_Y = seed2;
    #endif
    }
#else
void gpu_rand_seed( unsigned int seed1, unsigned int seed2 )
   {
        if (seed1 > 1) 
            GPU_RAN_SEED_X = seed1;   /* use default seeds if parameter is 0 */
        else if(seed1 == 1)
            GPU_RAN_SEED_X = CPU_GetCOUNT();
        else 
            GPU_RAN_SEED_X = 521288629;
            
        if (seed2 > 1) 
            GPU_RAN_SEED_Y = seed2;
        else if(seed2 == 1)
            GPU_RAN_SEED_Y = CPU_GetCOUNT();
        else
            GPU_RAN_SEED_Y = 362436069;
   }
#endif
