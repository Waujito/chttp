#include "server.h"
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <search.h>
#include <unistd.h>
#include "http.h"


#define HTTPHEAD_PROCESSING 0
#define HTTPHEADERS_PROCESSING 1
#define HTTPBODY_PROCESSING 2
#define HTTPPROCESSING_END 100

void connhandler(FILE *stream)
{
	printf("Got new connection on file %d\n", fileno(stream));

	struct HTTPRequest req;
	while (!feof(stream)) {
		printf("Got new request on stream %d\n", fileno(stream));
		int status = parseHTTPRequest(stream, &req);

		if (status == HTTPREQ_FAILED) {
			destroyHTTPRequest(&req);

			printf("Cannot parse request\n");
			goto closeHandler;
	
		} else if (status == HTTPREQ_EOF) {
			destroyHTTPRequest(&req);
			goto closeHandler;
		}

		for (size_t i = 0; i < req.headers.size; i++) {
			struct HTTPHeader header;
			vectorCopyEl_p(&req.headers, i, (char *)&header);
			printf("%s: %s\n", header.key, header.value);
		}

		printf("%s\n", req.body);

		destroyHTTPRequest(&req);
		fprintf(stream, "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nContent-Type: text/plain\r\nContent-Length:10\r\n\r\nasdfasdf\r\n");
		if (req.httpver == HTTPV_10) break;
		else if (req.httpver == HTTPV_11) continue;
		else break;
	}


closeHandler:
	printf("Conn closed %d\n", fileno(stream));
}



int main(int argc, const char *argv[]) 
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
