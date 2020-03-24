cmake_minimum_required(VERSION 3.5)

project(pixel-match-switcher)

set(CMAKE_PREFIX_PATH "${QTDIR}")
#set(CMAKE_INCLUDE_CURRENT_DIR ON) ## do we need this?
#list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/external")
set(CMAKE_AUTOMOC ON)

include(external/FindLibObs.cmake)
include(external/ObsPluginHelpers.cmake)

find_package(LibObs REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
if(UNIX)
  find_package(Qt5X11Extras REQUIRED)
endif()

find_path(LIBOBS_FRONTEND_INCLUDE_DIR HINTS ${LIBOBS_INCLUDE_DIRS})
find_file(LIBOBS_FRONTEND_API_LIB NAMES libobs-frontend-api.* HINTS ${LIBOBS_LIB})
find_path(LIBOBS_UI_DIR HINTS ${LIBSOBS_INCLUDE_DIRS})

set(pixel-match-switcher_HEADERS
        src/pixel-match-filter.h
        src/pixel-match-filter-ref.hpp
        src/pixel-match-core.hpp
        src/pixel-match-matching-tab.hpp
        src/pixel-match-dialog.hpp
        src/pixel-match-structs.hpp
        src/pixel-match-debug-tab.hpp
        ${LIBOBS_UI_DIR}/qt-display.hpp
        ${LIBOBS_UI_DIR}/qt-wrappers.hpp
)

set(pixel-match-switcher_SOURCES
        src/pixel-match-switcher.c
        src/pixel-match-filter.c
        src/pixel-match-filter-ref.cpp
        src/pixel-match-core.cpp
        src/pixel-match-matching-tab.cpp
        src/pixel-match-dialog.cpp
        src/pixel-match-debug-tab.cpp
        ${LIBOBS_UI_DIR}/qt-display.cpp
        ${LIBOBS_UI_DIR}/qt-wrappers.cpp        
)

add_library(pixel-match-switcher MODULE
        ${pixel-match-switcher_HEADERS}
        ${pixel-match-switcher_SOURCES}
)

include_directories(
        "${LIBOBS_INCLUDE_DIR}"
        "${LIBOBS_FRONTEND_INCLUDE_DIR}"
        "${LIBOBS_UI_DIR}"
        ${Qt5Core_INCLUDES}
        ${Qt5Widgets_INCLUDES}
)

if(UNIX)
include_directories(       
        "${Qt5X11Extras_DIR}"
        ${Qt5X11Extras_INCLUDE_DIRS}
)# TODO conditional
endif()


target_link_libraries(pixel-match-switcher
        ${LIBOBS_LIB}
        ${LIBOBS_FRONTEND_API_LIB}
#        ${LIBOBS_GLAD_LIB}
        Qt5::Core
        Qt5::Widgets)

      
#file(GLOB ASS_TRANSLATION_FILES
#  data/locale/*.ini)

file(GLOB EFFECT_FILES
   data/*.effect)

if(UNIX)
       set(CXX_COMMON_FLAGS "${CXX_COMMON_FLAGS} -Wno-old-style-cast")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-old-style-cast")
endif()

install(TARGETS pixel-match-switcher
	LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/obs-plugins)
install(FILES ${EFFECT_FILES}
	DESTINATION "${CMAKE_INSTALL_PREFIX}/share/obs/obs-plugins/pixel-match-switcher")

#install_external_plugin_with_data(pixel-match-switcher "${CMAKE_INSTALL_PREFIX}/share/obs/obs-plugins/")


# Linux
#if(UNIX AND NOT APPLE)
#	set(CMAKE_C_FLAGS "-Wall -Wextra -Wvla -Wno-unused-function -Werror-implicit-function-declaration -Wno-missing-braces -Wno-missing-field-initializers ${CMAKE_C_FLAGS} -std=c99 -fno-strict-aliasing")
#
#	if(ARCH EQUAL 64)
#		set(ARCH_NAME "x86_64")
#	else()
#		set(ARCH_NAME "i686")
#	endif()
#
#	set_target_properties(pixel-match-switcher PROPERTIES PREFIX "")
#
#	install(TARGETS pixel-match-switcher
#			LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/obs-plugins)
#	install(FILES ${ASS_TRANSLATION_FILES}
#			DESTINATION "${CMAKE_INSTALL_PREFIX}/share/obs/obs-plugins/pixel-match-switcher/locale")
#endif()
