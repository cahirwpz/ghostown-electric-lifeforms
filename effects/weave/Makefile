TOPDIR := $(realpath ../..)

CLEAN-FILES := data/bar.c data/bar-2.c data/stripes.c data/colors.c

PNG2C.bar := --bitmap bar,480x133x4 --palette bar,16
PNG2C.bar-2 := --palette bar2,16
PNG2C.stripes := --sprite stripes,320,4
PNG2C.colors := --palette stripes,16

SOURCES := weave.c

include $(TOPDIR)/build/effect.mk
