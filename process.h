/* process.h -- interface with function process.
 * Author: Luis Colorado <luiscolorado@gmail.com>
 * Date: Mon Sep 21 09:13:04 EEST 2020
 * Copyright: (C) 1992-2020 Luis Colorado.  All rights reserved.
 * License: BSD.
 */
#ifndef _PROCESS_H
#define _PROCESS_H

/* process info, one per direction */
struct process {
	int fd_in, fd_out;
	int at_eof;
	off_t offset;
	char *from;
	char *messg;
	int do_shutdown;
	int what_to_chek;
};

int process (struct process *proc);

#endif /* _PROCESS_H */
