TOPDIR := $(realpath ../..)

CLEAN-FILES := data/drdos8x8.c donut-pc

PSF2C.drdos8x8 := --name drdos8x8 --type console

include $(TOPDIR)/build/effect.mk

donut-pc: donut-pc.c donut.h
	cc -g -Og -Wall -o $@ $<
