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
#include <sys/time.h>
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
static uint8_t target_system;

static double get_time_seconds()
{
    struct timeval tp;
    gettimeofday(&tp,NULL);
    return (tp.tv_sec + (tp.tv_usec*1.0e-6));
}

static double last_msg_s;
static double last_hb_s;

/*
  update mavlink
 */
int mav_update(int dev_fd)
{
    uint8_t b;
    mavlink_message_t msg;

    while (read(dev_fd, &b, 1) == 1) {
        if (mavlink_parse_char(MAVLINK_COMM_0, b, &msg, &mav_status)) {
            double tnow = get_time_seconds();
            if (msg.msgid == MAVLINK_MSG_ID_ATTITUDE) {
                printf("msgid=%u dt=%f\n", msg.msgid, tnow - last_msg_s);
                last_msg_s = tnow;
            }
            if (target_system == 0 && msg.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
                printf("Got system ID %u\n", msg.sysid);
                target_system = msg.sysid;

                // get key messages at 200Hz
                mav_set_message_rate(MAVLINK_MSG_ID_ATTITUDE, 40);
            }
        }
    }

    double tnow = get_time_seconds();
    if (tnow - last_hb_s > 1.0) {
        last_hb_s = tnow;
        send_heartbeat();
    }

    return 0;
}

void mav_set_message_rate(uint32_t message_id, float rate_hz)
{
    mavlink_msg_command_long_send(MAVLINK_COMM_0,
                                  target_system,
                                  0,
                                  MAV_CMD_SET_MESSAGE_INTERVAL,
                                  0,
                                  message_id,
                                  (uint32_t)(1e6 / rate_hz),
                                  0, 0, 0, 0, 0);    
}

void send_heartbeat(void)
{
    printf("sending hb\n");
    mavlink_msg_heartbeat_send(
        MAVLINK_COMM_0,
        MAV_TYPE_GCS,
        MAV_AUTOPILOT_GENERIC,
        0, 0, 0);
}
