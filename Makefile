#	$Id: Makefile,v 1.1.1.1 2006/09/14 01:59:06 root Exp $

#
# Copyright (c) 2000 Opsycon AB  (www.opsycon.se)
# Copyright (c) 2000 Rtmx, Inc   (www.rtmx.com)
# Copyright (c) 2001 IP Unplugged AB (www.ipunplugged.com)
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
#	This product includes software developed for Rtmx, Inc by
#	Opsycon Open System Consulting AB, Sweden.
#	This product includes software developed by ipUnplugged AB.
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

PMONTOP=${.CURDIR}
MAKE=make -m ${PMONTOP}/tools/mk

pmontools:
	@cd tools; ${MAKE}

pmonlibs:
	@cd lib; ${MAKE}
clean:
	perl -e 'for(<zloader.*>){system qq(cd $$_;make cleanall;cd -;);}'
	perl -i -ne 'print;exit if(/^# DO NOT DELETE/);' lib/libc/Makefile
	perl -i -ne 'print;exit if(/^# DO NOT DELETE/);' lib/libm/Makefile 
	perl -i -ne 'print;exit if(/^# DO NOT DELETE/);' lib/libz/Makefile 
	rm -f `find . -type f \( -name ld.script -o -name tags \)`
	rm -rf `find . -type d -name compile -exec echo ./{}/\* \;`
