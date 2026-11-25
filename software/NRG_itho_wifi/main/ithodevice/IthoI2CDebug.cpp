#include "IthoDevice.h"
#include "i2c_service.h"

#include "globals.h"
#include "sys_log.h"

// Low level I2C commands of the debug page; all results go to one field on the page

void i2cDebugResult(const char *what, const std::string &text)
{
  const std::string out = std::string(what) + ": " + text;
  D_LOG("I2C: %s", out.c_str());
  jsonSysmessage("i2c_result", out.c_str());
}

// Send one frame and wait for a reply; returns the received length, 0 for none
static size_t serviceSend(uint8_t dest, uint16_t msgClass, uint8_t type, const uint8_t *data, size_t len, uint8_t *rx, std::string &sent)
{
  uint8_t frame[i2csvc::MAX_DATA + 7];
  const size_t n = i2csvc::buildFrame(frame, sizeof(frame), dest, msgClass, type, data, len);
  if (!n)
    return 0;
  sent = i2cBufToString(frame, n);

  I2CBusLock lock; // the frame and its reply are one transaction
  if (!i2cSendBytes(frame, n, I2C_CMD_OTHER))
    return 0;
  return i2cSlaveReceive(rx);
}

static std::string describeReply(const uint8_t *rx, size_t n)
{
  const i2csvc::Reply r = i2csvc::parseReply(rx, n);
  if (!r.payload)
    return "unexpected reply: " + i2cBufToString(rx, n);
  char head[80];
  snprintf(head, sizeof(head), "reply from %02X, class %04X, type %02X, %u data bytes%s: ", r.src, r.msgClass, r.type,
           (unsigned)r.len, r.ok ? "" : " (checksum FAIL)");
  return head + i2cBufToString(r.payload, r.len);
}

void i2cServiceRaw(uint8_t dest, uint16_t msgClass, uint8_t type, const std::vector<uint8_t> &data)
{
  uint8_t rx[I2C_SLAVE_RX_BUF_LEN]{};
  std::string sent;
  const size_t n = serviceSend(dest, msgClass, type, data.data(), data.size(), rx, sent);
  if (sent.empty())
    i2cDebugResult("Send", "invalid frame");
  else
    i2cDebugResult("Send", "sent " + sent + " | " + (n ? describeReply(rx, n) : "no reply"));
}

// Message to the Itho unit, decoded where the format is known
static void serviceCommand(const char *what, uint16_t msgClass, uint8_t type, const uint8_t *data = nullptr, size_t len = 0)
{
  uint8_t rx[I2C_SLAVE_RX_BUF_LEN]{};
  std::string sent;
  const size_t n = serviceSend(0x82, msgClass, type, data, len, rx, sent);
  if (!n)
  {
    i2cDebugResult(what, sent.empty() ? "send failed" : "sent " + sent + " | no reply");
    return;
  }
  std::string decoded;
  const i2csvc::Reply r = i2csvc::parseReply(rx, n);
  if (r.payload && r.ok && (r.msgClass & 0x3FFF) == (msgClass & 0x3FFF) && r.len)
  {
    if (msgClass == 0x10E1)
      decoded = std::to_string(i2csvc::serialNumber(r.payload, r.len));
    else if (msgClass == 0x10E2)
      decoded = "\"" + i2csvc::asciiText(r.payload, r.len) + "\"";
    else if (msgClass == 0x4220)
    {
      std::vector<uint8_t> active, reported;
      if (i2csvc::faultFlags(r.payload, r.len, active, reported))
        decoded = "active: " + i2csvc::faultList(active) + "; reported, no longer active: " + i2csvc::faultList(reported);
    }
  }
  i2cDebugResult(what, (decoded.empty() ? "" : decoded + " | ") + describeReply(rx, n));
}

// Fault history with texts for the connected device, in its own result field on the debug page.
// 3222 (CVE types): 3221 gives the buffer size, 16 entries if it does not answer.
// 4205 (HRU 250/300, DemandFlow): 10 entries.
static void faultHistoryCommand()
{
  const ithoFaultTable *t = i2csvc::findFaultTable(ithoDeviceGroup, ithoDeviceID, itho_fwversion);
  const uint16_t cls = t ? t->msgClass : 0x3222;
  uint8_t rx[I2C_SLAVE_RX_BUF_LEN]{};
  std::string sent;
  uint8_t count = cls == 0x4205 ? 10 : 16;
  if (cls == 0x3222)
  {
    const size_t n = serviceSend(0x82, 0x3221, i2csvc::TYPE_QUERY, nullptr, 0, rx, sent);
    const i2csvc::Reply r = i2csvc::parseReply(rx, n);
    if (r.payload && r.ok && (r.msgClass & 0x3FFF) == 0x3221 && r.len >= 1 && r.payload[0] > 0)
      count = r.payload[0];
  }
  const uint8_t req[]{0x00, count};
  const size_t n = serviceSend(0x82, cls, i2csvc::TYPE_QUERY, req, sizeof(req), rx, sent);
  char head[96];
  snprintf(head, sizeof(head), "%s, fw %u, message %04X", getIthoType(), itho_fwversion, cls);
  std::string text = head;
  const i2csvc::Reply r = i2csvc::parseReply(rx, n);
  std::vector<i2csvc::FaultEntry> entries;
  if (!n)
    text += ": no reply";
  else if (r.payload && r.ok && (r.msgClass & 0x3FFF) == cls && i2csvc::faultHistory(r.payload, r.len, cls, entries))
    text += ", " + std::to_string(entries.size()) + " entries" + (t ? "" : ", no fault texts for this device") + "\n" +
            i2csvc::faultHistoryText(entries, t, cls == 0x4205) + "\n" + describeReply(rx, n);
  else
    text += ": " + describeReply(rx, n);
  D_LOG("I2C: Fault history: %s", text.c_str());
  jsonSysmessage("i2c_faulthistory", text.c_str());
}

void i2cDebugCommand(const std::string &cmd)
{
  if (cmd == "type")
    sendQueryDevicetype(true);
  else if (cmd == "statusformat")
    sendQueryStatusFormat(true);
  else if (cmd == "status")
    sendQueryStatus(true);
  else if (cmd == "31DA")
    sendQuery31DA(true);
  else if (cmd == "31D9")
    sendQuery31D9(true);
  else if (cmd == "counters")
    sendQueryCounters(true);
  else if (cmd == "filterreset")
  {
    filterReset();
    i2cDebugResult("Filter reset", "sent with virtual remote 0");
  }
  else if (cmd == "serial")
    serviceCommand("Serial number", 0x10E1, i2csvc::TYPE_QUERY);
  else if (cmd == "article")
    serviceCommand("Article number", 0x10E2, i2csvc::TYPE_QUERY);
  else if (cmd == "faulthistory")
    faultHistoryCommand();
  else if (cmd == "wpufaults")
    serviceCommand("Heat pump faults", 0x4220, i2csvc::TYPE_QUERY);
  else if (cmd == "identify")
    serviceCommand("Identify", 0x03F0, i2csvc::TYPE_WRITE);
  else if (cmd == "deaeration")
  {
    static const uint8_t start[]{0x01, 0x01};
    serviceCommand("Start deaeration", 0x4E34, i2csvc::TYPE_WRITE, start, sizeof(start));
  }
  else
    i2cDebugResult("Debug command", "unknown: " + cmd);
}
