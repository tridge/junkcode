/*
  example code for reading RTCM data from a ntripclient child process and sending as
  GPS_INJECT MAVLink packets
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <termios.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

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
  open up a serial port at 115200
 */
static int open_serial_115200(const char *devname)
{
    int fd = open(devname, O_RDWR|O_CLOEXEC);
    if (fd == -1) {
        return -1;
    }
    struct termios options;

    tcgetattr(fd, &options);

    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    options.c_cflag &= ~(PARENB|CSTOPB|CSIZE);
    options.c_cflag |= CS8;

    options.c_lflag &= ~(ICANON|ECHO|ECHOE|ISIG);
    options.c_iflag &= ~(IXON|IXOFF|IXANY);
    options.c_oflag &= ~OPOST;

    tcsetattr(fd, TCSANOW, &options);
    tcflush(fd, TCIOFLUSH);
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

    dev_fd = open_serial_115200(devname);
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
