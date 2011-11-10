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

ZLIBDIR=	${S}/lib/libz

ZLIBDST?=	${OBJDIR}/lib/libz

ZLIB=	${ZLIBDST}/libz.a

ZLIBMAKE= \
	cd ${ZLIBDIR} && MAKEOBJDIR=${ZLIBDST} ${MAKE} \
	    CC='${CC}' CFLAGS='${CFLAGS}' AS='${AS}' AFLAGS='${AFLAGS}' \
	    LD='${LD}' STRIP='${STRIP}' CPP='${CPP}' STRIP='${STRIP}'	\
	    AR='${AR}' RANLIB='${RANLIB}' NM='${NM}' LORDER='${LORDER}'	\
	    XMACHINE='${MACHINE}' XMACHINE_ARCH='${MACHINE_ARCH}'	\
	    MKDEP='${MKDEP}' ZLIBDIR=${ZLIBDIR} PMONDIR=${PMONDIR}	\
	    ZLIBCPPFLAGS='${CPPFLAGS}' S='${S}'	\
	    OBJDIR='${ZLIBDST}'

${ZLIB}:		__always_make_zliblib
	@echo making sure the libz library is up to date...
	@${ZLIBMAKE} ${ZLIB}

clean::			__always_make_zliblib
	@echo cleaning the libz library objects
	@${ZLIBMAKE} clean

depend:: 		__always_make_zliblib
	@echo depending the libz library objects
	@${ZLIBMAKE} depend

__always_make_zliblib:	
	-mkdir -p ${ZLIBDST} 
