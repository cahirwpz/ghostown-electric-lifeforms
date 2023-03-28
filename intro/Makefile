TOPDIR := $(realpath ..)

DELTA := 0
VQ := 0
KLANG := 0

MODULE := JazzCat-DerKlang

RLE2PNG = rle2png.py
ELECTRONS2C = electrons2c.py

OBJECTS := data/$(MODULE).trk.o data/text-scroll.o

ifeq ($(DELTA), 1)
OBJECTS += data/$(MODULE)-Delta.smp.o
else ifeq ($(VQ), 1)
OBJECTS += data/$(MODULE)-VQ.smp.o
else ifeq ($(KLANG), 1)
OBJECTS += data/$(MODULE).o 
else
OBJECTS += data/$(MODULE).smp.o
endif

LIBS := libpt

BUILD-FILES := audiovq

CLEAN-FILES := main.o data/bar.c data/stripes.c data/colors.c \
	data/$(MODULE)*.o data/$(MODULE).{smp,trk}* data/text-scroll-font.c \
	data/intro.c data/electric-lifeforms-0[1-2].c \
	data/fruit-*.c data/anemone-gradient-pal.c \
	data/ghostown-logo-crop-px.c \
	data/logo-electrons.c data/electric-lifeforms.c \
	data/ghostown-logo-0[1-3].c \
	data/wireworld-fullscreen.c \
	data/wireworld-fullscreen-electrons.c \
	data/wireworld-vitruvian.c \
	data/wireworld-vitruvian-electrons.c \
	data/turmite-credits-[1-3].c \
	data/circles.c data/squares.c \
	data/grass-[1-2].c data/loader.c \
	data/texture-{in,out}side.c data/gradient.c data/gradient.png \
	data/tilemover-*.c data/chip.c data/wireworld_pcb_pal.c \
	data/pal-gold*.c data/pal-green*.c data/pal-red*.c data/pal-blue*.c

SOURCES := main.c weave.c textscroll.c turmite.c logo.c growing-tree.c \
	   tilemover.c sea-anemone.c game-of-life.c vitruvian.c uvmap.c \
	   samples.c palettes.c
MAIN := #

PNG2C.bar := --pixmap bar,480x67x4,+limit_bpp --palette bar_pal,32
PNG2C.stripes := --bitmap stripes,128x64x2,+limit_depth,+interleaved,-displayable --palette stripes_pal,16
PNG2C.text-scroll-font := --bitmap font,768x8x1,-displayable --palette font_pal,2
PNG2C.turmite-credits-1 := --bitmap turmite_credits_1,256x256x1,+onlydata,-displayable
PNG2C.turmite-credits-2 := --bitmap turmite_credits_2,256x256x1,+onlydata,-displayable
PNG2C.turmite-credits-3 := --bitmap turmite_credits_3,256x256x1,+onlydata,-displayable
PNG2C.electric-lifeforms-01 := --pixmap electric_lifeforms,320x256x4 --palette electric_lifeforms_1_pal,8
PNG2C.electric-lifeforms-02 := --palette electric_lifeforms_2_pal,8
PNG2C.fruit-1 := --bitmap fruit_1,16x16x2,+onlydata --palette fruit_1_pal,4
PNG2C.fruit-2 := --bitmap fruit_2,16x16x2,+onlydata --palette fruit_2_pal,4
PNG2C.grass-1 := --sprite grass_1,32,8 --palette grass_1_pal,4
PNG2C.grass-2 := --sprite grass_2,32,8 --palette grass_2_pal,4
PNG2C.tilemover-pal := --palette tilemover_pal,16
PNG2C.ghostown-logo-crop-px := --pixmap ghostown_logo_px,192x131x4
PNG2C.ghostown-logo-01 := --palette ghostown_logo_1_pal,8
PNG2C.ghostown-logo-02 := --palette ghostown_logo_2_pal,8
PNG2C.ghostown-logo-03 := --palette ghostown_logo_3_pal,8
ELECTRONS2C.wireworld-fullscreen := --name pcb_electrons
PNG2C.wireworld-fullscreen := --bitmap wireworld_pcb,160x128x1,+limit_depth
ELECTRONS2C.wireworld-vitruvian := --name vitruvian_electrons
PNG2C.wireworld-vitruvian := --bitmap wireworld_vitruvian,160x128x1,+limit_depth
PNG2C.chip := --sprite wireworld_chip,64,4,+attached --palette wireworld_chip_pal,16
PNG2C.wireworld-pcb-pal := --pixmap wireworld_pcb_pal,4x32x12
PNG2C.loader := --bitmap loader,64x64x1 --palette loader_pal,2

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

PNG2C.squares := \
	--bitmap square1,3x3x1,extract_at=0x0 \
	--bitmap square2,5x5x1,extract_at=0x4 \
	--bitmap square3,7x7x1,extract_at=0x10 \
	--bitmap square4,9x9x1,extract_at=0x18 \
	--bitmap square5,11x11x1,extract_at=0x28 \
	--bitmap square6,13x13x1,extract_at=0x40 \
	--bitmap square7,15x15x1,extract_at=0x54 \
	--bitmap square8,17x17x1,extract_at=0x70 \
	--bitmap square9,19x19x1,extract_at=0x88 \
	--bitmap square10,21x21x1,extract_at=0x108 \
	--bitmap square11,23x23x1,extract_at=0x130 \
	--bitmap square12,25x25x1,extract_at=0x154 \
	--bitmap square13,27x27x1,extract_at=0x180 \
	--bitmap square14,29x29x1,extract_at=0x208 \
	--bitmap square15,31x31x1,extract_at=0x238 \
	--bitmap square16,33x33x1,extract_at=0x270

PNG2C.pal-gold := --palette pal_gold,16,+shared
PNG2C.pal-gold-dark := --palette pal_gold_dark,16,+shared
PNG2C.pal-gold-light := --palette pal_gold_light,16,+shared

PNG2C.pal-blue := --palette pal_blue,16,+shared
PNG2C.pal-blue-dark := --palette pal_blue_dark,16,+shared
PNG2C.pal-blue-light := --palette pal_blue_light,16,+shared

PNG2C.pal-green := --palette pal_green,16,+shared
PNG2C.pal-green-dark := --palette pal_green_dark,16,+shared
PNG2C.pal-green-light := --palette pal_green_light,16,+shared

PNG2C.pal-red := --palette pal_red,16,+shared
PNG2C.pal-red-dark := --palette pal_red_dark,16,+shared
PNG2C.pal-red-light := --palette pal_red_light,16,+shared

PNG2C.anemone-gradient-pal := --palette anemone_gradient_pal,16

PNG2C.tilemover-windmills := --bitmap tilemover_windmills,32x32x1
PNG2C.tilemover-wave := --bitmap tilemover_wave,192x64x1
PNG2C.tilemover-drops := --bitmap tilemover_drops,16x16x1
PNG2C.tilemover-block := --bitmap tilemover_block,96x80x1
PNG2C.tilemover-bg-pal := --palette tilemover_bg_pal,6

PNG2C.texture-inside := --pixmap texture_in,64x64x8 --palette texture_pal,16
PNG2C.texture-outside := --pixmap texture_out,64x64x8
PNG2C.gradient := --pixmap gradient,16x16x12

CPPFLAGS += -DINTRO -DVQ=$(VQ) -DDELTA=$(DELTA) -DKLANG=$(KLANG) -I.

data/wireworld-%-electrons.c: data/wireworld-%.png
	@echo "[PNG] $(DIR)$< -> $(DIR)$@"
	$(PYTHON3) $(ELECTRONS2C) $(ELECTRONS2C.wireworld-$*) $< $@ || (rm -f $@ && exit 1)

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

data/gradient.png: $(GRADIENT) data/texture-inside.png
	@echo "[GEN] $@"
	$(PYTHON3) $(GRADIENT) load $(word 2,$^) copy hue 0.25 save $@

audiovq: audiovq.c
	@echo "[HOSTCC] $(DIR)$< -> $(DIR)$@"
	cc -O2 -march=native -Wall -o $@ $^ -lm
