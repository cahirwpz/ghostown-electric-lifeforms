TOPDIR := $(realpath ..)

OBJECTS := data/JazzCat-RitchieGlitchie-ctr.o \
	   data/JazzCat-RitchieGlitchie-smp.o

LIBS := libctr

CLEAN-FILES := main.o data/bar.c data/bar-2.c data/stripes.c data/colors.c \
	       data/JazzCat-RitchieGlitchie.smp

SOURCES := weave.c
MAIN := main.o

PNG2C.bar := --bitmap bar,480x133x4 --palette bar_pal,16
PNG2C.bar-2 := --palette bar2_pal,16
PNG2C.stripes := --sprite stripes,320,4
PNG2C.colors := --palette stripes_pal,16

include $(TOPDIR)/build/effect.mk

data/%.ctr data/%.smp: data/%.mod
	$(PYTHON3) data/CinterConvert.py $^ $@ $(@:.ctr=.smp)

# Total instrument memory is determined by converter script!
OBJCOPY-CHIP := --rename-section .data=.datachip,alloc,load,data,contents \
	        --pad-to 185738

data/%-ctr.o: data/%.ctr
	@echo "[OBJCOPY] $(DIR)$< -> $(DIR)$@"
	$(OBJCOPY)  -I binary -O amiga $^ $@

data/%-smp.o: data/%.smp
	@echo "[OBJCOPY] $(DIR)$< -> $(DIR)$@"
	$(OBJCOPY) $(OBJCOPY-CHIP) -I binary -O amiga $^ $@
