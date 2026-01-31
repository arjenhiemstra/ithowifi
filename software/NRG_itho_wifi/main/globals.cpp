#include "globals.h"

// Hardware globals now managed by HardwareManager (see managers/HardwareManager.cpp)
// Removed: i2c_sniffer_capable, hardware_rev_det, gpio pins, hardware strings

// Network clients now managed by NetworkManager (see managers/NetworkManager.cpp)
// Removed: WiFiClientSecure client, WiFiClient defaultclient

// RF communication objects now managed by RFManager (see managers/RFManager.cpp)
// Removed: IthoCC1101 rf, IthoPacket packet

// All major hardware/communication globals are now managed by manager classes (Phases 1-3 complete)
