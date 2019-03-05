find_program(ASCIIDOC asciidoc)

if (ASCIIDOC)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/woorpje.html
				   COMMAND ${ASCIIDOC} -o ${CMAKE_CURRENT_BINARY_DIR}/woorpje.html ${CMAKE_CURRENT_SOURCE_DIR}/woorpje.adoc 
				   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/woorpje.adoc)

add_custom_target (readyPackage ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/woorpje.html
				                    )

INSTALL (FILES ${CMAKE_CURRENT_BINARY_DIR}/woorpje.html DESTINATION doc)
endif(ASCIIDOC)
