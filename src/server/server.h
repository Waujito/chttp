#ifndef SERVER_H
#define SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include "utils.h"

/**
 * Represents a ready (created) socket.
 */
struct ssock {
	int fd;

	struct sockaddr *addr;
	socklen_t addrlen;
};

/**
 * Represents a socket data that should be used to create a sock (struct ssock).
 */
struct isock {
	// Same as socket(2)
	int socket_family;
	// Same as socket(2)
	int socket_type;
	// Same as socket(2)
	int protocol;
	// Same as bind(2)
	struct sockaddr *addr;
	// Same as bind(2)
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
 * Callback function that is called on each connection.
 *
 * @connStream Conntection stream file to be processed.
 * @args Additional args specified by each handler.
 */
typedef void (*connhandler_t)(FILE *connStream, void *args);

/**
 * Context for an entire server.
 */
struct ApplicationContext {
	/**
	* Vector of struct ssock
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

	/**
	 * Callback for connections.
	 */
	connhandler_t connhandler;

	/**
	 * Context user specified arguments passed to the connection handler function.
	 */
	void *connhandlerArgs;
};

/**
 * Initializes multiple contexts handler. MUST be called on application startup.
 */
int initApplication();
/**
 * Registers an application on context.
 */
size_t registerApplication(struct ApplicationContext *context);
/**
 * Closes all servers and an entire application, deallocates all memory used by library.
 */
int closeApplication(int sig);


/**
 * Initializes server context (a global variable used to store server information between threads)
 *
 * @Returns Context creation status: 0 on success, -1 + errno otherwise.
 */
int initContext(struct ApplicationContext *context, connhandler_t connhandler, void *connhandlerArgs);
//
// /**
//  * @Returns current application context.
//  */
// struct ApplicationContext getContext();

/**
 * Starts the server. A blocking function mainly used to start the server listeners.
 * After startup the context should be cleared manually (via closeServer())
 *
 * @Returns Server starting status: 0 on success, -1 + errno otherwise.
 */
int startServer(struct ApplicationContext *context);

/**
 * Create new socket.
 *
 * @res A pointer to sock to be bound.
 * @sockdata Data used to create socket.
 *
 * @Returns Socket creation status: 0 on success, -1 + errno otherwise.
 */
int createSocket(struct ssock *res, struct isock sockdata);

/**
 * Binds a socket to sockaddr.
 *
 * @socket Socket structure.
 *
 * @Returns Socket binding status: 0 on success, -1 + errno otherwise.
 */
int bindSocket(struct ssock socket);

/**
 * Simplifies creation of the unix socket (constructs struct isock).
 *
 * @res A pointer to sock to be bound.
 * @sockpath path to the socket
 *
 * @Returns Socket creation status: 0 on success, -1 + errno otherwise.
 */
int bindUnixSocket(struct ssock *res, const char *sockpath);

/**
 * Simplifies creation of the TCP socket (constructs struct isock).
 *
 * @res A pointer to sock to be bound.
 * @sin_port Port of tcp socket. See ip(7)
 * @sin_addr Address of socket. struct in_addr represents byte format of IPv4 address. See ip(7)
 *
 * @Returns Socket creation status: 0 on success, -1 + errno otherwise.
 */
int bindTCPSocket(struct ssock *res, in_port_t sin_port, struct in_addr sin_addr);

/**
 * Registers socket in context.
 *
 * @Returns Context registration status: 0 on success, -1 + errno otherwise.
 */
int contextRegisterSocket(struct ApplicationContext *context, struct ssock sock);

struct ConnectionListenerContext {
	struct ApplicationContext *context;
	// size_t variable which represents an index of struct connData in context.conns vector.
	size_t cip;
};
/**
 * Thread callback that listens on one connection.
 *
 * @cip A pointer to a ConnectionListenerContext structure  
 */
void *connListener(void *clContext);
	
struct SocketListenerContext {
	struct ApplicationContext *context;
	// size_t variable which represents an index of struct ssock in context.socks vector.
	size_t sip;
};
/**
 * Thread callback that listens on one socket, accepts new connections and redirects it to connListener()
 *
 * @slContext a pointer to SocketListenerContext structure.
 */
void *socketListener(void *slContext);

struct ClosingContext {
	struct ApplicationContext *context;
	// Signal to interrupt. Used for program execution status code: SIGINT and 0 = success, otherwise failure.
	int sig;
};
/**
 * Base function that closes an entire server: all the threads and context.
 * Should be run in the separate thread (or main thread) to escape crashes.
 *
 * @closeContext ClosingContext structure.
 * @Returns close status: 0 on success, -1 on close failure, 1 on @sig param pointing on failure.
 */
int closeServer(struct ClosingContext closeContext);

/**
 * Thread callback that used to call closeSocks. And stop the program.
 * @context pointer to ClosingContext structure.
 */
void *handlerThread(void *rawcontext);

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


#ifdef __cplusplus
}
#endif

#endif /* SERVER_H */
