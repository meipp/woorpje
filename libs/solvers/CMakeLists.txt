add_subdirectory (satencoding)
add_subdirectory (puresmt)

add_library (solvers STATIC $<TARGET_OBJECTS:satsolver> $<TARGET_OBJECTS:puresmt>)
target_include_directories (solvers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/pubinclude)
