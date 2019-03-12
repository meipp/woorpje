
find_program(ASCIIDOC asciidoc)
find_program(ASCIIDOCTOR asciidoctor)

if (ASCIIDOCTOR)
set(ASCIIDOC ${ASCIIDOCTOR})
endif(ASCIIDOCTOR)

if (ASCIIDOC)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/woorpje.html
				   COMMAND ${ASCIIDOC} -o ${CMAKE_CURRENT_BINARY_DIR}/woorpje.html ${CMAKE_CURRENT_SOURCE_DIR}/woorpje.adoc
                   COMMAND cp -r ${CMAKE_CURRENT_SOURCE_DIR}/images/ ${CMAKE_CURRENT_BINARY_DIR}/images/
				   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/woorpje.adoc
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks.adoc
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bechmark1.txt.table
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bechmark2.txt.table
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bechmark3.txt.table
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bechmark4.txt.table
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bechmark5.txt.table
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/images/benchmark1.png
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/images/benchmark2.png
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/images/benchmark3.png
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/images/benchmark4.png
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/images/benchmark5.png)

add_custom_target (readyPackage ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/woorpje.html
)

INSTALL (FILES ${CMAKE_CURRENT_BINARY_DIR}/woorpje.html DESTINATION doc)
INSTALL (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/images/ DESTINATION doc)
endif(ASCIIDOC)
