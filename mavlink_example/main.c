/*
  example code for reading RTCM data from a ntripclient child process and sending as
  GPS_INJECT MAVLink packets
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <asm/ioctls.h>
#include <asm/termbits.h>

#include "mavhelper.h"

#define MAVLINK_USE_CONVENIENCE_FUNCTIONS

#include "include/mavlink/mavlink_types.h"
mavlink_system_t mavlink_system = {42,11,};

static int dev_fd = -1;

void comm_send_ch(mavlink_channel_t chan, uint8_t c)
{
    write(dev_fd, &c, 1);
}

/*
  open up a serial port at given baudrate
 */
static int open_serial(const char *devname, uint32_t baudrate)
{
    int fd = open(devname, O_RDWR|O_CLOEXEC);
    if (fd == -1) {
        return -1;
    }

    struct termios2 tc;
    memset(&tc, 0, sizeof(tc));
    if (ioctl(fd, TCGETS2, &tc) == -1) {
        return -1;
    }
    
    /* speed is configured by c_[io]speed */
    tc.c_cflag &= ~CBAUD;
    tc.c_cflag |= BOTHER;
    tc.c_ispeed = baudrate;
    tc.c_ospeed = baudrate;

    tc.c_cflag &= ~(PARENB|CSTOPB|CSIZE);
    tc.c_cflag |= CS8;

    tc.c_lflag &= ~(ICANON|ECHO|ECHOE|ISIG);
    tc.c_iflag &= ~(IXON|IXOFF|IXANY);
    tc.c_oflag &= ~OPOST;
    
    if (ioctl(fd, TCSETS2, &tc) == -1) {
        return -1;
    }
    if (ioctl(fd, TCFLSH, TCIOFLUSH) == -1) {
        return -1;
    }

    return fd;
}

/*
  show usage
 */
static void usage(void)
{
    printf("mavexample: <options>\n");
    printf(" -D serial_device\n");
}

int main(int argc, const char *argv[])
{
    int opt;
    const char *devname = NULL;

    while ((opt = getopt(argc, (char * const *)argv, "D:")) != -1) {
        switch (opt) {
        case 'D':
            devname = optarg;
            break;
        default:
            printf("Invalid option '%c'\n", opt);
            usage();
            exit(1);
        }
    }

    if (!devname) {
        usage();
        exit(1);
    }

    dev_fd = open_serial(devname, 115200);
    if (dev_fd == -1) {
        printf("Failed to open %s\n", devname);
        exit(1);
    }

    while (true) {
        // run at 100Hz
        usleep(1000*100);
        int ret = mav_update(dev_fd);
        if (ret != 0) {
            printf("Failed mav_update\n");
            exit(1);
        }
    }

    return 0;
}
