/*
 * udpclient.h
 *
 *  Created on: 26/05/2013
 *      Author: elbillyto
 */

#ifndef UDPCLIENT_H_
#define UDPCLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFSIZE 1024

typedef struct {
    int sockfd, portno;
	int serverlen;
	struct sockaddr_in serveraddr;
	struct hostent *server;
	char *hostname;
	char buf[BUFSIZE];

} client;

int errorclient(char *msg) ;
client* initClient(int argc, char **argv) ;
void freeClient(client *conn) ;
char* trunk_send (client* conn, int entero);

#endif /* UDPCLIENT_H_ */
