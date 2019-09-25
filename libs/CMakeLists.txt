add_subdirectory (glucose)
add_subdirectory (host)
add_subdirectory (words)
add_subdirectory (parser)
add_subdirectory (solvers)
add_subdirectory (smt)

add_library(libs INTERFACE)
target_link_libraries (libs INTERFACE parser solvers  words host glucose smtsolver)

add_library(slibs INTERFACE)
target_link_libraries (slibs INTERFACE smparser solvers  words host glucose smtsolver)
