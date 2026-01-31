#include "IthoDevice.h"

uint8_t cmdCounter = 0;

void sendRemoteCmd(const uint8_t remoteIndex, const IthoCommand command)
{
  // command structure:
  //  [I2C addr ][  I2C command   ][len ][    timestamp         ][fmt ][    remote ID   ][cntr]<  opcode  ><len2><   len2 length command      >[chk2][cntr][chk ]
  //  0x82, 0x60, 0xC1, 0x01, 0x01, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0x16, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
  if (remoteIndex > virtualRemotes.getMaxRemotes())
    return;

  const RemoteTypes remoteType = virtualRemotes.getRemoteType(remoteIndex);
  if (remoteType == RemoteTypes::UNSETTYPE)
    return;

  // Get the corresponding command / remote type combination
  const uint8_t *remote_command = rfManager.radio.getRemoteCmd(remoteType, command);
  if (remote_command == nullptr)
    return;

  uint8_t i2c_command[64] = {0};
  uint8_t i2c_command_len = 0;

  /*
     First build the i2c header / wrapper for the remote command

  */
  //                       [I2C addr ][  I2C command   ][len ][    timestamp         ][fmt ][    remote ID   ][cntr]
  uint8_t i2c_header[] = {0x82, 0x60, 0xC1, 0x01, 0x01, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0x16, 0xFF, 0xFF, 0xFF, 0xFF};

  unsigned long curtime = millis();
  i2c_header[6] = (curtime >> 24) & 0xFF;
  i2c_header[7] = (curtime >> 16) & 0xFF;
  i2c_header[8] = (curtime >> 8) & 0xFF;
  i2c_header[9] = curtime & 0xFF;

  uint8_t id[3]{};
  virtualRemotes.getRemoteIDbyIndex(remoteIndex, &id[0]);
  i2c_header[11] = id[0];
  i2c_header[12] = id[1];
  i2c_header[13] = id[2];

  i2c_header[14] = cmdCounter;
  cmdCounter++;

  for (; i2c_command_len < sizeof(i2c_header) / sizeof(i2c_header[0]); i2c_command_len++)
  {
    i2c_command[i2c_command_len] = i2c_header[i2c_command_len];
  }

  // determine command length
  const int command_len = remote_command[2];

  // copy to i2c_command
  for (int i = 0; i < 2 + command_len + 1; i++)
  {
    i2c_command[i2c_command_len] = remote_command[i];

    if (i > 5 && (command == IthoJoin || command == IthoLeave))
    {
      // command bytes locations with ID in Join/Leave messages: 6/7/8 12/13/14 18/19/20 24/25/26 30/31/32 36/37/38 etc
      if (i % 6 == 0 || i % 6 == 1 || i % 6 == 2)
      {
        i2c_command[i2c_command_len] = *(id + (i % 6));
      }
    }
    i2c_command_len++;
  }

  // // if join or leave, add remote ID fields
  // if (command == IthoJoin || command == IthoLeave)
  // {
  //   // if len = 6/7/8 12/13/14 18/19/20 24/25/26 30/31/32 36/37/38 etc... continue; those are the device ID bytes on Join/Leave messages
  //   if (> 5 && (i % 6 == 0 || i % 6 == 1 || i % 6 == 2))
  //     continue;
  //   // set command ID's
  //   if (command_len > 0x05)
  //   {
  //     // add 1st ID
  //     i2c_command[21] = *id;
  //     i2c_command[22] = *(id + 1);
  //     i2c_command[23] = *(id + 2);
  //   }
  //   if (command_len > 0x0B)
  //   {
  //     // add 2nd ID
  //     i2c_command[27] = *id;
  //     i2c_command[28] = *(id + 1);
  //     i2c_command[29] = *(id + 2);
  //   }
  //   if (command_len > 0x12)
  //   {
  //     // add 3rd ID
  //     i2c_command[33] = *id;
  //     i2c_command[34] = *(id + 1);
  //     i2c_command[35] = *(id + 2);
  //   }
  //   if (command_len > 0x17)
  //   {
  //     // add 4th ID
  //     i2c_command[39] = *id;
  //     i2c_command[40] = *(id + 1);
  //     i2c_command[41] = *(id + 2);
  //   }
  //   if (command_len > 0x1D)
  //   {
  //     // add 5th ID
  //     i2c_command[45] = *id;
  //     i2c_command[46] = *(id + 1);
  //     i2c_command[47] = *(id + 2);
  //   }
  // }

  // if timer1-3 command use timer config of systemConfig.itho_timer1-3
  // example timer command: {0x22, 0xF3, 0x06, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00} 0xFF is timer value on pos 20 of i2c_command[]
  if (command == IthoTimer1)
  {
    i2c_command[20] = systemConfig.itho_timer1;
  }
  else if (command == IthoTimer2)
  {
    i2c_command[20] = systemConfig.itho_timer2;
  }
  else if (command == IthoTimer3)
  {
    i2c_command[20] = systemConfig.itho_timer3;
  }

  /*
     built the footer of the i2c wrapper
     chk2 = checksum of [fmt]+[remote ID]+[cntr]+[remote command]
     chk = checksum of the whole command

  */
  //                        [chk2][cntr][chk ]
  // uint8_t i2c_footer[] = { 0x00, 0x00, 0xFF };

  // calculate chk2 val
  uint8_t i2c_command_tmp[64] = {0};
  for (int i = 10; i < i2c_command_len; i++)
  {
    i2c_command_tmp[(i - 10)] = i2c_command[i];
  }

  i2c_command[i2c_command_len] = checksum(i2c_command_tmp, i2c_command_len - 11);
  i2c_command_len++;

  //[cntr]
  i2c_command[i2c_command_len] = 0x00;
  i2c_command_len++;

  // set i2c_command length value
  i2c_command[5] = i2c_command_len - 6;

  // calculate chk val
  i2c_command[i2c_command_len] = checksum(i2c_command, i2c_command_len - 1);
  i2c_command_len++;

#if defined(ENABLE_SERIAL)
  String str;
  char buf[250] = {0};
  for (int i = 0; i < i2c_command_len; i++)
  {
    snprintf(buf, sizeof(buf), " 0x%02X", i2c_command[i]);
    str += String(buf);
    if (i < i2c_command_len - 1)
    {
      str += ",";
    }
  }
  str += "\n";
  D_LOG(str.c_str());
#endif

  i2cSendBytes(i2c_command, i2c_command_len, I2C_CMD_REMOTE_CMD);
}

void filterReset()
{

  //[I2C addr ][  I2C command   ][len ][    timestamp         ][fmt ][    remote ID   ][cntr][cmd opcode][len ][  command ][  counter ][chk]
  uint8_t command[] = {0x82, 0x62, 0xC1, 0x01, 0x01, 0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0x16, 0xFF, 0xFF, 0xFF, 0xFF, 0x10, 0xD0, 0x02, 0x63, 0xFF, 0x00, 0x00, 0xFF};

  unsigned long curtime = millis();

  command[6] = (curtime >> 24) & 0xFF;
  command[7] = (curtime >> 16) & 0xFF;
  command[8] = (curtime >> 8) & 0xFF;
  command[9] = curtime & 0xFF;

  uint8_t id[3]{};
  virtualRemotes.getRemoteIDbyIndex(0, &id[0]);
  command[11] = id[0];
  command[12] = id[1];
  command[13] = id[2];

  command[14] = cmdCounter;
  cmdCounter++;

  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  i2cSendBytes(command, sizeof(command), I2C_CMD_FILTER_RESET);
}

void sendI2CPWMinit()
{

  // uint8_t command[] = {0x82, 0xEF, 0xC0, 0x00, 0x01, 0x06, 0x00, 0x00, 0x09, 0x00, 0x09, 0x00, 0xB6};
  // 82 EF CO 00 01 06 00 00 13 00 13 00 A2
  uint8_t command[] = {0x82, 0xEF, 0xC0, 0x00, 0x01, 0x06, 0x00, 0x00, 0x13, 0x00, 0x13, 0x00, 0xB6};

  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  i2cSendBytes(command, sizeof(command), I2C_CMD_PWM_INIT);
}

// init 82 EF CO 00 01 06 00 00 13 00 13 00 A2
void sendCO2init()
{
  uint8_t command[] = {0x82, 0xEF, 0xC0, 0x00, 0x01, 0x06, 0x00, 0x00, 0x13, 0x00, 0x13, 0x00, 0xA2};
  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  i2cSendBytes(command, sizeof(command), I2C_CMD_CO2_INIT);
}

void sendCO2query(bool updateweb)
{
  uint8_t command[] = {0x82, 0xEF, 0xC0, 0x00, 0x01, 0x06, 0x00, 0x00, 0x13, 0x00, 0x13, 0x00, 0xA2};
  uint8_t i2cbuf[512]{};

  size_t result = sendI2cQuery(command, sizeof(command), i2cbuf, I2C_CMD_QUERY_DEVICE_TYPE);
  if (!result)
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("sendCO2init", "failed");
    }

    return;
  }
  else
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("sendCO2init", i2cBufToString(i2cbuf, result).c_str());
    }
  }
}

// 60 B1 E0 01 03 00 66 02 A3

void sendCO2speed(uint8_t speed1, uint8_t speed2)
{
  //                    0     1     2     3     4     5     6     7     8     9     10    11    12    13    14    15    16    17    18   19    20    21
  uint8_t command[] = {0x82, 0xC1, 0x00, 0x01, 0x10, 0x14, 0x51, 0x13, 0x2D, 0x31, 0xE0, 0x08, 0x00, 0x00, 0xFF, 0x01, 0x01, 0x00, 0xFF, 0x01, 0xFF, 0xAC};
  //                   0x82, 0xC1, 0x00, 0x01, 0x10, 0x14, 0x51, 0x13, 0x2D, 0x31, 0xE0, 0x08, 0x00, 0x00, 0x12, 0x01, 0x01, 0x00, 0x0A, 0x01, 0x23, 0xAC
  command[14] = speed1;
  command[18] = speed2;

  // calculate chk2 val
  uint8_t i2c_command_tmp[32] = {0};
  // command[11] == 9;

  uint8_t i2c_command_startpos = 5;
  uint8_t i2c_command_endpos = sizeof(command) - 2;

  for (int i = i2c_command_startpos; i < i2c_command_endpos; i++)
  {
    i2c_command_tmp[(i - i2c_command_startpos)] = command[i];
  }
  command[sizeof(command) - 2] = checksum(i2c_command_tmp, i2c_command_endpos - i2c_command_startpos); // FIXME: check set correct byte, last or second last?

  for (int i = 0; i < sizeof(command); i++)
  {
    D_LOG("0x%02X, ", command[i]);
  }

  i2cSendBytes(command, sizeof(command), I2C_CMD_CO2_CMD);
}
void sendCO2value(uint16_t value)
{
  //                    0     1     2     3     4     5     6     7
  uint8_t command[] = {0x60, 0x92, 0x98, 0x01, 0x02, 0xFF, 0xFF, 0xFF};
  //                  {0x60, 0x92, 0x98, 0x01, 0x02, 0x03, 0x25, 0x4B}; 805ppm
  command[5] = value >> 8;
  command[6] = value & 0xFF;

  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  D_LOG("sendCO2value:%d", value);
  for (int i = 0; i < sizeof(command); i++)
  {
    D_LOG("0x%02X, ", command[i]);
  }

  i2cSendBytes(command, sizeof(command), I2C_CMD_CO2_CMD);
}

void IthoPWMcommand(uint16_t value, volatile uint16_t *ithoCurrentVal, bool *updateIthoMQTT)
{
  uint16_t valTemp = *ithoCurrentVal;
  *ithoCurrentVal = value;

  if (systemConfig.itho_forcemedium)
  {
    IthoCommand precmd = IthoCommand::IthoUnknown;
    const RemoteTypes remoteType = virtualRemotes.getRemoteType(0);
    if (rfManager.radio.getRemoteCmd(remoteType, IthoCommand::IthoMedium) != nullptr)
      precmd = IthoCommand::IthoMedium;
    else if (rfManager.radio.getRemoteCmd(remoteType, IthoCommand::IthoAuto) != nullptr)
      precmd = IthoCommand::IthoAuto;
    else
      W_LOG("SYS: force medium not executed, remote has no medium or auto command");

    if (precmd != IthoCommand::IthoUnknown)
      sendRemoteCmd(0, precmd);
  }

  uint8_t command[] = {0x00, 0x60, 0xC0, 0x20, 0x01, 0x02, 0xFF, 0x00, 0xFF};

  uint8_t b = static_cast<uint8_t>(value);

  command[6] = b;
  // command[8] = 0 - (67 + b);
  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  if (i2cSendBytes(command, sizeof(command), I2C_CMD_PWM_CMD))
  {
    *updateIthoMQTT = true;
  }
  else
  {
    *ithoCurrentVal = valTemp;
    ithoQueue.addToQueue(valTemp, 0, systemConfig.nonQ_cmd_clearsQ);
  }
}
