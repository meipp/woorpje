project(wordsolver)

SET (ToolName WoSol)
SET (VERSION_MAJOR 0)
SET (VERSION_MINOR 1)
SET (MAINDEVELOPERNAME "Mitja Kulszynski")
SET (MAINDEVELOPEREMAIL "mku@informatik.uni-kiel.de")
SET (ORGANISATION "Dependable Systems Group, Kiel Univeristy")
SET (DEPSACK "Based on Glucose.")

execute_process(COMMAND
  git describe --always --dirty
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  OUTPUT_VARIABLE GIT_HASH
  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
	

configure_file (
 "${PROJECT_SOURCE_DIR}/config.h.in"
 "${PROJECT_BINARY_DIR}/config.h"
)
add_subdirectory(libs)

SET(Boost_USE_STATIC_LIBS ON)
FIND_PACKAGE(Boost COMPONENTS program_options REQUIRED)


add_executable (${ToolName} main.cpp)
target_link_libraries (${ToolName}  words parser solvers glucose ${Boost_LIBRARIES} )
target_include_directories(${ToolName} PRIVATE ${PROJECT_BINARY_DIR})
