#	$Id: Makefile,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */
#
#
# Copyright (c) 2001 Opsycon AB  (www.opsycon.se)
# Copyright (c) 2002 Patrik Lindergren  (www.lindergren.com)
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#	This product includes software developed by Opsycon AB, Sweden.
#	This product includes software developed by Patrik Lindergren.
# 4. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
#

LIB=m
NOPIC=

MACHINE=	${XMACHINE}
MACHINE_ARCH=	${XMACHINE_ARCH}
CURDIR=$(shell pwd)

M=	${CURDIR}/arch/${MACHINE_ARCH}

CPPFLAGS=	-I$M ${MLIBCPPFLAGS} -U_KERNEL

VPATH+=	${M} ${MLIBDIR}
 
include ${M}/Makefile.inc

# Files to clean up
CLEANFILES+= ${OBJDIR}/lib${LIB}.a

include ${PMONDIR}/tools/scripts/pmon.lib.gmk

${OBJDIR}/lib${LIB}.a: ${OBJS}
	@echo building standard ${LIB} library
	@rm -f $@
	@${AR} rcs $@ ${OBJS}
#	${RANLIB} $@

# DO NOT DELETE

w_acos.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
w_acos.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
w_acos.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
w_acos.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
w_acos.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
w_acos.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
w_acos.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
w_acos.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
w_acos.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
w_asin.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
w_asin.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
w_asin.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
w_asin.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
w_asin.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
w_asin.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
w_asin.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
w_asin.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
w_asin.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
s_atan.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
s_atan.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
s_atan.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
s_atan.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
s_atan.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
s_atan.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
s_atan.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
s_atan.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
s_atan.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
w_atan2.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
w_atan2.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
w_atan2.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
w_atan2.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
w_atan2.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
w_atan2.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
w_atan2.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
w_atan2.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
w_atan2.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
s_ceil.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
s_ceil.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
s_ceil.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
s_ceil.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
s_ceil.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
s_ceil.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
s_ceil.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
s_ceil.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
s_ceil.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
s_cos.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
s_cos.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
s_cos.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
s_cos.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
s_cos.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
s_cos.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
s_cos.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
s_cos.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
s_cos.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
w_cosh.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
w_cosh.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
w_cosh.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
w_cosh.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
w_cosh.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
w_cosh.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
w_cosh.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
w_cosh.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
w_cosh.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
w_exp.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
w_exp.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
w_exp.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
w_exp.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
w_exp.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
w_exp.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
w_exp.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
w_exp.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
w_exp.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
s_expm1.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
s_expm1.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
s_expm1.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
s_expm1.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
s_expm1.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
s_expm1.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
s_expm1.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
s_expm1.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
s_expm1.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
s_fabs.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
s_fabs.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
s_fabs.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
s_fabs.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
s_fabs.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
s_fabs.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
s_fabs.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
s_fabs.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
s_fabs.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
s_floor.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
s_floor.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
s_floor.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
s_floor.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
s_floor.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
s_floor.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
s_floor.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
s_floor.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
s_floor.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
w_fmod.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
w_fmod.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
w_fmod.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
w_fmod.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
w_fmod.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
w_fmod.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
w_fmod.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
w_fmod.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
w_fmod.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
s_frexp.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
s_frexp.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
s_frexp.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
s_frexp.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
s_frexp.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
s_frexp.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
s_frexp.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
s_frexp.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
s_frexp.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
w_log.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
w_log.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
w_log.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
w_log.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
w_log.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
w_log.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
w_log.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
w_log.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
w_log.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
w_log10.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
w_log10.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
w_log10.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
w_log10.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
w_log10.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
w_log10.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
w_log10.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
w_log10.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
w_log10.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
s_modf.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
s_modf.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
s_modf.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
s_modf.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
s_modf.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
s_modf.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
s_modf.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
s_modf.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
s_modf.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
w_pow.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
w_pow.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
w_pow.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
w_pow.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
w_pow.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
w_pow.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
w_pow.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
w_pow.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
w_pow.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
s_sin.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
s_sin.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
s_sin.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
s_sin.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
s_sin.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
s_sin.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
s_sin.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
s_sin.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
s_sin.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
w_sinh.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
w_sinh.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
w_sinh.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
w_sinh.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
w_sinh.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
w_sinh.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
w_sinh.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
w_sinh.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
w_sinh.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
w_sqrt.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
w_sqrt.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
w_sqrt.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
w_sqrt.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
w_sqrt.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
w_sqrt.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
w_sqrt.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
w_sqrt.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
w_sqrt.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
s_tan.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
s_tan.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
s_tan.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
s_tan.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
s_tan.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
s_tan.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
s_tan.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
s_tan.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
s_tan.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
s_tanh.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
s_tanh.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
s_tanh.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
s_tanh.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
s_tanh.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
s_tanh.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
s_tanh.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
s_tanh.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
s_tanh.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
s_scalbn.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
s_scalbn.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
s_scalbn.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
s_scalbn.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
s_scalbn.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
s_scalbn.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
s_scalbn.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
s_scalbn.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
s_scalbn.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
s_copysign.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
s_copysign.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
s_copysign.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
s_copysign.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
s_copysign.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
s_copysign.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
s_copysign.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
s_copysign.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
s_copysign.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
e_acos.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
e_acos.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
e_acos.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
e_acos.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
e_acos.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
e_acos.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
e_acos.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
e_acos.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
e_acos.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
e_asin.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
e_asin.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
e_asin.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
e_asin.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
e_asin.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
e_asin.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
e_asin.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
e_asin.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
e_asin.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
e_atan2.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
e_atan2.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
e_atan2.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
e_atan2.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
e_atan2.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
e_atan2.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
e_atan2.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
e_atan2.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
e_atan2.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
k_cos.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
k_cos.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
k_cos.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
k_cos.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
k_cos.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
k_cos.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
k_cos.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
k_cos.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
k_cos.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
e_cosh.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
e_cosh.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
e_cosh.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
e_cosh.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
e_cosh.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
e_cosh.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
e_cosh.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
e_cosh.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
e_cosh.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
e_exp.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
e_exp.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
e_exp.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
e_exp.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
e_exp.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
e_exp.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
e_exp.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
e_exp.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
e_exp.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
e_fmod.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
e_fmod.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
e_fmod.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
e_fmod.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
e_fmod.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
e_fmod.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
e_fmod.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
e_fmod.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
e_fmod.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
e_log.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
e_log.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
e_log.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
e_log.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
e_log.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
e_log.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
e_log.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
e_log.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
e_log.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
e_log10.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
e_log10.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
e_log10.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
e_log10.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
e_log10.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
e_log10.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
e_log10.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
e_log10.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
e_log10.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
e_pow.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
e_pow.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
e_pow.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
e_pow.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
e_pow.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
e_pow.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
e_pow.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
e_pow.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
e_pow.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
k_sin.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
k_sin.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
k_sin.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
k_sin.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
k_sin.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
k_sin.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
k_sin.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
k_sin.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
k_sin.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
e_sinh.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
e_sinh.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
e_sinh.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
e_sinh.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
e_sinh.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
e_sinh.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
e_sinh.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
e_sinh.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
e_sinh.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
e_sqrt.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
e_sqrt.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
e_sqrt.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
e_sqrt.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
e_sqrt.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
e_sqrt.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
e_sqrt.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
e_sqrt.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
e_sqrt.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
k_tan.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
k_tan.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
k_tan.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
k_tan.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
k_tan.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
k_tan.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
k_tan.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
k_tan.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
k_tan.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
e_rem_pio2.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
e_rem_pio2.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
e_rem_pio2.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
e_rem_pio2.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
e_rem_pio2.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
e_rem_pio2.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
e_rem_pio2.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
e_rem_pio2.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
e_rem_pio2.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
k_rem_pio2.o: /home/aa/1a-yun-test/1B-PMON-BSP/include/math.h math_private.h
k_rem_pio2.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/types.h
k_rem_pio2.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/types.h
k_rem_pio2.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/cdefs.h
k_rem_pio2.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/cdefs.h
k_rem_pio2.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/ansi.h
k_rem_pio2.o: /home/aa/1a-yun-test/1B-PMON-BSP/Targets/LS1B/compile/ls1b.cloud/machine/endian.h
k_rem_pio2.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/sys/endian.h
k_rem_pio2.o: /home/aa/1a-yun-test/1B-PMON-BSP/sys/arch/mips/include/endian.h
