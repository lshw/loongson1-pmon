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


CFLAGS	+= -D_KERNEL

SRCS+=	argvize.c atob.c atof.c atoi.c atol.c bcopy.c bzero.c bcmp.c	\
	calloc.c cc2str.c close.c ctype_.c dbl2asci.c errno.c ethers.c	\
	exit.c fclose.c feof.c fflush.c ffs.c fgetc.c fgets.c fileno.c 	\
	fprintf.c fputs.c fread.c fseek.c fwrite.c getc.c getchar.c	\
	gethostnamadr.c getopt.c getprotoname.c gets.c getservbyname.c	\
	getservent.c getword.c index.c inet_addr.c lseek.c malloc.c	\
	memchr.c memcpy.c memset.c misc.c modf.c open.c pmalloc.c	\
	printf.c putc.c putchar.c puts.c qsort.c queue.c rand.c read.c	\
	realloc.c recv.c						\
	res_comp.c res_init.c res_mkquery.c res_query.c res_send.c	\
	rindex.c sbrk.c scanf.c send.c signal.c sigsetops.c sizemem.c	\
	sprintf.c stdio.c str2cc.c str_fmt.c strbalp.c strbequ.c	\
	strcasecmp.c strcat.c strccat.c strchr.c strcmp.c strcpy.c	\
	strcspn.c strdchr.c strempty.c strichr.c striequ.c stristr.c	\
	strlen.c strmerge.c strncat.c strnchr.c strncmp.c strncpy.c	\
	strnwrd.c strpat.c strpbrk.c strposn.c strrchr.c strrpset.c	\
	strrrot.c strrset.c strset.c strsort.c strspn.c strstr.c	\
	strtok.c strtoupp.c terms.c time.c tolower_.c		\
	toupper_.c ungetc.c vfprintf.c vsprintf.c write.c getbaud.c	\
	tcdrain.c tcflow.c tcflush.c tcgetattr.c tcgetpgrp.c		\
	tcsendbreak.c tcsetattr.c tcsetpgrp.c cfgetispeed.c		\
	cfgetospeed.c cfmakeraw.c cfsetispeed.c cfsetospeed.c		\
	cfsetspeed.c ulmin.c lmin.c ioctl.c filefs.c parseurl.c

SRCS+=	setjmp.S
