add_library (smparser INTERFACE)
add_subdirectory(smtparser)
target_sources (smparser INTERFACE  ${CMAKE_CURRENT_SOURCE_DIR}/parsing.cpp)
target_include_directories(smparser INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/pubinclude)

target_link_libraries(smparser INTERFACE sparser)
target_compile_definitions (smparser INTERFACE -DSMTPARSER)

add_library (parser INTERFACE)
add_subdirectory (standard)
target_link_libraries(parser INTERFACE stdparser)
target_link_libraries(parser INTERFACE sparser)
target_sources (parser INTERFACE  ${CMAKE_CURRENT_SOURCE_DIR}/parsing.cpp)
target_include_directories(parser INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/pubinclude ${Boost_INCLUDE_DIR})
target_compile_options (parser INTERFACE -Wall -Wextra -Wpedantic)

#
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers")
#
#
# function(gperf_generate_new input output)
#     add_custom_command(
#         OUTPUT ${output}
#         COMMAND ${GPERF} -G -t -L C++ ${input} > ${output}
#         DEPENDS ${input}
#         COMMENT "Generate ${output}" # Just for nice message during build
#     )
# endfunction()
#
# function(lex_generate_new input output)
#     add_custom_command(
#         OUTPUT ${output}
#         COMMAND ${FLEX}  ${input}
#         DEPENDS ${input}
#         COMMENT "Generate ${output}" # Just for nice message during build
#     )
# endfunction()
#
# gperf_generate_new(${CMAKE_CURRENT_SOURCE_DIR}/keywords.gperf ${CMAKE_CURRENT_BINARY_DIR}/key.cpp)
# lex_generate_new(${CMAKE_CURRENT_SOURCE_DIR}/lexer.lex ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.cc)
#
# add_library (parser parser.cpp ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.cc ${CMAKE_CURRENT_BINARY_DIR}/key.cpp)
# target_include_directories(parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/pubinclude
# 				  PRIVATE $<TARGET_PROPERTY:words,INTERFACE_INCLUDE_DIRECTORIES>
# 				  PRIVATE $<TARGET_PROPERTY:solvers,INTERFACE_INCLUDE_DIRECTORIES>
# )
#
# 

