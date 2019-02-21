add_subdirectory (satencoding)
add_subdirectory (reachability)

add_library (solvers STATIC $<TARGET_OBJECTS:satsolver> $<TARGET_OBJECTS:reachability>)
target_include_directories (solvers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/pubinclude)
