ifndef PMONCC
CC=$(CROSS_COMPILE)gcc -fexec-charset=GB2312 
CROSS_COMPILE ?= mipsel-linux-#mips-elf-
#CROSS_COMPILE ?= mipsel-loongson-linux-gnu-#mips-elf-
else
CC=${PMONCC}
endif


ifeq "$(shell echo ${CC}|grep  'mips-elf-')" ""
 ifeq "$(shell echo ${CC}|grep  '\-mabi=64')" ""
LD		= $(CROSS_COMPILE)ld  -m elf32ltsmip -G 0 -static -n -nostdlib
OUT_FORMAT="elf32-tradlittlemips"
 else
LD		= $(CROSS_COMPILE)ld  -m elf64ltsmip -G 0 -static -n -nostdlib
OUT_FORMAT="elf64-tradlittlemips"
 endif
else
ENDIAN_FLAG= -EL
LD		= $(CROSS_COMPILE)ld  -m elf32ebmip -G 0 -static -n -nostdlib -EL
CC :=${CC} -EL
OUT_FORMAT="elf32-littlemips"
endif

GZROMSTARTADDR?=0xffffffff81000000
ROMSTARTADDR?=0xffffffff80010000
RAMSTARTADDR?=0xffffffff88000000
export CROSS_COMPILE
export CC += ${MYCC}  -mno-abicalls -fno-pic
export LD
export MKDEP=makedepend
ifeq ("${tgt}","rom")
gencode=./genrom
endif

ifeq ("${tgt}","ejtag_rom")
gencode=./genrom_ejtag
endif

ifeq ("${tgt}","ejtag_rom1")
gencode=./genrom
CC += -DBOOT_FROM_EJTAG
endif

ifeq ("${tgt}","ejtag_ram")
gencode=./genrom
ROMSTARTADDR=0xffffffffff200200
endif


ifdef obj
export CC += -g3
pwd=$(shell pwd)
gcc_E:
	cd ../Targets/${TARGET}/compile/${TARGETEL};${pwd}/mymake  ${obj} > /tmp/tmp.c
endif

ifndef tgt
help:
	@echo use make tgt=sim for sim in linux
	@echo use make tgt=rom for generate romfile gzrom.bin
	@echo use make tgt=ram for generate ramfile gzram
	@echo use make cfg     for config
	@echo use make recfg   for change config
	@echo use make tgt=rom DEBUG=-g MYCC="'"-g3 -DMYDBG='"printf(\"debug:%s,%d\\n\",__FILE__,__LINE__);"'"'" to support MYDBG macro. 
else
all: ${tgt}
sim: clean pmon.bin.c
	gcc -o zloader  -DSIM zloader.c
ejtag_rom ejtag_rom1 ejtag_ram rom: clean ${START} zloader.o 
	gcc  -DSTARTADDR=${GZROMSTARTADDR} -DOUT_FORMAT=\"${OUT_FORMAT}\" -DOUT_ARCH=mips -Umips -E -P ld.script.S  > ld.script
	${LD} -T ld.script -e start -g -o gzrom ${START} zloader.o 
	${CROSS_COMPILE}objcopy -O binary gzrom gzrom.bin

ram: clean startram.o zloader.o 
	${CC}  -DSTARTADDR=${RAMSTARTADDR} -DOUT_FORMAT=\"${OUT_FORMAT}\" -DOUT_ARCH=mips -Umips -E -P ld.script.S  > ld.script
	${LD}  -T ld.script -e start -o gzram startram.o zloader.o 

${START}:
	rm -f ../Targets/${TARGET}/compile/${TARGETEL}/${START}
	gcc  -DSTARTADDR=${ROMSTARTADDR} -DOUT_FORMAT=\"${OUT_FORMAT}\" -DOUT_ARCH=mips -Umips -E -P ld.script.S  > ../Targets/${TARGET}/conf/ld.script
	make -C ../Targets/${TARGET}/compile/${TARGETEL}/
	cp ../Targets/${TARGET}/compile/${TARGETEL}/${START} .

startram.o:
	gcc  -DSTARTADDR=${RAMSTARTADDR} -DOUT_FORMAT=\"${OUT_FORMAT}\" -DOUT_ARCH=mips -Umips -E -P ld.script.S  > ld.script
	make -C ../Targets/${TARGET}/compile/${TARGETEL}/ DEBUG='-g -DMYDBG="printf(\"debug:%s,%d\\n\",__FILE__,__LINE__);"'
	$(CC) -D_LOCORE -G 0  -nostdinc   -DMIPS  -DCONS_BAUD="B115200"  -DCONFIG_CACHE_64K_4WAY  -D_KERNEL -D__OpenBSD__ -DPMON -D__PMON__ -mno-abicalls -c startram.S ${ZLOADER_OPTIONS} -D MEMSIZE=${MEMSIZE}
# $(CC) -D_LOCORE -G 0  -nostdinc   -DMIPS  -DCONS_BAUD="B115200"  -DCONFIG_CACHE_64K_4WAY  -D_KERNEL -D__OpenBSD__ -DPMON -D__PMON__ -EL -mno-abicalls -mcpu=r4000 -c startram.S -D MEMSIZE=${MEMSIZE}
zloader.o: zloader.c inflate.c  malloc.c  memop.c  pmon.bin.c initmips.c
	$(CC) -O2 ${DEBUG} -c zloader.c ${ZLOADER_OPTIONS} -DMEMSIZE=${MEMSIZE}

zlib_loader.o: zlib_loader.c  ./zlib_gzip zlib_pmon.bin.c initmips.c
	cd zlib_inflate;$(CC) -I ../../include -c *.c
	#cd zlib_inflate;$(CC) -I ../../include -EL -mcpu=r4000 -c *.c
	#$(CC) -I ../include -EL -mcpu=r4000 -c zlib_loader.c 
	$(CC) -I ../include -c zlib_loader.c 

initmips.c:  ../Targets/${TARGET}/compile/${TARGETEL}/pmon
	${gencode} $< > initmips.c

zlib_pmon.bin.c:  ../Targets/${TARGET}/compile/${TARGETEL}/pmon.bin
	./zlib_gzip $<  zlib_pmon.bin.gz
	./bin2c zlib_pmon.bin.gz zlib_pmon.bin.c biosdata
	
pmon.bin.c:  ../Targets/${TARGET}/compile/${TARGETEL}/pmon.bin
	gzip $< -c > pmon.bin.gz
	./bin2c pmon.bin.gz pmon.bin.c biosdata

endif

ifeq ("${tgt}","rom")
gencode=./genrom
endif

ifeq ("${tgt}","zlib_rom")
gencode=./genrom
endif


ifndef gencode
gencode=./genram
endif

cleanall: clean
	make -C ../Targets/${TARGET}/compile/${TARGETEL}/ clean
clean:
	rm -rf *.o zlib_gzip zloader pmon.bin.c gzrom gzrom.bin gzram  initmips.c pmon.bin.gz zlib_deflate/*.o zlib_inflate/*.o zlib_gzrom zlib_gzrom.bin
cfg1:
	perl -i -ne 'print;exit if(/^# DO NOT DELETE/);' ../lib/libc/Makefile 
	perl -i -ne 'print;exit if(/^# DO NOT DELETE/);' ../lib/libm/Makefile 
	perl -i -ne 'print;exit if(/^# DO NOT DELETE/);' ../lib/libz/Makefile 
	cd ../Targets/${TARGET}/conf/;pmoncfg ${TARGETEL}
cfg:
	# DO NOT DELETE
	perl -i -ne 'print;exit if(/^# DO NOT DELETE/);' ../lib/libc/Makefile 
	perl -i -ne 'print;exit if(/^# DO NOT DELETE/);' ../lib/libm/Makefile 
	perl -i -ne 'print;exit if(/^# DO NOT DELETE/);' ../lib/libz/Makefile 
	mkdir -p ../Targets/${TARGET}/compile
	cd ../Targets/${TARGET}/conf/;pmoncfg ${TARGETEL}
	make -C ../Targets/${TARGET}/compile/${TARGETEL}/ depend clean
recfg:
	vi ../Targets/${TARGET}/conf/$(TARGETEL)
	make cfg

zlib_gzip: zlib_gzip.c
	cd zlib_deflate/;gcc -I ../../include -c *.c
	cd zlib_inflate/;gcc -I ../../include -c *.c
	gcc -o zlib_gzip zlib_gzip.c zlib_deflate/*.o zlib_inflate/*.o
