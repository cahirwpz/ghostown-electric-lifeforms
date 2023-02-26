TOPDIR := $(realpath ..)

DELTA := 1
VQ := 0

MODULE := JazzCat-ElectricLifeforms
SMP_TOTALSIZE := 181610

RLE2PNG = rle2png.py
ELECTRONS2C = electrons2c.py

OBJECTS := data/$(MODULE).trk.o data/text-scroll.o

ifeq ($(DELTA), 1)
OBJECTS += data/$(MODULE)-Delta.smp.o
else ifeq ($(VQ), 1)
OBJECTS += data/$(MODULE)-VQ.smp.o
else
OBJECTS += data/$(MODULE).smp.o
endif

LIBS := libpt

BUILD-FILES := audiovq

CLEAN-FILES := main.o data/bar.c data/stripes.c data/colors.c \
	data/$(MODULE)-*.o data/$(MODULE).{smp,trk}* data/text-scroll-font.c \
	data/intro.c data/electric-lifeforms-0[1-2].c \
	data/fruit.c data/tilemover-pal.c data/anemone-pal.c \
	data/ghostown-logo-crop.c data/p46basedprng.c data/cell-gradient.c \
	data/logo-electrons.c data/electric-lifeforms.c \
	data/electric-lifeforms-wireworld.c data/ghostown-logo-0[1-3].c \
	data/turmite-credits-[1-3].c data/anemone-pal-[1-3].c \
	data/turmite-pal-[1-3].c data/circles.c

SOURCES := main.c weave.c textscroll.c turmite.c logo.c growing-tree.c \
	   tilemover.c sea-anemone.c game-of-life.c vitruvian.c
MAIN := #

PNG2C.bar := --bitmap bar,480x67x4,+limit_depth --palette bar_pal,32
PNG2C.stripes := --bitmap stripes,128x64x2,+limit_depth,+interleaved,-displayable --palette stripes_pal,16
PNG2C.text-scroll-font := --bitmap font,768x8x1,-displayable --palette font_pal,2
PNG2C.turmite-credits-1 := --bitmap turmite_credits_1,256x256x1,+onlydata,-displayable
PNG2C.turmite-credits-2 := --bitmap turmite_credits_2,256x256x1,+onlydata,-displayable
PNG2C.turmite-credits-3 := --bitmap turmite_credits_3,256x256x1,+onlydata,-displayable
PNG2C.turmite-pal := --palette turmite_pal,16
PNG2C.electric-lifeforms-01 := --bitmap electric_lifeforms,320x256x3,-displayable --palette electric_lifeforms_1_pal,8
PNG2C.electric-lifeforms-02 := --palette electric_lifeforms_2_pal,8
PNG2C.fruit := --bitmap fruit,16x16x1
PNG2C.tilemover-pal := --palette tilemover_pal,16
PNG2C.p46basedprng := --bitmap p46basedprng,156x75x1
PNG2C.electric-lifeforms-wireworld := --bitmap wireworld_logo,160x128x1
PNG2C.cell-gradient := --pixmap gradient,4x64x12
PNG2C.ghostown-logo-crop := --bitmap ghostown_logo,196x127x3,+shared
PNG2C.ghostown-logo-01 := --palette ghostown_logo_1_pal,8
PNG2C.ghostown-logo-02 := --palette ghostown_logo_2_pal,8
PNG2C.ghostown-logo-03 := --palette ghostown_logo_3_pal,8
PNG2C.circles := \
	--bitmap circle1,3x3x1,extract_at=0x0 \
	--bitmap circle2,5x5x1,extract_at=0x4 \
	--bitmap circle3,7x7x1,extract_at=0x10 \
	--bitmap circle4,9x9x1,extract_at=0x18 \
	--bitmap circle5,11x11x1,extract_at=0x28 \
	--bitmap circle6,13x13x1,extract_at=0x40 \
	--bitmap circle7,15x15x1,extract_at=0x54 \
	--bitmap circle8,17x17x1,extract_at=0x70 \
	--bitmap circle9,19x19x1,extract_at=0x88 \
	--bitmap circle10,21x21x1,extract_at=0x108 \
	--bitmap circle11,23x23x1,extract_at=0x130 \
	--bitmap circle12,25x25x1,extract_at=0x154 \
	--bitmap circle13,27x27x1,extract_at=0x180 \
	--bitmap circle14,29x29x1,extract_at=0x208 \
	--bitmap circle15,31x31x1,extract_at=0x238 \
	--bitmap circle16,33x33x1,extract_at=0x270

PNG2C.turmite-pal-1 := --palette turmite_pal_1,16
PNG2C.turmite-pal-1-dark := --palette turmite_pal_1_dark,16
PNG2C.turmite-pal-1-light := --palette turmite_pal_1_light,16
PNG2C.turmite-pal-2 := --palette turmite_pal_2,16
PNG2C.turmite-pal-2-dark := --palette turmite_pal_2_dark,16
PNG2C.turmite-pal-2-light := --palette turmite_pal_2_light,16
PNG2C.turmite-pal-3 := --palette turmite_pal_3,16
PNG2C.turmite-pal-3-dark := --palette turmite_pal_3_dark,16
PNG2C.turmite-pal-3-light := --palette turmite_pal_3_light,16

PNG2C.anemone-pal-1 := --palette anemone_pal_1,16
PNG2C.anemone-pal-1-dark := --palette anemone_pal_1_dark,16
PNG2C.anemone-pal-1-light := --palette anemone_pal_1_light,16
PNG2C.anemone-pal-2 := --palette anemone_pal_2,16
PNG2C.anemone-pal-2-dark := --palette anemone_pal_2_dark,16
PNG2C.anemone-pal-2-light := --palette anemone_pal_2_light,16
PNG2C.anemone-pal-3 := --palette anemone_pal_3,16
PNG2C.anemone-pal-3-dark := --palette anemone_pal_3_dark,16
PNG2C.anemone-pal-3-light := --palette anemone_pal_3_light,16

CPPFLAGS += -DINTRO -DVQ=$(VQ) -DDELTA=$(DELTA) -I.

data/%-electrons.c: data/%-electrons.png
	@echo "[PNG] $(DIR)$< -> $(DIR)$@"
	$(PYTHON3) $(ELECTRONS2C) $< $@ || (rm -f $@ && exit 1)

include $(TOPDIR)/build/effect.mk

data/%.trk data/%.smp: data/%.mod
	$(PYTHON3) data/ptsplit.py $^
	mv data/$*.mod.trk data/$*.trk
	mv data/$*.mod.smp data/$*.smp

OBJCOPY-CHIP := --rename-section .data=.datachip,alloc,load,data,contents

data/%.trk.o: data/%.trk
	@echo "[OBJCOPY] $(DIR)$< -> $(DIR)$@"
	$(OBJCOPY) -I binary -O amiga \
	  --redefine-sym _binary_data_$(subst -,_,$*)_trk_start=_Module \
	  --redefine-sym _binary_data_$(subst -,_,$*)_trk_end=_ModuleEnd \
	  --redefine-sym _binary_data_$(subst -,_,$*)_trk_size=_ModuleSize \
	  $^ $@

data/%-Delta.smp: data/%.smp
	@echo "[DELTA] $(DIR)$< -> $(DIR)$@"
	$(PYTHON3) delta.py $< $@

data/%-VQ.smp: data/%.smp audiovq
	@echo "[AUDIOVQ] $(DIR)$< -> $(DIR)$@"
	./audiovq $< $@

data/%.smp.o: data/%.smp
	@echo "[OBJCOPY] $(DIR)$< -> $(DIR)$@"
	$(OBJCOPY) $(OBJCOPY-CHIP) -I binary -O amiga \
	  --redefine-sym _binary_data_$(subst -,_,$*)_smp_start=_Samples \
	  --redefine-sym _binary_data_$(subst -,_,$*)_smp_end=_SamplesEnd \
	  --redefine-sym _binary_data_$(subst -,_,$*)_smp_size=_SamplesSize \
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

audiovq: audiovq.c
	@echo "[HOSTCC] $(DIR)$< -> $(DIR)$@"
	cc -O2 -march=native -Wall -o $@ $^ -lm
