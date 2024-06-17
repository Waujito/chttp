#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <stdio.h>
#include "utils.h"

/**
 * Represents a socket.
 */
struct ssock_t {
	int fd;

	struct sockaddr *addr;
	socklen_t addrlen;
};

/**
 * Represents a connection.
 */
struct connData {
	pthread_t *connThread;
	FILE *connStream;
	pthread_mutex_t *lock;
};

/**
 * Main server context.
 */
struct ApplicationContext {
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

	/**
	 * Locks close function. 
	 */
	pthread_mutex_t closeLock;
};

/**
 * Initializes server context (a global variable used to store server information between threads)
 *
 * @Returns Context creation status: 0 on success, -1 + errno otherwise.
 */
int initContext();

/**
 * @Returns current application context.
 */
struct ApplicationContext getContext();

/**
 * Starts the server. A blocking function mainly used to start the server listeners.
 * After startup the context should be cleared manually (via closeServer())
 *
 * @Returns Server starting status: 0 on success, -1 + errno otherwise.
 */
int startServer();

/**
 * Creates new socket and binds it to the sockpath
 *
 * @Returns Socket creation status: 0 on success, -1 + errno otherwise.
 */
int bindSocket(struct ssock_t *res, char *sockpath);

/**
 * Registers socket in context.
 *
 * @Returns Context registration status: 0 on success, -1 + errno otherwise.
 */
int contextRegisterSocket(struct ssock_t sock);

/**
 * Thread callback that listens on one connection.
 *
 * @cip A pointer to a size_t variable which represents an index of struct connData in context.conns vector.
 */
void *connListener(void *cip);
	
/**
 * Thread callback that listens on one socket, accepts new connections and redirects it to connListener()
 *
 * @sip A pointer to a size_t variable which represents an index of struct ssock_t in context.socks vector.
 */
void *serverListener(void *sip);

/**
 * Base function that closes an entire server: all the threads and context.
 * Should be run in the separate thread (or main thread) to escape crashes.
 *
 * @sig Signal to interrupt. Used for program execution status code: SIGINT and 0 = success, otherwise failure.
 * @Returns close status: 0 on success, -1 on close failure, 1 on @sig param pointing on failure.
 */
int closeServer(int sig);

/**
 * Thread callback that used to call closeSocks. And stop the program.
 */
void *handlerThread(void * rawsig);

/**
 * Handler callback that used to create handlerThread() thread.
 */
void handlerCallback(int sig);

/**
 * Binds sigactions to handlerCallback().
 *
 * @Returns status of binding: 0 on success, -1 + errno otherwise.
 */
int initSighandler();


#endif /* SERVER_H */
