/**\file
 *  This file serves as the wrapper for the platform/OS dependent functions
 *  It is needed to modify these functions accordingly based on the platform and the
 *  OS. Whenever the synopsys GMAC driver ported on to different platform, this file
 *  should be handled at most care.
 *  The corresponding function definitions for non-inline functions are available in 
 *  synopGMAC_plat.c file.
 * \internal
 * -------------------------------------REVISION HISTORY---------------------------
 * Synopsys 				01/Aug/2007		 	   Created
 */


#ifndef SYNOP_GMAC_PLAT_H
#define SYNOP_GMAC_PLAT_H 1

#include "GMAC_Pmon.h"

//sw: nothing to display
#ifdef GMAC_DEBUG
#define TR0(fmt, args...)	printf(fmt, ##args) 
#define TR(fmt, args...)		printf(fmt, ##args)
#else
#define TR0(fmt, args...)
#define TR(fmt, args...)
#endif

#define DEFAULT_DELAY_VARIABLE  10
#define DEFAULT_LOOP_VARIABLE   10000

/* Error Codes */
#define ESYNOPGMACNOERR   0
#define ESYNOPGMACNOMEM   1
#define ESYNOPGMACPHYERR  2
#define ESYNOPGMACBUSY    3

/**
  * These are the wrapper function prototypes for OS/platform related routines
  */
void *plat_alloc_memory(u32);
void plat_free_memory(void *);
void plat_delay(u32);

u32 synopGMACReadReg(u64 RegBase, u32 RegOffset);
void synopGMACWriteReg(u64 RegBase, u32 RegOffset, u32 RegData );
void synopGMACSetBits(u64 RegBase, u32 RegOffset, u32 BitPos);
void synopGMACClearBits(u64 RegBase, u32 RegOffset, u32 BitPos);
bool synopGMACCheckBits(u64 RegBase, u32 RegOffset, u32 BitPos);

#endif
