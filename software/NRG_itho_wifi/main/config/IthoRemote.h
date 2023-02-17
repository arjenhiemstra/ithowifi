#pragma once

#define MAX_NUMBER_OF_REMOTES 12
#define REMOTE_CONFIG_VERSION "002"

#include <cstdio>
#include <string>

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Ticker.h>

#include "cc1101/IthoPacket.h"
#include "sys_log.h"
#include "System.h"

enum RemoteFunctions : uint8_t
{
  UNSETFUNC,
  RECEIVE,
  VREMOTE,
  MONITOR
};

class IthoRemote
{
private:
  struct Remote
  {
    mutable uint8_t ID[3]{0, 0, 0};
    char name[32];
    mutable RemoteTypes remtype{RemoteTypes::UNSETTYPE};
    mutable RemoteFunctions remfunc{RemoteFunctions::UNSETFUNC};
    StaticJsonDocument<128> capabilities;
    void set(JsonObjectConst);
    void get(JsonObject, RemoteFunctions instanceFunc, int index) const;
  };
  RemoteFunctions instanceFunc{RemoteFunctions::UNSETFUNC};
  Remote remotes[MAX_NUMBER_OF_REMOTES];
  int remoteCount{0};
  int maxRemotes{10};
  mutable bool llMode = false;

  volatile uint8_t llModeTime{0};

  typedef struct
  {
    uint8_t code;
    const char *msg;
  } remote_command_char;

  const remote_command_char remote_command_msg_table[15]{
      {IthoUnknown, "IthoUnknown"},
      {IthoJoin, "IthoJoin"},
      {IthoLeave, "IthoLeave"},
      {IthoAway, "IthoAway"},
      {IthoLow, "IthoLow"},
      {IthoMedium, "IthoMedium"},
      {IthoHigh, "IthoHigh"},
      {IthoFull, "IthoFull"},
      {IthoTimer1, "IthoTimer1"},
      {IthoTimer2, "IthoTimer2"},
      {IthoTimer3, "IthoTimer3"},
      {IthoAuto, "IthoAuto"},
      {IthoAutoNight, "IthoAutoNight"},
      {IthoCook30, "IthoCook30"},
      {IthoCook60, "IthoCook60"}};

  const char *remote_unknown_msg = "CMD UNKNOWN ERROR";

public:
  IthoRemote();
  IthoRemote(RemoteFunctions instanceFunc);
  ~IthoRemote();
  int getRemoteCount();
  // mutable volatile bool llModeTimerUpdated { false };
  bool toggleLearnLeaveMode();
  bool remoteLearnLeaveStatus()
  {
    return llMode;
  };
  void updatellModeTimer();
  uint8_t getllModeTime()
  {
    return llModeTime;
  };
  void setllModeTime(const int timeVal)
  {
    llModeTime = timeVal;
  };
  int activeRemote{-1};
  int setMaxRemotes(unsigned int number) { return (maxRemotes < MAX_NUMBER_OF_REMOTES + 1) ? (maxRemotes = number) : (maxRemotes = MAX_NUMBER_OF_REMOTES); };
  int getMaxRemotes() { return maxRemotes; };
  int registerNewRemote(const int *id, const RemoteTypes remtype);
  int removeRemote(const int *id);
  int removeRemote(const uint8_t index);
  void addCapabilities(const uint8_t remoteIndex, const char *name, int32_t value);
  void updateRemoteName(const uint8_t index, const char *remoteName);
  void updateRemoteType(const uint8_t index, const uint16_t type);
  void updateRemoteID(const uint8_t index, const uint8_t *id);
  void updateRemoteFunction(const uint8_t index, const uint8_t remfunc);
  int remoteIndex(const int32_t id);
  int remoteIndex(const int *id);
  const int *getRemoteIDbyIndex(const int index);
  const char *getRemoteNamebyIndex(const int index);
  int getRemoteIndexbyName(const char *name);
  RemoteTypes getRemoteType(const int index) { return remotes[index].remtype; };
  RemoteFunctions getRemoteFunction(const int index) { return remotes[index].remfunc; };
  const char *lastRemoteName;
  bool checkID(const int *id);
  bool configLoaded;
  char config_struct_version[4];
  bool set(JsonObjectConst, const char *root);
  void get(JsonObject obj, const char *root) const;
  void getCapabilities(JsonObject obj) const;
  const char *rem_cmd_to_name(uint8_t code);

protected:
}; // IthoRemote

extern IthoRemote remotes;
extern IthoRemote virtualRemotes;