// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License
//
// This example shows how to buffer writes to a stream.
//
// Like "echo," it reads from the serial port and prints back the same thing.
// What's interesting with this program is that it writes in chunks of
// 8 bytes, even if it seems to write them one by one.
//
// As you'll see, you need to type at least 8 bytes to see something. That's
// because it waits for the buffered to be full before sending it to the serial.

#include <StreamUtils.h>

// Create a new Stream that buffers all writes to Serial
WriteBufferingStream bufferedSerial{Serial, 8};

void setup() {
  // Initialize serial port
  Serial.begin(9600);
  while (!Serial)
    continue;

  Serial.println(F("Send at least 8 bytes to see the result"));
}

void loop() {
  // Even if it looks like the bytes are sent one by one, they are actual
  // written in chunks of 8 bytes.
  while (Serial.available()) {
    bufferedSerial.write(Serial.read());
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