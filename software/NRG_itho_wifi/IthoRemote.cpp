#include "IthoRemote.h"

#include <string.h>
#include <Arduino.h>


// default constructor
IthoRemote::IthoRemote() {
  strlcpy(config_struct_version, REMOTE_CONFIG_VERSION, sizeof(config_struct_version));

  for (uint8_t i = 0; i < MAX_NUMBER_OF_REMOTES; i++) {
    strlcpy(remotes[i].name, "remote", sizeof(remotes[i].name));
  }

  configLoaded = false;

} //IthoRemote

// default destructor
IthoRemote::~IthoRemote() {
} //~IthoRemote


int IthoRemote::getRemoteCount() {
  return this->remoteCount;
}

bool IthoRemote::toggleLearnLeaveMode() {
  llMode = llMode ? false : true;
  if (llMode) {
    llModeTime = 120;
  }
  return llMode;
};

void IthoRemote::updatellModeTimer() {
  if (llModeTime > 0) {
    llModeTime--;
  }
}

//void IthoRemote::toggleLearnLeaveMode() {
//  llMode = llMode ? false : true;
//  if (llMode) {
//    llModeTime = 120;
//    timer.attach(1, +[](IthoRemote * remoteInstance) {
//      remoteInstance->updatellModeTimer();
//    }, this);
//  }
//  else {
//    llModeTime = 0;
//    timer.detach();
//  }
//}
//
//void IthoRemote::updatellModeTimer() {
//  if (llModeTime > 0) {
//    llModeTime--;
//  }
//  if (llModeTime == 0) {
//    timer.detach();
//  }
//  llModeTimerUpdated = true;
//}

int IthoRemote::registerNewRemote(int* id) {
  if (this->checkID(id)) {
    return -1; //remote already registered
  }
  if (this->remoteCount >= MAX_NUMBER_OF_REMOTES - 1) {
    return -2;
  }
  int zeroID[] = {0, 0, 0};
  int index = this->remoteIndex(zeroID); //find first index with no remote ID set
  if (index < 0) {
    return index;
  }
  for (uint8_t i = 0; i < 3; i++) {
    remotes[index].ID[i] = id[i];
  }

  this->remoteCount++;

  return 1;
}

int IthoRemote::removeRemote(int* id) {
  if (!this->checkID(id)) {
    return -1; //remote not registered
  }
  if (this->remoteCount < 1) {
    return -2;
  }

  int index = this->remoteIndex(id);

  if (index != -1) {
    for (uint8_t i = 0; i < 3; i++) {
      remotes[index].ID[i] = 0;
    }
    strlcpy(remotes[index].name, "remote", sizeof(remotes[index].name));
  }

  this->remoteCount--;

  return 1;
}

void IthoRemote::updateRemoteName(uint8_t index, char* remoteName) {
  strlcpy(remotes[index].name, remoteName, sizeof(remotes[index].name));
}

int IthoRemote::remoteIndex(int* id) {
  int noKnown = 0;
  for (uint8_t i = 0; i < MAX_NUMBER_OF_REMOTES; i++) {
    for (uint8_t y = 0; y < 3; y++) {
      if (id[y] == remotes[i].ID[y]) {
        noKnown++;
      }
    }
    if (noKnown == 3) {
      return i;
    }
    noKnown = 0;

  }
  return -1;
}

int * IthoRemote::getRemoteIDbyIndex(int index) {
  static int id[3];
  for (uint8_t i = 0; i < 3; i++) {
    id[i] = remotes[index].ID[i];
  }
  return id;
}


char * IthoRemote::getRemoteNamebyIndex(int index) {
  return remotes[index].name;
}

bool IthoRemote::checkID(int* id)
{
  int noKnown = 0;
  for (uint8_t i = 0; i < MAX_NUMBER_OF_REMOTES; i++) {
    for (uint8_t y = 0; y < 3; y++) {
      if (id[y] == remotes[i].ID[y]) {
        noKnown++;
      }
    }
    if (noKnown == 3) {
      return true;
    }
    noKnown = 0;

  }

  return false;

}

void IthoRemote::Remote::set(JsonObjectConst obj) {
  if (!(const char*)obj["name"].isNull()) {
    strlcpy(name, obj["name"], sizeof(name));
  }
  if (!(const char*)obj["id"].isNull()) {
    copyArray(obj["id"].as<JsonArray>(), ID);
  }
}

void IthoRemote::Remote::get(JsonObject obj, int index) const {
  obj["index"] = index;
  JsonArray id = obj.createNestedArray("id");
  for (uint8_t y = 0; y < 3; y++) {
    id.add(ID[y]);
  }
  obj["name"] = name;
}

bool IthoRemote::set(JsonObjectConst obj) {
  if (!configLoaded) {
    if ((const char*)obj["version_of_program"].isNull()) {
      return false;
    }
    if (obj["version_of_program"] != REMOTE_CONFIG_VERSION) {
      return false;
    }
  }

  if (!(const char*)obj["remotes"].isNull()) {
    JsonArrayConst remotesArray = obj["remotes"];
    remoteCount = 0;
    for (JsonObjectConst remote : remotesArray) {
      if (!(const char*)remote["index"].isNull()) {
        uint8_t index = remote["index"].as<uint8_t>();
        if (index < MAX_NUMBER_OF_REMOTES) {
          remotes[index].set(remote);

          uint8_t noZero = 0;
          for (uint8_t y = 0; y < 3; y++) {
            if (remotes[index].ID[y] == 0) {
              noZero++;
            }
          }
          if (noZero != 3) {
            remoteCount++;
          }
        }
      }
      if (remoteCount >= MAX_NUMBER_OF_REMOTES) break;
    }
  }
  else {
    return false;
  }
  return true;
}

void IthoRemote::get(JsonObject obj) const {
  // Add "remotes" object
  JsonArray rem = obj.createNestedArray("remotes");
  // Add each remote in the array
  for (int i = 0; i < MAX_NUMBER_OF_REMOTES; i++) {
    remotes[i].get(rem.createNestedObject(), i);
  }
  obj["version_of_program"] = config_struct_version;

}
