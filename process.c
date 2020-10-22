/* process.c -- process implementation.
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Mon Sep 21 09:15:25 EEST 2020
 * Copyright: (C) 1992-2020 Luis Colorado.  All rights reserved.
 * License: BSD
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "main.h"
#include "process.h"
#include "timestamp.h"

#define logger proc->logger
#define chr &proc->chrono

void shut_socket(struct process *proc)
{
	int res = shutdown(proc->fd_to, SHUT_WR);
	if (res < 0) {
		ERR(EXIT_FAILURE,
			"SHUTDOWN %s %s (ERR %d)\n",
			proc->messg,
			strerror(errno), errno);
	}
	INFO("%s: SHUTDOWN OK\n", proc->messg);
}

void *process(void *param)
{
	struct process *proc = param;

	startTs(&proc->chrono);

	int size;

	while ((size = read (proc->fd_from, proc->buffer, BUFFER_SIZE)) > 0) {
		char *p = proc->buffer;
		INFO("%s: %d bytes read\n", proc->messg, size);
		/* PERHAPS WE CAN'T DO IT IN ONE CHUNK */
		while (size > 0) {
			ssize_t res = write (proc->fd_to, p, size);
			if (res < 0) {
				ERR(EXIT_FAILURE,
					"write to %s: %s (ERR %d)\n",
					proc->to, strerror(errno), errno);
			} /* if */
			size -= res; p += res;
		} /* while */
	} /* while */

	if (size < 0) {
		ERR(EXIT_FAILURE,
			"read from %s: %s (ERR %d)\n",
			proc->from, strerror(errno), errno);
	} else { /* size == 0 ==> EOF */
		INFO("%s: EOF\n", proc->messg);
		if (proc->at_eof)
			proc->at_eof(proc);
	}
	return NULL;
} /* process */
