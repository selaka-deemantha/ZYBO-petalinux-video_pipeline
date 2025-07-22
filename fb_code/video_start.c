#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>

#define VDMA_BASE      0x43000000
#define VTC_BASE       0x43C00000
#define DYNCLK_BASE    0x43C10000

#define FRAMEBUFFER_ADDR 0x10000000  // Adjust accordingly

#define PAGE_SIZE 4096
#define MAP_SIZE PAGE_SIZE

// Helper macros for register offsets from Xilinx docs
#define VDMA_MM2S_CONTROL       0x00
#define VDMA_MM2S_STATUS        0x04
#define VDMA_MM2S_FRMDLY_STRIDE 0x58
#define VDMA_MM2S_START_ADDR    0x5C

#define VTC_CONTROL             0x00
#define VTC_SYNC_ENABLE         0x08

#define DYNCLK_CTRL             0x00
#define DYNCLK_FREQ             0x08

volatile uint32_t *map_device(off_t base) {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if(fd < 0) {
        perror("open /dev/mem");
        return NULL;
    }
    volatile uint32_t *map = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, base);
    if(map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return NULL;
    }
    close(fd);
    return map;
}

void unmap_device(volatile uint32_t *map) {
    munmap((void*)map, MAP_SIZE);
}

int main() {
    printf("main functions is starting\n");
    volatile uint32_t *vdma = map_device(VDMA_BASE);
    volatile uint32_t *vtc = map_device(VTC_BASE);
    volatile uint32_t *dynclk = map_device(DYNCLK_BASE);

    if(!vdma || !vtc || !dynclk) {
        printf("Failed to map devices\n");
        return -1;
    }

    // 1. Setup dynamic clock generator (example: enable and set pixel clock)
    dynclk[DYNCLK_CTRL/4] = 0x1;          // Enable dynamic clock
    dynclk[DYNCLK_FREQ/4] = 74250000;     // Set pixel clock frequency to 74.25 MHz (may vary)
    usleep(100000);                       // wait ~100ms for clock to stabilize

    // 2. Setup VTC (enable timing generator)
    vtc[VTC_CONTROL/4] = 0x1;             // Enable VTC generator
    vtc[VTC_SYNC_ENABLE/4] = 0x1;         // Enable sync signals

    // 3. Setup VDMA MM2S (memory to stream) channel
    vdma[VDMA_MM2S_CONTROL/4] = 0x0;      // Reset
    usleep(10000);
    vdma[VDMA_MM2S_FRMDLY_STRIDE/4] = 1280 * 4; // 1280 pixels * 4 bytes per pixel (assuming 32bpp)
    vdma[VDMA_MM2S_START_ADDR/4] = FRAMEBUFFER_ADDR;
    vdma[VDMA_MM2S_CONTROL/4] = 0x00000003; // Start, Circular mode

    printf("Video pipeline started\n");

    // Keep running
    while(1) sleep(1);

    unmap_device(vdma);
    unmap_device(vtc);
    unmap_device(dynclk);

    return 0;
}
