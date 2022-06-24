#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include "timer.h"

static int secs = 0;
static volatile sig_atomic_t *timeup;

static void timer_timeup(int unused);
static void timer_install_handler();

static void timer_timeup(int unused)
{
	(void)unused;
	*timeup = 1;
}

void timer_init_xboard(char *level)
{
	int moves, base, inc;
	int ret = sscanf(level, "level %d %d %d", &moves, &base, &inc);

	if (ret == 3 && moves == 0)
		secs = inc;
	else
		secs = 5;

	timer_install_handler();
}

void timer_init_secs(int n)
{
	secs = n;
	timer_install_handler();
}

static void timer_install_handler()
{
	struct sigaction sigalarm_action;
	sigalarm_action.sa_handler = timer_timeup;
	sigemptyset(&sigalarm_action.sa_mask);
	sigalarm_action.sa_flags = 0;
	sigaction(SIGALRM, &sigalarm_action, 0);
}

void timer_begin(volatile sig_atomic_t *i)
{
	if (secs < 1)
		timer_init_secs(5);

	timeup = i;
	alarm(secs);
}

void timer_end(void)
{
	alarm(0);
}
