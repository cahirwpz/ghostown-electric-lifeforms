TOPDIR := $(realpath ..)

ADPCM := 1
DELTA := 0

CTR_VER := Full
SMP_TOTALSIZE := 181610

RLE2PNG = rle2png.py
ELECTRONS2C = electrons2c.py

OBJECTS := data/JazzCat-ElectricLifeforms-$(CTR_VER).ctr.o \
	   data/text-scroll.o

ifeq ($(ADPCM), 1)
OBJECTS += data/JazzCat-ElectricLifeforms-$(CTR_VER)-ADPCM.smp.o
else ifeq ($(DELTA), 1)
OBJECTS += data/JazzCat-ElectricLifeforms-$(CTR_VER)-Delta.smp.o
else
OBJECTS += data/JazzCat-ElectricLifeforms-$(CTR_VER).smp.o
endif

LIBS := libctr

CLEAN-FILES := main.o data/bar.c data/bar-2.c data/stripes.c data/colors.c \
	data/JazzCat-ElectricLifeforms*.{smp,o} data/text-scroll-font.c \
	data/intro.c data/turmite-pal.c data/electric-lifeforms-0[1-2].c \
	data/fruit.c data/tilemover-pal.c data/anemone-pal.c \
	data/ghostown-logo-crop.c data/p46basedprng.c data/cell-gradient.c \
	data/logo-electrons.c data/electric-lifeforms.c \
	data/electric-lifeforms-wireworld.c data/ghostown-logo-0[1-3].c \
	data/turmite-credits-[1-3].c data/sea-anemone-pal[1-5].c

SOURCES := main.c weave.c textscroll.c turmite.c logo.c growing-tree.c \
	   tilemover.c sea-anemone.c game-of-life.c vitruvian.c \
	   adpcm.asm pngraw.c
MAIN := #

PNG2C.bar := --bitmap bar,480x67x4 --palette bar_pal,16
PNG2C.bar-2 := --palette bar2_pal,16
PNG2C.stripes := --sprite arrow,128,2,+onlydata
PNG2C.colors := --palette stripes_pal,16
PNG2C.text-scroll-font := --bitmap font,768x8x1 --palette font_pal,2
PNG2C.turmite-credits-1 := --bitmap turmite_credits_1,256x256x1
PNG2C.turmite-credits-2 := --bitmap turmite_credits_2,256x256x1
PNG2C.turmite-credits-3 := --bitmap turmite_credits_3,256x256x1
PNG2C.turmite-pal := --palette turmite_pal,16
PNG2C.electric-lifeforms-01 := --bitmap electric_lifeforms,320x256x3 --palette electric_lifeforms_1_pal,8
PNG2C.electric-lifeforms-02 := --palette electric_lifeforms_2_pal,8
PNG2C.fruit := --bitmap fruit,16x16x1
PNG2C.tilemover-pal := --palette tilemover_pal,16
PNG2C.sea-anemone-pal1 := --palette sea_anemone_pal1,16
PNG2C.sea-anemone-pal2 := --palette sea_anemone_pal2,16
PNG2C.sea-anemone-pal3 := --palette sea_anemone_pal3,16
PNG2C.sea-anemone-pal4 := --palette sea_anemone_pal4,16
PNG2C.sea-anemone-pal5 := --palette sea_anemone_pal5,16
PNG2C.p46basedprng := --bitmap p46basedprng,156x75x1
PNG2C.electric-lifeforms-wireworld := --bitmap wireworld_logo,160x128x1
PNG2C.cell-gradient := --pixmap gradient,4x64x12
PNG2C.ghostown-logo-crop := --bitmap ghostown_logo,196x127x3,+shared
PNG2C.ghostown-logo-01 := --palette ghostown_logo_1_pal,8
PNG2C.ghostown-logo-02 := --palette ghostown_logo_2_pal,8
PNG2C.ghostown-logo-03 := --palette ghostown_logo_3_pal,8

CPPFLAGS += -DINTRO -DADPCM=$(ADPCM) -DDELTA=$(DELTA) -I.

data/%-electrons.c: data/%-electrons.png
	@echo "[PNG] $(DIR)$< -> $(DIR)$@"
	$(PYTHON3) $(ELECTRONS2C) $< $@ || (rm -f $@ && exit 1)

include $(TOPDIR)/build/effect.mk

data/%.ctr data/%.smp: data/%.mod
	$(PYTHON3) data/CinterConvert.py $^ $@ $(@:.ctr=.smp)

# You MUST pad the section to size that is determined by the converter script.
# Look for `total instrument memory` in the output.
OBJCOPY-CHIP := --rename-section .data=.datachip,alloc,load,data,contents \
	        --pad-to $(SMP_TOTALSIZE)

data/%.ctr.o: data/%.ctr
	@echo "[OBJCOPY] $(DIR)$< -> $(DIR)$@"
	$(OBJCOPY) -I binary -O amiga \
	  --redefine-sym _binary_data_$(subst -,_,$*)_ctr_start=_CinterModule \
	  --redefine-sym _binary_data_$(subst -,_,$*)_ctr_end=_CinterModuleEnd \
	  --redefine-sym _binary_data_$(subst -,_,$*)_ctr_size=_CinterModuleSize \
	  $^ $@

data/%-ADPCM.smp: data/%.smp
	@echo "[ADPCM] $(DIR)$< -> $(DIR)$@"
	$(PYTHON3) adpcm.py $< $@

data/%-Delta.smp: data/%.smp
	@echo "[DELTA] $(DIR)$< -> $(DIR)$@"
	$(PYTHON3) delta.py $< $@

data/%.smp.o: data/%.smp
	@echo "[OBJCOPY] $(DIR)$< -> $(DIR)$@"
	$(OBJCOPY) $(OBJCOPY-CHIP) -I binary -O amiga \
	  --redefine-sym _binary_data_$(subst -,_,$*)_smp_start=_CinterSamples \
	  --redefine-sym _binary_data_$(subst -,_,$*)_smp_end=_CinterSamplesEnd \
	  --redefine-sym _binary_data_$(subst -,_,$*)_smp_size=_CinterSamplesSize \
	  $^ $@

data/%.o: data/%.txt
	@echo "[OBJCOPY] $(DIR)$< -> $(DIR)$@"
	$(OBJCOPY) -I binary -O amiga \
	  --redefine-sym _binary_data_$(subst -,_,$*)_txt_start=_Text \
	  --redefine-sym _binary_data_$(subst -,_,$*)_txt_end=_TextEnd \
	  --redefine-sym _binary_data_$(subst -,_,$*)_txt_size=_TextSize \
	  $^ $@

data/%.png: data/%.rle
	@echo "[RLE] $(DIR)$< -> $(DIR)$@"
	$(PYTHON3) $(RLE2PNG) $< $@ || (rm -f $@ && exit 1)
