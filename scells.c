#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>

#if __has_include(<X11/Xlib.h>)
#include<X11/Xlib.h>
#define HAS_X
#endif

#ifdef __OpenBSD__
#define SIGPLUS	     SIGUSR1+1
#define SIGMINUS     SIGUSR1-1
#else
#define SIGPLUS	     SIGRTMIN
#define SIGMINUS     SIGRTMIN
#endif

#define LENGTH(X)    (sizeof(X) / sizeof (X[0]))
#define CMDLENGTH    50
#define MIN(a, b)    ((a < b) ? a : b)
#define STATUSLENGTH (LENGTH(cells) * CMDLENGTH + 1)

#ifndef __OpenBSD__
void dummysighandler(int num);
#endif

void sighandler(int num);
void getcmds(int time);
void getsigcmds(unsigned int signal);
void setupsignals();
void sighandler(int signum);
int  getstatus(char *str, char *last);
void statusloop();
void termhandler(int sig);
void pstdout();

#ifdef HAS_X
void   setroot();
static int setupX();
static Display *dpy;
static int screen;
static Window root;
static void (*writestatus) () = setroot;
#else
static void (*writestatus) () = pstdout;
#endif

typedef struct {
	char* icon;
	char* command;
	unsigned int interval;
	unsigned int signal;
} Cell;

#include "config.h"

static char statusbar[LENGTH(cells)][CMDLENGTH] = {0};
static char statusstr[2][STATUSLENGTH];
static int  statusContinue = 1;

// opens process *cmd and stores output in *output
void getcmd(const Cell *block, char *output) {
	// make sure status is the same until output is ready
	char tempstatus[CMDLENGTH] = {0};
	strcpy(tempstatus, block->icon);
	FILE *cmdf = popen(block->command, "r");

	if (!cmdf)
		return;

	int i = strlen(block->icon);
	fgets(tempstatus+i, CMDLENGTH-i-delimLen, cmdf);
	i = strlen(tempstatus);

	// if both the block and command output are not empty
	if (i != 0) {
		// only chop off newline if one is present at the end
		i = tempstatus[i-1] == '\n' ? i-1 : i;
		if (delim[0] != '\0')
			strncpy(tempstatus+i, delim, delimLen);
		else
			tempstatus[i++] = '\0';
	}

	strcpy(output, tempstatus);
	pclose(cmdf);
}

void getcmds(int time) {
	const Cell* current;
	for (unsigned int i = 0; i < LENGTH(cells); i++) {
		current = cells + i;
		if ((current->interval != 0 && time % current->interval == 0) ||
			time == -1)
			getcmd(current, statusbar[i]);
	}
}

void getsigcmds(unsigned int signal) {
	const Cell *current;
	for (unsigned int i = 0; i < LENGTH(cells); i++) {
		current = cells + i;
		if (current->signal == signal)
			getcmd(current, statusbar[i]);
	}
}

void setupsignals() {
#ifndef __OpenBSD__
	// initialize all real time signals with the dummy handler
    for (int i = SIGRTMIN; i <= SIGRTMAX; i++)
        signal(i, dummysighandler);
#endif

	for (unsigned int i = 0; i < LENGTH(cells); i++) {
		if (cells[i].signal > 0)
			signal(SIGMINUS+cells[i].signal, sighandler);
	}

}

int getstatus(char *str, char *last) {
	strcpy(last, str);
	str[0] = '\0';

	for (unsigned int i = 0; i < LENGTH(cells); i++)
		strcat(str, statusbar[i]);

	str[strlen(str) - strlen(delim)] = '\0';
	return strcmp(str, last); // 0 if they are the same
}

#ifdef HAS_X
void setroot() {
	// Only set root if text has changed.
	if (!getstatus(statusstr[0], statusstr[1]))
		return;

	XStoreName(dpy, root, statusstr[0]);
	XFlush(dpy);
}

int setupX() {
	dpy = XOpenDisplay(NULL);
	if (!dpy) {
		fprintf(stderr, "ERROR: failed to open display!\n");
		return 1;
	}

	screen = DefaultScreen(dpy);
	root   = RootWindow(dpy, screen);
	return 1;
}
#endif

void pstdout() {
	// Only write out if text has changed.
	if (!getstatus(statusstr[0], statusstr[1]))
		return;

	printf("\r%s", statusstr[0]);
	fflush(stdout);
}

void statusloop() {
	setupsignals();
	int i = 0;
	getcmds(-1);

	while (1) {
		getcmds(i++);
		writestatus();

		if (!statusContinue)
			break;
	}
}

#ifndef __OpenBSD__
/* this signal handler should do nothing */
void dummysighandler(int signum) { return; }
#endif

void sighandler(int signum) {
	getsigcmds(signum-SIGPLUS);
	writestatus();
}

void termhandler(int sig) {
	statusContinue = 0;
	// add a newline in case we're
	// printing to stdout
	if (writestatus == pstdout) printf("\n");
}

int main(int argc, char** argv) {
	// Handle command line arguments
	for (int i = 0; i < argc; i++) {
		if (!strcmp("-d", argv[i])) // setting delimiter character?
			strncpy(delim, argv[++i], delimLen);
		else if (!strcmp("-p", argv[i])) // printing to stdout?
			writestatus = pstdout;
	}

#ifdef HAS_X
	if (!setupX())
		return 1;
#endif

	delimLen = MIN(delimLen, strlen(delim));
	delim[delimLen++] = '\0';
	signal(SIGTERM, termhandler);
	signal(SIGINT, termhandler);
	statusloop();

#ifdef HAS_X
	XCloseDisplay(dpy);
#endif

	return 0;
}
