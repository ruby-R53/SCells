#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

typedef struct {
	char* icon;
	char* command;
	unsigned int interval;
	unsigned int signal;
} Cell;

#include "config.h"

#if __has_include(<X11/Xlib.h>)
#include <X11/Xlib.h>
#define HAS_X
#endif

#ifdef __OpenBSD__
#define SIGPLUS	     SIGUSR1+1
#define SIGMINUS     SIGUSR1-1
#else
#define SIGPLUS	     SIGRTMIN
#define SIGMINUS     SIGRTMIN
#endif

#define LENGTH(X)    (sizeof(X) / sizeof(X[0]))
#define STATUSLENGTH (LENGTH(cells) * CMDLENGTH + 1)
#define MIN(a, b)    ((a < b) ? a : b)

#define ERROR(MSG)   \
	(fprintf(stderr, "\033[1;31mERROR\033[37m:\033[m %s\n", MSG))
// highlight it with bright red :)

#ifndef __OpenBSD__
void dummysighandler(int num);
#endif

void sighandler(int num);
void getcmds(unsigned int time, unsigned int signal);
void signalsetup();
void sighandler(int signum);
int  getstatus(char* str, char* last);
void statusloop();
void termhandler(int sig);
void pstdout();

#ifdef HAS_X
static Display* dpy;
static Window root;
void   setroot();
static int screen;
static int setupX();
static void (*writestatus) () = setroot;
#else
static void (*writestatus) () = pstdout;
#endif

static char statusbar[LENGTH(cells)][CMDLENGTH] = {0};
static char statusstr[2][STATUSLENGTH];
static int  statusContinue = 1;

// opens process *cmd and stores output in *output
void getcmd(const Cell* cell, char* output) {
	// make sure status is the same until output is ready
	char tempstatus[CMDLENGTH] = {0};
	strcpy(tempstatus, cell->icon);
	FILE* cmdf = popen(cell->command, "r");

	if (!cmdf)
		return;

	int i = strlen(cell->icon);
	fgets(tempstatus + i, CMDLENGTH - delimLen - i, cmdf);
	i = strlen(tempstatus);

	// if both the cell and command output are not empty
	if (i != 0) {
		// only chop off newline if present at the end
		i = (tempstatus[i-1] == '\n') ? i - 1 : i;
		if (delim[0] != '\0')
			strncpy(tempstatus+i, delim, delimLen);
		else
			tempstatus[++i] = '\0';
	}

	strcpy(output, tempstatus);
	pclose(cmdf);
}

void getcmds(unsigned int time, unsigned int signal) {
	const Cell* current;

	for (unsigned int i = 0; i < LENGTH(cells); ++i) {
		current = cells + i;
		if ((current->interval != 0 && time % current->interval == 0) ||
			time == -1 || current->signal == signal)
			getcmd(current, statusbar[i]);
	}
}

void signalsetup() {
#ifndef __OpenBSD__
	// initialize all real time signals with the dummy handler
    for (int i = SIGRTMIN; i <= SIGRTMAX; ++i)
        signal(i, dummysighandler);
#endif

	for (unsigned int i = 0; i < LENGTH(cells); ++i) {
		if (cells[i].signal > 0)
			signal(SIGMINUS + cells[i].signal, sighandler);
	}
}

int getstatus(char* str, char* last) {
	strcpy(last, str);
	str[0] = '\0';

	for (unsigned int i = 0; i < LENGTH(cells); ++i)
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
		ERROR("failed to open display!");
		return 2;
	}

	screen = DefaultScreen(dpy);
	root   = RootWindow(dpy, screen);
	return 1;
}
#endif

void pstdout() {
	// Only write out if text has changed.
	if (!getstatus(statusstr[0], statusstr[1])) return;

	printf("\r%s", statusstr[0]);
	fflush(stdout);
}

void statusloop() {
	signalsetup();
	int i = 0;
	getcmds(-1, 0);

	while (1) {
		getcmds(++i, 0);
		writestatus();

		if (!statusContinue) break;
		sleep(1.);
	}
}

#ifndef __OpenBSD__
// used to initialize all real-time signals
void dummysighandler(int signum) { return; }
#endif

void sighandler(int signum) {
	getcmds(0, signum - SIGPLUS);
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
	for (int i = 0; i < argc; ++i) {
		if (!strcmp("-d", argv[i])) { // setting delimiter character?
			if (argv[1+i] != NULL)
				strncpy(delim, argv[++i], delimLen);
			else {
				ERROR("a character must be specified!");
				return 1;
			}
		}
		else if (!strcmp("-p", argv[i])) // printing to stdout?
			writestatus = pstdout;
		else if (!strcmp("-h", argv[i])) { // wanting to get help info?
			printf("Usage: %s <options>\n"
					"<options> may be one of:\n"
					"-d '<char>'  set delimiter character to <char>\n"
					"-p           print output to stdout instead\n",
					argv[0]);
			return 0;
		}
	}

#ifdef HAS_X
	if (setupX() == 2)
		return 1;
#endif

	delimLen = MIN(delimLen, strlen(delim));
	delim[++delimLen] = '\0';
	signal(SIGTERM, termhandler);
	signal(SIGINT, termhandler);
	statusloop();

#ifdef HAS_X
	XCloseDisplay(dpy);
#endif

	return 0;
}
