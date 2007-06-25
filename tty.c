#define TTY_BUFSIZE	1024		/* size of one serial buffer	*/


static struct {
  char	*speed;
  int	code;
} tty_speeds[] = {			/* table of usable baud rates	*/
  { "50",	B50	}, { "75",	B75  	},	
  { "110",	B110	}, { "300",	B300	},
  { "600",	B600	}, { "1200",	B1200	},
  { "2400",	B2400	}, { "4800",	B4800	},
  { "9600",	B9600	},
#ifdef B14400
  { "14400",	B14400	},
#endif
#ifdef B19200
  { "19200",	B19200	},
#endif
#ifdef B38400
  { "38400",	B38400	},
#endif
#ifdef B57600
  { "57600",	B57600	},
#endif
#ifdef B115200
  { "115200",	B115200	},
#endif
  { NULL,	0	}
};
static struct termios	tty_saved,	/* saved TTY device state	*/
			tty_current;	/* current TTY device state	*/
static char		*in_buff,	/* line input/output buffers	*/
			*out_buff,
			*in_ptr,
			*out_ptr;
static int		in_size,	/* buffer sizes and counters	*/
			out_size,
			in_cnt,
			out_cnt,
			tty_sdisc,	/* saved TTY line discipline	*/
			tty_ldisc,	/* current TTY line discipline	*/
			tty_fd = -1;	/* TTY file descriptor		*/


/* Find a serial speed code in the table. */
static int
tty_find_speed(char *speed)
{
  int i;

  i = 0;
  while (tty_speeds[i].speed != NULL) {
	if (!strcmp(tty_speeds[i].speed, speed)) return(tty_speeds[i].code);
	i++;
  }
  return(-EINVAL);
}


/* Set the number of stop bits. */
static int
tty_set_stopbits(struct termios *tty, char *stopbits)
{
  switch(*stopbits) {
	case '1':
		tty->c_cflag &= ~CSTOPB;
		break;

	case '2':
		tty->c_cflag |= CSTOPB;
		break;

	default:
		return(-EINVAL);
  }
  if (opt_v) printf("STOPBITS = %c\n", *stopbits);
  return(0);
}


/* Set the number of data bits. */
static int
tty_set_databits(struct termios *tty, char *databits)
{
  tty->c_cflag &= ~CSIZE;
  switch(*databits) {
	case '5':
		tty->c_cflag |= CS5;
		break;

	case '6':
		tty->c_cflag |= CS6;
		break;

	case '7':
		tty->c_cflag |= CS7;
		break;

	case '8':
		tty->c_cflag |= CS8;
		break;

	default:
		return(-EINVAL);
  }
  if (opt_v) printf("DATABITS = %c\n", *databits);
  return(0);
}


/* Set the type of parity encoding. */
static int
tty_set_parity(struct termios *tty, char *parity)
{
  switch(toupper(*parity)) {
	case 'N':
		tty->c_cflag &= ~(PARENB | PARODD);
		break;  

	case 'O':
		tty->c_cflag &= ~(PARENB | PARODD);
		tty->c_cflag |= (PARENB | PARODD);
		break;

	case 'E':
		tty->c_cflag &= ~(PARENB | PARODD);
		tty->c_cflag |= (PARENB);
		break;

	default:
		return(-EINVAL);
  }
  if (opt_v) printf("PARITY = %c\n", *parity);
  return(0);
}


/* Set the line speed of a terminal line. */
static int
tty_set_speed(struct termios *tty, char *speed)
{
  int code;

  if ((code = tty_find_speed(speed)) < 0) return(code);
  tty->c_cflag &= ~CBAUD;
  tty->c_cflag |= code;
  return(0);
}


/* Put a terminal line in a transparent state. */
static int
tty_set_raw(struct termios *tty)
{
  int i;
  int speed;

  for(i = 0; i < NCCS; i++)
		tty->c_cc[i] = '\0';		/* no spec chr		*/
  tty->c_cc[VMIN] = 1;
  tty->c_cc[VTIME] = 0;
  tty->c_iflag = (IGNBRK | IGNPAR);		/* input flags		*/
  tty->c_oflag = (0);				/* output flags		*/
  tty->c_lflag = (0);				/* local flags		*/
  speed = (tty->c_cflag & CBAUD);		/* save current speed	*/
  tty->c_cflag = (CRTSCTS | HUPCL | CREAD);	/* UART flags		*/
#if 0
  tty->c_cflag |= CLOCAL;			/* not good!		*/
#endif
  tty->c_cflag |= speed;			/* restore speed	*/
  return(0);
}


/* Fetch the state of a terminal. */
static int
tty_get_state(struct termios *tty)
{
  if (ioctl(tty_fd, TCGETS, tty) < 0) {
	fprintf(stderr, "dip: tty_get_state: %s\n", strerror(errno));
	return(-errno);
  }
  return(0);
}


/* Set the state of a terminal. */
static int
tty_set_state(struct termios *tty)
{
  if (ioctl(tty_fd, TCSETS, tty) < 0) {
	fprintf(stderr, "dip: tty_set_state: %s\n", strerror(errno));
	return(-errno);
  }
  return(0);
}


/* Get the line discipline of a terminal line. */
int
tty_get_disc(int *disc)
{
  if (ioctl(tty_fd, TIOCGETD, disc) < 0) {
	fprintf(stderr, "dip: tty_get_disc: %s\n", strerror(errno));
	return(-errno);
  }
  return(0);
}


/* Set the line discipline of a terminal line. */
int
tty_set_disc(int disc)
{
  if (disc == -1) disc = tty_sdisc;

  if (ioctl(tty_fd, TIOCSETD, &disc) < 0) {
	fprintf(stderr, "dip: tty_set_disc(%d): %s\n", disc, strerror(errno));
	return(-errno);
  }
  return(0);
}


/* Get the encapsulation type of a terminal line. */
int
tty_get_encap(int *encap)
{
  if (ioctl(tty_fd, SIOCGIFENCAP, encap) < 0) {
	fprintf(stderr, "dip: tty_get_encap: %s\n", strerror(errno));
	return(-errno);
  }
  return(0);
}


/* Set the encapsulation type of a terminal line. */
int
tty_set_encap(int encap)
{
  if (ioctl(tty_fd, SIOCSIFENCAP, &encap) < 0) {
	fprintf(stderr, "dip: tty_set_encap(%d): %s\n", encap, strerror(errno));
	return(-errno);
  }
  return(0);
}


/* Fetch the name of the network interface attached to this terminal. */
int
tty_get_name(char *name)
{
  if (ioctl(tty_fd, SIOCGIFNAME, name) < 0) {
	fprintf(stderr, "dip: tty_get_name: %s\n", strerror(errno));
	return(-errno);
  }
  return(0);
}


/* Read one character (byte) from the TTY link. */
int
tty_getc(void)
{
  int s;

  if (in_cnt == 0) {
	s = read(tty_fd, in_buff, in_size);
	if (s < 0) return(-1);
	in_cnt = s;
	in_ptr = in_buff;
  }

  if (in_cnt < 0) {
	if (opt_v == 1) printf("dip: tty_getc: I/O error.\n");
	return(-1);
  }

  s = (int) *in_ptr;
  s &= 0xFF;
  in_ptr++;
  in_cnt--;
  return(s);
}


/* Write one character (byte) to the TTY link. */
int
tty_putc(int c)
{
  int s;

  if ((out_cnt == out_size) || (c == -1)) {
	s = write(tty_fd, out_buff, out_cnt);
	out_cnt = 0;
	out_ptr = out_buff;
	if (s < 0) return(-1);
  }

  if (c != -1) {
	*out_ptr = (char) c;
	out_ptr++;
	out_cnt++;
  }

  return(0);
}


/* Output a string of characters to the TTY link. */
void
tty_puts(char *s)
{
  while(*s != '\0') tty_putc((int) *s++);
  tty_putc(-1);	/* flush */
}


/* Return the TTY link's file descriptor. */
int
tty_askfd(void)
{
  return(tty_fd);
}


/* Set the number of databits a terminal line. */
int
tty_databits(char *bits)
{
  if (tty_set_databits(&tty_current, bits) < 0) return(-1);
  return(tty_set_state(&tty_current));
}


/* Set the number of stopbits of a terminal line. */
int
tty_stopbits(char *bits)
{
  if (tty_set_stopbits(&tty_current, bits) < 0) return(-1);
  return(tty_set_state(&tty_current));
}


/* Set the type of parity of a terminal line. */
int
tty_parity(char *type)
{
  if (tty_set_parity(&tty_current, type) < 0) return(-1);
  return(tty_set_state(&tty_current));
}


/* Set the line speed of a terminal line. */
int
tty_speed(char *speed)
{
  if (tty_set_speed(&tty_current, speed) < 0) return(-1);
  return(tty_set_state(&tty_current));
}


/* Hangup the line. */
int
tty_hangup(void)
{
  struct termios tty;

  tty = tty_current;
  (void) tty_set_speed(&tty, "0");
  if (tty_set_state(&tty) < 0) {
	fprintf(stderr, "dip: tty_hangup(DROP): %s\n", strerror(errno));
	return(-errno);
  }

  (void) sleep(1);

  if (tty_set_state(&tty_current) < 0) {
	fprintf(stderr, "dip: tty_hangup(RAISE): %s\n", strerror(errno));
	return(-errno);
  }
  return(0);
}


/* Close down a terminal line. */
int
tty_close(void)
{
  (void) tty_hangup();
  (void) tty_set_disc(tty_sdisc);
  return(0);
}


/* Open and initialize a terminal line. */
int
tty_open(char *name)
{
  char path[PATH_MAX];
  register char *sp;
  int fd;

  /* Try opening the TTY device. */
  if (name != NULL) {
	if ((sp = strrchr(name, '/')) != (char *)NULL) *sp++ = '\0';
	  else sp = name;
	sprintf(path, "/dev/%s", sp);
	if ((fd = open(path, O_RDWR)) < 0) {
		fprintf(stderr, "dip: tty_open(%s, RW): %s\n",
						path, strerror(errno));
		return(-errno);
	}
	tty_fd = fd;
	if (opt_v) printf("TTY = %s (%d) ", path, fd);
  } else tty_fd = 0;

  /* Size and allocate the I/O buffers. */
  in_size = TTY_BUFSIZE;
  out_size = in_size;
  in_buff = (char *) malloc(in_size);
  out_buff = (char *) malloc(out_size);
  if (in_buff == (char *)NULL || out_buff == (char *)NULL) {
	fprintf(stderr, "dip: tty_open: cannot allocate(%d, %d) buffers (%d)\n",
						in_size, out_size, errno);
	return(-ENOMEM);
  }
  in_cnt = 0;
  out_cnt = 0;
  in_ptr = in_buff;
  out_ptr = out_buff;
  out_size -= 4; /* safety */
  if (opt_v) printf("IBUF=%d OBUF=%d\n", in_size, out_size);

  /* Fetch the current state of the terminal. */
  if (tty_get_state(&tty_saved) < 0) {
	fprintf(stderr, "dip: tty_open: cannot get current state!\n");
	return(-errno);
  }
  tty_current = tty_saved;

  /* Fetch the current line discipline of this terminal. */
  if (tty_get_disc(&tty_sdisc) < 0) {
	fprintf(stderr, "dip: tty_open: cannot get current line disc!\n");
	return(-errno);
  } 
  tty_ldisc = tty_sdisc;

  /* Put this terminal line in a 8-bit transparent mode. */
  if (tty_set_raw(&tty_current) < 0) {
	fprintf(stderr, "dip: tty_open: cannot set RAW mode!\n");
	return(-errno);
  }

  /* If we are running in MASTER mode, set the default speed. */
  if ((name != NULL) && (tty_set_speed(&tty_current, "9600") != 0)) {
	fprintf(stderr, "dip: tty_open: cannot set 9600 bps!\n");
	return(-errno);
  }

  /* Set up a completely 8-bit clean line. */
  if (tty_set_databits(&tty_current, "8") ||
      tty_set_stopbits(&tty_current, "1") ||
      tty_set_parity(&tty_current, "N")) {
	fprintf(stderr, "dip: tty_open: cannot set 8N1 mode!\n");
	return(-errno);
  }
  return(tty_set_state(&tty_current));
}
