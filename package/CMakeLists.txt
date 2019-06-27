configure_file(PKGBUILD.in ${CMAKE_CURRENT_BINARY_DIR}/PKGBUILD)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/woorpje.install  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_custom_target(makearch COMMAND cp ${PROJECT_BINARY_DIR}/${CPACK_PACKAGE_FILE_NAME}.zip ${CMAKE_CURRENT_BINARY_DIR}
                  		   COMMAND makepkg -f
)                


