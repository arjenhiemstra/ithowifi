/*
   Author: Klusjesman, supersjimmie, modified and reworked by arjenhiemstra
*/

#pragma once

#include <cstdio>
#include <string>

#include "CC1101.h"
#include "IthoPacket.h"

#define MAX_NUM_OF_REMOTES 10

struct ithoRFDevice
{
  uint32_t deviceId{0};
  RemoteTypes remType{RemoteTypes::RFTCVE};
  //  char name[16];
  IthoCommand lastCommand{IthoUnknown};
  time_t timestamp;
  uint8_t counter;
  int32_t co2{0xEFFF};
  int32_t temp{0xEFFF};
  int32_t hum{0xEFFF};
  int32_t dewpoint{0xEFFF};
  int32_t battery{0xEFFF};
};

struct RFmessage
{
  //<HEADER> <addr0> <addr1> <addr2> <param0> <param1> <OPCODE> <LENGTH> <PAYLOAD> <CHECKSUM>
  // from: https://github.com/ghoti57/evofw3/wiki/Message-Body#message-body
  // Header format:
  // 00TTAAPP
  // TT specifies the type of the message
  // TT
  // 00 RQ
  // 01  I
  // 10  W
  // 11 RP

  // AA specifies which address fields are present
  // AA
  // 00 addr0 + addr1 + addr2
  // 01 addr2
  // 10 addr0 + addr2
  // 11 addr0 + addr1

  // PP which params are present PP
  // 1x Param 0 is present
  // x1 Param 1 is present
  //
  // ie. 0x16 == 00 01 01 10
  uint8_t header{0xC0};
  uint8_t deviceid0[3]{0};
  uint8_t deviceid1[3]{0};
  uint8_t deviceid2[3]{0};
  uint8_t opt0{0};
  uint8_t opt1{0};
  uint16_t opcode{0};
  uint8_t len{0};
  const uint8_t *command{nullptr};
  //uint8_t checksum;
};

struct ithoRFDevices
{
  uint8_t count{0};
  ithoRFDevice device[MAX_NUM_OF_REMOTES];
};

// pa table settings
const uint8_t ithoPaTableSend[8] = {0x6F, 0x26, 0x2E, 0x8C, 0x87, 0xCD, 0xC7, 0xC0};
const uint8_t ithoPaTableReceive[8] = {0x6F, 0x26, 0x2E, 0x7F, 0x8A, 0x84, 0xCA, 0xC4};

class IthoPacket;

class IthoCC1101 : protected CC1101
{
private:
  // receive
  CC1101Packet inMessage;  // temp storage message2
  IthoPacket inIthoPacket; // stores last received message data

  // send
  IthoPacket outIthoPacket; // stores state of "remote"

  // settings
  uint8_t sendTries;  // number of times a command is send at one button press
  uint8_t cc_freq[3]; // FREQ0, FREQ1, FREQ2

  // Itho remotes
  bool bindAllowed;
  bool allowAll;
  ithoRFDevices ithoRF;

  typedef struct
  {
    IthoCommand code;
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
  // functions
public:
  IthoCC1101(uint8_t counter = 0, uint8_t sendTries = 3); // set initial counter value
  ~IthoCC1101();

  // init
  void init()
  {
    CC1101::init(); // init,reset CC1101
    initReceive();
  }
  void initReceive();
  uint8_t getLastCounter()
  {
    return outIthoPacket.counter; // counter is increased before sending a command
  }
  void setSendTries(uint8_t sendTries)
  {
    this->sendTries = sendTries;
  }
  void setDeviceID(uint8_t byte0, uint8_t byte1, uint8_t byte2)
  {
    this->outIthoPacket.deviceId[0] = byte0;
    this->outIthoPacket.deviceId[1] = byte1;
    this->outIthoPacket.deviceId[2] = byte2;
  }

  bool addRFDevice(uint8_t byte0, uint8_t byte1, uint8_t byte2, RemoteTypes deviceType);
  bool addRFDevice(uint32_t ID, RemoteTypes deviceType);
  bool updateRFDeviceID(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t remote_index);
  bool updateRFDeviceID(uint32_t ID, uint8_t remote_index);
  bool updateRFDeviceType(RemoteTypes deviceType, uint8_t remote_index);
  bool removeRFDevice(uint8_t byte0, uint8_t byte1, uint8_t byte2);
  bool removeRFDevice(uint32_t ID);
  bool checkRFDevice(uint8_t byte0, uint8_t byte1, uint8_t byte2);
  bool checkRFDevice(uint32_t ID);
  void setBindAllowed(bool input)
  {
    bindAllowed = input;
  }
  bool getBindAllowed()
  {
    return bindAllowed;
  }
  void setAllowAll(bool input)
  {
    allowAll = input;
  }
  bool getAllowAll()
  {
    return allowAll;
  }
  const struct ithoRFDevices &getRFdevices() const
  {
    return ithoRF;
  }
  // receive
  uint8_t receivePacket(); // read RX fifo
  bool checkForNewPacket();
  IthoPacket getLastPacket()
  {
    return inIthoPacket; // retrieve last received/parsed packet from remote
  }
  IthoCommand getLastCommand()
  {
    return inIthoPacket.command; // retrieve last received/parsed command from remote
  }
  RemoteTypes getLastRemType()
  {
    return inIthoPacket.remType; // retrieve last received/parsed rf device type
  }
  uint8_t getLastInCounter()
  {
    return inIthoPacket.counter; // retrieve last received/parsed command from remote
  }
  uint32_t getFrequency()
  {
    return ((uint32_t)cc_freq[2] << 16) | ((uint32_t)cc_freq[1] << 8) | ((uint32_t)cc_freq[0] << 0);
  }

  uint8_t ReadRSSI();
  bool checkID(const uint8_t *id);
  int *getLastID();
  String getLastIDstr(bool ashex = true);
  String getLastMessagestr(bool ashex = true);
  String LastMessageDecoded();

  // send
  const uint8_t *getRemoteCmd(const RemoteTypes type, const IthoCommand command);
  void sendRFCommand(uint8_t remote_index, IthoCommand command);
  void sendRFMessage(RFmessage *message);
  void sendCommand(IthoCommand command);

  void handleBind();
  void handleLevel();
  void handleTimer();
  void handleStatus();
  void handleRemotestatus();
  void handleTemphum();
  void handleCo2();
  void handleBattery();

  const char *rem_cmd_to_name(IthoCommand code);

protected:
private:
  IthoCC1101(const IthoCC1101 &c);
  IthoCC1101 &operator=(const IthoCC1101 &c);

  // init CC1101 for receiving
  void initReceiveMessage();

  // init CC1101 for sending
  void initSendMessage(uint8_t len);
  void finishTransfer();

  // parse received message
  bool parseMessageCommand();
  bool checkIthoCommand(IthoPacket *itho, const uint8_t commandBytes[]);

  // send
  void createMessageStart(IthoPacket *itho, CC1101Packet *packet);
  uint8_t checksum(IthoPacket *itho, uint8_t len);

  uint8_t messageEncode(IthoPacket *itho, CC1101Packet *packet);
  void messageDecode(CC1101Packet *packet, IthoPacket *itho);

}; // IthoCC1101
