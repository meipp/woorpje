add_library(satsolver solver.cpp Main.cc 
        regular/commons.h regular/proplog.cpp regular/nfa.h 
        regular/encoding.h regular/nfaencoder.cpp regular/wordencoder.cpp
        regular/rAbs.cpp regular/nfa.cpp regular/util.cpp)

target_include_directories(satsolver
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../pubinclude ${Boost_INCLUDE_DIR}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/regular
        PUBLIC $<TARGET_PROPERTY:words,INTERFACE_INCLUDE_DIRECTORIES>
        PUBLIC $<TARGET_PROPERTY:glucose,INTERFACE_INCLUDE_DIRECTORIES>
)


add_definitions(-O3 )

