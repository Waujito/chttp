#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

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
