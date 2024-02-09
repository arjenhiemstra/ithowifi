# StreamUtils - github.com/bblanchon/ArduinoStreamUtils
# Copyright Benoit Blanchon 2019-2023
# MIT License

add_library(Stm32Core
	EEPROM.cpp
	EEPROM.h
	Stream.h
)

target_include_directories(Stm32Core
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../avr
)

target_compile_definitions(Stm32Core
	PUBLIC
		ARDUINO_ARCH_STM32
		STM32_CORE_VERSION_MAJOR=2
		STM32_CORE_VERSION_MINOR=4
		STM32_CORE_VERSION_PATCH=0
)

add_streamutils_test(StreamUtilsTestStm32 Stm32Core)
