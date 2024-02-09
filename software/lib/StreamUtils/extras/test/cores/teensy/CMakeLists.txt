# StreamUtils - github.com/bblanchon/ArduinoStreamUtils
# Copyright Benoit Blanchon 2019-2023
# MIT License

add_library(TeensyCore
	EEPROM.cpp
	EEPROM.h
)

target_include_directories(TeensyCore
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../avr
)

target_compile_definitions(TeensyCore
	PUBLIC
		CORE_TEENSY
)

add_streamutils_test(StreamUtilsTestTeensy TeensyCore)