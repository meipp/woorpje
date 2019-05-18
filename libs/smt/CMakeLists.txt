option (ENABLE_Z3 "Enable Z3")

if (ENABLE_Z3)
   find_package(Z3)
endif(ENABLE_Z3)

add_library (smtsolver STATIC)
target_include_directories(smtsolver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/pubinclude  PUBLIC $<TARGET_PROPERTY:words,INTERFACE_INCLUDE_DIRECTORIES>)

if (ENABLE_Z3)
   target_sources (smtsolver PUBLIC z3.cpp)
endif(ENABLE_Z3)




