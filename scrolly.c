/* scrolly_drv.c - kernel module driver for the scrolling LED display */

/* Copyright 1998 */

/* by David Austin */

/* includes */

#include "scrolly.h"

/* defines */

#define LPT 0x278 /* the address of the parallel port */
#define STACK_SIZE 4096
#define PRIORITY 1
#define bash_port(data)   outb(~((char) (data)), LPT)

/* parts of the parallel port we use */

#define SRCLK 0x01
#define LOAD  0x02
#define SRDTA 0x04
#define RESET 0x08

/* globals */

RT_TASK mytask;
static int row_count = 0; /* the number of the next row to be output */

/*****************************************************************************/

void output_row(unsigned char *rp) {
  static char i;
  static unsigned char mask;
  
  mask = 0x80 >> row_count;

  for (i = ROW_LENGTH - 1; i >= 0; i--) {
    if ((rp[i] & mask) > 0)
      { /* clock a 1 through */
	bash_port(SRDTA);
	bash_port(SRDTA + SRCLK);
      }
    else 
      { /* clock a 0 through*/
	bash_port(0x00);
	bash_port(SRCLK);
      }
  }

  /* If we just sent the first row then do a reset+load */
  if (row_count == 0)
    {
      bash_port(RESET);
      bash_port(RESET + LOAD);
      bash_port(0x00);
    }
  else 
    {    
      bash_port(LOAD);
      bash_port(0x00);
    }


  /* Update row_count */
  row_count++;
  if (row_count == NUM_ROWS)
    row_count = 0;
}

/*****************************************************************************/

void drv_loop(int dummy)
{
  unsigned char row[ROW_LENGTH];
  int i;

  /* Empty the buffer */
  for (i = 0; i < ROW_LENGTH; i++)
    row[i] = 0x00;

  /* Start the refresh */
  while (1)
    {
      /* Check for row data */
      if (row_count == 0)
	rtf_get(1, row, sizeof(row));
      
      /* Do the refresh */
      output_row(row);

      /* Suspend until re-started by the rt-kernel */
      rt_task_wait();
    }
}

/*****************************************************************************/

int init_module(void)
{
  rtf_create(1, ROW_LENGTH * 100); /* FIFO for row data */

  outb(0x80, (LPT + 3)); /* set all pins as outputs */

  row_count = 0;

  rt_task_init(&mytask, drv_loop, 0, STACK_SIZE, PRIORITY);

  rt_task_make_periodic(&mytask, rt_get_time(), 
			(RT_TICKS_PER_SEC * 1.5) / 1000);

  return 0;
}

/*****************************************************************************/

void cleanup_module(void)
{
  rt_task_delete(&mytask);
}


