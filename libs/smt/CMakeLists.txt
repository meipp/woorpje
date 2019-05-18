option (ENABLE_Z3 "Enable Z3")


add_library (smtsolver STATIC)
target_include_directories(smtsolver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/pubinclude  PUBLIC $<TARGET_PROPERTY:words,INTERFACE_INCLUDE_DIRECTORIES>)

if (ENABLE_Z3)
   find_package(Z3)
   add_definitions (-DENABLEZ3)
   target_sources (smtsolver PRIVATE z3.cpp)
   target_link_Libraries (smtsolver ${Z3_LIBRARIES})
endif(ENABLE_Z3)


   target_sources (smtsolver PRIVATE core.cpp)	


