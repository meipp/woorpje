set (GLUCOSE_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(core)
add_subdirectory(utils)
if (ENABLE_STATIC)
add_library (glucose STATIC $<TARGET_OBJECTS:core>  $<TARGET_OBJECTS:utils>)
else()
add_library (glucose SHARED $<TARGET_OBJECTS:core>  $<TARGET_OBJECTS:utils>)
endif(ENABLE_STATIC)


target_include_directories (glucose PUBLIC ${GLUCOSE_INCLUDE})
