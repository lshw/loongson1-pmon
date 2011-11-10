/*
 *fcr-nand.h
 */

#ifndef _FCR_NAND_H
#define _FCR_NAND_H
#include <machine/types.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>

static void fcr_soc_hwcontrol(struct mtd_info *mtd, int dat, unsigned int ctrl);

int fcr_soc_foryaffs_init(struct mtd_info *mtd);

int fcr_soc_nand_init(void);

#endif
