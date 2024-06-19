add_library(chttpserv STATIC 
	http.c server.c utils.c
)

target_include_directories(chttpserv
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(chttpserv 
	PUBLIC chttp_compiler_flags
)
