cmake_minimum_required(VERSION 3.5)

project(pixel-match-switcher)

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

set(pixel-match-switcher_HEADERS
        src/pixel-match-filter.h
        src/pixel-match-core.hpp
        src/pixel-match-dialog.hpp
        src/pixel-match-debug-tab.hpp
)

set(pixel-match-switcher_SOURCES
        src/pixel-match-switcher.c
        src/pixel-match-filter.c
        src/pixel-match-core.cpp
        src/pixel-match-dialog.cpp
        src/pixel-match-debug-tab.cpp
)

set(pixel-match-switcher_UI
  # TODO: .ui file
)
      
add_library(pixel-match-switcher MODULE
        ${pixel-match-switcher_HEADERS}
        ${pixel-match-switcher_SOURCES}
        ${pixel-match-switcher_UI})

include_directories(
        "${LIBOBS_INCLUDE_DIRS}"
        "${LIBOBS_FRONTEND_INCLUDE_DIR}"
        ${Qt5Core_INCLUDES}
        ${Qt5Widgets_INCLUDES})

target_link_libraries(pixel-match-switcher
        ${LIBOBS_LIB}
        ${LIBOBS_FRONTEND_API_LIB}
        Qt5::Core
        Qt5::Widgets)

# TODO: installation
      
#file(GLOB ASS_TRANSLATION_FILES
#  "data/locale/*.ini")

#install_obs_plugin_with_data(pixel-match-switcher data)

if(UNIX)
       set(CXX_COMMON_FLAGS "${CXX_COMMON_FLAGS} -Wno-old-style-cast")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-old-style-cast")
endif()

# Linux
if(UNIX AND NOT APPLE)
	set(CMAKE_C_FLAGS "-Wall -Wextra -Wvla -Wno-unused-function -Werror-implicit-function-declaration -Wno-missing-braces -Wno-missing-field-initializers ${CMAKE_C_FLAGS} -std=c99 -fno-strict-aliasing")

	if(ARCH EQUAL 64)
		set(ARCH_NAME "x86_64")
	else()
		set(ARCH_NAME "i686")
	endif()

	set_target_properties(pixel-match-switcher PROPERTIES PREFIX "")

	install(TARGETS pixel-match-switcher
			LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/obs-plugins)
#	install(FILES ${ASS_TRANSLATION_FILES}
#			DESTINATION "${CMAKE_INSTALL_PREFIX}/share/obs/obs-plugins/pixel-match-switcher/locale")
endif()
