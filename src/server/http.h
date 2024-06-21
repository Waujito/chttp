#ifndef HTTP_H
#define HTTP_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This section lists http methods defines.
 *
 */

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

#include <search.h>
#include "utils.h"
#include <stdio.h>
#include <sys/types.h>
/**
 * This sections lists possible HTTP versions.
 */
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

/**
 * @Retruns string version from one provided with defines. (NULL if version not valid).
 */
const char *HTTPVersionToString(int version);

struct HTTPHead {
	int method;
	char *path;
	int httpver;
};

/**
 * Parses HTTP head line (GET / HTTP/1.1)
 *
 * @line crlf- null- terminated line of http request.
 * @res A pointer to HTTPHead structure which will be rewritten.
 *
 * @Returns 0 if parsing was success, -1 otherwise.
 */
int parseHTTPHead(const char *line, struct HTTPHead *res);
void destroyHTTPHead(struct HTTPHead *head);


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

/*
 * This section lists HTTPHeaders tools
 */

/**
 * Represents an HTTP Header. To avoid Segmentation fault don't declare this structure. Use buildHTTPHeader() instead.
 * If value is NULL headers is empty (not persist).
 */
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
 * Safely builds HTTPHeader structure. Allocates memory for structure and copies user-passed values.
 * @res Pointer to blank structure to be created
 * @key, @value - Key and Value of HTTP header respectively. Null-terminated strings.
 *
 * @Returns Status of build: 0 on success, -1 otherwise.
 */
int buildHTTPHeader(struct HTTPHeader *res, const char *key, const char *value);
/**
 * Deallocates memory of HTTPHeader.
 *
 * @header Pointer to HTTPHeader structure.
 */
void destroyHTTPHeader(struct HTTPHeader *header);

/**
 * Initializes storage for HTTP Headers.
 */
int createHTTPHeaderVector(struct vector_p *headers);

/**
 * Returns index of header with key in headers vector. Designed for thread-safety for internal use.
 * When element is not found returns -1.
 */
ssize_t findHTTPHeader_p(struct vector_p *headers, const char *key);
/**
 * Returns matching http header.
 *
 * @headers vector_p of HTTPHeader structures.
 * @key Header key
 *
 * @Returns HTTP header value or NULL if header is undefined.
 */
char *getHTTPHeader_p(struct vector_p *headers, const char *key);
/**
 * Inserts HTTPHeader structure into headers vector. 
 * If header key is already specified resets it.
 */
int addHTTPHeader_p(struct vector_p *headers, struct HTTPHeader *header);
/**
 * Adds http header to headers array but also constructs it from key-value pair.
 */
int addKVHTTPHeader_p(struct vector_p *headers, const char *key, const char *value);
/**
 * Deletes HTTPHeader from headers array. (In fact setts header value to NULL).
 * @Returns 0 on successfull delete, -1 if element was not found.
 */
int deleteHTTPHeader_p(struct vector_p *headers, const char *key);
/**
 *
 * Frees space for HTTP Header vector including the vector itself. 
 * Notice that this function is not thread-safe!
 */
void destroyHTTPHeaderVector(struct vector_p *headers);

struct HTTPRequest {
	int method;
	char *path;
	int httpver;

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
 * @Returns One of HTTPREQ_ defines statuses. 
 */
int parseHTTPRequest(FILE *stream, struct HTTPRequest *res);

/**
 * Frees HTTPRequest structure.
 */
void destroyHTTPRequest(struct HTTPRequest *req);


struct HTTPResponse {
	int httpver;
	int status;
	struct vector_p *headers;
	size_t bodyc;
	const char *body;
};

/**
 * Returns response status description (e.g. `OK` on 200 status). NULL if status is not defined.
 */
const char *responseStatusDesc(int status);

/**
 * Writes HTTPResponse to stream. Notice that on errors buffer may be corrupted (semi-writte).
 *
 * @Returns HTTPResponse writing status: 0 on success, -1 otherwise.
 */
int writeHTTPResponse(struct HTTPResponse *response, FILE *stream);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_H */
