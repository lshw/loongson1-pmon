/*
 *  drivers/mtd/nand/fcr_soc.c
 */

#include <machine/types.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include "fcr-nand.h"
/*
 * MTD structure for fcr_soc board
 */
static struct mtd_info *fcr_soc_mtd = NULL;
/*
 * Define partitions for flash device
 */
#if 0
static const struct mtd_partition partition_info[] = {
	{
	 .name = "fcr_soc flash partition 1",
	 .offset = 12*1024*1024,
	 .size = 32 * 1024 * 1024}
};
#endif




#define NUM_PARTITIONS 1

static void fcr_soc_hwcontrol(struct mtd_info *mtd, int dat,unsigned int ctrl)
{
	struct nand_chip *chip = mtd->priv;

if((ctrl & NAND_CTRL_ALE)==NAND_CTRL_ALE)
		*(volatile unsigned int *)(0xbf000014) = dat;
if ((ctrl & NAND_CTRL_CLE)==NAND_CTRL_CLE)
		*(volatile unsigned int *)(0xbf000010) = dat;
}

static void find_good_part(struct mtd_info *fcr_soc_mtd)
{
int offs;
int start=-1;
char name[20];
int idx=0;
for(offs=0;offs< fcr_soc_mtd->size;offs+=fcr_soc_mtd->erasesize)
{
if(fcr_soc_mtd->block_isbad(fcr_soc_mtd,offs)&& start>=0)
{
	sprintf(name,"g%d",idx++);
	add_mtd_device(fcr_soc_mtd,start,offs-start,name);
	start=-1;
}
else if(start<0)
{
 start=offs;
}

}

if(start>=0)
{
	sprintf(name,"g%d",idx++);
	add_mtd_device(fcr_soc_mtd,start,offs-start,name);
}
}

//Main initialization for yaffs ----- by zhly
int fcr_soc_foryaffs_init(struct mtd_info *mtd)
{
	struct nand_chip *this;
	
	if(!mtd)
	{
		if(!(mtd = kmalloc(sizeof(struct mtd_info),GFP_KERNEL)))
		{
			printk("unable to allocate mtd_info structure!\n");
			return -ENOMEM;
		}
		memset(mtd, 0, sizeof(struct mtd_info));
	}

	this = kmalloc(sizeof(struct nand_chip),GFP_KERNEL);
	if(!this)
	{
		printk("Unable to allocate nand_chip structure!\n");
		return -ENOMEM;
	}
	memset(this,0,sizeof(struct nand_chip));
		
	fcr_soc_mtd=mtd;
	fcr_soc_mtd->priv = this;

	this->IO_ADDR_R = (void *)0xbf000040;
	this->IO_ADDR_W = (void *)0xbf000040;
	this->cmd_ctrl = fcr_soc_hwcontrol;
	this->ecc.mode = NAND_ECC_SOFT;
	
	if(nand_scan(fcr_soc_mtd,1))
	{
		kfree(fcr_soc_mtd);
		printk("nand_scan failed!\n");
		return -1;		
	}

	find_good_part(fcr_soc_mtd);

	return 0;
}

/*
 * Main initialization routine
 */
int fcr_soc_nand_init(void)
{
	struct nand_chip *this;

	/* Allocate memory for MTD device structure and private data */
	fcr_soc_mtd = kmalloc(sizeof(struct mtd_info) + sizeof(struct nand_chip), GFP_KERNEL);
	if (!fcr_soc_mtd) {
		printk("Unable to allocate fcr_soc NAND MTD device structure.\n");
		return -ENOMEM;
	}
	/* Get pointer to private data */
	this = (struct nand_chip *)(&fcr_soc_mtd[1]);

	/* Initialize structures */
	memset(fcr_soc_mtd, 0, sizeof(struct mtd_info));
	memset(this, 0, sizeof(struct nand_chip));

	/* Link the private data with the MTD structure */
	fcr_soc_mtd->priv = this;


	/* Set address of NAND IO lines */
	this->IO_ADDR_R = (void  *)0xbf000040;
	this->IO_ADDR_W = (void  *)0xbf000040;
	/* Set address of hardware control function */
	this->cmd_ctrl = fcr_soc_hwcontrol;
	/* 15 us command delay time */
	this->chip_delay = 15;
	this->ecc.mode = NAND_ECC_SOFT;
	/* Scan to find existence of the device */
	if (nand_scan(fcr_soc_mtd, 1)) {
		kfree(fcr_soc_mtd);
		return -ENXIO;
	}

	/* Register the partitions */
//	add_mtd_partitions(fcr_soc_mtd, partition_info, NUM_PARTITIONS);
	add_mtd_device(fcr_soc_mtd,0,0,"total");
	add_mtd_device(fcr_soc_mtd,0,0x2000000,"kernel");

	add_mtd_device(fcr_soc_mtd,0x2000000,0x2000000,"os");
	
        find_good_part(fcr_soc_mtd);


	/* Return happy */
	return 0;
}


