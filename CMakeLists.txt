project(wordsolver)
cmake_minimum_required(VERSION 3.10)

option(HIDEDEVNAMES "Hide developers names for review")

SET (ToolName woorpje)
SET (Translator ${ToolName}2SMT)
SET (VERSION_MAJOR 0)
SET (VERSION_MINOR "3_1")
if (HIDEDEVNAMES)
SET (MAINDEVELOPERNAME "Developers Hidden for  review")
SET (MAINDEVELOPEREMAIL "Hidden@review.com")
SET (ORGANISATION "Organisation hidden for review")
else()
SET (MAINDEVELOPERNAME "Mitja Kulczynski, Danny Boegsted Poulsen and Thorsten Ehlers")
SET (MAINDEVELOPEREMAIL "mku@informatik.uni-kiel.de, dannybpoulsen@cs.aau.dk and thorsten.ehlers@dlr.de")
SET (ORGANISATION "'Dependable Systems Group, Kiel University' and 'Technical Faculty of IT and Design' and  'Distributed, Embedded and Intelligent Systems, Aalborg University'")
endif(HIDEDEVNAMES)

SET (DEPSACK "${ToolName} is a word equation solver based on Glucose.")

option (ENABLE_STATIC "STATIC_LINKING")

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake_modules;${CMAKE_MODULE_PATH}")

option (ENABLE_TRACE "Enable Trace")
if (ENABLE_TRACE)
  	add_definitions (-DENABLETRACE)
endif(ENABLE_TRACE)

if (NOT ENABLE_STATIC)
# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
#SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
SET(CMAKE_INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
#SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

execute_process(COMMAND
  git describe --always --dirty
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  OUTPUT_VARIABLE GIT_HASH
  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)


configure_file (
 "${PROJECT_SOURCE_DIR}/config.h.in"
 "${PROJECT_BINARY_DIR}/config.h"
)
add_subdirectory(external)
add_subdirectory(libs)

SET(Boost_USE_STATIC_LIBS ON)
FIND_PACKAGE(Boost COMPONENTS program_options REQUIRED)

if (ENABLE_STATIC)
set(CMAKE_EXE_LINKER_FLAGS " -static")
endif(ENABLE_STATIC)

add_executable (${ToolName} main.cpp)
target_link_libraries (${ToolName}  libs ${Boost_LIBRARIES} )
target_include_directories(${ToolName} PRIVATE ${PROJECT_BINARY_DIR})

add_executable (${ToolName}SMT main.cpp)
target_link_libraries (${ToolName}SMT  slibs ${Boost_LIBRARIES} )
target_include_directories(${ToolName}SMT PRIVATE ${PROJECT_BINARY_DIR})



add_executable (${Translator} woorpje2smt.cpp)
target_link_libraries (${Translator}  libs ${Boost_LIBRARIES} )
target_include_directories(${Translator} PRIVATE ${PROJECT_BINARY_DIR})

SET(CPACK_GENERATOR "ZIP")
SET(CPACK_PACKAGE_FILE_NAME ${ToolName}-${VERSION_MAJOR}_${VERSION_MINOR})
INSTALL (TARGETS ${ToolName} DESTINATION bin)
INSTALL (TARGETS ${ToolName}SMT DESTINATION bin)
INSTALL (TARGETS ${Translator} DESTINATION bin)
INSTALL (FILES LICENSE DESTINATION license)


enable_testing()
add_subdirectory(test)

add_subdirectory(package)

INCLUDE(CPack)
