/*
   Author: Klusjesman, supersjimmie, modified and reworked by arjenhiemstra
*/

#pragma once

#include <cstdio>
#include <string>

#include "CC1101.h"
#include "IthoPacket.h"
#include "CC1101Packet.h"
#include "RFSignal.h"

#if !defined(MAX_NUM_OF_REMOTES)
#define MAX_NUM_OF_REMOTES 12
#endif
struct ithoRFDevice
{
  uint8_t sourceID[3]{};      // ID of the remote the software is emulating
  uint8_t destinationID[3]{}; // ID of the destination RF device (ie. CVE), only used for bi-directional communication
  RemoteTypes remType{RemoteTypes::RFTCVE};
  //  char name[16];
  IthoCommand lastCommand{IthoUnknown};
  time_t timestamp{};
  uint8_t counter{};
  bool bidirectional{false};
  int32_t co2{0xEFFF};
  int32_t temp{0xEFFF};
  int32_t setpoint{0xEFFF};
  int32_t setpointCool{0xEFFF}; // Spider 01FF second setpoint
  uint8_t spiderFlags{0xEF};    // Spider 01FF byte 5
  uint8_t demand{0xFF};         // Spider 0008 relay/heat demand, 0-200 (0.5 %)
  bool thermostat{false};       // Spider heating side bound to us: answer its RQ 01FF
  int32_t setpointCmd{0xEFFF};  // commanded heating setpoint (0.01 C), 0xEFFF = echo the Spider's own
  uint8_t controllerID[3]{};    // Spider's heater/gateway: W 01FF is sent as this id; 0 = our own id
  uint8_t zone{0xFF};           // Spider on a zone controller (controllerID): its zone, 0xFF = none
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
// Low power PA table for close-range communication
// CC1101 868MHz PA values: 0x03=-30, 0x26=-15, 0x34=-6, 0x60=0, 0x84=+5, 0xC5=+7, 0xC0=+10 dBm
const uint8_t ithoPaTableSendLow[8] = {0x6F, 0x26, 0x2E, 0x8C, 0x87, 0xCD, 0xC7, 0x03};

class IthoPacket;

typedef void (*RFStatusCallback_t)(const uint8_t *payload, uint8_t len, uint8_t srcId0, uint8_t srcId1, uint8_t srcId2);

class IthoCC1101 : protected CC1101
{
private:
  uint8_t chipVersion{};
  uint8_t detectAttempt{};

  // receive
  CC1101Packet inMessage[2];
  uint8_t lastRssi{0}, lastLqi{0}, lastFreqest{0}; // signal status of the packet last decoded
  volatile uint8_t inMessageIdx{0}; // receivePacket fills inMessage[inMessageIdx], task decodes from inMessage[1 - inMessageIdx]
  IthoPacket inPacket;

  // send
  uint8_t defaultID[3]{33, 66, 99};

  IthoPacket outIthoPacket; // stores state of "remote"
  uint8_t counter{};
  uint8_t CC1101MessageLen{};
  uint8_t IthoPacketLen{};
  // settings
  uint8_t sendTries;  // number of times a command is send at one button press
  bool lowPowerTx{false}; // use low TX power for close-range communication
  uint8_t txPowerLevel{0xC0}; // per-remote TX power level (PA table byte 7)
  uint8_t cc_freq[3]; // FREQ0, FREQ1, FREQ2

  // Itho remotes
  bool bindAllowed;
  bool allowAll;
  bool bindInitiatorActive{false};
  uint8_t bindInitiatorRemIndex{0};

  static RFStatusCallback_t rf31DACallback;
  static RFStatusCallback_t rf31D9Callback;
  ithoRFDevices ithoRF;
  bool remoteStatusChanged{false}; // set by handlers whose data should be published right away
  bool spiderReplyPending{false};
  uint8_t spiderReplyID[3]{};
  ithoRFDevice *findRFDevice(const uint8_t id[3]);
  ithoRFDevice *findZoneDevice(uint32_t ctl, uint8_t zone);
  void ownID(const ithoRFDevice &dev, uint8_t out[3]) const;
  void sendToSpider(const uint8_t id[3], uint8_t header, const uint8_t *command);
  void sendToSpiderAs(const uint8_t from[3], const uint8_t id[3], uint8_t header, const uint8_t *command);

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
  void setLowPowerMode(bool lowPower);
  void setTxPowerLevel(uint8_t power);
  void setDefaultID(uint8_t byte0, uint8_t byte1, uint8_t byte2)
  {
    defaultID[0] = byte0;
    defaultID[1] = byte1;
    defaultID[2] = byte2;
  }
  bool addRFDevice(uint8_t byte0, uint8_t byte1, uint8_t byte2, RemoteTypes deviceType, bool bidirectional = false);
  bool updateRFDevice(uint8_t remote_index, uint8_t byte0, uint8_t byte1, uint8_t byte2, RemoteTypes deviceType, bool bidirectional = false);
  // updateRFSourceID
  bool updateSourceID(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t remote_index);
  // updateRFDestinationID
  bool updateDestinationID(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t remote_index);
  String getSourceID(uint8_t remote_index);
  String getDestinationID(uint8_t remote_index);

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
  // Itho Spider heating thermostat role (ADR-0011); devices are looked up by id, not slot index
  bool setThermostat(const uint8_t id[3], bool on, int32_t setpointCmd, const uint8_t ctl[3] = nullptr, uint8_t zone = 0xFF);
  bool setThermostatSetpoint(const uint8_t id[3], int32_t setpointCmd);
  bool consumeSpiderReply(uint8_t id[3]);
  void sendSpiderBindAccept(const uint8_t id[3]);
  void sendSpiderStatus(const uint8_t id[3], bool reply, bool useCmd = true);
  void sendSpiderZoneEcho(const uint8_t id[3], int32_t setpointCenti);
  void sendSpiderRequest(const uint8_t id[3]);
  void sendSpiderZoneSetpoint(const uint8_t id[3]); // W 22C9 as the Spider to its zone controller
  void sendSpiderProbe(const uint8_t id[3], const uint8_t ctl[3]); // RQ 01FF as ctl, Spider not registered
  bool consumeRemoteStatusChanged()
  {
    bool c = remoteStatusChanged;
    remoteStatusChanged = false;
    return c;
  }
  void setBindInitiatorActive(bool active, uint8_t remIndex = 0)
  {
    bindInitiatorActive = active;
    bindInitiatorRemIndex = remIndex;
  }
  bool isBindInitiatorActive() const
  {
    return bindInitiatorActive;
  }
  uint8_t getBindInitiatorRemIndex() const
  {
    return bindInitiatorRemIndex;
  }
  // receive
  bool receivePacket(); // read RX fifo
  // signal diagnostics of the packet last decoded (RF debug log)
  int lastRssiDbm() const { return rfsignal::rssiDbm(lastRssi); }
  uint8_t lastLinkQuality() const { return rfsignal::linkQuality(lastLqi); }
  long lastFreqOffsetHz() const { return rfsignal::freqOffsetHz(lastFreqest); }
  void decodeBufferedPacket(); // decode raw packet buffered by receivePacket
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
  uint8_t getDetectAttempt() const { return detectAttempt; } // 0 plain, 1 after reset, 2 at 1 MHz SPI, 3 not found
  // bool checkID(const uint8_t *id);
  uint8_t *getLastID(IthoPacket *packetPtr);
  String getLastIDstr(IthoPacket *packetPtr, bool ashex);
  // String getLastMessagestr(bool ashex = true);
  String LastMessageDecoded(IthoPacket *packetPtr);

  // send
  const uint8_t *getRemoteCmd(const RemoteTypes type, const IthoCommand command);
  void sendRFCommand(uint8_t remote_index, IthoCommand command);
  int8_t sendJoinReply(uint8_t remote_index);
  void sendBindConfirm(uint8_t remote_index);
  void send1060();
  void send1298(uint8_t remote_index, uint16_t co2level);
  void send12A0(uint8_t remote_index, uint8_t humidity, int16_t temperature, int16_t dewpoint);
  void send31E0(uint8_t remote_index, uint8_t zone = 0, uint8_t demand = 0, uint8_t flags = 0);
  void sendRQ31DA(uint8_t remote_index);
  void sendRQ31D9(uint8_t remote_index);
  void send10E0();
  void send2E10(uint8_t remote_index, IthoCommand command);
  void send31D9(uint8_t speedstatus = 0, uint8_t info = 0);
  void send31D9(uint8_t *command);
  void send31DA(uint8_t faninfo = 0, uint8_t timer_remaining = 0);
  void send31DA(uint8_t *command);
  void send31DATo(uint8_t *command, const uint8_t to[3], bool reply); // addressed: RP (reply) or I (acknowledge)
  bool isOwnID(uint32_t id) const { return id == ((uint32_t)defaultID[0] << 16 | (uint32_t)defaultID[1] << 8 | defaultID[2]); }
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
  void handleSpiderStatus(IthoPacket *packetPtr);
  void handleRelayDemand(IthoPacket *packetPtr);
  const char *rem_cmd_to_name(IthoCommand code);

  static void setRF31DACallback(RFStatusCallback_t cb) { rf31DACallback = cb; }
  static void setRF31D9Callback(RFStatusCallback_t cb) { rf31D9Callback = cb; }

}; // IthoCC1101
