## N64 PI DMA tests

These tests bruteforce Nintendo 64 PI DMA transfers with all kind of sizes and
alignments to show all the weirdest behaviors.

The test verifies the output against golden files that were acquired on real
hardware, so they are verified to be correct.

You can refer to this document for information on the findings:
https://n64brew.dev/wiki/Peripheral_Interface#Unaligned_DMA_transfer


### How to compile

Install [libdragon-docker](https://github.com/anacierdem/libdragon-docker). Then
just run:

	$ libdragon make

This will build the test ROM (`pi_dma_test.z64`). 

If you want to regenerate the golden files on real hardware, change
`MODE_GENERATE` to `1` at the top of `pi_dma_test.c`, and recompile. The generate
ROM will require a 64drive development kit with a SD card to save the golden
files on the SD.
