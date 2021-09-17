find_package(OpenMP REQUIRED)
add_library(satsolver solver.cpp Main.cc regular/commons.h regular/encoder.cpp regular/proplog.cpp regular/rAbs.cpp regular/automaton.cpp regular/util.cpp)

target_include_directories(satsolver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../pubinclude
        PRIVATE OpenMP::OpenMP_CXX
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/regular
        PUBLIC $<TARGET_PROPERTY:words,INTERFACE_INCLUDE_DIRECTORIES>
        PUBLIC $<TARGET_PROPERTY:glucose,INTERFACE_INCLUDE_DIRECTORIES>
        )

target_link_libraries(satsolver PRIVATE "${OpenMP_CXX_FLAGS}")
target_compile_options(satsolver PRIVATE "${OpenMP_CXX_FLAGS}")

add_definitions(-O3 )

