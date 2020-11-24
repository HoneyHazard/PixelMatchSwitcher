cmake_minimum_required(VERSION 3.5)

project(pixel-match-switcher)

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


if(BUILD_OUT_OF_TREE)
    if(WIN32)
        if (QTDIR OR DEFINED ENV{QTDIR} OR DEFINED ENV{QTDIR32} OR DEFINED ENV{QTDIR64})
            # Qt path set by user or env var
        else()
            set(QTDIR "" CACHE PATH "Path to Qt (e.g. C:\Qt\5.10.1\msvc2017_64)")
            message(WARNING "QTDIR variable is missing.  Please set this variable to specify path to Qt (e.g. C:/Qt/5.7/msvc2015_64)")
        endif()
        if (DepsPath OR DEFINED ENV{DepsPath} OR DEFINED ENV{DepsPath32} OR DEFINED ENV{DepsPath64})
            # Dependencies path set by user or env var
        else()
            set(DepsPath "" CACHE PATH "Path to compiled dependencies (e.g. D:/dependencies/win64)")
            message(WARNING "DepsPath variable is missing.  Please set this variable to specify path to compiled dependencies (e.g. D:/dependencies/win64)")
        endif()
    else()
        set(CMAKE_PREFIX_PATH "${QTDIR}")
    endif()

    set(CMAKE_AUTOMOC ON)

    include(external/FindLibObs.cmake)
    include(external/ObsPluginHelpers.cmake)

    find_package(LibObs REQUIRED)
    find_package(Qt5Core REQUIRED)
    find_package(Qt5Widgets REQUIRED)

    find_path(LIBOBS_FRONTEND_INCLUDE_DIR HINTS ${LIBOBS_INCLUDE_DIRS})
    find_file(LIBOBS_FRONTEND_API_LIB NAMES libobs-frontend-api.* HINTS ${LIBOBS_LIB})
    find_path(LIBOBS_UI_DIR HINTS ${LIBSOBS_INCLUDE_DIRS})

    include_directories(
            "${LIBOBS_INCLUDE_DIR}"
            "${LIBOBS_FRONTEND_INCLUDE_DIR}"
            ${Qt5Core_INCLUDES}
            ${Qt5Widgets_INCLUDES}
    )
    if(UNIX AND NOT APPLE)
            find_package(Qt5X11Extras REQUIRED)
            include_directories(
                    "${Qt5X11Extras_DIR}"
                    "${Qt5X11Extras_INCLUDE_DIRS}"
            )
    endif()

    if(MSVC)
            include_directories("${W32_PTHREADS_INCLUDE_DIR}")
    endif()
else()
    # for qt-display.hpp:
    set(LIBOBS_UI_DIR "../..")
endif()

include_directories("${LIBOBS_UI_DIR}")

set(pixel-match-switcher_HEADERS
        ${VERSION_FILE}
        src/pm-filter.h
        src/pm-filter-ref.hpp
        src/pm-core.hpp
        src/pm-match-config-widget.hpp
        src/pm-match-results-widget.hpp
        src/pm-match-list-widget.hpp
        src/pm-preview-config-widget.hpp
        src/pm-preview-display-widget.hpp
        src/pm-presets-widget.hpp
        src/pm-preset-exists-dialog.hpp
        src/pm-presets-selector-dialog.hpp
        src/pm-toggles-widget.hpp
        src/pm-image-view.hpp
        src/pm-match-image-dialog.hpp
        src/pm-help-widget.hpp
        src/pm-dialog.hpp
        src/pm-about-box.hpp
        src/pm-structs.hpp
        src/pm-linger-queue.hpp
        src/pm-presets-retriever.hpp
        src/pm-debug-tab.hpp
        ${LIBOBS_UI_DIR}/qt-display.hpp
        ${LIBOBS_UI_DIR}/qt-wrappers.hpp
)

set(pixel-match-switcher_SOURCES
        src/pm-switcher.c
        src/pm-filter.c
        src/pm-filter-ref.cpp
        src/pm-core.cpp
        src/pm-match-config-widget.cpp
        src/pm-match-results-widget.cpp
        src/pm-match-list-widget.cpp
        src/pm-preview-config-widget.cpp
        src/pm-preview-display-widget.cpp
        src/pm-presets-widget.cpp
        src/pm-preset-exists-dialog.cpp
        src/pm-presets-selector-dialog.cpp
        src/pm-toggles-widget.cpp
        src/pm-image-view.cpp
        src/pm-match-image-dialog.cpp
        src/pm-help-widget.cpp
        src/pm-debug-tab.cpp
        src/pm-dialog.cpp
        src/pm-about-box.cpp
        src/pm-structs.cpp
        src/pm-linger-queue.cpp
        src/pm-presets-retriever.cpp
        ${LIBOBS_UI_DIR}/qt-display.cpp
        ${LIBOBS_UI_DIR}/qt-wrappers.cpp
)

qt5_add_resources(pixel-match-switcher_SOURCES
        pm.qrc)

add_library(pixel-match-switcher MODULE
        ${pixel-match-switcher_HEADERS}
        ${pixel-match-switcher_SOURCES}
)

target_link_libraries(pixel-match-switcher
        Qt5::Core
        Qt5::Widgets
        ${LIBCURL_LIBRARIES})
if(UNIX AND NOT APPLE)
        target_link_libraries(pixel-match-switcher Qt5::X11Extras)
        #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-old-style-cast")
        #set(CXX_COMMON_FLAGS "${CXX_COMMON_FLAGS} -Wno-old-style-cast")
endif()

if(BUILD_OUT_OF_TREE)
        target_link_libraries(pixel-match-switcher
                ${LIBOBS_LIB}
                ${LIBOBS_FRONTEND_API_LIB}
        )
        if (MSVC)
                target_link_libraries(pixel-match-switcher "${W32_PTHREADS_LIB}")
        endif()
        if (UNIX AND NOT APPLE)
            file(GLOB EFFECT_FILES
                data/*.effect)
            install(TARGETS pixel-match-switcher
                LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/obs-plugins)
            install(FILES ${EFFECT_FILES}
                DESTINATION "${CMAKE_INSTALL_PREFIX}/share/obs/obs-plugins/pixel-match-switcher")
        endif()
else()
        target_link_libraries(pixel-match-switcher
                libobs
                obs-frontend-api
        )
        if(MSVC)
                target_link_libraries(pixel-match-switcher w32-pthreads)
        endif()
        set_target_properties(pixel-match-switcher PROPERTIES FOLDER "plugins")
        install_obs_plugin_with_data(pixel-match-switcher data)
endif()

target_compile_options(pixel-match-switcher PUBLIC
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
