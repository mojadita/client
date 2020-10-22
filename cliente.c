/* $Id: cliente.c,v 1.6 2012/01/21 18:14:31 luis Exp $
 * Author: Luis Colorado <lc@luiscoloradosistemas.com>
 * Date: Thu Feb 26 12:44:15 MET 1998
 * Copyright (c) 1992-2020 by LUIS COLORADO.  All rights reserved.
 * License: BSD
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
#include <pthread.h>

#include "main.h"
#include "usage.h"
#include "process.h"

#define DEFAULT_SERVER      "127.0.0.1"
#define DEFAULT_SERVICE     "telnet"

#define chr &chrono_main

int flags = 0;

int main (int argc, char **argv)
{
    int opt;
    struct sockaddr_in server;
    struct servent *service, fallback;
    struct hostent *host;
    char *servername = DEFAULT_SERVER;
    char *serverport = DEFAULT_SERVICE;
    struct process *p;
    int to = 0;
    FILE *logger = stderr;
    struct chrono chrono_main;

    char *prog_name = strrchr(argv[0], '/');
    if (prog_name) {
        prog_name++;
    } else {
        prog_name = argv[0];
    }

    startTs(&chrono_main);

    /* process the program options... */
    while ((opt = getopt(argc, argv, "h:p:dt:l:")) != EOF) {
        switch (opt) {
        case 'p':  serverport = optarg; break;
        case 'h':  servername = optarg; break;
        case 'd':  flags |= FLAG_DEBUG; break;
        case 't':  to = abs(atoi(optarg)); break;
        case 'l':  logger = fopen(optarg, "wt");
                    if (!logger) {
                        logger = stderr;
                        ERR(EXIT_FAILURE,
                            "%s: %s (ERR %d)\n",
                            optarg,
                            strerror(errno), errno);
                    }
                    setbuf(logger, NULL);
                    break;
        default: do_usage(argv[0]); break;
        } /* switch */
    } /* while */

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
        INFO("Trying %s:%d\n",
            inet_ntoa(server.sin_addr),
            ntohs(server.sin_port));
    } /* if */

    /* Connect to the server */
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        ERR(EXIT_FAILURE, "socket: %s (ERR %d)\n",
            strerror(errno), errno);
    } /* if */

    int res = connect(sd,
            (struct sockaddr *)&server, sizeof server);
    if (res < 0) {
        ERR(EXIT_FAILURE, "connect: %s:%d %s (ERR %d)\n",
            inet_ntoa(server.sin_addr), ntohs(server.sin_port),
            strerror(errno), errno);
    } /* if */

    if (flags & FLAG_DEBUG) {
        INFO("Connected!\n");
    } /* if */

    struct process proc_sender;
    proc_sender.fd_from = 0; /* STDIN */
    proc_sender.fd_to   = sd;
    proc_sender.logger  = logger;
    proc_sender.at_eof  = shut_socket;
    proc_sender.offset  = 0;
    proc_sender.from    = "STDIN";
    proc_sender.to      = "SERVER";
    proc_sender.messg   = "\033[1;32mSTDIN >>> SERVER\033[m";
    proc_sender.close_messg = "shutdown server";
    proc_sender.buffer  = malloc(BUFFER_SIZE);
    struct process proc_receiver;
    proc_receiver.fd_from = sd;
    proc_receiver.fd_to   = 1; /* STDOUT */
    proc_receiver.logger  = logger;
    proc_receiver.at_eof  = NULL;
    proc_receiver.offset  = 0;
    proc_receiver.from    = "SERVER";
    proc_receiver.to      = "STDOUT";
    proc_receiver.messg   = "\033[1;33mSTDOUT <<< SERVER\033[m";
    proc_receiver.close_messg = "terminating";
    proc_receiver.buffer  = malloc(BUFFER_SIZE);

    pthread_t sender, receiver;
    res = pthread_create(&sender, NULL, process, &proc_sender);
    if (res < 0) {
        ERR(EXIT_FAILURE,
            "pthread_create: %s (ERR %d)\n",
            strerror(errno), errno);
    }
    res = pthread_create(&receiver, NULL, process, &proc_receiver);
    if (res < 0) {
        ERR(EXIT_FAILURE,
            "pthread_create: %s (ERR %d)\n",
            strerror(errno), errno);
    }

    pthread_join(sender, NULL);
    pthread_join(receiver, NULL);

    INFO("program ended\n");

} /* main */

/* $Id: cliente.c,v 1.6 2012/01/21 18:14:31 luis Exp $ */
