/*
  example code for mavlink from C
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "mavhelper.h"

#define MAVLINK_USE_CONVENIENCE_FUNCTIONS

#include "include/mavlink/mavlink_types.h"
static mavlink_system_t mavlink_system = {42,11,};

extern void comm_send_ch(mavlink_channel_t chan, uint8_t c);

#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
#include "include/mavlink/ardupilotmega/mavlink.h"

static mavlink_status_t mav_status;

/*
  update mavlink
 */
int mav_update(int dev_fd)
{
    uint8_t b;
    mavlink_message_t msg;

    while (read(dev_fd, &b, 1) == 1) {
        if (mavlink_parse_char(MAVLINK_COMM_0, b, &msg, &mav_status)) {
            printf("msgid=%u\n", msg.msgid);
        }
    }

    return 0;
}
