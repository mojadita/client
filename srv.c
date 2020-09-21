/* $Id: srv.c,v 1.6 2012/01/21 18:14:31 luis Exp $
 * Author: Luis Colorado <lc@luiscoloradosistemas.com>
 * Date: Thu Feb 26 12:44:15 MET 1998
 * Copyright: (C) 1992-2020 Luis Colorado.  All rights reserved.
 * License: BSD.
 */

#define PROGNAME	"srv"

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

extern char *optarg;
extern int optind, opterr, optopt;

int bind_port = 0;
int listen_val = 0;
int num_clients = 0;

int main (int argc, char **argv)
{
	int opt, sd, res;

	/* process the program options... */
	while ((opt = getopt(argc, argv, "b:l:n:")) != EOF) {
		switch (opt) {
        case 'b': bind_port = atol(optarg); break;
        case 'l': listen_val = atol(optarg); break;
        case 'n': num_clients = atol(optarg); break;
		} /* switch */
	} /* while */

	/* Construct the sockaddr_in for the connect system call */
    for(;;) {
        sd = socket (AF_INET, SOCK_STREAM, 0);
        if (sd < 0) {
            perror (PROGNAME ": socket");
        } /* if */
        if (bind_port) {
            struct sockaddr_in srv_name;
            srv_name.sin_family = AF_INET;
            srv_name.sin_port = htons(bind_port);
            srv_name.sin_addr.s_addr = INADDR_ANY;
            int res = bind(sd,
                    (const struct sockaddr *)&srv_name,
                    sizeof srv_name);
            if (res < 0) {
                perror(PROGNAME ": bind");
                exit(1);
            }
            printf(PROGNAME ": bound to port %d\n", bind_port);
        }
        if (listen_val >= 0) {
            int res = listen(sd, listen_val);
            if (res < 0) {
                perror(PROGNAME ": listen");
                exit(1);
            }
            printf(PROGNAME ": listen set to %d\n", listen_val);
        }
        {
            struct sockaddr_in server;
            socklen_t server_sz = sizeof server;
            int res = getsockname(sd,
                    (struct sockaddr *) &server,
                    &server_sz);
            bind_port = ntohs(server.sin_port);
            printf("bound to port %d\n", bind_port);
        }

        struct sockaddr_in client;
        socklen_t client_sz = sizeof client;
        int cd = accept(sd,
                (struct sockaddr *)&client,
                &client_sz);
        if (cd == -1) {
            perror (PROGNAME ": accept");
            break;
        } /* if */

        /*************************************************
         * THIS IS THE TRICK... JUST CLOSE THE ACCEPTING
         * SOCKET AND THE SERVER WILL REFUSE INCOMING
         * CONNECTIONS FROM THEN ON.
         *************************************************/
        close(sd);

        printf("Accepted connection from %s:%d\n",
                inet_ntoa(client.sin_addr),
                ntohs(client.sin_port));
        char *greeting = PROGNAME": This is the echo "
                "server ready for input\r\n";
        write(cd, greeting, strlen(greeting));

        char buffer[1024];
        ssize_t n;

        while ((n = read(cd, buffer, sizeof buffer)) > 0) {
            printf("Recibido: [%.*s]\n", (int)n, buffer);
            write(cd, buffer, n);
        } /* while */
        if(n < 0) {
            perror(PROGNAME ": read");
        } else { /* == 0 */
            printf("Recibido: EOF\n");
            char *greeting = "\r\n" PROGNAME ": Have a nice day :)\r\n";
            write(cd, greeting, strlen(greeting));
        }
        close(cd);
    } /* for (;;) */

} /* main */
