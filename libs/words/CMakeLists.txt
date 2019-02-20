add_library (words words.cpp linconstraint.cpp)
target_include_directories (words PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/pubinclude)
