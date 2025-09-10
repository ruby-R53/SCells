/*
 * modify this file to change what commands 
 * output to your statusbar, and recompile
 * using make
 */
static const Cell cells[] = {
	// "[icon]", "[cmd]", "[upd. interval]", "[upd. sig.]"
	{"", "date +'%a %b %d %X'",	1, 0},
	{"", "wpctl get-volume @DEFAULT_AUDIO_SINK@", 0, 10},
};

/* 
 * sets delimiter between status commands,
 * NULL ('\0') means no delimiter
 */
static char delim[]  = " | ";
static int  delimLen = 3;

/*
 * maximum length of the commands, change
 * accordingly
 */
#define CMDLENGTH 50
