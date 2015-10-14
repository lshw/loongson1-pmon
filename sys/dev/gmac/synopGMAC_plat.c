/**\file
 *  This file defines the wrapper for the platform/OS related functions
 *  The function definitions needs to be modified according to the platform 
 *  and the Operating system used.
 *  This file should be handled with greatest care while porting the driver
 *  to a different platform running different operating system other than
 *  Linux 2.6.xx.
 * \internal
 * ----------------------------REVISION HISTORY-----------------------------
 * Synopsys			01/Aug/2007			Created
 */
 
#include "synopGMAC_plat.h"
#include "synopGMAC_Dev.h"

dma_addr_t gmac_dmamap(unsigned long va, size_t size)
{
	return VA_TO_PA(va);
}

/**
  * This is a wrapper function for Memory allocation routine. In linux Kernel 
  * it it kmalloc function
  * @param[in] bytes in bytes to allocate
  */

void *plat_alloc_memory(u32 bytes) 
{
	return (void*)malloc((size_t)bytes, M_DEVBUF, M_DONTWAIT);
}

/**
  * This is a wrapper function for consistent dma-able Memory allocation routine. 
  * In linux Kernel, it depends on pci dev structure
  * @param[in] bytes in bytes to allocate
  */

void *plat_alloc_consistent_dmaable_memory(synopGMACdevice *pcidev, u32 size, u32 *addr)
{
	void *buf;

	buf = (void*)malloc((size_t)size, M_DEVBUF, M_DONTWAIT);
	CPU_IOFlushDCache(buf, size, SYNC_W);

	*addr = gmac_dmamap(buf, size);
	buf = (unsigned char *)CACHED_TO_UNCACHED(buf);

	return buf;
}

/**
  * This is a wrapper function for freeing consistent dma-able Memory.
  * In linux Kernel, it depends on pci dev structure
  * @param[in] bytes in bytes to allocate
  */


void plat_free_consistent_dmaable_memory(synopGMACdevice *pcidev, u32 size, void * addr, u32 dma_addr) 
{
	free(PHYS_TO_CACHED(UNCACHED_TO_PHYS(addr)), M_DEVBUF);
}

/**
  * This is a wrapper function for Memory free routine. In linux Kernel 
  * it it kfree function
  * @param[in] buffer pointer to be freed
  */
void plat_free_memory(void *buffer) 
{
	free(buffer, M_DEVBUF);
	return ;
}

dma_addr_t plat_dma_map_single(void *hwdev, void *ptr, size_t size, int direction)
{
	unsigned long addr = (unsigned long) ptr;
	CPU_IOFlushDCache(addr, size, direction);
	return gmac_dmamap(addr,size);
}

/**
  * This is a wrapper function for platform dependent delay 
  * Take care while passing the argument to this function 
  * @param[in] buffer pointer to be freed
  */
void plat_delay(u32 delay)
{
	while (delay--);
	return;
}


/**
 * The Low level function to read register contents from Hardware.
 * 
 * @param[in] pointer to the base of register map  
 * @param[in] Offset from the base
 * \return  Returns the register contents 
 */
u32 synopGMACReadReg(u64 RegBase, u32 RegOffset)
{
	u64 addr;
	u32 data;

	addr = RegBase + (u64)RegOffset;

	data = *(volatile u32 *)addr;

	return data;
}

/**
 * The Low level function to write to a register in Hardware.
 * 
 * @param[in] pointer to the base of register map  
 * @param[in] Offset from the base
 * @param[in] Data to be written 
 * \return  void 
 */
void synopGMACWriteReg(u64 RegBase, u32 RegOffset, u32 RegData )
{
	u64 addr;

	addr = RegBase + (u64)RegOffset;
	*(volatile u32 *)addr = RegData;

	return;
}

/**
 * The Low level function to set bits of a register in Hardware.
 * 
 * @param[in] pointer to the base of register map  
 * @param[in] Offset from the base
 * @param[in] Bit mask to set bits to logical 1 
 * \return  void 
 */
void synopGMACSetBits(u64 RegBase, u32 RegOffset, u32 BitPos)
{
	u32 data;
	data = synopGMACReadReg(RegBase, RegOffset);
	data |= BitPos; 
	synopGMACWriteReg(RegBase, RegOffset, data);

	return;
}


/**
 * The Low level function to clear bits of a register in Hardware.
 * 
 * @param[in] pointer to the base of register map  
 * @param[in] Offset from the base
 * @param[in] Bit mask to clear bits to logical 0 
 * \return  void 
 */
void  synopGMACClearBits(u64 RegBase, u32 RegOffset, u32 BitPos)
{
	u32 data;
	data = synopGMACReadReg(RegBase, RegOffset);
	data &= (~BitPos); 
	synopGMACWriteReg(RegBase, RegOffset, data);

	return;
}

/**
 * The Low level function to Check the setting of the bits.
 * 
 * @param[in] pointer to the base of register map  
 * @param[in] Offset from the base
 * @param[in] Bit mask to set bits to logical 1 
 * \return  returns TRUE if set to '1' returns FALSE if set to '0'. Result undefined there are no bit set in the BitPos argument.
 * 
 */
bool  synopGMACCheckBits(u64 RegBase, u32 RegOffset, u32 BitPos)
{
	u32 data;
	data = synopGMACReadReg(RegBase, RegOffset);
	data &= BitPos; 
	if(data)
		return 1;
	else
		return 0;
}
