/*
 * YAFFS: Yet Another Flash File System. A NAND-flash specific file system.
 *
 * Copyright (C) 2002-2007 Aleph One Ltd.
 *   for Toby Churchill Ltd and Brightstar Engineering
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * yaffscfg.c  The configuration for the "direct" use of yaffs.
 *
 * This file is intended to be modified to your requirements.
 * There is no need to redistribute this file.
 */
#include "yaffsfs.h"
#include "yaffs_packedtags2.h"
#include "yaffs_mtdif.h"
#include "yaffs_mtdif2.h"
#include <mtdfile.h>
#include <stdlib.h>


unsigned yaffs_traceMask = 0x0; /* Disable logging */
static int yaffs_errno = 0;

void yaffsfs_SetError(int err)
{
	//Do whatever to set error
	yaffs_errno = err;
}

int yaffsfs_GetError(void)
{
	return yaffs_errno;
}

void yaffsfs_Lock(void)
{
}

void yaffsfs_Unlock(void)
{
}

__u32 yaffsfs_CurrentTime(void)
{
	return 0;
}

void *yaffs_malloc(size_t size)
{
	return malloc(size);
}

void yaffs_free(void *ptr)
{
	free(ptr);
}

void yaffsfs_LocalInitialisation(void)
{
	// Define locking semaphore.
}

// Configuration for:
// /ram  2MB ramdisk
// /boot 2MB boot disk (flash)
// /flash 14MB flash disk (flash)
// NB Though /boot and /flash occupy the same physical device they
// are still disticnt "yaffs_Devices. You may think of these as "partitions"
// using non-overlapping areas in the same device.
//
#if 0
#include <linux/mtd/partitions.h>

static int isMounted = 0;
#if 1
 yaffs_Device *kernelDev = (void *)0;
 yaffs_Device *osDev = (void *)0;
#endif

struct mtd_info *mtd_kernel = (void *)0,*mtd_os = (void *)0;
extern struct mtd_info  *_soc_mtd ;
extern struct mtd_partition partition_info[];

 yaffsfs_DeviceConfiguration yaffsfs_config[] = {
#if 1
	{ "/kernel",0},
	{ "/os",0},
//	{ "/flash", &flashDev},
#endif
	{(void *)0,(void *)0}
};
static void yaffs_StartUppackdev(yaffs_Device **dev,struct mtd_info **mtd,int partitionnum)
{
       	int nBlocks;
        yaffs_Device *kernelDev = *dev;
        struct mtd_info *mtd_kernel = *mtd;
        if( partition_info[partitionnum].size != 0)
            mtd_kernel->size = partition_info[partitionnum].size;
        else
            mtd_kernel->size = _soc_mtd->size - partition_info[partitionnum].offset;

       	kernelDev->genericDevice = mtd_kernel;

        kernelDev->nReservedBlocks = 5;
	kernelDev->nShortOpCaches = 0; // Use caches
	kernelDev->useNANDECC = 0; // do not use YAFFS's ECC
        kernelDev->writeChunkWithTagsToNAND = nandmtd2_WriteChunkWithTagsToNAND;
        kernelDev->readChunkWithTagsFromNAND = nandmtd2_ReadChunkWithTagsFromNAND;
        kernelDev->markNANDBlockBad = nandmtd2_MarkNANDBlockBad;
        kernelDev->queryNANDBlock = nandmtd2_QueryNANDBlock;
        kernelDev->spareBuffer = YMALLOC(mtd_kernel->oobsize);
        kernelDev->isYaffs2 = 1;
#if 1 //(LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17))
        kernelDev->nDataBytesPerChunk = mtd_kernel->writesize;
        kernelDev->nChunksPerBlock = mtd_kernel->erasesize / mtd_kernel->writesize;
#else
        kernelDev->nDataBytesPerChunk = mtd_kernel->oobblock;
        kernelDev->nChunksPerBlock = mtd_kernel->erasesize / mtd_kernel->oobblock;
#endif
        nBlocks = mtd_kernel->size / mtd_kernel->erasesize;
        kernelDev->nCheckpointReservedBlocks = 10;
        kernelDev->startBlock = partition_info[partitionnum].offset / mtd_kernel->erasesize ;
        kernelDev->endBlock = nBlocks - 1;
	kernelDev->eraseBlockInNAND = nandmtd_EraseBlockInNAND;
	kernelDev->initialiseNAND = nandmtd_InitialiseNAND;
}

int yaffs_StartUp(void)
{
        int nBlocks;
        mtd_kernel = malloc(2*sizeof(struct mtd_info));
        kernelDev = malloc(2*sizeof(yaffs_Device));
        if(!mtd_kernel || !kernelDev){
            printf("malloc error!\n");
            return -1;
        }

        memset(kernelDev,0,2*sizeof(yaffs_Device));
        mtd_os = &mtd_kernel[1];
        osDev = &kernelDev[1];
        memcpy(mtd_kernel,_soc_mtd,sizeof(struct mtd_info));
        memcpy(mtd_os,_soc_mtd,sizeof(struct mtd_info));
       
	yaffsfs_LocalInitialisation();
        
        yaffs_StartUppackdev(&kernelDev,&mtd_kernel,0);
        yaffs_StartUppackdev(&osDev,&mtd_os,1);
        yaffsfs_config[0].dev = kernelDev;
        yaffsfs_config[1].dev = osDev;

	yaffs_initialise(yaffsfs_config);

	return 0;
}

 

#endif
