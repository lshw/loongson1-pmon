/*
 *	This is a program for loongson 2g southbridge memory space scan
 *	Auther: sw
 */

#include "memscan.h"
#include <stdio.h>

#define CACHEDMEM	0x80000000		//kseg0
#define UNCACHEDMEM	0xa0000000		//kseg1

void memscan()
{
	int i;
	printf("\n====begin to scan memspace...\n");
// 0xbf00,0000 - 0xbfff,ffff	
	for(i = 0xbfe40000;i < 0xbfffffff;i = i+4)
	{	
		printf("==addr: %08x      val: %02x ",i | UNCACHEDMEM,readb(i | UNCACHEDMEM));
		printf("   %02x ",readb(i+1 | UNCACHEDMEM));
		printf("   %02x ",readb(i+2 | UNCACHEDMEM));
		printf("    %02x \n",readb(i+3 | UNCACHEDMEM));
	}	

/*
	for(i = 0xbf000000;i < 0xbfffffff;i = i+4)
	{	
		if(i % 64 == 0)
			printf("==addr: %08x      val: %02x ",i | UNCACHEDMEM,readb(i | UNCACHEDMEM));
		else
			readb(i | UNCACHEDMEM);
	        readb(i+1 | UNCACHEDMEM);
		readb(i+2 | UNCACHEDMEM);
		readb(i+3 | UNCACHEDMEM);
	}
*/

	printf("====scan from 0xbf00,0000 - 0xbfff,ffff done\n");

//kseg1	- DDR Slave 0 
// 0x0 - 0x0fff,ffff	
//	for(i = 0x0;i < 0x0fffffff;i++)
//		printf("==addr: %08x      val: %02x \n",i | UNCACHEDMEM,readb(i | UNCACHEDMEM));	
	
	for(i = 0x0;i < 0x0fffffff;i++)
		readb(i | UNCACHEDMEM);

	printf("====scan from 0x0 - 0x0fff,ffff done\n");

//kseg1	- PCI
// 0x0 - 0x0fff,ffff	
//	for(i = 0x10000000;i < 0x1fffffff;i++)
//		printf("==addr: %08x      val: %02x \n",i | UNCACHEDMEM,readb(i | UNCACHEDMEM));	
	
	for(i = 0x10000000;i < 0x1fffffff;i++)
		readb(i | UNCACHEDMEM);

	printf("====scan from 0x1000,0000 - 0x1fff,ffff done\n");

//sw: may be something wrong here
	
//kuseg - reserved
//	for(i = 0x20000000;i < 0x3fffffff;i++)
//		printf("==addr: %08x      val: %02x \n",i | UNCACHEDMEM,readb(i | UNCACHEDMEM));	
	
	for(i = 0x20000000;i < 0x3fffffff;i++)
		readb(i | UNCACHEDMEM);

	printf("====scan from 0x2000,0000 - 0x3fff,ffff done\n");
	
//kuseg	- DDR Slave 1
// 0x4000,0000 - 0x7fff,ffff
	for(i = 0x40000000;i < 0x7fffffff;i++)
		printf("==addr: %08x      val: %02x \n",i | UNCACHEDMEM,readb(i | UNCACHEDMEM));
	printf("====scan from 0x4000,0000 - 0x7fff,ffff done\n");
		
	
}









