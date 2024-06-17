#include "server.h"
#include <err.h>
#include <stdlib.h>

int main(int argc, char *argv[]) 
{
	if (initContext() || initSighandler()) {
		perror("Unable to initialize context");
		return 1;
	}


	if (argc < 2) {
		if (argc == 0) {
			errx(EXIT_FAILURE, "Invalid arguments. Accepted format: </path/to/socket>+");
		} else {
			errx(EXIT_FAILURE, "Invalid arguments. Accepted format: %s </path/to/socket>+", argv[0]);
		}
	}

	char *sockaddr = argv[1];

	struct ssock_t *sock = malloc(sizeof(struct ssock_t));
	if (bindSocket(sock, sockaddr) || contextRegisterSocket(*sock)) {
		perror("Unable to set up a socket");
		return 1;
	}

	if (startServer()) {
		perror("Unable to start up server");
		return 1;
	}

	closeServer(0);

	return 0;
}
