#ifndef HTTP_REQ_H
#define HTTP_REQ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <search.h>
#include "utils.h"
#include <stdio.h>

#ifndef HTTP_HEAD_H
#define HTTP_HEAD_H

#include <sys/types.h>
/**
 * This section lists http methods defines.
 *
 */
#ifndef HTTP_METHOD_H
#define HTTP_METHOD_H

/**
 * This variable passed when parser unable to determine the http request
 */
#define HTTPM_FAILED -1
/**
 * This variable may be used when app shouldn't care about http request type. Used for user-development purpose.
 */
#define HTTPM_NULL 0

#define HTTPM_GET 1
#define HTTPM_HEAD 2
#define HTTPM_POST 3

/**
* Parses an HTTP request method. 
*
* @Method_str null-terminated string containing request method.
* @Returns HTTPM_FAILED on failure or one of HTTPM_ defined values otherwise.
*/
int parseHTTPMethod(const char *method_str);

#endif /* HTTP_METHOD_H */

/**
 * This sections lists possible HTTP versions.
 */
#ifndef HTTPVER_H
#define HTTPVER_H

/**
 * Invalid HTTP version indicator.
 */
#define HTTPV_INVAL -1
#define HTTPV_10 10
#define HTTPV_11 11


/**
* Parses an HTTP version. (e.g. HTTP/1.0)
*
* @version_str null-terminated string containing HTTP version.
* @Returns HTTPV_INVAL on failure or one of HTTPV_ defined values otherwise.
*/
int parseHTTPVersion(const char *version_str);

#endif /* HTTPVER_H */

struct httpHead {
	int method;
	char *path;
	int httpver;
};

/**
 * Parses HTTP head line (GET / HTTP/1.1)
 *
 * @line crlf- null- terminated line of http request.
 * @res A pointer to httpHead structure which will be rewritten.
 *
 * @Returns 0 if parsing was success, -1 otherwise.
 */
int parseHTTPHead(const char *line, struct httpHead *res);


/**
 * Deletes LF or CRLF signature from line.
 *
 * As specified CRLF signature MUST present in each HTTP request.
 * But I think there is nothing bad if we also make ability to pass LF Unix signature without any error.
 * (This use case should be avoided by the client, but it is OK if client uses something like ncat without crlf flag.)
 *
 *
 * @line Null-terminated line with LF or CRLF sign.
 * @Returns line size of -1 if no signature detected.
 */
ssize_t deleteNLSignature(char *line);
#endif /* HTTP_HEAD_H */

struct HTTPHeader {
	char *key;
	char *value;
};

/**
 * Parses http header (key: value\r\n) from string line.
 * 
 * @line (crlf- or lf-) and null- terminated string line.
 * @res Pointer to HTTPHeader structure.
 * Note that on success status HTTPHeader should be destroyHTTPHeader()-ed
 *
 * @Returns parsing status: 0 on success, -1 otherwise.
 */
int parseHTTPHeader(const char *line, struct HTTPHeader *res);
/**
 * Deallocates memory of HTTPHeader.
 *
 * @header Pointer to HTTPHeader structure.
 */
void destroyHTTPHeader(struct HTTPHeader *header);

/**
 * Returns matching http header.
 *
 * @headers vector_p of HTTPHeader structures.
 * @key Header key
 *
 * @Returns HTTP header value or NULL if header is undefined.
 */
char *getHTTPHeader_p(struct vector_p *headers, char *key);

struct HTTPRequest {
	struct httpHead head;
	/**
	 * Vector of struct HTTPHeader 
	 */
	struct vector_p headers;

	char *body;
	size_t bodyc;
};

/**
 * Indicates that end of file reached and connection should be closed.
 */
#define HTTPREQ_EOF 1
#define HTTPREQ_SUCCESS 0
#define HTTPREQ_FAILED -1

/**
 * Reads for HTTP request in stream. 
 *
 * @stream "r"-flagged stdio stream. Notice that the cursor position will be changed. 
 * After request handling it will be pointing to the start of next HTTP request. Function is blocking and waiting for input.
 *
 * @req A pointer to HTTPRequest structure where new request is stored.
 *
 * @Returns One of HTTPREQ_ defines statuses. Notice that res should be destroyHTTPRequest()-ed EVEN AFTER error.
 */
int parseHTTPRequest(FILE *stream, struct HTTPRequest *res);

/**
 * Frees HTTPRequest structure.
 */
void destroyHTTPRequest(struct HTTPRequest *req);


#ifdef __cplusplus
}
#endif

#endif /* HTTP_REQ_H */
