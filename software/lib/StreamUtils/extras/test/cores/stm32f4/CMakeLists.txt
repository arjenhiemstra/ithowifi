# StreamUtils - github.com/bblanchon/ArduinoStreamUtils
# Copyright Benoit Blanchon 2019-2023
# MIT License

add_library(Stm32F4Core INTERFACE)

target_include_directories(Stm32F4Core
	INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../stm32f1
		${CMAKE_CURRENT_SOURCE_DIR}/../avr
)

target_compile_definitions(Stm32F4Core
	INTERFACE
		ARDUINO_ARCH_STM32F4
)

add_streamutils_test(SteamUtilsTestStm32F4 Stm32F4Core)
