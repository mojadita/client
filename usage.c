/* usage.c -- output usage for program.
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Mon Sep 21 22:06:49 EEST 2020
 * Copyright: (C) 1992-2020 Luis Colorado.  All rights reserved.
 * License: BSD.
 */

#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "usage.h"

#define logger stderr

#define chr &chrono

static struct chrono chrono;

void do_usage(char *prog)
{
    startTs(&chrono);
    INFO("Usage: %s [ options ...]\n", prog);
    INFO("Options:\n");
    INFO("  -h server  Specifies a host to contact.\n");
    INFO("  -p service Specifies the port to connect to.\n");
    INFO("  -d         Debug. Be verbose.\n");
    INFO("  -t timeout Set a timeout in secs. (def. no timeout).\n");
    INFO("  -l         EOF on local side forces exit\n");
    INFO("  -c         EOF on connection side forces exit\n");

    exit(EXIT_SUCCESS);
} /* do_usage */
