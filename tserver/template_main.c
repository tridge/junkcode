/* 
   Copyright (C) Andrew Tridgell 2002
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* a simple program that allows template to be used as a scripting system */
#include "includes.h"

static void usage(void)
{
	puts("
template [options] <files>

Options:
  -c     run as a cgi script
  -h show help
");
}

int main(int argc, char *argv[])
{
	struct cgi_state *cgi;
	extern char *optarg;
	int opt;
	int use_cgi = 0;

	while ((opt = getopt(argc, argv, "ch")) != -1) {
		switch (opt) {
		case 'c':
			use_cgi = 1;
			break;
		case 'h':
			usage();
			exit(1);
		}
	}

	cgi = cgi_init();

	if (use_cgi) {
		cgi->setup(cgi);
		cgi->load_variables(cgi);
	}
	
	if (argc > 1) {
		int i;
		for (i=1; i<argc; i++) {
			cgi->tmpl->process(cgi->tmpl, argv[i], 1);
		}
	} else {
		/* this allows us to work as a shell script */
		cgi->tmpl->process(cgi->tmpl, "/proc/self/fd/0", 1);
	}

	return 0;
}
