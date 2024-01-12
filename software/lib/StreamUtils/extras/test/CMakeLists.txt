# StreamUtils - github.com/bblanchon/ArduinoStreamUtils
# Copyright Benoit Blanchon 2019-2023
# MIT License

if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
	add_compile_options(-Wall -Wextra -Werror -Wundef)
	add_link_options(-static)
endif()

file(GLOB SOURCES CONFIGURE_DEPENDS *.cpp)

add_subdirectory(doctest)

function(add_streamutils_test EXECUTABLE CORE)
	add_executable(${EXECUTABLE} ${SOURCES})
	target_link_libraries(${EXECUTABLE} ${CORE} doctest)
	add_test(${EXECUTABLE} ${EXECUTABLE})
endfunction()

add_subdirectory(cores/avr)
add_subdirectory(cores/esp32)
add_subdirectory(cores/esp8266)
add_subdirectory(cores/mbed)
add_subdirectory(cores/nrf52)
add_subdirectory(cores/rp2040)
add_subdirectory(cores/samd)
add_subdirectory(cores/stm32)
add_subdirectory(cores/stm32f1)
add_subdirectory(cores/stm32f4)
add_subdirectory(cores/teensy)