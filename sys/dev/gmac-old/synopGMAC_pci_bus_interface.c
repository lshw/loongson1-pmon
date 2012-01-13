/**\file
 * This file encapsulates all the PCI dependent initialization and resource allocation
 * on Linux
 */  
 
/*	sw
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
*/

#include "synopGMAC_plat.h"
#include "synopGMAC_pci_bus_interface.h"
  


#if 0		//sw
#include "GMAC_Pmon.h"


/**********************************************************************************/ 


#define SYNOPGMAC_VENDOR_ID  0x0700
#define SYNOPGMAC_DEVICE_ID  0x1108
  
#define BAR0  0
#define BAR1  1
#define BAR2  2
#define BAR3  3
#define BAR4  4
#define BAR5  5
static u8 synopGMAC_driver_name[] = "synopGMAC_pci_driver";
static struct pci_device_id ids[] = { 
    {PCI_DEVICE (SYNOPGMAC_VENDOR_ID, SYNOPGMAC_DEVICE_ID),}, {0,} 
};

u32 synop_pci_using_dac;

/**********************************************************************************/ 
extern u8 *synopGMACMappedAddr;
extern u32 synopGMACMappedAddrSize;
extern struct pci_dev *synopGMACpcidev;

/* Get the pci revision id */ 
  static unsigned char
get_revision (struct pci_dev *dev) 
{
  u8 revision;
  pci_read_config_byte (dev, PCI_REVISION_ID, &revision);
  return revision;
}


/**
 * probe function of Linux pci driver.
 * 	- Ioremap the BARx memory (It is BAR0 here) 
 *	- lock the memory for the device
 * \return Returns 0 on success and Error code on failure.
 */ 
static int probe (struct pci_dev *pdev, const struct pci_device_id *my_pci_id) 
{
  s32 retval;
  u16 word_data = 0;
  u32 double_word_data = 0;
  u32 the_register_resource_base, the_register_resource_size;
  
    /* Do probing type stuff here.  
     * Like calling request_region();
     */ 
    TR ("ENABLING PCI DEVICE\n");
  
    /* Enable the device */ 
    if (pci_enable_device (pdev))
    {
      return -ENODEV;
    }
  if (!(retval = pci_set_dma_mask (pdev, DMA_64BIT_MASK))
	&& !(retval = pci_set_consistent_dma_mask (pdev, DMA_64BIT_MASK)))
    {
      synop_pci_using_dac = 1;
    }
  
  else
    {
      if ((pci_set_dma_mask (pdev, DMA_32BIT_MASK))
	   && (retval = pci_set_consistent_dma_mask (pdev, DMA_32BIT_MASK)))
	{
	  TR0 ("NO usable DMA Configuration, aborting\n");
	  return retval;
	}
      synop_pci_using_dac = 0;
    }
  if (synop_pci_using_dac)
    TR ("64 bit double-address cycle Supported with this device\n");
  
  else
    TR ("32 bit double-address cycle Supported with this device\n");
  
    //Setting the pci as master
    pci_set_master (pdev);
  
    /* Get the pci revision id */ 
    if (get_revision (pdev) == 0x42)
    return -ENODEV;
 
    TR ("synopGMAC device found at slot %d, func %d\n",PCI_SLOT (pdev->devfn), PCI_FUNC (pdev->devfn));
  
  {
    
      //u16 w  = 0, dw = 0;
    TR ("****************************************************\n");
    pci_read_config_word (pdev, PCI_COMMAND, &word_data);
    TR ("COMMAND = %04x\n", word_data);
    pci_read_config_word (pdev, PCI_STATUS, &word_data);
    TR ("STATUS  = %04x\n", word_data);
    pci_read_config_dword (pdev, PCI_CLASS_REVISION, &double_word_data);
    TR ("CLASS_REVISION = %08x\n", double_word_data);
    pci_read_config_dword (pdev, PCI_BASE_ADDRESS_0, &double_word_data);
    TR ("BASE_ADDRESS_0 = %08x\n", double_word_data);
    pci_read_config_dword (pdev, PCI_BASE_ADDRESS_1, &double_word_data);
    TR ("BASE_ADDRESS_1 = %08x\n", double_word_data);
    pci_read_config_dword (pdev, PCI_BASE_ADDRESS_2, &double_word_data);
    TR ("BASE_ADDRESS_2 = %08x\n", double_word_data);
    pci_read_config_dword (pdev, PCI_BASE_ADDRESS_3, &double_word_data);
    TR ("BASE_ADDRESS_3 = %08x\n", double_word_data);
    pci_read_config_dword (pdev, PCI_BASE_ADDRESS_4, &double_word_data);
    TR ("BASE_ADDRESS_4 = %08x\n", double_word_data);
    pci_read_config_dword (pdev, PCI_BASE_ADDRESS_5, &double_word_data);
    TR ("BASE_ADDRESS_5 = %08x\n", double_word_data);
    TR ("****************************************************\n");
    
    /*These are the retry counts without which driver fais to run movies :) */ 
    pci_write_config_byte (pdev, 0x40, 0xff);
    pci_write_config_byte (pdev, 0x41, 0xff);
  }
  
    /* Get the resource start address and the size for BARx in question In this case BAR0 */ 
  the_register_resource_base = pci_resource_start (pdev, BAR0);
  the_register_resource_size = pci_resource_len (pdev, BAR0);
  TR ("BAR0 Base is %x size is %d\n", the_register_resource_base,the_register_resource_size);
  
    /*
       Get the iomapped address which is nothing but the physical to virtual mapped address 
       ioremap_nocahe  is similare to ioremap on most of the architectures. But The pci-ahb bridge 
       BAR0 is mapped to 16M address space. If we ask for this much memory, Kernel refuses to 
       give the same. 
       
       Note that 16M is too much of memory to request from kernel. 
       Lets ask for less memory so that kernel is happy giving it  :)
     */ 
    synopGMACMappedAddr = (u8 *) ioremap_nocache ((u32) the_register_resource_base, (size_t) (128 * 1024));
    synopGMACMappedAddrSize = (128 * 1024);	// this is needed for remove function
  
  if (!synopGMACMappedAddr){
      TR0 ("ioremap_nocache failed with addrrss %08x\n", (u32) synopGMACMappedAddr);
  }
  
  TR ("Physical address = %08x\n", the_register_resource_base);
  TR ("Remapped address = %08x\n", (u32) synopGMACMappedAddr);
  
    /*Check if region is already locked by any other driver ? */ 
    if (check_mem_region ((u32) synopGMACMappedAddr, synopGMACMappedAddrSize))
    {
      synopGMACMappedAddr = 0;	// Errored in checking memory region   
      TR0("Memory Already Locked !!\n");
      iounmap (synopGMACMappedAddr);
      return -EBUSY;
    }
  
    /*Great We have free memory of required size.. Lets Lock it... */ 
    request_mem_region ((u32) synopGMACMappedAddr, synopGMACMappedAddrSize,"synopGMACmemory");
    TR ("Requested memory region for synopGMACMappedAddr = 0x%x\n", (u32) synopGMACMappedAddr);
  
    /*Now pci interface is ready. Let give this information the the HOST module */ 
    synopGMACpcidev = pdev;
  return 0;			// Everything is fine. So return 0.
}


/**
 * remove function of Linux pci driver.
 * 
 * 	- Releases the memory allocated by probe function
 *	- Unmaps the memory region 
 *
 * \return Returns 0 on success and Error code on failure.
 */ 
  static void remove (struct pci_dev *dev) 
{
  
    /* Do the reverse of what probe does */ 
    if (synopGMACMappedAddr)
    {
      TR0 ("Releaseing synopGMACMappedAddr 0x%x whose size is %d\n", (u32) synopGMACMappedAddr, synopGMACMappedAddrSize);
      
      /*release the memory region which we locked using request_mem_region */ 
      release_mem_region ((u32) synopGMACMappedAddr, synopGMACMappedAddrSize);
    }
  TR0 ("Unmapping synopGMACMappedAddr =0x%x\n", (u32) synopGMACMappedAddr);
  iounmap (synopGMACMappedAddr);
}

static struct pci_driver pci_driver = 
{ .name = synopGMAC_driver_name,
  .id_table = ids, 
  .probe = probe,
  .remove = remove, 
};


/**
 * Function to initialize the Linux Pci Bus Interface.
 * Registers the pci_driver 
 * \return Returns 0 on success and Error code on failure.
 */

/* sw we do nothing in gs232
  s32 __init synopGMAC_init_pci_bus_interface (void) 
{
  s32 retval;
  TR ("Now Going to Call pci_register_driver\n");
  if (synopGMACMappedAddr)
    return -EBUSY;
  if ((retval = pci_register_driver (&pci_driver)))
    {
      return retval;
    }
  if (!synopGMACMappedAddr)
    {
      pci_unregister_driver (&pci_driver);
      return -ENODEV;
    }
  return 0;
}
*/

#endif	//#if 0


int  synopGMAC_init_pci_bus_interface (void) 
{
  TR ("Now Going to Call pci_register_driver\n");
  TR ("===we do nothing in pci config\n");
  
  return 0;
}

/**
 * Function to De-initialize the Linux Pci Bus Interface.
 * 
 * Unregisters the pci_driver 
 *
 * \return Returns 0 on success and Error code on failure.
 */ 

/*	sw
 void __exit synopGMAC_exit_pci_bus_interface (void) 
{
  TR0 ("Now Calling pci_unregister_driver\n");
  pci_unregister_driver (&pci_driver);
} 
*/


/*
module_init(synopGMAC_init_pci_bus_interface);
module_exit(synopGMAC_exit_pci_bus_interface);

MODULE_AUTHOR("Synopsys India");
MODULE_LICENSE("GPL/BSD");
MODULE_DESCRIPTION("SYNOPSYS GMAC DRIVER PCI INTERFACE");

EXPORT_SYMBOL(synopGMAC_init_pci_bus_interface);
*/ 
