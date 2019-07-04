
find_program(ASCIIDOC asciidoc)
find_program(ASCIIDOCTOR asciidoctor)

if (ASCIIDOCTOR)
set(ASCIIDOC ${ASCIIDOCTOR})
endif(ASCIIDOCTOR)

if (ASCIIDOC)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/woorpje.html
				   COMMAND ${ASCIIDOC} -o ${CMAKE_CURRENT_BINARY_DIR}/woorpje.html ${CMAKE_CURRENT_SOURCE_DIR}/woorpje.adoc
                   OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/woorpjeLevi.html
                           COMMAND ${ASCIIDOC} -o ${CMAKE_CURRENT_BINARY_DIR}/woorpjeLevi.html ${CMAKE_CURRENT_SOURCE_DIR}/woorpjeLevi.adoc
                   COMMAND cp -r ${CMAKE_CURRENT_SOURCE_DIR}/images/ ${CMAKE_CURRENT_BINARY_DIR}/images/
				   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/woorpjeLevi.adoc
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
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/images/benchmark5.png
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/benchmarksLevi.adoc
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bechmarkLevi1.txt.table
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bechmarkLevi2.txt.table
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bechmarkLevi3.txt.table
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bechmarkLevi4.txt.table
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bechmarkLevi5.txt.table)

add_custom_target (readyPackage ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/woorpje.html)
add_custom_target (readyPackage2 ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/woorpjeLevi.html)

INSTALL (FILES ${CMAKE_CURRENT_BINARY_DIR}/woorpje.html DESTINATION doc)
INSTALL (FILES ${CMAKE_CURRENT_BINARY_DIR}/woorpjeLevi.html DESTINATION doc)
INSTALL (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/images/ DESTINATION doc)
endif(ASCIIDOC)
