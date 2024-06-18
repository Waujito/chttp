#define _GNU_SOURCE
#include "server.h"
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <search.h>
#include "http.h"

#define HTTPHEAD_PROCESSING 0
#define HTTPHEADERS_PROCESSING 1
#define HTTPBODY_PROCESSING 2
#define HTTPPROCESSING_END 100

void connhandler(FILE *stream)
{
	printf("Got new connection on file %d\n", fileno(stream));

	struct httpHead head;
	struct hsearch_data headers;
	char *body;
	size_t bodyc;

	// hcreate_r(1000, &headers);

	char *line = NULL;
	size_t nlineLen = 0;

	int processing_state = HTTPHEAD_PROCESSING;  
	while(getline(&line, &nlineLen, stream) != -1) {
		printf("%s", line);

		if (processing_state == HTTPHEAD_PROCESSING) {
			if (parseHTTPHead(line, &head)) {
				printf("Unable to parse head\n");
				goto closeConn;
			} else {
				processing_state++;
			}
		} else if (processing_state == HTTPHEADERS_PROCESSING) {
			printf("Process headers\n");

			if (!strcmp(line, "\r\n") || !strcmp(line, "\n"))
				processing_state++;
		} 
		if (processing_state == HTTPBODY_PROCESSING) {
			printf("Process body\n");
			bodyc = 0;
			body = NULL;
			processing_state = HTTPPROCESSING_END;
		} 

		if (processing_state == HTTPPROCESSING_END) {
			printf("Process ended\n");
			printf("Connection handled!\n");

			fprintf(stream, "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/plain\r\nContent-Length:10\r\n\r\nabcdefghsdgflkjds;lfkgjs;ldfkgj\r\n");
			processing_state = HTTPHEAD_PROCESSING;

			goto closeConn;
		}
	}

closeConn:
	free(line);
}

int main(int argc, char *argv[]) 
{
	if (initContext(connhandler) || initSighandler()) {
		perror("Unable to initialize context");
		return 1;
	}

	struct args_t args;	
	if (parseArgs(argc, argv, &args)) {
		return 1;
	}

	for (int i = 0; i < args.unixc; i++) {
		struct ssock *sock = malloc(sizeof(struct ssock));
		if (bindUnixSocket(sock, args.unixSocks[i]) || contextRegisterSocket(*sock)) {
			fprintf(stderr, "Unable to set up a unix socket %s: %s\n", args.unixSocks[i], strerror(errno));
			closeServer(1);
			return 1;
		}
	}

	for (int i = 0; i < args.TCPc; i++) {
		struct ssock *sock = malloc(sizeof(struct ssock));
		if (bindTCPSocket(sock, args.TCPPorts[i], args.TCPAddrs[i]) || contextRegisterSocket(*sock)) {
			fprintf(stderr, "Unable to set up a tcp socket %ud:%d: %s\n", 
	  			args.TCPAddrs[i].s_addr, args.TCPPorts[i], strerror(errno));
			closeServer(1);
			return 1;
		}
	}

	if (startServer()) {
		perror("Unable to start up server");
		return 1;
	}

	closeServer(0);

	return 0;
}
