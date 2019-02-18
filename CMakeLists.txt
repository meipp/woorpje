add_subdirectory(libs)

add_executable (WordSolver Main.cc)
target_link_libraries (WordSolver glucose)
