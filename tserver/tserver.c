#include "includes.h"

/* they have pressed "Install" */
static void do_install(struct cgi_state *cgi)
{
	struct template_state *tmpl = cgi->tmpl;

	tmpl->put(tmpl, "install_script", "./installroot.sh");

	cgi->setvar(cgi, "archive_location");
	cgi->setvar(cgi, "proxy_server");

	cgi->download(cgi, "install.html");
}

/* they have pressed "Upgrade" */
static void do_upgrade(struct cgi_state *cgi)
{
	struct template_state *tmpl = cgi->tmpl;

	tmpl->put(tmpl, "install_script", "./upgraderoot.sh");

	cgi->setvar(cgi, "archive_location");
	cgi->setvar(cgi, "proxy_server");

	cgi->download(cgi, "install.html");
}

/* they have pressed "Diagnostics" */
static void do_diagnostics(struct cgi_state *cgi)
{
	cgi->download(cgi, "diagnostics.html");
}

/* they have pressed "Diagnostics" */
static void do_main_page(struct cgi_state *cgi)
{
	cgi->download(cgi, "index.html");
}

/* the main webserver process, called with stdin and stdout setup
 */
static void run_webserver(void)
{
	struct cgi_state *cgi;
	const char *action;
	struct stat st;
	int i;
	struct {
		char *name;
		void (*fn)(struct cgi_state *);
	} actions[] = {
		{"Install", do_install},
		{"Upgrade", do_upgrade},
		{"Diagnostics", do_diagnostics},
		{NULL, do_main_page}
	};

	if (chdir("files") != 0) {
		fprintf(stderr,"Can't find files directory?\n");
		exit(1);
	}

	cgi = cgi_init();

	cgi->setup(cgi);
	cgi->load_variables(cgi);

	/* handle a direct file download */
	if (!strstr(cgi->pathinfo, "..") && *cgi->pathinfo != '/' &&
	    stat(cgi->pathinfo, &st) == 0 && S_ISREG(st.st_mode)) {
		cgi->download(cgi, cgi->pathinfo);
		cgi->destroy(cgi);
		return;
	}

	action = cgi->get(cgi, "action");

	for (i=0; actions[i].name; i++) {
		if (action && fnmatch(actions[i].name, action, 0) == 0) break;
	}
	
	/* call the action handler */
	actions[i].fn(cgi);

	cgi->destroy(cgi);
}


/* main program, just start listening and answering queries */
int main(int argc, char *argv[])
{
	tcp_listener(TSERVER_PORT, TSERVER_LOGFILE, run_webserver);
	return 0;
}

