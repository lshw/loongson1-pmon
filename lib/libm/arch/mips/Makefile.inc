#	$Id: Makefile.inc,v 1.1.1.1 2006/09/14 01:59:06 root Exp $ */
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


CPPFLAGS	+= -D_KERNEL

CPPFLAGS += -D_IEEE_LIBM -D_ISOC99_SOURCE -D_SVID_SOURCE $(CFLAGS_FP_ADD)

DO_C99_MATH = false

ifeq ($(strip $(DO_C99_MATH)),true)
EXTRA_CFLAGS += -DDO_C99_MATH
SRCS+= e_acos.c e_acosh.c e_asin.c e_atan2.c e_atanh.c e_cosh.c\
         e_exp.c e_fmod.c e_gamma.c e_gamma_r.c e_hypot.c e_j0.c\
         e_j1.c e_jn.c e_lgamma.c e_lgamma_r.c e_log.c e_log10.c\
         e_pow.c e_remainder.c e_rem_pio2.c e_scalb.c e_sinh.c\
         e_sqrt.c k_cos.c k_rem_pio2.c k_sin.c k_standard.c k_tan.c\
         s_asinh.c s_atan.c s_cbrt.c s_ceil.c s_copysign.c s_cos.c\
         s_erf.c s_expm1.c s_fabs.c s_finite.c s_floor.c s_frexp.c\
         s_ilogb.c s_ldexp.c s_lib_version.c s_log1p.c s_logb.c\
         s_matherr.c s_modf.c s_nextafter.c s_rint.c s_scalbn.c\
         s_signgam.c s_significand.c s_sin.c s_tan.c s_tanh.c\
         w_acos.c w_acosh.c w_asin.c w_atan2.c w_atanh.c w_cabs.c\
         w_cosh.c w_drem.c w_exp.c w_fmod.c w_gamma.c w_gamma_r.c\
         w_hypot.c w_j0.c w_j1.c w_jn.c w_lgamma.c w_lgamma_r.c\
         w_log.c w_log10.c w_pow.c w_remainder.c w_scalb.c w_sinh.c\
         w_sqrt.c ceilfloor.c fpmacros.c frexpldexp.c logb.c rndint.c\
         scalb.c sign.c
else
SRCS+=   w_acos.c w_asin.c s_atan.c w_atan2.c s_ceil.c s_cos.c \
	 w_cosh.c w_exp.c s_expm1.c s_fabs.c s_floor.c w_fmod.c \
	 s_frexp.c w_log.c w_log10.c s_modf.c w_pow.c s_sin.c \
	 w_sinh.c w_sqrt.c s_tan.c s_tanh.c s_scalbn.c s_copysign.c \
	 sign.c e_acos.c e_asin.c e_atan2.c k_cos.c e_cosh.c e_exp.c \
	 e_fmod.c e_log.c e_log10.c e_pow.c k_sin.c e_sinh.c e_sqrt.c \
	 k_tan.c e_rem_pio2.c k_rem_pio2.c
endif

