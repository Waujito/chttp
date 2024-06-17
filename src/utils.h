
#ifndef UTILS_H
#define UTILS_H

#include <pthread.h>

/**
 * A dynamic array
 */
struct vector
{
	void **arr;
	size_t size;
	size_t capacity;
	pthread_mutex_t *lock;
};

int initVector(struct vector *vec, size_t startCapacity);
void growVector(struct vector *vec);
size_t insertVector(struct vector *vec, void *el);
inline size_t vectorInsertEl(struct vector *vec, void *el) {
	return insertVector(vec, el);
}
void *vectorGetEl(struct vector *vec, size_t i);
void vectorSetEl(struct vector *vec, size_t i, void *el);


/**
 * Stores program args.
 */
struct args_t {
	char **unixSocks;
	int unixc;

	struct in_addr *TCPAddrs;
	int *TCPPorts;
	int TCPc;
};

/**
 * Parses program args.
 */
int parseArgs(int argc, char *argv[], struct args_t *res);

#endif /* UTILS_H */
