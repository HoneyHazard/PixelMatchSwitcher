cmake_minimum_required(VERSION 3.5)

project(content-activated-switcher)

set(CMAKE_PREFIX_PATH "${QTDIR}")
set(CMAKE_INCLUDE_CURRENT_DIR ON) ## do we need this?
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/external")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC_ON)

include(external/FindLibObs.cmake)
find_package(LibObs REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_path(LIBOBS_FRONTEND_INCLUDE_DIR HINTS ${LIBOBS_INCLUDE_DIRS})
find_file(LIBOBS_FRONTEND_API_LIB NAMES libobs-frontend-api.* HINTS ${LIBOBS_LIB})

set(content-activated-switcher_HEADERS
  # TODO: headers
  )

set(content-activated-switcher_SOURCES
        src/cas-filter.c
        src/cas-plugin.c
)

set(content-activated-switcher_UI
  # TODO: .ui file
)
      
add_library(content-activated-switcher MODULE
        ${content-activated-switcher_HEADERS}
        ${content-activated-switcher_SOURCES}
        ${content-activated-switcher_UI})

include_directories(
        "${LIBOBS_INCLUDE_DIRS}"
        "${LIBOBS_FRONTEND_INCLUDE_DIR}"
        ${Qt5Core_INCLUDES}
        ${Qt5Widgets_INCLUDES})

target_link_libraries(content-activated-switcher
        ${LIBOBS_LIB}
        ${LIBOBS_FRONTEND_API_LIB}
        Qt5::Core
        Qt5::Widgets)

# TODO: installation
      
#file(GLOB ASS_TRANSLATION_FILES
#  "data/locale/*.ini")

#install_obs_plugin_with_data(content-activated-switcher data)

# Linux
if(UNIX AND NOT APPLE)
	set(CMAKE_C_FLAGS "-Wall -Wextra -Wvla -Wno-unused-function -Werror-implicit-function-declaration -Wno-missing-braces -Wno-missing-field-initializers ${CMAKE_C_FLAGS} -std=c99 -fno-strict-aliasing")

	if(ARCH EQUAL 64)
		set(ARCH_NAME "x86_64")
	else()
		set(ARCH_NAME "i686")
	endif()

	set_target_properties(content-activated-switcher PROPERTIES PREFIX "")

	install(TARGETS content-activated-switcher
			LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/obs-plugins)
#	install(FILES ${ASS_TRANSLATION_FILES}
#			DESTINATION "${CMAKE_INSTALL_PREFIX}/share/obs/obs-plugins/content-activated-switcher/locale")
endif()
