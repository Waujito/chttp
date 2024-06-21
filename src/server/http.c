#include "http.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "HttpStatusCodes_C.h"


#define HEADPROCESS_METHOD 1
#define HEADPROCESS_PATH 2
#define HEADPROCESS_HTTPV 3
#define HEADPROCESS_END 4

ssize_t deleteNLSignature(char *line) {
	size_t lineLen = strlen(line);
	if (line[lineLen - 2] == '\r' && line[lineLen - 1] == '\n') {
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

int parseHTTPHead(const char *line, struct HTTPHead *res)
{
	int method;
	char *path = NULL;
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

			if (httpver == HTTPV_INVAL) {
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

	memset(res, 0, sizeof(struct HTTPHead));
	res->method = method;
	res->path = path;
	res->httpver = httpver;

	errno = 0;
	free(rreq);

	return 0;
parsingError:
	if (path != NULL)
		free(path);

	err = errno;
	free(rreq);
	errno = err;
error:
	return -1;
}
void destroyHTTPHead(struct HTTPHead *head) 
{
	free(head->path);
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

int buildHTTPHeader(struct HTTPHeader *res, const char *key, const char *value) {
	size_t keylen = strlen(key);
	size_t vallen = strlen(value);

	// Header is contiguous key value line (name\0value\0)
	size_t reslen = keylen + 1 + vallen + 1;
	char *line = malloc(sizeof(char) * reslen);
	if (line == NULL) return -1;

	strcpy(line, key);
	char *vline = line + keylen + 1;
	strcpy(vline, value);

	memset(res, 0, sizeof(struct HTTPHeader));
	res->key = line;
	res->value = vline;

	return 0;
}

void destroyHTTPHeader(struct HTTPHeader *header) {
	free(header->key);
}	

inline int createHTTPHeaderVector(struct vector_p *headers) {
	return initVector_p(headers, sizeof(struct HTTPHeader), 2);
}

ssize_t findHTTPHeader_p(struct vector_p *headers, const char *key) {
	for (size_t i = 0; i < headers->size; i++) {
		struct HTTPHeader header;
		vectorCopyEl_p(headers, i, (char *)&header);
		if (!strcmp(header.key, key)) {
			return i;
		}
	}

	return -1;
}
char *getHTTPHeader_p(struct vector_p *headers, const char *key) {
	ssize_t i = findHTTPHeader_p(headers, key);
	if (i == -1) {
		return NULL;
	}
	struct HTTPHeader header;
	vectorCopyEl_p(headers, i, (char *)&header);

	return header.value;
}

int addHTTPHeader_p(struct vector_p *headers, struct HTTPHeader *header) {
	ssize_t i = findHTTPHeader_p(headers, header->key);
	if (i != -1) {
		struct HTTPHeader oldHeader;
		vectorCopyEl_p(headers, i, (char *)&oldHeader);
		destroyHTTPHeader(&oldHeader);

		vectorSetEl_p(headers, i, (char *)header);
	} else {
		vectorInsertEl_p(headers, (char *)header);
	}

	return 0;
}
inline int addKVHTTPHeader_p(struct vector_p *headers, const char *key, const char *value) {
	struct HTTPHeader header;
	if (buildHTTPHeader(&header, key, value))
		return -1;
	if (addHTTPHeader_p(headers, &header)) {
		destroyHTTPHeader(&header);
		return -1;
	}

	return 0;
}

int deleteHTTPHeader_p(struct vector_p *headers, const char *key) {
	ssize_t i = findHTTPHeader_p(headers, key);
	if (i != -1) {
		struct HTTPHeader header;
		// Since header is defined as contiguous char line (name\0value\0) this action is safe.
		vectorCopyEl_p(headers, i, (char *)&header);
		header.value = NULL;
		vectorSetEl_p(headers, i, (char *)&header);

		return 0;
	} else {
		return -1;
	}
}

void destroyHTTPHeaderVector(struct vector_p *headers) {
	for (size_t i = 0; i < headers->size; i++) {
		struct HTTPHeader header;
		vectorCopyEl_p(headers, i, (char *)&header);
		destroyHTTPHeader(&header);	
	}
	vectorDestroy_p(headers);
}

#define HTTPHEAD_PROCESSING 0
#define HTTPHEADERS_PROCESSING 1
#define HTTPBODY_PROCESSING 2
#define HTTPPROCESSING_END 100

int parseHTTPRequest(FILE *stream, struct HTTPRequest *res)
{
	memset(res, 0, sizeof(struct HTTPRequest));

	struct HTTPHead head;
	struct vector_p headers;
	char *body;
	size_t bodyc;

	createHTTPHeaderVector(&headers);

	char *line = NULL;
	size_t nlineLen = 0;

	int processing_state = HTTPHEAD_PROCESSING;  
	while(getline(&line, &nlineLen, stream) != -1) {
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
				addHTTPHeader_p(&headers, &header);
			}
		} 
	}

	if (feof(stream) && processing_state == HTTPHEAD_PROCESSING) {
		free(line);
		destroyHTTPHeaderVector(&headers);
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

processBody:
	if (bodyc != 0) {
		body = malloc(sizeof(char) * (bodyc + 1));
		if (body == NULL) goto error;

		if (fread(body, sizeof(char), bodyc, stream) != bodyc) {
			free(body);
			printf("Unable to read %zu bytes of data\n", bodyc);
			goto error;
		}
		body[bodyc] = '\0';
	} else body = NULL;

	processing_state = HTTPPROCESSING_END;

	res->method = head.method;
	res->path = head.path;
	res->httpver = head.httpver;
	res->headers = headers;
	res->body = body;
	res->bodyc = bodyc;

	free(line);
	return HTTPREQ_SUCCESS;

error:
	free(line);
	destroyHTTPHeaderVector(&headers);
	return HTTPREQ_FAILED;
}

void destroyHTTPRequest(struct HTTPRequest *req)
{
	for (size_t i = 0; i < req->headers.size; i++) {
		struct HTTPHeader header;
		vectorCopyEl_p(&req->headers, i, (char *)&header);

		destroyHTTPHeader(&header);
	}
	free(req->body);
	vectorDestroy_p(&req->headers);
	free(req->path);
}

int initHTTPResponse(struct HTTPResponse *response, int httpver) {
	memset(response, 0, sizeof(struct HTTPResponse));

	if (createHTTPHeaderVector(&response->headers))
		return -1;

	response->httpver = httpver;
	response->bodyc = 0;
	response->body = NULL;

	return 0;
}
void destroyHTTPResponse(struct HTTPResponse *response) {
	destroyHTTPHeaderVector(&response->headers);
}

int writeHTTPResponse(struct HTTPResponse *response, FILE *stream) 
{
	const char *httpvs = HTTPVersionToString(response->httpver);
	if (httpvs == NULL) {
		errno = EINVAL;
		goto error;
	}
	if (response->status == 0) {
		errno = EINVAL;
		goto error;
	}

	const char *statusDesc = HttpStatus_reasonPhrase(response->status);

	if (statusDesc != NULL) {
		fprintf(stream, "%s %d %s\r\n", httpvs, response->status, statusDesc);
	} else {
		fprintf(stream, "%s %d\r\n", httpvs, response->status);
	}

	// https://www.w3.org/Protocols/HTTP/1.0/draft-ietf-http-spec.html#BodyLength
	char bodycs[24];
	sprintf(bodycs, "%zu", response->bodyc);
	addKVHTTPHeader_p(&response->headers, "Content-Length", bodycs);

	for (size_t i = 0; i < response->headers.size; i++) {
		struct HTTPHeader header;
		vectorCopyEl_p(&response->headers, i, (char *)&header);
		fprintf(stream, "%s: %s\r\n", header.key, header.value);
	}
	fprintf(stream, "\r\n");

	if (response->bodyc != 0) 
		if (fwrite(response->body, sizeof(char), response->bodyc, stream) < response->bodyc)
			goto error;

	fflush(stream);

	return 0;

error:
	return -1;
}



#define HTTPHEAD_PROCESSING 0
#define HTTPHEADERS_PROCESSING 1
#define HTTPBODY_PROCESSING 2
#define HTTPPROCESSING_END 100

void httpConnetionHandler(FILE *stream, void *rawargs)
{
	struct HTTPConnectionHandlerArgs *args = rawargs;

	while (!feof(stream)) {
		struct HTTPRequest req;
		int status = parseHTTPRequest(stream, &req);

		if (status == HTTPREQ_FAILED) {
			destroyHTTPRequest(&req);

			printf("Cannot parse request\n");
			goto closeHandler;
	
		} else if (status == HTTPREQ_EOF) {
			destroyHTTPRequest(&req);
			goto closeHandler;
		}

		struct HTTPResponse resp;
		if (initHTTPResponse(&resp, req.httpver)) {
			destroyHTTPRequest(&req);
			goto closeHandler;
		}

		args->httpRequestProcessor(&req, &resp);
		destroyHTTPRequest(&req);

		if (writeHTTPResponse(&resp, stream)) {
			printf("HTTP Response is invalid: %s\n", strerror(errno));
			goto closeHandler;
		}
		destroyHTTPResponse(&resp);

		if (req.httpver == HTTPV_10) break;
		else if (req.httpver == HTTPV_11) continue;
		else break;
	}


closeHandler:
	return; 
}


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
	} else if (!strcmp(method_str, "PUT")) {
		return HTTPM_POST;
	} else if (!strcmp(method_str, "DELETE")) {
		return HTTPM_POST;
	} else if (!strcmp(method_str, "CONNECT")) {
		return HTTPM_POST;
	} else if (!strcmp(method_str, "OPTIONS")) {
		return HTTPM_POST;
	} else if (!strcmp(method_str, "TRACE")) {
		return HTTPM_POST;
	} else if (!strcmp(method_str, "PATCH")) {
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

const char *HTTPVersionToString(int version)
{
	if (version == HTTPV_11) {
		return "HTTP/1.1";
	} else if (version == HTTPV_10) {
		return "HTTP/1.0";
	} else {
		return NULL;
	}
}


