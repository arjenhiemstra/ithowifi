# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.*)
idf_component_register(SRCS ${app_sources})

#IDF >v5.x prepare
# file(GLOB_RECURSE ESPAS_CPP ../components/arduino/libraries/ESPAsyncWebServer/src/*.cpp)
# file(GLOB_RECURSE ASYNCTCP_CPP ../components/arduino/libraries/AsyncTCP/src/*.cpp)


# idf_component_register(
#     SRCS 
#         ${app_sources}
#         "../components/arduino/libraries/Syslog/src/Syslog.cpp"
#         "../components/arduino/libraries/ArduinoNvs/src/ArduinoNvs.cpp"
#         "../components/arduino/libraries/pubsubclient/src/PubSubClient.cpp"
#         "../components/arduino/libraries/FS_FilePrint/FSFilePrint.cpp"
#         "../components/arduino/libraries/Arduino-Log/ArduinoLog.cpp"
#         "../components/arduino/libraries/MycilaWebSerial/src/MycilaWebSerial.cpp"
#         ${ESPAS_CPP}
#         ${ASYNCTCP_CPP}
#     INCLUDE_DIRS "."
#     PRIV_INCLUDE_DIRS 
#         "../components/arduino/libraries/ArduinoJson/src"
#         "../components/arduino/libraries/ArduinoStreamUtils/src"
#         "../components/arduino/libraries/pubsubclient/src"
#         "../components/arduino/libraries/ESPAsyncWebServer/src"
#         "../components/arduino/libraries/AsyncTCP/src"
#         "../components/arduino/libraries/Arduino-Log"
#         "../components/arduino/libraries/FS_FilePrint"
#         "../components/arduino/libraries/Syslog/src"
#         "../components/arduino/libraries/MycilaWebSerial/src"
#         "../components/arduino/libraries/ArduinoNvs/src"
# )