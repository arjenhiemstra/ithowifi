# StreamUtils - github.com/bblanchon/ArduinoStreamUtils
# Copyright Benoit Blanchon 2019-2023
# MIT License

add_library(MbedRp2040Core INTERFACE)

target_include_directories(MbedRp2040Core
	INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../coreapi
)

target_compile_definitions(MbedRp2040Core
	INTERFACE
		ARDUINO_ARCH_RP2040
		ARDUINO_RASPBERRY_PI_PICO
		ARDUINO_ARCH_MBED_RP2040
		ARDUINO_ARCH_MBED
)

add_streamutils_test(StreamUtilsTestMbedRp2040 MbedRp2040Core)
