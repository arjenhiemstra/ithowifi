/*
   Author: Klusjesman, supersjimmie, modified and reworked by arjenhiemstra
*/

#pragma once

#include <cstdio>
#include <string>

#include "CC1101.h"
#include "IthoPacket.h"
#include "CC1101Packet.h"
#include "sys_log.h"

#if !defined(MAX_NUM_OF_REMOTES)
#define MAX_NUM_OF_REMOTES 10
#endif
struct ithoRFDevice
{
  uint8_t ownDeviceID[3]{};
  uint8_t remoteID[3]{};
  RemoteTypes remType{RemoteTypes::RFTCVE};
  //  char name[16];
  IthoCommand lastCommand{IthoUnknown};
  time_t timestamp{};
  uint8_t counter{};
  bool bidirectional{false};
  int32_t co2{0xEFFF};
  int32_t temp{0xEFFF};
  int32_t setpoint{0xEFFF};
  int32_t hum{0xEFFF};
  uint8_t pir{0xEF};
  int32_t dewpoint{0xEFFF};
  int32_t battery{0xEFFF};
};

struct RFmessage
{
  uint8_t header{0xC0};
  uint8_t deviceid0[3]{0};
  uint8_t deviceid1[3]{0};
  uint8_t deviceid2[3]{0};
  uint8_t opt0{0};
  uint8_t opt1{0};
  uint16_t opcode{0};
  uint8_t len{0};
  const uint8_t *command{nullptr};
  // uint8_t checksum;
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
  uint8_t chipVersion{};

  // receive
  IthoPacket inPacket;

  // send
  uint8_t defaultID[3]{33, 66, 99};

  IthoPacket outIthoPacket; // stores state of "remote"
  uint8_t counter{};
  uint8_t CC1101MessageLen{};
  uint8_t IthoPacketLen{};
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

  static const remote_command_char remote_command_msg_table[];
  static const char *remote_unknown_msg;

  // init CC1101 for receiving
  void initReceive();

  void initReceiveMessage();

  // init CC1101 for sending
  void initSendMessage(uint8_t len);
  void finishTransfer();

  // parse received message
  bool checkIthoCommand(IthoPacket *itho, const uint8_t commandBytes[]);

  // send
  void createMessageStart(IthoPacket *itho, CC1101Packet *packet);
  uint8_t checksum(IthoPacket *itho, uint8_t len);

  uint8_t messageEncode(IthoPacket *itho, CC1101Packet *packet);
  void messageDecode(CC1101Packet *packet, IthoPacket *packetPtr);
  // functions
public:
  IthoCC1101(); // set initial counter value
  ~IthoCC1101();

  // init
  void init()
  {
    CC1101::init(); // init,reset CC1101
    initReceive();
  }

  void setSendTries(uint8_t number)
  {
    sendTries = number;
  }
  void setDefaultID(uint8_t byte0, uint8_t byte1, uint8_t byte2)
  {
    defaultID[0] = byte0;
    defaultID[1] = byte1;
    defaultID[2] = byte2;
  }
  bool addRFDevice(uint8_t byte0, uint8_t byte1, uint8_t byte2, RemoteTypes deviceType, bool bidirectional = false);
  bool updateRFDevice(uint8_t remote_index, uint8_t byte0, uint8_t byte1, uint8_t byte2, RemoteTypes deviceType, bool bidirectional = false);
  // updateRFSourceID
  bool updateRFOwnDeviceID(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t remote_index);
  // updateRFDestinationID
  bool updateRFRemoteID(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t remote_index);
  bool updateRFDeviceType(RemoteTypes deviceType, uint8_t remote_index);
  bool removeRFDevice(uint8_t byte0, uint8_t byte1, uint8_t byte2);
  bool removeRFDevice(uint8_t remote_index);
  bool checkRFDevice(uint8_t byte0, uint8_t byte1, uint8_t byte2);
  void setRFDeviceBidirectional(uint8_t remote_index, bool bidirectional);
  bool getRFDeviceBidirectional(uint8_t remote_index);
  bool getRFDeviceBidirectionalByID(uint8_t byte0, uint8_t byte1, uint8_t byte2);
  int8_t getRemoteIndexByID(uint8_t byte0, uint8_t byte1, uint8_t byte2);

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
  bool receivePacket(); // read RX fifo
  IthoPacket *checkForNewPacket();
  bool parseMessage(IthoPacket *packetPtr);

  IthoPacket *getLastPacket(IthoPacket *packetPtr)
  {
    return &inPacket;
  }
  IthoCommand getLastCommand(IthoPacket *packetPtr)
  {
    return packetPtr->command; // retrieve last received/parsed command from remote
  }
  RemoteTypes getLastRemType(IthoPacket *packetPtr)
  {
    return packetPtr->remType; // retrieve last received/parsed rf device type
  }
  uint32_t getFrequency()
  {
    return ((uint32_t)cc_freq[2] << 16) | ((uint32_t)cc_freq[1] << 8) | ((uint32_t)cc_freq[0] << 0);
  }

  uint8_t ReadRSSI();
  uint8_t getChipVersion();
  // bool checkID(const uint8_t *id);
  uint8_t *getLastID(IthoPacket *packetPtr);
  String getLastIDstr(IthoPacket *packetPtr, bool ashex);
  // String getLastMessagestr(bool ashex = true);
  String LastMessageDecoded(IthoPacket *packetPtr);

  // send
  const uint8_t *getRemoteCmd(const RemoteTypes type, const IthoCommand command);
  void sendRFCommand(uint8_t remote_index, IthoCommand command);
  int8_t sendJoinReply(uint8_t remote_index);
  void send10E0();
  void send2E10(uint8_t remote_index, IthoCommand command);
  void send31D9(uint8_t speedstatus = 0, uint8_t info = 0);
  void send31D9(uint8_t *command);
  void send31DA(uint8_t faninfo = 0, uint8_t timer_remaining = 0);
  void send31DA(uint8_t *command);
  void sendRFMessage(RFmessage *message);
  void sendCommand(IthoCommand command);

  void handleBind(IthoPacket *packetPtr);
  void handleLevel(IthoPacket *packetPtr);
  void handleTimer(IthoPacket *packetPtr);
  void handle31D9(IthoPacket *packetPtr);
  void handle31DA(IthoPacket *packetPtr);
  void handleRemotestatus(IthoPacket *packetPtr);
  void handleTemphum(IthoPacket *packetPtr);
  void handleCo2(IthoPacket *packetPtr);
  void handleBattery(IthoPacket *packetPtr);
  void handlePIR(IthoPacket *packetPtr);
  void handleZoneTemp(IthoPacket *packetPtr);
  void handleZoneSetpoint(IthoPacket *packetPtr);
  void handleDeviceInfo(IthoPacket *packetPtr);
  const char *rem_cmd_to_name(IthoCommand code);

}; // IthoCC1101
