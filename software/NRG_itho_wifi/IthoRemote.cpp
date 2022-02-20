#include "IthoRemote.h"

#include <string>
#include <Arduino.h>


// default constructor
IthoRemote::IthoRemote() {
  strlcpy(config_struct_version, REMOTE_CONFIG_VERSION, sizeof(config_struct_version));

  for (uint8_t i = 0; i < MAX_NUMBER_OF_REMOTES; i++) {
    sprintf(remotes[i].name, "remote%d", i);
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

int IthoRemote::registerNewRemote(const int* id) {
  if (this->checkID(id)) {
    return -1; //remote already registered
  }
  if (activeRemote != -1 && activeRemote < maxRemotes) {
    for (uint8_t i = 0; i < 3; i++) {
      remotes[activeRemote].ID[i] = id[i];
    }
    return 1;
  }  
  if (this->remoteCount >= maxRemotes - 1) {
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

int IthoRemote::removeRemote(const int* id) {
  if (!this->checkID(id)) {
    return -1; //remote not registered
  }
  if (this->remoteCount < 1) {
    return -2;
  }

  int index = this->remoteIndex(id);

  if (index < 0) return -1;

  for (uint8_t i = 0; i < 3; i++) {
    remotes[index].ID[i] = 0;
  }
  sprintf(remotes[index].name, "remote%d", index);
  //strlcpy(remotes[index].name, remName, sizeof(remotes[index].name));
  remotes[index].capabilities = nullptr;

  this->remoteCount--;

  return 1;
}

int IthoRemote::removeRemote(const uint8_t index, const char * type) {

  if (!(index < maxRemotes )) return -1;

  if (remotes[index].ID[0] != 0 && remotes[index].ID[1] != 0 && remotes[index].ID[2] != 0) {
    for (uint8_t i = 0; i < 3; i++) {
      remotes[index].ID[i] = 0;
    }
    this->remoteCount--;
    if (strcmp(type, "vremote") == 0) {
      remotes[index].ID[0] = sys.getMac(6 - 3);
      remotes[index].ID[1] = sys.getMac(6 - 2);
      remotes[index].ID[2] = sys.getMac(6 - 1) + index;
    }
  }

  sprintf(remotes[index].name, "remote%d", index);
  remotes[index].remtype = UNSETTYPE;
  if (strcmp(type, "remote") == 0) remotes[index].remtype = NORMAL;
  if (strcmp(type, "vremote") == 0) remotes[index].remtype = RFTCVE;
  remotes[index].capabilities = nullptr;

  return 1;
}

void IthoRemote::updateRemoteName(const uint8_t index, const char* remoteName) {
  strlcpy(remotes[index].name, remoteName, sizeof(remotes[index].name));
}

void IthoRemote::updateRemoteType(const uint8_t index, const uint16_t type) {
  remotes[index].remtype = static_cast<RemoteTypes>(type);
}

void IthoRemote::addCapabilities(uint8_t remoteIndex, const char* name, int32_t value) {
  if (strcmp(name, "temp") == 0 || strcmp(name, "dewpoint") == 0) {
    float tempVal = value / 100.0f;
    remotes[remoteIndex].capabilities[name] = tempVal;
  }
  else {
    remotes[remoteIndex].capabilities[name] = value;
  }

}

int IthoRemote::remoteIndex(const int32_t id) {
  if (id < 0) return -1;
  int tempID[3];
  tempID[0] = (id >> 16) & 0xFF;
  tempID[1] = (id >> 8) & 0xFF;
  tempID[2] = id & 0xFF;
  return remoteIndex(tempID);
}

int IthoRemote::remoteIndex(const int* id) {
  int noKnown = 0;
  for (uint8_t i = 0; i < maxRemotes; i++) {
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

const int * IthoRemote::getRemoteIDbyIndex(const int index) {
  static int id[3];
  for (uint8_t i = 0; i < 3; i++) {
    id[i] = remotes[index].ID[i];
  }
  return id;
}


const char * IthoRemote::getRemoteNamebyIndex(const int index) {
  return remotes[index].name;
}

bool IthoRemote::checkID(const int* id)
{
  int noKnown = 0;
  for (uint8_t i = 0; i < maxRemotes; i++) {
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
    copyArray(obj["id"].as<JsonArrayConst>(), ID);
  }
  if (!(const char*)obj["remtype"].isNull()) {
    remtype = obj["remtype"];
  }
}

void IthoRemote::Remote::get(JsonObject obj, const char* root, int index) const {
  obj["index"] = index;

  if (remtype == RemoteTypes::UNSETTYPE) {
    if (strcmp(root, "remotes") == 0) {
      remtype = RemoteTypes::RFTCVE;
    }
    else if (strcmp(root, "vremotes") == 0) {
      remtype = RemoteTypes::RFTCVE;
      ID[0] = sys.getMac(6 - 3);
      ID[1] = sys.getMac(6 - 2);
      ID[2] = sys.getMac(6 - 1) + index;
    }
  }

  JsonArray id = obj.createNestedArray("id");
  for (uint8_t y = 0; y < 3; y++) {
    id.add(ID[y]);
  }
  obj["name"] = name;
  obj["remtype"] = remtype;
  if (capabilities.isNull()) {
    obj["capabilities"] = nullptr;
  }
  else {
    obj["capabilities"] = capabilities;
  }

}

bool IthoRemote::set(JsonObjectConst obj, const char* root) {
  if (!configLoaded) {
    if ((const char*)obj["version_of_program"].isNull()) {
      return false;
    }
    if (obj["version_of_program"] != REMOTE_CONFIG_VERSION) {
      return false;
    }
  }
  if (!(const char*)obj["remfunc"].isNull()) {
    remfunc = static_cast<RemoteFunctions>(obj["remfunc"]);
  }
  if (!(const char*)obj[root].isNull()) {
    JsonArrayConst remotesArray = obj[root];
    remoteCount = 0;
    for (JsonObjectConst remote : remotesArray) {
      if (!(const char*)remote["index"].isNull()) {
        uint8_t index = remote["index"].as<uint8_t>();
        if (index < maxRemotes) {
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
      if (remoteCount >= maxRemotes) break;
    }
  }
  else {
    D_LOG("IthoRemote::set root not present \n");
    return false;
  }
  return true;
}

void IthoRemote::get(JsonObject obj, const char* root) const {

  if (remfunc == RemoteFunctions::UNSETFUNC) {
    if (strcmp(root, "remotes") == 0) {
      remfunc = RemoteFunctions::RECEIVE;
    }
    else if (strcmp(root, "vremotes") == 0) {
      remfunc = RemoteFunctions::SEND;
    }
  }

  // Add "remotes" object
  JsonArray rem = obj.createNestedArray(root);
  // Add each remote in the array
  for (int i = 0; i < maxRemotes; i++) {
    remotes[i].get(rem.createNestedObject(), root, i);
  }
  obj["remfunc"] = remfunc;
  obj["version_of_program"] = config_struct_version;

}

void IthoRemote::getCapabilities(JsonObject obj) const {

  for (int i = 0; i < remoteCount; i++) {
    obj[remotes[i].name] = remotes[i].capabilities;
  }


}
