/*
  API for creating ntripclient child process and sending mavlink GPS_INJECT_DATA packets
 */
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
                       const char *errlog);

/*
  update NTRIP mavlink, checking for data from child
  process. Restarting child process as needed. Should be called at
  approximately 10Hz
 */
int ntrip_mavlink_update(void);
