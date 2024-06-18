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
* Parses an http request method. 
*
* @method_str null-terminated string containing request method.
* @Returns HTTPM_FAILED on failure or one of HTTP_METHOD defined values otherwise.
*/
int parseHTTPMethod(const char *method_str);

#endif /* HTTP_METHOD_H */

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
 * @line Null-terminated line with LF or CRLF sign.
 * @Returns line size of -1 if no signature detected.
 */
ssize_t deleteNLSignature(char *line);
#endif /* HTTP_HEAD_H */
