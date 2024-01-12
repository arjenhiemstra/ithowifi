# StreamUtils - github.com/bblanchon/ArduinoStreamUtils
# Copyright Benoit Blanchon 2019-2023
# MIT License

add_library(Esp8266Core
	EEPROM.cpp
	EEPROM.h
	Stream.h
)

target_include_directories(Esp8266Core
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../avr
)

target_compile_definitions(Esp8266Core
	PUBLIC
		ARDUINO_ARCH_ESP8266
)

add_streamutils_test(StreamUtilsTestEsp8266 Esp8266Core)
