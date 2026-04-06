#pragma once

#include "cc1101/IthoCC1101.h"
#include "cc1101/IthoPacket.h"

class RFManager
{
public:
  // RF communication objects
  IthoCC1101 radio;
  IthoPacket packet;

  RFManager();

  // Convenience accessors
  IthoCC1101 &getRadio() { return radio; }
  IthoPacket &getPacket() { return packet; }
};

extern RFManager rfManager;
