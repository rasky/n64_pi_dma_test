#include <libdragon.h>
#include <memory.h>

/* 1 = generate logfiles on SD; 0 = run the test */
#define MODE_GENERATE     0

uint8_t ram_buffer[512] __attribute__((aligned(8)));
uint8_t log_buffer[512] __attribute__((aligned(8)));

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

	for (int ram_offs = 0; ram_offs < 8; ram_offs++) {
		for (int rom_offs = 0; rom_offs < 2; rom_offs++) {

			FILE *log; char logfn[256];

			#if MODE_GENERATE
			sprintf(logfn, "sd:/pidma_ram%x_rom%x.log", ram_offs, rom_offs);
			log = fopen(logfn, "wb");
			assertf(log, "Cannot create logfile: %s", logfn);
			#else
			sprintf(logfn, "rom:/pidma_ram%x_rom%x.log", ram_offs, rom_offs);
			log = fopen(logfn, "rb");
			assertf(log, "Cannot open logfile: %s", logfn);
			#endif

			printf("Offsets: RAM=%d, ROM=%d...\n", ram_offs, rom_offs);

			for (int sz = 1; sz < 384; sz++) {
				memset(ram_buffer, 0xAA, sizeof(ram_buffer));
				data_cache_hit_writeback_invalidate(ram_buffer, sizeof(ram_buffer));

				dma_read(ram_buffer+ram_offs, rom_buffer+rom_offs, sz);

				#if MODE_GENERATE
					fwrite(ram_buffer, 1, sizeof(ram_buffer), log);
				#else
					_Static_assert(sizeof(log_buffer) == sizeof(ram_buffer), "different buffer sizes");
					int read = fread(log_buffer, 1, sizeof(log_buffer), log);
					assertf(read == sizeof(log_buffer), "invalid log file size");
					for (int i=0;i<sizeof(ram_buffer);i++) {
						if (log_buffer[i] != ram_buffer[i]) {
							printf("ERROR with DMA size %d.\n", sz);

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
							nfails++;

							printf("Skipping rest of this test.\n\n");
							sz=384;
							break;
						}
					}
				#endif
			}

			fclose(log);
		}
	}

	printf("Test finished\n");
	if (nfails)
		printf("%d failures\n", nfails);
	else
		printf("SUCCESS\n");
}
