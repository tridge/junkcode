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

#include "ntrip_mavlink.h"

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
    printf("ntrip_mavlink: <options>\n");
    printf(" -D serial_device\n");
    printf(" -s NTRIP server IP\n");
    printf(" -m mount_point\n");
    printf(" -u username\n");
    printf(" -p password\n");
    printf(" -r port\n");
}

int main(int argc, const char *argv[])
{
    int opt;
    const char *devname = NULL;
    const char *server = NULL;
    const char *mount_point = NULL;
    const char *username = NULL;
    const char *password = NULL;
    unsigned port = 2101;
    uint8_t mav_channel = 0;
    uint8_t target_system = 0;
    uint8_t target_component = 0;

    while ((opt = getopt(argc, (char * const *)argv, "D:s:m:r:u:p:")) != -1) {
        switch (opt) {
        case 'D':
            devname = optarg;
            break;
        case 's':
            server = optarg;
            break;
        case 'm':
            mount_point = optarg;
            break;
        case 'r':
            port = atoi(optarg);
            break;
        case 'u':
            username = optarg;
            break;
        case 'p':
            password = optarg;
            break;
        default:
            printf("Invalid option '%c'\n", opt);
            usage();
            exit(1);
        }
    }

    if (!devname || !server || !mount_point || !username || !password) {
        usage();
        exit(1);
    }

    dev_fd = open_serial_115200(devname);
    if (dev_fd == -1) {
        printf("Failed to open %s\n", devname);
        exit(1);
    }

    printf("Starting NTRIP client\n");
    int ret = ntrip_mavlink_init(server,
                                 mount_point,
                                 port,
                                 username,
                                 password,
                                 mav_channel,
                                 target_system,
                                 target_component,
                                 "/tmp/ntriperror");
    if (ret != 0) {
        printf("Failed to start NTRIP client\n");
        exit(1);
    }

    while (true) {
        // run at 10Hz
        usleep(1000*100);
        ret = ntrip_mavlink_update();
        if (ret != 0) {
            printf("Failed ntrip_mavlink_update\n");
            exit(1);
        }
    }

    return 0;
}
