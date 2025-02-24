#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#define I2C_DEV "/dev/i2c-1"
#define ACCEL_ADDR 0x68
#define DATA_REG 0x3B

#define MPU6050_ADDR 0x68
#define PWR_MGMT_1 0x6b
#define ACCEL_XOUT_H 0x3b
#define ACCEL_XOUT_L 0x3c
#define ACCEL_YOUT_H 0x3d
#define ACCEL_YOUT_L 0x3e
#define ACCEL_ZOUT_H 0x3f
#define ACCEL_ZOUT_L 0x40

#define SCALE_FACTOR 32768.0f // 16-bit signed value max for ±4g range

struct AccelerometerData {
    int16_t x;
    int16_t y;
    int16_t z;
    float xg;
    float yg;
    float zg;
};

void initialize_mpu6050(int fd) {
    char write_buffer[2];
    write_buffer[0] = PWR_MGMT_1;
    write_buffer[1] = 0x00;
    if (write(fd, write_buffer, 2) != 2) {
        perror("Failed to wake up MPU-6050");
    }
}

float convert_to_g(int16_t raw_value) {
    float g_value = ((float) raw_value / SCALE_FACTOR) * 4.0f;  // ±4g
    return g_value;
    //printf("Acceleration: %.4f g\n", g_value);
}

int main() {
    int fd;
    char write_buffer[1];
    char read_buffer[6];
    struct AccelerometerData accel;

    fd = open(I2C_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open I2C device");
        return 1;
    }

    if (ioctl(fd, I2C_SLAVE, ACCEL_ADDR) < 0) {
        perror("Failed to set I2C slave address");
        close(fd);
        return 1;
    }

    initialize_mpu6050(fd);

    printf("Reading accelerometer data in real time...\n");

    while (1) {
        write_buffer[0] = DATA_REG;
        if (write(fd, write_buffer, 1) != 1) {
            perror("Failed to write register address");
            break;
        }

        if (read(fd, read_buffer, 6) != 6) {
            perror("Failed to read data from accelerometer");
            break;
        }

        accel.x = (int16_t)((read_buffer[0] << 8) | read_buffer[1]);
        accel.y = (int16_t)((read_buffer[2] << 8) | read_buffer[3]);
        accel.z = (int16_t)((read_buffer[4] << 8) | read_buffer[5]);

        accel.xg = (float) convert_to_g(accel.x);
        accel.yg = (float) convert_to_g(accel.y);
        accel.zg = (float) convert_to_g(accel.z);

        // Convert to g and print the values
        printf("X: %.4f g, Y: %.4f g, Z: %.4f g\n", accel.xg, accel.yg, accel.zg);


        usleep(3000000); // Sleep 3s
    }

    close(fd);
    return 0;
}
