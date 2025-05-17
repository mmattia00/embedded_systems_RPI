#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <arpa/inet.h>
#include <errno.h>

#define I2C_DEV "/dev/i2c-1"  // I2C device file (Raspberry Pi I2C bus)

// MPU6050 Accelerometer I2C address and registers
#define MPU_ADDR 0x68
#define MPU_PWR_MGMT_1 0x6B
#define MPU_ACCEL_REG 0x3B
#define ACCEL_CONFIG 0x1C
#define ACCEL_SCALE 16384.0f  // Scale factor for ±2g full scale range

// TCS34725 RGB sensor I2C address and registers
#define TCS_ADDR 0x29
#define ENABLE_REG 0x00
#define ATIME_REG 0x01
#define CONTROL_REG 0x0F
#define CDATA_REG 0x14
#define RDATA_REG 0x16
#define GDATA_REG 0x18
#define BDATA_REG 0x1A
#define ENABLE_PON 0x01  // Power ON bit
#define ENABLE_AEN 0x02  // ADC Enable bit

// UDP server IP and port
#define SERVER_IP "192.168.1.120"
#define SERVER_PORT 12345
#define SAMPLE_COUNT 10  // Number of samples to buffer before sending UDP packet

// Structure to hold one combined sample of accelerometer and RGB data
struct CombinedSample {
    float xg, yg, zg;  // Accelerometer readings in g
    float r, g, b;     // Normalized RGB color values
};

// Convert raw accelerometer 16-bit value to 'g' units based on scale factor
float convert_accel(int16_t raw) {
    return ((float)raw / ACCEL_SCALE);
}

// Normalize RGB channel value by clear channel and scale to 0-255 range
float normalize_rgb(uint16_t color, uint16_t clear) {
    if (clear == 0) return 0;  // Avoid division by zero
    float value = ((float)color / clear) * 255.0f;
    return value > 255.0f ? 255.0f : value;  // Clamp to max 255
}

// Initialize MPU6050 sensor: wake up and set accelerometer scale to ±2g
void init_mpu(int fd) {
    // Wake up MPU6050 by clearing sleep bit in power management register
    uint8_t wakeup[2] = {MPU_PWR_MGMT_1, 0x00};
    write(fd, wakeup, 2);

    // Set accelerometer configuration to ±2g full scale range
    uint8_t accel_config[2] = {ACCEL_CONFIG, 0x00};  // FS_SEL = 0 (±2g)
    write(fd, accel_config, 2);
}

// Initialize TCS34725 RGB sensor: power on, enable ADC, set integration time and gain
void init_tcs(int fd) {
    uint8_t setup[][2] = {
        {ENABLE_REG | 0x80, ENABLE_PON},             // Power ON
        {ENABLE_REG | 0x80, ENABLE_PON | ENABLE_AEN},// Power ON + ADC Enable
        {ATIME_REG | 0x80, 0xD6},                    // Integration time (e.g., 27.7ms)
        {CONTROL_REG | 0x80, 0x01}                   // Gain setting (4x)
    };
    for (int i = 0; i < 4; i++) {
        write(fd, setup[i], 2);  // Write each config register
        usleep(1000);            // Short delay between writes
    }
}

// Read a 16-bit word (2 bytes) from a TCS34725 register (little endian)
uint16_t read_word(int fd, uint8_t reg) {
    uint8_t cmd = reg | 0x80;   // Command bit (0x80) for register read
    write(fd, &cmd, 1);         // Write register address
    uint8_t buf[2];
    read(fd, buf, 2);           // Read 2 bytes
    return buf[0] | (buf[1] << 8);  // Combine bytes into 16-bit value
}

int main() {
    // Open I2C device and set slave address for MPU6050
    int mpu_fd = open(I2C_DEV, O_RDWR);
    ioctl(mpu_fd, I2C_SLAVE, MPU_ADDR);
    init_mpu(mpu_fd);  // Initialize accelerometer

    // Open I2C device and set slave address for TCS34725
    int tcs_fd = open(I2C_DEV, O_RDWR);
    ioctl(tcs_fd, I2C_SLAVE, TCS_ADDR);
    init_tcs(tcs_fd);  // Initialize RGB sensor

    // Create UDP socket for sending data to server
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // Define server address structure
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(SERVER_PORT)
    };
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    struct CombinedSample buffer[SAMPLE_COUNT];  // Buffer for samples
    int count = 0;  // Current number of collected samples

    while (1) {
        // Read accelerometer data (6 bytes: X, Y, Z 16-bit values)
        uint8_t reg = MPU_ACCEL_REG;
        uint8_t acc_buf[6];
        write(mpu_fd, &reg, 1);   // Set register pointer to accelerometer start
        read(mpu_fd, acc_buf, 6); // Read accelerometer data

        // Combine high and low bytes for each axis
        int16_t ax = (acc_buf[0] << 8) | acc_buf[1];
        int16_t ay = (acc_buf[2] << 8) | acc_buf[3];
        int16_t az = (acc_buf[4] << 8) | acc_buf[5];

        // Small delay before reading RGB sensor (integration time)
        usleep(100000);

        // Read color data from TCS34725
        uint16_t clear = read_word(tcs_fd, CDATA_REG);
        uint16_t r = read_word(tcs_fd, RDATA_REG);
        uint16_t g = read_word(tcs_fd, GDATA_REG);
        uint16_t b = read_word(tcs_fd, BDATA_REG);

        // Convert raw accelerometer values to 'g' units and normalize colors
        buffer[count].xg = convert_accel(ax);
        buffer[count].yg = convert_accel(ay);
        buffer[count].zg = convert_accel(az);
        buffer[count].r = normalize_rgb(r, clear);
        buffer[count].g = normalize_rgb(g, clear);
        buffer[count].b = normalize_rgb(b, clear);

        // Print sample to console
        printf("Sample %d - X: %.2f Y: %.2f Z: %.2f | R: %.2f G: %.2f B: %.2f\n",
               count + 1,
               buffer[count].xg, buffer[count].yg, buffer[count].zg,
               buffer[count].r, buffer[count].g, buffer[count].b);

        // Prepare JSON payload for MQTT publishing
        char json_payload[256];
        snprintf(json_payload, sizeof(json_payload),
                 "{\"xg\":%.2f,\"yg\":%.2f,\"zg\":%.2f,\"r\":%.2f,\"g\":%.2f,\"b\":%.2f}",
                 buffer[count].xg, buffer[count].yg, buffer[count].zg,
                 buffer[count].r, buffer[count].g, buffer[count].b);

        // Build command string for mosquitto_pub to publish telemetry to ThingsBoard
        char cmd[512];
        snprintf(cmd, sizeof(cmd),
                 "mosquitto_pub -q 1 -h 192.168.1.120 -p 1883 -t v1/devices/me/telemetry "
                 "-u \"pceoEBhUFEAEQkJIwbSQ\" -m '%s'",
                 json_payload);

        // Execute the mosquitto_pub command to send MQTT message
        system(cmd);

        count++;  // Increment sample count

        // Once enough samples collected, send buffer via UDP to server
        if (count == SAMPLE_COUNT) {
            sendto(sockfd, buffer, sizeof(buffer), 0,
                   (struct sockaddr*)&server_addr, sizeof(server_addr));
            printf("Sent 10 combined samples to server.\n");
            count = 0;  // Reset count for next batch
        }

        sleep(1);  // Wait 1 second before next sample
    }

    // Cleanup: close file descriptors and socket (never reached here due to infinite loop)
    close(mpu_fd);
    close(tcs_fd);
    close(sockfd);
    return 0;
}
