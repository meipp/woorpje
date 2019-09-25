
	    add_library (smparser INTERFACE)
    add_subdirectory(smtparser)
    target_sources (smparser INTERFACE  ${CMAKE_CURRENT_SOURCE_DIR}/parsing.cpp)
    target_include_directories(smparser INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/pubinclude)

    target_link_libraries(smparser INTERFACE sparser)
    target_compile_definitions (smparser INTERFACE -DSMTPARSER)

    add_library (parser INTERFACE)
add_subdirectory (standard)
target_link_libraries(parser INTERFACE stdparser)
target_sources (parser INTERFACE  ${CMAKE_CURRENT_SOURCE_DIR}/parsing.cpp)
target_include_directories(parser INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/pubinclude)
