add_subdirectory (satencoding)

add_library (solvers STATIC $<TARGET_OBJECTS:satsolver>)
target_include_directories (solvers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/pubinclude)
