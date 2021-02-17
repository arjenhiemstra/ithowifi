/*
 * Author: Klusjesman, modified bij supersjimmie for Arduino/ESP8266
 */

#ifndef CC1101PACKET_H_
#define CC1101PACKET_H_


#include <stdio.h>
#include "hardware.h"

#if defined (__HW_VERSION_ONE__) || defined (__HW_VERSION_TWO__)
#include <Arduino.h>
#endif

#define CC1101_BUFFER_LEN        64
#define CC1101_DATA_LEN          CC1101_BUFFER_LEN - 3


class CC1101Packet
{
  public:
    uint8_t length;
    uint8_t data[128];
};


#endif /* CC1101PACKET_H_ */
