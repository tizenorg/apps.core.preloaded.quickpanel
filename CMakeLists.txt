CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(quickpanel C)

SET(VENDOR "org.tizen")
SET(PACKAGE ${PROJECT_NAME})
SET(PKGNAME "${VENDOR}.${PACKAGE}")
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(BINDIR "${PREFIX}/bin")
SET(RESDIR "${PREFIX}/res")
SET(LOCALEDIR "${RESDIR}/locale")
SET(ICONDIR "${RESDIR}/icons")
SET(EDJDIR "${RESDIR}/edje")
SET(DATADIR "${PREFIX}/data")
SET(DATADIR_RW "${PREFIX_RW}/data")
SET(DESKTOPDIR "/usr/share/applications")
SET(MANIFESTXMLDIR "/usr/share/packages")
SET(SHARED_DIR "${PREFIX}/shared/res")

IF("${CMAKE_BUILD_TYPE}" STREQUAL "")
	SET(CMAKE_BUILD_TYPE "Release")
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "")
MESSAGE("Build type: ${CMAKE_BUILD_TYPE}")

INCLUDE_DIRECTORIES(
		${CMAKE_CURRENT_SOURCE_DIR}/daemon
		${CMAKE_CURRENT_SOURCE_DIR}/daemon/minictrl
		${CMAKE_CURRENT_SOURCE_DIR}/daemon/notifications
		${CMAKE_CURRENT_SOURCE_DIR}/daemon/service
		${CMAKE_CURRENT_SOURCE_DIR}/daemon/settings
		${CMAKE_CURRENT_SOURCE_DIR}/daemon/settings/modules
		${CMAKE_CURRENT_SOURCE_DIR}/daemon/vi
		${CMAKE_CURRENT_SOURCE_DIR}/daemon/page
		${CMAKE_CURRENT_SOURCE_DIR}/daemon/datetime
		${CMAKE_CURRENT_SOURCE_DIR}/data
		${CMAKE_CURRENT_SOURCE_DIR}/test
		)

INCLUDE(FindPkgConfig)

IF(X11_SUPPORT)
pkg_check_modules(extra_pkgs REQUIRED
	ecore-x
	x11
	xi
	utilX
	inputproto
)
ENDIF(X11_SUPPORT)

IF(WAYLAND_SUPPORT)
pkg_check_modules(extra_pkgs REQUIRED
	ecore-wayland
)
ENDIF(WAYLAND_SUPPORT)

pkg_check_modules(pkgs REQUIRED
	capi-appfw-application
	capi-system-runtime-info
	capi-system-info
	capi-system-device
	capi-network-tethering
	capi-network-bluetooth
	capi-network-connection
	capi-network-wifi
	capi-media-player
	capi-media-sound-manager
	capi-media-metadata-extractor
	capi-system-system-settings
	capi-base-utils-i18n
	capi-ui-efl-util
	tapi
	feedback
	appcore-common
	notification
	badge
	vconf
	libprivilege-control
	edbus
	evas
	ecore
	edje
	ecore-input
	icu-i18n
	elementary
	efl-assist
	dlog
	syspopup-caller
	minicontrol-viewer
	minicontrol-monitor
	pkgmgr
	pkgmgr-info
	iniparser
	alarm-service
)

FOREACH(flag ${pkgs_CFLAGS} ${extra_pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -Wall -g -O2 -fpie")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g")
SET(CMAKE_C_FLAGS_RELEASE "-O2")

FIND_PROGRAM(UNAME NAMES uname)
EXEC_PROGRAM("${UNAME}" ARGS "-m" OUTPUT_VARIABLE "ARCH")
IF("${ARCH}" STREQUAL "arm")
	ADD_DEFINITIONS("-DTARGET")
	MESSAGE("add -DTARGET")
ENDIF("${ARCH}" STREQUAL "arm")

IF("${VENDOR}" STREQUAL "org.tizen")
	ADD_DEFINITIONS("-DQP_SETTING_ENABLE")
	ADD_DEFINITIONS("-DQP_BRIGHTNESS_ENABLE")
	ADD_DEFINITIONS("-DQP_MINICTRL_ENABLE")
	ADD_DEFINITIONS("-DQP_SCREENREADER_ENABLE")
	ADD_DEFINITIONS("-DQP_SMART_ALERT_ENABLE")
	ADD_DEFINITIONS("-DQP_SERVICE_NOTI_LED_ENABLE")
	ADD_DEFINITIONS("-DQP_ANIMATED_IMAGE_ENABLE")
	ADD_DEFINITIONS("-DQP_REMINDER_ENABLE")
	ADD_DEFINITIONS("-DQP_EMERGENCY_MODE_ENABLE")
ENDIF("${VENDOR}" STREQUAL "org.tizen")
MESSAGE("##### VENDOR: ${VENDOR}")

ADD_DEFINITIONS("-DVENDOR=\"${VENDOR}\"")
ADD_DEFINITIONS("-DPACKAGE=\"${PACKAGE}\"")
ADD_DEFINITIONS("-DPACKAGE_NAME=\"${PKGNAME}\"")
ADD_DEFINITIONS("-DPREFIX=\"${PREFIX}\"")
ADD_DEFINITIONS("-DRESDIR=\"${RESDIR}\"")
ADD_DEFINITIONS("-DLOCALEDIR=\"${LOCALEDIR}\"")
ADD_DEFINITIONS("-DICONDIR=\"${ICONDIR}\"")
ADD_DEFINITIONS("-DEDJDIR=\"${EDJDIR}\"")
ADD_DEFINITIONS("-DDATADIR=\"${DATADIR}\"")
ADD_DEFINITIONS("-DDATADIR_RW=\"${DATADIR_RW}\"")
ADD_DEFINITIONS("-DSHARED_DIR=\"${SHARED_DIR}\"")
#ADD_DEFINITIONS("-DFILE_DEBUG")
ADD_DEFINITIONS("-D_DLOG_USED")

SET(BUILD_COMMON_SOURCE
	${BUILD_COMMON_SOURCE}
	daemon/accessibility.c
	daemon/common.c
	daemon/datetime/datetime.c
	daemon/datetime/util-time.c
	daemon/dbus_utility.c
	daemon/device/brightness.c
	daemon/handler_controller.c
	daemon/list_util.c
	daemon/media.c
	daemon/minictrl/minictrl.c
	daemon/modules.c
	daemon/notifications/activenoti.c
	daemon/notifications/animated_image.c
	daemon/notifications/noti_box.c
	daemon/notifications/noti.c
	daemon/notifications/noti_gridbox.c
	daemon/notifications/noti_listbox.c
	daemon/notifications/noti_list_item.c
	daemon/notifications/noti_node.c
	daemon/notifications/noti_section.c
	daemon/notifications/noti_util.c
	daemon/notifications/noti_view_boxtype.c
	daemon/notifications/noti_view.c
	daemon/notifications/noti_view_listype.c
	daemon/notifications/noti_view_ongoing.c
	daemon/page/page_base.c
	daemon/page/page_edit.c
	daemon/page/pager.c
	daemon/page/page_setting_all.c
	daemon/preference.c
	daemon/service/animated_icon.c
	daemon/service/configuration.c
	daemon/service/emergency_mode.c
	daemon/service/keyboard.c
	daemon/service/noti_led.c
	daemon/service/reminder.c
	daemon/service/smart_alert.c
	daemon/service/uninstall.c
	daemon/settings/modules/assistive_light.c
	daemon/settings/modules/bluetooth.c
	daemon/settings/modules/flightmode.c
	daemon/settings/modules/gps.c
	daemon/settings/modules/mobile_data.c
	daemon/settings/modules/rotate-lock.c
	daemon/settings/modules/sound-profile.c
	daemon/settings/modules/tethering.c
	daemon/settings/modules/ultra_power_saving.c
	daemon/settings/modules/wifi.c
	daemon/settings/setting_module_api.c
	daemon/settings/settings.c
	daemon/settings/settings_gridbox.c
	daemon/settings/settings_icon_common.c
	daemon/settings/settings_ipc.c
	daemon/settings/settings_view_all.c
	daemon/settings/settings_view_featured.c
	daemon/sim_controller.c
	daemon/vi/vi_manager.c
)

IF(X11_SUPPORT)
ADD_DEFINITIONS("-DHAVE_X")
SET(BUILD_SOURCE
	${BUILD_SOURCE}
	daemon/common_uic_x11.c
	daemon/notifications/noti_win_x11.c
	daemon/page/pager_common_x11.c
	daemon/quickpanel-ui_x11.c
	daemon/service/keyboard_x_x11.c
	daemon/settings/setting_utils_x11.c
)
ENDIF(X11_SUPPORT)

IF(WAYLAND_SUPPORT)
ADD_DEFINITIONS("-DHAVE_WAYLAND")
SET(BUILD_SOURCE
	${BUILD_SOURCE}
	daemon/common_uic.c
	daemon/notifications/noti_win.c
	daemon/page/pager_common.c
	daemon/quickpanel-ui.c
	daemon/service/keyboard_x.c
	daemon/settings/setting_utils.c
)
ENDIF(WAYLAND_SUPPORT)

AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/test TEST_SRCS)

SET(SRCS ${BUILD_COMMON_SOURCE} ${BUILD_SOURCE} ${TEST_SRCS})

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")


ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS} ${extra_pkgs_LDFLAGS} "-pie")
#-lefence

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})
INSTALL(DIRECTORY DESTINATION ${DATADIR})
INSTALL(DIRECTORY DESTINATION ${DATADIR_RW})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/icons/quickpanel_icon_default.png DESTINATION ${RESDIR})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${PKGNAME}.efl DESTINATION /etc/smack/accesses.d)

# icon for setting
INSTALL(FILES ${CMAKE_SOURCE_DIR}/icons/settings_noti_panel.png DESTINATION ${SHARED_DIR}/icons)

# icons for notification
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/data/images_noti_icons/ DESTINATION ${SHARED_DIR}/noti_icons FILES_MATCHING PATTERN "*.png")

# License
INSTALL(FILES ${CMAKE_SOURCE_DIR}/LICENSE DESTINATION /usr/share/license RENAME ${PKGNAME})

# i18n
ADD_SUBDIRECTORY(data)
ADD_SUBDIRECTORY(po)

