BUILD_DIR=build
include $(N64_INST)/include/n64.mk

src = pi_dma_test.c
rsp = 
asm = 
obj = $(src:%.c=$(BUILD_DIR)/%.o) $(asm:%.S=$(BUILD_DIR)/%.o) $(rsp:%.S=$(BUILD_DIR)/%.text.o) $(rsp:%.S=$(BUILD_DIR)/%.data.o)

all: pi_dma_test.z64

pi_dma_test.z64: $(BUILD_DIR)/pi_dma_test.dfs
pi_dma_test.z64: N64_ROM_TITLE="PI DMA Test"
$(BUILD_DIR)/pi_dma_test.dfs: data/
$(BUILD_DIR)/pi_dma_test.elf: $(obj)

clean:
	rm -f $(obj)
	rm -f $(obj_makerom)
	rm -f $(asm:%.S=$(BUILD_DIR)/%.o)
	rm -f $(rsp:%.S=$(BUILD_DIR)/%.rsp)
	rm -f $(src:%.c=$(BUILD_DIR)/%.d) $(asm:%.S=$(BUILD_DIR)/%.d)

-include $(src:%.c=$(BUILD_DIR)/%.d) $(asm:%.S=$(BUILD_DIR)/%.d)

.PHONY: clean test
