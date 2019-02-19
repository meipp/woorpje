file(GLOB files "*.cc")

add_library (utils OBJECT ${files})
target_include_directories(utils PUBLIC ${GLUCOSE_INCLUDE})
set_property(TARGET utils PROPERTY POSITION_INDEPENDENT_CODE ON)
