# StreamUtils - github.com/bblanchon/ArduinoStreamUtils
# Copyright Benoit Blanchon 2019-2023
# MIT License

add_library(Rp2040Core
	EEPROM.cpp
	EEPROM.h
)

target_include_directories(Rp2040Core
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../coreapi
)

target_compile_definitions(Rp2040Core
	PUBLIC
		ARDUINO_ARCH_RP2040
		ARDUINO_PICO_MAJOR=3
)

add_streamutils_test(StreamUtilsTestRp2040 Rp2040Core)
