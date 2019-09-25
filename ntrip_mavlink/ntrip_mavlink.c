/*
  example code for reading RTCM data from a ntripclient child process and sending as
  GPS_INJECT MAVLink packets
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
#include "ntrip_mavlink.h"

#define MAVLINK_USE_CONVENIENCE_FUNCTIONS

#include "include/mavlink/mavlink_types.h"
static mavlink_system_t mavlink_system = {42,11,};

extern void comm_send_ch(mavlink_channel_t chan, uint8_t c);

#include "include/mavlink/ardupilotmega/mavlink.h"

static pid_t child_pid;
static int child_fd = -1;

/*
  arguments for ntripclient, filled in by ntrip_mavlink_init()
 */
static char *ntripclient_cmd[] = {
    "ntripclient",
    "-s", NULL,
    "-m", NULL,
    "-r", NULL,
    "-u", NULL,
    "-p", NULL,
    NULL,
};

static const char *errlog_filename;
static mavlink_channel_t out_channel;
static uint8_t target_system;
static uint8_t target_component;

/*
  start or re-start child process
 */
static int start_child(void)
{
    // pipe used to receive data from child
    int fd[2];
    if (pipe(fd) != 0) {
        return -1;
    }

    signal(SIGCHLD, SIG_IGN);

    pid_t pid = fork();
    if (pid == 0) {
        // child code
        close(fd[0]);
        close(0);
        close(1);
        close(2);
        // setup /dev/null as stdin
        open("/dev/null", O_RDONLY);
        // setup pipe as stdout
        dup2(fd[1], 1);

        // setup tmp file for stderr
        open(errlog_filename, O_WRONLY|O_CREAT|O_TRUNC, 0644);
        execvp(ntripclient_cmd[0], ntripclient_cmd);

        // only reach here on failure to start ntripclient
        exit(1);
    }

    // parent code
    child_fd = fd[0];
    close(fd[1]);
    child_pid = pid;

    // set it non-blocking
    int flags = fcntl(child_fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(child_fd, F_SETFL, flags);

    // success
    return 0;
}

/*
  setup ntrip client, return 0 on success
 */
int ntrip_mavlink_init(const char *caster_ip,
                       const char *mount_point,
                       unsigned port,
                       const char *username,
                       const char *password,
                       unsigned mavlink_channel,
                       uint8_t mav_target_system,
                       uint8_t mav_target_component,
                       const char *errlog)
{
    // if we already have a child then kill it
    if (child_pid != 0) {
        kill(child_pid, SIGKILL);
    }
    if (child_fd >= 0) {
        close(child_fd);
    }

    // free old strings if set
    uint8_t i;
    for (i=2; i<= 10; i+= 2) {
        if (ntripclient_cmd[i]) {
            free(ntripclient_cmd[i]);
            ntripclient_cmd[i] = NULL;
        }
    }

    // note that we rely on aligning indexes in ntripclient_cmd
    ntripclient_cmd[2] = strdup(caster_ip);
    ntripclient_cmd[4] = strdup(mount_point);
    char portstr[6];
    snprintf(portstr, 5, "%u", port);
    portstr[5] = 0;
    ntripclient_cmd[6] = strdup(portstr);
    ntripclient_cmd[8] = strdup(username);
    ntripclient_cmd[10] = strdup(password);

    errlog_filename = errlog;
    out_channel = mavlink_channel;
    target_system = mav_target_system;
    target_component = mav_target_component;

    return start_child();
}

/*
  update NTRIP mavlink, checking for data from child
  process. Restarting child process as needed. Should be called at
  approximately 10Hz
 */
int ntrip_mavlink_update(void)
{
    // check that child still exists
    if (child_pid != 0 && kill(child_pid, 0) == -1) {
        // child is gone, restart it
        if (start_child() != 0) {
            return -1;
        }
    }

    // check we are started
    if (child_pid == 0 || child_fd == -1) {
        return -1;
    }

    uint8_t buf[110];
    ssize_t n = read(child_fd, buf, 110);
    if (n <= 0) {
        return 0;
    }

    // send data as GPS_INJECT_DATA mavlink message
    mavlink_msg_gps_inject_data_send(out_channel,
                                     target_system,
                                     target_component,
                                     n,
                                     buf);
    return 0;
}
