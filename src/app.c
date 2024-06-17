#include "server.h"
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void connhandler(FILE *stream)
{
	printf("Got new connection on file %d\n", fileno(stream));

	char *line = NULL;
	size_t lineLen = 0;

	while(getline(&line, &lineLen, stream) != -1) {
		if (!strcmp(line, "ping\n")) {
			fprintf(stream, "pong\n");
		} else {
			fprintf(stream, "Invalid command\n");
		}
	}

closeConn:
	free(line);
	return;
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
			fprintf(stderr, "Unable to set up a unix socket %s: %s", args.unixSocks[i], strerror(errno));
			return 1;
		}
	}

	for (int i = 0; i < args.TCPc; i++) {
		struct ssock *sock = malloc(sizeof(struct ssock));
		if (bindTCPSocket(sock, args.TCPPorts[i], args.TCPAddrs[i]) || contextRegisterSocket(*sock)) {
			fprintf(stderr, "Unable to set up a tcp socket %ud:%d: %s", 
	  			args.TCPAddrs[i].s_addr, args.TCPPorts[i], strerror(errno));
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
