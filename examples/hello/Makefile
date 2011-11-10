#
#  Makefile to build hello.c
#

CROSS_COMPILE 	=mips-elf-

#
# Include the make variables (CC, etc...)
#

AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump
SIZE		= $(CROSS_COMPILE)size

hello:	hello.c
	${CC} -G 0 -O2 -EL -mips2 -mcpu=r4000 -Ttext=0x80200000 -N -o hello hello.c -nostdlib -e main
	cp hello /tftpboot
clean:
	rm -f hello
