/* process.c -- process implementation.
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Mon Sep 21 09:15:25 EEST 2020
 * Copyright: (C) 2005-2020 Luis Colorado.  All rights reserved.
 * License: BSD
 */

#include "main.h"
#include "process.h"

int process (struct process *proc)

		int fd_in, int fd_out, char *messg)
{
	static char buffer[BUFSIZ];
       	char *p = buffer;
	int res, tam;

	/* let's read the data into the buffer in chunks of BUFSIZ. */
	res = read (proc->fd_in, buffer, sizeof buffer);

	if (flags & FLAG_DEBUG) {
		fprintbuf(stderr, proc->offset, tam, buffer, messg);
	}

	switch (res) {
	case -1: /* READ ERROR */
		perror (PROGNAME ": read");
		exit (EXIT_FAILURE);
	case 0: /* EOF ON INPUT */
		return -1;
	default: /* WE HAVE DATA, SO WRITE IT TO fd_out */
		tam = res;
		p = buffer;
		/* PERHAPS WE CAN'T DO IT IN ONE CHUNK */
		while (tam > 0) {
			res = write (fd_out, p, tam);
			if (res == -1) {
				perror (PROGNAME": write");
				exit (EXIT_FAILURE);
			} /* if */
			tam -= res; p += res;
		} /* if */
		return 0;
	} /* switch */
} /* process */
