#include <libdragon.h>
#include <memory.h>
#include <stdbool.h>

/* 1 = generate logfiles on SD; 0 = run the test */
#define MODE_GENERATE     0

#define BUFFER_SIZE       512

uint8_t *ram_buffer  = (uint8_t*)0x80300000;
uint8_t *ram_buffer2 = (uint8_t*)0x80310000;
uint8_t *log_buffer  = (uint8_t*)0x80320000;

volatile uint32_t * const PI_regs = (volatile uint32_t*)0xa4600000;

/* A copy of DMA read from libdragon, that also times the transfer. */
uint32_t my_dma_read(void * ram_address, unsigned long pi_address, unsigned long len) 
{
    assert(len > 0);

    disable_interrupts();

    while (dma_busy()) ;
    MEMORY_BARRIER();
    PI_regs[0] = (uint32_t)ram_address;
    MEMORY_BARRIER();
    PI_regs[1] = (pi_address | 0x10000000) & 0x1FFFFFFF;
    MEMORY_BARRIER();
    PI_regs[3] = len-1;
    MEMORY_BARRIER();

    volatile uint32_t t0 = TICKS_READ();
    MEMORY_BARRIER();
    while (dma_busy()) ;
    MEMORY_BARRIER();
    volatile uint32_t t1 = TICKS_READ();
    MEMORY_BARRIER();

    enable_interrupts();

    return TICKS_DISTANCE(t0, t1);
}

void my_dma_read_len_only(unsigned long len) {
    assert(len > 0);

    disable_interrupts();
    while (dma_busy()) ;
    MEMORY_BARRIER();
    PI_regs[3] = len-1;
    MEMORY_BARRIER();
    while (dma_busy()) ;
    enable_interrupts();
}


bool compare_buffer(uint8_t *ram_buffer, FILE *log, int sz, const char *errmsg) {
	int read = fread(log_buffer, 1, BUFFER_SIZE, log);
	assertf(read == BUFFER_SIZE, "invalid log file size");
	for (int i=0;i<BUFFER_SIZE;i++) {
		if (log_buffer[i] != ram_buffer[i]) {
			printf("ERROR with DMA size %d: %s.\n", sz, errmsg);

			int base = (i/16)*16;
			printf("Found:  %04x  ", base);
			for (int j=0;j<16;j++) printf("%02x ", ram_buffer[base+j]);
			printf("\n");
			printf("Expect: %04x  ", base);
			for (int j=0;j<16;j++) printf("%02x ", log_buffer[base+j]);
			printf("\n");
			printf("              ");
			for (int j=0;j<16;j++) printf("%s ", log_buffer[base+j] == ram_buffer[base+j] ? "  " : "^^");
			printf("\n");
			return false;
		}
	}
	return true;
}

int main(void) {
	init_interrupts();
	console_init();
	console_set_debug(true);
	debug_init_usblog();
	debug_init_isviewer();

	if (dfs_init(DFS_DEFAULT_LOCATION) != DFS_ESUCCESS) {
		assertf(0, "Cannot initialize DFS");
	}

	uint32_t rom_buffer = dfs_rom_addr("counter.dat");
	assertf(rom_buffer != 0, "counter.dat not found in DFS");

	#if MODE_GENERATE
	printf("PI_DMA_TEST -- Generation mode\n\n");
	printf("This program requires a 64drive with a SD card.\n");
	printf("Make sure a SD is inserted or the program will crash.\n\n");
	printf("Generation will start in 1 second...\n");
	wait_ms(1000);

	if (!debug_init_sdfs("sd:/", 0)) {
		assertf(0, "Cannot initialize SDFS");
	}
	#else
	printf("PI_DMA_TEST -- Running test\n\n");
	#endif

	int nfails = 0;
	int nfailstimings = 0;
	bool check_timings = true;

	for (int ram_offs = 0; ram_offs < 16; ram_offs++) {
		for (int rom_offs = 0; rom_offs < 2; rom_offs++) {

			FILE *log; char logfn[256];

			#if MODE_GENERATE
			sprintf(logfn, "sd:/pidma_ram%d_rom%d.log", ram_offs, rom_offs);
			log = fopen(logfn, "wb");
			assertf(log, "Cannot create logfile: %s", logfn);
			#else
			sprintf(logfn, "rom:/pidma_ram%d_rom%d.log", ram_offs, rom_offs);
			log = fopen(logfn, "rb");
			assertf(log, "Cannot open logfile: %s", logfn);
			#endif

			printf("Offsets: RAM=%d, ROM=%d...\n", ram_offs, rom_offs);

			for (int sz = 1; sz < 384; sz++) {
				memset(ram_buffer, 0xAA, BUFFER_SIZE);
				data_cache_hit_writeback_invalidate(ram_buffer, BUFFER_SIZE);

				// Do 8 runs to find min/max range
				int nruns = MODE_GENERATE ? 8 : 4;
				uint16_t minticks = 65535, maxticks = 0;
				uint16_t avgticks = 0;
				for (int r=0;r<nruns;r++) {
					uint16_t ticks = my_dma_read(ram_buffer+ram_offs, rom_buffer+rom_offs, sz);
					if (ticks < minticks) minticks = ticks;
					if (ticks > maxticks) maxticks = ticks;
					avgticks += ticks;
				}
				avgticks /= nruns;

				uint32_t post_dst = PI_regs[0];
				uint32_t post_src = PI_regs[1];
				uint32_t post_len = PI_regs[3];

				// Save the buffer
				memcpy(ram_buffer2, ram_buffer, BUFFER_SIZE);

				// Do a second DMA transfer of 8 bytes without touching the
				// src/dst registers.
				data_cache_hit_writeback_invalidate(ram_buffer, BUFFER_SIZE);
				my_dma_read_len_only(8);

				#if MODE_GENERATE
					fwrite(ram_buffer2, 1, BUFFER_SIZE, log);
					fwrite(&minticks, 1, 2, log);
					fwrite(&maxticks, 1, 2, log);
					fwrite(&post_dst, 1, 4, log);
					fwrite(&post_src, 1, 4, log);
					fwrite(&post_len, 1, 4, log);
					fwrite(ram_buffer, 1, BUFFER_SIZE, log);
				#else
					if (!compare_buffer(ram_buffer2, log, sz, "first transfer")) {
						nfails++;
						printf("Skipping rest of this test\n\n");
						break;
					}

					uint16_t exp_minticks, exp_maxticks;
					uint32_t exp_post_src, exp_post_dst, exp_post_len;
					fread(&exp_minticks, 1, 2, log);
					fread(&exp_maxticks, 1, 2, log);
					fread(&exp_post_dst, 1, 4, log);
					fread(&exp_post_src, 1, 4, log);
					fread(&exp_post_len, 1, 4, log);

					if (check_timings) {					
						if (avgticks < exp_minticks*0.9 || avgticks > exp_maxticks*1.1) {
							printf("ERROR on timing of DMA of size %d.\n", sz);
							printf("Found:  %d\n", avgticks);
							printf("Expect: [%d .. %d]\n", exp_minticks, exp_maxticks);
							printf("\n");
							nfails++;
							if (++nfailstimings == 8) {
								printf("FATAL: Too many timing errors, disabling checks");
								check_timings = false;
							}
						}
					}

					if (exp_post_dst != post_dst) {
						printf("ERROR on PI_DRAM_ADDR_REG after DMA of size %d.\n", sz);
						printf("Found:  %08lx\n", post_dst);
						printf("Expect: %08lx\n", exp_post_dst);
						printf("\n");
						nfails++;
					}
					if (exp_post_src != post_src) {
						printf("ERROR on PI_CART_ADDR_REG after DMA of size %d.\n", sz);
						printf("Found:  %08lx\n", post_src);
						printf("Expect: %08lx\n", exp_post_src);
						printf("\n");
						nfails++;
					}
					if (exp_post_len != post_len) {
						printf("ERROR on PI_WR_LEN_REG after DMA of size %d.\n", sz);
						printf("Found:  %08lx\n", post_len);
						printf("Expect: %08lx\n", exp_post_len);
						printf("\n");
						nfails++;
					}

					if (!compare_buffer(ram_buffer, log, sz, "followup transfer of 8 bytes")) {
						nfails++;
						printf("Skipping rest of this test\n\n");
						break;
					}
				#endif
			}

			if (log) fclose(log);
		}
	}

	printf("Test finished\n");
	if (nfails)
		printf("%d failures\n", nfails);
	else
		printf("SUCCESS\n");
}
