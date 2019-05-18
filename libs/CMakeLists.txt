add_subdirectory (glucose)
add_subdirectory (host)
add_subdirectory (words)
add_subdirectory (parser)
add_subdirectory (solvers)
add_subdirectory (smt)

add_library(libs INTERFACE)
target_link_libraries (libs INTERFACE host parser words solvers glucose smtsolver)
