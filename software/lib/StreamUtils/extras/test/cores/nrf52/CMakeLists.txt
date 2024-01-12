# StreamUtils - github.com/bblanchon/ArduinoStreamUtils
# Copyright Benoit Blanchon 2019-2023
# MIT License

add_library(Nrf52Core INTERFACE)

target_include_directories(Nrf52Core
	INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../avr
)

target_compile_definitions(Nrf52Core
	INTERFACE
		ARDUINO_ARCH_NRF52
)

add_streamutils_test(StreamUtilsTestNrf52 Nrf52Core)
