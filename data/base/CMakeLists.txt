ADD_CUSTOM_TARGET(quickpanel_noti.edj
		COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/image
		${CMAKE_CURRENT_SOURCE_DIR}/quickpanel_noti.edc
		${CMAKE_CURRENT_BINARY_DIR}/quickpanel_noti.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/quickpanel_noti.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} quickpanel_noti.edj)

ADD_CUSTOM_TARGET(quickpanel_theme.edj
		COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/image
		${CMAKE_CURRENT_SOURCE_DIR}/quickpanel_theme.edc
		${CMAKE_CURRENT_BINARY_DIR}/quickpanel_theme.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/quickpanel_theme.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} quickpanel_theme.edj)

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/quickpanel_noti.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/quickpanel_theme.edj DESTINATION ${EDJDIR})

