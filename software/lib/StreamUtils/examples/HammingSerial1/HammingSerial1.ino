// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License
//
// This example shows how to use Hamming codes for error correction
//
// To run this program, you need two boards that support Serial1 with the same
// voltage. For example: Arduino Mega, Leonardo, or Nano Every (all 5V boards)
//
// Connect the TX pin of one board to the RX pin of the other and vice-versa.

#include <StreamUtils.h>

HammingStream<7, 4> eccSerial1(Serial1);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  Serial1.begin(9600);
  while (!Serial)
    continue;

  // Discard any remaining data in the input buffer
  while (Serial1.available())
    Serial1.read();
}

void loop() {
  // Did we receive something?
  if (eccSerial1.available())
    // Print it
    Serial.write(eccSerial1.read());

  // Do we have something to send?
  if (Serial.available())
    // Sent it
    eccSerial1.write(Serial.read());
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