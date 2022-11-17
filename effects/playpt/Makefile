TOPDIR := $(realpath ../..)

OBJECTS := data/virgill-colombia.o
CLEAN-FILES := data/lat2-08.c

LIBS := libpt

PSF2C.lat2-08 := --name latin2 --type console

include $(TOPDIR)/build/effect.mk

data/%.o: data/%.mod
	@echo "[OBJCOPY] $(DIR)$< -> $(DIR)$@"
	$(OBJCOPY) -I binary -O amiga \
	  --rename-section .data=.datachip,alloc,load,data,contents \
	  --redefine-sym _binary_data_$(subst -,_,$*)_start=_PtModule \
	  --redefine-sym _binary_data_$(subst -,_,$*)_end=_PtModuleEnd \
	  --redefine-sym _binary_data_$(subst -,_,$*)_size=_PtModuleSize \
	  $^ $@
