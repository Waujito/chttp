#include "server.h"
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <search.h>
#include <unistd.h>
#include "http.h"


void httpRequestProcessor(struct HTTPRequest *request, struct HTTPResponse *response) 
{
	response->status = 200;
	addKVHTTPHeader_p(&response->headers, "Server", "chttp");
	response->body = "Hello there\r\n";
	response->bodyc = strlen(response->body);
}

struct HTTPConnectionHandlerArgs httpConnhandlerArgs = {
	.httpRequestProcessor = httpRequestProcessor,
};

int main(int argc, const char *argv[]) 
{
	initApplication();

	struct ApplicationContext serverContext;
	registerApplication(&serverContext);


	if (initContext(&serverContext, httpConnetionHandler, &httpConnhandlerArgs) || initSighandler()) {
		perror("Unable to initialize context");
		return 1;
	}

	struct args_t args;	
	if (parseArgs(argc, argv, &args)) {
		return 1;
	}

	for (int i = 0; i < args.unixc; i++) {
		struct ssock *sock = malloc(sizeof(struct ssock));
		if (bindUnixSocket(sock, args.unixSocks[i]) || contextRegisterSocket(&serverContext, *sock)) {
			fprintf(stderr, "Unable to set up a unix socket %s: %s\n", args.unixSocks[i], strerror(errno));
			closeApplication(0);
			return 1;
		}
	}

	for (int i = 0; i < args.TCPc; i++) {
		struct ssock *sock = malloc(sizeof(struct ssock));
		if (bindTCPSocket(sock, args.TCPPorts[i], args.TCPAddrs[i]) || contextRegisterSocket(&serverContext, *sock)) {
			fprintf(stderr, "Unable to set up a tcp socket %ud:%d: %s\n", 
	  			args.TCPAddrs[i].s_addr, args.TCPPorts[i], strerror(errno));
			closeApplication(0);
			return 1;
		}
	}

	if (startServer(&serverContext)) {
		perror("Unable to start up server");
		return 1;
	}

	closeServer((struct ClosingContext) {.context = &serverContext, .sig = 1ULL});

	return 0;
}
