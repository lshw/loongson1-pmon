/*
 *  drivers/mtd/nand_ecc.h
 *
 *  Copyright (C) 2000 Steven J. Hill (sjhill@cotw.com)
 *
 * $Id: nand_ecc.h,v 1.1.1.1 2006/05/08 03:32:49 cpu Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This file is the header for the ECC algorithm.
 */

/*
 * Creates non-inverted ECC code from line parity
 */
void nand_trans_result(u_char reg2, u_char reg3, u_char *ecc_code);

/*
 * Calculate 3 byte ECC code for 256 byte block
 */
extern struct mtd_info;
int nand_calculate_ecc(struct mtd_info *mtd, const u_char *dat,
		       u_char *ecc_code);

/*
 * Detect and correct a 1 bit error for 256 byte block
 */
int nand_correct_data(struct mtd_info *mtd, u_char *dat,
		      u_char *read_ecc, u_char *calc_ecc);
