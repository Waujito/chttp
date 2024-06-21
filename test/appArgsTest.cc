#include <gtest/gtest.h>
#include <netinet/in.h>
#include "server/utils.h"

TEST(ParseArgs, ParsesNormally) {
	const char *argv[] = {
		"program",
		"-T",
		"127.0.0.1:8888",
		"-U",
		"/tmp/1234.socket"
	};

	int argc = 5;

	struct args_t args;
	ASSERT_EQ(parseArgs(argc, argv, &args), 0);

	ASSERT_EQ(args.unixc, 1);
	ASSERT_STREQ(args.unixSocks[0], "/tmp/1234.socket");

	ASSERT_EQ(args.TCPc, 1);
	ASSERT_EQ(args.TCPAddrs[0].s_addr, htonl(2130706433U));
	ASSERT_EQ(args.TCPPorts[0], 8888);

	destroyArgs(&args);
}

TEST(ParseArgs, ParseError) {
	const char *argv[] = {
		"program",
		"-Tfidf",
		"127.0.0.1:8888"
	};

	int argc = 3;

	struct args_t args;
	testing::internal::CaptureStderr();
	ASSERT_EQ(parseArgs(argc, argv, &args), -1);
	std::string errout = testing::internal::GetCapturedStderr();

	ASSERT_STREQ(errout.c_str(), "Invalid arguments. Accepted format: program [-U </path/to/socket>...] [-T ip_addr:port...]\n");
}
