add_subdirectory(libs)

add_executable (WordSolver Main.cc)
target_link_libraries (WordSolver glucose words)


add_executable (WordSolver2 main.cpp)
target_link_libraries (WordSolver2  words parser)
