# StreamUtils - github.com/bblanchon/ArduinoStreamUtils
# Copyright Benoit Blanchon 2019-2023
# MIT License

add_library(AvrCore
	Client.h
	EEPROM.cpp
	EEPROM.h
	Print.h
	Stream.h
)

target_include_directories(AvrCore
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(AvrCore
	PUBLIC
		ARDUINO_ARCH_AVR
)

add_streamutils_test(StreamUtilsTestAvr AvrCore)
