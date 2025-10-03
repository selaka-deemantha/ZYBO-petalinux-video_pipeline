#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#define DEVICE_PATH "/dev/fft_dma"
#define NUM_SAMPLES 1024
#define BUF_SIZE (NUM_SAMPLES * 8) // 64-bit samples
#define OUTPUT_FILE "fft_output.txt"

int main() {
    int fd;
    ssize_t ret;
    uint64_t rx_buf[NUM_SAMPLES];

    fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return EXIT_FAILURE;
    }

    ret = read(fd, rx_buf, BUF_SIZE);
    if (ret != BUF_SIZE) {
        fprintf(stderr, "Failed to read all samples: read %zd bytes\n", ret);
        close(fd);
        return EXIT_FAILURE;
    }

    FILE *fout = fopen(OUTPUT_FILE, "w");
    if (!fout) {
        perror("Failed to open output file");
        close(fd);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < NUM_SAMPLES; i++) {
        fprintf(fout, "%016llx\n", (unsigned long long)rx_buf[i]);
    }

    fclose(fout);
    close(fd);

    printf("All %d samples saved to %s\n", NUM_SAMPLES, OUTPUT_FILE);
    return EXIT_SUCCESS;
}

