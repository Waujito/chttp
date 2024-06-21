#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <err.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>      
#include <netinet/in.h>
#include <netinet/tcp.h>
#include "utils.h"
#include "server.h"

#ifndef LISTEN_BACKLOG
/**
* Specifies maximum available connections for the server.
*/
#define LISTEN_BACKLOG 1000
#endif

/**
* Global variable that maintains multiple ApplicationContexts. Used for fast close of an entire application 
* which may contain multiple Servers on.
*/
struct vector registeredApplications;

int initApplication() {
	return initVector(&registeredApplications, 2);
}

size_t registerApplication(struct ApplicationContext *context) {
	return insertVector(&registeredApplications, context);
}

int initContext(struct ApplicationContext *context, connhandler_t connhander, void *connhandlerArgs) 
{
	memset(context, 0, sizeof(struct ApplicationContext));
	if (	initVector(&context->socks, 2) ||
		initVector(&context->socksThreads, 2) || 
		initVector(&context->conns, 2) || 
		pthread_mutex_init(&context->closeLock, NULL)) {

		goto error;
	}

	context->connhandler = connhander;
	context->connhandlerArgs = connhandlerArgs;

	return 0;

error:
	return -1;
}

int startServer(struct ApplicationContext *context)
{
	size_t sz = context->socks.size;

	for (size_t i = 0; i < sz; i++) {
		struct SocketListenerContext *slContext = malloc(sizeof(struct SocketListenerContext));
		slContext->context = context;
		slContext->sip = i;

		pthread_t *thr = malloc(sizeof(pthread_t));

		if (pthread_create(thr, NULL, socketListener, slContext)) {
			perror("Unable to create thread");
			goto error;
		}

		insertVector(&context->socksThreads, thr);
	}

	for (size_t i = 0; i < context->socksThreads.size; i++) {
		pthread_t *thr = vectorGetEl(&context->socksThreads, i);
		if (thr == NULL) continue;
		
		pthread_join(*thr, NULL);
	}

	return 0;
error:
	return -1;
}


int createSocket(struct ssock *res, struct isock sockdata)
{
	int fd = socket(sockdata.socket_family, sockdata.socket_type, sockdata.protocol);

	if (fd == -1) {
		int ecode = errno;
		perror("Unable to create socket");
		errno = ecode;
		goto error;
	}

	errno = 0;

	memset(res, 0, sizeof(struct ssock));
	res->fd = fd;
	res->addr = sockdata.addr;
	res->addrlen = sockdata.addrlen;

	return 0;
error:
	return -1;
}

int bindSocket(struct ssock socket)
{
	if (bind(socket.fd, socket.addr, socket.addrlen)) {
		int err = errno;
		perror("Unable to bind socket to the given address");
		errno = err;

		goto error;
	}

	errno = 0;
	return 0;
error:
	return -1;
}

int bindUnixSocket(struct ssock *res, const char *sockpath) {
	struct isock sockdata = {
		.socket_family = AF_UNIX,
		.socket_type = SOCK_STREAM,
		.protocol = 0
	};


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

	sockdata.addr = (struct sockaddr *)addr;
	sockdata.addrlen = sizeof(*addr);

	if (createSocket(res, sockdata))
		goto error;

	if (bindSocket(*res)) {
		int err = errno;
		close(res->fd);
		errno = err;
		goto error;
	}

	return 0;

error:
	return -1;
}

int bindTCPSocket(struct ssock *res, in_port_t sin_port, struct in_addr sin_addr) {
	struct isock sockdata = {
		.socket_family = AF_INET,
		.socket_type = SOCK_STREAM,
		.protocol = 0
	};


	struct sockaddr_in *addr = calloc(sizeof(struct sockaddr_in), sizeof(char));

	addr->sin_family = AF_INET;
	addr->sin_port = htons(sin_port);
	addr->sin_addr = sin_addr;

	sockdata.addr = (struct sockaddr *)addr;
	sockdata.addrlen = sizeof(*addr);


	if (createSocket(res, sockdata))
		goto error;

	int reuse = 1;
	if (
		setsockopt(res->fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse))
#ifdef SO_REUSEPORT
		|| setsockopt(res->fd, SOL_SOCKET, SO_REUSEPORT, (const char *)&reuse, sizeof(reuse))
#endif
	) {
		int err = errno;
		close(res->fd);
		errno = err;
		goto error;
	}

	if (bindSocket(*res)) {
		int err = errno;
		close(res->fd);
		errno = err;
		goto error;
	}

	return 0;
error:
	return -1;
}



int contextRegisterSocket(struct ApplicationContext *context, struct ssock sock)
{
	struct ssock *sockp = malloc(sizeof(struct ssock));
	*sockp = sock;

	insertVector(&context->socks, sockp);

	return 0;
}

void *connListener(void *rawCLContext)
{
	if (rawCLContext == NULL) return NULL;
	struct ConnectionListenerContext *clContextp = rawCLContext;
	struct ConnectionListenerContext clContext = *clContextp;
	free(rawCLContext);

	struct ApplicationContext *context = clContext.context;
	size_t ci = clContext.cip;

	struct connData *conn = vectorGetEl(&context->conns, ci);
	if (conn == NULL) return NULL;

	FILE *stream = conn->connStream;
	
	context->connhandler(stream, context->connhandlerArgs);

closeConn:
	vectorSetEl(&context->conns, ci, NULL);

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

void *socketListener(void *rawSLContext)
{
	if (rawSLContext == NULL) return NULL;
	struct SocketListenerContext *slContextp = rawSLContext;
	struct SocketListenerContext slContext = *slContextp;
	free(rawSLContext);
	struct ApplicationContext *context = slContext.context;
	size_t si = slContext.sip;

	struct ssock *sockp = vectorGetEl(&context->socks, si);
	if (sockp == NULL) return NULL;
	struct ssock sock = *sockp;


	if (listen(sock.fd, LISTEN_BACKLOG)) {
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
		int nfd = accept(sock.fd, sock.addr, &sock.addrlen);

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

		size_t ci = insertVector(&context->conns, conn);
		struct ConnectionListenerContext *clContext = malloc(sizeof(struct ConnectionListenerContext));
		clContext->cip = ci;
		clContext->context = context;

		if (pthread_create(cthread, &thattr, connListener, clContext))
			goto connError;


		continue;

connError:
		perror("Unable to accept new connection");
		break;
	}

	pthread_attr_destroy(&thattr);

error:
	return NULL;
}

int closeServer(struct ClosingContext closeContext)
{
	struct ApplicationContext *context = closeContext.context;
	int sig = closeContext.sig;
	pthread_mutex_lock(&context->closeLock);

	int err = 0;

	printf("Closing...\n");

	for (size_t i = 0; i < context->socksThreads.size; i++) {
		pthread_t **scpt = (pthread_t **) context->socksThreads.arr + i;
		pthread_t *sockThread = *scpt;
		if (sockThread == NULL) continue;

		pthread_cancel(*sockThread);

		free(sockThread);
		*scpt = NULL;
	}

	printf("Closed listeners\n");

	for (size_t i = 0; i < context->socks.size; i++) {
		struct ssock **sockp = (struct ssock **)context->socks.arr + i;
		struct ssock *sock = *sockp;

		if (sock == NULL) continue;

		close(sock->fd);

		if (sock->addr->sa_family == AF_UNIX) {
			if (unlink(sock->addr->sa_data)) {
				err = errno;
				fprintf(stderr, "Unable to unbind unix socket %s: %s\n", sock->addr->sa_data, strerror(errno));
				errno = err;
			}
		}

		free(sock->addr);
		free(sock);
		*sockp = NULL;
	}

	printf("Closed socks\n");

	for (size_t i = 0; i < context->conns.size; i++) {
		struct connData *cd = vectorGetEl(&context->conns, i);

		if (cd == NULL) continue;
		struct connData ccd = *cd;

		if (ccd.lock == NULL || pthread_mutex_lock(ccd.lock))
			continue;

		if (vectorGetEl(&context->conns, i) == NULL) continue;
		cd->lock = NULL;

		pthread_mutex_t *lock = ccd.lock;

		pthread_t *connThread = ccd.connThread;
		pthread_cancel(*connThread);
		free(ccd.connThread);

		printf("Closing connection file %d\n", fileno(ccd.connStream));
		fclose(ccd.connStream);

		free(cd);
		vectorSetEl(&context->conns, i, NULL);

		pthread_mutex_unlock(lock);
		pthread_mutex_destroy(lock);
	}

	free(context->socksThreads.arr);
	free(context->socks.arr);
	free(context->conns.arr);

	printf("Closed all connections\n");

	if (errno == 0 && (sig == SIGINT || sig == 0)) {
		printf("Interrupted!\n");
		return 0;
	} else {
		if (errno != 0) {
			printf("Something went wrong while closing the app: %s\n", strerror(errno));
			return -1;
		} else { 
			printf("Internal failure!\n");
			return 1;
		}
	}
}

int closeApplication(int sig) {
	int closeStatus = 0;
	for (size_t i = 0; i < registeredApplications.size; i++) {
		struct ApplicationContext *context = vectorGetEl(&registeredApplications, i);
		struct ClosingContext clContext = { .context=  context, .sig = sig };
		int curStatus = closeServer(clContext);
		if (curStatus) closeStatus = curStatus;
	}

	vectorDestroy(&registeredApplications);

	return closeStatus;
}
void *handlerThread(void *sigp) {
	int sig = *(int *)sigp;
	free(sigp);

	int closeStatus = closeApplication(sig);	

	if (closeStatus) 
		exit(EXIT_FAILURE);
	else 
		exit(EXIT_SUCCESS);

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
	closeApplication(sig);
	exit(EXIT_FAILURE);
}

int initSighandler() {
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_handler = handlerCallback;

	if (	sigaction(SIGINT, &act, NULL) == -1 || 
		sigaction(SIGTERM, &act, NULL) == -1 || 
		sigaction(SIGCHLD, &act, NULL) == -1  || 
		sigaction(SIGALRM, &act, NULL) == -1 ) {

		perror("sigaction");
		return -1;
	}

	return 0;
}
