option (ENABLE_Z3 "Enable Z3")
option (ENABLE_CVC4 "Enable CVC4")

add_library (smtsolver STATIC)
target_include_directories(smtsolver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/pubinclude  PUBLIC $<TARGET_PROPERTY:words,INTERFACE_INCLUDE_DIRECTORIES>)

if (ENABLE_Z3)
   find_package(Z3)
   add_definitions (-DENABLEZ3)
   target_sources (smtsolver PRIVATE z3.cpp)
   target_link_Libraries (smtsolver ${Z3_LIBRARIES})
   target_include_directories(smtsolver PRIVATE ${Z3_INCLUDE_DIRS})
endif(ENABLE_Z3)

if (ENABLE_CVC4)
   find_package(CVC4)
   add_definitions (-DENABLECVC4)
   target_sources (smtsolver PRIVATE cvc4.cpp)
   target_link_Libraries (smtsolver ${CVC4_LIBRARIES})
   target_include_directories(smtsolver PRIVATE ${CVC4_INCLUDE_DIRS})
endif(ENABLE_CVC4)

target_sources (smtsolver PRIVATE core.cpp)	


