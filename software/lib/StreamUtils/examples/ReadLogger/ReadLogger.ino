// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License
//
// This example shows how to log what read from a Stream

#include <StreamUtils.h>

// Create a new stream that will forward all calls to Serial, and log to Serial.
// It will write back everything it reads, just like "echo"
ReadLoggingStream loggingStream(Serial, Serial);

void setup() {
  // Initialize serial port
  Serial.begin(9600);
  while (!Serial)
    continue;
}

void loop() {
  // Read the serial port.
  // Because loggingStream write everything it read, the program will show what
  // you sent.
  while (Serial.available()) {
    loggingStream.write(Serial.read());
  }
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