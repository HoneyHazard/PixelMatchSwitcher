cmake_minimum_required(VERSION 3.5)

project(pixel-match-switcher)

#set(LIB_NAME "${PROJECT_NAME}-lib")
add_library(${PROJECT_NAME} MODULE)
#add_library(${PROJECT_NAME} SHARED)

# For plugin version info in UI and XML
if(NOT DEFINED PIXEL_MATCH_SWITCHER_VERSION_OVERRIDE)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
        execute_process(COMMAND git describe --always --tags --dirty=-modified
        OUTPUT_VARIABLE PIXEL_MATCH_SWITCHER_VERSION
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
        set(PIXEL_MATCH_SWITCHER_VERSION "${CPACK_PACKAGE_VERSION}")
    endif()
else()
    set(PIXEL_MATCH_SWITCHER_VERSION "${PIXEL_MATCH_SWITCHER_VERSION_OVERRIDE}")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/pm-version.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/pm-version.hpp)
set(VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/pm-version.hpp")

message(STATUS "CMAKE_PROJECT_NAME is ${CMAKE_PROJECT_NAME}")
if(${CMAKE_PROJECT_NAME} STREQUAL "obs-studio")
  if(NOT DEFINED BUILD_OUT_OF_TREE)
    message(STATUS "${PROJECT_NAME} configured for in-tree build")
  endif()
else()
  set(BUILD_OUT_OF_TREE ON)
  message(STATUS "${PROJECT_NAME} configured for out-of-tree build")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

if(BUILD_OUT_OF_TREE)
    if(WIN32)
        if (QTDIR OR DEFINED ENV{QTDIR} OR DEFINED ENV{QTDIR32} OR DEFINED ENV{QTDIR64})
            # Qt path set by user or env var
        else()
            set(QTDIR "" CACHE PATH "Path to Qt (e.g. C:\Qt\6\)")
            message(WARNING "QTDIR variable is missing.  Please set this variable to specify path to Qt (e.g. C:/Qt/6/msvc2015_64)")
        endif()
        if (DepsPath OR DEFINED ENV{DepsPath} OR DEFINED ENV{DepsPath32} OR DEFINED ENV{DepsPath64})
            # Dependencies path set by user or env var
        else()
            set(DepsPath "" CACHE PATH "Path to compiled dependencies (e.g. D:/dependencies/win64)")
            message(WARNING "DepsPath variable is missing.  Please set this variable to specify path to compiled dependencies (e.g. D:/dependencies/win64)")
        endif()

        find_path(CURL_INCLUDE_DIR "${DepsPath}" "/include/curl")
        find_file(CURL_LIB "libcurl.lib" "${DepsPath}/bin")
    else()
        find_package(Libcurl REQUIRED)
    endif()

    find_package(Qt6 COMPONENTS Core Widgets Networking REQUIRED)
    
    find_path(LIBOBS_FRONTEND_INCLUDE_DIR HINTS ${LIBOBS_INCLUDE_DIRS})
    find_file(LIBOBS_FRONTEND_API_LIB NAMES libobs-frontend-api.* HINTS ${LIBOBS_LIB})
    find_path(LIBOBS_UI_DIR HINTS ${LIBSOBS_INCLUDE_DIRS})

    target_include_directories(${PROJECT_NAME} PRIVATE
            "${CURL_INCLUDE_DIR}"
            "${LIBOBS_INCLUDE_DIR}"
            "${LIBOBS_FRONTEND_INCLUDE_DIR}"
            ${Qt6Core_INCLUDES}
            ${Qt6Widgets_INCLUDES}
    )
    if(MSVC)
            include_directories("${W32_PTHREADS_INCLUDE_DIR}")
    endif()
else()
    # for qt-display.hpp:
    set(LIBOBS_UI_DIR "../..")
	
	# for linking and including libobs
	set(LIBOBS_INCLUDE_DIR "../../../libobs")
	
endif()

if(UNIX AND NOT APPLE)
    find_package(Qt6X11Extras REQUIRED)
	target_include_directories(${PROJECT_NAME} PRIVATE
            "${Qt6X11Extras_DIR}"
            "${Qt6X11Extras_INCLUDE_DIRS}"
    )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
	"${LIBOBS_UI_DIR}"
	"${CMAKE_CURRENT_BINARY_DIR}")

# headers
set(pixel-match-switcher_HEADERS
        ${VERSION_FILE}
		src/pm-module.h
		src/pm-filter.h
        src/pm-filter-ref.hpp
        src/pm-core.hpp
        src/pm-add-action-menu.hpp
        src/pm-spoiler-widget.hpp
        src/pm-match-config-widget.hpp
        src/pm-match-reaction-widget.hpp
        src/pm-match-results-widget.hpp
        src/pm-match-list-widget.hpp
        src/pm-preview-config-widget.hpp
        src/pm-preview-display-widget.hpp
        src/pm-presets-widget.hpp
        src/pm-preset-exists-dialog.hpp
        src/pm-presets-retrieval-dialog.hpp
        src/pm-toggles-widget.hpp
        src/pm-image-view.hpp
        src/pm-match-image-dialog.hpp
        src/pm-list-selector-dialog.hpp
        src/pm-help-widget.hpp
        src/pm-dialog.hpp
        src/pm-about-box.hpp
        src/pm-structs.hpp
        src/pm-reaction.hpp
        src/pm-linger-queue.hpp
        src/pm-presets-retriever.hpp
        src/pm-debug-tab.hpp
        ${LIBOBS_UI_DIR}/qt-display.hpp
        ${LIBOBS_UI_DIR}/qt-wrappers.hpp
)

#sources
set(pixel-match-switcher_SOURCES
		src/pm-module.c
		src/pm-filter.c
		src/pm-switcher.c
        src/pm-filter-ref.cpp
        src/pm-core.cpp
        src/pm-spoiler-widget.cpp
        src/pm-add-action-menu.cpp
        src/pm-match-config-widget.cpp
        src/pm-match-results-widget.cpp
        src/pm-match-reaction-widget.cpp
        src/pm-match-list-widget.cpp
        src/pm-preview-config-widget.cpp
        src/pm-preview-display-widget.cpp
        src/pm-presets-widget.cpp
        src/pm-preset-exists-dialog.cpp
        src/pm-presets-retrieval-dialog.cpp
        src/pm-toggles-widget.cpp
        src/pm-image-view.cpp
        src/pm-match-image-dialog.cpp
        src/pm-list-selector-dialog.cpp
        src/pm-help-widget.cpp
        src/pm-debug-tab.cpp
        src/pm-dialog.cpp
        src/pm-about-box.cpp
        src/pm-structs.cpp
        src/pm-reaction.cpp
        src/pm-linger-queue.cpp
        src/pm-presets-retriever.cpp
        ${LIBOBS_UI_DIR}/qt-display.cpp
        ${LIBOBS_UI_DIR}/qt-wrappers.cpp
		pm.qrc
)

#qt6_add_resources(pixel-match-switcher_SOURCES
#        pm.qrc)

#add_library(pixel-match-switcher MODULE
#        ${pixel-match-switcher_HEADERS}
        #${pixel-match-switcher_SOURCES}
#)

target_sources(
  ${PROJECT_NAME} PRIVATE
		${pixel-match-switcher_HEADERS}
		${pixel-match-switcher_SOURCES})

if(BUILD_OUT_OF_TREE)
  include(cmake/ObsPluginHelpers.cmake)
endif()

find_qt(COMPONENTS Widgets Core)
find_package(CURL REQUIRED)
#find_package(Libcurl REQUIRED)
if(CURL_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE "${CURL_INCLUDE_DIRS}")
elseif(Libcurl_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE "${LIBCURL_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "Couldn't find CURL or Libcurl - abort")
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC
		OBS::libobs
        OBS::frontend-api
        Qt::Core
        Qt::Widgets
		CURL::libcurl
        ${LIBCURL_LIBRARIES})
if(UNIX AND NOT APPLE)
        target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::X11Extras)
endif()

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON)

if(BUILD_OUT_OF_TREE)
        if (MSVC)
                target_link_libraries(${PROJECT_NAME} PRIVATE "${W32_PTHREADS_LIB}")
        endif()
        if (UNIX AND NOT APPLE)
            file(GLOB EFFECT_FILES
                data/*.effect)
            install(TARGETS ${PROJECT_NAME}
                LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/obs-plugins)
            install(FILES ${EFFECT_FILES}
                DESTINATION "${CMAKE_INSTALL_PREFIX}/share/obs/obs-plugins/pixel-match-switcher")
        endif()
else()
        #if(MSVC)
        #        target_link_libraries(${PROJECT_NAME} w32-pthreads)
        #endif()
        set_target_properties(pixel-match-switcher PROPERTIES FOLDER "plugins")
        #install_obs_plugin_with_data(pixel-match-switcher data)
endif()

setup_plugin_target(${PROJECT_NAME})

if(BROWSER_AVAILABLE_INTERNAL)
    target_compile_definitions(${PROJECT_NAME} PUBLIC BROWSER_AVAILABLE=1)
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC BROWSER_AVAILABLE=0)
endif()

target_compile_options(${PROJECT_NAME} PUBLIC
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>,$<C_COMPILER_ID:GNU>>:
          -Wno-old-style-cast>
     $<$<CXX_COMPILER_ID:MSVC>:
          /wd26812>)



#file(GLOB ASS_TRANSLATION_FILES
#  data/locale/*.ini)

#install_external_plugin_with_data(pixel-match-switcher "${CMAKE_INSTALL_PREFIX}/share/obs/obs-plugins/")

# Linux
#if(UNIX AND NOT APPLE)
#    set(CMAKE_C_FLAGS "-Wall -Wextra -Wvla -Wno-unused-function -Werror-implicit-function-declaration -Wno-missing-braces -Wno-missing-field-initializers ${CMAKE_C_FLAGS} -std=c99 -fno-strict-aliasing")
#
#    if(ARCH EQUAL 64)
#        set(ARCH_NAME "x86_64")
#    else()
#        set(ARCH_NAME "i686")
#    endif()
#
#    set_target_properties(pixel-match-switcher PROPERTIES PREFIX "")
#
#    install(TARGETS pixel-match-switcher
#            LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/obs-plugins)
#    install(FILES ${ASS_TRANSLATION_FILES}
#            DESTINATION "${CMAKE_INSTALL_PREFIX}/share/obs/obs-plugins/pixel-match-switcher/locale")
#endif()
