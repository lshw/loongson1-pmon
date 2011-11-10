/*	$Id: powerpc.h,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */

/*
 * Copyright (c) 2001 ipUnplugged AB (www.ipunplugged.com)
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for Patrik Lindergren, by
 *	ipUnplugged AB, Sweden.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */


/*
 * PowerPC Special Registers
 */
#define	CTR	9
#define SPRG0	272
#define SPRG1	273
#define SPRG2	274
#define SPRG3	275
#define PVR	287
#define	HID0	1008
#define	HID1	1009
#define	L2CR	1017


/*
 * Hardware Implementation-Dependent Register 0 (HID0)
 */
#define HID0_NMCP	0x80000000	/* Enable MCP */
#define HID0_EBA	0x20000000	/* Enable/Disable system bus address parity check */
#define HID0_EBD	0x10000000	/* Enable/Disable system bus data parity check */
#define HID0_BCLK	0x08000000	/* CLK_OUT output enable aned clock selection */
#define HID0_ECLK	0x02000000	/* CLK_OUT output enable aned clock selection */
#define HID0_PAR	0x01000000	/* Disable precharge of ARTRY* amd SHD[0] or SHD[1] */
#define HID0_DOZE	0x00800000	/* Doze Mode Enable */
#define HID0_NAP	0x00400000	/* NAP Mode enable */
#define HID0_SLEEP	0x00200000	/* Sleep Mode Enable */
#define HID0_DPM	0x00100000	/* Dynamic Power Management enable */
#define HID0_EIEC	0x00040000	/* Enable internal error checking */
#define HID0_NHR	0x00010000	/* Not hard reset */
#define HID0_ICE	0x00008000	/* Instruction Cache Enable */
#define HID0_DCE	0x00004000	/* Data Cache enable */
#define HID0_ILOCK	0x00002000	/* Instruction cache Lock */
#define HID0_DLOCK	0x00001000	/* Data cache Lock*/
#define HID0_ICFI	0x00000800	/* Instruction cache invalidate */
#define HID0_DCFI	0x00000400	/* Data cache flash invalidate */
#define HID0_SPD	0x00000200	/* Speculative data and instruction cache */
#define HID0_IFTT	0x00000100	/* I-Fetch TTx encoding differentiation */
#define HID0_SGE	0x00000080	/* Store gathering enable */
#define HID0_DCFA	0x00000040	/* Data Cache Flush assist */
#define HID0_BTIC	0x00000020	/* Branch Target Instruction Cache enable */
#define HID0_BHTE	0x00000004	/* Branch History Table enable */
#define HID0_NOPDST	0x00000002	/* No-Op dst, dstt, dstst and dststt instructions */
#define HID0_NOPTI	0x00000001	/* No-Op the data cache touch instructions */

/* Defines for MPC740/750/7400 L2-Cache register L2CR */

#define L2CR_L2E           (0x80000000)  /* Enable cache    */
#define L2CR_L2PE          (0x40000000)  /* Parity enable   */
#define L2CR_L2SIZ_256     (0x10000000)  /* Cache of 256KB  */ 
#define L2CR_L2SIZ_512     (0x20000000)  /* Cache of 512KB */ 
#define L2CR_L2SIZ_1024    (0x30000000)  /* Cache of 1024KB */ 
#define L2CR_L2SIZ_2048    (0x00000000)  /* Cache of 2048KB */ 
#define L2CR_L2CLK_DIS     (0x00000000)  /* Disable DLL+CLK */
#define L2CR_L2CLK_1       (0x02000000)  /* Div by 1   */
#define L2CR_L2CLK_15      (0x04000000)  /* Div by 1.5 */
#define L2CR_L2CLK_35      (0x06000000)  /* Div by 3.5 */
#define L2CR_L2CLK_2       (0x08000000)  /* Div by 2   */
#define L2CR_L2CLK_25      (0x0A000000)  /* Div by 2.5 */
#define L2CR_L2CLK_3       (0x0C000000)  /* Div by 3   */
#define L2CR_L2CLK_4       (0x0E000000)  /* Div by 4   */
#define L2CR_L2CLK_SPEED_MSK (0x10)      /* Cache speed mask */
#define L2CR_L2RAM_FLOW    (0x00000000)  /* flow sync burst RAM*/
#define L2CR_L2RAM_RES     (0x00800000)  /* reserved   */
#define L2CR_L2RAM_PIPE    (0x01000000)  /* pipl sync burst RAM*/
#define L2CR_L2RAM_LATEW   (0x01800000)  /* late write sync RAM*/
#define L2CR_L2DO          (0x00400000)  /* Enable Data only caching*/
#define L2CR_L2I           (0x00200000)  /* Enable Cache Invalidation*/
#define L2CR_L2CTL         (0x00100000)  /* Enable low power mode ZZ */
#define L2CR_L2WT          (0x00080000)  /* Enable write-thru mode */
#define L2CR_L2TS          (0x00040000)  /* Enable test mode */
#define L2CR_L2OH_05       (0x00000000)  /* Output hold of 0.5ns */
#define L2CR_L2OH_10       (0x00010000)  /* Output hold of 1.0ns */
#define L2CR_L2OH_MOUT     (0x00020000)  /* more output hold */
#define L2CR_L2OH_EMOUT    (0x00030000)  /* even more output hold */
#define L2CR_L2SL          (0x00008000)  /* Enable DLL slow for L2-clock < 100MHz*/
#define L2CR_L2DF          (0x00004000)  /* Enable differential clock pins */
#define L2CR_L2BYP         (0x00002000)  /* Enable DLL bypass */
#define L2CR_L2FA          (0x00001000)  /* Flush Assist bit */
#define L2CR_L2HWF         (0x00000800)  /* Start HW flusg operation of L2 Cache */
#define L2CR_L2IO          (0x00000400)  /* L2 Instruction cache only enable */
#define L2CR_L2CLKSTP      (0x00000200)  /* Enable clock stop to L2 cache rams */
#define L2CR_L2DRO         (0x00000100)  /* Roll-over enable for DLL */
#define L2CR_L2CTR         (0x000000FE)  /* Mask for L2CR_L2CTR DLL 127..0 counter */
#define L2CR_L2IP          (0x00000001)  /* RO, Indicate invalidate in progress */

/*
 * Machine State Register (MSR)
 */
#define PPC_MSR_FP	0x00002000

#define GETHID1(x) \
	 __asm __volatile("mfspr %0, 1009" : "=r"(x));

#ifdef __ASSEMBLER__
#define HIADJ(x)	(x)@ha
#define HI(x)		(x)@h
#define LO(x)		(x)@l

/*
 *  Use this macro to prevent reordering by as/ld and processor
 */
#define	IORDER		eieio; sync

/*
 *  Macros used to setup BAT regs.
 */
#define IBAT_SETUP(batno, batuval, batlval)     \
        lis 3, HIADJ(batuval); addi 3, 3, LO(batuval);  \
        lis 4, HIADJ(batlval); addi 4, 4, LO(batlval);  \
        mtibatu batno, 3; mtibatl batno, 4

#define DBAT_SETUP(batno, batuval, batlval)     \
        lis 3, HIADJ(batuval); addi 3, 3, LO(batuval);  \
        lis 4, HIADJ(batlval); addi 4, 4, LO(batlval);  \
        mtdbatu batno, 3; mtdbatl batno, 4

#endif /* __ASSEMBLER__ */

