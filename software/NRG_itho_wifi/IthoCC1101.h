/*
   Author: Klusjesman, supersjimmie, modified and reworked by arjenhiemstra
*/

#pragma once

#include <cstdio>
#include <string>

#include <Ticker.h>

#include "CC1101.h"
#include "IthoPacket.h"



#define MAX_NUM_OF_REMOTES 10

struct ithoRFDevice {
  uint32_t deviceId {0};
//  char name[16];
  IthoCommand lastCommand {IthoUnknown};
  int32_t co2 {0xEFFF};
  int32_t temp {0xEFFF};
  int32_t hum {0xEFFF};
  int32_t dewpoint {0xEFFF};
  int32_t battery {0xEFFF};
};

struct ithoRFDevices {
  uint8_t count {0};
  ithoRFDevice device[MAX_NUM_OF_REMOTES];
};

//pa table settings
const uint8_t ithoPaTableSend[8] = {0x6F, 0x26, 0x2E, 0x8C, 0x87, 0xCD, 0xC7, 0xC0};
const uint8_t ithoPaTableReceive[8] = {0x6F, 0x26, 0x2E, 0x7F, 0x8A, 0x84, 0xCA, 0xC4};

class IthoPacket;

class IthoCC1101 : protected CC1101
{
  private:
    //receive
    CC1101Packet inMessage;                       //temp storage message2
    IthoPacket inIthoPacket;                        //stores last received message data

    //send
    IthoPacket outIthoPacket;                       //stores state of "remote"

    //settings
    uint8_t sendTries;                            //number of times a command is send at one button press
    uint8_t calEnabled;
    uint8_t calFinised;
    uint16_t timeoutCCcal;
    uint8_t cc_freq[3]; //FREQ0, FREQ1, FREQ2
    uint32_t f0;
    unsigned long lastValid;
    uint32_t lastF;

    //CC1101 calibration
    Ticker calibrationTask;
    void cc_cal_task();
    uint32_t cc_cal( uint8_t validMsg, bool timeout );
    void cc_cal_update( uint8_t msgError, bool timeout );

    //Itho remotes
//    std::vector<ithoRFDevices> IthoRFDevices;
    bool bindAllowed;
    bool allowAll;
    ithoRFDevices ithoRF;

    //functions
  public:
    IthoCC1101(uint8_t counter = 0, uint8_t sendTries = 3);   //set initial counter value
    ~IthoCC1101();

    //init
    void init() {
      CC1101::init();  //init,reset CC1101
      initReceive();
    }
    void initReceive();
    uint8_t getLastCounter() {
      return outIthoPacket.counter;  //counter is increased before sending a command
    }
    void setSendTries(uint8_t sendTries) {
      this->sendTries = sendTries;
    }
    void setDeviceID(uint8_t byte0, uint8_t byte1, uint8_t byte2) {
      this->outIthoPacket.deviceId[0] = byte0;
      this->outIthoPacket.deviceId[1] = byte1;
      this->outIthoPacket.deviceId[2] = byte2;
    }

    bool addRFDevice(uint8_t byte0, uint8_t byte1, uint8_t byte2);
    bool addRFDevice(uint32_t ID);
    bool removeRFDevice(uint8_t byte0, uint8_t byte1, uint8_t byte2);
    bool removeRFDevice(uint32_t ID);
    bool checkRFDevice(uint8_t byte0, uint8_t byte1, uint8_t byte2);
    bool checkRFDevice(uint32_t ID);
    void setBindAllowed(bool input) {
      bindAllowed = input;
    }
    bool getBindAllowed() {
      return bindAllowed;
    }
    void setAllowAll(bool input) {
      allowAll = input;
    }
    bool getAllowAll() {
      return allowAll;
    }
    const struct ithoRFDevices &getRFdevices() const {
      return ithoRF;
    }    
    //receive
    uint8_t receivePacket();  //read RX fifo
    bool checkForNewPacket();
    IthoPacket getLastPacket() {
      return inIthoPacket;  //retrieve last received/parsed packet from remote
    }
    IthoCommand getLastCommand() {
      return inIthoPacket.command;  //retrieve last received/parsed command from remote
    }
    uint8_t getLastInCounter() {
      return inIthoPacket.counter;  //retrieve last received/parsed command from remote
    }
    uint8_t ReadRSSI();
    bool checkID(const uint8_t *id);
    int * getLastID();
    String getLastIDstr(bool ashex = true);
    String getLastMessagestr(bool ashex = true);
    String LastMessageDecoded();

    //send
    void sendCommand(IthoCommand command);

    //calibration
    uint8_t getCCcalEnabled() {
      return calEnabled;
    }
    uint8_t getCCcalFinised() {
      return calFinised;
    }

    void setCCcalEnable( uint8_t enable );
    void abortCCcal();
    void resetCCcal();

    void setCCcal(uint32_t F);
    uint32_t getCCcal() {
      return ( (uint32_t)cc_freq[2] << 16 ) | ( (uint32_t)cc_freq[1] <<  8 ) | ( (uint32_t)cc_freq[0] <<  0 );
    }

    void setCCcalTimeout( uint16_t timeoutCCcal );
    uint16_t getCCcalTimeout() {
      return timeoutCCcal;
    }
    uint32_t getCCcalTimer() {
      return (millis() - lastValid) > timeoutCCcal ? 0 : timeoutCCcal - (millis() - lastValid);
    }
    void handleBind();
    void handleLevel();
    void handleTimer();
    void handleStatus();
    void handleRemotestatus();
    void handleTemphum();
    void handleCo2();
    void handleBattery();

  protected:
  private:
    IthoCC1101( const IthoCC1101 &c);
    IthoCC1101& operator=( const IthoCC1101 &c);

    //init CC1101 for receiving
    void initReceiveMessage();

    //init CC1101 for sending
    void initSendMessage(uint8_t len);
    void finishTransfer();

    //parse received message
    bool parseMessageCommand();
    bool checkIthoCommand(IthoPacket *itho, const uint8_t commandBytes[]);

    //send
    void createMessageStart(IthoPacket *itho, CC1101Packet *packet);
    void createMessageCommand(IthoPacket *itho, CC1101Packet *packet);
    void createMessageJoin(IthoPacket *itho, CC1101Packet *packet);
    void createMessageLeave(IthoPacket *itho, CC1101Packet *packet);
    uint8_t* getMessageCommandBytes(IthoCommand command);
    uint8_t getCounter2(IthoPacket *itho, uint8_t len);

    uint8_t messageEncode(IthoPacket *itho, CC1101Packet *packet);
    void messageDecode(CC1101Packet *packet, IthoPacket *itho);


}; //IthoCC1101
