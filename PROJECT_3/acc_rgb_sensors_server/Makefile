# Compiler to use
CC = gcc

# Compiler flags:
# -Wall: enable all warnings
# -Wextra: enable extra warnings
# -O2: optimization level 2 for better performance
CFLAGS = -Wall -Wextra -O2

# Linker flags:
# -lm: link the math library (required for math functions like sqrt, pow)
LDFLAGS = -lm

# Name of the output executable
TARGETS = acc_rgb_sensors_server

# Default target: build the executable(s)
all: $(TARGETS)

# Build rule for the server executable
# Compile acc_rgb_sensors_server.c into the executable acc_rgb_sensors_server
acc_rgb_sensors_server: acc_rgb_sensors_server.c
	$(CC) $(CFLAGS) -o acc_rgb_sensors_server acc_rgb_sensors_server.c $(LDFLAGS)

# Clean rule to remove the generated executable(s)
clean:
	rm -f $(TARGETS)
