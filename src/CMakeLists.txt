add_executable(chttp app.c)

add_subdirectory(server)

target_link_libraries(chttp
	PUBLIC chttpserv chttp_compiler_flags
)

add_custom_command(TARGET chttp
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:chttp> ../
)
