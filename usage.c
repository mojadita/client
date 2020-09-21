/* usage.c -- output usage for program.
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: 
 * Copyright: (C) 2015-2020 Luis Colorado.  All rights reserved.
 * License: BSD.
 */

#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "usage.h"

void do_usage(char *prog)
{
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
