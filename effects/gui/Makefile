TOPDIR := $(realpath ../..)

CLEAN-FILES := data/toggle_0.c data/toggle_1.c data/koi8r.8x8.c data/pointer.c

LIBS := libgui

PNG2C.toggle_0 := --bitmap toggle_0,16x16x1
PNG2C.toggle_1 := --bitmap toggle_1,16x16x1
PNG2C.pointer := --sprite pointer,16,1 --palette pointer,4
PSF2C.koi8r.8x8 := --name font --type gui

include $(TOPDIR)/build/effect.mk
