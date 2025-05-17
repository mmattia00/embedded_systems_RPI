#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 12345        // UDP port number where the server listens
#define NUM_SAMPLES 10    // Number of samples expected in each UDP packet

// Structure representing a combined sensor sample with accelerometer and RGB data
typedef struct {
    float xg, yg, zg;  // Accelerometer values in g (gravity units)
    float r, g, b;     // Normalized RGB color values (0-255 scale)
} CombinedSample;

// Function to compute and print basic statistics for an array of float data
void compute_stats_float(float data[], int n, const char* label) {
    float min = data[0], max = data[0];
    double sum = 0.0, sq_sum = 0.0;

    // Calculate min, max, and sum of the data
    for (int i = 0; i < n; i++) {
        if (data[i] < min) min = data[i];
        if (data[i] > max) max = data[i];
        sum += data[i];
    }

    double mean = sum / n;

    // Calculate sum of squared differences from the mean (for stddev)
    for (int i = 0; i < n; i++) {
        sq_sum += pow(data[i] - mean, 2);
    }

    // Compute standard deviation
    double stddev = sqrt(sq_sum / n);

    // Print the statistics for the given label
    printf("%s: min=%.2f, max=%.2f, mean=%.2f, stddev=%.2f\n", label, min, max, mean, stddev);
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    CombinedSample buffer[NUM_SAMPLES];  // Buffer to store received samples

    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // Zero out the server and client address structures
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    // Setup the server address: IPv4, any incoming IP, specified port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Listen on all network interfaces
    server_addr.sin_port = htons(PORT);        // Convert port number to network byte order

    // Bind the socket to the specified IP and port so it can receive data
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(1);
    }

    printf("Server listening on port %d...\n", PORT);

    // Infinite loop to continuously receive packets
    while (1) {
        // Receive UDP packet into buffer from any client
        ssize_t recv_len = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                    (struct sockaddr *)&client_addr, &addr_len);
        if (recv_len < 0) {
            perror("recvfrom failed");
            continue;  // Skip to next iteration if error occurs
        }

        // Verify packet size matches expected size of NUM_SAMPLES samples
        if (recv_len != sizeof(buffer)) {
            fprintf(stderr, "Received unexpected packet size: %ld bytes\n", recv_len);
            continue;  // Skip if packet size unexpected
        }

        // Separate accelerometer and color data into individual arrays for stats
        float xg[NUM_SAMPLES], yg[NUM_SAMPLES], zg[NUM_SAMPLES];
        float r[NUM_SAMPLES], g[NUM_SAMPLES], b[NUM_SAMPLES];

        for (int i = 0; i < NUM_SAMPLES; i++) {
            xg[i] = buffer[i].xg;
            yg[i] = buffer[i].yg;
            zg[i] = buffer[i].zg;
            r[i] = buffer[i].r;
            g[i] = buffer[i].g;
            b[i] = buffer[i].b;
        }

        // Print statistics for each sensor channel for the received packet
        printf("\n--- Statistics for received packet ---\n");
        compute_stats_float(xg, NUM_SAMPLES, "Accel X");
        compute_stats_float(yg, NUM_SAMPLES, "Accel Y");
        compute_stats_float(zg, NUM_SAMPLES, "Accel Z");
        compute_stats_float(r, NUM_SAMPLES, "Color R");
        compute_stats_float(g, NUM_SAMPLES, "Color G");
        compute_stats_float(b, NUM_SAMPLES, "Color B");
    }

    // Cleanup: close the socket (never reached due to infinite loop)
    close(sockfd);
    return 0;
}
