/* $Id: nmeasrv.c,v 1.3 2012/01/21 16:01:56 luis Exp $
 * Author: Luis Colorado <luis.colorado@hispalinux.es>
 * Date: Sat Jan 22 12:23:02     2011
 * Disclaimer: (C) 2011 LUIS COLORADO SISTEMAS S.L.
 *		All rights reserved.
 * Description: This program binds to a default port (or
 *		or specified on command line, and redirects all
 *		input to the connections that arrive to that port.
 * $Log: nmeasrv.c,v $
 * Revision 1.3  2012/01/21 16:01:56  luis
 * Ya funciona como servidor y cierra la conexion con el GPS cuando se agotan
 * los clientes.
 *
 * Revision 1.2  2011-01-23 13:11:46  luis
 * * input device is open only in presence of connections, closed otherwise
 *   (except if input is from stdin, which is not closed).
 * * corrected an error with 'flags' being used for two conflicting variables.
 * * Traffic log consists now in only the number of bytes between square
 *   brackets.
 *
 * Revision 1.1  2011-01-23 00:59:40  luis
 * * Changed author email in cliente.c
 * * Added comments on closing braces to pair the control sentence.
 * * Added nmeasrv.c as an example of server listening for connections on
 *   a specific port.  Doesn't work well when specifying input file yet.
 *
 */

#define PROGNAME	"nmeasrv"
#define MAX			2

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <getopt.h>

#define FLAG_DEBUG		1
#define FLAG_DEBUG2		2
#define DEFAULT_LISTEN_SZ	15

int flags = 0;

int listen_sz = DEFAULT_LISTEN_SZ;

char *in_filename = NULL;
char *in_port = "nmeasrv";
int in_fd = 0;

int sd_bind_socket = -1;

int sd_out[MAX];
int n_out = 0;

void do_usage ()
{
	fprintf (stderr, "Usage: " PROGNAME " [ options ...]\n");
	fprintf (stderr, "Options:\n");
  	fprintf (stderr, "  -i file    Specifies a local filesystem file/device.\n");
	fprintf (stderr, "  -p port    Specifies alternate port to bind.\n");
	fprintf (stderr, "  -l num     Buffer size to listen(2).\n");
  	fprintf (stderr, "  -d         Debug. Be verbose.\n");
  	fprintf (stderr, "  -D         Debug. Trace read/write chunks\n");
	exit (EXIT_SUCCESS);
} /* do_usage */

int main (int argc, char **argv)
{
	int opt, sd, res;

	struct sockaddr_in our_addr;
	struct servent *service;
	int i;

	for (i = 0; i < MAX; i++) sd_out[i] = -1;

	/* process the program options... */
	while ((opt = getopt(argc, argv, "i:p:dl:")) != EOF) {
		switch (opt) {
		case 'i':
			/* INPUT is selected from a local input file/device.
			 * File/Device is specified as the parameter.  On EOF
			 * on input from this file, the program pauses for some
			 * time and tries to reopen the file again, so service
			 * doesn't fail. */
			 in_filename = optarg;
			 in_fd = -1; /* to initialize input as closed. */
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

	/* Obtain the server/port info */
	service = getservbyname (in_port, "tcp");
	if (!service)
		service = getservbyport(htons(atoi(in_port)), "tcp");

	/* Construct the sockaddr_in for the connect system call */
	our_addr.sin_family = AF_INET;
	our_addr.sin_port = (service ? service->s_port : htons(atoi(in_port)));
	our_addr.sin_addr.s_addr = INADDR_ANY;

	/* construct the server part */
	/* SOCKET */
	if (flags & FLAG_DEBUG) {
		fprintf(stderr, "Opening listening socket\n");
	} /* if */
	sd = socket (AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		fprintf(stderr, PROGNAME ": socket: %s (errno = %d)\n",
			strerror(errno), errno);
		exit(EXIT_FAILURE);
	} /* if */
	/* BIND */
	if (flags & FLAG_DEBUG) {
		fprintf (stderr, "Trying to bind: [%s:%d]\n",
			inet_ntoa(our_addr.sin_addr),
			ntohs (our_addr.sin_port));
	} /* if */
	res = bind(sd, (const struct sockaddr *)&our_addr, sizeof our_addr);
	if (res < 0) {
		fprintf(stderr, PROGNAME ": bind: %s (errno = %d)",
			strerror(errno), errno);
		exit(EXIT_FAILURE);
	} /* if */
	/* LISTEN */
	if (flags & FLAG_DEBUG) {
		fprintf(stderr, "Listening on socket (listen_sz = %d)\n",
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
		struct timeval to;


		/* prepare the select call... */

		/* TIMEOUT */
		to.tv_sec = 10;
		to.tv_usec = 0;

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
			if (sd_out[i] >= 0) {
				FD_SET(sd_out[i], &readset);
				if (sd_out[i] > sd_max) sd_max = sd_out[i];
			} /* if */
		} /* for */

		res = select (sd_max+1, &readset, NULL, NULL, &to);
		switch (res) {
		case -1: /* error in select */
			fprintf(stderr, PROGNAME ": select: %s (errno = %d)",
				strerror(errno), errno);
			exit (EXIT_FAILURE);
		case 0: /* Timeout */
			if (flags & FLAG_DEBUG) {
				fprintf (stderr, PROGNAME ": Timeout\n");
			} /* if */
			continue;
		default: /* Data on some direction */
			if (FD_ISSET(sd, &readset)) { /* data in listening socket. */
				struct sockaddr_in peer;
				int peer_sz = sizeof peer;
				int new_sd = accept(sd, (struct sockaddr *)&peer, &peer_sz);
				if (new_sd < 0) {
					fprintf(stderr, PROGNAME ": accept: %s (errno = %d)\n",
						strerror(errno), errno);
					goto check_in_fd;
				} /* if */
				for (i = 0; i < MAX; i++) {
					if (sd_out[i] < 0) {
						sd_out[i] = new_sd;
						break;
					} /* if */
				} /* for */
				if (i >= MAX) {
					fprintf(stderr,
						PROGNAME ": Connection from %s:%d closed, MAX(%d) reached.\n",
						inet_ntoa(peer.sin_addr),
						ntohs(peer.sin_port));
					close(new_sd);
					goto check_in_fd;
				} /* if */
				n_out++;
				if (flags & FLAG_DEBUG) {
					fprintf(stderr,
						PROGNAME ": New connection from %s:%d (slot %d, sd %d)\n",
						inet_ntoa(peer.sin_addr),
						ntohs(peer.sin_port),
						i, sd_out[i]);
				} /* if */
				/* If in_fd < 0 we have to open the in port */
				if (in_fd < 0) {
					if (flags & FLAG_DEBUG) {
						fprintf(stderr,
							PROGNAME ": opening %s for input\n",
							in_filename);
					} /* if */
					in_fd = open(in_filename, O_RDONLY);
					if (in_fd < 0) {
						fprintf(stderr, PROGNAME ": open: %s (errno = %d)\n",
							strerror(errno), errno);
						sleep(1);
					} /* if */
					goto check_out_sd;
				} /* if */
			} /* if */
check_in_fd:
			if ((in_fd >= 0) && FD_ISSET(in_fd, &readset)) {
				static char buffer [1024];
				int n = read(in_fd, buffer, sizeof buffer);
				switch (n) {
				case -1: /* ERROR */
					fprintf(stderr, PROGNAME ": read(in_fd=%d): %s (errno = %d)\n",
						in_fd, strerror(errno), errno);
					close(in_fd); in_fd = -1;
					break;
				case 0: /* EOF */
					fprintf(stderr, PROGNAME ": read(in_fd=%d): EOF\n", in_fd);
					close(in_fd); in_fd = -1;
					break;
				default:
					if (flags & FLAG_DEBUG2) {
						fprintf(stderr, " [%d]", n);
					} /* if */
					for (i = 0; i < MAX; i++) {
						if (sd_out[i] >= 0) {
							int res = write(sd_out[i], buffer, n);
							if (res != n) {
								if (flags & FLAG_DEBUG) {
									fprintf(stderr,
										PROGNAME ": connection closed (slot %d, sd_out=%d): res(%d) != n(%d)\n",
										i, sd_out[i], res, n);
								} /* if */
								close(sd_out[i]); sd_out[i] = -1;
								n_out--;
								if ((n_out == 0) && (in_fd > 0)) {
									if (flags & FLAG_DEBUG) {
										fprintf(stderr,
											PROGNAME ": closing input file %s at descriptor %d, as no clients are connected\n",
											in_filename,
											in_fd);
									} /* if */
									close(in_fd); in_fd = -1;
								} /* if */
							} /* if */
						} /* if */
					} /* for */
				} /* switch */
			} /* if */
check_out_sd:
			for (i = 0; i < MAX; i++) {
				if ((sd_out[i] >= 0) && FD_ISSET(sd_out[i], &readset)) {
					char buffer[1024];
					int r = read(sd_out[i], buffer, sizeof buffer);
					switch (r) {
					case -1:
						fprintf(stderr, PROGNAME ": read(sd_out[%d]=%d): %s (errno = %d)\n",
							i, sd_out[i], strerror(errno), errno);
						close(sd_out[i]); sd_out[i] = -1;
						n_out--;
						if ((n_out == 0) && (in_fd > 0)) {
							if (flags & FLAG_DEBUG) {
								fprintf(stderr,
									PROGNAME ": closing input file %s at descriptor %d, as no clients are connected\n",
									in_filename, in_fd);
							} /* if */
							close (in_fd); in_fd = -1;
						} /* if */
						break;
					case 0:
						if (flags & FLAG_DEBUG) {
							fprintf(stderr, PROGNAME ": read(sd_out[%d]=%d): EOF\n", i, sd_out[i]);
						} /* if */
						close(sd_out[i]); sd_out[i] = -1;
						n_out--;
						if ((n_out == 0) && (in_fd > 0)) {
							if (flags & FLAG_DEBUG) {
								fprintf(stderr,
									PROGNAME ": closing input file %s at descriptor %d, as no clients are connected\n",
									in_filename, in_fd);
							} /* if */
							close (in_fd);
							in_fd = -1;
						} /* if */
						break;
					default:
						if (flags & FLAG_DEBUG2) {
							fprintf(stderr, " <sd_out[%d]=%d: n=%d>", i, sd_out[i], r);
						} /* if */
						break;
					} /* switch */
				} /* if */
			} /* for */
			break;
		} /* switch */
	} /* for (;;) */
} /* main */

/* $Id: nmeasrv.c,v 1.3 2012/01/21 16:01:56 luis Exp $ */
