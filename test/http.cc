#include <gtest/gtest.h>
#include <cstring>
#include "server/http.h"

char *stringToCharArr(std::string sline) {
	char *line = (char *)malloc(sizeof(char) * (strlen(sline.c_str()) + 1));
	strcpy(line, sline.c_str());

	return line;
}

TEST(HTTPParser, HTTPMethod) {
	ASSERT_EQ(parseHTTPMethod("GET"), HTTPM_GET);
	ASSERT_EQ(parseHTTPMethod("HEAD"), HTTPM_HEAD);
	ASSERT_EQ(parseHTTPMethod("POST"), HTTPM_POST);
	ASSERT_EQ(parseHTTPMethod("QWERTY"), HTTPM_FAILED);

	// Try to hack parser
	ASSERT_EQ(parseHTTPMethod("GET\0\0\0\0"), HTTPM_GET);
	ASSERT_EQ(parseHTTPMethod("GET\n\0"), HTTPM_FAILED);
	ASSERT_EQ(parseHTTPMethod("GE\0T"), HTTPM_FAILED);
	ASSERT_EQ(parseHTTPMethod("\0GET\0"), HTTPM_FAILED);
}

TEST(HTTPParse, HTTPVersion) {
	ASSERT_EQ(parseHTTPVersion("HTTP/1.0"), HTTPV_10);
	ASSERT_EQ(parseHTTPVersion("HTTP/1.1"), HTTPV_11);

	ASSERT_EQ(parseHTTPVersion("HTTP/1.12"), HTTPV_INVAL);

	// HTTP 2 is not supported yet.
	ASSERT_EQ(parseHTTPVersion("HTTP/2.0"), HTTPV_INVAL);

	ASSERT_EQ(parseHTTPVersion("HTTP/\n1.0"), HTTPV_INVAL);
}

TEST(HTTPParse, CRLFSignature) {
	char *line = stringToCharArr("QWERTY\r\n");
	ASSERT_EQ(deleteNLSignature(line), 6);
	ASSERT_STREQ(line, "QWERTY");
	free(line);

	line = stringToCharArr("QWERTY\n");
	ASSERT_EQ(deleteNLSignature(line), 6);
	ASSERT_STREQ(line, "QWERTY");
	free(line);

	line = stringToCharArr("QWERTY\rd");
	ASSERT_EQ(deleteNLSignature(line), -1);
	ASSERT_STREQ(line, "QWERTY\rd");
	free(line);
}

TEST(HTTPParse, HTTPHead) {
	struct HTTPHead head;

	ASSERT_EQ(parseHTTPHead("GET / HTTP/1.1\r\n", &head), 0);
	ASSERT_EQ(head.method, HTTPM_GET);
	ASSERT_EQ(head.httpver, HTTPV_11);
	ASSERT_STREQ(head.path, "/");
	destroyHTTPHead(&head);
	
	ASSERT_EQ(parseHTTPHead("GET / HTTP/1.1\r", &head), -1);

	ASSERT_EQ(parseHTTPHead("GET / HTTP/1.1\n", &head), 0);
	destroyHTTPHead(&head);

	ASSERT_EQ(parseHTTPHead("GETS / HTTP/1.1\r\n", &head), -1);
	ASSERT_EQ(parseHTTPHead("GET HTTP/1.1\r\n", &head), -1);
	ASSERT_EQ(parseHTTPHead("GET / HTTP/1.12\r\n", &head), -1);
	errno = 0;
	ASSERT_EQ(parseHTTPHead("GET \0 HTTP/1.1\r\n", &head), -1);
	ASSERT_EQ(errno, EINVAL);
}


TEST(HTTPParse, HTTPHeader) {
	struct HTTPHeader header;

	ASSERT_EQ(parseHTTPHeader("Authorization: Bearer\r\n", &header), 0);
	ASSERT_STREQ(header.key, "Authorization");
	ASSERT_STREQ(header.value, "Bearer");
	destroyHTTPHeader(&header);

	ASSERT_EQ(parseHTTPHeader("Authorization: Bearer", &header), -1);

	ASSERT_EQ(parseHTTPHeader("A:B\r\n", &header), 0);
	ASSERT_STREQ(header.key, "A");
	ASSERT_STREQ(header.value, "B");
	destroyHTTPHeader(&header);

	ASSERT_EQ(parseHTTPHeader("A: B\r\n", &header), 0);
	ASSERT_STREQ(header.key, "A");
	ASSERT_STREQ(header.value, "B");
	destroyHTTPHeader(&header);

	ASSERT_EQ(parseHTTPHeader("A:  B\r\n", &header), 0);
	ASSERT_STREQ(header.key, "A");
	ASSERT_STREQ(header.value, " B");
	destroyHTTPHeader(&header);

	ASSERT_EQ(parseHTTPHeader("A : B \r\n", &header), 0);
	ASSERT_STREQ(header.key, "A ");
	ASSERT_STREQ(header.value, "B ");
	destroyHTTPHeader(&header);
}

TEST(HTTP, HTTPHeaderUnit) {
	struct HTTPHeader header;

	buildHTTPHeader(&header, "Authorization", "Bearer");
	ASSERT_STREQ(header.key, "Authorization");
	ASSERT_STREQ(header.value, "Bearer");
	destroyHTTPHeader(&header);
}

TEST(HTTP, HTTPHeaderVector) {
	struct vector_p headers;
	struct HTTPHeader header;

	createHTTPHeaderVector(&headers);
	buildHTTPHeader(&header, "Authorization", "Bearer");
	addHTTPHeader_p(&headers, &header);
	ASSERT_STREQ(getHTTPHeader_p(&headers, "Authorization"), "Bearer");
	ASSERT_EQ(getHTTPHeader_p(&headers, "QWERTY"), nullptr);

	buildHTTPHeader(&header, "Authorization2", "Bearer2");
	addHTTPHeader_p(&headers, &header);
	ASSERT_STREQ(getHTTPHeader_p(&headers, "Authorization"), "Bearer");
	ASSERT_STREQ(getHTTPHeader_p(&headers, "Authorization2"), "Bearer2");
	ASSERT_EQ(getHTTPHeader_p(&headers, "QWERTY"), nullptr);

	deleteHTTPHeader_p(&headers, "Authorization2");
	ASSERT_STREQ(getHTTPHeader_p(&headers, "Authorization"), "Bearer");
	ASSERT_EQ(getHTTPHeader_p(&headers, "Authorization2"), nullptr);
	ASSERT_EQ(getHTTPHeader_p(&headers, "QWERTY"), nullptr);

	destroyHTTPHeaderVector(&headers);
}

TEST(HTTPparse, HTTPRequest) {
	struct HTTPRequest req;
	
	FILE * stream = tmpfile();
	fprintf(stream, "GET / HTTP/1.1\r\nHead: example.com\r\nContent-Length: 10\r\n\r\nabcdefghjklmn\r\n");
	fflush(stream);
	ASSERT_EQ(fseek(stream, 0, SEEK_SET), 0);
	ASSERT_EQ(parseHTTPRequest(stream, &req), 0);
	ASSERT_EQ(req.bodyc, 10);
	ASSERT_EQ(req.method, HTTPM_GET);
	ASSERT_STREQ(req.path, "/");
	ASSERT_EQ(req.httpver, HTTPV_11);
	ASSERT_STREQ(req.body, "abcdefghjk");
	struct HTTPHeader *header = (struct HTTPHeader *)vectorGetEl_p(&req.headers, 0);
	ASSERT_STREQ(header->key, "Head");
	ASSERT_STREQ(header->value, "example.com");
	header = (struct HTTPHeader *)vectorGetEl_p(&req.headers, 1);
	ASSERT_STREQ(header->key, "Content-Length");
	ASSERT_STREQ(header->value, "10");
	destroyHTTPRequest(&req);

	ASSERT_EQ(parseHTTPRequest(stream, &req), -1);
	destroyHTTPRequest(&req);

}

TEST(HTTP, ResponseWriter) {
	FILE *stream = tmpfile();
	struct vector_p headers;
	createHTTPHeaderVector(&headers);
	addKVHTTPHeader_p(&headers, "Authorization", "Bearer");

	const char *body = "abcdefghjk\r\n";
	size_t bodyc = strlen(body);
	struct HTTPResponse response = {
		HTTPV_11,
		200,
		&headers,
		bodyc,
		body,
	};

	writeHTTPResponse(&response, stream);
	fseek(stream, 0, SEEK_SET);

	char *line = NULL;
	size_t lineLen = 0;

	getline(&line, &lineLen, stream);
	ASSERT_STREQ(line, "HTTP/1.1 200 OK\r\n");

	getline(&line, &lineLen, stream);
	ASSERT_STREQ(line, "Authorization: Bearer\r\n");

	getline(&line, &lineLen, stream);
	ASSERT_STREQ(line, "Content-Length: 12\r\n");

	getline(&line, &lineLen, stream);
	ASSERT_STREQ(line, "\r\n");

	getline(&line, &lineLen, stream);
	ASSERT_STREQ(line, body);

	free(line);
	destroyHTTPHeaderVector(&headers);
}
