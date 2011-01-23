/* $Id: nmeasrv.c,v 1.2 2011/01/23 13:11:46 luis Exp $
 * Author: Luis Colorado <luis.colorado@hispalinux.es>
 * Date: Sat Jan 22 12:23:02     2011
 * Disclaimer: (C) 2011 LUIS COLORADO SISTEMAS S.L.
 *		All rights reserved.
 * Description: This program binds to a default port (or
 *		or specified on command line, and redirects all
 *		input to the connections that arrive to that port.
 * $Log: nmeasrv.c,v $
 * Revision 1.2  2011/01/23 13:11:46  luis
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
#define MAX			1024

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

extern char *optarg;
extern int optind, opterr, optopt;

#define FLAG_DEBUG		1

int flags = 0;

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
  	fprintf (stderr, "  -d         Debug. Be verbose.\n");
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
	while ((opt = getopt(argc, argv, "i:p:d")) != EOF) {
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
	if (flags & FLAG_DEBUG) {
		fprintf(stderr, "Opening listening socket\n");
	} /* if */
	sd = socket (AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		perror(PROGNAME ": socket");
		exit(EXIT_FAILURE);
	} /* if */
	if (flags & FLAG_DEBUG) {
		fprintf (stderr, "Trying to bind %s:%d\n",
			inet_ntoa(our_addr.sin_addr),
			ntohs (our_addr.sin_port));
	} /* if */
	res = bind(sd, (const struct sockaddr *)&our_addr, sizeof our_addr);
	if (sd == -1) {
		perror(PROGNAME ": bind");
		exit(EXIT_FAILURE);
	} /* if */
	if (flags & FLAG_DEBUG) {
		fprintf(stderr, "Listening on socket\n");
	} /* if */
	res = listen(sd, 5);
	if (sd == -1) {
		perror(PROGNAME ": listen");
		exit(EXIT_FAILURE);
	} /* if */

	/* Construct the FD_SET for the select system call */
	for (;;) {
		fd_set readset;
		int sd_max = 0;

		if ((in_fd < 0) && (n_out > 0)) {
			if (flags & FLAG_DEBUG) {
				fprintf(stderr,
					PROGNAME ": opening %s for input\n",
					in_filename);
			} /* if */
			in_fd = open(in_filename, O_RDONLY);
			if (in_fd < 0) {
				fprintf(stderr, "%s: open: %s(errno = %d)\n",
					PROGNAME,
					strerror(errno),
					errno);
				sleep(1);
				continue;
			} /* if */
		} /* if */

		/* prepare the select call... */
		FD_ZERO(&readset);
		if (in_fd >= 0) {
			sd_max = in_fd;
			FD_SET(in_fd, &readset);
		} /* if */
		FD_SET(sd, &readset);
		if (sd > sd_max) sd_max = sd;

		res = select (sd_max+1, &readset, NULL, NULL, NULL);
		switch (res) {
		case -1: /* error in select */
			perror (PROGNAME ": select");
			exit (EXIT_FAILURE);
		case 0: /* Timeout */
			if (flags & FLAG_DEBUG) {
				fprintf (stderr,
					PROGNAME ": Timeout\n");
			} /* if */
			continue;
		default: /* Data on some direction */
			if (FD_ISSET(sd, &readset)) {
				struct sockaddr_in peer;
				int peer_sz = sizeof peer;
				int new_sd = accept(sd, (struct sockaddr *)&peer, &peer_sz);
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
					continue;
				} /* if */
				n_out++;
				if (flags & FLAG_DEBUG) {
					fprintf(stderr,
						PROGNAME ": New connection from %s:%d (slot %d, sd %d)\n",
						inet_ntoa(peer.sin_addr),
						ntohs(peer.sin_port),
						i, sd_out[i]);
				} /* if */
			} /* if */
			if ((in_fd >= 0) && FD_ISSET(in_fd, &readset)) {
				static char buffer [1024];
				int n;
				int fl = fcntl(in_fd, F_GETFL);
				fcntl(in_fd, F_SETFL, fl | O_NONBLOCK);
				n = read(in_fd, buffer, sizeof buffer);
				if (n <= 0) {
					close(in_fd);
					in_fd = -1;
					if (flags & FLAG_DEBUG) {
						fprintf(stderr,
							PROGNAME ": closing input file %s at descriptor %d.\n",
							in_filename, in_fd);
					} /* if */
					continue;
				} /* if */
				fcntl(in_fd, F_SETFL, fl);
				if (flags & FLAG_DEBUG) {
					fprintf(stderr,
						"[%d]",
						n);
				} /* if */
				for (i = 0; i < MAX; i++) {
					if (sd_out[i] >= 0) {
						int res = write(sd_out[i], buffer, n);
						if (res != n) {
							if (flags & FLAG_DEBUG) {
								fprintf(stderr,
									PROGNAME ": connection closed (slot %d)\n",
									i);
							} /* if */
							close(sd_out[i]);
							n_out--;
							sd_out[i] = -1;
							if ((n_out == 0) && (in_fd != 0)) {
								if (flags & FLAG_DEBUG) {
									fprintf(stderr,
										PROGNAME ": closing input file %s at descriptor %d, as no clients connected\n",
										in_filename,
										in_fd);
								} /* if */
								close(in_fd);
								in_fd = -1;
							} /* if */
						} /* if */
					} /* if */
				} /* for */
			} /* if */
		} /* switch */
	} /* for (;;) */
} /* main */

/* $Id: nmeasrv.c,v 1.2 2011/01/23 13:11:46 luis Exp $ */
