TOPDIR := $(realpath ../..)

SOURCES := butterfly-gears.c textureloop.asm

CLEAN-FILES := \
	data/gears_testscreen.c \
	data/gears_testscreen_debug.c \
	data/texture_butterfly.c \
	data/texture_butterfly2.c \
	data/textureloop-generated.asm

PNG2C.gears_testscreen := --bitmap testscreen,320x280x5 --palette testscreen,32,+store_unused
PNG2C.gears_testscreen_debug := --bitmap testscreen,320x280x5 --palette testscreen,32,+store_unused
PNG2C.texture_butterfly := --pixmap texture_butterfly,128x128x12
PNG2C.texture_butterfly2 := --pixmap texture_butterfly2,128x128x12

include $(TOPDIR)/build/effect.mk

textureloop.asm: data/textureloop-generated.asm

%.asm: %.txt
	@echo "[GEN] $(DIR)$< -> $(DIR)$@"
	$(PYTHON3) data/gen-unrolled-loop.py $< > $@
