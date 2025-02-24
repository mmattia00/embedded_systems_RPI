#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <errno.h>
#include <stdint.h> // for int16_t


#define I2C_DEV "/dev/i2c-1"         // Path to the I2C device file
#define ACCEL_ADDR 0x68              // I2C address of the accelerometer (MPU-6050)
#define DATA_REG 0x3B                // Register address for the accelerometer data (starting from X-axis)

#define PWR_MGMT_1 0x6b             // Power management register to wake up the sensor
#define SCALE_FACTOR 32768.0f       // Scaling factor for 16-bit signed value (±4g range)

struct AccelerometerData {
    int16_t x;                     // Raw accelerometer data for X-axis
    int16_t y;                     // Raw accelerometer data for Y-axis
    int16_t z;                     // Raw accelerometer data for Z-axis
    float xg;                      // Accelerometer data in g's for X-axis
    float yg;                      // Accelerometer data in g's for Y-axis
    float zg;                      // Accelerometer data in g's for Z-axis
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
float convert_to_g(int16_t raw_value) {
    return ((float) raw_value / SCALE_FACTOR) * 4.0f;  // Convert raw value to g based on the ±4g range
}

int main() {
    int fd;
    char write_buffer[1];
    char read_buffer[6];
    struct AccelerometerData accel;

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

    printf("Reading accelerometer data in real time...\n");

    while (1) {
        // Set the register address to read data from (starting at DATA_REG)
        write_buffer[0] = DATA_REG;
        if (write(fd, write_buffer, 1) != 1) {
            perror("Failed to write register address");
            break;
        }

        // Read 6 bytes of accelerometer data (X, Y, Z)
        if (read(fd, read_buffer, 6) != 6) {
            perror("Failed to read data from accelerometer");
            break;
        }

        // Combine the high and low bytes for X, Y, and Z axis readings
        accel.x = (int16_t)((read_buffer[0] << 8) | read_buffer[1]);
        accel.y = (int16_t)((read_buffer[2] << 8) | read_buffer[3]);
        accel.z = (int16_t)((read_buffer[4] << 8) | read_buffer[5]);

        // Convert raw data to g's for each axis
        accel.xg = convert_to_g(accel.x);
        accel.yg = convert_to_g(accel.y);
        accel.zg = convert_to_g(accel.z);

        // Print the accelerometer data in g's for X, Y, and Z axes
        printf("X: %.4f g, Y: %.4f g, Z: %.4f g\n", accel.xg, accel.yg, accel.zg);

        usleep(3000000); // Sleep for 3 seconds before reading data again
    }

    // Close the I2C device file
    close(fd);
    return 0;
}
