#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <errno.h>
#include <stdint.h> // for int16_t

#define I2C_DEV "/dev/i2c-1"         // Path to the I2C device file on the Raspberry Pi
#define ACCEL_ADDR 0x68              // I2C address of the MPU-6050 accelerometer sensor
#define DATA_REG 0x3B                // Starting register address for accelerometer data (X-axis high byte)

#define PWR_MGMT_1 0x6b              // Power management register address (to wake sensor)
#define ACCEL_CONFIG 0x1C            // Accelerometer configuration register address
#define SCALE_FACTOR 16384.0f        // Scale factor for ±2g full scale range

// Structure to hold raw and converted accelerometer data for all three axes
struct AccelerometerData {
    int16_t x;                      // Raw X-axis accelerometer value (16-bit signed)
    int16_t y;                      // Raw Y-axis accelerometer value (16-bit signed)
    int16_t z;                      // Raw Z-axis accelerometer value (16-bit signed)
    float xg;                       // Converted X-axis acceleration in g (gravity)
    float yg;                       // Converted Y-axis acceleration in g
    float zg;                       // Converted Z-axis acceleration in g
};

// Function to initialize MPU-6050 by waking it up from sleep mode
// and setting accelerometer full scale range to ±2g
void initialize_mpu6050(int fd) {
    char write_buffer[2];

    // Wake up MPU-6050 by clearing sleep bit (set PWR_MGMT_1 to 0x00)
    write_buffer[0] = PWR_MGMT_1;
    write_buffer[1] = 0x00;
    if (write(fd, write_buffer, 2) != 2) {
        perror("Failed to wake up MPU-6050");
    }

    // Set accelerometer full-scale range to ±2g by writing 0x00 to ACCEL_CONFIG
    write_buffer[0] = ACCEL_CONFIG;
    write_buffer[1] = 0x00; // FS_SEL = 0 (±2g)
    if (write(fd, write_buffer, 2) != 2) {
        perror("Failed to set accelerometer scale to ±2g");
    }
}

// Convert raw accelerometer data to g (gravitational acceleration)
// raw_value is a signed 16-bit number from sensor
float convert_to_g(int16_t raw_value) {
    return ((float) raw_value / SCALE_FACTOR);
}

int main() {
    int fd;                        // File descriptor for I2C device
    char write_buffer[1];          // Buffer to store register address for reading
    char read_buffer[6];           // Buffer to hold 6 bytes read from accelerometer (X, Y, Z data)
    struct AccelerometerData accel; // Struct to hold raw and converted accelerometer values

    // Open the I2C device file with read/write permissions
    fd = open(I2C_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open I2C device");  // Report error if opening fails
        return 1;
    }

    // Tell the I2C driver which device address we want to communicate with
    if (ioctl(fd, I2C_SLAVE, ACCEL_ADDR) < 0) {
        perror("Failed to set I2C slave address"); // Report error if setting device fails
        close(fd);  // Close file before exiting
        return 1;
    }

    // Wake up the MPU-6050 and configure accelerometer scale to ±2g
    initialize_mpu6050(fd);

    printf("Reading accelerometer data in real time...\n");

    // Infinite loop to continuously read accelerometer data
    while (1) {
        // Specify the starting register address from which to read accelerometer data
        write_buffer[0] = DATA_REG;
        if (write(fd, write_buffer, 1) != 1) {
            perror("Failed to write register address");  // Error if cannot set register pointer
            break;  // Exit loop on failure
        }

        // Read 6 bytes from sensor (2 bytes for each of X, Y, Z axis)
        if (read(fd, read_buffer, 6) != 6) {
            perror("Failed to read data from accelerometer"); // Error if read fails
            break;  // Exit loop on failure
        }

        // Combine two bytes (high and low) to form 16-bit signed values for each axis
        accel.x = (int16_t)((read_buffer[0] << 8) | read_buffer[1]);
        accel.y = (int16_t)((read_buffer[2] << 8) | read_buffer[3]);
        accel.z = (int16_t)((read_buffer[4] << 8) | read_buffer[5]);

        // Convert raw values to acceleration in g's using scaling factor
        accel.xg = convert_to_g(accel.x);
        accel.yg = convert_to_g(accel.y);
        accel.zg = convert_to_g(accel.z);

        // Print accelerometer readings in g for each axis
        printf("X: %.4f g, Y: %.4f g, Z: %.4f g\n", accel.xg, accel.yg, accel.zg);

        usleep(3000000);  // Wait for 3 seconds before next reading
    }

    // Close the I2C device before exiting
    close(fd);
    return 0;
}
