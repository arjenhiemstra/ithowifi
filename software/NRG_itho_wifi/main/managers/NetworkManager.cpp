#include "NetworkManager.h"

// Global instance
NetworkManager networkManager;

NetworkManager::NetworkManager()
{
  // Default constructor - clients are initialized by their own constructors
}

void NetworkManager::initialize()
{
  // Configure secure client to skip CA verification
  // This is set once during system initialization
  secureClient.setInsecure();
}
