#include "IthoRemote.h"
#include <string.h>
#include <Arduino.h>


// default constructor
IthoRemote::IthoRemote() {

  for (uint8_t i = 0; i < MAX_NUMBER_OF_REMOTES; i++) {
    strlcpy(remotes[i].name, "Remote", sizeof(remotes[i].name));
  }

} //IthoRemote

// default destructor
IthoRemote::~IthoRemote() {
} //~IthoRemote


int IthoRemote::getRemoteCount() {
  return this->remoteCount;
}

int IthoRemote::registerNewRemote(int* id) {
  if (this->checkID(id)) {
    return -1; //remote already registered
  }
  if (this->remoteCount >= MAX_NUMBER_OF_REMOTES - 1) {
    return -2;
  }
  int zeroID[] = {0, 0, 0, 0, 0, 0, 0, 0};
  int index = this->remoteIndex(zeroID); //find first index with no remote ID set
  if (index < 0) {
    return index;
  }
  for (uint8_t i = 0; i < 8; i++) {
    remotes[index].ID[i] = id[i];
  }

  this->remoteCount++;

  return 1;
}

int IthoRemote::removeRemote(int* id) {
  if (!this->checkID(id)) {
    return -1; //remote not registered
  }
  if (!this->remoteCount > 0) {
    return -2;
  }

  int index = this->remoteIndex(id);

  if (index != -1) {
    for (uint8_t i = 0; i < 8; i++) {
      remotes[index].ID[i] = 0;
    }
    strlcpy(remotes[index].name, "Remote", sizeof(remotes[index].name));
  }

  this->remoteCount--;

  return 1;
}

int IthoRemote::remoteIndex(int* id) {
  int noKnown = 0;
  for (uint8_t i = 0; i < MAX_NUMBER_OF_REMOTES; i++) {
    for (uint8_t y = 0; y < 8; y++) {
      if (id[y] == remotes[i].ID[y]) {
        noKnown++;
      }
    }
    if (noKnown == 8) {
      return i;
    }
    noKnown = 0;

  }
  return -1;
}

int * IthoRemote::getRemoteIDbyIndex(int index) {
  static int id[8];
  for (uint8_t i = 0; i < 8; i++) {
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
    for (uint8_t y = 0; y < 8; y++) {
      if (id[y] == remotes[i].ID[y]) {
        noKnown++;
      }
    }
    if (noKnown == 8) {
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
  for (uint8_t y = 0; y < 8; y++) {
    id.add(ID[y]);
  }
  obj["name"] = name;
}

void IthoRemote::set(JsonObjectConst obj) {
  if (!(const char*)obj["remotes"].isNull()) {
    JsonArrayConst remotesArray = obj["remotes"];
    remoteCount = 0;
    for (JsonObjectConst remote : remotesArray) {
      if (!(const char*)remote["index"].isNull()) {
        uint8_t index = remote["index"].as<uint8_t>();
        if(index < MAX_NUMBER_OF_REMOTES)
          remotes[index].set(remote);
      }
      remoteCount++;
      if (remoteCount >= MAX_NUMBER_OF_REMOTES) break;
    }
  }
}

void IthoRemote::get(JsonObject obj) const {
  // Add "remotes" object
  JsonArray rem = obj.createNestedArray("remotes");
  // Add each remote in the array
  for (int i = 0; i < MAX_NUMBER_OF_REMOTES; i++) {
    remotes[i].get(rem.createNestedObject(), i);
  }

}
