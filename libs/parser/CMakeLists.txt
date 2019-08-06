option (ENABLE_SMTPARSER "Enable SMTParser")
add_library (parser INTERFACE)
if (ENABLE_SMTPARSER)
     target_compile_definitions (parser INTERFACE -DSMTPARSER)
	 add_subdirectory(smtparser)
   	target_link_libraries(parser INTERFACE sparser)
else()
	add_subdirectory (standard)
		target_link_libraries(parser INTERFACE stdparser)
endif(ENABLE_SMTPARSER)




target_sources (parser INTERFACE  ${CMAKE_CURRENT_SOURCE_DIR}/parsing.cpp)
target_include_directories(parser INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/pubinclude)
