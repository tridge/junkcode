volatile int count1=0;
volatile int count2=0;

volatile int in_critical_section = 0;

void sig_handler()
{
  if (in_critical_section) {
    count1++;
  } else {
    do_signal_work();
  }
}


void mainline(void)
{
  in_critical_section = 1;

  do_normal_work();

  while (count1 != count2) {
    do_signal_work();
    count2++;
  }
  in_critical_section = 0;

  while (count1 != count2) {
    /* here we have to block signals, but we expect it almost never to
     * happen.  it will only happen if a signal arrives between the
     * end of the first while loop and the unsetting of the
     * in_critical_section flag, which should be so rare that the
     * performance hit will be tiny
     */
    block_signals();
    do_signal_work();
    count2++;
    unblock_signals();
  }
}
