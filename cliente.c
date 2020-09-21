/* $Id: cliente.c,v 1.6 2012/01/21 18:14:31 luis Exp $
 * Author: Luis Colorado <lc@luiscoloradosistemas.com>
 * Date: Thu Feb 26 12:44:15 MET 1998
 *
 * Copyright (c) 1998-2017 by LUIS COLORADO
 * All rights reserved.
 *
 * This software was developed by LUIS COLORADO
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code MUST retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form MUST reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "main.h"
#include "process.h"

#define DEFAULT_SERVER		"127.0.0.1"
#define DEFAULT_SERVICE		"telnet"
#define PROGNAME			"cliente"

#define FLAG_DEBUG      (1 << 0)
#define FLAG_OPTEOFLOCL	(1 << 1)
#define FLAG_OPTEOFCONN (1 << 2)

int flags = 0;

#define N				(2)

struct process proc[N] = {
	{	.fd_in        =  0,
		.fd_out       = -1,
		.flag_eof     = FLAG_OPTEOFLOCL,
		.offset       =  0L,
	 	.from         = "STDIN",
		.messg        = ">>> OUTPUT TO SOCKET",
		.what_to_shut = SHUT_WR,
	},{
		.fd_in        = -1,
		.fd_out       =  1,
		.flag_eof     = FLAG_OPTEOFCONN,
		.offset       =  0L,
		.from         = "SOCKET",
		.messg        = "<<< INPUT FROM SOCKET",
		.what_to_shut = SHUT_RD,
	},
};

struct process *proc_end = proc + N;

void do_usage (void);

int main (int argc, char **argv)
{
	int opt;
	struct sockaddr_in server;
	struct servent *service, fallback;
	struct hostent *host;
	char *servername = DEFAULT_SERVER;
	char *serverport = DEFAULT_SERVICE;
	struct process *p;

	static struct timeval timeout = { 0, 0 },
		*t;

	/* process the program options... */
	while ((opt = getopt(argc, argv, "h:p:dt:cl")) != EOF) {
		switch (opt) {
		case 'p':  serverport = optarg; break;
		case 'h':  servername = optarg; break;
		case 'd':  flags |= FLAG_DEBUG; break;
		case 't':  timeout.tv_sec = abs(atoi(optarg)); break;
		case 'l':  flags |= FLAG_OPTEOFLOCL; break;
		case 'c':  flags |= FLAG_OPTEOFCONN; break;
		default: do_usage();
		} /* switch */
	} /* while */

	t = (timeout.tv_sec > 0) ? &timeout : NULL;

	/* Obtain the server/port info */
	service = getservbyname(serverport, "tcp");
	if (!service)
		service = getservbyport(htons(atoi(serverport)), "tcp");
	if (!service) {
		service = &fallback;
		fallback.s_port = htons(atoi(serverport));
	} /* if */

	host = gethostbyname(servername);

	if (!host) {
		ERR(EXIT_FAILURE, "%s does not exist\n",
			servername);
	} /* if */

	/* Construct the sockaddr_in for the connect system call */
	server.sin_family = AF_INET;
	server.sin_port = service->s_port;
	server.sin_addr = *(struct in_addr *)(host->h_addr_list[0]);

	if (flags & FLAG_DEBUG) {
		WARN("Trying %s:%d\n",
			inet_ntoa(server.sin_addr),
			ntohs(server.sin_port));
	} /* if */

	/* Connect to the server */
	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0) {
		ERR(EXIT_FAILURE, "SOCKET: %s (ERR %d)\n",
			strerror(errno), errno);
	} /* if */
	int res = connect(sd,
			(struct sockaddr *)&server, sizeof server);
	if (res < 0) {
		ERR(EXIT_FAILURE, "CONNECT: %s (ERR %d)",
			strerror(errno), errno);
	} /* if */

	if (flags & FLAG_DEBUG) {
		fprintf(stderr, "Connected!\n");
	} /* if */

	proc[0].fd_out = sd;
	proc[1].fd_in  = sd;

	/* main loop */
	for (;;) {
		fd_set readset;
		int max_fd = -1;

		/* prepare the select call... */
		FD_ZERO(&readset);
		for (p = proc; p < proc_end; p++) {
			if(!p->eof_in) {
				FD_SET(p->fd_in, &readset);
				if (max_fd < p->fd_in) max_fd = p->fd_in;
			}
		}

		/* if no fd to monitor, then exit */
		if (max_fd < 0) break;
			
		res = select(max_fd + 1, &readset, NULL, NULL, t);

		int should_exit = 0;
		switch (res) {
		case -1: /* error in select */
			ERR(EXIT_FAILURE, "SELECT: %s (ERR %d)\n",
				strerror(errno), errno);
			/* NOTREACHED */
		case 0: /* Timeout */
			ERR(EXIT_SUCCESS, "Timeout\n");
			/* NOTREACHED */
		default: /* Data on some direction */
			for(p = proc; p < proc_end; p++) {
				if (FD_ISSET(p->fd_in, &readset)) {
					eof = process(p);
					if (eof && (flags & p->flag_eof)) {
						if (flags & FLAG_DEBUG) {
							WARN("%s EOF -> EXIT\n",
								p->from);
						} /* if */
					} /* if */
				} /* if */
			} /* for */
			break;
		} /* switch */
	} /* for (;;) */
} /* main */

void do_usage()
{
	fprintf(stderr, "Usage: " PROGNAME " [ options ...]\n");
	fprintf(stderr, "Options:\n");
  	fprintf(stderr, "  -h server  Specifies a host to contact.\n");
  	fprintf(stderr, "  -p service Specifies the port to connect to.\n");
  	fprintf(stderr, "  -d         Debug. Be verbose.\n");
  	fprintf(stderr, "  -t timeout Set a timeout in secs. (def. no timeout).\n");
	fprintf(stderr, "  -l         EOF on local side forces exit\n");
	fprintf(stderr, "  -c         EOF on connection side forces exit\n");
	exit(0);
} /* do_usage */

/* $Id: cliente.c,v 1.6 2012/01/21 18:14:31 luis Exp $ */
