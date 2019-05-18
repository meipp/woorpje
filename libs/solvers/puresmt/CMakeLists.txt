add_library (puresmt OBJECT solver.cpp )
target_include_directories(puresmt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../pubinclude
									 PUBLIC $<TARGET_PROPERTY:words,INTERFACE_INCLUDE_DIRECTORIES>
									 PUBLIC $<TARGET_PROPERTY:smtsolver,INTERFACE_INCLUDE_DIRECTORIES>
)

