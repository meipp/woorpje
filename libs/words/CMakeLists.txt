add_library (words words.cpp linconstraint.cpp algorithms.cpp outputsmt.cpp)
target_include_directories (words PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/pubinclude
						   		  PUBLIC $<TARGET_PROPERTY:host,INTERFACE_INCLUDE_DIRECTORIES>	
)

target_link_libraries(words smtparser)

target_compile_options (words PRIVATE -Wextra -Wall -Wpedantic)
