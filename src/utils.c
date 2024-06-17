#include "utils.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <netinet/in.h>

int initVector(struct vector *vec, size_t startCapacity) {
	memset(vec, 0, sizeof(struct vector));
	vec->capacity = startCapacity;

	vec->arr = malloc(sizeof(void *) * startCapacity);
	vec->lock = malloc(sizeof(pthread_mutex_t));
	

	if (vec->arr == NULL || vec->lock == NULL || pthread_mutex_init(vec->lock, NULL)) {
		return -1;
	}


	return 0;
}
void growVector(struct vector *vec) {
	vec->capacity *= 2;
	void **tmp = realloc(vec->arr, sizeof(void *) * vec->capacity);

	if (tmp == NULL) {
		free(vec->arr);
		raise(SIGTERM);
		return;
	}

	vec->arr = tmp;
}
size_t insertVector(struct vector *vec, void *el) {
	pthread_mutex_lock(vec->lock);

	if (vec->size == vec->capacity) {
		growVector(vec);
	}

	vec->arr[vec->size] = el;
	size_t i = vec->size++;

	pthread_mutex_unlock(vec->lock);

	return i;
}
void *vectorGetEl(struct vector *vec, size_t i) {
	pthread_mutex_lock(vec->lock);

	void *el = vec->arr[i];

	pthread_mutex_unlock(vec->lock);

	return el; 
}

void vectorSetEl(struct vector *vec, size_t i, void *el) {
	pthread_mutex_lock(vec->lock);

	void **elp = vec->arr + i;
	*elp = el;

	pthread_mutex_unlock(vec->lock);
}


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
