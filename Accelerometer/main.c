#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h> // For htons/ntohs if needed

#define I2C_DEV "/dev/i2c-1"
#define ACCEL_ADDR 0x5d
#define DATA_REG 0x28

struct AccelerometerData {
    int16_t x;
    int16_t y;
    int16_t z;
};

int main() {
    int fd;
    char write_buffer[1];
    char read_buffer[6];
    struct AccelerometerData accel;

    // Open I2C device
    fd = open(I2C_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open I2C device");
        return 1;
    }

    // Set slave address
    if (ioctl(fd, I2C_SLAVE, ACCEL_ADDR) < 0) {
        perror("Failed to set I2C slave address");
        close(fd);
        return 1;
    }

    printf("Reading accelerometer data in real time...\n");

    while (1) {
        // Write register address
        write_buffer[0] = DATA_REG;
        if (write(fd, write_buffer, 1) != 1) {
            perror("Failed to write register address");
            break;
        }

        // Read 6 bytes (X, Y, Z each with LSB + MSB)
        if (read(fd, read_buffer, 6) != 6) {
            perror("Failed to read data from accelerometer");
            break;
        }

        // Use memcpy for easier conversion
        memcpy(&accel.x, &read_buffer[0], sizeof(accel.x));
        memcpy(&accel.y, &read_buffer[2], sizeof(accel.y));
        memcpy(&accel.z, &read_buffer[4], sizeof(accel.z));

        // If sensor is big-endian, convert using ntohs()
        // accel.x = ntohs(accel.x);
        // accel.y = ntohs(accel.y);
        // accel.z = ntohs(accel.z);

        printf("X: %d, Y: %d, Z: %d\n", accel.x, accel.y, accel.z);

        usleep(100000); // Sleep 100ms
    }

    close(fd);
    return 0;
}
