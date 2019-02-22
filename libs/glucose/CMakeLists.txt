set (GLUCOSE_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(core)
add_subdirectory(utils)

add_library (glucose SHARED $<TARGET_OBJECTS:core>  $<TARGET_OBJECTS:utils>)
target_include_directories (glucose PUBLIC ${GLUCOSE_INCLUDE})
INSTALL (TARGETS glucose DESTINATION lib)
