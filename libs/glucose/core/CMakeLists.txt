file(GLOB files "*.cc")

add_library (core OBJECT ${files})
target_include_directories(core PUBLIC ${GLUCOSE_INCLUDE})
set_property(TARGET core PROPERTY POSITION_INDEPENDENT_CODE ON)
