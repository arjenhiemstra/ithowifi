#include "config/IthoRemote.h"

// #include <string>
// #include <Arduino.h>

IthoRemote remotes(RemoteFunctions::RECEIVE);
IthoRemote virtualRemotes(RemoteFunctions::VREMOTE);

// default constructor
IthoRemote::IthoRemote()
{
  IthoRemote(RemoteFunctions::UNSETFUNC);
}
IthoRemote::IthoRemote(RemoteFunctions initFunc)
{
  instanceFunc = initFunc;
  strlcpy(config_struct_version, REMOTE_CONFIG_VERSION, sizeof(config_struct_version));

  for (uint8_t i = 0; i < MAX_NUM_OF_REMOTES; i++)
  {
    snprintf(remotes[i].name, sizeof(remotes[i].name), "remote%d", i);
  }

  configLoaded = false;

} // IthoRemote

// default destructor
IthoRemote::~IthoRemote()
{
} //~IthoRemote

const IthoRemote::remote_command_char IthoRemote::remote_command_msg_table[]{
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
    {IthoCook60, "IthoCook60"},
    {IthoTimerUser, "IthoTimerUser"},
    {IthoJoinReply, "IthoJoinReply"},
    {IthoPIRmotionOn, "IthoPIRmotionOn"},
    {IthoPIRmotionOff, "IthoPIRmotionOff"},
    {Itho31D9, "Itho31D9"},
    {Itho31DA, "Itho31DA"},
    {IthoDeviceInfo, "IthoDeviceInfo"}};

const char *IthoRemote::remote_unknown_msg = "CMD UNKNOWN ERROR";

const IthoRemote::remote_type_char IthoRemote::remote_type_table[]{
    {UNSETTYPE, "unset type"},
    {RFTCVE, "RFT CVE"},
    {RFTAUTO, "RFT Auto"},
    {RFTN, "RFT-N"},
    {RFTAUTON, "RFT Auto-N"},
    {DEMANDFLOW, "RFT DF/QF"},
    {RFTRV, "RFT RV"},
    {RFTCO2, "RFT CO2"},
    {RFTPIR, "RFT PIR"},
    {RFTSPIDER, "RFT Spider"},
    {ORCON15LF01, "Orcon 15lf01"}};

const char *IthoRemote::remote_type_unknown_msg = "Type unknown error";

const IthoRemote::remote_func_char IthoRemote::remote_func_table[]{
    {UNSETFUNC, "unset func"},
    {RECEIVE, "receive"},
    {VREMOTE, "vremote"},
    {MONITOR, "monitor"},
    {SEND, "send"}};

const char *IthoRemote::remote_func_unknown_msg = "Function unknown error";

int IthoRemote::getRemoteCount()
{
  return remoteCount;
}

bool IthoRemote::toggleLearnLeaveMode()
{
  llMode = llMode ? false : true;
  if (llMode)
  {
    llModeTime = 120;
  }
  return llMode;
};

void IthoRemote::updatellModeTimer()
{
  if (llModeTime > 0)
  {
    llModeTime--;
  }
}

int IthoRemote::registerNewRemote(uint8_t byte0, uint8_t byte1, uint8_t byte2, const RemoteTypes remtype)
{
  if (checkID(byte0, byte1, byte2))
  {
    return -1; // remote already registered
  }
  if (remoteCount >= maxRemotes - 1)
  {
    return -2;
  }

  int index = remoteIndex(0, 0, 0); // find first index with no remote ID set

  if (copy_id_remote_idx != -1 && copy_id_remote_idx < maxRemotes)
  {
    index = copy_id_remote_idx;
  }

  if (index < 0) // no valid spot found
  {
    return index;
  }
  remotes[index].ID[0] = byte0;
  remotes[index].ID[1] = byte1;
  remotes[index].ID[2] = byte2;

  remotes[index].remtype = remtype;

  if (copy_id_remote_idx > 0) // existing remote updated, reset copy_id_remote_idx;
  {
    copy_id_remote_idx = -1;
  }
  else // new remote added, update remote count
  {
    remoteCount++;
  }

  return index;
}

int IthoRemote::removeRemote(uint8_t byte0, uint8_t byte1, uint8_t byte2)
{
  if (!checkID(byte0, byte1, byte2))
  {
    return -1; // remote not registered
  }
  if (remoteCount < 1)
  {
    return -2;
  }

  int index = remoteIndex(byte0, byte1, byte2);

  if (index < 0)
    return -1;

  for (uint8_t i = 0; i < 3; i++)
  {
    remotes[index].ID[i] = 0;
  }
  snprintf(remotes[index].name, sizeof(remotes[index].name), "remote%d", index);
  // strlcpy(remotes[index].name, remName, sizeof(remotes[index].name));
  remotes[index].remtype = RemoteTypes::UNSETTYPE;
  remotes[index].remfunc = RemoteFunctions::UNSETFUNC;
  remotes[index].capabilities = nullptr;
  remotes[index].bidirectional = false;

  remoteCount--;

  return 1;
}

int IthoRemote::removeRemote(const uint8_t index)
{

  if (!(index < maxRemotes))
    return -1;

  if (remotes[index].ID[0] != 0 && remotes[index].ID[1] != 0 && remotes[index].ID[2] != 0)
  {
    for (uint8_t i = 0; i < 3; i++)
    {
      remotes[index].ID[i] = 0;
    }
    remoteCount--;
    if (instanceFunc == RemoteFunctions::VREMOTE)
    {
      remotes[index].ID[0] = sys.getMac(3);
      remotes[index].ID[1] = sys.getMac(4);
      remotes[index].ID[2] = sys.getMac(5) + index;
    }
  }

  snprintf(remotes[index].name, sizeof(remotes[index].name), "remote%d", index);
  remotes[index].remtype = RemoteTypes::UNSETTYPE;
  remotes[index].remfunc = RemoteFunctions::UNSETFUNC;
  if (instanceFunc == RemoteFunctions::RECEIVE)
  {
    remotes[index].remfunc = RemoteFunctions::RECEIVE;
  }
  if (instanceFunc == RemoteFunctions::VREMOTE)
  {
    remotes[index].remtype = RemoteTypes::RFTCVE;
    remotes[index].remfunc = RemoteFunctions::VREMOTE;
  }
  remotes[index].capabilities = nullptr;
  remotes[index].bidirectional = false;

  return 1;
}

void IthoRemote::updateRemoteName(const uint8_t index, const char *remoteName)
{
  strlcpy(remotes[index].name, remoteName, sizeof(remotes[index].name));
}

void IthoRemote::updateRemoteType(const uint8_t index, const uint16_t type)
{
  remotes[index].remtype = static_cast<RemoteTypes>(type);
}
void IthoRemote::updateRemoteID(const uint8_t index, uint8_t byte0, uint8_t byte1, uint8_t byte2)
{
  remotes[index].ID[0] = byte0;
  remotes[index].ID[1] = byte1;
  remotes[index].ID[2] = byte2;
}
void IthoRemote::updateRemoteFunction(const uint8_t index, const uint8_t remfunc)
{
  remotes[index].remfunc = static_cast<RemoteFunctions>(remfunc);
}

void IthoRemote::updateRemoteBidirectional(const uint8_t index, bool bidirectional)
{
  remotes[index].bidirectional = bidirectional;
}

void IthoRemote::addCapabilities(uint8_t remoteIndex, const char *name, int32_t value)
{
  if (strcmp(name, "temp") == 0 || strcmp(name, "dewpoint") == 0 || strcmp(name, "setpoint") == 0)
  {
    remotes[remoteIndex].capabilities[name] = static_cast<int>((value / 100.0) * 10 + 0.5) / 10.0;
  }
  else
  {
    remotes[remoteIndex].capabilities[name] = value;
  }
  if (strcmp(name, "lastcmd") == 0)
  {
    remotes[remoteIndex].capabilities["lastcmdmsg"] = rem_cmd_to_name(value);
  }
}

// int IthoRemote::remoteIndex(const int32_t id)
// {
//   if (id < 0)
//     return -1;
//   int tempID[3];
//   tempID[0] = (id >> 16) & 0xFF;
//   tempID[1] = (id >> 8) & 0xFF;
//   tempID[2] = id & 0xFF;
//   return remoteIndex(tempID);
// }

int IthoRemote::remoteIndex(uint8_t byte0, uint8_t byte1, uint8_t byte2)
{
  for (uint8_t i = 0; i < maxRemotes; i++)
  {
    if (byte0 == remotes[i].ID[0] && byte1 == remotes[i].ID[1] && byte2 == remotes[i].ID[2])
      return i;
  }
  return -1;
}

void IthoRemote::getRemoteIDbyIndex(const int index, uint8_t *id)
{
  if (!id)
    return;

  for (uint8_t i = 0; i < 3; i++)
  {
    id[i] = remotes[index].ID[i];
  }
}

const char *IthoRemote::getRemoteNamebyIndex(const int index)
{
  return remotes[index].name;
}

int IthoRemote::getRemoteIndexbyName(const char *name)
{
  if (name == nullptr)
    return -1;
  if (strcmp(name, "") == 0)
    return -1;

  for (uint8_t i = 0; i < maxRemotes; i++)
  {
    if (strcmp(name, remotes[i].name) == 0)
      return i;
  }
  return -1;
}

bool IthoRemote::checkID(uint8_t byte0, uint8_t byte1, uint8_t byte2)
{
  if (byte0 == 0 && byte1 == 0 && byte2 == 0)
    return false;

  for (uint8_t i = 0; i < maxRemotes; i++)
  {
    if (byte0 == remotes[i].ID[0] && byte1 == remotes[i].ID[1] && byte2 == remotes[i].ID[2])
      return true;
  }
  return false;
}

void IthoRemote::Remote::set(JsonObject obj)
{
  if (!obj["name"].isNull())
  {
    strlcpy(name, obj["name"] | "", sizeof(name));
  }
  if (!obj["id"].isNull())
  {
    if (obj["id"].is<const char *>())
    {
      // parse hex
    }
    else
    {
      copyArray(obj["id"].as<JsonArray>(), ID);
    }
  }
  if (!obj["remtype"].isNull())
  {
    remtype = obj["remtype"];
  }
  if (!obj["remfunc"].isNull())
  {
    remfunc = static_cast<RemoteFunctions>(obj["remfunc"]);
  }
  if (!obj["bidirectional"].isNull())
  {
    bidirectional = obj["bidirectional"];
  }
}

void IthoRemote::Remote::get(JsonObject obj, RemoteFunctions instanceFunc, int index, bool human_reaadble) const
{
  obj["index"] = index;

  if (remtype == RemoteTypes::UNSETTYPE)
  {
    if (instanceFunc == RemoteFunctions::RECEIVE)
    {
      remtype = RemoteTypes::RFTCVE;
    }
    else if (instanceFunc == RemoteFunctions::VREMOTE)
    {
      remtype = RemoteTypes::RFTCVE;
      ID[0] = sys.getMac(3);
      ID[1] = sys.getMac(4);
      ID[2] = sys.getMac(5) + index;
    }
  }
  if (remfunc == RemoteFunctions::UNSETFUNC)
  {
    if (instanceFunc == RemoteFunctions::RECEIVE)
    {
      remfunc = RemoteFunctions::RECEIVE;
    }
    else if (instanceFunc == RemoteFunctions::VREMOTE)
    {
      remfunc = RemoteFunctions::VREMOTE;
    }
  }

  JsonArray id = obj["id"].to<JsonArray>();
  for (uint8_t y = 0; y < 3; y++)
  {
    id.add(ID[y]);
  }
  obj["name"] = name;
  obj["remfunc"] = remfunc;
  obj["remfuncname"] = rem_func_to_name(remfunc);
  obj["remtype"] = remtype;
  obj["remtypename"] = rem_type_to_name(remtype);

  if (capabilities.isNull())
  {
    obj["capabilities"] = nullptr;
  }
  else
  {
    obj["capabilities"] = capabilities;
  }
  obj["bidirectional"] = bidirectional;
}

bool IthoRemote::set(JsonObject obj, const char *root)
{
  if (!configLoaded)
  {
    if ((const char *)obj["version_of_program"].isNull())
    {
      return false;
    }
    if (obj["version_of_program"] != REMOTE_CONFIG_VERSION)
    {
      return false;
    }
  }
  if (!obj[root].isNull())
  {
    JsonArray remotesArray = obj[root];
    remoteCount = 0;
    for (JsonObject remote : remotesArray)
    {
      if (!remote["index"].isNull())
      {
        uint8_t index = remote["index"].as<uint8_t>();
        if (index < maxRemotes)
        {
          remotes[index].set(remote);

          uint8_t noZero = 0;
          for (uint8_t y = 0; y < 3; y++)
          {
            if (remotes[index].ID[y] == 0)
            {
              noZero++;
            }
          }
          if (noZero != 3)
          {
            remoteCount++;
          }
        }
      }
      if (remoteCount >= maxRemotes)
        break;
    }
  }
  else
  {
    D_LOG("SYS: IthoRemote::set root not present ");
    return false;
  }
  return true;
}

void IthoRemote::get(JsonObject obj, const char *root) const
{
  // Add "remotes" object
  JsonArray rem = obj[root].to<JsonArray>();
  // Add each remote in the array
  bool human_readable = false;
  if (strcmp(root, "vremotesinfo") == 0)
    human_readable = true;

  for (int i = 0; i < maxRemotes; i++)
  {
    remotes[i].get(rem.add<JsonObject>(), instanceFunc, i, human_readable);
  }
  obj["remfunc"] = instanceFunc;
  obj["version_of_program"] = config_struct_version;
}

void IthoRemote::getCapabilities(JsonObject obj) const
{

  for (int i = 0; i < remoteCount; i++)
  {
    obj[remotes[i].name] = remotes[i].capabilities;
  }
}

const char *IthoRemote::rem_cmd_to_name(uint8_t code)
{
  size_t i;

  for (i = 0; i < sizeof(remote_command_msg_table) / sizeof(remote_command_msg_table[0]); ++i)
  {
    if (remote_command_msg_table[i].code == code)
    {
      return remote_command_msg_table[i].msg;
    }
  }
  return remote_unknown_msg;
}

const char *IthoRemote::Remote::rem_type_to_name(const RemoteTypes type) const
{
  size_t i;

  for (i = 0; i < sizeof(remote_type_table) / sizeof(remote_type_table[0]); ++i)
  {
    if (remote_type_table[i].type == type)
    {
      return remote_type_table[i].msg;
    }
  }
  return remote_type_unknown_msg;
}

const char *IthoRemote::Remote::rem_func_to_name(const RemoteFunctions func) const
{
  size_t i;

  for (i = 0; i < sizeof(remote_func_table) / sizeof(remote_func_table[0]); ++i)
  {
    if (remote_func_table[i].func == func)
    {
      return remote_func_table[i].msg;
    }
  }
  return remote_type_unknown_msg;
}

void IthoRemote::reset()
{
}