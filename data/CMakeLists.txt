INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/icons DESTINATION ${RESDIR} FILES_MATCHING PATTERN "*.png")
#CONFIGURE_FILE(${PROJECT_NAME}.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/org.tizen.${PROJECT_NAME}.desktop)
#INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.tizen.${PROJECT_NAME}.desktop DESTINATION ${DESKTOPDIR})
CONFIGURE_FILE(${PROJECT_NAME}.xml.in ${CMAKE_CURRENT_BINARY_DIR}/org.tizen.${PROJECT_NAME}.xml)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.tizen.${PROJECT_NAME}.xml DESTINATION ${MANIFESTXMLDIR})
CONFIGURE_FILE(${PROJECT_NAME}.sh.in ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME})
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME} DESTINATION /etc/init.d/)

MESSAGE("EDJE_CC CUSTOM FLAG : ${EDJEFLAGS}")

ADD_CUSTOM_TARGET(quickpanel.edj
		COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/images_icon  -id ${CMAKE_CURRENT_SOURCE_DIR}/images "${EDJEFLAGS}"
		${CMAKE_CURRENT_SOURCE_DIR}/quickpanel.edc
		${CMAKE_CURRENT_BINARY_DIR}/quickpanel.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/quickpanel.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} quickpanel.edj)

ADD_CUSTOM_TARGET(quickpanel_activenoti.edj
                COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/images_icon  -id ${CMAKE_CURRENT_SOURCE_DIR}/images "${EDJEFLAGS}"
                ${CMAKE_CURRENT_SOURCE_DIR}/quickpanel_activenoti.edc
                ${CMAKE_CURRENT_BINARY_DIR}/quickpanel_activenoti.edj
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/quickpanel_activenoti.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} quickpanel_activenoti.edj)

ADD_CUSTOM_TARGET(quickpanel_voice_control.edj
                COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/images_icon  -id ${CMAKE_CURRENT_SOURCE_DIR}/images "${EDJEFLAGS}"
                ${CMAKE_CURRENT_SOURCE_DIR}/quickpanel_voice_control.edc
                ${CMAKE_CURRENT_BINARY_DIR}/quickpanel_voice_control.edj
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/quickpanel_voice_control.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} quickpanel_voice_control.edj)



ADD_CUSTOM_TARGET(quickpanel_theme.edj
		COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/images_icon -id ${CMAKE_CURRENT_SOURCE_DIR}/images "${EDJEFLAGS}"
		${CMAKE_CURRENT_SOURCE_DIR}/quickpanel_theme.edc
		${CMAKE_CURRENT_BINARY_DIR}/quickpanel_theme.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/quickpanel_theme.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} quickpanel_theme.edj)

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/quickpanel.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/quickpanel_activenoti.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/quickpanel_voice_control.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/quickpanel_theme.edj DESTINATION ${EDJDIR})
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/icons DESTINATION ${RESDIR} FILES_MATCHING PATTERN "*.png")

