cmake_minimum_required (VERSION 3.1)

find_program(GPERF "gperf")
find_program(FLEX "flex++")

if (NOT GPERF)
    message(FATAL_ERROR "gperf is required")
endif(NOT GPERF)

if (NOT FLEX)
   	 message(FATAL_ERROR "flex++ is required")
endif(NOT FLEX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers")


function(gperf_generate_new input output)
    add_custom_command(
        OUTPUT ${output}
        COMMAND ${GPERF} -G -t -L C++ ${input} > ${output}
        DEPENDS ${input}
        COMMENT "Generate ${output}" # Just for nice message during build
    )
endfunction()

function(lex_generate_new input output)
    add_custom_command(
        OUTPUT ${output}
        COMMAND ${FLEX}  ${input}
        DEPENDS ${input}
        COMMENT "Generate ${output}" # Just for nice message during build
    )
endfunction()

gperf_generate_new(${CMAKE_CURRENT_SOURCE_DIR}/keywords.gperf ${CMAKE_CURRENT_BINARY_DIR}/key.cpp)
lex_generate_new(${CMAKE_CURRENT_SOURCE_DIR}/lexer.lex ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.cc)

add_library (parser parser.cpp ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.cc ${CMAKE_CURRENT_BINARY_DIR}/key.cpp)
target_include_directories(parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/pubinclude PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../words/pubinclude)

