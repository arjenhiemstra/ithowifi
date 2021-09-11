/*
 * Author: Klusjesman, modified bij supersjimmie for Arduino/ESP8266
 */

#pragma once

#include <stdio.h>
#include <Arduino.h>

#define CC1101_BUFFER_LEN        64
#define CC1101_DATA_LEN          CC1101_BUFFER_LEN - 3
#define MAX_RAW                  162

class CC1101Packet
{
	public:
		uint8_t length;
		uint8_t data[MAX_RAW];
};
