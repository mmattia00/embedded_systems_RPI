# Compiler flags:
# -Wall: enable all warnings
# -Wextra: enable extra warnings
# -O2: optimization level 2 for better performance
CFLAGS = -Wall -Wextra -O2

# Name of the output executable
TARGETS = acc_rgb_sensors_client

# Default target: build the executable(s)
all: $(TARGETS)

# Build rule for the client executable
# Compile acc_rgb_sensors_client.c into acc_rgb_sensors_client
client: acc_rgb_sensors_client.c
	$(CC) $(CFLAGS) -o acc_rgb_sensors_client acc_rgb_sensors_client.c

# Clean rule to remove the generated executable(s)
clean:
	rm -f $(TARGETS)
