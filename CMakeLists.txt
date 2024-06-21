cmake_minimum_required(VERSION 3.15)

project(chttp VERSION 0.1.0)

add_library(chttp_compiler_flags INTERFACE)
target_compile_features(chttp_compiler_flags INTERFACE c_std_23)
target_compile_options(chttp_compiler_flags INTERFACE
	"$<BUILD_INTERFACE:-Wall;-Wextra;-Wno-unused-parameter;-Wno-sign-compare;-Wno-unused-label;-pedantic;-Wno-unused-function;-O0;-g>" 
)
target_compile_definitions(chttp_compiler_flags INTERFACE
	"_GNU_SOURCE"
)

configure_file(CHTTPConfig.h.in CHTTPConfig.h)

add_subdirectory(src)

enable_testing()
add_subdirectory(test)

add_custom_target(run
	COMMAND ./chttp -T 127.0.0.1:8888
	DEPENDS chttp
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
