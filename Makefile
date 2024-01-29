BUILD_DIR=build
include $(N64_INST)/include/n64.mk

src = pi_dma_test.c
obj = $(src:%.c=$(BUILD_DIR)/%.o) $(asm:%.S=$(BUILD_DIR)/%.o)

assets = $(wildcard data/*.log)
assets_conv = $(addprefix filesystem/,$(notdir $(assets:%.log=%.log)))

all: pi_dma_test.z64

filesystem/%.log: data/%.log
	@mkdir -p $(dir $@)
	@echo "    [DATA] $@"
	@cp $< $@
	$(N64_BINDIR)/mkasset -o filesystem $<

pi_dma_test.z64: $(BUILD_DIR)/pi_dma_test.dfs
pi_dma_test.z64: N64_ROM_TITLE="PI DMA Test"
$(BUILD_DIR)/pi_dma_test.dfs: $(assets_conv)
$(BUILD_DIR)/pi_dma_test.elf: $(obj)

clean:
	rm -f $(obj)
	rm -f $(obj_makerom)
	rm -f $(asm:%.S=$(BUILD_DIR)/%.o)
	rm -f $(rsp:%.S=$(BUILD_DIR)/%.rsp)
	rm -f $(src:%.c=$(BUILD_DIR)/%.d) $(asm:%.S=$(BUILD_DIR)/%.d)

-include $(src:%.c=$(BUILD_DIR)/%.d) $(asm:%.S=$(BUILD_DIR)/%.d)

.PHONY: clean test
