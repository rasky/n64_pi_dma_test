BUILD_DIR = build

include n64rasky.mk

src = pi_dma_test.c
rsp = 
asm = 
obj = $(src:%.c=$(BUILD_DIR)/%.o) $(asm:%.S=$(BUILD_DIR)/%.o) $(rsp:%.S=$(BUILD_DIR)/%.text.o) $(rsp:%.S=$(BUILD_DIR)/%.data.o)

all: pi_dma_test.z64

pi_dma_test.dfs: dfs/
pi_dma_test.z64: pi_dma_test.dfs
pi_dma_test.z64: N64_ROM_TITLE="MVS64: ${ROMNAME}"
pi_dma_test.elf: $(obj)

mvsmakerom: $(obj_makerom)
	@echo "    [LD] $@"
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(obj)
	rm -f $(obj_makerom)
	rm -f $(asm:%.S=$(BUILD_DIR)/%.o)
	rm -f $(rsp:%.S=$(BUILD_DIR)/%.rsp)
	rm -f $(src:%.c=$(BUILD_DIR)/%.d) $(asm:%.S=$(BUILD_DIR)/%.d)
	rm -f mvs64-*.elf
	rm -f mvs64-*.dfs
	rm -f mvs64-*.z64

-include $(src:%.c=$(BUILD_DIR)/%.d) $(asm:%.S=$(BUILD_DIR)/%.d) $(src_makerom:%.c=$(BUILD_DIR)/%.d)

.PHONY: clean test
