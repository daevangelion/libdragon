BUILD_DIR=build
LIBDRAGON_PREVIEW=2
ifeq ($(N64_TLBCODE),1)
N64_TLBCODE_UNITS ?= tlbcode-units.json
endif
include $(N64_INST)/include/n64.mk

all: tlbcode.z64

# Supply a real videoconv64 H.264 asset, e.g. CLIP=/path/to/clip.h264.
filesystem/clip.h264: $(CLIP)
	@test -n "$(CLIP)" || { echo 'Set CLIP to a videoconv64 H.264 asset'; exit 1; }
	@mkdir -p filesystem
	cp "$<" "$@"

$(BUILD_DIR)/tlbcode.dfs: filesystem/clip.h264
$(BUILD_DIR)/tlbcode.elf: $(BUILD_DIR)/tlbcode.o
tlbcode.z64: N64_ROM_TITLE="TLB Code Units"
tlbcode.z64: $(BUILD_DIR)/tlbcode.dfs

-include $(wildcard $(BUILD_DIR)/*.d)
.PHONY: all
