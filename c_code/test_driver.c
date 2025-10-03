#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#define DEVICE_PATH "/dev/fft_dma"   // Your char device
#define NUM_SAMPLES 1024             // Number of 64-bit samples
#define BUF_SIZE    (NUM_SAMPLES * 8) // 64-bit = 8 bytes per sample
#define OUTPUT_FILE "fft_output.txt"  // Output file in current directory

int main() {
    int fd;
    ssize_t ret;
    uint64_t rx_buf[NUM_SAMPLES];

    printf("Opening device: %s\n", DEVICE_PATH);
    fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return EXIT_FAILURE;
    }
    printf("Device opened successfully, fd=%d\n", fd);

    printf("Starting S2MM DMA transfer for %d samples (%d bytes)...\n", NUM_SAMPLES, BUF_SIZE);

    ssize_t total_read = 0;
    while (total_read < BUF_SIZE) {
        ret = read(fd, ((uint8_t*)rx_buf) + total_read, BUF_SIZE - total_read);
        if (ret < 0) {
            if (errno == EINTR)
                continue; // Retry if interrupted
            perror("DMA read failed");
            close(fd);
            return EXIT_FAILURE;
        }
        if (ret == 0) {
            fprintf(stderr, "EOF reached unexpectedly after %zd bytes\n", total_read);
            break;
        }
        total_read += ret;
        printf("Read %zd bytes, total read: %zd\n", ret, total_read);
    }

    printf("DMA transfer completed, total bytes read: %zd\n", total_read);

    // Open file to save samples
    FILE *fout = fopen(OUTPUT_FILE, "w");
    if (!fout) {
        perror("Failed to open output file");
        close(fd);
        return EXIT_FAILURE;
    }

    // Save all samples in Verilog-style format: %016h\n
    for (int i = 0; i < NUM_SAMPLES; i++) {
        fprintf(fout, "%016llx\n", (unsigned long long)rx_buf[i]);
    }

    fclose(fout);
    printf("All %d samples saved to %s\n", NUM_SAMPLES, OUTPUT_FILE);

    printf("Closing device\n");
    close(fd);
    return EXIT_SUCCESS;
}
