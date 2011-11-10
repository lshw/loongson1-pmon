#	$Id: Makefile,v 1.1.1.1 2006/09/14 01:59:06 root Exp $

BISON?= bison -y

PROG=	pmoncfg
DESTDIR=/opt/pmon2000/tools

SRCS=	files.c gram.y hash.c main.c mkheaders.c mkioconf.c mkmakefile.c \
	pack.c scan.l sem.c util.c 

OBJS=	files.o hash.o main.o mkheaders.o mkioconf.o mkmakefile.o pack.o sem.o util.o gram.o scan.o 

CFLAGS+=-I${.CURDIR} -I. -DYY_SKIP_YYWRAP

LEX=flex


CLEANFILES=gram.c scan.c gram.h pmoncfg.cat8
MAN=	pmoncfg.8

.y.c:
	${BISON} -d -o $@ $< 

${PROG}: ${OBJS}
	${CC} -o $@ ${OBJS} ${LIBS}

install:
	cp ${PROG} ${DESTDIR}/bin

clean:
	${RM} -rf ${OBJS} ${CLEANFILES} ${PROG}
