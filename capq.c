/*
 * capq - print out state of CAP queue (obtained from capqd).
 *
 * capq [-c] [-i interval] [-h cap-host]
 *
 * Author: Paul Mackerras, June 1993.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/termios.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <termios.h>

#define CAPQD_PORT	6123	/* capqd's well-known port number */
#define CAP_HOST_DFLT	"cafe"	/* default value for -h option */

FILE *capqd_w, *capqd_r;	/* streams to and from capqd */
char *cap_host;			/* hostname where capqd is */

/* used for terminal capabilities */
char termcap_entry[1024], string_caps[1024];

#define MAXLINES	100	/* max # lines for continuous display */
#define MAXCOLS		100	/* max # columns  */

double last_change_time;	/* cap_host's time of last queue change */
double our_time;		/* time according to our clock */
double host_time_diff;		/* cap_host's clock - our clock */
double timez[MAXLINES];		/* start time for each line of display */
int num_lines;			/* # entries valid in timez[] */

/* Convert from a timeval to a floating-point number of seconds */
#define double_time(x)		((x)->tv_sec + (x)->tv_usec / 1.0E6)

/* Termcap routines */
extern int tgetent();
extern int tgetnum();
extern char *tgetstr();
extern int tgetflag();
extern short ospeed;

/* Option values */
int continuous;			/* => display the queue continuously */
double interval;		/* with updates at this interval */

/* Capability strings and terminal information from termcap */
int screen_lines;		/* screen has this many lines */
int screen_width;		/* and this many columns */
char *term;			/* terminal type */
char *home;			/* go to top-left corner of screen */
char *init_str;			/* initialize terminal */
char *finish_str;		/* reset terminal before exit */
char *clr_screen;		/* clear whole screen */
char *clr_eol;			/* clear to end of line */
char *clr_eos;			/* clear to end of screen */

int size_changed;		/* set when the window size changes */

void getout();
void hoist();
int outchar();

main(argc, argv)
    int argc;
    char **argv;
{
    int fd, c, n;
    int queue_empty;
    double t;
    char *ptr;
    extern char *optarg;
    fd_set ready;
    struct timeval *timo, tval;
    struct termios tio;

    /* set defaults */
    cap_host = getenv("AP1000HOST");
    if( cap_host == NULL )
	cap_host = CAP_HOST_DFLT;
    continuous = 0;
    interval = 1.0;

    /* parse command-line options */
    while( (c = getopt(argc, argv, "c:h:")) != -1 ){
	switch( c ){
	case 'h':
	    cap_host = optarg;
	    break;
	case 'c':
	  continuous = 1;
	  interval = atof(optarg);
	  break;
	case '?':
	    fprintf(stderr, "Usage: %s [-h host]\n", argv[0]);
	    exit(1);
	}
    }

    /* connect to the capqd process */
    fd = connect_tcp(cap_host, CAPQD_PORT);
    if( fd < 0 )
	exit(1);
    capqd_r = fdopen(fd, "r");
    capqd_w = fdopen(fd, "w");
    if( capqd_r == NULL || capqd_w == NULL ){
	fprintf(stderr, "fdopen failed!\n");
	exit(1);
    }

    /* request the current state of the queue */
    fputs("queue\n", capqd_w);
    fflush(capqd_w);

    if( !continuous ){
	/* just print the queue once and exit. */
	show_queue(0, "");
	exit(0);
    }

    /* Continuous mode - get information about the terminal. */
    if( tcgetattr(0, &tio) == 0 )
	ospeed = cfgetospeed(&tio);
    screen_lines = 24;
    screen_width = 80;
    home = clr_screen = clr_eol = finish_str = NULL;
    term = getenv("TERM");
    if( term != NULL && tgetent(termcap_entry, term) > 0 ){
	ptr = string_caps;
	screen_lines = tgetnum("li");
	screen_width = tgetnum("co");
	home = tgetstr("ho", &ptr);
	clr_screen = tgetstr("cl", &ptr);
	clr_eol = tgetstr("ce", &ptr);
	clr_eos = tgetstr("cd", &ptr);
	init_str = tgetstr("ti", &ptr);
	finish_str = tgetstr("te", &ptr);
	if( init_str != NULL )
	    tputs(init_str, screen_lines, outchar);
    }

    /* default to the ANSI sequences */
    if( home == NULL )
	home = "\033[H";
    if( clr_screen == NULL )
	clr_screen = "\033[H\033[J";
    if( clr_eol == NULL )
	clr_eol = "\033[K";
    if( clr_eos == NULL )
	clr_eos = "\033[J";
    if( finish_str == NULL )
	finish_str = "";
    tputs(clr_screen, screen_lines, outchar);
    fflush(stdout);

    if( screen_lines > MAXLINES )
	screen_lines = MAXLINES;
    if( screen_width > MAXCOLS )
	screen_width = MAXCOLS;

    /* fix up the terminal and exit on these signals */
    signal(SIGINT, getout);
    signal(SIGTERM, getout);
    /* update the terminal size on this signal */
    signal(SIGWINCH, hoist);

    /*
     * This loop waits for either (a) an update to arrive from capqd,
     * or (b) for it to be time to change the time values displayed
     * for jobs in the queue, or (c) for the terminal window size to change.
     */
    num_lines = 0;
    size_changed = 0;
    for(;;){
	if( num_lines <= 1 )
	    /* no times to update - don't time out */
	    timo = NULL;

	else {
	    /* work out how long until the first time displayed
	       should change, i.e. until it's a multiple of `interval'. */
	    gettimeofday(&tval, NULL);
	    our_time = double_time(&tval);
	    t = timez[1];
	    if( t == 0 )
		t = timez[1];
	    t = our_time + host_time_diff - t;
	    t = interval - fmod(t, interval);
	    tval.tv_sec = (int) floor(t);
	    tval.tv_usec = (int) floor((t - tval.tv_sec) * 1.0E6);
	    timo = &tval;
	}

	/* wait for something */
	FD_ZERO(&ready);
	FD_SET(fd, &ready);
	n = select(fd+1, &ready, NULL, NULL, timo);
	if( n < 0 && errno != EINTR ){
	    tputs(finish_str, screen_lines, outchar);
	    perror("select");
	    exit(1);
	}

	if( n > 0 ){
	    /* update display with new information, then ask for
	       another report when the queue changes. */
	    size_changed = 0;
	    show_queue();
	    fprintf(capqd_w, "queue %.2f\n", last_change_time);
	    fflush(capqd_w);

	} else if( size_changed ){
	    /* ask for the queues again */
	    fprintf(capqd_w, "queue\n");
	    fflush(capqd_w);
	    size_changed = 0;

	} else if( n == 0 ){
	    /* timeout - no new information from capqd */
	    update_times();
	}

	/* leave the cursor at the top left of the screen */
	tputs(home, 0, outchar);
	fflush(stdout);
    }
}

/*
 * Fatal signal - clean up terminal and exit.
 */
void
getout(sig)
    int sig;
{
    tputs(finish_str, screen_lines, outchar);
    exit(0);
}

/*
 * Window size changed - winch up the new size.
 */
void
hoist(sig)
    int sig;
{
    struct winsize winsz;

    if( ioctl(fileno(stdout), TIOCGWINSZ, &winsz) == 0 ){
	size_changed = 1;
	screen_lines = winsz.ws_row;
	screen_width = winsz.ws_col;
	if( screen_lines > MAXLINES )
	    screen_lines = MAXLINES;
	if( screen_width > MAXCOLS )
	    screen_width = MAXCOLS;
    }
}

/*
 * Output character routine for tputs to use.
 */
int
outchar(c)
    int c;
{
    putchar(c);
}

/*
 * New information from capqd - display it, and (in continuous mode)
 * record the start times for the users shown on each line of the display.
 */
int
show_queue()
{
    int n, index, run_done, lnum;
    double start_time;
    char *ptr;
    char line[80];
    char user[64], more[8];
    int pid[3];
    struct timeval now;
    char str[MAXCOLS+1];

    /* initialize, print heading */
    run_done = 0;
    timez[0] = 0;
    lnum = 1;
    sprintf(str, "        CAP queue on %.50s", cap_host);
    putstr(str, 0);

    /* get lines from capqd */
    for(;;){
	if( fgets(line, sizeof(line), capqd_r) == NULL ){
	    /* EOF or error - capqd must have gone away */
	    if( continuous ){
		tputs(finish_str, screen_lines, outchar);
		fflush(stdout);
	    }
	    fprintf(stderr, "read error\n");
	    exit(1);
	}

	if( line[0] == 'E' )
	    /* end of queue report */
	    break;
	if( line[0] == 'T' ){
	    /* first line of queue report: T last-change-time time-now */
	    gettimeofday(&now, NULL);
	    our_time = double_time(&now);
	    sscanf(line+2, "%lf %lf", &last_change_time, &host_time_diff);
	    host_time_diff -= our_time;
	    continue;
	}

	/* line specifying next user in queue:
	   index user start-time { pids... } */
	n = sscanf(line, "%d %s %lf { %d %d %d %d %s", &index,
		   user, &start_time, &pid[0], &pid[1], &pid[2],
		   &pid[3], more);
	if( (n -= 3) <= 0 ){
	    /* couldn't parse line - ignore it */
#ifdef	DEBUG
	    fprintf(stderr, "bad line %s", line);
#endif
	    continue;
	}

	/* accumulate a line to be printed in str */
	ptr = str;
	if( index == 0 ){
	    /* this is the running job */
	    sprint_time(&ptr, start_time);
	    sprintf(ptr, "  Run  %.20s  (pid %d", user, pid[0]);
	    ptr += strlen(ptr);
	    if( n > 1 ){
		/* print the rest of the pids which are waiting */
		strcpy(ptr, ", ");
		ptr += 2;
		sprint_pids_waiting(&ptr, pid, n, 1);
		strcpy(ptr, " waiting");
		ptr += 8;
	    }
	    *ptr++ = ')';
	    run_done = 1;

	} else {
	    /* a user in the queue */
	    if( continuous && !run_done ){
		/* no running job - leave a blank line for it */
		str[0] = 0;
		putstr(str, lnum);
		run_done = 1;
		timez[lnum] = 0;
		++lnum;
	    }
	    if( continuous && lnum >= screen_lines - 1 ){
		/* no more room on screen */
		if( lnum == screen_lines - 1 ){
		    strcpy(ptr, "                (more)");
		    ptr += strlen(ptr);
		    start_time = 0;
		} else
		    ptr = NULL;	/* don't print anything */
	    } else {
		/* format this line into str */
		sprint_time(&ptr, start_time);
		sprintf(ptr, "  %3d  %.20s  (", index, user);
		ptr += strlen(ptr);
		sprint_pids_waiting(&ptr, pid, n, 0);
		*ptr++ = ')';
	    }
	}
	/* print out the line we've formatted */
	if( ptr != NULL ){
	    *ptr = 0;
	    if( continuous ){
		putstr(str, lnum);
		timez[lnum] = start_time;
		++lnum;
	    } else
		printf("%s\n", str);
	}
    }

    if( lnum > screen_lines )
	lnum = screen_lines;
    num_lines = lnum;

    /* clear the remainder of the screen */
    if( continuous && lnum < screen_lines )
	tputs(clr_eos, screen_lines - lnum + 1, outchar);
}

/*
 * Output a line to the screen.  In continuous mode, truncate the
 * line to the screen width and clear the remainder of the line.
 */
putstr(str, lnum)
    char *str;
    int lnum;
{
    if( continuous ){
	if( lnum < screen_lines ){
	    str[screen_width] = 0;
	    fputs(str, stdout);
	    if( strlen(str) < screen_width )
		tputs(clr_eol, 1, outchar);
	    if( lnum < screen_lines - 1 )
		putchar('\n');
	}
    } else
	printf("%s\n", str);
}

/*
 * Format the time since time t on the cap_host into the buffer
 * at **pp, advancing *pp past the formatted string.
 */
sprint_time(pp, t)
    char **pp;
    double t;
{
    int x;

    t = floor(our_time + host_time_diff - t + 0.5);
    if( t > 3600 ){
	x = floor(t / 3600);
	sprintf(*pp, "%3d:", x);
	*pp += strlen(*pp);
	t -= x * 3600.0;
    } else {
	strcpy(*pp, "    ");
	*pp += 4;
    }
    x = floor(t / 60);
    sprintf(*pp, "%.2d:%.2d", x, (int)(t - x * 60));
    *pp += strlen(*pp);
}

/*
 * Format a list of pids waiting into **pp, advancing *pp.
 */
sprint_pids_waiting(pp, pid, n, i)
    char **pp;
    int pid[], n, i;
{
    sprintf(*pp, "pid%s %d", (n > i+1? "s": ""), pid[i]);
    *pp += strlen(*pp);
    for( ++i; i < n && i < 4; ++i ){
	sprintf(*pp, ", %d", pid[i]);
	*pp += strlen(*pp);
    }
    if( n >= 5 ){
	strcpy(*pp, ", ...");
	*pp += 5;
    }
}

/*
 * Update the times displayed for each user.
 */
update_times()
{
    double t;
    int i;
    struct timeval now;
    char *ptr, str[12];

    gettimeofday(&now, NULL);
    our_time = double_time(&now);
    for( i = 0; i < num_lines; ++i ){
	ptr = str;
	if( (t = timez[i]) != 0 ){
	    sprint_time(&ptr, t);
	    if( screen_width < sizeof(str) )
		str[screen_width] = 0;
	    *ptr = 0;
	    printf("%s", str);
	}
	if( i < num_lines - 1 )
	    printf("\n");
    }
}

/*
 * Establish a TCP/IP connection with the process at the given hostname
 * and port number.
 */
int
connect_tcp(host, port)
    char *host;
    int port;
{
    int fd;
    unsigned long hnum;
    struct hostent *hent;
    struct sockaddr_in addr;

    hnum = inet_addr(host);
    if( hnum == -1 ){
	hent = gethostbyname(host);
	if( hent == NULL ){
	    fprintf(stderr, "hostname %s not recognized\n", host);
	    return -1;
	}
	hnum = *(unsigned long *)(hent->h_addr_list[0]);
    }

    if( (fd = socket(PF_INET, SOCK_STREAM, 0)) < 0 ){
	perror("socket");
	return -1;
    }

    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = hnum;

    if( connect(fd, &addr, sizeof(addr)) < 0 ){
	perror("connect");
	return -1;
    }

    return fd;
}
