//include libraries for i2c
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>


//Structs needed for I2C messages
struct i2c_rdwr_ioctl_data packets;
struct i2c_msg messages[2]; //Set size depending on number of messages in o

int const r_len = 6;
int const w_len = 1;

char write_bytes[w_len];
char read_bytes[r_len];

int main()
{
    char i2cFile[15];
    int addr = 0x5d;
    sprintf(i2cFile, "/dev/i2c-%d", dev);
    int fd = open(i2cFile, O_RDWR);