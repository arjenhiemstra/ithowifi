// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License
//
// This example shows how to read from EEPROM

#include <StreamUtils.h>

void setup() {
  // Initialize serial port
  Serial.begin(9600);
  while (!Serial)
    continue;

#if STREAMUTILS_ENABLE_EEPROM

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  Serial.println("Initializing EEPROM...");
  EEPROM.begin(512);
#endif

  Serial.println("Make sure to run the EepromWrite example first!");

  Serial.println("Reading EEPROM... ");
  EepromStream s(0, 12);
  Serial.print(s.readString());

#else
  Serial.println("EepromStream is not supported on this platform. Sorry");
#endif
}

void loop() {
  // not used in this example
}

/*****************************************************
 *                                                   *
 *  Love this project?                               *
 *               Star it on GitHub!                  *
 *                                                   *
 *                       .,,.                        *
 *                       ,,:1.                       *
 *                      ,.,:;1                       *
 *                     .,,,::;:                      *
 *                     ,,,,::;;.                     *
 *                    .,,,:::;;;                     *
 *       .....,,,,...,.,,,,,,:::,,,,,,,,,,,,,        *
 *      ,,,,,,,,,,,:,...,,,,,,:::,,,,:::;;;11l       *
 *       .;::::::::,,,,,,,,,,:::::,,::;;;1lt         *
 *          .;;;:::,,,,,,,,::::::;:::;;1t:           *
 *             :;;:,,,,,,::::::;;;;;;l1              *
 *                ,,,,:::::::;;;;;;l                 *
 *               .,,,,::::;;;;;;;::::                *
 *               ,,,,,:::;;;;;::,:::1                *
 *              ,,,,,::;;;t1:,,:::::;l               *
 *             .,,,,:;;ll    ;::::::;;,              *
 *             ,,,:;ll.        .1:::;;l              *
 *            .,:lt,              .1;;l:             *
 *                                                   *
 *  https://github.com/bblanchon/ArduinoStreamUtils  *
 *                                                   *
 *****************************************************/