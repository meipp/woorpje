add_subdirectory (standard)

add_library (parser INTERFACE)
target_sources (parser INTERFACE  ${CMAKE_CURRENT_SOURCE_DIR}/parsing.cpp)
target_link_libraries(parser INTERFACE stdparser)
target_include_directories(parser INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/pubinclude)
