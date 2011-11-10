#
# Makefile for the Linux/MIPS kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#

obj-y := sys.o decode.o ops.o ops2.o prim_ops.o fpu.o debug.o

EXTRA_CFLAGS += -I./ -I./include -DIN_LINUX_KERNEL -DDEBUG

O_TARGET := x86emu.o

include $(TOPDIR)/Rules.make
