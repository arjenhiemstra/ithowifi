# StreamUtils - github.com/bblanchon/ArduinoStreamUtils
# Copyright Benoit Blanchon 2019-2023
# MIT License

add_library(SamdCore INTERFACE)

target_include_directories(SamdCore
	INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../avr
)

target_compile_definitions(SamdCore
	INTERFACE
		ARDUINO_ARCH_SAMD
)

add_streamutils_test(StreamUtilsTestSamd SamdCore)
