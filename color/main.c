#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <stdint.h>

#define I2C_DEV "/dev/i2c-1"
#define TCS34725_ADDR 0x29

// Register addresses
#define ENABLE_REG 0x00
#define ATIME_REG 0x01
#define CONTROL_REG 0x0F
#define CDATA_REG 0x14
#define RDATA_REG 0x16
#define GDATA_REG 0x18
#define BDATA_REG 0x1A

// Enable register bits
#define ENABLE_PON 0x01 // Power ON
#define ENABLE_AEN 0x02 // RGBC Enable

// Open I2C device
int init_i2c(const char *device, int addr) {
    int file = open(device, O_RDWR);
    if (file < 0) {
        perror("Unable to open I2C device");
        exit(EXIT_FAILURE);
    }
    if (ioctl(file, I2C_SLAVE, addr) < 0) {
        perror("Unable to configure I2C device");
        close(file);
        exit(EXIT_FAILURE);
    }
    return file;
}

// Write to a register
void i2c_write(int file, uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {reg | 0x80, value}; // 0x80 sets command mode
    if (write(file, buffer, 2) != 2) {
        perror("Failed to write to I2C device");
    }
}

// Read 16-bit data from a register
uint16_t i2c_read_word(int file, uint8_t reg) {
    uint8_t cmd = reg | 0x80; // Command mode
    if (write(file, &cmd, 1) != 1) {
        perror("Failed to write register address");
    }
    uint8_t buffer[2];
    if (read(file, buffer, 2) != 2) {
        perror("Failed to read from I2C device");
    }
    return buffer[0] | (buffer[1] << 8); // Little-endian
}

int main() {
    int file = init_i2c(I2C_DEV, TCS34725_ADDR);

    // Initialize sensor
    i2c_write(file, ENABLE_REG, ENABLE_PON);
    usleep(2400); // Wait 2.4ms
    i2c_write(file, ENABLE_REG, ENABLE_PON | ENABLE_AEN);

    // Configure integration time and gain
    i2c_write(file, ATIME_REG, 0xD6);  // ~100ms integration time
    i2c_write(file, CONTROL_REG, 0x01); // 4x gain

    printf("Reading RGBC values from TCS34725...\n");

    while (1) {
        usleep(100000); // Wait 100ms between reads
        uint16_t clear = i2c_read_word(file, CDATA_REG);
        uint16_t red = i2c_read_word(file, RDATA_REG);
        uint16_t green = i2c_read_word(file, GDATA_REG);
        uint16_t blue = i2c_read_word(file, BDATA_REG);

        printf("R: %u, G: %u, B: %u, C: %u\n", red, green, blue, clear);
    }

    close(file);
    return 0;
}
