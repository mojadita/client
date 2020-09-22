/* timestamp.c -- routine to print timestamps.
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 * Date:
 * Copyright: (C) 2020 Luis Colorado.  All rights reserved.
 * License: BSD.
 */

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "main.h"
#include "timestamp.h"

#define P(_fmt, _var) do {			  \
		if (_var != 0) {			  \
			ssize_t n = snprintf(buf, \
				bufsz,  			  \
				"%s"_fmt, sep, _var); \
			sep = " ";				  \
			buf += n; bufsz -= n;	  \
			ret_val += n;			  \
		}							  \
	} while(0)

static size_t lap(
		struct timeval *from,
		struct timeval to,
		char *buf,
		size_t bufsz)
{
	if (from->tv_usec > to.tv_usec) {
		to.tv_usec += 1000000;
		to.tv_sec--;
	}
	to.tv_sec  -= from->tv_sec;
	to.tv_usec -= from->tv_usec;

	int ss = to.tv_sec % 60; /* secs */
	to.tv_sec /= 60;
	int mm = to.tv_sec % 60; /* mins */
	to.tv_sec /= 60;
	int hh = to.tv_sec % 24; /* hours */
	to.tv_sec /= 24;
    int dd = to.tv_sec % 7;  /* days */
	to.tv_sec /= 7;			 /* weeks */
	char *sep = "";

	ssize_t ret_val = 0;
	P("%ldw", to.tv_sec);
	P("%dd",  dd);
	P("%dh",  hh);
	P("%dm",  mm);
	ret_val += snprintf(
		buf, bufsz,
		"%s%d.%06lds",
		sep, ss,
		to.tv_usec);

	return ret_val;
}
#undef P

static struct timeval origin = {0, 0};
static struct timeval last   = {0, 0};

int startTs(void)
{
	int res = gettimeofday(&origin, NULL);
	if (res >= 0) {
		last = origin;
	}
	return res;
}

static char buffer[256];

char *getTs(void)
{
	char *buf    = buffer;
	size_t bufsz = sizeof buffer;
	struct timeval now;

	int res = gettimeofday(&now, NULL);
	if (res < 0) return NULL;

	size_t n = lap(&last, now, buf, bufsz);
	buf     += n;
	bufsz   -= n;

	n  = snprintf(buf, bufsz, " ");
	buf     += n;
	bufsz   += n;

	n        = lap(&origin, now, buf, bufsz);
	buf     += n;
	bufsz	-= n;

	last     = now;

	return buffer;
}

#undef APP
