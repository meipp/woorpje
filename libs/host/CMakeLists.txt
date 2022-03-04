add_library (host)
target_sources (host PRIVATE hash.cpp)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
target_sources (host PRIVATE linux.cpp)
else()
target_sources (host PRIVATE dummy.cpp)
endif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")

target_link_libraries (host murmur)
target_include_directories (host  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/pubinclude)
target_compile_options (host PRIVATE -Wall -Wextra -Wpedantic)

