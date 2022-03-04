add_subdirectory (satencoding)
add_subdirectory (puresmt)
add_subdirectory (levis)

add_library (solvers INTERFACE) 
target_include_directories (solvers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/pubinclude ${Boost_INCLUDE_DIR})
target_link_libraries (solvers INTERFACE satsolver puresmt levis)
