// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License
//
// This example shows how to use Hamming codes for error correction
//
// To run this program, you need two boards connected like so:
//   BOARD 1 - BOARD 2
//   pin 10  - pin 11
//   pin 11  - pin 10
//
// SoftwareSerial is here for demonstration purposes; please don't use it in
// production, as it is notoriously unreliable.

#include <SoftwareSerial.h>
#include <StreamUtils.h>

SoftwareSerial linkSerial(10, 11);  // RX, TX
HammingStream<7, 4> eccLinkSerial(linkSerial);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  linkSerial.begin(4800);
}

void loop() {
  // Did we receive something?
  if (eccLinkSerial.available())
    // Print it
    Serial.write(eccLinkSerial.read());

  // Do we have something to send?
  if (Serial.available())
    // Sent it
    eccLinkSerial.write(Serial.read());
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