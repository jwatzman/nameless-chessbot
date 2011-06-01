#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include "timer.h"

static int secs = 0;
static volatile int *timeup;

static void timer_timeup(int unused);

static void timer_timeup(int unused)
{
	(void)unused;
	*timeup = 1;
}

void timer_init(char *level)
{
	int moves, base, inc;
	int ret = sscanf(level, "level %d %d %d", &moves, &base, &inc);

	if (ret == 3 && moves == 0)
	{
		fprintf(stderr, "using inc %d\n", inc);
		secs = inc;
	}
	else
	{
		fprintf(stderr, "using default\n");
		secs = 5;
	}

	struct sigaction sigalarm_action;
	sigalarm_action.sa_handler = timer_timeup;
	sigemptyset(&sigalarm_action.sa_mask);
	sigalarm_action.sa_flags = 0;
	sigaction(SIGALRM, &sigalarm_action, 0);
}

void timer_begin(volatile int *i)
{
	if (secs < 1)
		timer_init("");

	timeup = i;
	alarm(secs);
}

void timer_end(void)
{
	alarm(0);
}
