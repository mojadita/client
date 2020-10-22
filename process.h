/* process.h -- interface with function process.
 * Author: Luis Colorado <luiscolorado@gmail.com>
 * Date: Mon Sep 21 09:13:04 EEST 2020
 * Copyright: (C) 1992-2020 Luis Colorado.  All rights reserved.
 * License: BSD.
 */
#ifndef _PROCESS_H
#define _PROCESS_H

#include <stdio.h>

#define BUFFER_SIZE		(65536)

/* process info, one per direction */
struct process {
	int	fd_from,
		fd_to;
	FILE *logger;
	void (*at_eof)(struct process *p);
	off_t offset;
	char *from;
	char *to;
	char *messg;
	char *close_messg;
	char *buffer;
	struct chrono chrono;
};

void *process(void *param);

void shut_socket(struct process *proc);
void close_output(struct process *proc);

#endif /* _PROCESS_H */
