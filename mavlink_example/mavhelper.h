/*
  update mavlink parser
 */
int mav_update(int dev_fd);

void mav_set_message_rate(uint32_t message_id, float rate_hz);
void send_heartbeat(void);

