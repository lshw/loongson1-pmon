/* Use biosemu/x86emu to POST VGA cards
 *
 * Copyright 2002 Fuxin Zhang,BaoJian Zheng
 * Institute of Computing Technology,Chinese Academy of Sciences,China 
 */

#include <stdio.h>
#include <stdlib.h>
#include <dev/pci/pcivar.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/io.h>
#include "biosemui.h"

/* Length of the BIOS image */
#define MAX_BIOSLEN         (64 * 1024L)

static	u32              debugFlags = 0;
static	BE_VGAInfo       VGAInfo[1] = {{0}};

static	RMREGS          regs;
static  RMSREGS         sregs;

extern struct pci_device *vga_dev;

//int vga_available = 0;

int vga_bios_init(void)
{
	struct pci_device *pdev;
	unsigned long romsize = 0;
	unsigned long romaddress = 0;
	unsigned char magic[2];
	unsigned short ppcidata; /* pointer to pci data structure */
	unsigned char pcisig[4]; /* signature of pci data structure */
	unsigned char codetype;
/*	unsigned int vesa_mode = 0;*/

	if (vga_dev != NULL)
	{
		pdev = vga_dev;

		printf("Found VGA device: vendor=0x%04x, device=0x%04x\n",
				PCI_VENDOR(pdev->pa.pa_id),pdev->pa.pa_device);

		if (PCI_VENDOR(pdev->pa.pa_id) == 0x1002 && pdev->pa.pa_device == 0x4750)
			BE_wrw(0xc015e,0x4750);

		if (PCI_VENDOR((pdev->pa.pa_id) == 0x102b)) {
			printf("skipping matrox cards\n");
			return -1;
		}

		pci_read_config_dword(pdev,0x30,(int*)&romaddress);
		romaddress &= (~0xf);
		/* enable rom address decode */
		pci_write_config_dword(pdev,0x30,romaddress|1);

		if (romaddress == 0) {
			printf("No rom address assigned,skipped\n");
			return -1;
		}
#ifdef BONITOEL
		romaddress |= 0x10000000;
#endif
        
#ifdef VGAROM_IN_BIOS
                 {
                  extern unsigned char vgarom[];
		  romaddress=vgarom;
       		}
#endif
		printf("Rom mapped to %lx\n",romaddress);

		magic[0] = readb(romaddress);
		magic[1] = readb(romaddress + 1);

		if (magic[0]==0x55 && magic[1]==0xaa) {
			printf("VGA bios found\n");

			/* rom size is stored at offset 2,in 512 byte unit*/
			romsize = (readb(romaddress + 2)) * 512;
			printf("rom size is %ldk\n",romsize/1024);

			ppcidata = readw(romaddress + 0x18);
			printf("PCI data structure at offset %x\n",ppcidata);
			pcisig[0] = readb(romaddress + ppcidata);
			pcisig[1] = readb(romaddress + ppcidata + 1);
			pcisig[2] = readb(romaddress + ppcidata + 2);
			pcisig[3] = readb(romaddress + ppcidata + 3);
			if (pcisig[0]!='P' || pcisig[1]!='C' ||
					pcisig[2]!='I' || pcisig[3]!='R') {
				printf("PCIR expected,read %c%c%c%c\n",
					pcisig[0],pcisig[1],pcisig[2],pcisig[3]);
				printf("Invalid pci signature found,give up\n");
				return -1;
			}

			codetype  = readb(romaddress + ppcidata + 0x14);

			if (codetype != 0) {
				printf("Not x86 code in rom,give up\n");
				return -1;
			}

		} else {
			printf("No valid bios found,magic=%x%x\n",magic[0],magic[1]);
			return -1;
		}


		memset(VGAInfo,0,sizeof(BE_VGAInfo));
		VGAInfo[0].pciInfo = pdev;
		VGAInfo[0].BIOSImage = (void*)malloc(romsize);
		if (VGAInfo[0].BIOSImage == NULL) {
			printf("Error alloc memory for vgabios\n");
			return -1;
		}
		VGAInfo[0].BIOSImageLen = romsize;
#ifdef VGAROM_IN_BIOS
		  memcpy(VGAInfo[0].BIOSImage,(char*)romaddress,romsize);
#else		  
		  memcpy(VGAInfo[0].BIOSImage,(char*)(0xa0000000|romaddress),romsize);
#endif

    		BE_init(debugFlags,65536,&VGAInfo[0]);

    		regs.h.ah = pdev->pa.pa_bus;
    		regs.h.al = (pdev->pa.pa_device<<3)|(pdev->pa.pa_function&0x7);

        	// Execute the BIOS POST code
#ifdef DEBUG_EMU_VGA
		//X86EMU_trace_on();
#endif
        	BE_callRealMode(0xC000,0x0003,&regs,&sregs);
#if 0
{
    RMREGS in;
    RMREGS out;
    in.e.eax = 0x0003;
BE_int86(0x10,&in,&out);
}
#endif

		//BE_exit();
		pci_read_config_dword(pdev,0x30,(int*)&romaddress);
		/* disable rom address decode */
		pci_write_config_dword(pdev,0x30,romaddress & ~1);

		printf("vgabios_init: Emulation done\n");
		vga_available = 1;
	return 1;

	} else{ 
		printf("No VGA PCI device available\n");
		return -1;
	}

}
