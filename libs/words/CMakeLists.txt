set (GLUCOSE_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR})


add_library (words words.cpp)
target_include_directories (words PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/pubinclude)
