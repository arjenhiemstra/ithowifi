cmake_minimum_required(VERSION 3.16)

#set(EXTRA_COMPONENT_DIRS components/arduino)
set(IDF_SKIP_GIT_CHECKS ON CACHE BOOL "Skip ESP-IDF Git probing" FORCE)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

get_filename_component(configName "${CMAKE_BINARY_DIR}" NAME)
list(APPEND EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/.pio/libdeps/${configName}/esp_littlefs")

project(NRG_itho_wifi)

