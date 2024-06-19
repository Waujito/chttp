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

int parseHTTPVersion(const char *version_str)
{
	errno = 0;

	if (version_str == NULL) {
		errno = EINVAL;
		return HTTPV_INVAL;
	} else if (!strcmp(version_str, "HTTP/1.0")) {
		return HTTPV_10;
	} else if (!strcmp(version_str, "HTTP/1.1")) {
		return HTTPV_11;
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
	while (reqprocess_state <= HEADPROCESS_END) {
		char *token;
		token = strtok_r(req, " ", &next_token);
		req = next_token;
		if (token == NULL) break;

		if (reqprocess_state == HEADPROCESS_METHOD) {
			method = parseHTTPMethod(token);

			if (method == HTTPM_FAILED) {
				printf("HTTP invalid method\n");
				errno = EINVAL;
				goto parsingError;
			}
		} else if (reqprocess_state == HEADPROCESS_PATH) {
			path = malloc(sizeof(char) * (strlen(token) + 1));
			strcpy(path, token);

		} else if (reqprocess_state == HEADPROCESS_HTTPV) {
			httpver = parseHTTPVersion(token);

			if (method == HTTPV_INVAL) {
				printf("HTTP invalid version\n");
				errno = EINVAL;
				goto parsingError;
			}
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

int parseHTTPHeader(const char *line, struct HTTPHeader *res)
{
	char *req = malloc(sizeof(char) * (strlen(line) + 1));
	char *rreq = req;
	strcpy(req, line);

	ssize_t lineLen = deleteNLSignature(req);
	if (lineLen == -1) goto error;

	char *next_token = NULL;
	char *token;
	token = strtok_r(req, ":", &next_token);
	req = next_token;


	
	res->key = token;
	res->value = req;

	// Delete leading space.
	if (strlen(res->value) != 0 && *res->value == ' ') res->value++;

	return 0;
error:
	free(rreq);
	return -1;
}

void destroyHTTPHeader(struct HTTPHeader *header) {
	free(header->key);
}	

char *getHTTPHeader_p(struct vector_p *headers, char *key) {
	for (size_t i = 0; i < headers->size; i++) {
		struct HTTPHeader header;
		vectorCopyEl_p(headers, i, (char *)&header);		
		if (!strcmp(header.key, key)) {
			return header.value;
		}
	}

	return NULL;
}

#define HTTPHEAD_PROCESSING 0
#define HTTPHEADERS_PROCESSING 1
#define HTTPBODY_PROCESSING 2
#define HTTPPROCESSING_END 100

int parseHTTPRequest(FILE *stream, struct HTTPRequest *res)
{
	memset(res, 0, sizeof(struct HTTPRequest));

	struct httpHead head;
	struct vector_p headers;
	char *body;
	size_t bodyc;

	initVector_p(&headers, sizeof(struct HTTPHeader), 2);

	char *line = NULL;
	size_t nlineLen = 0;

	int processing_state = HTTPHEAD_PROCESSING;  
	while(getline(&line, &nlineLen, stream) != -1) {
		// printf("%s", line);
		if (processing_state == HTTPHEAD_PROCESSING) {
			if (parseHTTPHead(line, &head)) {
				printf("Unable to parse head\n");
				goto error;
			} else {
				processing_state++;
			}
		} else if (processing_state == HTTPHEADERS_PROCESSING) {
			struct HTTPHeader header;

			if (!strcmp(line, "\r\n") || !strcmp(line, "\n")) {
				goto keepProcess;
			} else if (parseHTTPHeader(line, &header)) {
				printf("Unable to parse header string: %s", line);
				goto error;
			} else {
				vectorInsertEl_p(&headers, (char *)&header);
			}
		} 
	}

	if (feof(stream) && processing_state == HTTPHEAD_PROCESSING) {
		free(line);
		return HTTPREQ_EOF;
	}
	printf("Request processing failed\n");
	goto error;

keepProcess:
	bodyc = 0;

	char *contentSizeH = getHTTPHeader_p(&headers, "Content-Length");
	if (contentSizeH == NULL) goto processBody;

	char *end;
	long contentSZ = strtoll(contentSizeH, &end, 10);

	if (end - contentSizeH != strlen(contentSizeH))
		goto error;

	bodyc = contentSZ;
	printf("%zu\n", bodyc);

processBody:
	body = malloc(sizeof(char) * bodyc);
	if (body == NULL) goto error;

	if (fread(body, sizeof(char), bodyc, stream) != bodyc) {
		printf("Unable to read %zu bytes of data\n", bodyc);
		goto error;
	}

	processing_state = HTTPPROCESSING_END;

	res->head = head;
	res->headers = headers;
	res->body = body;
	res->bodyc = bodyc;

	free(line);
	return HTTPREQ_SUCCESS;

error:
	free(line);
	return HTTPREQ_FAILED;
}

void destroyHTTPRequest(struct HTTPRequest *req)
{
	free(req->body);
}
