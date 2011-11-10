#
# Makefile for the Linux/MIPS kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#

obj-y := freebiosvga.o besys.o bios.o biosemu.o

EXTRA_CFLAGS += -I./x86emu/include -I./ -DIN_LINUX_KERNEL -DPLAY_SAFE -DDEBUG

O_TARGET := vga.o

include $(TOPDIR)/Rules.make
