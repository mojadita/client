/* process.c -- process implementation.
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Mon Sep 21 09:15:25 EEST 2020
 * Copyright: (C) 2005-2020 Luis Colorado.  All rights reserved.
 * License: BSD
 */

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "main.h"
#include "process.h"
#include "fprintbuf.h"

int process (struct process *proc)
{
	static char buffer[65536];
    char *p = buffer;

	/* let's read the data into the buffer in chunks of BUFSIZ. */
	int tam = read (proc->fd_in, buffer, sizeof buffer);

	switch (tam) {
	case -1: /* READ ERROR */
		ERR(EXIT_FAILURE,
			"READ: %s (ERR %d)",
			strerror(errno), errno);
	case 0: /* EOF ON INPUT */
		return -1;
	default: /* WE HAVE DATA, SO WRITE IT TO fd_out */

		if (flags & FLAG_DEBUG) {
			fprintbuf(stderr,
				proc->offset,
				tam, buffer,
				"%s (%d bytes)", proc->messg, tam);
			proc->offset += tam;
		}

		p = buffer;
		/* PERHAPS WE CAN'T DO IT IN ONE CHUNK */
		while (tam > 0) {
			ssize_t res = write (proc->fd_out, p, tam);
			if (res < 0) {
				ERR(EXIT_FAILURE,
					"WRITE -> %s: %s (ERR %d)\n",
					proc->from, strerror(errno), errno);
			} /* if */
			tam -= res; p += res;
		} /* while */
	} /* switch */
	return 0;
} /* process */
