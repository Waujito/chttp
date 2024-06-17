#define _GNU_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/io.h>
#include <errno.h>
#include <err.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include "utils.h"

#define LISTEN_BACKLOG 50


struct ssock_t {
	int fd;

	struct sockaddr *addr;
	socklen_t addrlen;
};

struct connData {
	pthread_t *connThread;
	FILE *connStream;
	pthread_mutex_t *lock;
};

int bindSocket(struct ssock_t *res, char *sockpath)
{
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);

	if (fd == -1) {
		int ecode = errno;
		perror("Unable to create socket");
		errno = ecode;
		goto error;
	}

	struct sockaddr_un *addr = calloc(sizeof(struct sockaddr_un), sizeof(char));

	addr->sun_family = AF_UNIX;

	// Typically length is 108. Minus one because of null terminator.
	int sockpathLen = sizeof(addr->sun_path) - 1;
	int userpathLen = strlen(sockpath);

	if (userpathLen > sockpathLen) {
		errno = EINVAL;
		fprintf(stderr, "The socket path is too large. Max size is %d, %d given", sockpathLen, userpathLen);
		errno = EINVAL;

		goto error;
	}


	strcpy(addr->sun_path, sockpath);

	if (bind(fd, (struct sockaddr *)addr, sizeof(*addr))) {
		int err = errno;
		perror("Unable to bind socket to the given address");
		errno = err;

		goto error;
	}

	errno = 0;

	memset(res, 0, sizeof(struct ssock_t));
	res->fd = fd;
	res->addr = (struct sockaddr *)addr;
	res->addrlen = sizeof(*addr);

	return 0;
error:
	return -1;
}

struct {
	/**
	* Vector of struct ssock_t
	*/
	struct vector socks;	

	/**
	* Vector of struct pthread_t
	*/
	struct vector socksThreads;

	/**
	* Vector of struct connData
	*/
	struct vector conns;
} context;

pthread_mutex_t closeLock;

void closeSocks(int sig)
{
	printf("%d\n", gettid());
	pthread_mutex_lock(&closeLock);

	int err = 0;

	printf("Closing... %zu %zu %zu\n", context.socksThreads.size, context.socks.size, context.conns.size);

	for (size_t i = 0; i < context.socksThreads.size; i++) {
		pthread_t **scpt = (pthread_t **) context.socksThreads.arr + i;
		pthread_t *sockThread = *scpt;
		if (sockThread == NULL) continue;

		pthread_cancel(*sockThread);

		free(sockThread);
		*scpt = NULL;
	}

	printf("Closed listener %zu\n", context.socks.size);

	for (size_t i = 0; i < context.socks.size; i++) {
		struct ssock_t **sockp = (struct ssock_t **)context.socks.arr + i;
		struct ssock_t *sock = *sockp;

		printf("Closing sock1\n");
		if (sock == NULL) continue;

		printf("Closing sock\n");
		close(sock->fd);
		printf("Closed sock\n");
		if (unlink(sock->addr->sa_data)) {
			err = errno;
			fprintf(stderr, "Unable to unbind socket %s: %s\n", sock->addr->sa_data, strerror(errno));
		}

		free(sock->addr);
		free(sock);
		*sockp = NULL;
	}

	printf("Closed sock and listener\n");

	for (size_t i = 0; i < context.conns.size; i++) {
		struct connData *cd = vectorGetEl(&context.conns, i);

		if (cd == NULL) continue;
		struct connData ccd = *cd;

		if (ccd.lock == NULL || pthread_mutex_lock(ccd.lock))
			continue;

		if (vectorGetEl(&context.conns, i) == NULL) continue;
		cd->lock = NULL;

		pthread_mutex_t *lock = ccd.lock;

		pthread_t *connThread = ccd.connThread;
		pthread_cancel(*connThread);
		free(ccd.connThread);

		printf("Closing %d\n", fileno(ccd.connStream));
		fclose(ccd.connStream);

		free(cd);
		vectorSetEl(&context.conns, i, NULL);

		pthread_mutex_unlock(lock);
		pthread_mutex_destroy(lock);
	}

	free(context.socksThreads.arr);
	free(context.socks.arr);
	free(context.conns.arr);

	printf("Deleted connection threads\n");

	if (errno == 0 && sig == SIGINT) {
		printf("Interrupted!\n");
		exit(EXIT_SUCCESS);
	} else {
		if (errno != 0)
			printf("Something went wrong while closing app: %s\n", strerror(errno));
		else 
			printf("Internal failure!\n");

		exit(EXIT_FAILURE);
	}

	pthread_mutex_unlock(&closeLock);
}

void *handlerThread(void * rawsig) {
	int *sig = (int *)rawsig;

	closeSocks(*sig);

	return NULL;
}

void handlerCallback(int sig) {
	int *sigp = malloc(sizeof(int));
	*sigp = sig;

	pthread_t thr;

	if (pthread_create(&thr, NULL, handlerThread, sigp)) {
		goto error;
	}

	pthread_join(thr, NULL);
error:
	perror("Unable to handle cancel gracefully");
	return closeSocks(sig);
}

void *connListener(void *cip)
{
	if (cip == NULL) return NULL;
	int ci = *(int *)cip;
	printf("%d", ci);
	free(cip);

	struct connData *conn = vectorGetEl(&context.conns, ci);
	if (conn == NULL) return NULL;

	FILE *stream = conn->connStream;

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
	vectorSetEl(&context.conns, ci, NULL);

	pthread_mutex_t *lock = conn->lock;
	if (lock == NULL) return NULL;

	pthread_mutex_lock(lock);

	fclose(stream);
	conn->connStream = NULL;
	free(conn->connThread);
	conn->lock = NULL;
	free(conn);

	pthread_mutex_unlock(lock);
	pthread_mutex_destroy(lock);
	free(lock);

	return NULL;
}

void *serverListener(void *rawsock)
{

	struct ssock_t *sock = (struct ssock_t *)rawsock;

	if (listen(sock->fd, LISTEN_BACKLOG)) {
		perror("Unable to listen socket");
		goto error;
	}


	pthread_attr_t thattr;
	if (	pthread_attr_init(&thattr) || 
		pthread_attr_setdetachstate(&thattr, PTHREAD_CREATE_DETACHED)) {

		perror("Unable to init attrs");
		goto error;
	}


	while (1) {
		int nfd = accept(sock->fd, sock->addr, &sock->addrlen);

		if (nfd == -1)
			goto connError;

		FILE *rwstream = fdopen(nfd, "r+");
		if (rwstream == NULL)
			goto connError;
		

		pthread_t *cthread = malloc(sizeof(pthread_t));
		pthread_mutex_t *connLock = malloc(sizeof(pthread_mutex_t));
		if (pthread_mutex_init(connLock, NULL))
			goto connError;

		struct connData *conn = calloc(sizeof(struct connData), sizeof(char));
		conn->connThread = cthread;
		conn->connStream = rwstream;
		conn->lock = connLock;

		size_t ci = insertVector(&context.conns, conn);
		size_t *cip = malloc(sizeof(size_t));
		*cip = ci;

		if (pthread_create(cthread, &thattr, connListener, cip))
			goto connError;


		continue;

connError:
		perror("Unable to accept new connection");
		goto error;
	}

	pthread_attr_destroy(&thattr);

	return NULL;

error:
	raise(SIGCHLD);
	return NULL;
}


int main(int argc, char *argv[]) 
{
	printf("Main ttid: %d\n", gettid());
	pthread_mutex_init(&closeLock, NULL);

	memset(&context, 0, sizeof(context));

	if (	initVector(&context.socks, 2) 		||
		initVector(&context.conns, 2) 	||
		initVector(&context.socksThreads, 2)	) {

		perror("Unable to initialize vector");
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
	if (bindSocket(sock, sockaddr)) {
		perror("Unable to set up a socket");
		return 1;
	}


	insertVector(&context.socks, sock);
	
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_handler = handlerCallback;

	if (	sigaction(SIGINT, &act, NULL) == -1 || 
		sigaction(SIGTERM, &act, NULL) == -1 || 
		sigaction(SIGCHLD, &act, NULL) == -1  || 
		sigaction(SIGALRM, &act, NULL) == -1 ) {

		perror("sigaction");
		exit(EXIT_FAILURE);
	}

	pthread_t *thr = malloc(sizeof(pthread_t));

	if (pthread_create(thr, 0, serverListener, sock)) {
		perror("Unable to start listener");
		raise(SIGTERM);
	}

	insertVector(&context.socksThreads, thr);
	
	pthread_join(*thr, NULL);

	printf("Hello, world!\n");
	closeSocks(0);

	return 0;
}

