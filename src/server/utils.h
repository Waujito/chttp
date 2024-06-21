
#ifndef UTILS_H
#define UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

/**
 * A dynamic array
 */
struct vector
{
	void **arr;
	size_t size;
	size_t capacity;
	pthread_mutex_t lock;
};

int initVector(struct vector *vec, size_t startCapacity);
void growVector(struct vector *vec);
size_t insertVector(struct vector *vec, void *el);
#define vectorInsertEl insertVector 

void *vectorGetEl(struct vector *vec, size_t i);
void vectorSetEl(struct vector *vec, size_t i, void *el);
void vectorDestroy(struct vector *vec);

/**
 * A dynamic array of primitive types (non-pointers, fixed size).
 * Designed to reduce cache misses.
 */
struct vector_p
{
	char *arr;
	/* Size of each element */
	size_t elsz;

	/* Count of elements in vector*/
	size_t size;
	size_t capacity;
	pthread_mutex_t lock;
};

/**
 * Initializes a vector of primitives.
 *
 * @vec pointer to vector to be initialized
 * @elsz size of each element 
 * @startCapacity Starting capacity
 *
 * @Returns 0 if vector created sucessfully, -1 otherwise.
 */
int initVector_p(struct vector_p *vec, size_t elsz, size_t startCapacity);
void growVector_p(struct vector_p *vec);

/**
 * Points to start of element of index i in vec->arr.
 */
char *vectorElPtr_p(struct vector_p *vec, size_t i);

size_t insertVector_p(struct vector_p *vec, char *el);
#define vectorInsertEl_p insertVector_p

/**
 * Notice. This function is semi-thread-safe. It returns pointer to character array inside the vector.
 * But this may lead to errors when multiple threads are inserting elements and this may lead to unexpected realloc call.
 * Which may cause segmentation faults. Use this function in one thread and make sure to not utilize returned pointer.
 * Use vectorCopyEl_p when not sure about safety.
 */
char *vectorGetEl_p(struct vector_p *vec, size_t i);
/**
 * Copies bytes of element of vector on position i to buffer. Basically same as get element, but thread-safe.
 *
 * @buf Bytes destination
 */
void vectorCopyEl_p(struct vector_p *vec, size_t i, char *buf);

void vectorSetEl_p(struct vector_p *vec, size_t i, char *el);
void vectorDestroy_p(struct vector_p *vec);


/**
 * Stores program args.
 */
struct args_t {
	const char **unixSocks;
	int unixc;

	struct in_addr *TCPAddrs;
	int *TCPPorts;
	int TCPc;
};

/**
 * Parses program args.
 */
int parseArgs(int argc, const char *argv[], struct args_t *res);
/**
 * Destroys program args storage.
 */
void destroyArgs(struct args_t *args);


#ifdef __cplusplus
}
#endif

#endif /* UTILS_H */
