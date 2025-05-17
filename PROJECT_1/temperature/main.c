#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <errno.h>
#include <stdint.h> // For int16_t

#define I2C_DEV "/dev/i2c-1"         // Path to the I2C device file (I2C bus 1)
#define ACCEL_ADDR 0x68              // I2C address of the MPU-6050 sensor
#define DATA_REG 0x41                // Register address for temperature data (high byte)
#define PWR_MGMT_1 0x6b              // Power management register to wake up the sensor

// Structure to hold raw and converted temperature data
struct TemperatureData {
    int16_t raw;                     // Raw 16-bit temperature data read from sensor
    float converted;                 // Converted temperature in degrees Celsius
};

// Function to initialize MPU-6050 by waking it from sleep mode
void initialize_mpu6050(int fd) {
    char write_buffer[2] = {PWR_MGMT_1, 0x00}; // Write 0x00 to PWR_MGMT_1 to clear sleep bit
    if (write(fd, write_buffer, 2) != 2) {     // Write 2 bytes: register and value
        perror("Failed to wake up MPU-6050");  // Print error if write fails
    }
}

// Convert the raw temperature reading to Celsius using MPU-6050 formula
float convert_to_temperature(int16_t raw_value) {
    // According to datasheet: Temp (°C) = (raw / 340) + 36.53
    return (raw_value / 340.0f) + 36.53f;
}

int main() {
    int fd;                          // File descriptor for I2C device
    char write_buffer[1];            // Buffer to hold register address for write
    char read_buffer[2];             // Buffer to hold the 2 bytes read from sensor
    struct TemperatureData temp;     // Variable to store temperature data

    // Open the I2C device file for read/write access
    fd = open(I2C_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open I2C device");  // Error opening device file
        return 1;
    }

    // Set the I2C slave address for MPU-6050 device on the bus
    if (ioctl(fd, I2C_SLAVE, ACCEL_ADDR) < 0) {
        perror("Failed to set I2C slave address"); // Error setting device address
        close(fd);
        return 1;
    }

    // Initialize the sensor by waking it up (clear sleep bit)
    initialize_mpu6050(fd);

    printf("Reading temperature data in real time...\n");

    // Main loop to continuously read temperature data
    while (1) {
        // Specify the register address (0x41) to read temperature high byte from
        write_buffer[0] = DATA_REG;
        if (write(fd, write_buffer, 1) != 1) {
            perror("Failed to write register address");  // Error writing register address
            break;
        }

        // Read 2 bytes from the temperature register (high and low bytes)
        if (read(fd, read_buffer, 2) != 2) {
            perror("Failed to read data from sensor");   // Error reading data
            break;
        }

        // Combine two bytes into a single signed 16-bit integer
        temp.raw = (int16_t)((read_buffer[0] << 8) | read_buffer[1]);

        // Convert the raw sensor value into Celsius temperature
        temp.converted = convert_to_temperature(temp.raw);

        // Output the raw and converted temperature values
        printf("Raw: %d, Converted: %.2f °C\n", temp.raw, temp.converted);

        usleep(3000000); // Sleep for 3 seconds before next read cycle
    }

    // Close the I2C device file before exiting
    close(fd);
    return 0;
}

