/*
 * Author: Klusjesman, modified bij supersjimmie for Arduino/ESP8266
 */

#pragma once

#include <cstdio>
#include <Arduino.h>

#define CC1101_BUFFER_LEN 64
#define CC1101_DATA_LEN CC1101_BUFFER_LEN - 3
#define MAX_RAW 162

class CC1101Packet
{
public:
	uint8_t length = 0;
	uint8_t data[MAX_RAW]{};
	uint8_t rssi = 0; // raw CC1101 RSSI / LQI / FREQEST status registers, latched as the packet is read (see RFSignal.h)
	uint8_t lqi = 0;
	uint8_t freqest = 0;
};
