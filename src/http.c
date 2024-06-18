#include "http.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int parseHTTPMethod(const char *method_str)
{
	errno = 0;

	if (method_str == NULL) {
		errno = EINVAL;
		return HTTPM_FAILED;
	} else if (!strcmp(method_str, "GET")) {
		return HTTPM_GET;
	} else if (!strcmp(method_str, "HEAD")) {
		return HTTPM_HEAD;
	} else if (!strcmp(method_str, "POST")) {
		return HTTPM_POST;
	} else {
		errno = EINVAL;
		return HTTPM_FAILED;
	}
}

#define HEADPROCESS_METHOD 1
#define HEADPROCESS_PATH 2
#define HEADPROCESS_HTTPV 3
#define HEADPROCESS_END 4

ssize_t deleteNLSignature(char *line) {
	size_t lineLen = strlen(line);
	if (line[lineLen - 2] == '\r' || line[lineLen - 1] == '\n') {
		line[lineLen - 2] = '\0';
		lineLen -= 2;
	} else if (line[lineLen - 1] == '\n') {
		line[lineLen - 1] = '\0';
		lineLen -= 1;
	} else {
		printf("Invalid HTTP request. Prehaps a CRLF signature missing\n");
		errno = EINVAL;
		return -1;
	}

	errno = 0;
	return lineLen;
}

int parseHTTPHead(const char *line, struct httpHead *res)
{
	int method;
	char *path;
	int httpver;

	int err = 0;

	char *req = malloc(sizeof(char) * (strlen(line) + 1));
	char *rreq = req;
	strcpy(req, line);

	ssize_t lineLen = deleteNLSignature(req);
	if (lineLen == -1) goto parsingError;

	char *next_token = NULL;

	int reqprocess_state = HEADPROCESS_METHOD;
	while (1) {
		char *token;
		token = strtok_r(req, " ", &next_token);
		req = next_token;
		if (token == NULL) break;

		if (reqprocess_state == HEADPROCESS_METHOD) {
			method = parseHTTPMethod(token);

			if (method == HTTPM_FAILED) {
				printf("HTTP method processing failed\n");
				errno = EINVAL;
				goto parsingError;
			}

		} else if (reqprocess_state == HEADPROCESS_PATH) {
			path = malloc(sizeof(char) * (strlen(token) + 1));
			strcpy(path, token);

		} else if (reqprocess_state == HEADPROCESS_HTTPV) {
			// TODO: Implement request versioning.
			httpver = 0;

		} else {
			goto parsingError;
		}

		reqprocess_state++;
	}

	if (reqprocess_state != HEADPROCESS_END) {
		printf("Invalid HTTP request line\n");
		errno = EINVAL;
		goto parsingError;
	}

	memset(res, 0, sizeof(struct httpHead));
	res->method = method;
	res->path = path;
	res->httpver = httpver;

	errno = 0;
	free(rreq);

	return 0;

parsingError:
	err = errno;
	free(rreq);
	errno = err;
error:
	return -1;
}
