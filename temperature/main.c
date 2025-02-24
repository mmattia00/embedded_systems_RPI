#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <errno.h>
#include <stdint.h> // for int16_t


#define I2C_DEV "/dev/i2c-1"         // Path to the I2C device file
#define ACCEL_ADDR 0x68              // I2C address of the accelerometer (MPU-6050)
#define DATA_REG 0x41                // Register address for the temperature data (starting from high byte)

#define PWR_MGMT_1 0x6b             // Power management register to wake up the sensor
#define SCALE_FACTOR 32768.0f       // Scaling factor for 16-bit signed value (±4g range)

struct TemperatureData {
    int16_t raw;                     // Raw accelerometer data for X-axis
                     // Raw accelerometer data for Z-axis
    float converted;                      // Accelerometer data in g's for X-axis
                    // Accelerometer data in g's for Z-axis
};

// Function to initialize the MPU-6050 sensor by waking it up
void initialize_mpu6050(int fd) {
    char write_buffer[2];
    write_buffer[0] = PWR_MGMT_1;  // Power management register address
    write_buffer[1] = 0x00;        // Value to wake up the sensor (0x00 turns off sleep mode)
    
    if (write(fd, write_buffer, 2) != 2) {
        perror("Failed to wake up MPU-6050");
    }
}

// Function to convert raw accelerometer data to g's
float convert_to_temperature(int16_t raw_value) {
    return (raw_value / 340.0f) + 36.53f;
}

int main() {
    int fd;
    char write_buffer[1];
    char read_buffer[6];
    struct TemperatureData temp;

    // Open the I2C device
    fd = open(I2C_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open I2C device");
        return 1;
    }

    // Set the I2C slave address to communicate with the MPU-6050 sensor
    if (ioctl(fd, I2C_SLAVE, ACCEL_ADDR) < 0) {
        perror("Failed to set I2C slave address");
        close(fd);
        return 1;
    }

    // Initialize the MPU-6050 sensor
    initialize_mpu6050(fd);

    printf("Reading temperature data in real time...\n");

    while (1) {
        // Set the register address to read data from (starting at DATA_REG)
        write_buffer[0] = DATA_REG;
        if (write(fd, write_buffer, 1) != 1) {
            perror("Failed to write register address");
            break;
        }

        // Read 6 bytes of accelerometer data (X, Y, Z)
        if (read(fd, read_buffer, 2) != 2) {
            perror("Failed to read data from accelerometer");
            break;
        }

        // Combine the high and low bytes for X, Y, and Z axis readings
        temp.raw = (int16_t)((read_buffer[0] << 8) | read_buffer[1]);

        temp.converted = convert_to_temperature(temp.raw);

        // Convert raw data to g's for each axis

        // Print the accelerometer data in g's for X, Y, and Z axes
        printf("raw: %d, converted: %.2f C\n", temp.raw, temp.converted);

        usleep(3000000); // Sleep for 3 seconds before reading data again
    }

    // Close the I2C device file
    close(fd);
    return 0;
}
