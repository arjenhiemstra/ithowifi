# StreamUtils - github.com/bblanchon/ArduinoStreamUtils
# Copyright Benoit Blanchon 2019-2023
# MIT License

add_library(Stm32F1Core INTERFACE)

target_include_directories(Stm32F1Core
	INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../avr
)

target_compile_definitions(Stm32F1Core
	INTERFACE
		ARDUINO_ARCH_STM32F1
)

add_streamutils_test(StreamUtilsTestStm32F1 Stm32F1Core)
