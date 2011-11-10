#	$OpenBSD: Makefile,v 1.7 1997/04/27 20:56:23 millert Exp $
#	$NetBSD: Makefile,v 1.12 1995/10/22 00:45:53 christos Exp $

FILES=	pmon.doc.mk pmon.dep.mk pmon.lib.mk pmon.man.mk pmon.nls.mk \
	pmon.obj.mk pmon.own.mk pmon.prog.mk \
	pmon.subdir.mk pmon.sys.mk
NOOBJ=	noobj

install:
	${INSTALL} ${INSTALL_COPY} -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}/usr/share/mk

.include <bsd.prog.mk>
