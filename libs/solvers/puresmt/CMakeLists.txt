add_library (puresmt solver.cpp )
target_include_directories(puresmt
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../pubinclude ${Boost_INCLUDE_DIR}
	PUBLIC $<TARGET_PROPERTY:words,INTERFACE_INCLUDE_DIRECTORIES>
	PUBLIC $<TARGET_PROPERTY:smtsolver,INTERFACE_INCLUDE_DIRECTORIES>
)

