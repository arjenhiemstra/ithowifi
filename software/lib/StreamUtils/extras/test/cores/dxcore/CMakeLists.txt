# StreamUtils - github.com/bblanchon/ArduinoStreamUtils
# Copyright Benoit Blanchon 2019-2023
# MIT License

add_library(DxCore
	EEPROM.cpp
	EEPROM.h
)

target_include_directories(DxCore
	INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../avr
)

target_compile_definitions(DxCore
	PUBLIC
		ARDUINO_avrda
		ARDUINO_ARCH_MEGAAVR
		DXCORE=\"$\"
		DXCORE_MAJOR=1UL
		DXCORE_MINOR=5UL
		DXCORE_PATCH=11UL
		DXCORE_RELEASED=1
)

add_streamutils_test(StreamUtilsTestDxCore DxCore)
