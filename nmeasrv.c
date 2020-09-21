/* $Id: nmeasrv.c,v 1.7 2012/01/31 21:22:47 luis Exp $
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Sat Jan 22 12:23:02     2011
 * Copyright: (C) 1992-2011 LUIS COLORADO SISTEMAS S.L.
 *		All rights reserved.
 * License: BSD
 * Description: This program binds to a default port (or
 *		or specified on command line, and redirects all
 *		input to the connections that arrive to that port.
 */

#define PROGNAME	"nmeasrv"
#define AUTHOR		"Luis Colorado <lc@luiscoloradosistemas.com>"
#define COPYRIGHT	"(C) 2012 LUIS COLORADO SISTEMAS S.L.U. ALL RIGHTS RESERVED."
#define MAX			128

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <getopt.h>

#define FLAG_DEBUG		1
#define FLAG_DEBUG2		2
#define DEFAULT_LISTEN_SZ	15

struct client_info {
	int sd;
	struct sockaddr_in sin;
};

int flags = 0;
int listen_sz = DEFAULT_LISTEN_SZ;
char *in_filename = NULL;
char *bind_address = "0.0.0.0";
char *in_port = "nmeasrv";
int in_fd = 0; /* stdin */
int out_fd = 1; /* stdout */
int sd_bind_socket = -1;
struct client_info *sd_out[MAX];
int n_out = 0;

void do_usage ()
{
	fprintf(stderr, "Usage: " PROGNAME " [ options ...]\n");
	fprintf(stderr, "Options:\n");
  	fprintf(stderr, "  -i file    Specifies a local filesystem file/device.\n");
	fprintf(stderr, "  -b ip      Specifies the bind addres to bind to (default: 0.0.0.0)\n");
	fprintf(stderr, "  -p port    Specifies alternate port to bind.\n");
	fprintf(stderr, "  -l num     Buffer size to listen(2).\n");
  	fprintf(stderr, "  -d         Debug. Be verbose.\n");
  	fprintf(stderr, "  -D         Debug. Trace read/write chunks\n");
	exit (EXIT_SUCCESS);
} /* do_usage */

int main (int argc, char **argv)
{
	int opt, sd, res;

	struct sockaddr_in our_addr;
	struct servent *service;
	struct hostent *host;
	int i;

	for (i = 0; i < MAX; i++) sd_out[i] = NULL;

	/* process the program options... */
	while ((opt = getopt(argc, argv, "i:b:p:dDl:")) != EOF) {
		switch (opt) {
		case 'i':
			/* INPUT is selected from a local input file/device.
			 * File/Device is specified as the parameter.  On EOF
			 * on input from this file, the program pauses for some
			 * time and tries to reopen the file again, so service
			 * doesn't fail. */
			 in_filename = optarg;
			 in_fd = -1; /* to initialize input as closed. */
             out_fd = -1;
			 break;
		case 'b':
			bind_address = optarg;
			break;
		case 'p':
			/* The parameter to this argument specifies an alternate
			 * port to listen to for incoming connections. */
			 in_port = optarg;
			 break;
		case 'd':
			/* Debug flag.  It makes program to trace to stderr. */
			flags |= FLAG_DEBUG;
			break;
		case 'D':
			/* Debug flag.  It makes program to trace to stderr. */
			flags |= FLAG_DEBUG2;
			break;
		case 'l':
			/* Buffer size to listen. */
			listen_sz = atoi(optarg);
			if (listen_sz <= 0) listen_sz = DEFAULT_LISTEN_SZ;
			break;
		default:
			do_usage();
			exit(EXIT_SUCCESS);
		} /* switch */
	} /* while */

	if (flags & (FLAG_DEBUG | FLAG_DEBUG2)) {
		fprintf(stderr, PROGNAME": $Id: nmeasrv.c,v 1.7 2012/01/31 21:22:47 luis Exp $\n");
		fprintf(stderr, PROGNAME": Author: "AUTHOR"\n");
		fprintf(stderr, PROGNAME": Date compiled: "__DATE__"\n");
		fprintf(stderr, PROGNAME": COPYRIGHT: "COPYRIGHT"\n");
	} /* if */

	/* Obtain the server/port info */
	service = getservbyname (in_port, "tcp");
	if (!service)
		service = getservbyport(htons(atoi(in_port)), "tcp");

	/* Construct the sockaddr_in for the connect system call */
	our_addr.sin_family = AF_INET;
	our_addr.sin_port = (service ? service->s_port : htons(atoi(in_port)));
	our_addr.sin_addr.s_addr = INADDR_ANY;

	host = gethostbyname(bind_address);
	if (host) {
		our_addr.sin_addr = *(struct in_addr *)(host->h_addr_list[0]);
	} /* if */

	/* construct the server part */
	/* SOCKET */
	if (flags & FLAG_DEBUG) {
		fprintf(stderr, PROGNAME": Opening listening socket\n");
	} /* if */
	sd = socket (AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		fprintf(stderr, PROGNAME ": socket: %s (errno = %d)\n",
			strerror(errno), errno);
		exit(EXIT_FAILURE);
	} /* if */

	/* BIND */
	if (flags & FLAG_DEBUG) {
		fprintf (stderr, PROGNAME": Trying to bind: [%s:%d]\n",
			inet_ntoa(our_addr.sin_addr),
			ntohs (our_addr.sin_port));
	} /* if */
	res = bind(sd, (const struct sockaddr *)&our_addr, sizeof our_addr);
	if (res < 0) {
		fprintf(stderr, PROGNAME ": bind: %s (errno = %d)\n",
			strerror(errno), errno);
		exit(EXIT_FAILURE);
	} /* if */
	if (ntohs(our_addr.sin_port) == 0) {
		struct sockaddr_in new_addr;
		socklen_t sz = sizeof new_addr;
		int res;
		res = getsockname(sd, (struct sockaddr *)&new_addr, &sz);
		if (res < 0) {
			fprintf(stderr, PROGNAME": getsockname: %s (errno = %d)\n",
				strerror(errno), errno);
		} else {
			fprintf(stderr, PROGNAME ": bound to [%s:%d]\n",
				inet_ntoa(new_addr.sin_addr),
				ntohs(new_addr.sin_port));
		} /* if */
	} /* if */

	/* LISTEN */
	if (flags & FLAG_DEBUG) {
		fprintf(stderr, PROGNAME": Listening on socket (listen_sz = %d)\n",
			listen_sz);
	} /* if */
	res = listen(sd, listen_sz);
	if (res < 0) {
		fprintf(stderr, PROGNAME ": listen: %s (errno = %d)\n",
			strerror(errno), errno);
		exit(EXIT_FAILURE);
	} /* if */

	/* continous MAIN loop */
	for (;;) {
		fd_set readset;
		int sd_max = 0;

		/* prepare the select call... */

		/* Construct the FD_SET for the select system call */
		FD_ZERO(&readset);

		/* The listening socket, always present */
		FD_SET(sd, &readset);
		if (sd > sd_max) sd_max = sd;

		/* The input descriptor, only if >= 0 */
		if (in_fd >= 0) {
			FD_SET(in_fd, &readset);
			if (in_fd > sd_max) sd_max = in_fd;
		} /* if */

		/* The socket descriptors */
		for (i = 0; i < MAX; i++) {
			if (sd_out[i]) {
				FD_SET(sd_out[i]->sd, &readset);
				if (sd_out[i]->sd > sd_max) sd_max = sd_out[i]->sd;
			} /* if */
		} /* for */

		res = select (sd_max+1, &readset, NULL, NULL, NULL);
		switch (res) {
		case -1: /* error in select */
			fprintf(stderr, PROGNAME ": select: %s (errno = %d)",
				strerror(errno), errno);
			exit (EXIT_FAILURE);
		case 0: /* Timeout */
			if (flags & FLAG_DEBUG) fprintf (stderr, PROGNAME ": Timeout\n");
			continue;
		default: /* Data on some direction */
			if (FD_ISSET(sd, &readset)) { /* data in listening socket. */
				struct sockaddr_in peer;
				socklen_t peer_sz = sizeof peer;
				int new_sd = accept(sd, (struct sockaddr *)&peer, &peer_sz);
				if (new_sd < 0) {
					fprintf(stderr, PROGNAME ": accept: %s (errno = %d)\n",
						strerror(errno), errno);
					continue;
				} /* if */
				for (i = 0; i < MAX; i++) if (!sd_out[i]) break;
				if (i >= MAX) {
					fprintf(stderr,
						PROGNAME ": Connection from [%s:%d] closed, MAX(%d) reached.\n",
						inet_ntoa(peer.sin_addr),
						ntohs(peer.sin_port), MAX);
					close(new_sd);
					continue;
				} /* if */
				/* If in_fd < 0 we have to open the in port */
				if (in_fd < 0) {
					out_fd = in_fd = open(in_filename, O_RDONLY);
					if (in_fd < 0) {
						fprintf(stderr, PROGNAME ": open: %s: %s (errno = %d). Closing new_sd(%d).\n",
							in_filename, strerror(errno), errno, new_sd);
						close(new_sd);
						sleep(1);
						continue;
					} /* if */
					if (flags & FLAG_DEBUG) {
						fprintf(stderr,
							PROGNAME ": opening %s for input as in_fd=%d\n",
							in_filename, in_fd);
					} /* if */
				} /* if */
				n_out++;
				sd_out[i] = malloc(sizeof(struct client_info));
				sd_out[i]->sd = new_sd;
				sd_out[i]->sin = peer;
				if (flags & FLAG_DEBUG) {
					fprintf(stderr,
						PROGNAME ": New connection from [%s:%d]: slot %d, sd %d\n",
						inet_ntoa(sd_out[i]->sin.sin_addr),
						ntohs(sd_out[i]->sin.sin_port),
						i, sd_out[i]->sd);
				} /* if */
			} /* if */
			if ((in_fd >= 0) && FD_ISSET(in_fd, &readset)) {
				static char buffer [1024];
				int n = read(in_fd, buffer, sizeof buffer);
				switch (n) {
				case -1: /* ERROR */
					fprintf(stderr, PROGNAME ": read(%s, in_fd=%d): %s (errno = %d)\n",
						in_fd ? in_filename : "<stdin>",
						in_fd, strerror(errno), errno);
					close(in_fd); in_fd = -1;
					break;
				case 0: /* EOF */
					if (flags & FLAG_DEBUG) {
						fprintf(stderr, PROGNAME ": read(%s, in_fd=%d): EOF\n",
							in_fd ? in_filename : "<stdin>", in_fd);
					} /* if */
					if (in_fd > 0) { /* don't close stdin */
						close(in_fd); in_fd = -1;
						for (i = 0; i < MAX; i++) {
							if (sd_out[i]) {
								if (flags & FLAG_DEBUG) {
									fprintf(stderr, PROGNAME 
										": Closing connection to [%s:%d]: slot %d, "
										"fd=%d: input file closed\n",
										inet_ntoa(sd_out[i]->sin.sin_addr), ntohs(sd_out[i]->sin.sin_port),
										i, sd_out[i]->sd);
								} /* if */
								close(sd_out[i]->sd);
								free(sd_out[i]);
								sd_out[i] = NULL;
							} /* if */
						} /* for */
					} /* if */
					continue;
				default:
					if (flags & FLAG_DEBUG2) {
						fprintf(stderr, " [fd=%d, read=%d]", in_fd, n);
					} /* if */
					for (i = 0; i < MAX; i++) {
						if (sd_out[i]) {
							int res = write(sd_out[i]->sd, buffer, n);
							if (res != n) {
								if (flags & FLAG_DEBUG) {
									fprintf(stderr,
										PROGNAME ": Closing connection to [%s:%d]: write: slot %d, fd=%d: (res(%d) != n(%d))\n",
										inet_ntoa(sd_out[i]->sin.sin_addr), ntohs(sd_out[i]->sin.sin_port),
										i, sd_out[i]->sd, res, n);
								} /* if */
								close(sd_out[i]->sd); free(sd_out[i]); sd_out[i] = NULL;
								n_out--;
								if ((n_out == 0) && (in_fd > 0)) {
									if (flags & FLAG_DEBUG) {
										fprintf(stderr,
											PROGNAME ": Closing input file %s fd=%d: No clients are connected\n",
											in_filename, in_fd);
									} /* if */
									close(in_fd); in_fd = -1;
								} /* if */
							} /* if */
						} /* if */
					} /* for */
				} /* switch */
			} /* if */
			for (i = 0; i < MAX; i++) {
				if (sd_out[i] && FD_ISSET(sd_out[i]->sd, &readset)) {
					char buffer[1024];
					int r = read(sd_out[i]->sd, buffer, sizeof buffer);
					switch (r) {
					case -1: /* read error */
						fprintf(stderr, PROGNAME ": Closing connection to [%s:%d]: read: slot=%d, fd=%d: %s (errno = %d)\n",
							inet_ntoa(sd_out[i]->sin.sin_addr), ntohs(sd_out[i]->sin.sin_port),
							i, sd_out[i]->sd, strerror(errno), errno);
						close(sd_out[i]->sd); free(sd_out[i]); sd_out[i] = NULL;
						n_out--;
						if ((n_out == 0) && (in_fd > 0)) {
							if (flags & FLAG_DEBUG) {
								fprintf(stderr,
									PROGNAME ": Closing input file %s fd=%d: No clients are connected\n",
									in_filename, in_fd);
							} /* if */
							close (in_fd); in_fd = -1;
						} /* if */
						break;
					case 0: /* EOF on input */
						if (flags & FLAG_DEBUG) {
							fprintf(stderr, PROGNAME ": Closing connection to [%s:%d]: read: slot=%d, fd=%d: EOF\n",
								inet_ntoa(sd_out[i]->sin.sin_addr), ntohs(sd_out[i]->sin.sin_port),
								i, sd_out[i]->sd);
						} /* if */
						close(sd_out[i]->sd); free(sd_out[i]); sd_out[i] = NULL;
						n_out--;
						if ((n_out == 0) && (in_fd > 0)) {
							if (flags & FLAG_DEBUG) {
								fprintf(stderr,
									PROGNAME ": Closing input file %s fd=%d: No clients are connected\n",
									in_filename, in_fd);
							} /* if */
							close (in_fd); in_fd = -1;
						} /* if */
						break;
					default:
						if (flags & FLAG_DEBUG2) {
							fprintf(stderr, " <[%s:%d]: slot=%d, fd=%d: read=%d>",
								inet_ntoa(sd_out[i]->sin.sin_addr), ntohs(sd_out[i]->sin.sin_port),
								i, sd_out[i]->sd, r);
						} /* if */
                        if (out_fd >= 0) {
                            /* only one attempt, for now */
                            write(out_fd, buffer, r);
                        }
						break;
					} /* switch */
				} /* if */
			} /* for */
			break;
		} /* switch */
	} /* for (;;) */
} /* main */

/* $Id: nmeasrv.c,v 1.7 2012/01/31 21:22:47 luis Exp $ */
