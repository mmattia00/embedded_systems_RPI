#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <errno.h>
#include <stdint.h> // For int16_t

#define I2C_DEV "/dev/i2c-1"         // Path to the I2C device file
#define ACCEL_ADDR 0x68              // I2C address of the MPU-6050
#define DATA_REG 0x41                // Register address for the temperature data (high byte)
#define PWR_MGMT_1 0x6b              // Power management register to wake up the sensor

struct TemperatureData {
    int16_t raw;                     // Raw temperature data
    float converted;                 // Converted temperature in Celsius
};

// Initialize the MPU-6050 sensor by waking it up
void initialize_mpu6050(int fd) {
    char write_buffer[2] = {PWR_MGMT_1, 0x00}; // 0x00 turns off sleep mode
    if (write(fd, write_buffer, 2) != 2) {
        perror("Failed to wake up MPU-6050");
    }
}

// Convert raw temperature data to Celsius
float convert_to_temperature(int16_t raw_value) {
    return (raw_value / 340.0f) + 36.53f;
}

int main() {
    int fd;
    char write_buffer[1];
    char read_buffer[2];
    struct TemperatureData temp;

    // Open the I2C device
    fd = open(I2C_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open I2C device");
        return 1;
    }

    // Set the I2C slave address
    if (ioctl(fd, I2C_SLAVE, ACCEL_ADDR) < 0) {
        perror("Failed to set I2C slave address");
        close(fd);
        return 1;
    }

    // Initialize the sensor
    initialize_mpu6050(fd);

    printf("Reading temperature data in real time...\n");

    while (1) {
        // Set the register address to read from
        write_buffer[0] = DATA_REG;
        if (write(fd, write_buffer, 1) != 1) {
            perror("Failed to write register address");
            break;
        }

        // Read 2 bytes of temperature data
        if (read(fd, read_buffer, 2) != 2) {
            perror("Failed to read data from sensor");
            break;
        }

        // Combine high and low bytes into a 16-bit integer
        temp.raw = (int16_t)((read_buffer[0] << 8) | read_buffer[1]);
        temp.converted = convert_to_temperature(temp.raw);

        // Print the temperature
        printf("Raw: %d, Converted: %.2f °C\n", temp.raw, temp.converted);

        usleep(3000000); // Wait 3 seconds before reading again
    }

    close(fd);
    return 0;
}
