/*
   Author: Klusjesman, supersjimmie, modified and reworked by arjenhiemstra
*/

#include "IthoCC1101.h"
#include "IthoPacket.h"
#include <string>
#include <Arduino.h>
#include <SPI.h>

// #define CRC_FILTER

////original sync byte pattern
// #define STARTBYTE 6 //relevant data starts 6 bytes after the sync pattern bytes 170/171
// #define SYNC1 170
// #define SYNC0 171
// #define MDMCFG2 0x02 //16bit sync word / 16bit specific

// alternative sync byte pattern (filter much more non-itho messages out)
#define STARTBYTE 0 // relevant data starts 0 bytes after the sync pattern bytes 179/42/171/42
#define SYNC1 187   // byte11 = 179, byte13 = 171 with SYNC1 = 187, 179 and 171 differ only by 1 bit
#define SYNC0 42
#define MDMCFG2 0x03 // 32bit sync word / 30bit specific

// alternative sync byte pattern
// #define STARTBYTE 2 //relevant data starts 2 bytes after the sync pattern bytes 179/42
// #define SYNC1 179
// #define SYNC0 42
// #define MDMCFG2 0x02 //16bit sync word / 16bit specific

// alternative sync byte pattern
// #define STARTBYTE 1 //relevant data starts 1 byte after the sync pattern bytes 89/149/85/149
// #define SYNC1 93 //byte11 = 89, byte13 = 85 with SYNC1 = 94, 89 and 85 differ only by 1 bit and line up with the start bit
// #define SYNC0 149
// #define MDMCFG2 0x03 //32bit sync word / 30bit specific

//
// Header field (1 byte)
// The bits are used as follows <00TTAAPP> where

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

#define DEVICEID_MASK 0xC

#define MESSAGE_TYPE_RQ_MASK 0x0
#define MESSAGE_TYPE_I_MASK 0x1
#define MESSAGE_TYPE_W_MASK 0x2
#define MESSAGE_TYPE_RP_MASK 0x3

#define OPT0_MASK 0x2
#define OPT1_MASK 0x1

#define HEADER_REMOTE_CMD 0x16        // message type: I, addr2, param0
#define HEADER_REMOTE_1FC9 0x18       // message type: I,addr0+addr2
#define HEADER_RFT_BIDIRECTIONAL 0x1C // message type: I, addr0+addr1
#define HEADER_JOIN_REPLY 0x2C        // message type: W, addr0+addr1
#define HEADER_10E0 0x18              // message type: 1, addr0+addr2
#define HEADER_31D9 0x1A              // message type: I, addr0+addr2, param0
#define HEADER_31DA 0x18              // message type: I, addr0+addr2
#define HEADER_2E10 0x14              // message type: I, addr2

// default constructor
IthoCC1101::IthoCC1101() : CC1101()
{

  sendTries = 3;

  cc_freq[0] = 0x6A;
  cc_freq[1] = 0x65;
  cc_freq[2] = 0x21;

  // ithoRF.device[0].sourceID[0] = defaultID[0];
  // ithoRF.device[0].sourceID[1] = defaultID[1];
  // ithoRF.device[0].sourceID[2] = defaultID[2];

  bindAllowed = false;
  allowAll = true;

} // IthoCC1101

// default destructor
IthoCC1101::~IthoCC1101()
{
} //~IthoCC1101

//                                  { IthoUnknown,  IthoJoin, IthoLeave,  IthoAway, IthoLow, IthoMedium,  IthoHigh,  IthoFull, IthoTimer1,  IthoTimer2,  IthoTimer3,  IthoAuto,  IthoAutoNight, IthoCook30,  IthoCook60, IthoTimerUser, IthoJoinReply, IthoPIRmotionOn, IthoPIRmotionOff }
const uint8_t *RFTCVE_Remote_Map[] = {nullptr, ithoMessageCVERFTJoinCommandBytes, ithoMessageLeaveCommandBytes, ithoMessageAwayCommandBytes, ithoMessageLowCommandBytes, ithoMessageMediumCommandBytes, ithoMessageHighCommandBytes, nullptr, ithoMessageTimer1CommandBytes, ithoMessageTimer2CommandBytes, ithoMessageTimer3CommandBytes, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
const uint8_t *RFTAUTO_Remote_Map[] = {nullptr, ithoMessageAUTORFTJoinCommandBytes, ithoMessageAUTORFTLeaveCommandBytes, nullptr, ithoMessageAUTORFTLowCommandBytes, nullptr, ithoMessageAUTORFTHighCommandBytes, nullptr, ithoMessageAUTORFTTimer1CommandBytes, ithoMessageAUTORFTTimer2CommandBytes, ithoMessageAUTORFTTimer3CommandBytes, ithoMessageAUTORFTAutoCommandBytes, ithoMessageAUTORFTAutoNightCommandBytes, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
const uint8_t *RFTN_Remote_Map[] = {nullptr, ithoMessageAUTORFTNJoinCommandBytes, ithoMessageAUTORFTLeaveCommandBytes, ithoMessageAwayCommandBytes, ithoMessageLowCommandBytes, ithoMessageMediumCommandBytes, ithoMessageHighCommandBytes, nullptr, ithoMessageTimer1CommandBytes, ithoMessageTimer2CommandBytes, ithoMessageTimer3CommandBytes, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
const uint8_t *RFTAUTON_Remote_Map[] = {nullptr, ithoMessageAUTORFTNJoinCommandBytes, ithoMessageAUTORFTLeaveCommandBytes, nullptr, ithoMessageAUTORFTLowCommandBytes, nullptr, ithoMessageAUTORFTHighCommandBytes, nullptr, ithoMessageAUTORFTTimer1CommandBytes, ithoMessageAUTORFTTimer2CommandBytes, ithoMessageAUTORFTTimer3CommandBytes, ithoMessageAUTORFTAutoCommandBytes, ithoMessageAUTORFTAutoNightCommandBytes, nullptr, nullptr, nullptr, ithoMessageJoinReplyCommandBytes, nullptr, nullptr};
const uint8_t *DEMANDFLOW_Remote_Map[] = {nullptr, ithoMessageDFJoinCommandBytes, ithoMessageLeaveCommandBytes, nullptr, ithoMessageDFLowCommandBytes, nullptr, ithoMessageDFHighCommandBytes, nullptr, ithoMessageDFTimer1CommandBytes, ithoMessageDFTimer2CommandBytes, ithoMessageDFTimer3CommandBytes, nullptr, nullptr, ithoMessageDFCook30CommandBytes, ithoMessageDFCook60CommandBytes, nullptr, nullptr, nullptr, nullptr};
const uint8_t *RFTRV_Remote_Map[] = {nullptr, ithoMessageRVJoinCommandBytes, ithoMessageLeaveCommandBytes, nullptr, ithoMessageLowCommandBytes, ithoMessageRV_CO2MediumCommandBytes, ithoMessageHighCommandBytes, nullptr, ithoMessageRV_CO2Timer1CommandBytes, ithoMessageRV_CO2Timer2CommandBytes, ithoMessageRV_CO2Timer3CommandBytes, ithoMessageRV_CO2AutoCommandBytes, ithoMessageRV_CO2AutoNightCommandBytes, nullptr, nullptr, nullptr, ithoMessageJoinReplyCommandBytes, nullptr, nullptr};
const uint8_t *RFTCO2_Remote_Map[] = {nullptr, ithoMessageCO2JoinCommandBytes, ithoMessageLeaveCommandBytes, nullptr, ithoMessageLowCommandBytes, ithoMessageRV_CO2MediumCommandBytes, ithoMessageHighCommandBytes, nullptr, ithoMessageRV_CO2Timer1CommandBytes, ithoMessageRV_CO2Timer2CommandBytes, ithoMessageRV_CO2Timer3CommandBytes, ithoMessageRV_CO2AutoCommandBytes, ithoMessageRV_CO2AutoNightCommandBytes, nullptr, nullptr, nullptr, ithoMessageJoinReplyCommandBytes, nullptr, nullptr};
const uint8_t *RFTPIR_Remote_Map[] = {nullptr, ithoMessagePIRJoinCommandBytes, ithoMessageLeaveCommandBytes, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, ithoMessageRFTPIRonCommandBytes, ithoMessageRFTPIRoffCommandBytes};
const uint8_t *RFTSpider_Remote_Map[] = {nullptr, ithoMessageSpiderJoinCommandBytes, ithoMessageLeaveCommandBytes, nullptr, ithoMessageSpiderLowCommandBytes, ithoMessageRV_CO2MediumCommandBytes, ithoMessageSpiderHighCommandBytes, nullptr, ithoMessageRV_CO2Timer1CommandBytes, ithoMessageRV_CO2Timer2CommandBytes, ithoMessageRV_CO2Timer3CommandBytes, ithoMessageRV_CO2AutoCommandBytes, ithoMessageRV_CO2AutoNightCommandBytes, nullptr, nullptr, nullptr, ithoMessageJoinReplyCommandBytes, nullptr, nullptr}; // auto night might be wrong

struct ihtoRemoteCmdMap
{
  RemoteTypes type;
  const uint8_t **commandMapping;
};

const struct ihtoRemoteCmdMap ihtoRemoteCmdMapping[]{
    {RFTCVE, RFTCVE_Remote_Map},
    {RFTAUTO, RFTAUTO_Remote_Map},
    {RFTN, RFTN_Remote_Map},
    {RFTAUTON, RFTAUTON_Remote_Map},
    {DEMANDFLOW, DEMANDFLOW_Remote_Map},
    {RFTRV, RFTRV_Remote_Map},
    {RFTCO2, RFTCO2_Remote_Map},
    {RFTPIR, RFTPIR_Remote_Map},
    {RFTSPIDER, RFTSpider_Remote_Map}};

void IthoCC1101::initSendMessage(uint8_t len)
{
  // finishTransfer();
  writeCommand(CC1101_SIDLE);
  delayMicroseconds(1);
  writeRegister(CC1101_IOCFG0, 0x2E);
  delayMicroseconds(1);
  writeRegister(CC1101_IOCFG1, 0x2E);
  delayMicroseconds(1);
  writeCommand(CC1101_SIDLE);
  writeCommand(CC1101_SPWD);
  delayMicroseconds(2);

  /*
    Configuration reverse engineered from remote print. The commands below are used by IthoDaalderop.
    Base frequency    868.299866MHz
    Channel       0
    Channel spacing   199.951172kHz
    Carrier frequency 868.299866MHz
    Xtal frequency    26.000000MHz
    Data rate     38.3835kBaud
    Manchester      disabled
    Modulation      2-FSK
    Deviation     50.781250kHz
    TX power      ?
    PA ramping      enabled
    Whitening     disabled
  */
  writeCommand(CC1101_SRES);
  delayMicroseconds(1);
  writeRegister(CC1101_IOCFG0, 0x2E);      // High impedance (3-state)
  writeRegister(CC1101_FREQ2, cc_freq[2]); // 00100001  878MHz-927.8MHz
  writeRegister(CC1101_FREQ1, cc_freq[1]); // 01100101
  writeRegister(CC1101_FREQ0, cc_freq[0]); // 01101010
  writeRegister(CC1101_MDMCFG4, 0x5A);     // difference compared to message1
  writeRegister(CC1101_MDMCFG3, 0x83);     // difference compared to message1
  writeRegister(CC1101_MDMCFG2, 0x00);     // 00000000  2-FSK, no manchester encoding/decoding, no preamble/sync
  writeRegister(CC1101_MDMCFG1, 0x22);     // 00100010
  writeRegister(CC1101_MDMCFG0, 0xF8);     // 11111000
  writeRegister(CC1101_CHANNR, 0x00);      // 00000000
  writeRegister(CC1101_DEVIATN, 0x50);     // difference compared to message1
  writeRegister(CC1101_FREND0, 0x17);      // 00010111  use index 7 in PA table
  writeRegister(CC1101_MCSM0, 0x18);       // 00011000  PO timeout Approx. 146microseconds - 171microseconds, Auto calibrate When going from IDLE to RX or TX (or FSTXON)
  writeRegister(CC1101_FSCAL3, 0xA9);      // 10101001
  writeRegister(CC1101_FSCAL2, 0x2A);      // 00101010
  writeRegister(CC1101_FSCAL1, 0x00);      // 00000000
  writeRegister(CC1101_FSCAL0, 0x11);      // 00010001
  writeRegister(CC1101_FSTEST, 0x59);      // 01011001  For test only. Do not write to this register.
  writeRegister(CC1101_TEST2, 0x81);       // 10000001  For test only. Do not write to this register.
  writeRegister(CC1101_TEST1, 0x35);       // 00110101  For test only. Do not write to this register.
  writeRegister(CC1101_TEST0, 0x0B);       // 00001011  For test only. Do not write to this register.
  writeRegister(CC1101_PKTCTRL0, 0x12);    // 00010010  Enable infinite length packets, CRC disabled, Turn data whitening off, Serial Synchronous mode
  writeRegister(CC1101_ADDR, 0x00);        // 00000000
  writeRegister(CC1101_PKTLEN, 0xFF);      // 11111111  //Not used, no hardware packet handling

  // 0x6F,0x26,0x2E,0x8C,0x87,0xCD,0xC7,0xC0
  writeBurstRegister(CC1101_PATABLE | CC1101_WRITE_BURST, ithoPaTableSend, 8);

  // difference, message1 sends a STX here
  writeCommand(CC1101_SIDLE);
  writeCommand(CC1101_SIDLE);

  writeRegister(CC1101_MDMCFG4, 0x5A); // difference compared to message1
  writeRegister(CC1101_MDMCFG3, 0x83); // difference compared to message1
  writeRegister(CC1101_DEVIATN, 0x50); // difference compared to message1
  writeRegister(CC1101_IOCFG0, 0x2D);  // GDO0_Z_EN_N. When this output is 0, GDO0 is configured as input (for serial TX data).
  writeRegister(CC1101_IOCFG1, 0x0B);  // Serial Clock. Synchronous to the data in synchronous serial mode.

  writeCommand(CC1101_STX);
  writeCommand(CC1101_SIDLE);

  writeRegister(CC1101_MDMCFG4, 0x5A); // difference compared to message1
  writeRegister(CC1101_MDMCFG3, 0x83); // difference compared to message1
  writeRegister(CC1101_DEVIATN, 0x50); // difference compared to message1
  // writeRegister(CC1101_IOCFG0 ,0x2D);   //GDO0_Z_EN_N. When this output is 0, GDO0 is configured as input (for serial TX data).
  // writeRegister(CC1101_IOCFG1 ,0x0B);   //Serial Clock. Synchronous to the data in synchronous serial mode.

  // Itho is using serial mode for transmit. We want to use the TX FIFO with fixed packet length for simplicity.
  writeRegister(CC1101_IOCFG0, 0x2E);
  writeRegister(CC1101_IOCFG1, 0x2E);
  writeRegister(CC1101_PKTCTRL0, 0x00);
  writeRegister(CC1101_PKTCTRL1, 0x00);

  writeRegister(CC1101_PKTLEN, len);
}

void IthoCC1101::finishTransfer()
{
  writeCommand(CC1101_SIDLE);
  delayMicroseconds(1);

  writeRegister(CC1101_IOCFG0, 0x2E);
  writeRegister(CC1101_IOCFG1, 0x2E);

  writeCommand(CC1101_SIDLE);
  writeCommand(CC1101_SPWD);
}

void IthoCC1101::initReceive()
{
  /*
    Configuration reverse engineered from RFT print.

    Base frequency    868.299866MHz
    Channel       0
    Channel spacing   199.951172kHz
    Carrier frequency 868.299866MHz
    Xtal frequency    26.000000MHz
    Data rate     38.3835kBaud
    RX filter BW    325.000000kHz
    Manchester      disabled
    Modulation      2-FSK
    Deviation     50.781250kHz
    TX power      0x6F,0x26,0x2E,0x7F,0x8A,0x84,0xCA,0xC4
    PA ramping      enabled
    Whitening     disabled
  */
  writeCommand(CC1101_SRES);

  writeRegister(CC1101_TEST0, 0x09);
  writeRegister(CC1101_FSCAL2, 0x00);

  // 0x6F,0x26,0x2E,0x7F,0x8A,0x84,0xCA,0xC4
  writeBurstRegister(CC1101_PATABLE | CC1101_WRITE_BURST, ithoPaTableReceive, 8);

  writeCommand(CC1101_SCAL);

  // wait for calibration to finish
  while ((readRegisterWithSyncProblem(CC1101_MARCSTATE, CC1101_STATUS_REGISTER)) != CC1101_MARCSTATE_IDLE)
    yield();

  writeRegister(CC1101_FSCAL2, 0x00);
  writeRegister(CC1101_MCSM0, 0x18); // no auto calibrate
  writeRegister(CC1101_FREQ2, cc_freq[2]);
  writeRegister(CC1101_FREQ1, cc_freq[1]);
  writeRegister(CC1101_FREQ0, cc_freq[0]);
  writeRegister(CC1101_IOCFG0, 0x2E);  // High impedance (3-state)
  writeRegister(CC1101_IOCFG2, 0x06);  // 0x06 Assert when sync word has been sent / received, and de-asserts at the end of the packet.
  writeRegister(CC1101_FSCTRL1, 0x0F); // change 06
  writeRegister(CC1101_FSCTRL0, 0x00);
  writeRegister(CC1101_MDMCFG4, 0x6A);
  writeRegister(CC1101_MDMCFG3, 0x83);
  writeRegister(CC1101_MDMCFG2, 0x10); // Enable digital DC blocking filter before demodulator, 2-FSK, Disable Manchester encoding/decoding, No preamble/sync
  writeRegister(CC1101_MDMCFG1, 0x22); // Disable FEC
  writeRegister(CC1101_MDMCFG0, 0xF8);
  writeRegister(CC1101_CHANNR, 0x00);
  writeRegister(CC1101_DEVIATN, 0x50);
  writeRegister(CC1101_FREND1, 0x56);
  writeRegister(CC1101_FREND0, 0x10);
  writeRegister(CC1101_MCSM0, 0x18); // no auto calibrate
  writeRegister(CC1101_FOCCFG, 0x16);
  writeRegister(CC1101_BSCFG, 0x6C);
  writeRegister(CC1101_AGCCTRL2, 0x43);
  writeRegister(CC1101_AGCCTRL1, 0x40);
  writeRegister(CC1101_AGCCTRL0, 0x91);
  writeRegister(CC1101_FSCAL3, 0xE9);
  writeRegister(CC1101_FSCAL2, 0x21);
  writeRegister(CC1101_FSCAL1, 0x00);
  writeRegister(CC1101_FSCAL0, 0x1F);
  writeRegister(CC1101_FSTEST, 0x59);
  writeRegister(CC1101_TEST2, 0x81);
  writeRegister(CC1101_TEST1, 0x35);
  writeRegister(CC1101_TEST0, 0x09);
  writeRegister(CC1101_PKTCTRL1, 0x04); // No address check, Append two bytes with status RSSI/LQI/CRC OK,
  writeRegister(CC1101_PKTCTRL0, 0x32); // Infinite packet length mode, CRC disabled for TX and RX, No data whitening, Asynchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins
  writeRegister(CC1101_ADDR, 0x00);
  writeRegister(CC1101_PKTLEN, 0xFF);

  writeCommand(CC1101_SCAL);

  // wait for calibration to finish
  while ((readRegisterWithSyncProblem(CC1101_MARCSTATE, CC1101_STATUS_REGISTER)) != CC1101_MARCSTATE_IDLE)
    yield();

  writeRegister(CC1101_MCSM0, 0x18); // no auto calibrate

  writeCommand(CC1101_SIDLE);
  writeCommand(CC1101_SIDLE);

  writeRegister(CC1101_MDMCFG2, 0x00); // Enable digital DC blocking filter before demodulator, 2-FSK, Disable Manchester encoding/decoding, No preamble/sync
  writeRegister(CC1101_IOCFG0, 0x0D);  // Serial Data Output. Used for asynchronous serial mode.

  writeCommand(CC1101_SRX);

  while ((readRegisterWithSyncProblem(CC1101_MARCSTATE, CC1101_STATUS_REGISTER)) != CC1101_MARCSTATE_RX)
    yield();

  initReceiveMessage();
}

void IthoCC1101::initReceiveMessage()
{
  uint8_t marcState;

  writeCommand(CC1101_SIDLE); // idle

  // set datarate
  writeRegister(CC1101_MDMCFG4, 0x6A); // set kBaud
  writeRegister(CC1101_MDMCFG3, 0x83); // set kBaud
  writeRegister(CC1101_DEVIATN, 0x50);

  // set fifo mode with fixed packet length and sync bytes
  // writeRegister(CC1101_PKTLEN , 63);      //63 bytes message (sync at beginning of message is removed by CC1101)

  // set fifo mode with fixed packet length and sync bytes
  writeRegister(CC1101_PKTCTRL0, 0x02);
  writeRegister(CC1101_SYNC1, SYNC1);
  writeRegister(CC1101_SYNC0, SYNC0);
  writeRegister(CC1101_MDMCFG2, MDMCFG2);
  writeRegister(CC1101_PKTCTRL1, 0x00);

  writeCommand(CC1101_SRX); // switch to RX state

  // Check that the RX state has been entered
  while (((marcState = readRegisterWithSyncProblem(CC1101_MARCSTATE, CC1101_STATUS_REGISTER)) & CC1101_BITS_MARCSTATE) != CC1101_MARCSTATE_RX)
  {
    if (marcState == CC1101_MARCSTATE_RXFIFO_OVERFLOW) // RX_OVERFLOW
      writeCommand(CC1101_SFRX);                       // flush RX buffer
  }
}

bool IthoCC1101::receivePacket()
{
  CC1101Packet inMessage;
  readData(&inMessage, MAX_RAW);
  messageDecode(&inMessage, &inPacket);
  initReceiveMessage();

  return true;
}

IthoPacket *IthoCC1101::checkForNewPacket()
{
  return &inPacket;
}

bool IthoCC1101::parseMessage(IthoPacket *packetPtr)
{

  packetPtr->header = 0;
  packetPtr->deviceId0 = 0;
  packetPtr->deviceId1 = 0;
  packetPtr->deviceId2 = 0;
  packetPtr->param0 = 0;
  packetPtr->param1 = 0;
  packetPtr->opcode = 0;
  packetPtr->len = 0;
  packetPtr->error = 0;
  packetPtr->payloadPos = 0;

  uint8_t dataPos = 0;
  packetPtr->error = 0;
  packetPtr->remType = RemoteTypes::UNSETTYPE;
  packetPtr->command = IthoUnknown;

  // first byte is the header of the message, this determines the structure of the rest of the message
  // The bits are used as follows <00TTAAPP>
  //  00 - Unused
  //  TT - Message type
  //  AA - Present DeviceID fields
  //  PP - Present Params
  packetPtr->header = packetPtr->dataDecoded[0];
  dataPos++;

  // packet type: RQ-Request:00, I-Inform:01, W-Write:10, RP-Response:11
  if ((packetPtr->dataDecoded[0] >> 4) > 3)
  {
    packetPtr->error = 1;
    return false;
  }

  packetPtr->deviceId0 = 0;
  packetPtr->deviceId1 = 0;
  packetPtr->deviceId2 = 0;

  // get DeviceID fields
  uint8_t idfield = (packetPtr->dataDecoded[0] >> 2) & 0x03;

  if (idfield == 0x00 || idfield == 0x02 || idfield == 0x03)
  {
    packetPtr->deviceId0 = packetPtr->dataDecoded[dataPos] << 16 | packetPtr->dataDecoded[dataPos + 1] << 8 | packetPtr->dataDecoded[dataPos + 2];
    dataPos += 3;
    if (idfield == 0x00 || idfield == 0x03)
    {
      packetPtr->deviceId1 = packetPtr->dataDecoded[dataPos] << 16 | packetPtr->dataDecoded[dataPos + 1] << 8 | packetPtr->dataDecoded[dataPos + 2];
      dataPos += 3;
    }
    if (idfield == 0x00 || idfield == 0x02)
    {
      packetPtr->deviceId2 = packetPtr->dataDecoded[dataPos] << 16 | packetPtr->dataDecoded[dataPos + 1] << 8 | packetPtr->dataDecoded[dataPos + 2];
      dataPos += 3;
    }
  }
  else
  {
    packetPtr->deviceId2 = packetPtr->dataDecoded[dataPos] << 16 | packetPtr->dataDecoded[dataPos + 1] << 8 | packetPtr->dataDecoded[dataPos + 2];
    dataPos += 3;
  }

  if (packetPtr->deviceId0 == 0 && packetPtr->deviceId1 == 0 && packetPtr->deviceId2 == 0)
    return false;

  // determine param0 present
  if (packetPtr->header & OPT0_MASK)
  {
    packetPtr->param0 = packetPtr->dataDecoded[dataPos];

    dataPos++;
  }

  // determine param1 present
  if (packetPtr->header & OPT1_MASK)
  {
    packetPtr->param1 = packetPtr->dataDecoded[dataPos];
    dataPos++;
  }

  // Get the two bytes of the opcode
  packetPtr->opcode = packetPtr->dataDecoded[dataPos] << 8 | packetPtr->dataDecoded[dataPos + 1];
  dataPos += 2;

  // Payload length
  packetPtr->len = packetPtr->dataDecoded[dataPos];
  if (packetPtr->len > MAX_PAYLOAD)
  {
    packetPtr->error = 1;
    return false;
  }

  dataPos++;
  packetPtr->payloadPos = dataPos;

  // Now we have parsed all the variable fields and know the total lenth of the message
  // with that we can determine if the message CRC is correct
  uint8_t mLen = packetPtr->payloadPos + packetPtr->len;

  if (checksum(packetPtr, mLen) != packetPtr->dataDecoded[mLen])
  {
    packetPtr->error = 2;
#if defined(CRC_FILTER)
    packetPtr->command = IthoUnknown;
    return false;
#endif
  }

  // handle command
  switch (packetPtr->opcode)
  {
  case IthoPacket::Type::BIND:
    handleBind(packetPtr);
    break;
  case IthoPacket::Type::LEVEL:
    handleLevel(packetPtr);
    break;
  case IthoPacket::Type::SETPOINT:
    handleLevel(packetPtr);
    break;
  case IthoPacket::Type::TIMER:
    handleTimer(packetPtr);
    break;
  case IthoPacket::Type::FAN31DA:
    handle31DA(packetPtr);
    break;
  case IthoPacket::Type::FAN31D9:
    handle31D9(packetPtr);
    break;
  case IthoPacket::Type::REMOTESTATUS:
    handleRemotestatus(packetPtr);
    break;
  case IthoPacket::Type::TEMPHUM:
    handleTemphum(packetPtr);
    break;
  case IthoPacket::Type::CO2:
    handleCo2(packetPtr);
    break;
  case IthoPacket::Type::BATTERY:
    handleBattery(packetPtr);
    break;
  case IthoPacket::Type::PIR:
    handlePIR(packetPtr);
    break;
  case IthoPacket::Type::ZONETEMP:
    handleZoneTemp(packetPtr);
    break;
  case IthoPacket::Type::ZONESETPOINT:
    handleZoneSetpoint(packetPtr);
    break;
  case IthoPacket::Type::DEVICEINFO:
    handleDeviceInfo(packetPtr);
    break;
    // default:
    //   return false;
  }

  return true;
}

bool IthoCC1101::checkIthoCommand(IthoPacket *itho, const uint8_t commandBytes[])
{
  int len = 6;
  if (itho->dataDecoded[itho->payloadPos - 3] == 0x1F && itho->dataDecoded[itho->payloadPos - 2] == 0xC9)
    len = 2 + itho->dataDecoded[itho->payloadPos - 1] + 1;

  for (int i = 0; i < len; i++)
  {
    // if len = 6/7/8 12/13/14 18/19/20 24/25/26 30/31/32 36/37/38 etc... continue; those are the device ID bytes on Join/Leave messages
    if (i > 5 && (i % 6 == 0 || i % 6 == 1 || i % 6 == 2))
      continue;

    if (itho->dataDecoded[i + (itho->payloadPos - 3)] != commandBytes[i])
    {
      return false;
    }
  }
  return true;
}

// sendCommand -> sendRFCommand(0, command) results in backwards compatible behaviour
void IthoCC1101::sendCommand(IthoCommand command)
{
  sendRFCommand(0, command);
}

const uint8_t *IthoCC1101::getRemoteCmd(const RemoteTypes type, const IthoCommand command)
{

  const struct ihtoRemoteCmdMap *ihtoRemoteCmdMapPtr = ihtoRemoteCmdMapping;
  const struct ihtoRemoteCmdMap *ihtoRemoteCmdMapEndPtr = ihtoRemoteCmdMapping + sizeof(ihtoRemoteCmdMapping) / sizeof(ihtoRemoteCmdMapping[0]);
  while (ihtoRemoteCmdMapPtr < ihtoRemoteCmdMapEndPtr)
  {
    if (ihtoRemoteCmdMapPtr->type == type)
    {
      return *(ihtoRemoteCmdMapPtr->commandMapping + command);
    }
    ihtoRemoteCmdMapPtr++;
  }
  return nullptr;
}

void IthoCC1101::createMessageStart(IthoPacket *itho, CC1101Packet *packet)
{

  // fixed, set start structure in data buffer manually
  for (uint8_t i = 0; i < 7; i++)
  {
    packet->data[i] = 170;
  }
  packet->data[7] = 171;
  packet->data[8] = 254;
  packet->data[9] = 0;
  packet->data[10] = 179;
  packet->data[11] = 42;
  packet->data[12] = 171;
  packet->data[13] = 42;

  //[start of command specific data]
}

void IthoCC1101::sendRFCommand(uint8_t remote_index, IthoCommand command)
{
  RFmessage message;

  uint8_t sourceId[3]{};
  if (ithoRF.device[remote_index].sourceID[0] == 0 && ithoRF.device[remote_index].sourceID[1] == 0 && ithoRF.device[remote_index].sourceID[2] == 0)
  {
    sourceId[0] = defaultID[0];
    sourceId[1] = defaultID[1];
    sourceId[2] = defaultID[2];
  }
  else
  {
    sourceId[0] = ithoRF.device[remote_index].sourceID[0];
    sourceId[1] = ithoRF.device[remote_index].sourceID[1];
    sourceId[2] = ithoRF.device[remote_index].sourceID[2];
  }

  message.deviceid0[0] = sourceId[0];
  message.deviceid0[1] = sourceId[1];
  message.deviceid0[2] = sourceId[2];

  if (ithoRF.device[remote_index].bidirectional)
  {

    if (command == IthoCommand::IthoJoin || command == IthoCommand::IthoLeave)
    {
      message.header = HEADER_REMOTE_1FC9;
      message.deviceid2[0] = sourceId[0];
      message.deviceid2[1] = sourceId[1];
      message.deviceid2[2] = sourceId[2];
    }
    else
    {

      message.header = HEADER_RFT_BIDIRECTIONAL;
      message.deviceid1[0] = ithoRF.device[remote_index].destinationID[0];
      message.deviceid1[1] = ithoRF.device[remote_index].destinationID[1];
      message.deviceid1[2] = ithoRF.device[remote_index].destinationID[2];
    }
  }
  else
  {
    message.header = HEADER_REMOTE_CMD; // header of regular itho remote commands seems always to be 0x16 (message type: I, addr2, opt0)

    message.deviceid2[0] = sourceId[0];
    message.deviceid2[1] = sourceId[1];
    message.deviceid2[2] = sourceId[2];

    ithoRF.device[remote_index].counter += 1;
    message.opt0 = ithoRF.device[remote_index].counter;
  }

  message.command = getRemoteCmd(ithoRF.device[remote_index].remType, command);

  if (!message.command)
    return;

  sendRFMessage(&message);
}

int8_t IthoCC1101::sendJoinReply(uint8_t remote_index)
{

  if (remote_index > MAX_NUM_OF_REMOTES - 1)
    return -1;

  uint8_t sourceId[3]{};
  if (ithoRF.device[remote_index].sourceID[0] == 0 && ithoRF.device[remote_index].sourceID[1] == 0 && ithoRF.device[remote_index].sourceID[2] == 0)
  {
    sourceId[0] = defaultID[0];
    sourceId[1] = defaultID[1];
    sourceId[2] = defaultID[2];
  }
  else
  {
    sourceId[0] = ithoRF.device[remote_index].sourceID[0];
    sourceId[1] = ithoRF.device[remote_index].sourceID[1];
    sourceId[2] = ithoRF.device[remote_index].sourceID[2];
  }

  RFmessage message;

  message.header = HEADER_JOIN_REPLY;

  message.deviceid0[0] = sourceId[0];
  message.deviceid0[1] = sourceId[1];
  message.deviceid0[2] = sourceId[2];

  message.deviceid1[0] = ithoRF.device[remote_index].destinationID[0];
  message.deviceid1[1] = ithoRF.device[remote_index].destinationID[1];
  message.deviceid1[2] = ithoRF.device[remote_index].destinationID[2];

  message.command = getRemoteCmd(ithoRF.device[remote_index].remType, IthoCommand::IthoJoinReply);

  if (!message.command)
    return -2;

  sendRFMessage(&message);

  return remote_index;
}

void IthoCC1101::send2E10(uint8_t remote_index, IthoCommand command)
{
  // if (remote_index > MAX_NUM_OF_REMOTES - 1)
  //   return;

  uint8_t sourceId[3]{};
  if (ithoRF.device[remote_index].sourceID[0] == 0 && ithoRF.device[remote_index].sourceID[1] == 0 && ithoRF.device[remote_index].sourceID[2] == 0)
  {
    sourceId[0] = defaultID[0];
    sourceId[1] = defaultID[1];
    sourceId[2] = defaultID[2];
  }
  else
  {
    sourceId[0] = ithoRF.device[remote_index].sourceID[0];
    sourceId[1] = ithoRF.device[remote_index].sourceID[1];
    sourceId[2] = ithoRF.device[remote_index].sourceID[2];
  }

  RFmessage message;
  message.header = HEADER_2E10; // header of regular itho remote commands seems always to be 0x16 (message type: I, addr2, opt0)

  message.deviceid2[0] = sourceId[0];
  message.deviceid2[1] = sourceId[1];
  message.deviceid2[2] = sourceId[2];

  ithoRF.device[remote_index].counter += 1;
  message.opt0 = ithoRF.device[remote_index].counter;

  message.command = getRemoteCmd(ithoRF.device[remote_index].remType, command);

  sendRFMessage(&message);
}

void IthoCC1101::send10E0()
{
  // H:18 RQ P0:-- P1:-- 96,A4,3B --,--,-- 96,A4,3B 10E0 26:00,0x00,0x01,0x00,0x1B,0x31,0x19,0x01,0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,0x0E,0x05,0x07,0xE2,0x43,0x56,0x45,0x2D,0x52,0x46,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  // if (remote_index > MAX_NUM_OF_REMOTES - 1)
  //   return;

  uint8_t sourceId[3]{};
  // if (ithoRF.device[remote_index].sourceID[0] == 0 && ithoRF.device[remote_index].sourceID[1] == 0 && ithoRF.device[remote_index].sourceID[2] == 0)
  // {
  sourceId[0] = defaultID[0];
  sourceId[1] = defaultID[1];
  sourceId[2] = defaultID[2];
  // }
  // else
  // {
  //   sourceId[0] = ithoRF.device[remote_index].sourceID[0];
  //   sourceId[1] = ithoRF.device[remote_index].sourceID[1];
  //   sourceId[2] = ithoRF.device[remote_index].sourceID[2];
  // }

  RFmessage message;

  message.header = HEADER_10E0; // 0x18 // 0b00011000

  message.deviceid0[0] = sourceId[0];
  message.deviceid0[1] = sourceId[1];
  message.deviceid0[2] = sourceId[2];

  message.deviceid2[0] = sourceId[0];
  message.deviceid2[1] = sourceId[1];
  message.deviceid2[2] = sourceId[2];

  // NRG-ITHO
  // const uint8_t command[] = {0x10, 0xE0, 0x26, 0x00, 0x00, 0x01, 0x00, 0x1B, 0x31, 0x19, 0x01, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1D, 0x04, 0x07, 0xE3, 0x4E, 0x52, 0x47, 0x2D, 0x49, 0x54, 0x48, 0x4F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  // original
  const uint8_t command[] = {0x10, 0xE0, 0x26, 0x00, 0x00, 0x01, 0x00, 0x1B, 0x31, 0x19, 0x01, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0E, 0x05, 0x07, 0xE2, 0x43, 0x56, 0x45, 0x2D, 0x52, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  message.command = &command[0];

  sendRFMessage(&message);
}

void IthoCC1101::send31D9(uint8_t speedstatus, uint8_t info)
{
  // 0x31, 0xD9, 0x11, 0x00, 0x06, 0xAD, Z, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00
  // 0x00, 0x86, 0x23, 0x07, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00
  // low:
  // 00,06,0F,00,20,20,20,20,20,20,20,20,20,20,20,20,00
  //       0x80, 0x82, 0xB1, 0xD9, 0x01, 0x10, 0x86, 0x04, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x4F
  uint8_t command[] = {0x31, 0xD9, 0x11, 0x00, 0x06, 0x00, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00};

  // command[4] = 0x06 | info;
  command[5] = speedstatus;

  send31D9(command);
}
void IthoCC1101::send31D9(uint8_t *command)
{

  RFmessage message;
  message.header = HEADER_31D9; // header of regular itho remote commands seems always to be 0x16 (message type: I, addr2, opt0)

  message.deviceid0[0] = defaultID[0];
  message.deviceid0[1] = defaultID[1];
  message.deviceid0[2] = defaultID[2];

  message.deviceid2[0] = defaultID[0];
  message.deviceid2[1] = defaultID[1];
  message.deviceid2[2] = defaultID[2];

  counter += 1;
  message.opt0 = counter;

  message.command = command;

  sendRFMessage(&message);
}
void IthoCC1101::send31DA(uint8_t faninfo, uint8_t timer_remaining)
{
  // low?
  // 0x31, 0xDA,0x1D,0x00,0xC8,0x40,0x02,0x89,0x34,0xEF,0x7F,0xFF,0x7F,0xFF,0x7F,0xFF,0x7F,0xFF,0xF8,0x08,0xEF,0x01,0xC6,0x00,0x00,0x00,0xEF,0xEF,0x7F,0xFF,0x7F,0xFF
  // command[4] == air quality
  // command[5] == air quality based on
  // command[21] == fan info (mode)
  // command[24] == timer min remaining
  //
  /*
    {0X0B, "timer 1"},
    {0X0C, "timer 2"},
    {0X0D, "timer 3"},
    {0X0E, "timer 4"},
    {0X0F, "timer 5"},
    {0X10, "timer 6"},
    {0X11, "timer 7"},
    {0X12, "timer 8"},
    {0X13, "timer 9"},
    {0X14, "timer 10"},
  */
  //
  //                   0xB1, 0xDA, 0x01, 0x1C, 0xC8, 0x40, 0x03, 0xB3, 0x38, 0xEF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0xF8, 0x08, 0xEF, 0x01, 0x01, 0x00, 0x00, 0x00, 0xEF, 0xEF, 0x7F, 0xFF, 0x7F, 0xFF
  // uint8_t orgcmd = {0x31, 0xDA, 0x1D, 0x00, 0xC8, 0x40, 0x02, 0x89, 0x34, 0xEF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0xF8, 0x08, 0xEF, 0x01, 0xC6, 0x00, 0x00, 0x00, 0xEF, 0xEF, 0x7F, 0xFF, 0x7F, 0xFF};
  uint8_t command[] = {0x31, 0xDA, 0x1D, 0x00, 0xC8, 0x40, 0xEF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0xEF, 0xF8, 0x08, 0xEF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF, 0xEF, 0x7F, 0xFF, 0x7F, 0xFF};
  command[21] = faninfo;
  if (faninfo > 0X0A && faninfo < 0X15)
  {
    command[24] = timer_remaining;
  }

  send31DA(&command[0]);
}
void IthoCC1101::send31DA(uint8_t *command)
{

  RFmessage message;
  message.header = HEADER_31DA; // 31DA request has a different structure ie.: H:0C RQ P0:-- P1:-- 97,28,55 96,A4,3B --,--,-- 31DA 01:00 (cmd:unknown)

  message.deviceid0[0] = defaultID[0];
  message.deviceid0[1] = defaultID[1];
  message.deviceid0[2] = defaultID[2];

  message.deviceid2[0] = defaultID[0];
  message.deviceid2[1] = defaultID[1];
  message.deviceid2[2] = defaultID[2];

  message.command = &command[0];

  sendRFMessage(&message);
}
//<HEADER> <addr0> <addr1> <addr2> <param0> <param1> <OPCODE> <LENGTH> <PAYLOAD> <CHECKSUM>
// rewrite: uint8_t header, uint32_t deviceid0, uint32_t deviceid1, uint32_t deviceid2, uint8_t opt0, uint8_t opt1, uint8_t opcode*, uint8_t len*, uint8_t* command
void IthoCC1101::sendRFMessage(RFmessage *message)
{

  if (message->header == 0xC0) // header not set
    return;
  if (message->command == nullptr)
    return;

  CC1101Packet CC1101Message;
  IthoPacket ithoPacket;

  uint8_t delaytime = 40;

  // itho = ithoPacket
  // packet = CC1101Message
  // set start message structure
  createMessageStart(&ithoPacket, &CC1101Message);

  uint8_t messagePos = 0;
  // set message structure, for itho RF remotes this seems to be always 0x16
  ithoPacket.dataDecoded[messagePos] = message->header;

  // set deviceIDs based on header bits xxxxAAxx
  if ((message->header & DEVICEID_MASK) ^ 0x4) // true if AA bits are 00, 10 or 11
  {
    ithoPacket.dataDecoded[++messagePos] = message->deviceid0[0];
    ithoPacket.dataDecoded[++messagePos] = message->deviceid0[1];
    ithoPacket.dataDecoded[++messagePos] = message->deviceid0[2];
  }
  if (((message->header & DEVICEID_MASK) == 0) || ((message->header & DEVICEID_MASK) == DEVICEID_MASK)) // true if AA bits are 00 or 11
  {
    ithoPacket.dataDecoded[++messagePos] = message->deviceid1[0];
    ithoPacket.dataDecoded[++messagePos] = message->deviceid1[1];
    ithoPacket.dataDecoded[++messagePos] = message->deviceid1[2];
  }
  if (((message->header & DEVICEID_MASK)) != DEVICEID_MASK) // true if AA bits are 00, 01 or 10
  {
    ithoPacket.dataDecoded[++messagePos] = message->deviceid2[0];
    ithoPacket.dataDecoded[++messagePos] = message->deviceid2[1];
    ithoPacket.dataDecoded[++messagePos] = message->deviceid2[2];
  }

  // set option fields
  if (message->header & OPT0_MASK)
  {
    ithoPacket.dataDecoded[++messagePos] = message->opt0;
  }
  if (message->header & OPT1_MASK)
  {
    ithoPacket.dataDecoded[++messagePos] = message->opt1;
  }

  // determine command length
  const uint8_t command_len = message->command[2];
  const uint16_t opcode = message->command[0] << 8 | message->command[1];

  uint8_t sourceId[3]{0};
  if (opcode == 0x1FC9)
  {
    if (message->header == HEADER_REMOTE_CMD)
    {
      sourceId[0] = message->deviceid2[0];
      sourceId[1] = message->deviceid2[1];
      sourceId[2] = message->deviceid2[2];
    }
    else if (message->header == HEADER_JOIN_REPLY || message->header == HEADER_REMOTE_1FC9)
    {
      sourceId[0] = message->deviceid0[0];
      sourceId[1] = message->deviceid0[1];
      sourceId[2] = message->deviceid0[2];
    }
  }
  for (int i = 0; i < 2 + command_len + 1; i++)
  {
    ithoPacket.dataDecoded[++messagePos] = message->command[i];
    if (i > 5 && opcode == 0x1FC9)
    {
      // command bytes locations with ID in Join/Leave messages: 6/7/8 12/13/14 18/19/20 24/25/26 30/31/32 36/37/38 etc
      if (i % 6 == 0 || i % 6 == 1 || i % 6 == 2)
      {
        ithoPacket.dataDecoded[messagePos] = sourceId[i % 6];
      }
    }
  }

  // timer could be made configurable
  // if (command == IthoTimerUser)
  // {
  //   ithoPacket.dataDecoded[10] = timer_value;
  // }

  // set checksum (used to be called counter2)
  ++messagePos;
  ithoPacket.dataDecoded[messagePos] = checksum(&ithoPacket, messagePos);

  ithoPacket.length = messagePos + 1;

  CC1101Message.length = messageEncode(&ithoPacket, &CC1101Message);
  CC1101Message.length += 1;

  // set end byte - even/uneven cmd length determines last byte?
  if (opcode == 0x1FC9)
  {
    CC1101Message.data[CC1101Message.length] = 0xCA;
  }
  else
  {
    CC1101Message.data[CC1101Message.length] = 0xAC;
  }

  CC1101Message.length += 1;

  // set end 'noise'
  for (uint8_t i = CC1101Message.length; i < CC1101Message.length + 7; i++)
  {
    CC1101Message.data[i] = 0xAA;
  }
  CC1101Message.length += 7;

  CC1101MessageLen = CC1101Message.length;
  IthoPacketLen = ithoPacket.length;

  // send messages
  for (int i = 0; i < sendTries; i++)
  {

    // message2
    initSendMessage(CC1101Message.length);
    sendData(&CC1101Message);

    finishTransfer();
    delay(delaytime);
  }
  initReceive();
}

/*
   Counter2 is the decimal sum of all bytes in decoded form from
   deviceType up to the last byte before counter2 subtracted
   from zero.
*/
uint8_t IthoCC1101::checksum(IthoPacket *itho, uint8_t len)
{

  uint8_t val = 0;

  for (uint8_t i = 0; i < len; i++)
  {
    val += itho->dataDecoded[i];
  }

  return 0 - val;
}

uint8_t IthoCC1101::messageEncode(IthoPacket *itho, CC1101Packet *packet)
{

  uint8_t out_bytecounter = 14;   // index of Outbuf, start at offset 14, first part of the message is set manually
  uint8_t out_bitcounter = 0;     // bit position of current outbuf byte
  uint8_t out_patterncounter = 0; // bit counter to add 1 0 bit pattern after every 8 bits
  uint8_t bitSelect = 4;          // bit position of the inData byte (4 - 7, 0 - 3)
  uint8_t out_shift = 7;          // bit shift inData bit in position of outbuf byte

  // we need to zero the out buffer first cause we are using bitshifts
  for (unsigned int i = out_bytecounter; i < sizeof(packet->data) / sizeof(packet->data[0]); i++)
  {
    packet->data[i] = 0;
  }

  for (uint8_t dataByte = 0; dataByte < itho->length; dataByte++)
  {
    for (uint8_t dataBit = 0; dataBit < 8; dataBit++)
    { // process a full dataByte at a time resulting in 20 output bits (2.5 bytes) with the pattern 7x6x5x4x 10 3x2x1x0x 10 7x6x5x4x 10 3x2x1x0x 10 etc
      if (out_bitcounter == 8)
      { // check if new byte is needed
        out_bytecounter++;
        out_bitcounter = 0;
      }

      if (out_patterncounter == 8)
      { // check if we have to start with a 1 0 pattern
        out_patterncounter = 0;
        packet->data[out_bytecounter] = packet->data[out_bytecounter] | 1 << out_shift;
        out_shift--;
        out_bitcounter++;
        packet->data[out_bytecounter] = packet->data[out_bytecounter] | 0 << out_shift;
        if (out_shift == 0)
          out_shift = 8;
        out_shift--;
        out_bitcounter++;
      }

      if (out_bitcounter == 8)
      { // check if new byte is needed
        out_bytecounter++;
        out_bitcounter = 0;
      }

      // set the even bit
      uint8_t bit = (itho->dataDecoded[dataByte] & (1 << bitSelect)) >> bitSelect; // select bit and shift to bit pos 0
      bitSelect++;
      if (bitSelect == 8)
        bitSelect = 0;

      packet->data[out_bytecounter] = packet->data[out_bytecounter] | bit << out_shift; // shift bit in corect pos of current outbuf byte
      out_shift--;
      out_bitcounter++;
      out_patterncounter++;

      // set the odd bit (inverse of even bit)
      bit = ~bit & 0b00000001;
      packet->data[out_bytecounter] = packet->data[out_bytecounter] | bit << out_shift;
      if (out_shift == 0)
        out_shift = 8;
      out_shift--;
      out_bitcounter++;
      out_patterncounter++;
    }
  }
  if (out_bitcounter < 8)
  { // add closing 1 0 pattern to fill last packet->data byte and ensure DC balance in the message
    for (uint8_t i = out_bitcounter; i < 8; i += 2)
    {
      packet->data[out_bytecounter] = packet->data[out_bytecounter] | 1 << out_shift;
      out_shift--;
      packet->data[out_bytecounter] = packet->data[out_bytecounter] | 0 << out_shift;
      if (out_shift == 0)
        out_shift = 8;
      out_shift--;
    }
  }

  return out_bytecounter;
}

void IthoCC1101::messageDecode(CC1101Packet *packet, IthoPacket *packetPtr)
{

  packetPtr->length = 0;
  int lenInbuf = packet->length;

  lenInbuf -= STARTBYTE; // correct for sync byte pos

  while (lenInbuf >= 5)
  {
    lenInbuf -= 5;
    packetPtr->length += 2;
  }
  if (lenInbuf >= 3)
  {
    packetPtr->length++;
  }

  for (unsigned int i = 0; i < sizeof(packetPtr->dataDecoded) / sizeof(packetPtr->dataDecoded[0]); i++)
  {
    packetPtr->dataDecoded[i] = 0;
  }
  // for (unsigned int i = 0; i < sizeof(packetPtr->dataDecodedChk) / sizeof(packetPtr->dataDecodedChk[0]); i++)
  // {
  //   packetPtr->dataDecodedChk[i] = 0;
  // }

  uint8_t out_i = 0;         // byte index
  uint8_t out_j = 4;         // bit index
  uint8_t out_i_chk = 0;     // byte index
  uint8_t out_j_chk = 4;     // bit index
  uint8_t in_bitcounter = 0; // process per 10 input bits

  for (int i = STARTBYTE; i < packet->length; i++)
  {

    for (int j = 7; j > -1; j--)
    {
      if (in_bitcounter == 0 || in_bitcounter == 2 || in_bitcounter == 4 || in_bitcounter == 6)
      {                              // select input bits for output
        uint8_t x = packet->data[i]; // select input byte
        x = x >> j;                  // select input bit
        x = x & 0b00000001;
        x = x << out_j; // set value for output bit
        packetPtr->dataDecoded[out_i] = packetPtr->dataDecoded[out_i] | x;
        out_j += 1; // next output bit
        if (out_j > 7)
          out_j = 0;
        if (out_j == 4)
          out_i += 1;
      }
      if (in_bitcounter == 1 || in_bitcounter == 3 || in_bitcounter == 5 || in_bitcounter == 7)
      {                              // select input bits for check output
        uint8_t x = packet->data[i]; // select input byte
        x = x >> j;                  // select input bit
        x = x & 0b00000001;
        x = x << out_j_chk; // set value for output bit
        // packetPtr->dataDecodedChk[out_i_chk] = packetPtr->dataDecodedChk[out_i_chk] | x;
        out_j_chk += 1; // next output bit
        if (out_j_chk > 7)
          out_j_chk = 0;
        if (out_j_chk == 4)
        {
          // packetPtr->dataDecodedChk[out_i_chk] = ~packetPtr->dataDecodedChk[out_i_chk]; // inverse bits
          out_i_chk += 1;
        }
      }
      in_bitcounter += 1; // continue cyling in groups of 10 bits
      if (in_bitcounter > 9)
        in_bitcounter = 0;
    }
  }
  // clear packet data
  for (unsigned int i = 0; i < sizeof(packet->data) / sizeof(packet->data[0]); i++)
  {
    packet->data[i] = 0;
  }
}

uint8_t IthoCC1101::ReadRSSI()
{
  uint8_t rssi = 0;
  uint8_t value = 0;

  rssi = (readRegister(CC1101_RSSI, CC1101_STATUS_REGISTER));

  if (rssi >= 128)
  {
    value = 255 - rssi;
    value /= 2;
    value += 74;
  }
  else
  {
    value = rssi / 2;
    value += 74;
  }
  return (value);
}

uint8_t IthoCC1101::getChipVersion()
{
  // try to find the CC1101 chip
  uint8_t i = 0;
  bool flagFound = false;
  while ((i < 10) && !flagFound)
  {
    chipVersion = CC1101::getChipVersion();
    if ((chipVersion == CC1101_VERSION_CURRENT) || (chipVersion == CC1101_VERSION_LEGACY) || (chipVersion == CC1101_VERSION_CLONE))
    {
      flagFound = true;
    }
    else
    {
      delay(10);
      i++;
    }
  }

  return chipVersion;
}

// bool IthoCC1101::checkID(const uint8_t *id)
// {
//   for (uint8_t i = 0; i < 3; i++)
//     if (id[i] != packetPtr->deviceId2[i])
//       return false;
//   return true;
// }

String IthoCC1101::getLastIDstr(IthoPacket *packetPtr, bool ashex)
{
  String str;
  uint32_t tempID = 0;
  if (packetPtr->deviceId0 != 0)
    tempID = packetPtr->deviceId0;
  else if (packetPtr->deviceId2 != 0)
    tempID = packetPtr->deviceId2;

  int8_t id[3];
  id[0] = tempID >> 16 & 0xFF;
  id[1] = tempID >> 8 & 0xFF;
  id[2] = tempID & 0xFF;

  for (uint8_t i = 0; i < 3; i++)
  {
    if (ashex)
      str += String(id[i], HEX);
    else
      str += String(id[i]);
    if (i < 2)
      str += ",";
  }
  return str;
}

uint8_t *IthoCC1101::getLastID(IthoPacket *packetPtr)
{
  uint32_t tempID = 0;
  if (packetPtr->deviceId0 != 0)
    tempID = packetPtr->deviceId0;
  else if (packetPtr->deviceId2 != 0)
    tempID = packetPtr->deviceId2;

  static uint8_t id[3];
  id[0] = tempID >> 16 & 0xFF;
  id[1] = tempID >> 8 & 0xFF;
  id[2] = tempID & 0xFF;

  return id;
}

// String IthoCC1101::getLastMessagestr(bool ashex)
// {
//   String str = "Length=" + String(inMessage.length) + ".";
//   for (uint8_t i = 0; i < inMessage.length; i++)
//   {
//     if (ashex)
//       str += String(inMessage.data[i], HEX);
//     else
//       str += String(inMessage.data[i]);
//     if (i < inMessage.length - 1)
//       str += ":";
//   }
//   return str;
// }

String IthoCC1101::LastMessageDecoded(IthoPacket *packetPtr)
{

  String str;

  if (packetPtr->length > 11)
  {

    char bufhead[10]{};
    snprintf(bufhead, sizeof(bufhead), "H:%02X ", packetPtr->header);
    str += String(bufhead);

    str += String(MsgType[(packetPtr->header >> 4) & 0x3]);

    str += " P0:";
    if (packetPtr->header & 2)
    {
      char buf[10]{};
      snprintf(buf, sizeof(buf), "%02X ", packetPtr->param0);
      str += String(buf);
    }
    else
    {
      str += "--";
    }

    str += " P1:";
    if (packetPtr->header & 1)
    {
      char buf[10]{};
      snprintf(buf, sizeof(buf), "%02X ", packetPtr->param1);
      str += String(buf);
    }
    else
    {
      str += "--";
    }

    if (packetPtr->deviceId0 == 0)
    {
      str += " --,--,--";
    }
    else
    {
      str += " ";
      char buf[10]{};
      snprintf(buf, sizeof(buf), "%02X,%02X,%02X", static_cast<uint8_t>(packetPtr->deviceId0 >> 16 & 0xFF), static_cast<uint8_t>(packetPtr->deviceId0 >> 8 & 0xFF), static_cast<uint8_t>(packetPtr->deviceId0 & 0xFF));
      str += String(buf);
    }
    if (packetPtr->deviceId1 == 0)
    {
      str += " --,--,--";
    }
    else
    {
      str += " ";
      char buf[10]{};
      snprintf(buf, sizeof(buf), "%02X,%02X,%02X", static_cast<uint8_t>(packetPtr->deviceId1 >> 16 & 0xFF), static_cast<uint8_t>(packetPtr->deviceId1 >> 8 & 0xFF), static_cast<uint8_t>(packetPtr->deviceId1 & 0xFF));
      str += String(buf);
    }
    if (packetPtr->deviceId2 == 0)
    {
      str += " --,--,--";
    }
    else
    {
      str += " ";
      char buf[10]{};
      snprintf(buf, sizeof(buf), "%02X,%02X,%02X", static_cast<uint8_t>(packetPtr->deviceId2 >> 16 & 0xFF), static_cast<uint8_t>(packetPtr->deviceId2 >> 8 & 0xFF), static_cast<uint8_t>(packetPtr->deviceId2 & 0xFF));
      str += String(buf);
    }

    str += " ";

    char buf[10]{};
    snprintf(buf, sizeof(buf), "%04X", packetPtr->opcode);
    str += String(buf);

    str += " ";
    snprintf(buf, sizeof(buf), "%02X", packetPtr->len);
    str += String(buf);
    str += ":";

    for (int i = packetPtr->payloadPos; i < packetPtr->payloadPos + packetPtr->len; i++)
    {
      snprintf(buf, sizeof(buf), "%02X", packetPtr->dataDecoded[i]);
      str += String(buf);
      if (i < packetPtr->payloadPos + packetPtr->len - 1)
      {
        str += ",";
      }
    }
    //str += "\n";
  }
  else
  {
    for (uint8_t i = 0; i < packetPtr->length; i++)
    {
      char buf[10]{};
      snprintf(buf, sizeof(buf), "%02X", packetPtr->dataDecoded[i]);
      str += String(buf);
      if (i < packetPtr->length - 1)
        str += ",";
    }
  }
  //str += "\n";
  return str;
}

bool IthoCC1101::addRFDevice(uint8_t byte0, uint8_t byte1, uint8_t byte2, RemoteTypes remType, bool bidirectional)
{
  if (!bindAllowed)
    return false;
  if (checkRFDevice(byte0, byte1, byte2))
    return false; // device already registered

  for (auto &item : ithoRF.device)
  {
    if (item.destinationID[0] == 0 && item.destinationID[1] == 0 && item.destinationID[2] == 0)
    { // pick the first available slot
      item.destinationID[0] = byte0;
      item.destinationID[1] = byte1;
      item.destinationID[2] = byte2;
      item.remType = remType;
      item.bidirectional = bidirectional;
      ithoRF.count++;
      return true;
    }
  }
  return false;
}

bool IthoCC1101::updateSourceID(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t remote_index)
{
  if (remote_index > MAX_NUM_OF_REMOTES - 1)
    return false;

  ithoRF.device[remote_index].sourceID[0] = byte0;
  ithoRF.device[remote_index].sourceID[1] = byte1;
  ithoRF.device[remote_index].sourceID[2] = byte2;

  return true;
}

bool IthoCC1101::updateDestinationID(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t remote_index)
{
  if (remote_index > MAX_NUM_OF_REMOTES - 1)
    return false;

  ithoRF.device[remote_index].destinationID[0] = byte0;
  ithoRF.device[remote_index].destinationID[1] = byte1;
  ithoRF.device[remote_index].destinationID[2] = byte2;

  return true;
}

String IthoCC1101::getSourceID(uint8_t remote_index)
{
  if (remote_index > MAX_NUM_OF_REMOTES - 1)
    return "remote_index out of bounds";

  String str;

  for (uint8_t i = 0; i < 3; i++)
  {
    str += String(ithoRF.device[remote_index].sourceID[i], HEX);

    if (i < 2)
      str += ",";
  }
  return str;
}

String IthoCC1101::getDestinationID(uint8_t remote_index)
{
  if (remote_index > MAX_NUM_OF_REMOTES - 1)
    return "remote_index out of bounds";

  String str;
  for (uint8_t i = 0; i < 3; i++)
  {
    str += String(ithoRF.device[remote_index].destinationID[i], HEX);

    if (i < 2)
      str += ",";
  }
  return str;
}

bool IthoCC1101::updateRFDevice(uint8_t remote_index, uint8_t byte0, uint8_t byte1, uint8_t byte2, RemoteTypes deviceType, bool bidirectional)
{
  if (remote_index > MAX_NUM_OF_REMOTES - 1)
    return false;

  ithoRF.device[remote_index].destinationID[0] = byte0;
  ithoRF.device[remote_index].destinationID[1] = byte1;
  ithoRF.device[remote_index].destinationID[2] = byte2;
  ithoRF.device[remote_index].remType = deviceType;
  ithoRF.device[remote_index].bidirectional = bidirectional;
  ithoRF.device[remote_index].lastCommand = IthoUnknown;
  ithoRF.device[remote_index].timestamp = (time_t)0;
  ithoRF.device[remote_index].co2 = 0xEFFF;
  ithoRF.device[remote_index].temp = 0xEFFF;
  ithoRF.device[remote_index].hum = 0xEFFF;
  ithoRF.device[remote_index].dewpoint = 0xEFFF;
  ithoRF.device[remote_index].battery = 0xEFFF;

  return true;
}

bool IthoCC1101::updateRFDeviceType(RemoteTypes deviceType, uint8_t remote_index)
{
  if (remote_index > MAX_NUM_OF_REMOTES - 1)
    return false;

  ithoRF.device[remote_index].remType = deviceType;

  return true;
}

bool IthoCC1101::removeRFDevice(uint8_t byte0, uint8_t byte1, uint8_t byte2)
{
  if (!bindAllowed)
    return false;
  if (!checkRFDevice(byte0, byte1, byte2))
    return false; // device not registered

  for (auto &item : ithoRF.device)
  {
    if (item.destinationID[0] == byte0 && item.destinationID[1] == byte1 && item.destinationID[2] == byte2)
    {
      item.sourceID[0] = 0;
      item.sourceID[1] = 0;
      item.sourceID[2] = 0;
      item.destinationID[0] = 0;
      item.destinationID[1] = 0;
      item.destinationID[2] = 0;

      //      strlcpy(item.name, "", sizeof(item.name));
      item.remType = RemoteTypes::UNSETTYPE;
      item.bidirectional = false;
      item.lastCommand = IthoUnknown;
      item.timestamp = (time_t)0;
      item.co2 = 0xEFFF;
      item.temp = 0xEFFF;
      item.hum = 0xEFFF;
      item.dewpoint = 0xEFFF;
      item.battery = 0xEFFF;
      ithoRF.count--;
      return true;
    }
  }

  return false; // not found
}

bool IthoCC1101::removeRFDevice(uint8_t remote_index)
{
  if (remote_index > MAX_NUM_OF_REMOTES - 1)
    return false;
  ithoRF.device[remote_index].sourceID[0] = 0;
  ithoRF.device[remote_index].sourceID[1] = 0;
  ithoRF.device[remote_index].sourceID[2] = 0;
  ithoRF.device[remote_index].destinationID[0] = 0;
  ithoRF.device[remote_index].destinationID[1] = 0;
  ithoRF.device[remote_index].destinationID[2] = 0;
  ithoRF.device[remote_index].remType = RemoteTypes::UNSETTYPE;
  ithoRF.device[remote_index].bidirectional = false;
  ithoRF.device[remote_index].lastCommand = IthoUnknown;
  ithoRF.device[remote_index].timestamp = (time_t)0;
  ithoRF.device[remote_index].co2 = 0xEFFF;
  ithoRF.device[remote_index].temp = 0xEFFF;
  ithoRF.device[remote_index].hum = 0xEFFF;
  ithoRF.device[remote_index].dewpoint = 0xEFFF;
  ithoRF.device[remote_index].battery = 0xEFFF;

  return true;
}

bool IthoCC1101::checkRFDevice(uint8_t byte0, uint8_t byte1, uint8_t byte2)
{
  for (auto &item : ithoRF.device)
  {
    if (item.destinationID[0] == byte0 && item.destinationID[1] == byte1 && item.destinationID[2] == byte2)
    {
      return true;
    }
  }
  return false;
}
void IthoCC1101::setRFDeviceBidirectional(uint8_t remote_index, bool bidirectional)
{
  if (remote_index > MAX_NUM_OF_REMOTES - 1)
    return;

  ithoRF.device[remote_index].bidirectional = bidirectional;
}
bool IthoCC1101::getRFDeviceBidirectional(uint8_t remote_index)
{
  if (remote_index > MAX_NUM_OF_REMOTES - 1)
    return false;

  return ithoRF.device[remote_index].bidirectional;
}
bool IthoCC1101::getRFDeviceBidirectionalByID(uint8_t byte0, uint8_t byte1, uint8_t byte2)
{
  int8_t remote_index = getRemoteIndexByID(byte0, byte1, byte2);

  if (remote_index >= 0)
  {
    return ithoRF.device[remote_index].bidirectional;
  }
  return false;
}

int8_t IthoCC1101::getRemoteIndexByID(uint8_t byte0, uint8_t byte1, uint8_t byte2)
{
  int8_t index = 0;
  for (auto &item : ithoRF.device)
  {
    if (item.destinationID[0] == byte0 && item.destinationID[1] == byte1 && item.destinationID[2] == byte2)
    {
      return index;
    }
    index++;
  }
  return -1;
}

void IthoCC1101::handleBind(IthoPacket *packetPtr)
{
  uint32_t tempID = 0;
  if (packetPtr->deviceId0 != 0)
    tempID = packetPtr->deviceId0;
  else if (packetPtr->deviceId2 != 0)
    tempID = packetPtr->deviceId2;
  else
    return;

  uint8_t byte0 = tempID >> 16 & 0xFF;
  uint8_t byte1 = tempID >> 8 & 0xFF;
  uint8_t byte2 = tempID & 0xFF;

  if (checkIthoCommand(packetPtr, ithoMessageLeaveCommandBytes) || checkIthoCommand(packetPtr, ithoMessageAUTORFTLeaveCommandBytes))
  {
    packetPtr->command = IthoLeave;
    if (bindAllowed)
    {
      removeRFDevice(byte0, byte1, byte2);
    }
  }
  else if (checkIthoCommand(packetPtr, ithoMessageCVERFTJoinCommandBytes))
  {
    packetPtr->command = IthoJoin;
    packetPtr->remType = RemoteTypes::RFTCVE;
    if (bindAllowed)
    {
      addRFDevice(byte0, byte1, byte2, RemoteTypes::RFTCVE);
    }
  }
  else if (checkIthoCommand(packetPtr, ithoMessageAUTORFTJoinCommandBytes))
  {
    packetPtr->command = IthoJoin;
    packetPtr->remType = RemoteTypes::RFTAUTO;
    if (bindAllowed)
    {
      addRFDevice(byte0, byte1, byte2, RemoteTypes::RFTAUTO);
    }
  }
  else if (checkIthoCommand(packetPtr, ithoMessageAUTORFTNJoinCommandBytes))
  {
    packetPtr->command = IthoJoin;
    packetPtr->remType = RemoteTypes::RFTAUTON;
    if (bindAllowed)
    {
      addRFDevice(byte0, byte1, byte2, RemoteTypes::RFTAUTON);
    }
    // if (getRFDeviceBidirectionalByID(byte0, byte1, byte2))
    // {
    //   sendTries = 1;
    //   delay(10);
    //   sendJoinReply(byte0, byte1, byte2);
    //   delay(10);
    //   send10E0();
    //   sendTries = 3;
    // }
  }
  else if (checkIthoCommand(packetPtr, ithoMessageDFJoinCommandBytes))
  {
    packetPtr->command = IthoJoin;
    packetPtr->remType = RemoteTypes::DEMANDFLOW;
    if (bindAllowed)
    {
      addRFDevice(byte0, byte1, byte2, RemoteTypes::DEMANDFLOW);
    }
  }
  else if (checkIthoCommand(packetPtr, ithoMessageCO2JoinCommandBytes))
  {
    packetPtr->command = IthoJoin;
    packetPtr->remType = RemoteTypes::RFTCO2;
    if (bindAllowed)
    {
      addRFDevice(byte0, byte1, byte2, RemoteTypes::RFTCO2, true);
    }
    // TODO: handle join handshake
  }
  else if (checkIthoCommand(packetPtr, ithoMessageRVJoinCommandBytes))
  {
    packetPtr->command = IthoJoin;
    packetPtr->remType = RemoteTypes::RFTRV;
    if (bindAllowed)
    {
      addRFDevice(byte0, byte1, byte2, RemoteTypes::RFTRV, true);
    }
    // TODO: handle join handshake
  }
  else if (checkIthoCommand(packetPtr, ithoMessagePIRJoinCommandBytes))
  {
    packetPtr->command = IthoJoin;
    packetPtr->remType = RemoteTypes::RFTPIR;
    if (bindAllowed)
    {
      addRFDevice(byte0, byte1, byte2, RemoteTypes::RFTPIR);
    }
  }
  else if (checkIthoCommand(packetPtr, ithoMessageSpiderJoinCommandBytes))
  {
    packetPtr->command = IthoJoin;
    packetPtr->remType = RemoteTypes::RFTSPIDER;
    if (bindAllowed)
    {
      addRFDevice(byte0, byte1, byte2, RemoteTypes::RFTSPIDER, true);
    }
  }
  else if (checkIthoCommand(packetPtr, orconMessageJoinCommandBytes))
  {
    packetPtr->command = IthoJoin;
    packetPtr->remType = RemoteTypes::ORCON15LF01;
    if (bindAllowed)
    {
      addRFDevice(byte0, byte1, byte2, RemoteTypes::ORCON15LF01);
    }
    // TODO: handle join handshake
  }

  for (auto &item : ithoRF.device)
  {
    if (item.destinationID[0] == byte0 && item.destinationID[1] == byte1 && item.destinationID[2] == byte2)
    {
      item.lastCommand = packetPtr->command;
      return;
    }
  }
}

void IthoCC1101::handleLevel(IthoPacket *packetPtr)
{
  RemoteTypes remtype = RemoteTypes::UNSETTYPE;
  uint32_t tempID = 0;
  if (packetPtr->deviceId0 != 0)
    tempID = packetPtr->deviceId0;
  else if (packetPtr->deviceId2 != 0)
    tempID = packetPtr->deviceId2;
  else
    return;

  uint8_t byte0 = tempID >> 16 & 0xFF;
  uint8_t byte1 = tempID >> 8 & 0xFF;
  uint8_t byte2 = tempID & 0xFF;

  if (!allowAll)
  {
    if (!checkRFDevice(byte0, byte1, byte2))
    {
      if (checkIthoCommand(packetPtr, ithoMessageSpiderLowCommandBytes)) // makes it possible to treat unknown spider remote cmd low as join in user code, todo: implement better solution
      {
        packetPtr->command = IthoLow;
        packetPtr->remType = RemoteTypes::RFTSPIDER;
      }
      return;
    }
  }
  for (auto &item : ithoRF.device)
  {
    if (item.destinationID[0] == byte0 && item.destinationID[1] == byte1 && item.destinationID[2] == byte2)
    {
      remtype = item.remType;
    }
  }

  if (remtype == RemoteTypes::ORCON15LF01)
  {
    if (checkIthoCommand(packetPtr, orconMessageAwayCommandBytes))
    {
      packetPtr->command = IthoAway;
    }
    else if (checkIthoCommand(packetPtr, orconMessageAutoCommandBytes))
    {
      packetPtr->command = IthoAuto;
    }
    else if (checkIthoCommand(packetPtr, orconMessageButton1CommandBytes))
    {
      packetPtr->command = IthoLow;
    }
    else if (checkIthoCommand(packetPtr, orconMessageButton2CommandBytes))
    {
      packetPtr->command = IthoMedium;
    }
    else if (checkIthoCommand(packetPtr, orconMessageButton3CommandBytes))
    {
      packetPtr->command = IthoHigh;
    }
  }
  else
  {
    if (checkIthoCommand(packetPtr, ithoMessageLowCommandBytes) || checkIthoCommand(packetPtr, ithoMessageAUTORFTLowCommandBytes) || checkIthoCommand(packetPtr, ithoMessageDFLowCommandBytes))
    {
      packetPtr->command = IthoLow;
    }
    if (checkIthoCommand(packetPtr, ithoMessageSpiderLowCommandBytes))
    {
      packetPtr->command = IthoLow;
    }
    else if (checkIthoCommand(packetPtr, ithoMessageMediumCommandBytes) || checkIthoCommand(packetPtr, ithoMessageRV_CO2MediumCommandBytes))
    {
      packetPtr->command = IthoMedium;
    }
    else if (checkIthoCommand(packetPtr, ithoMessageHighCommandBytes) || checkIthoCommand(packetPtr, ithoMessageAUTORFTHighCommandBytes) || checkIthoCommand(packetPtr, ithoMessageDFHighCommandBytes))
    {
      packetPtr->command = IthoHigh;
    }
    if (checkIthoCommand(packetPtr, ithoMessageSpiderHighCommandBytes))
    {
      packetPtr->command = IthoHigh;
    }
    else if (checkIthoCommand(packetPtr, ithoMessageAwayCommandBytes))
    {
      packetPtr->command = IthoAway;
    }
    else if (checkIthoCommand(packetPtr, ithoMessageRV_CO2AutoCommandBytes) || checkIthoCommand(packetPtr, ithoMessageAUTORFTAutoCommandBytes))
    {
      packetPtr->command = IthoAuto;
    }
    else if (checkIthoCommand(packetPtr, ithoMessageRV_CO2AutoNightCommandBytes) || checkIthoCommand(packetPtr, ithoMessageAUTORFTAutoNightCommandBytes))
    {
      packetPtr->command = IthoAutoNight;
    }
  }

  time_t now;
  time(&now);

  for (auto &item : ithoRF.device)
  {
    if (item.destinationID[0] == byte0 && item.destinationID[1] == byte1 && item.destinationID[2] == byte2)
    {
      item.lastCommand = packetPtr->command;
      item.timestamp = now;
      return;
    }
  }
}

void IthoCC1101::handleTimer(IthoPacket *packetPtr)
{
  RemoteTypes remtype = RemoteTypes::UNSETTYPE;
  uint32_t tempID = 0;
  if (packetPtr->deviceId0 != 0)
    tempID = packetPtr->deviceId0;
  else if (packetPtr->deviceId2 != 0)
    tempID = packetPtr->deviceId2;
  else
    return;

  uint8_t byte0 = tempID >> 16 & 0xFF;
  uint8_t byte1 = tempID >> 8 & 0xFF;
  uint8_t byte2 = tempID & 0xFF;

  if (!allowAll)
  {
    if (!checkRFDevice(byte0, byte1, byte2))
      return;
  }
  for (auto &item : ithoRF.device)
  {
    if (item.destinationID[0] == byte0 && item.destinationID[1] == byte1 && item.destinationID[2] == byte2)
    {
      remtype = item.remType;
    }
  }
  if (remtype == RemoteTypes::ORCON15LF01)
  {
    if (checkIthoCommand(packetPtr, orconMessageTimer1CommandBytes))
    {
      packetPtr->command = IthoTimer1;
    }
    else if (checkIthoCommand(packetPtr, orconMessageTimer2CommandBytes))
    {
      packetPtr->command = IthoTimer2;
    }
    else if (checkIthoCommand(packetPtr, orconMessageTimer3CommandBytes))
    {
      packetPtr->command = IthoTimer3;
    }
  }
  else
  {
    if (checkIthoCommand(packetPtr, ithoMessageTimer1CommandBytes) || checkIthoCommand(packetPtr, ithoMessageAUTORFTTimer1CommandBytes) || checkIthoCommand(packetPtr, ithoMessageRV_CO2Timer1CommandBytes) || checkIthoCommand(packetPtr, ithoMessageDFTimer1CommandBytes))
    {
      packetPtr->command = IthoTimer1;
    }
    else if (checkIthoCommand(packetPtr, ithoMessageTimer2CommandBytes) || checkIthoCommand(packetPtr, ithoMessageAUTORFTTimer2CommandBytes) || checkIthoCommand(packetPtr, ithoMessageRV_CO2Timer2CommandBytes) || checkIthoCommand(packetPtr, ithoMessageDFTimer2CommandBytes))
    {
      packetPtr->command = IthoTimer2;
    }
    else if (checkIthoCommand(packetPtr, ithoMessageTimer3CommandBytes) || checkIthoCommand(packetPtr, ithoMessageAUTORFTTimer3CommandBytes) || checkIthoCommand(packetPtr, ithoMessageRV_CO2Timer3CommandBytes) || checkIthoCommand(packetPtr, ithoMessageDFTimer3CommandBytes))
    {
      packetPtr->command = IthoTimer3;
    }
    else if (checkIthoCommand(packetPtr, ithoMessageDFCook30CommandBytes))
    {
      packetPtr->command = IthoCook30;
    }
    else if (checkIthoCommand(packetPtr, ithoMessageDFCook60CommandBytes))
    {
      packetPtr->command = IthoCook60;
    }
  }

  time_t now;
  time(&now);

  for (auto &item : ithoRF.device)
  {
    if (item.destinationID[0] == byte0 && item.destinationID[1] == byte1 && item.destinationID[2] == byte2)
    {
      item.lastCommand = packetPtr->command;
      item.timestamp = now;
      return;
    }
  }
}

void IthoCC1101::handle31D9(IthoPacket *packetPtr)
{
  D_LOG("handle31D9");
  uint32_t tempID = 0;
  if (packetPtr->deviceId0 != 0)
  {
    tempID = packetPtr->deviceId0;
  }
  else if (packetPtr->deviceId2 != 0)
  {
    tempID = packetPtr->deviceId2;
  }
  else
  {
    D_LOG("deviceId not found");
    return;
  }

  uint8_t byte0 = tempID >> 16 & 0xFF;
  uint8_t byte1 = tempID >> 8 & 0xFF;
  uint8_t byte2 = tempID & 0xFF;

  if (!checkRFDevice(byte0, byte1, byte2))
  {
    return;
  }

  packetPtr->command = Itho31D9;

  time_t now;
  time(&now);

  for (auto &item : ithoRF.device)
  {
    if (item.destinationID[0] == byte0 && item.destinationID[1] == byte1 && item.destinationID[2] == byte2)
    {
      item.lastCommand = packetPtr->command;
      item.timestamp = now;
      return;
    }
  }
}

void IthoCC1101::handle31DA(IthoPacket *packetPtr)
{
  D_LOG("handle31DA");
  uint32_t tempID = 0;
  if (packetPtr->deviceId0 != 0)
  {
    tempID = packetPtr->deviceId0;
  }
  else if (packetPtr->deviceId2 != 0)
  {
    tempID = packetPtr->deviceId2;
  }
  else
  {
    D_LOG("deviceId not found");
    return;
  }

  uint8_t byte0 = tempID >> 16 & 0xFF;
  uint8_t byte1 = tempID >> 8 & 0xFF;
  uint8_t byte2 = tempID & 0xFF;

  if (!checkRFDevice(byte0, byte1, byte2))
  {
    return;
  }

  packetPtr->command = Itho31DA;

  time_t now;
  time(&now);

  for (auto &item : ithoRF.device)
  {
    if (item.destinationID[0] == byte0 && item.destinationID[1] == byte1 && item.destinationID[2] == byte2)
    {
      // item.destinationID[0] = (packetPtr->deviceId1 >> 16) & 0xFF;
      // item.destinationID[1] = (packetPtr->deviceId1) >> 8 & 0xFF;
      // item.destinationID[2] = (packetPtr->deviceId1) & 0xFF;
      item.lastCommand = packetPtr->command;
      item.timestamp = now;
      D_LOG("IthoStatus remote ID match");
      return;
    }
  }
}

void IthoCC1101::handleRemotestatus(IthoPacket *packetPtr)
{
  uint32_t tempID = 0;
  if (packetPtr->deviceId0 != 0)
    tempID = packetPtr->deviceId0;
  else if (packetPtr->deviceId2 != 0)
    tempID = packetPtr->deviceId2;
  else
    return;

  uint8_t byte0 = tempID >> 16 & 0xFF;
  uint8_t byte1 = tempID >> 8 & 0xFF;
  uint8_t byte2 = tempID & 0xFF;

  if (!checkRFDevice(byte0, byte1, byte2))
    return;

  for (auto &item : ithoRF.device)
  {
    if (item.destinationID[0] == byte0 && item.destinationID[1] == byte1 && item.destinationID[2] == byte2)
    {
      // store last command
      return;
    }
  }
}
void IthoCC1101::handleTemphum(IthoPacket *packetPtr)
{
  /*
     message length: 6
     message opcode: 0x12A0
     byte[0]    : unknown
     byte[1]    : humidity
     bytes[2:3] : temperature
     bytes[4:5] : dewpoint temperature

  */
  if (packetPtr->error > 0)
    return;
  uint32_t tempID = 0;
  if (packetPtr->deviceId0 != 0)
    tempID = packetPtr->deviceId0;
  else if (packetPtr->deviceId2 != 0)
    tempID = packetPtr->deviceId2;
  else
    return;

  uint8_t byte0 = tempID >> 16 & 0xFF;
  uint8_t byte1 = tempID >> 8 & 0xFF;
  uint8_t byte2 = tempID & 0xFF;

  if (!checkRFDevice(byte0, byte1, byte2))
    return;
  int32_t tempHum = packetPtr->dataDecoded[packetPtr->payloadPos + 1];
  int32_t tempTemp = packetPtr->dataDecoded[packetPtr->payloadPos + 2] << 8 | packetPtr->dataDecoded[packetPtr->payloadPos + 3];
  int32_t tempDewp = packetPtr->dataDecoded[packetPtr->payloadPos + 4] << 8 | packetPtr->dataDecoded[packetPtr->payloadPos + 5];

  for (auto &item : ithoRF.device)
  {
    if (item.destinationID[0] == byte0 && item.destinationID[1] == byte1 && item.destinationID[2] == byte2)
    {
      item.temp = tempTemp;
      item.hum = tempHum;
      item.dewpoint = tempDewp;
      return;
    }
  }
}

void IthoCC1101::handleCo2(IthoPacket *packetPtr)
{
  /*
     message length: 3
     message opcode: 0x1298
     byte[0]    : unknown
     bytes[1:2] : CO2 level

  */
  if (packetPtr->error > 0)
    return;
  uint32_t tempID = 0;
  if (packetPtr->deviceId0 != 0)
    tempID = packetPtr->deviceId0;
  else if (packetPtr->deviceId2 != 0)
    tempID = packetPtr->deviceId2;
  else
    return;

  uint8_t byte0 = tempID >> 16 & 0xFF;
  uint8_t byte1 = tempID >> 8 & 0xFF;
  uint8_t byte2 = tempID & 0xFF;

  if (!checkRFDevice(byte0, byte1, byte2))
    return;
  int32_t tempVal = packetPtr->dataDecoded[packetPtr->payloadPos + 1] << 8 | packetPtr->dataDecoded[packetPtr->payloadPos + 2];

  for (auto &item : ithoRF.device)
  {
    if (item.destinationID[0] == byte0 && item.destinationID[1] == byte1 && item.destinationID[2] == byte2)
    {
      item.co2 = tempVal;
      return;
    }
  }
}
void IthoCC1101::handleBattery(IthoPacket *packetPtr)
{
  /*
     message length: 3
     message opcode: 0x1060
     byte[0]  : zone_id
     byte[1]  : battery level percentage (0xFF = no percentage present)
     byte[2]  : battery state (0x01 OK, 0x00 LOW)

  */
  if (packetPtr->error > 0)
    return;
  uint32_t tempID = 0;
  if (packetPtr->deviceId0 != 0)
    tempID = packetPtr->deviceId0;
  else if (packetPtr->deviceId2 != 0)
    tempID = packetPtr->deviceId2;
  else
    return;

  uint8_t byte0 = tempID >> 16 & 0xFF;
  uint8_t byte1 = tempID >> 8 & 0xFF;
  uint8_t byte2 = tempID & 0xFF;

  if (!checkRFDevice(byte0, byte1, byte2))
    return;
  int32_t tempVal = 10;
  if (packetPtr->dataDecoded[packetPtr->payloadPos + 1] == 0xFF)
  {
    if (packetPtr->dataDecoded[packetPtr->payloadPos + 2] == 0x01)
      tempVal = 100;
  }
  else
  {
    tempVal = (int32_t)packetPtr->dataDecoded[packetPtr->payloadPos + 1] / 2;
  }
  for (auto &item : ithoRF.device)
  {
    if (item.destinationID[0] == byte0 && item.destinationID[1] == byte1 && item.destinationID[2] == byte2)
    {
      item.battery = tempVal;
      return;
    }
  }
}

void IthoCC1101::handlePIR(IthoPacket *packetPtr)
{
  /*
     message length: 3
     message opcode: 0x2E10
     byte[0]    : unknown
     byte[1]    : movement (1) / no movement (0)
     byte[2]    : unknown

  */
  if (packetPtr->error > 0)
    return;
  uint32_t tempID = 0;
  if (packetPtr->deviceId0 != 0)
    tempID = packetPtr->deviceId0;
  else if (packetPtr->deviceId2 != 0)
    tempID = packetPtr->deviceId2;
  else
    return;

  uint8_t byte0 = tempID >> 16 & 0xFF;
  uint8_t byte1 = tempID >> 8 & 0xFF;
  uint8_t byte2 = tempID & 0xFF;

  if (!checkRFDevice(byte0, byte1, byte2))
    return;
  int32_t tempVal = packetPtr->dataDecoded[packetPtr->payloadPos + 1];

  for (auto &item : ithoRF.device)
  {
    if (item.destinationID[0] == byte0 && item.destinationID[1] == byte1 && item.destinationID[2] == byte2)
    {
      item.pir = tempVal;
      return;
    }
  }
}

void IthoCC1101::handleZoneTemp(IthoPacket *packetPtr)
{
  /*
     message length: 3
     message opcode: 0x30C9
     byte[0]    : zone
     byte[1:2]  : temperature
  */
  if (packetPtr->error > 0)
    return;
  uint32_t tempID = 0;
  if (packetPtr->deviceId0 != 0)
    tempID = packetPtr->deviceId0;
  else if (packetPtr->deviceId2 != 0)
    tempID = packetPtr->deviceId2;
  else
    return;

  uint8_t byte0 = tempID >> 16 & 0xFF;
  uint8_t byte1 = tempID >> 8 & 0xFF;
  uint8_t byte2 = tempID & 0xFF;

  if (!checkRFDevice(byte0, byte1, byte2))
    return;
  // int32_t tempZone = packetPtr->dataDecoded[packetPtr->payloadPos + 0];
  int32_t tempTemp = packetPtr->dataDecoded[packetPtr->payloadPos + 1] << 8 | packetPtr->dataDecoded[packetPtr->payloadPos + 2];

  for (auto &item : ithoRF.device)
  {
    if (item.destinationID[0] == byte0 && item.destinationID[1] == byte1 && item.destinationID[2] == byte2)
    {
      item.temp = tempTemp;
      return;
    }
  }
}

void IthoCC1101::handleZoneSetpoint(IthoPacket *packetPtr)
{
  /*
     message length: 6
     message opcode: 0x22C9
     byte[0]    : zone
     byte[1:2]  : setpoint
     byte[1:2]  : setpoint 2 (seems 2 degrees above setpoint but never lower than 20C)
  */
  if (packetPtr->error > 0)
    return;
  uint32_t tempID = 0;
  if (packetPtr->deviceId0 != 0)
    tempID = packetPtr->deviceId0;
  else if (packetPtr->deviceId2 != 0)
    tempID = packetPtr->deviceId2;
  else
    return;

  uint8_t byte0 = tempID >> 16 & 0xFF;
  uint8_t byte1 = tempID >> 8 & 0xFF;
  uint8_t byte2 = tempID & 0xFF;

  if (!checkRFDevice(byte0, byte1, byte2))
    return;
  // int32_t tempZone = packetPtr->dataDecoded[packetPtr->payloadPos + 0];
  int32_t tempSetpoint = packetPtr->dataDecoded[packetPtr->payloadPos + 1] << 8 | packetPtr->dataDecoded[packetPtr->payloadPos + 2];

  for (auto &item : ithoRF.device)
  {
    if (item.destinationID[0] == byte0 && item.destinationID[1] == byte1 && item.destinationID[2] == byte2)
    {
      item.setpoint = tempSetpoint;
      return;
    }
  }
}

void IthoCC1101::handleDeviceInfo(IthoPacket *packetPtr)
{
  D_LOG("handleDeviceInfo");
  uint32_t tempID = 0;
  if (packetPtr->deviceId0 != 0)
  {
    tempID = packetPtr->deviceId0;
  }
  else if (packetPtr->deviceId2 != 0)
  {
    tempID = packetPtr->deviceId2;
  }
  else
  {
    return;
  }

  uint8_t byte0 = tempID >> 16 & 0xFF;
  uint8_t byte1 = tempID >> 8 & 0xFF;
  uint8_t byte2 = tempID & 0xFF;

  if (!checkRFDevice(byte0, byte1, byte2))
  {
    return;
  }

  packetPtr->command = IthoDeviceInfo;

  time_t now;
  time(&now);

  for (auto &item : ithoRF.device)
  {
    if (item.destinationID[0] == byte0 && item.destinationID[1] == byte1 && item.destinationID[2] == byte2)
    {
      item.lastCommand = packetPtr->command;
      item.timestamp = now;
      D_LOG("IthoDeviceInfo remote ID match");
      return;
    }
  }
}

const IthoCC1101::remote_command_char IthoCC1101::remote_command_msg_table[]{
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

const char *IthoCC1101::remote_unknown_msg = "CMD UNKNOWN ERROR";

const char *IthoCC1101::rem_cmd_to_name(IthoCommand code)
{
  size_t i;

  for (i = 0; i < sizeof(IthoCC1101::remote_command_msg_table) / sizeof(IthoCC1101::remote_command_msg_table[0]); ++i)
  {
    if (IthoCC1101::remote_command_msg_table[i].code == code)
    {
      return IthoCC1101::remote_command_msg_table[i].msg;
    }
  }
  return IthoCC1101::remote_unknown_msg;
}
