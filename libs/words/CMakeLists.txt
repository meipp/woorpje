add_library (words words.cpp linconstraint.cpp algorithms.cpp)
target_include_directories (words PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/pubinclude)
target_compile_options (words PRIVATE -Wextra -Wall -Wpedantic)
