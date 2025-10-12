#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#if __has_include(<X11/Xlib.h>)
#include <X11/Xlib.h>
#define HAS_X
#endif

#if !__linux__
#define SIGPLUS	     SIGUSR1+1
#define SIGMINUS     SIGUSR1-1
#else
#define SIGPLUS	     SIGRTMIN
#define SIGMINUS     SIGRTMIN
#endif

#define LENGTH(X)    (sizeof(X) / sizeof(X[0]))
#define MIN(a, b)    ((a < b) ? a : b)

typedef struct {
	char* icon;
	char* command;
	unsigned int interval;
	unsigned int signal;
} Cell;

#include "config.h"

#define STATUSLENGTH (LENGTH(cells) * CMDLENGTH + 1)

#if __linux__
void dummysighandler(int num);
#endif

static void getcmds(unsigned int time, unsigned int signal);
static void signalsetup(void);
static void sighandler(int signum);
static void statusloop(void);
static void termhandler(int sig);
static void pstdout(void);
static int  getstatus(void);

#ifdef HAS_X
static Display* dpy;
static Window root;
static void setroot(void);
static void (*writestatus) (void) = setroot;
static int screen;
static int Xsetup(void);
#else
static void (*writestatus) (void) = pstdout;
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

	if (!cmdf) return;

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
		if ((current->interval > 0 && time % current->interval == 0) ||
			time == -1 || current->signal == signal)
			getcmd(current, statusbar[i]);
	}
}

void signalsetup(void) {
#if __linux__
	// initialize all real time signals with the dummy handler
    for (int i = SIGRTMIN; i <= SIGRTMAX; ++i)
        signal(i, dummysighandler);
#endif

	for (unsigned int i = 0; i < LENGTH(cells); ++i) {
		if (cells[i].signal > 0)
			signal(SIGMINUS + cells[i].signal, sighandler);
	}
}

int getstatus(void) {
	strcpy(statusstr[1], statusstr[0]);

	char* str = statusstr[0];
	str[0] = '\0';

	for (unsigned int i = 0; i < LENGTH(cells); ++i)
		strcat(statusstr[0], statusbar[i]);

	// terminate our status string properly so that
	// the program knows where it ends
	str[strlen(statusstr[0]) - strlen(delim)] = '\0';
	return strcmp(statusstr[0], statusstr[1]); // 0 if they are the same
}

#ifdef HAS_X
void setroot(void) {
	XStoreName(dpy, root, statusstr[0]);
	XFlush(dpy);
}

int Xsetup(void) {
	dpy = XOpenDisplay(NULL);

	if (!dpy) {
		fprintf(stderr, "SCells: failed to open display");
		return 2;
	}

	screen = DefaultScreen(dpy);
	root   = RootWindow(dpy, screen);
	return 1;
}
#endif

void pstdout(void) {
	printf("\r%s", statusstr[0]);
	fflush(stdout);
}

void statusloop(void) {
	signalsetup();
	int i = 0;

	getcmds(-1, 0);

	while (statusContinue) {
		getcmds(++i, 0);

		// only write status if text has changed
		if (!getstatus()) continue;
		writestatus();

		sleep(1);
	}
}

#if __linux__
// used to initialize all real-time signals
void dummysighandler(int signum) { return; }
#endif

void sighandler(int signum) {
	getcmds(0, signum - SIGPLUS);
	if (!getstatus()) return;
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
				fprintf(stderr, "SCells: a character must be specified");
				return 1;
			}
		}
		else if (!strcmp("-p", argv[i])) // printing to stdout?
			writestatus = pstdout;
		else if (!strcmp("-h", argv[i])) { // wanting to get help info?
			printf("Usage: %s <options>\n"
					"<options> may be one of:\n"
					"-d '<char>'  set delimiter character to <char>\n"
					"-p           print output to stdout instead\n"
					"-h           show this help message and quit\n",
					argv[0]);
			return 0;
		}
	}

#ifdef HAS_X
	if (Xsetup() == 2) return 1;
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
