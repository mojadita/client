/* $Id: cliente.c,v 1.4 2011/01/23 00:59:39 luis Exp $
 * Author: Luis Colorado <lc@luiscoloradosistemas.com>
 * Date: Thu Feb 26 12:44:15 MET 1998
 * $Log: cliente.c,v $
 * Revision 1.4  2011/01/23 00:59:39  luis
 * * Changed author email in cliente.c
 * * Added comments on closing braces to pair the control sentence.
 * * Added nmeasrv.c as an example of server listening for connections on
 *   a specific port.  Doesn't work well when specifying input file yet.
 *
 * Revision 1.3  2000-07-16 23:58:18  luis
 * Change in the email address of the author.
 *
 * Revision 1.2  1998/04/04 11:45:36  luis
 * Included options to check individually for EOF on connection/local sides
 *
 * Revision 1.1  1998/02/26 19:31:07  luis
 * Initial revision
 *
 */

#define DEFAULT_SERVER "127.0.0.1"
#define DEFAULT_SERVICE "telnet"
#define PROGNAME	"cliente"

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

extern char *optarg;
extern int optind, opterr, optopt;

#define FLAG_DEBUG	1
#define FLAG_EOFSTDIN	2
#define FLAG_EOFSOCKET	4
#define FLAG_EOFALL	6
#define FLAG_OPTEOFLOCL	8
#define FLAG_OPTEOFCONN 16

int flags = 0;

void do_usage (void);
int process (int fd_in, int fd_out);

int main (int argc, char **argv)
{
	int opt, sd, res;
	struct sockaddr_in server;
	struct servent *service, fallback;
	struct hostent *host;
	char *servername = DEFAULT_SERVER;
	char *serverport = DEFAULT_SERVICE;
	static struct timeval timeout = { 0, 0 }, *t;

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
	service = getservbyname (serverport, "tcp");
	if (!service)
		service = getservbyport(atoi(serverport), "tcp");
	if (!service) {
		service = &fallback;
		fallback.s_port = htons(atoi(serverport));
	} /* if */
	host = gethostbyname (servername);
	if (!host) {
		fprintf (stderr,
			PROGNAME ": error: %s does not exist\n",
			servername);
		exit (EXIT_FAILURE);
	} /* if */

	/* Construct the sockaddr_in for the connect system call */
	server.sin_family = AF_INET;
	server.sin_port = service->s_port;
	server.sin_addr = *(struct in_addr *)(host->h_addr_list[0]);

	if (flags & FLAG_DEBUG) {
		fprintf (stderr, "Trying %s:%d\n",
			inet_ntoa(server.sin_addr),
			ntohs (server.sin_port));
	} /* if */

	/* Connect to the server */
	sd = socket (AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		perror (PROGNAME ": socket");
	} /* if */
	res = connect (sd, (struct sockaddr *)&server, sizeof server);
	if (res == -1) {
		perror (PROGNAME ": connect");
	} /* if */

	/* Construct the FD_SET for the select system call */
	for (;;) {
		fd_set readset;

		/* Check for EOF on both paths */
		if ((flags & FLAG_EOFALL) == FLAG_EOFALL) {
			break;
		} /* if */

		/* prepare the select call... */
		FD_ZERO(&readset);
		if (!(flags & FLAG_EOFSTDIN))
			FD_SET(0, &readset);
		if (!(flags & FLAG_EOFSOCKET))
			FD_SET(sd, &readset);

		res = select (sd+1, &readset, NULL, NULL, t);
		switch (res) {
		case -1: /* error in select */
			perror (PROGNAME ": select");
			exit (EXIT_FAILURE);
		case 0: /* Timeout */
			if (flags & FLAG_DEBUG) {
				fprintf (stderr,
					PROGNAME ": Timeout\n");
				exit (EXIT_SUCCESS);
			} /* if */
			break;
		default: /* Data on some direction */
			if (FD_ISSET(0, &readset)) {
				if (process(0, sd)) {
					flags |= FLAG_EOFSTDIN;
					if (flags & FLAG_DEBUG) {
						fprintf (stderr,
							PROGNAME
							": stdin EOF\n");
					} /* if */
					if (flags & FLAG_OPTEOFLOCL) {
					  if (flags & FLAG_DEBUG) {
					    fprintf (stderr,
					      PROGNAME
					      ": EOF stdin -> EXIT\n");
					  } /* if */
					  exit (EXIT_SUCCESS);
					} /* if */
				} /* if */
			} /* if */
			if (FD_ISSET(sd, &readset)) {
				if (process(sd, 1)) {
					flags |= FLAG_EOFSOCKET;
					if (flags & FLAG_DEBUG) {
						fprintf (stderr,
							PROGNAME
							": socket EOF\n");
					} /* if */
					if (flags & FLAG_OPTEOFCONN) {
					  if (flags & FLAG_DEBUG) {
					    fprintf (stderr,
					      PROGNAME
					      ": EOF socket -> EXIT\n");
					  } /* if */
					  exit (EXIT_SUCCESS);
					} /* if */
				} /* if */
			} /* if */
		} /* switch */
	} /* for (;;) */
} /* main */

int process (int fd_in, int fd_out)
{
	char buffer[BUFSIZ], *p = buffer;
	int res, tam;

	/* let's read the data into the buffer in chunks of BUFSIZ. */
	res = read (fd_in, buffer, sizeof buffer);

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

void do_usage ()
{
	fprintf (stderr, "Usage: " PROGNAME " [ options ...]\n");
	fprintf (stderr, "Options:\n");
  	fprintf (stderr, "  -h server  Specifies a host to contact.\n");
  	fprintf (stderr, "  -p service Specifies the port to connect to.\n");
  	fprintf (stderr, "  -d         Debug. Be verbose.\n");
  	fprintf (stderr, "  -t timeout Set a timeout in secs. (def. no timeout).\n");
	fprintf (stderr, "  -l         EOF on local side forces exit\n");
	fprintf (stderr, "  -c         EOF on connection side forces exit\n");
	exit (0);
} /* do_usage */

/* $Id: cliente.c,v 1.4 2011/01/23 00:59:39 luis Exp $ */
