include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)


add_executable(chttp_test
	httpParser.cc
	vectorsTest.cc
	appArgsTest.cc
)

target_link_libraries(chttp_test
	GTest::gtest_main
	chttpserv
)
target_include_directories(chttp_test
	PUBLIC ../src
)

include(GoogleTest)
gtest_discover_tests(chttp_test)
