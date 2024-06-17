#include "server.h"
#include <err.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

struct args_t {
	char **unixSocks;
	int unixc;

	struct in_addr *TCPAddrs;
	int *TCPPorts;
	int TCPc;
};

int parseArgs(int argc, char *argv[], struct args_t *res)
{
	memset(res, 0, sizeof(*res));

	if (argc < 2) 
		goto error;

	int unixc = 0;
	int TCPc = 0;

	for (int i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-U"))
			unixc++;
		else if (!strcmp(argv[i], "-T"))
			TCPc++;
	}

	char **unixSocks = malloc(sizeof(char *) * unixc);
	struct in_addr *TCPAddrs = malloc(sizeof(struct in_addr) * TCPc);
	int *TCPPorts = malloc(sizeof(int *) * TCPc);

	char inType = '\0';
	char inSched = 0;

	int usi = 0;
	int tci = 0;
	for (int i = 1; i < argc; i++) {
		char *data = argv[i];
		if (inSched) {
			if (inType == 'U') {
				unixSocks[usi++] = data;
			} else if (inType == 'T') {
				// TCPAddr size
				int asz = 0;

				// Pointer to pos where ip port string starts.
				char *portp;

				int datalen = strlen(data);

				int colons = 0;
				for (int j = 0; j < datalen; j++) {
					if (data[j] == ':') {
						if (colons++) goto error;

						asz = j;
						portp = data + j + 1;
					}
				}

				if (colons == 0 || portp >= data + datalen) goto error;

				char *addrData = malloc(sizeof(char) * (asz + 1));
				strncpy(addrData, data, asz);
				addrData[asz] = '\0';

				char *end;
				long port = strtoll(portp, &end, 10);

				struct in_addr iaddr;

				if (end - portp != datalen - asz - 1 || end == portp || !inet_aton(addrData, &iaddr))
      					goto error;

				TCPPorts[tci] = port;
				TCPAddrs[tci++] = iaddr;
			} else {
      				goto error;
      			}

			inSched = 0;
		} else {
			if (!strcmp(data, "-U")) {
				inType = 'U';
				inSched = 1;
			} else if (!strcmp(data, "-T")) {
				inType = 'T';
				inSched = 1;
			} else {
				goto error;
			}
		}
	}


	res->TCPAddrs = TCPAddrs;
	res->TCPPorts = TCPPorts;
	res->TCPc = TCPc;

	res->unixSocks = unixSocks;
	res->unixc = unixc;

	printf("%d %d\n", unixc, TCPc);

	return 0;

error:
	if (argc == 0) {
		fprintf(stderr, "Invalid arguments. Accepted format: [-U </path/to/socket>...] [-T ip_addr:port...]\n");
	} else {
		fprintf(stderr, "Invalid arguments. Accepted format: %s [-U </path/to/socket>...] [-T ip_addr:port...]\n",
			argv[0]);
	}

	return -1;
}

int main(int argc, char *argv[]) 
{
	if (initContext() || initSighandler()) {
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
