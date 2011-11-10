#	$OpenBSD: bsd.prog.mk,v 1.30 2001/08/23 16:39:33 art Exp $
#	$NetBSD: bsd.prog.mk,v 1.55 1996/04/08 21:19:26 jtc Exp $
#	@(#)bsd.prog.mk	5.26 (Berkeley) 6/25/91

.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif

.include <pmon.own.mk>

.SUFFIXES: .out .o .c .cc .C .cxx .y .l .s .8 .7 .6 .5 .4 .3 .2 .1 .0

.if ${WARNINGS:L} == "yes"
CFLAGS+=       ${CDIAGFLAGS}
CXXFLAGS+=     ${CXXDIAGFLAGS}
.endif
CFLAGS+=	${COPTS}
CXXFLAGS+=     ${CXXOPTS}

.if (${MACHINE_ARCH} == "powerpc") || (${MACHINE_ARCH} == "alpha") || (${MACHINE_ARCH} == "sparc64")
CRTBEGIN?=       ${DESTDIR}/usr/lib/crtbegin.o
CRTEND?=         ${DESTDIR}/usr/lib/crtend.o
.endif

LIBCRT0?=	${DESTDIR}/usr/lib/crt0.o
LIBC?=		${DESTDIR}/usr/lib/libc.a
LIBGCC?=	${DESTDIR}/usr/lib/libgcc.a
LIBL?=		${DESTDIR}/usr/lib/libl.a
LIBM?=		${DESTDIR}/usr/lib/libm.a
LIBY?=		${DESTDIR}/usr/lib/liby.a
LIBZ?=		${DESTDIR}/usr/lib/libz.a

.if defined(SHAREDSTRINGS)
CLEANFILES+=strings
.c.o:
	${CC} -E ${CFLAGS} ${.IMPSRC} | xstr -c -
	@${CC} ${CFLAGS} -c x.c -o ${.TARGET}
	@rm -f x.c

.cc.o:
	${CXX} -E ${CXXFLAGS} ${.IMPSRC} | xstr -c -
	@mv -f x.c x.cc
	@${CXX} ${CXXFLAGS} -c x.cc -o ${.TARGET}
	@rm -f x.cc

.C.o:
	${CXX} -E ${CXXFLAGS} ${.IMPSRC} | xstr -c -
	@mv -f x.c x.C
	@${CXX} ${CXXFLAGS} -c x.C -o ${.TARGET}
	@rm -f x.C

.cxx.o:
	${CXX} -E ${CXXFLAGS} ${.IMPSRC} | xstr -c -
	@mv -f x.c x.cxx
	@${CXX} ${CXXFLAGS} -c x.cxx -o ${.TARGET}
	@rm -f x.cxx
.endif


.if defined(PROG)
SRCS?=	${PROG}.c
.if !empty(SRCS:N*.h:N*.sh)
OBJS+=	${SRCS:N*.h:N*.sh:R:S/$/.o/g}
LOBJS+=	${LSRCS:.c=.ln} ${SRCS:M*.c:.c=.ln}
.endif

.if defined(OBJS) && !empty(OBJS)
.if defined(DESTDIR)

${PROG}: ${LIBCRT0} ${OBJS} ${LIBC} ${CRTBEGIN} ${CRTEND} ${DPADD}
	${CC} ${LDFLAGS} ${LDSTATIC} -o ${.TARGET} -nostdlib -L${DESTDIR}/usr/lib ${LIBCRT0} ${CRTBEGIN} ${OBJS} ${LDADD} -lgcc -lc -lgcc ${CRTEND}

.else

${PROG}: ${LIBCRT0} ${OBJS} ${LIBC} ${CRTBEGIN} ${CRTEND} ${DPADD}
	${CC} ${LDFLAGS} ${LDSTATIC} -o ${.TARGET} ${OBJS} ${LDADD}

.endif	# defined(DESTDIR)
.endif	# defined(OBJS) && !empty(OBJS)

.if	!defined(MAN)
MAN=	${PROG}.1
.endif	# !defined(MAN)
.endif	# defined(PROG)

.MAIN: all
all: ${PROG} _SUBDIRUSE

.if !target(clean)
clean: _SUBDIRUSE
	rm -f a.out [Ee]rrs mklog core *.core \
	    ${PROG} ${OBJS} ${LOBJS} ${CLEANFILES}
.endif

cleandir: _SUBDIRUSE clean

.if !target(lint)
lint: ${LOBJS}
.if defined(LOBJS) && !empty(LOBJS)
	@${LINT} ${LINTFLAGS} ${LDFLAGS:M-L*} ${LOBJS} ${LDADD}
.endif
.endif

.if !defined(NOMAN)
.include <pmon.man.mk>
.endif

.if !defined(NONLS)
.include <pmon.nls.mk>
.endif

.include <pmon.obj.mk>
.include <pmon.dep.mk>
.include <pmon.subdir.mk>
.include <pmon.sys.mk>
