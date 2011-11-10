/* $Id: disassemble.c,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2001-2002 Opsycon AB  (www.opsycon.se / www.opsycon.com)
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
 *	This product includes software developed by Opsycon AB, Sweden.
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
#undef FLOATINGPT
#include <stdio.h>
#include <termio.h>
#ifdef _KERNEL
#undef _KERNEL
#include <sys/ioctl.h>
#define _KERNEL
#else
#include <sys/ioctl.h>
#endif
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <machine/frame.h>
#include "pmon.h"

/* regsize states */
#define REGSZ_32        0
#define REGSZ_64        1

typedef enum {
	RD_RS_RT, RT_RS_IMM, OFF, RS_RT_OFF, RS_OFF,
	NONE, RT_RD, COFUN, RD_RT,
	RS_RT, TARGET, JALR, RS, RD_RT_SFT, LOAD_STORE,
	RT_IMM, RD, RD_RT_RS, RD_RS,
	RT_RS_SIMM, RS_SIMM, RT_SIMM, JR, RT_C0, RT_C1,
	RT_CN, RT_CC1, LDSTC0, LDSTC1,
	LDSTCN, WORD, RT_C2, RT_CC2, BPCODE, CACHE_OP,
	CP_OFF, STORE, STOREC1, STORECN,
	FT_FS_FD_D, FT_FS_FD_S, FS_FD_D, FS_FD_S, FS_FD_W,
	FS_FD_L, FT_FS_D, FT_FS_S,
	RT_RD_TO, RT_CC1_TO, RT_CN_TO, RT_C0_TO, RT_C1_TO
} TYPE;

typedef struct {
    const char     *str;
    long            mask, code;
    TYPE            type;
} DISTBL;

#define	REGREG	((register_t *)cpuinfotab[whatcpu])

#define FS_(x)          (((x) >> 11) & ((1L <<  5) - 1))
#define FT_(x)          (((x) >> 16) & ((1L <<  5) - 1))
#define FD_(x)          (((x) >>  6) & ((1L <<  5) - 1))
#define RS_(x)          (((x) >> 21) & ((1L <<  5) - 1))
#define RT_(x)          (((x) >> 16) & ((1L <<  5) - 1))
#define RD_(x)          (((x) >> 11) & ((1L <<  5) - 1))
#define IMM_(x)         (((x) >>  0) & ((1L << 16) - 1))
#define TARGET_(x)      (((x) >>  0) & ((1L << 26) - 1))
#define SHAMT_(x)       (((x) >>  6) & ((1L <<  5) - 1)) 


#define comma()		strcat(dest,",")
#define rd()		strcat(dest,regname[(int)RD_(inst)])
#define rs()		strcat(dest,regname[(int)RS_(inst)]), rsvalue = REGREG[(int)RS_(inst)]
#define rt()		strcat(dest,regname[(int)RT_(inst)]), rtvalue = REGREG[(int)RT_(inst)]
#define fd()		strcat(dest,c1reg[(int)FD_(inst)])
#define fs()		strcat(dest,c1reg[(int)FS_(inst)])
#define ft()		strcat(dest,c1reg[(int)FT_(inst)])
#define c0()		strcat(dest,c0reg[(int)RD_(inst)])
#define c1()		strcat(dest,c1reg[(int)RD_(inst)])
#define c2()		strcat(dest,regs_hw[(int)RD_(inst)])
#define cn()		strcat(dest,regs_hw[(int)RD_(inst)])
#define c0ft()		strcat(dest,c0reg[(int)RT_(inst)])
#define c1ft()		strcat(dest,c1reg[(int)RT_(inst)])
#define cnft()		strcat(dest,regs_hw[(int)RT_(inst)])
#define cc1()		strcat(dest,regs_hw[(int)RD_(inst)])
#define cc2()		strcat(dest,regs_hw[(int)RD_(inst)])

const DISTBL *get_distbl __P((u_int32_t));
void simm __P((char *));
void imm __P((char *));
void mkcomment __P((char *, char *, long));
int dispchist __P((int, int));

long	inst, cur_loc;
static char *searching = "searching..  ";

const char         * const *regname;	/* pointer to either regs_sw or regs_hw */

/* software register names */
const char           * const regs_sw[] =
{
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "s8", "ra"
};

/* hardware register names */
const char           * const regs_hw[] =
{
    "$0", "$1", "$2", "$3", "$4", "$5", "$6", "$7",
    "$8", "$9", "$10", "$11", "$12", "$13", "$14", "$15",
    "$16", "$17", "$18", "$19", "$20", "$21", "$22", "$23",
    "$24", "$25", "$26", "$27", "$28", "$29", "$30", "$31"
};

const char * const *  c0reg;		/* pointer to either regs_c0 or c1reg */

const char           * const regs_c0[] =
{
    "C0_INX", "C0_RAND", "C0_TLBLO0", "C0_TLBLO1",
    "C0_CTEXT", "C0_PGMASK", "C0_WIRED", "$7",
    "C0_BADADDR", "C0_COUNT", "C0_TLBHI", "C0_COMPARE",
    "C0_SR", "C0_CAUSE", "C0_EPC", "C0_PRID",
    "C0_CONFIG", "C0_LLADDR", "C0_WATCHLO", "C0_WATCHHI", 
    "$20", "$21", "$22", "$23", "$24", "$25", 
    "C0_ECC", "C0_CACHERR", "C0_TAGLO", "C0_TAGHI", "C0_ERRPC", "$31"
};

const char           * const c1reg[] =
{
    "$f0", "$f1", "$f2", "$f3", "$f4", "$f5", "$f6", "$f7",
    "$f8", "$f9", "$f10", "$f11", "$f12", "$f13", "$f14", "$f15",
    "$f16", "$f17", "$f18", "$f19", "$f20", "$f21", "$f22", "$f23",
    "$f24", "$f25", "$f26", "$f27", "$f28", "$f29", "$f30", "$f31"
};

const DISTBL distbl[] =
{
    /* special aliases for certain instructions */
    {"nop", 0xffffffffL, 0x00000000L, NONE},		/* sll 0,0,0 */
    {"li", 0xffe00000L, 0x24000000L, RT_SIMM},		/* addiu rd,0,simm */
    {"li", 0xffe00000L, 0x34000000L, RT_IMM},		/* ori rd,0,imm*/
    {"move", 0xfc1f07ffL, 0x00000021L, RD_RS}, 		/* addu rd,0,rs */
    {"move", 0xfc1f07ffL, 0x00000025L, RD_RS}, 		/* or rd,0,rs */
    {"neg", 0xffe007ffL, 0x00000022L, RD_RT}, 		/* sub rd,rt,0 */
    {"negu", 0xffe007ffL, 0x00000023L, RD_RT}, 		/* subu rd,rt,0 */
    {"dmove", 0xfc1f07ffL, 0x0000002dL, RD_RS}, 	/* daddu rd,0,rs */
    {"dneg", 0xffe007ffL, 0x0000002eL, RD_RT}, 		/* dsub rd,rt,0 */
    {"dnegu", 0xffe007ffL, 0x0000002fL, RD_RT}, 	/* dsubu rd,rt,0 */
    {"not", 0xfc1f07ffL, 0x00000027L, RD_RS}, 		/* nor rd,0,rs */
    {"b", 0xffff0000L, 0x10000000L, OFF},		/* beq 0,0 */
    {"b", 0xffff0000L, 0x04010000L, OFF},		/* bgez 0 */
    {"bal", 0xffff0000L, 0x04110000L, OFF},		/* bgezal 0 */

    /* machine insns */
    {"add", 0xfc0007ffL, 0x00000020L, RD_RS_RT},
    {"addi", 0xfc000000L, 0x20000000L, RT_RS_SIMM},
    {"addiu", 0xfc000000L, 0x24000000L, RT_RS_SIMM},
    {"addu", 0xfc0007ffL, 0x00000021L, RD_RS_RT},

    {"dadd", 0xfc0007ffL, 0x0000002cL, RD_RS_RT},
    {"daddi", 0xfc000000L, 0x60000000L, RT_RS_SIMM},
    {"daddiu", 0xfc000000L, 0x64000000L, RT_RS_SIMM},
    {"daddu", 0xfc0007ffL, 0x0000002dL, RD_RS_RT},

    {"and", 0xfc0007ffL, 0x00000024L, RD_RS_RT},
    {"andi", 0xfc000000L, 0x30000000L, RT_RS_IMM},

    {"bc0f", 0xffff0000L, 0x41000000L, CP_OFF},
    {"bc1f", 0xffff0000L, 0x45000000L, CP_OFF},
    {"bc2f", 0xffff0000L, 0x49000000L, CP_OFF},
    {"bc3f", 0xffff0000L, 0x4d000000L, CP_OFF},
    {"bc0t", 0xffff0000L, 0x41010000L, CP_OFF},
    {"bc1t", 0xffff0000L, 0x45010000L, CP_OFF},
    {"bc2t", 0xffff0000L, 0x49010000L, CP_OFF},
    {"bc3t", 0xffff0000L, 0x4d010000L, CP_OFF},

    {"bc0fl", 0xffff0000L, 0x41020000L, CP_OFF},
    {"bc1fl", 0xffff0000L, 0x45020000L, CP_OFF},
    {"bc2fl", 0xffff0000L, 0x49020000L, CP_OFF},
    {"bc3fl", 0xffff0000L, 0x4d020000L, CP_OFF},
    {"bc0tl", 0xffff0000L, 0x41030000L, CP_OFF},
    {"bc1tl", 0xffff0000L, 0x45030000L, CP_OFF},
    {"bc2tl", 0xffff0000L, 0x49030000L, CP_OFF},
    {"bc3tl", 0xffff0000L, 0x4d030000L, CP_OFF},

    {"beq",    0xfc000000L, 0x10000000L, RS_RT_OFF},
    {"bne",    0xfc000000L, 0x14000000L, RS_RT_OFF},
    {"blez",   0xfc1f0000L, 0x18000000L, RS_OFF},
    {"bgtz",   0xfc1f0000L, 0x1c000000L, RS_OFF},

    {"beql",   0xfc000000L, 0x50000000L, RS_RT_OFF},
    {"bnel",   0xfc000000L, 0x54000000L, RS_RT_OFF},
    {"blezl",  0xfc1f0000L, 0x58000000L, RS_OFF},
    {"bgtzl",  0xfc1f0000L, 0x5c000000L, RS_OFF},

    {"bltz",   0xfc1f0000L, 0x04000000L, RS_OFF},
    {"bgez",   0xfc1f0000L, 0x04010000L, RS_OFF},

    {"bltzl",  0xfc1f0000L, 0x04020000L, RS_OFF},
    {"bgezl",  0xfc1f0000L, 0x04030000L, RS_OFF},

    {"bltzal", 0xfc1f0000L, 0x04100000L, RS_OFF},
    {"bgezal", 0xfc1f0000L, 0x04110000L, RS_OFF},

    {"bltzall",0xfc1f0000L, 0x04120000L, RS_OFF},
    {"bgezall",0xfc1f0000L, 0x04130000L, RS_OFF},

    {"break", 0xfc00003fL, 0x0000000dL, BPCODE},

    {"cache", 0xfc000000L, 0xbc000000L, CACHE_OP},

    {"cfc0", 0xffe007ffL, 0x40400000L, RT_RD},
    {"cfc1", 0xffe007ffL, 0x44400000L, RT_CC1},
    {"cfc2", 0xffe007ffL, 0x48400000L, RT_CN},
    {"cfc3", 0xffe007ffL, 0x4c400000L, RT_CN},
    {"tlbp", 0xffffffffL, 0x42000008L, NONE},
    {"tlbr", 0xffffffffL, 0x42000001L, NONE},
    {"tlbwi", 0xffffffffL, 0x42000002L, NONE},
    {"tlbwr", 0xffffffffL, 0x42000006L, NONE},
    {"rfe", 0xffffffffL, 0x42000010L, NONE},
    {"eret", 0xffffffffL, 0x42000018L, NONE},
    {"cop0", 0xfe000000L, 0x42000000L, COFUN},

    {"add.s", 0xfee0003fL, 0x46000000L, FT_FS_FD_S},
    {"add.d", 0xfee0003fL, 0x46200000L, FT_FS_FD_D},
    {"sub.s", 0xfee0003fL, 0x46000001L, FT_FS_FD_S},
    {"sub.d", 0xfee0003fL, 0x46200001L, FT_FS_FD_D},
    {"mul.s", 0xfee0003fL, 0x46000002L, FT_FS_FD_S},
    {"mul.d", 0xfee0003fL, 0x46200002L, FT_FS_FD_D},
    {"div.s", 0xfee0003fL, 0x46000003L, FT_FS_FD_S},
    {"div.d", 0xfee0003fL, 0x46200003L, FT_FS_FD_D},
    {"abs.s", 0xfee0003fL, 0x46000005L, FS_FD_S},
    {"abs.d", 0xfee0003fL, 0x46200005L, FS_FD_D},
    {"mov.s", 0xfee0003fL, 0x46000006L, FS_FD_S},
    {"mov.d", 0xfee0003fL, 0x46200006L, FS_FD_D},
    {"neg.s", 0xfee0003fL, 0x46000007L, FS_FD_S},
    {"neg.d", 0xfee0003fL, 0x46200007L, FS_FD_D},
    {"sqrt.d", 0xfee0003fL, 0x46200004, FS_FD_D},
    {"sqrt.s", 0xfee0003fL, 0x46000004, FS_FD_S},

    {"c.f.s", 0xfee0003fL, 0x46000030L, FT_FS_S},
    {"c.f.d", 0xfee0003fL, 0x46200030L, FT_FS_D},
    {"c.un.s", 0xfee0003fL, 0x46000031L, FT_FS_S},
    {"c.un.d", 0xfee0003fL, 0x46200031L, FT_FS_D},
    {"c.eq.s", 0xfee0003fL, 0x46000032L, FT_FS_S},
    {"c.eq.d", 0xfee0003fL, 0x46200032L, FT_FS_D},
    {"c.ueq.s", 0xfee0003fL, 0x46000033L, FT_FS_S},
    {"c.ueq.d", 0xfee0003fL, 0x46200033L, FT_FS_D},
    {"c.olt.s", 0xfee0003fL, 0x46000034L, FT_FS_S},
    {"c.olt.d", 0xfee0003fL, 0x46200034L, FT_FS_D},
    {"c.ult.s", 0xfee0003fL, 0x46000035L, FT_FS_S},
    {"c.ult.d", 0xfee0003fL, 0x46200035L, FT_FS_D},
    {"c.ole.s", 0xfee0003fL, 0x46000036L, FT_FS_S},
    {"c.ole.d", 0xfee0003fL, 0x46200036L, FT_FS_D},
    {"c.ule.s", 0xfee0003fL, 0x46000037L, FT_FS_S},
    {"c.ule.d", 0xfee0003fL, 0x46200037L, FT_FS_D},
    {"c.sf.s", 0xfee0003fL, 0x46000038L, FT_FS_S},
    {"c.sf.d", 0xfee0003fL, 0x46200038L, FT_FS_D},
    {"c.ngle.s", 0xfee0003fL, 0x46000039L, FT_FS_S},
    {"c.ngle.d", 0xfee0003fL, 0x46200039L, FT_FS_D},
    {"c.seq.s", 0xfee0003fL, 0x4600003aL, FT_FS_S},
    {"c.seq.d", 0xfee0003fL, 0x4620003aL, FT_FS_D},
    {"c.ngl.s", 0xfee0003fL, 0x4600003bL, FT_FS_S},
    {"c.ngl.d", 0xfee0003fL, 0x4620003bL, FT_FS_D},
    {"c.lt.s", 0xfee0003fL, 0x4600003cL, FT_FS_S},
    {"c.lt.d", 0xfee0003fL, 0x4620003cL, FT_FS_D},
    {"c.nge.s", 0xfee0003fL, 0x4600003dL, FT_FS_S},
    {"c.nge.d", 0xfee0003fL, 0x4620003dL, FT_FS_D},
    {"c.le.s", 0xfee0003fL, 0x4600003eL, FT_FS_S},
    {"c.le.d", 0xfee0003fL, 0x4620003eL, FT_FS_D},
    {"c.ngt.s", 0xfee0003fL, 0x4600003fL, FT_FS_S},
    {"c.ngt.d", 0xfee0003fL, 0x4620003fL, FT_FS_D},

    {"cvt.s.w", 0xfee0003fL, 0x46800020L, FS_FD_W},
    {"cvt.s.l", 0xfee0003fL, 0x46a00020L, FS_FD_L},
    {"cvt.s.d", 0xfee0003fL, 0x46200020L, FS_FD_D},
    {"cvt.d.s", 0xfee0003fL, 0x46000021L, FS_FD_S},
    {"cvt.d.w", 0xfee0003fL, 0x46800021L, FS_FD_W},
    {"cvt.d.l", 0xfee0003fL, 0x46a00021L, FS_FD_L},
    {"cvt.w.d", 0xfee0003fL, 0x46200024L, FS_FD_D},
    {"cvt.w.s", 0xfee0003fL, 0x46000024L, FS_FD_S},
    {"cvt.l.d", 0xfee0003fL, 0x46200025L, FS_FD_D},
    {"cvt.l.s", 0xfee0003fL, 0x46000025L, FS_FD_S},

    {"ceil.l.d", 0xffff003fL, 0x4620000aL, FS_FD_L},
    {"ceil.l.s", 0xffff003fL, 0x4600000aL, FS_FD_L},
    {"ceil.w.d", 0xffff003fL, 0x4620000eL, FS_FD_W},
    {"ceil.w.s", 0xffff003fL, 0x4600000eL, FS_FD_W},
    {"floor.l.d", 0xffff003fL, 0x4620000bL, FS_FD_L},
    {"floor.l.s", 0xffff003fL, 0x4600000bL, FS_FD_L},
    {"floor.w.d", 0xffff003fL, 0x4620000fL, FS_FD_W},
    {"floor.w.s", 0xffff003fL, 0x4600000fL, FS_FD_W},
    {"round.l.d", 0xffff003fL, 0x46200008L, FS_FD_L},
    {"round.l.s", 0xffff003fL, 0x46000008L, FS_FD_L},
    {"round.w.d", 0xffff003fL, 0x4620000cL, FS_FD_W},
    {"round.w.s", 0xffff003fL, 0x4600000cL, FS_FD_W},
    {"trunc.l.d", 0xffff003fL, 0x46200009L, FS_FD_L},
    {"trunc.l.s", 0xffff003fL, 0x46000009L, FS_FD_L},
    {"trunc.w.d", 0xffff003fL, 0x4620000dL, FS_FD_W},
    {"trunc.w.s", 0xffff003fL, 0x4600000dL, FS_FD_W},

    {"cop1", 0xfe000000L, 0x46000000L, COFUN},

    {"cop2", 0xfe000000L, 0x4a000000L, COFUN},
    {"cop3", 0xfe000000L, 0x4e000000L, COFUN},

    {"ctc0", 0xffe007ffL, 0x40c00000L, RT_RD_TO},
    {"ctc1", 0xffe007ffL, 0x44c00000L, RT_CC1_TO},
    {"ctc2", 0xffe007ffL, 0x48c00000L, RT_CN_TO},
    {"ctc3", 0xffe007ffL, 0x4cc00000L, RT_CN_TO},

    {"div", 0xfc00ffffL, 0x0000001aL, RS_RT},
    {"divu", 0xfc00ffffL, 0x0000001bL, RS_RT},
    {"ddiv", 0xfc00ffffL, 0x0000001eL, RS_RT},
    {"ddivu", 0xfc00ffffL, 0x0000001fL, RS_RT},

    {"j", 0xfc000000L, 0x08000000L, TARGET},
    {"jal", 0xfc000000L, 0x0c000000L, TARGET},
    {"jalr", 0xfc1f07ffL, 0x00000009L, JALR},
    {"jr", 0xfc1fffffL, 0x00000008L, JR},

    {"lui", 0xfc000000L, 0x3c000000L, RT_IMM},

    {"lb", 0xfc000000L, 0x80000000L, LOAD_STORE},
    {"lbu", 0xfc000000L, 0x90000000L, LOAD_STORE},
    {"lh", 0xfc000000L, 0x84000000L, LOAD_STORE},
    {"lhu", 0xfc000000L, 0x94000000L, LOAD_STORE},
    {"lw", 0xfc000000L, 0x8c000000L, LOAD_STORE},
    {"lwl", 0xfc000000L, 0x88000000L, LOAD_STORE},
    {"lwr", 0xfc000000L, 0x98000000L, LOAD_STORE},
    {"lwu", 0xfc000000L, 0x9c000000L, LOAD_STORE},
    {"ldl", 0xfc000000L, 0x68000000L, LOAD_STORE},
    {"ldr", 0xfc000000L, 0x6c000000L, LOAD_STORE},

    {"ll", 0xfc000000L, 0xc0000000L, LOAD_STORE},
    {"lwc1", 0xfc000000L, 0xc4000000L, LDSTC1},
    {"lwc2", 0xfc000000L, 0xc8000000L, LDSTCN},
    {"lwc3", 0xfc000000L, 0xcc000000L, LDSTCN},

    {"lld", 0xfc000000L, 0xd0000000L, LOAD_STORE},
    {"ldc1", 0xfc000000L, 0xd4000000L, LDSTC1},
    {"ldc2", 0xfc000000L, 0xd8000000L, LDSTCN},
    {"ld", 0xfc000000L, 0xdc000000L, LOAD_STORE},
    
    {"mfc0", 0xffe007ffL, 0x40000000L, RT_C0},
    {"mfc1", 0xffe007ffL, 0x44000000L, RT_C1},
    {"mfc2", 0xffe007ffL, 0x48000000L, RT_CN},
    {"mfc3", 0xffe007ffL, 0x4c000000L, RT_CN},

    {"dmfc0", 0xffe007ffL, 0x40200000L, RT_C0},
    {"dmfc1", 0xffe007ffL, 0x44200000L, RT_C1},
    {"dmfc2", 0xffe007ffL, 0x48200000L, RT_CN},
    {"dmfc3", 0xffe007ffL, 0x4c200000L, RT_CN},

    {"mtc0", 0xffe007ffL, 0x40800000L, RT_C0_TO},
    {"mtc1", 0xffe007ffL, 0x44800000L, RT_C1_TO},
    {"mtc2", 0xffe007ffL, 0x48800000L, RT_CN_TO},
    {"mtc3", 0xffe007ffL, 0x4c800000L, RT_CN_TO},

    {"dmtc0", 0xffe007ffL, 0x40a00000L, RT_C0_TO},
    {"dmtc1", 0xffe007ffL, 0x44a00000L, RT_C1_TO},
    {"dmtc2", 0xffe007ffL, 0x48a00000L, RT_CN_TO},
    {"dmtc3", 0xffe007ffL, 0x4ca00000L, RT_CN_TO},

    {"mfhi", 0xffff07ffL, 0x00000010L, RD},
    {"mflo", 0xffff07ffL, 0x00000012L, RD},
    {"mthi", 0xfc1fffffL, 0x00000011L, RS},
    {"mtlo", 0xfc1fffffL, 0x00000013L, RS},

    {"mult", 0xfc00ffffL, 0x00000018L, RS_RT},
    {"multu", 0xfc00ffffL, 0x00000019L, RS_RT},
    {"dmult", 0xfc00ffffL, 0x0000001cL, RS_RT},
    {"dmultu", 0xfc00ffffL, 0x0000001dL, RS_RT},

    /* R4100: */
    {"madd16",  0xfc00ffff, 0x00000028, RS_RT},
    {"dmadd16",  0xfc00ffff, 0x00000020, RS_RT},
    {"standby", ~0, 0x42000021, NONE},
    {"suspend", ~0, 0x42000022, NONE},
    {"hibernate", ~0, 0x42000023, NONE},

    {"nor", 0xfc0007ffL, 0x00000027L, RD_RS_RT},
    {"or", 0xfc0007ffL, 0x00000025L, RD_RS_RT},
    {"ori", 0xfc000000L, 0x34000000L, RT_RS_IMM},

    {"sb", 0xfc000000L, 0xa0000000L, STORE},
    {"sh", 0xfc000000L, 0xa4000000L, STORE},
    {"swl", 0xfc000000L, 0xa8000000L, STORE},
    {"sw", 0xfc000000L, 0xac000000L, STORE},
    {"sdl", 0xfc000000L, 0xb0000000L, STORE},
    {"sdr", 0xfc000000L, 0xb4000000L, STORE},
    {"swr", 0xfc000000L, 0xb8000000L, STORE},

    {"sc", 0xfc000000L, 0xe0000000L, STORE},
    {"swc1", 0xfc000000L, 0xe4000000L, STOREC1},
    {"swc2", 0xfc000000L, 0xe8000000L, STORECN},
    {"swc3", 0xfc000000L, 0xec000000L, STORECN},

    {"scd", 0xfc000000L, 0xf0000000L, STORE},
    {"sdc1", 0xfc000000L, 0xf4000000L, STOREC1},
    {"sdc2", 0xfc000000L, 0xf8000000L, STORECN},
    {"sd", 0xfc000000L, 0xfc000000L, STORE},

    {"sll", 0xffe0003fL, 0x00000000L, RD_RT_SFT},
    {"sllv", 0xfc0007ffL, 0x00000004L, RD_RT_RS},
    {"dsll", 0xffe0003fL, 0x00000038L, RD_RT_SFT},
    {"dsllv", 0xfc0007ffL, 0x00000014L, RD_RT_RS},
    {"dsll32", 0xfc0007ffL, 0x0000003cL, RD_RT_RS},

    {"slt", 0xfc0007ffL, 0x0000002aL, RD_RS_RT},
    {"slti", 0xfc000000L, 0x28000000L, RT_RS_SIMM},
    {"sltiu", 0xfc000000L, 0x2c000000L, RT_RS_SIMM},
    {"sltu", 0xfc0007ffL, 0x0000002bL, RD_RS_RT},

    {"sra", 0xffe0003fL, 0x00000003L, RD_RT_SFT},
    {"srav", 0xfc0007ffL, 0x00000007L, RD_RT_RS},
    {"dsra", 0xffe0003fL, 0x0000003bL, RD_RT_SFT},
    {"dsrav", 0xfc0007ffL, 0x00000017L, RD_RT_RS},
    {"dsra32", 0xfc0007ffL, 0x0000003fL, RD_RT_RS},

    {"srl", 0xffe0003fL, 0x00000002L, RD_RT_SFT},
    {"srlv", 0xfc0007ffL, 0x00000006L, RD_RT_RS},
    {"dsrl", 0xffe0003fL, 0x0000003aL, RD_RT_SFT},
    {"dsrlv", 0xfc0007ffL, 0x00000016L, RD_RT_RS},
    {"dsrl32", 0xfc0007ffL, 0x0000003eL, RD_RT_RS},

    {"sub", 0xfc0007ffL, 0x00000022L, RD_RS_RT},
    {"subu", 0xfc0007ffL, 0x00000023L, RD_RS_RT},
    {"dsub", 0xfc0007ffL, 0x0000002eL, RD_RS_RT},
    {"dsubu", 0xfc0007ffL, 0x0000002fL, RD_RS_RT},

    {"teqi", 0xfc1f0000L, 0x040c0000L, RS_SIMM},
    {"teq", 0xfc00003fL, 0x00000034L, RS_RT},
    {"tgei", 0xfc1f0000L, 0x04080000L, RS_SIMM},
    {"tge", 0xfc00003fL, 0x00000030L, RS_RT},
    {"tgeiu", 0xfc1f0000L, 0x04090000L, RS_SIMM},
    {"tgeu", 0xfc00003fL, 0x00000031L, RS_RT},
    {"tlti", 0xfc1f0000L, 0x040a0000L, RS_SIMM},
    {"tlt", 0xfc00003fL, 0x00000032L, RS_RT},
    {"tltiu", 0xfc1f0000L, 0x040b0000L, RS_SIMM},
    {"tltu", 0xfc00003fL, 0x00000033L, RS_RT},
    {"tnei", 0xfc1f0000L, 0x040e0000L, RS_SIMM},
    {"tne", 0xfc00003fL, 0x00000036L, RS_RT},

    {"sync", 0xffffffffL, 0x0000000fL, NONE},
    {"syscall", 0xffffffffL, 0x0000000cL, NONE},
    {"xor", 0xfc0007ffL, 0x00000026L, RD_RS_RT},
    {"xori", 0xfc000000L, 0x38000000L, RT_RS_IMM},

	/* must be last !! never be move/remove */
    {".word", 0x00000000L, 0x00000000L, WORD}
};

int 
md_is_branch(adr)
     void *adr;
{
    const DISTBL   *pt;
    u_int32_t inst;

    inst = load_word (adr);
    pt = get_distbl (inst);
    switch (pt->type) {
    case OFF:
    case RS_RT_OFF:
    case RS_OFF:
    case CP_OFF:
    case TARGET:
    case JALR:
    case JR:
	return (1);
    default:
	return (0);
    }
}

int 
md_is_cond_branch(adr)
     void *adr;
{
    const DISTBL *pt;
    u_int32_t inst;

    inst = load_word (adr);
    pt = get_distbl (inst);
    switch (pt->type) {
    case RS_RT_OFF:
    case RS_OFF:
    case CP_OFF:
	return (1);
    default:
	return (0);
    }
}

int
md_is_jr (adr)
     void *adr;
{
    const DISTBL   *pt;
    u_int32_t inst;

    inst = load_word (adr);
    pt = get_distbl (inst);
    return (pt->type == JR);
}

void *
md_branch_target(adr)
     void *adr;
{
    const DISTBL *pt;
    int32_t val;
    u_int32_t inst;

    inst = load_word (adr);
    pt = get_distbl (inst);
    switch (pt->type) {
    case OFF:
    case RS_RT_OFF:
    case RS_OFF:
    case CP_OFF:
	val = inst & 0xffff;
	if (val & 0x8000)
	    val |= 0xffff0000;
	return(adr + 4 + (val << 2));
    case TARGET:
	val = inst & 0x3ffffff;
	return ((void *)(((int)(adr + 4) & 0xf0000000) | (val << 2)));
    case JALR:
    case JR:
	val = RS_ (inst);
	return ((void *)(int)REGREG[val]);
    default:
	return (0);
    }
}

int
md_is_call (adr)
     void *adr;
{
    u_int32_t inst;

    inst = load_word(adr);
    switch (getfield(inst, 6, 26)) {
    case 0:
	/* special: jalr */
	return (getfield (inst, 6, 0) == 9);
    case 1:
	/* regimm: bal */
	return (getfield (inst, 2, 19) == 2);
    case 3:
	/* jal */
	return (1);
    }
    return (0);
}

const DISTBL *
get_distbl (bits)
     u_int32_t bits;
{
    const DISTBL *pt = distbl;
    static const DISTBL *lastpt = 0;
    static u_int32_t lastbits;

    /* simple cache for repeated lookups */
    if (lastpt && bits == lastbits)
      return lastpt;

    while ((bits & pt->mask) != pt->code)
	++pt;
    lastpt = pt;
    lastbits = bits;
    return (pt);
}

int
md_is_writeable (adr)
     void *adr;
{
	u_int32_t x;

	x = load_word (adr);
	store_word (adr, ~x);
	flush_cache(DCACHE, adr);
	if (load_word (adr) != ~x)
		return (0);
	store_word (adr, x);
	return (1);
}


const Optdesc         l_opts[] =
{
    {"-b", "list only branches"},
    {"-c", "list only calls"},
    {"-t", "list trace buffer"},
    {"-r", "show register values with trace"},
    {0}};

/*************************************************************
 *  dis(ac,av)
 *      the 'l' (disassemble) command
 */
int rflag;			/* Wanting effective addresses for load/store instructions */
int rsvalue;			/* Computed by rs() macro for displaying load/store effective addrs */
int rtvalue;
int do_rt;
int do_rs;

int
md_disassemble(ac, av)
     int             ac;
     char           *av[];
{
	void *adr;
	int siz, l;
	int             bflag, cflag, i, j, n, tflag;
static void *last_adr;	/* For repeat of l command */
	void *prev_adr;	/* For figuring print of a0-a3 after jal type inst. */

    rflag = bflag = cflag = n = tflag = 0;

    siz = moresz;
    adr = md_get_excpc;

    for (i = 1; i < ac; i++) {
	if (av[i][0] == '-') {
	    for (j = 1; av[i][j] != 0; j++) {
		switch (av[i][j]) {
		case 'b':
		    bflag = 1;
		    break;
		case 'c':
		    cflag = 1;
		    break;
		case 't':
		    tflag = 1;
		    n++;
		    break;
		case 'r':
		    rflag = 1;
		    break;
		default:
		    printf ("%c: unknown option\n", av[i][j]);
		    return (-1);
		}
	    }
	} else {
	    switch (n) {
	    case 0:
		if (!get_rsa ((u_int32_t *)&adr, av[i]))
		    return (-1);
		break;
	    case 1:
		if (!get_rsa (&siz, av[i]))
		    return (-1);
		break;
	    default:
		printf ("%s: unknown option\n", av[i]);
		return (-1);
	    }
	    n++;
	}
    }

    if (repeating_cmd )
      adr = last_adr - 4;
    
    if (matchenv ("regstyle")) {
	regname = regs_sw;
	c0reg = regs_c0;
    } else {
	regname = regs_hw;
	c0reg = regs_hw;
    }

    ioctl (STDIN, CBREAK, NULL);

    if (tflag) {
	dispchist (n, siz);
	rflag = 0;
	return (0);
    }

#if 0
    if (adr < K0BASE || adr >= K2BASE || ((int)adr & 3)) {
	printf ("%08x: illegal instruction address\n", adr);
	rflag = 0;
	return (1);
    }
#endif

    if (n > 1 && siz == 1 && md_is_branch (adr))
      siz = 2;			/* include branch delay slot */
    l = siz;

    if (cflag || bflag)
	printf ("%s", searching);
    while (1) {
	/* Enable this if you want a 'label' at the start of each
	 * procedure.
	 * if (adr2sym(prnbuf,adr)) {
	 * strcat(prnbuf,":");
	 * if (more(prnbuf,&l,(n>1)?0:siz)) break;
	 * }
	 */
	if (cflag || bflag) {
	    int match;
	    char *p;
	    if (cflag)
	      match = md_is_call (adr);
	    else
	      match = md_is_branch (adr);
	    if (!match) {
		dotik (128, 0);
		adr += 4L;
		continue;
	    }
	    p = searching;
	    while(*p++)
		printf("\b \b");
	}
	prev_adr = adr;
	adr = md_disasm (prnbuf, adr);
	last_adr = adr;
	if (more (prnbuf, &l, (n > 1) ? 0 : siz))
	    break;
	if (rflag && ( md_is_call( prev_adr ) ) )
	{
	    /*
	     * We're showing registers and we just displayed
	     * a JAL type instruction.  Show a0-a3
	     */

	    printf( "\t\t\t# a0=0x%x a1=0x%x a2=0x%x a3=0x%x\n",
		cpuinfotab[whatcpu]->a0, cpuinfotab[whatcpu]->a1,
		cpuinfotab[whatcpu]->a2, cpuinfotab[whatcpu]->a3);
	}
	
	if (cflag || bflag)
	    printf ("%s", searching);
    }
    rflag = 0;
    return (0);
}

/**************************************************************/
void * 
md_disasm (dest, addr)
     char *dest;
     void *addr;
{
    const DISTBL   *pt;
    char           tmp[40];
    long            v, v1, w;
    int             i;
#ifdef FLOATINGPT
    float *s_fs;		/* For getting at FS argument via float */
    float *s_ft;		/* For getting at FT argument via float */
    double *d_fs;		/* For getting at FS argument via double */
    double *d_ft;		/* For getting at FT argument via double */
    int *w_fs;			/* For getting at FS argument via binary fixed single */
    long long *l_fs;		/* For getting at FS argument via binary fixed long */
    int fpdis;			/* Actually show the floating point values switch */
#endif /* FLOATINGPT */

    inst = load_word(addr);;
    if (regname == 0)
	regname = regs_sw;

    if (!adr2symoff (dest, (int)addr, 12))
	sprintf (dest, "%08x", addr);
    sprintf (tmp, " %08x ", inst);
    strcat (dest, tmp);

    pt = get_distbl (inst);
    i = strlen (pt->str);
    strcat (dest, pt->str);
    do_rt = 0;
    do_rs = 0;

#ifdef FLOATINGPT
    /*
     * It is possible the floating point values are bogus for printing
     * so give user a way to not show them.
     */

    fpdis = matchenv ("fpdis");
    
    /*
     * Get pointers to various ways of looking at floating point operands
     * as part of the rflag display rather than having do do these casts
     * over and over
     */

    d_fs = (double *)&Fpr[(int)FS_(inst)];
    d_ft = (double *)&Fpr[(int)FT_(inst)];
    l_fs = (long long *)d_fs;
    s_fs = (float *)(((int *)d_fs)+1);
    s_ft = (float *)(((int *)d_ft)+1);
    w_fs = (int *)s_fs;
#endif /* FLOATINGPT */

    while (i++ < 8)
	strcat (dest, " ");
    switch (pt->type) {
    case FT_FS_FD_D:
	fd (); comma (); fs (); comma (); ft ();
#ifdef FLOATINGPT
	if (rflag )
	{
	    mkcomment( dest, "#", 0);
	    if (fpdis )
	    {
		if (dpdenorm( (struct IEEEdp *)d_fs ) )
		    sprintf (tmp, " fs=0.0 (dp denorm)");
		else if (dpnan( (struct IEEEdp *)d_fs ) )
		    sprintf (tmp, " fs=Nan");
		else
		    sprintf (tmp, " fs=%e", *d_fs);
		strcat (dest, tmp);
		if (dpdenorm( (struct IEEEdp *)d_ft ) )
		    sprintf (tmp, " ft=0.0 (dp denorm)");
		else if (dpnan( (struct IEEEdp *)d_ft ) )
		    sprintf (tmp, " ft=Nan");
		else
		    sprintf (tmp, " ft=%e", *d_ft);
	    }
	    else
	      sprintf (tmp, " fs=%llx ft=%llx", *d_fs, *d_ft);
	    strcat (dest, tmp);
	}
#endif /* FLOATINGPT */
	break;
    case FT_FS_FD_S:
	fd (); comma (); fs (); comma (); ft ();
#ifdef FLOATINGPT
	if (rflag )
	{
	    mkcomment( dest, "#", 0);
	    if (fpdis )
	    {
		if (spdenorm( (struct IEEEsp *)s_fs ) )
		    sprintf (tmp, " fs=0.0 (sp denorm)");
		else if (spnan( (struct IEEEsp *)s_fs ) )
		    sprintf (tmp, " fs=Nan");
		else
		    sprintf (tmp, " fs=%e", *s_fs);
		strcat (dest, tmp);
		if (spdenorm( (struct IEEEsp *)s_ft ) )
		    sprintf (tmp, " ft=0.0 (sp denorm)");
		if (spnan( (struct IEEEsp *)s_ft ) )
		    sprintf (tmp, " ft=Nan");
		else
		    sprintf (tmp, " ft=%e", *s_ft);
	    }		
	    else
	      sprintf (tmp, " fs=0x%x ft=0x%x", *s_fs, *s_ft);
	    strcat (dest, tmp);
	}
#endif /* FLOATINGPT */
	break;
    case FS_FD_D:
	fd (); comma (); fs ();
#ifdef FLOATINGPT
	if (rflag )
	{
	    mkcomment( dest, "#", 0);
	    if (fpdis )
		if (dpdenorm( (struct IEEEdp *)d_fs ) )
		    sprintf (tmp, " fs=0.0 (dp denorm)");
		else if (dpnan( (struct IEEEdp *)d_fs ) )
		    sprintf (tmp, " fs=Nan");
		else
		    sprintf (tmp, " fs=%e", *d_fs);
	    else
	      sprintf (tmp, " fs=%llx", *d_fs);
	    strcat (dest, tmp);
	}
#endif  /* FLOATINGPT */
	break;
    case FS_FD_S:
	fd (); comma (); fs ();
#ifdef FLOATINGPT
	if (rflag )
	{
	    mkcomment( dest, "#", 0);
	    if (fpdis )
		if (spdenorm( (struct IEEEsp *)s_fs ) )
		    sprintf (tmp, " fs=0.0 (sp denorm)");
		else if (spnan( (struct IEEEsp *)s_fs ) )
		    sprintf (tmp, " fs=Nan");
		else
		    sprintf (tmp, " fs=%e", *s_fs);
	    else
	      sprintf (tmp, " fs=0x%x", *s_fs);
	    strcat (dest, tmp);
	}
#endif /* FLOATINGPT */
	break;
    case FS_FD_W:
	fd (); comma (); fs ();
#ifdef FLOATINGPT
	if (rflag )
	{
	    mkcomment( dest, "# ", 0);
	    sprintf (tmp, "fs=0x%x", *w_fs);
	    strcat (dest, tmp);
	}
#endif /* FLOATINGPT */
	break;
    case FS_FD_L:
	fd (); comma (); fs ();
#ifdef FLOATINGPT
	if (rflag )
	{
	    mkcomment( dest, "# ", 0);
	    sprintf (tmp, "fs=%llx", *l_fs);
	    strcat (dest, tmp);
	}
#endif /* FLOATINGPT */
	break;
    case FT_FS_D:
	fs (); comma (); ft ();
#ifdef FLOATINGPT
	if (rflag )
	{
	    mkcomment( dest, "#", 0);
	    if (fpdis )
	    {
		if (dpdenorm( (struct IEEEdp *)d_fs ) )
		    sprintf (tmp, " fs=0.0 (dp denorm)");
		else if (dpnan( (struct IEEEdp *)d_fs ) )
		    sprintf (tmp, " fs=Nan");
		else
		    sprintf (tmp, " fs=%e", *d_fs);
		strcat (dest, tmp);
		if (dpdenorm( (struct IEEEdp *)d_ft ) )
		    sprintf (tmp, " ft=0.0 (dp denorm)");
		else if (dpnan( (struct IEEEdp *)d_ft ) )
		    sprintf (tmp, " ft=Nan");
		else
		    sprintf (tmp, " ft=%e", *d_ft);
	    }
	    else
	      sprintf (tmp, " fs=%llx ft=%llx", *d_fs, *d_ft);
	    strcat (dest, tmp);
	}
#endif /* FLOATINGPT */
	break;
    case FT_FS_S:
	fs (); comma (); ft ();
#ifdef FLOATINGPT
	if (rflag )
	{
	    mkcomment( dest, "#", 0);
	    if (fpdis )
	    {
		if (spdenorm( (struct IEEEsp *)s_fs ) )
		    sprintf (tmp, " fs=0.0 (sp denorm)");
		if (spnan( (struct IEEEsp *)s_fs ) )
		    sprintf (tmp, " fs=Nan");
		else
		    sprintf (tmp, " fs=%e", *s_fs);
		strcat (dest, tmp);
		if (spdenorm( (struct IEEEsp *)s_ft ) )
		    sprintf (tmp, " ft=0.0 (sp denorm)");
		else if (spnan( (struct IEEEsp *)s_ft ) )
		    sprintf (tmp, " ft=Nan");
		else
		    sprintf (tmp, " ft=%e", *s_ft);
	    }		
	    else
	      sprintf (tmp, " fs=0x%x ft=0x%x", *s_fs, *s_ft);
	    strcat (dest, tmp);
	}
#endif /* FLOATINGPT */
	break;
    case RT_RS_IMM:
	rt (); comma (); rs (); comma (); imm (dest);
	if (rflag )
	{
	    sprintf (tmp, " rs=0x%x", rsvalue);
	    strcat (dest, tmp);
	}
	break;
    case RT_RS_SIMM:
	rt (); comma (); rs (); comma (); simm (dest);
	if (rflag )
	{
	    sprintf (tmp, " rs=0x%x", rsvalue);
	    strcat (dest, tmp);
	}
	break;
    case RT_IMM:
	rt (); comma (); imm (dest);
	break;
    case RT_SIMM:
	rt (); comma (); simm (dest);
	break;
    case RS_SIMM:
	rs (); comma (); simm (dest);
	if (rflag )
	{
	    sprintf (tmp, " rs=0x%x", rsvalue);
	    strcat (dest, tmp);
	}
	break;
    case RT_RD:
	rt (); comma (); rd();
	break;
    case RT_RD_TO:
	rt (); comma (); rd();
	if (rflag )
	    mkcomment( dest, "# rt=0x%x", rtvalue);
	break;
    case RD:
	rd ();
	break;
    case RD_RS:
	rd (); comma (); rs ();
	if (rflag )
	    mkcomment( dest, "# rs=0x%x", rsvalue);
	break;
    case RT_C0:
	rt (); comma (); c0 ();
	break;
    case RT_C0_TO:
	rt (); comma (); c0 ();
	if (rflag )
	    mkcomment( dest, "# rt=0x%x", rtvalue);
	break;
    case RT_C1:
	rt (); comma (); c1 ();
	break;
    case RT_C1_TO:
	rt (); comma (); c1 ();
	if (rflag )
	    mkcomment( dest, "# rt=0x%x", rtvalue);
	break;
    case RT_C2:
	rt (); comma (); c2 ();
	break;
    case RT_CN:
	rt (); comma (); cn ();
	break;
    case RT_CN_TO:
	rt (); comma (); cn ();
	if (rflag )
	    mkcomment( dest, "# rt=0x%x", rtvalue);
	break;
    case RT_CC1:
	rt (); comma (); cc1 ();
	break;
    case RT_CC1_TO:
	rt (); comma (); cc1 ();
	if (rflag )
	    mkcomment( dest, "# rt=0x%x", rtvalue);
	break;
    case RT_CC2:
	rt (); comma (); cc2 ();
	break;
    case RD_RT_RS:
	rd (); comma (); rt (); comma (); rs();
	if (rflag )
	{
	    mkcomment( dest, "# rt=0x%x", rtvalue);
	    sprintf (tmp, " rs=0x%x", rsvalue);
	    strcat (dest, tmp);
	}
	break;
    case JR:
    case RS:
	rs ();
	if (rflag )
	    mkcomment( dest, "# rs=0x%x", rsvalue);
	break;
    case RD_RS_RT:
	rd (); comma ();
    case RS_RT:
	rs (); comma (); rt ();
	if (rflag )
	{
	    mkcomment( dest, "# rs=0x%x ", rsvalue);
	    sprintf (tmp, " rt=0x%x", rtvalue);
	    strcat (dest, tmp);
	}
	break;
    case RD_RT:
	rd (); comma (); rt ();
	if (rflag )
	    mkcomment( dest, "# rt=0x%x", rtvalue);
	break;
    case RD_RT_SFT:
	rd (); comma (); rt (); comma ();
	sprintf (tmp, "0x%x", SHAMT_ (inst));
	strcat (dest, tmp);
	mkcomment (dest, "# %d", SHAMT_ (inst));
	if (rflag )
	{
	    sprintf (tmp, " rt=0x%x", rtvalue);
	    strcat (dest, tmp);
	}
	break;
    case RS_RT_OFF:
    case RS_OFF:
	rs (); comma ();
	do_rs = 1;
	if (pt->type == RS_RT_OFF)
	{
	    rt (); comma ();
	    if (rflag )
		do_rt = 1;
	}
    case CP_OFF:
    case OFF:
	v = IMM_ (inst);
	if (v & (1L << 15))
	    v |= 0xffff0000L;
	v1 = (long)addr + 4 + (v << 2);
	if (!adr2symoff (tmp, v1, 0))
	    sprintf (tmp, "%x", v1);
	strcat (dest, tmp);
	mkcomment (dest, "# 0x%08x", v1);
	if (rflag && do_rs )
	{
	    sprintf (tmp, " rs=0x%x", rsvalue);
	    strcat (dest, tmp);
	    do_rs = 0;
	}
	if (rflag && do_rt )
	{
	    sprintf (tmp, " rt=0x%x", rtvalue);
	    strcat (dest, tmp);
	    do_rt = 0;
	}
	break;
    case BPCODE:
	sprintf (tmp, "%d", (inst >> 16) & 0x3ff);
	strcat (dest, tmp);
	break;
    case COFUN:
	sprintf (tmp, "0x%x", inst & 0x01ffffffL);
	strcat (dest, tmp);
	break;
    case NONE:
	break;
    case TARGET:
	v = (inst & 0x03ffffff) << 2;
	v |= ((int)addr & 0xf0000000);
	if (!adr2symoff (tmp, v, 0))
	    sprintf (tmp, "%x", v);
	strcat (dest, tmp);
	mkcomment (dest, "# 0x%08x", v);
	break;
    case JALR:
	if (RD_ (inst) != 31L)
	    rd (); comma ();
	rs ();
	if (rflag )
	    mkcomment( dest, "# rs=0x%x", rsvalue);
	break;
    case LDSTC0:
	v = IMM_ (inst);
	if (v & (1L << 15))
	    v |= 0xffff0000L;
	c0ft (); comma ();
	sprintf (tmp, "%d", v);
	strcat (dest, tmp);
	strcat (dest, "(");
	rs ();
	strcat (dest, ")");
	mkcomment (dest, "# 0x%x", v);
	if (rflag )
	{
	    /* If wanting register contents, then add this too */
	    sprintf (tmp, " addr=0x%x", (int)(v + rsvalue));
	    strcat (dest, tmp);
	}
	break;
    case LDSTC1:
    case STOREC1:
	v = IMM_ (inst);
	if (v & (1L << 15))
	    v |= 0xffff0000L;
	c1ft (); comma ();
	sprintf (tmp, "%d", v);
	strcat (dest, tmp);
	strcat (dest, "(");
	rs ();
	strcat (dest, ")");
	mkcomment (dest, "# 0x%x", v);
	if (rflag )
	{
	    /* If wanting register contents, then add this too */
	    sprintf (tmp, " addr=0x%x", (int)(v + rsvalue));
	    strcat (dest, tmp);
#ifdef FLOATINGPT
	    if (pt->type == STOREC1 )
	    {
	      if (fpdis) {
		if (dpdenorm( (struct IEEEdp *)d_ft ))
		    sprintf (tmp, " ft=0.0 (dp denorm)");
		else if (dpnan ((struct IEEEdp *)d_ft))
		    sprintf (tmp, " ft=Nan");
		else
		    sprintf (tmp, " ft=%e", *d_ft);
	      }
	      else
		sprintf (tmp, " rt=0x%llx", Fpr[(int)RT_(inst)]);
	      strcat (dest, tmp);
	    }
#endif /* FLOATINGPT */
	}
	break;
    case LDSTCN:
    case STORECN:
	v = IMM_ (inst);
	if (v & (1L << 15))
	    v |= 0xffff0000L;
	cnft (); comma ();
	sprintf (tmp, "%d", v);
	strcat (dest, tmp);
	strcat (dest, "(");
	rs ();
	strcat (dest, ")");
	mkcomment (dest, "# 0x%x", v);
	if (rflag )
	{
	    /* If wanting register contents, then add this too */
	    sprintf (tmp, " addr=0x%x", (int)(v + rsvalue));
	    strcat (dest, tmp);
	}
	break;
    case LOAD_STORE:
    case STORE:
	rt (); comma ();
    load_store:
	v = IMM_ (inst);
	if (v & (1L << 15))
	    v |= 0xffff0000L;
	sprintf (tmp, "%d", v);
	strcat (dest, tmp);
	strcat (dest, "(");
	rs ();
	strcat (dest, ")");
	if (rflag )
	{
	    /* If wanting register contents, then add this too */
	    mkcomment( dest, "# addr=0x%x", (int)(v + rsvalue));
	    if (pt->type == STORE )
	    {
		sprintf (tmp, " rt=0x%x", (int)rtvalue);
		strcat (dest, tmp);
	    }
	}
	else
	mkcomment (dest, "# 0x%x", v);
	break;
    case CACHE_OP:
	sprintf (tmp, "%d,", RT_(inst));
	strcat (dest, tmp);
	goto load_store;
    case WORD:
	sprintf (tmp, "%08x", inst);
	strcat (dest, tmp);
	strcat (dest, "      # ");
	w = (long)addr;
	for (i = 0; i < 4; i++) {
	    v = load_byte ((u_int8_t *)w);
	    w++;
	    if (isprint (v))
		strccat (dest, v);
	    else
		strccat (dest, '.');
	}
	break;
    }
    return (addr + 4L);
}

/*************************************************************
 *  simm(dest)
 *      signed immediate value
 */
void
simm (dest)
     char           *dest;
{
    char            tmp[20];
    long            v;

    v = IMM_ (inst);
    sprintf (tmp, "0x%x", v);
    strcat (dest, tmp);
    if (v & (1L << 15))
	v |= 0xffff0000L;
    mkcomment (dest, "# %d", v);
}

/*************************************************************
 *  imm(dest)
 *      unsigned immediate value
 */
void
imm (dest)
     char           *dest;
{
    char            tmp[20];
    long            v;

    v = IMM_ (inst);
    sprintf (tmp, "0x%x", v);
    strcat (dest, tmp);
    mkcomment (dest, "# %d", v);
}

/*************************************************************
 *  mkcomment(p,fmt,v)
 *      generate an appropriate comment
 */
void
mkcomment (p, fmt, v)
     char           *p, *fmt;
     long            v;
{
    char            tmp[20];
    int             n;

    for (n = 50 - strlen (p); n > 0; n--)
	strcat (p, " ");
    sprintf (tmp, fmt, v);
    strcat (p, tmp);
}

/*************************************************************
 *  dispchist(args,siz)
 *      display the pc history (trace buffer)
 */
int
dispchist (args, siz)
     int             args, siz;
{
    int  i, l;
    void *adr;

    l = siz;
    for (i = 0;; i++) {
	adr = (void *)getpchist(i);
	if (adr == 0)
	    break;
	md_disasm (prnbuf, adr);
	if (more (prnbuf, &l, (args > 1) ? 0 : siz))
	    break;
    }
    return(0);
}


/*----------------------------------------------------------------*/

int
md_stacktrace(ac, av)
	int ac;
	char **av;
{
extern int optind;
	int vflag = 0;
	int c, siz, cnt;
	void *addr;

	optind = 0;
	while((c = getopt (ac, av, "v")) != EOF) {
		switch(c) {
		case 'v':
			vflag++;
			break;

		default:
			return(-1);
		}
	}
	cnt = siz = moresz;
	if(optind < ac) {
		if(!get_rsa(&cnt, av[optind++])) {
			return(-1);
		}
		siz = 0;
	}
	if(optind != ac) {
		return(-1);
	}

	ioctl (STDIN, CBREAK, NULL);

	addr = (void *)(int)cpuinfotab[whatcpu]->sp;

	while(addr != NULL) {
		void *nextframe = (void *)load_word(addr);
		void *pc = (void *)load_word((void *)(int)addr + 4);
		char *p = prnbuf;
		int framesize = (int)nextframe - (int)addr;

		if(nextframe == 0) {
			framesize = 0;
		}

		if(!adr2symoff (p, (u_int32_t)pc, 24)) {
			sprintf(p, "              0x%08x", pc);
		}
		p += strlen(p);
		/* XXX Wind up saved arg regs and print ? */
		/* XXX Useful? Well most code is optimized... */

		if(vflag) {
			p += sprintf(p, " frame=0x%08x size=%-5d",
						nextframe, framesize);
		}
		if(more(prnbuf, &cnt, siz)) {
			break;
		}
		if(addr == nextframe) {
			more("end of stack or selfpointing!", &cnt, siz);
			break;
		}
		addr = nextframe;
	}
	return(0);
}

void *
md_dumpframe (void *pframe)
{
        int nextframe;
        int lr;
        int *access;

        access = (int *)(pframe);
        nextframe = load_word(access);

        access = (int *)(nextframe+4);
        lr = load_word(access);

        printf("lr %x fp %x nfp %x\n", lr, pframe, nextframe);

        return((void *)nextframe);
}
/*
 *      Frame tracing.
 */
void
md_do_stacktrace(addr, have_addr, count, modif)
	void *addr;
	int have_addr;
	int count;
	char *modif;
{
	void *xadr;

	__asm__ volatile(" move %0, $29\n" : "=r"(xadr));

	if (have_addr == -1) {	/* Stacktrace ourself */
		addr = xadr;
	}
        else if(have_addr == 0) {
                addr = (void *)(int)cpuinfotab[whatcpu]->sp;
        }
#ifdef DUMP_FRAME
        while (addr != 0) {
                addr = md_dumpframe(addr);
        }
#endif
} 

/*
 *  Command table registration
 *  ==========================
 */
extern const Optdesc md_r_opts[];

static const Cmd MDebugCmd[] =
{
	{"Debugger"},
	{"r",           "[reg* [val|field val]]",
			md_r_opts,
			"display/set register",
			md_registers, 1, 4, CMD_REPEAT},
	{"l",           "[-bct][adr [cnt]]",
			l_opts,
			"list (disassemble) memory",
			md_disassemble, 1, 5, CMD_REPEAT},
	{"bt",          "[-v] [cnt]",
			0,
			"stack backtrace",
			md_stacktrace, 1, 3, CMD_REPEAT},
	{0, 0}
};

static void init_cmd __P((void)) __attribute__ ((constructor));

static void
init_cmd()
{
        cmdlist_expand(MDebugCmd, 1);
}

