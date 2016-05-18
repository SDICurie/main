THIS_DIR   := $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST))))
T          := $(abspath $(THIS_DIR)/../../..)
OUT        := $(abspath $(T)/../out/current/quark_se/cos)
TOOLS_PATH ?= $(T)/tools
COS_LDS    ?= $(T)/bsp/bootable/cos/arc.lds

ARCH=arc
include $(T)/build/Makefile.toolchain

ifeq ($(BUILDVARIANT),release)
CFLAGS += -DNDEBUG=1
endif

CFLAGS += -g
CFLAGS += -I$(T)/bsp/include
CFLAGS += -I$(T)/bsp/bootable/cos/include
CFLAGS += -D__CPU_ARC__
CFLAGS += -nostartfiles
CFLAGS += -ffreestanding -nostartfiles -fno-exceptions
CFLAGS += -fno-asynchronous-unwind-tables
CFLAGS += -fdata-sections -ffunction-sections
# See later on if we can make this code position independent
#CFLAGS += -pie

CFLAGS += -mARCv2EM -mav2em
CFLAGS += -Werror

.PHONY: FORCE

_OBJS = main.o crt_init.o startup.o gpio_fuel_gauge_enable.o
OBJS = $(patsubst %,$(OUT)/%,$(_OBJS))

$(OUT)/%.o: %.c
	@echo $(ANSI_RED)"[cPP]"$(ANSI_OFF) $@
	$(AT)$(CC) $(CFLAGS) -I$(PROJECT_INCLUDES) -c -o $@ $<

$(OUT)/%.o: %.S
	@echo $(ANSI_RED)"[cPP]"$(ANSI_OFF) $@
	$(AT)$(CC) $(CFLAGS) -I$(PROJECT_INCLUDES) -c -o $@ $<

$(OUT)/cos.lds: $(COS_LDS) | $(OUT)
	@echo $(ANSI_RED)"[cPP]"$(ANSI_OFF) $@
	$(AT)$(CC) $(CFLAGS) -E -P -o $@ -ansi -D__ASSEMBLY__  -x assembler-with-cpp -P $< -I$(PROJECT_INCLUDES)

$(OUT)/cos.a: $(OBJS)
	echo $(OBJS)

$(OUT)/cos.elf: $(OUT)/cos.lds $(OBJS)
	@echo $(ANSI_RED)"[cLD]"$(ANSI_OFF) $@
	$(AT)$(LD) -o $@ \
		-T $(OUT)/cos.lds \
		$(OBJS) \
		-Map $(OUT)/cos.map

$(OUT)/cos.bin: $(OUT)/cos.elf
	@echo $(ANSI_RED)"[cX]"$(ANSI_OFF) $@
	$(AT)$(OBJCOPY) -O binary $< $@

$(OUT)/cos.c: $(OUT)/cos.bin
	@echo $(ANSI_RED)"[cXXD]"$(ANSI_OFF) $@
	$(AT)cd $(OUT) && xxd -i cos.bin $@

.PHONY: cos
cos: $(OUT)/cos.bin $(OUT)/cos.c | $(OUT)

image: cos

all: image

clean:
	rm -f $(OBJS) $(OUT)/cos.*
