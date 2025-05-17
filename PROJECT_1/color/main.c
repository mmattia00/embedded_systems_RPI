#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <stdint.h>

#define I2C_DEV "/dev/i2c-1"       // I2C bus device file
#define TCS34725_ADDR 0x29         // I2C address of TCS34725 RGB color sensor

// Register addresses for TCS34725
#define ENABLE_REG 0x00            // Enable register
#define ATIME_REG 0x01             // Integration time register
#define CONTROL_REG 0x0F           // Control register (gain)
#define CDATA_REG 0x14             // Clear data low byte register
#define RDATA_REG 0x16             // Red data low byte register
#define GDATA_REG 0x18             // Green data low byte register
#define BDATA_REG 0x1A             // Blue data low byte register

// Enable register bits
#define ENABLE_PON 0x01            // Power ON bit
#define ENABLE_AEN 0x02            // RGBC enable bit (enable color sensing)

// Open and initialize the I2C device and set slave address
int init_i2c(const char *device, int addr) {
    int file = open(device, O_RDWR);     // Open I2C device file for read/write
    if (file < 0) {
        perror("Unable to open I2C device");  // Error if open fails
        exit(EXIT_FAILURE);
    }
    if (ioctl(file, I2C_SLAVE, addr) < 0) {  // Set device address on I2C bus
        perror("Unable to configure I2C device"); // Error if ioctl fails
        close(file);
        exit(EXIT_FAILURE);
    }
    return file;  // Return file descriptor for further I2C operations
}

// Write a single byte value to a register on the sensor
void i2c_write(int file, uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {reg | 0x80, value}; // Set command bit (0x80) with register address
    if (write(file, buffer, 2) != 2) {       // Write register address + value to device
        perror("Failed to write to I2C device");  // Error if write fails
    }
}

// Read a 16-bit (2 bytes) value from a sensor register (little-endian)
uint16_t i2c_read_word(int file, uint8_t reg) {
    uint8_t cmd = reg | 0x80;     // Set command bit with register address
    if (write(file, &cmd, 1) != 1) {   // Write register address to device
        perror("Failed to write register address");
    }
    uint8_t buffer[2];
    if (read(file, buffer, 2) != 2) {   // Read two bytes of data from the register
        perror("Failed to read from I2C device");
    }
    return buffer[0] | (buffer[1] << 8);  // Combine low and high byte into 16-bit value
}

// Normalize raw color value relative to clear channel to scale 0-255
uint8_t normalize_color(uint16_t color, uint16_t clear) {
    if (clear == 0) return 0;    // Avoid division by zero
    uint32_t scaled = ((uint32_t)color * 255) / clear;  // Scale color to 0-255 range
    return (scaled > 255) ? 255 : scaled;  // Cap at 255 max
}

int main() {
    // Initialize the I2C connection to the TCS34725 sensor
    int file = init_i2c(I2C_DEV, TCS34725_ADDR);

    // Power ON the sensor by writing to ENABLE register
    i2c_write(file, ENABLE_REG, ENABLE_PON);
    usleep(2400); // Wait 2.4 milliseconds for power-up

    // Enable RGBC (color sensing) by setting both PON and AEN bits
    i2c_write(file, ENABLE_REG, ENABLE_PON | ENABLE_AEN);

    // Configure integration time (~100ms) for color data collection
    i2c_write(file, ATIME_REG, 0xD6);

    // Set gain to 4x for better sensitivity
    i2c_write(file, CONTROL_REG, 0x01);

    printf("Reading RGBC values from TCS34725...\n");

    // Infinite loop to continuously read color data
    while (1) {
        usleep(100000); // Wait 100 milliseconds between readings

        // Read raw 16-bit clear, red, green, and blue channel data
        uint16_t clear = i2c_read_word(file, CDATA_REG);
        uint16_t red = i2c_read_word(file, RDATA_REG);
        uint16_t green = i2c_read_word(file, GDATA_REG);
        uint16_t blue = i2c_read_word(file, BDATA_REG);

        // Normalize color channels relative to clear channel to scale 0–255
        uint8_t r_norm = normalize_color(red, clear);
        uint8_t g_norm = normalize_color(green, clear);
        uint8_t b_norm = normalize_color(blue, clear);

        // Print normalized RGB values
        printf("RGB  -> R: %u, G: %u, B: %u\n", r_norm, g_norm, b_norm);
    }

    // Close I2C device file (unreachable here because of infinite loop)
    close(file);
    return 0;
}

