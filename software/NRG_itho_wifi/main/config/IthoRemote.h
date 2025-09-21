#pragma once

#if !defined(MAX_NUM_OF_REMOTES)
#define MAX_NUM_OF_REMOTES 12
#endif
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
  UNSETFUNC = 0,
  RECEIVE = 1,
  VREMOTE = 2,
  MONITOR = 3,
  SEND = 5
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
    bool bidirectional{false};
    JsonDocument capabilities;
    void set(JsonObject);
    void get(JsonObject, RemoteFunctions instanceFunc, int index, bool human_reaadble = false) const;
    const char *rem_type_to_name(RemoteTypes type) const;
    const char *rem_func_to_name(RemoteFunctions func) const;
  };
  RemoteFunctions instanceFunc{RemoteFunctions::UNSETFUNC};
  Remote remotes[MAX_NUM_OF_REMOTES];
  int remoteCount{0};
  int maxRemotes{MAX_NUM_OF_REMOTES};
  mutable bool llMode = false;

  uint8_t llModeTime{0};

  typedef struct
  {
    uint8_t code;
    const char *msg;
  } remote_command_char;

  static const remote_command_char remote_command_msg_table[];
  static const char *remote_unknown_msg;

  typedef struct
  {
    RemoteTypes type;
    const char *msg;
  } remote_type_char;

  static const remote_type_char remote_type_table[];
  static const char *remote_type_unknown_msg;


  typedef struct
  {
    RemoteFunctions func;
    const char *msg;
  } remote_func_char;

  static const remote_func_char remote_func_table[];
  static const char *remote_func_unknown_msg;

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
  int copy_id_remote_idx{-1};
  int setMaxRemotes(unsigned int number) { return (maxRemotes < MAX_NUM_OF_REMOTES + 1) ? (maxRemotes = number) : (maxRemotes = MAX_NUM_OF_REMOTES); };
  int getMaxRemotes() { return maxRemotes; };
  int registerNewRemote(uint8_t byte0, uint8_t byte1, uint8_t byte2, const RemoteTypes remtype);
  int removeRemote(uint8_t byte0, uint8_t byte1, uint8_t byte2);
  int removeRemote(const uint8_t index);
  void addCapabilities(const uint8_t remoteIndex, const char *name, int32_t value);
  void updateRemoteName(const uint8_t index, const char *remoteName);
  void updateRemoteType(const uint8_t index, const uint16_t type);
  void updateRemoteID(const uint8_t index, uint8_t byte0, uint8_t byte1, uint8_t byte2);
  void updateRemoteBidirectional(const uint8_t index, bool bidirectional);
  void updateRemoteFunction(const uint8_t index, const uint8_t remfunc);
  // int remoteIndex(const int32_t id);
  int remoteIndex(uint8_t byte0, uint8_t byte1, uint8_t byte2);
  void getRemoteIDbyIndex(const int index, uint8_t *id);
  const char *getRemoteNamebyIndex(const int index);
  int getRemoteIndexbyName(const char *name);
  RemoteTypes getRemoteType(const int index) { return remotes[index].remtype; };
  RemoteFunctions getRemoteFunction(const int index) { return remotes[index].remfunc; };
  bool getRemoteBidirectional(const int index) { return remotes[index].bidirectional; };
  const char *lastRemoteName;
  bool checkID(uint8_t byte0, uint8_t byte1, uint8_t byte2);
  bool configLoaded;
  char config_struct_version[4];
  bool set(JsonObject, const char *root);
  void get(JsonObject obj, const char *root) const;
  void getCapabilities(JsonObject obj) const;
  const char *rem_cmd_to_name(uint8_t code);

protected:
}; // IthoRemote

extern IthoRemote remotes;
extern IthoRemote virtualRemotes;