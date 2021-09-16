add_library (satsolver solver.cpp Main.cc regular/commons.h regular/encoder.cpp regular/proplog.cpp regular/rAbs.cpp regular/automaton.cpp regular/util.cpp)

target_include_directories(satsolver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../pubinclude
									 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/regular
									 PUBLIC $<TARGET_PROPERTY:words,INTERFACE_INCLUDE_DIRECTORIES>
									 PUBLIC $<TARGET_PROPERTY:glucose,INTERFACE_INCLUDE_DIRECTORIES>
)

add_definitions(-O3)

