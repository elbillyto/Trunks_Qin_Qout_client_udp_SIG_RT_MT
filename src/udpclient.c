/* 
 * udpclient.c - A simple UDP client
 * usage: initClient( <host> <port>)
 * 			   send (client* conn, data)
 */
#include "udpclient.h"



/* 
 * error - wrapper for perror
 */
int errorclient(char *msg) {
	perror(msg);
	return EXIT_FAILURE;
}

/**
 * Init client
 */
client* initClient(int argc, char** argv) {

    client *conn;


    conn = (client *)malloc (sizeof (client));
    if (conn == NULL) return (NULL);


	conn->hostname = argv[0];
	conn->portno = atoi(argv[1]);

	/* socket: create the socket */
	conn->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (conn->sockfd < 0)
		errorclient("ERROR opening socket");

	/* gethostbyname: get the server's DNS entry */
	conn->server = gethostbyname(conn->hostname );
	if (conn->server == NULL) {
		fprintf(stderr, "ERROR, no such host as %s\n", conn->hostname );
		exit(0);
	}

	/* build the server's Internet address */
	bzero((char *) &conn->serveraddr, sizeof(struct sockaddr_in ));
	conn->serveraddr.sin_family = AF_INET;
	bcopy((char *) conn->server->h_addr, (char *) &conn->serveraddr.sin_addr.s_addr,
			conn->server->h_length);
	conn->serveraddr.sin_port = htons(conn->portno );

	/* get a message from the user */
	bzero(conn->buf, BUFSIZE);


    return (conn);
}
/*
 * free client
 */
void freeClient(client *conn) {

	free(conn);

}

char* trunk_send (client* conn, int _int) {

		int n;
		//itoa(...) non-ANSI-C standard !
		sprintf(conn->buf,"%d", _int);
		/* send the message to the server */
		conn->serverlen = sizeof(struct sockaddr_in );
		n = sendto(conn->sockfd, conn->buf, strlen(conn->buf), 0,  (__CONST_SOCKADDR_ARG) &conn->serveraddr, conn->serverlen);
		if (n < 0){
			errorclient("ERROR in sendto");

		}
		/* print the server's reply */
		n = recvfrom(conn->sockfd, conn->buf, strlen(conn->buf), 0, (__SOCKADDR_ARG) &conn->serveraddr, (socklen_t *)&conn->serverlen);
		if (n < 0)
			errorclient("ERROR in recvfrom");
		printf(">>> Echo from Device: %s", conn->buf);

	return conn->buf;
}
