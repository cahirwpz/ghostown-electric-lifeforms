TOPDIR := $(realpath ../..)

SOURCES := pt.asm

include $(TOPDIR)/build/lib.mk

VASMFLAGS.pt += -DMINIMAL=1

pt.o: pt.asm ptplayer.asm $(TOPDIR)/include/ptplayer.h
