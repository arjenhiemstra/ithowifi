#include "tasks/task_cc1101.h"
#include "tasks/boot_phases.h"
#include "cc1101/spider_protocol.h"
#include "cc1101/spider_discovery.h"

extern bool updateMQTTRemotesInfo; // task_mqtt.cpp

#define TASK_CC1101_PRIO 5

// globals
Ticker TaskCC1101Timeout;
TaskHandle_t xTaskCC1101Handle = nullptr;
uint32_t TaskCC1101HWmark = 0;
bool send31D9 = false;
bool send31D9debug = false;
bool send31DAdebug = false;
uint8_t status31D9{0};
bool fault31D9 = false;
bool frost31D9 = false;
bool filter31D9 = false;
bool send31DA = false;
bool sendJoinReply = false;
bool send10E0 = false;
bool sendBindConfirm = false;
bool sendBatteryStatus = false;
// Itho Spider thermostat (ADR-0011): all Spider traffic is sent from this task
static bool sendSpiderBind = false;
static uint8_t spiderBindID[3]{};
// A Spider only listens briefly after it transmits (verified 2026-10-15): pending actions are sent
// right after an I frame from that Spider, per remote slot (ADR-0011)
struct SpiderPending
{
  volatile bool push;  // deliver W 01FF until the Spider reports the setpoint, or pushUntil passes
  volatile uint32_t pushUntil;
  uint32_t lastPush;   // last W 01FF after a plain broadcast (rate limit)
  volatile bool pollWanted;
  uint32_t lastPoll; // 0 = never polled since boot
  uint8_t repeatLeft; // extra copies of the W 01FF still to send, 1 s apart
  uint32_t repeatAt;
  uint32_t windowAt; // send at this time (Spider polled its controller, answer not seen), 0 = none
};
static SpiderPending spiderPending[MAX_NUM_OF_REMOTES]{};
constexpr uint32_t SPIDER_PUSH_MS = 15 * 60 * 1000; // relay-bound Spiders only listen at their relay poll (5-10 min idle)
constexpr uint32_t SPIDER_PUSH_GAP_MS = 30 * 1000; // after plain broadcasts; poll windows are always used
constexpr uint32_t SPIDER_POLL_MS = 5 * 60 * 1000;
constexpr uint8_t SPIDER_PUSH_REPEATS = 2;      // copies after the first one, like the spider.yaml blueprint
constexpr uint32_t SPIDER_REPEAT_MS = 1000;
constexpr uint8_t SPIDER_ZONE_TRIES = 3; // W 22C9 to a zone controller, 1 s apart, until it echoes the setpoint

static bool spiderConfirmed(const uint8_t sid[3])
{
  for (const auto &item : rfManager.radio.getRFdevices().device)
  {
    if (memcmp(item.destinationID, sid, 3) == 0)
      return item.setpointCmd != 0xEFFF && item.setpoint == item.setpointCmd;
  }
  return false;
}

// Spider discovery (ADR-0011): driven by the web UI, run on received frames in this task
static spider::Discovery spiderDisc;
static uint32_t spiderDiscStarted = 0;
static uint32_t spiderDiscStepAt = 0;
static volatile uint8_t spiderDiscCmd = 0; // 1 start, 2 cancel
static const char *spiderDiscResult = "idle";
static int spiderDiscSlot = -1;
static bool spiderDiscNotify = false;
constexpr uint32_t SPIDER_DISC_TIMEOUT_MS = 20 * 60 * 1000; // fallback: some Spiders send 22C9 only every ~15 min
constexpr uint32_t SPIDER_DISC_HEATER_MS = 12 * 60 * 1000; // relay-bound Spiders poll every 5-10 min when idle
static uint32_t spiderGwUntil = 0;  // gateway bind window end (accept a Spider's 01FF bind), 0 = off
static uint32_t spiderGwSpider = 0; // Spider bound in the last gateway bind window
constexpr uint32_t SPIDER_GW_MS = 3 * 60 * 1000;
static uint32_t spiderProbeAt = 0; // pending discovery probe (RQ 01FF as the candidate's controller), 0 = none
static spider::Probe spiderProbe{};

void spiderDiscoveryRequest(bool start)
{
  spiderDiscCmd = start ? 1 : 2;
}

void spiderGatewayBindRequest()
{
  spiderDiscCmd = 3;
}

static void spiderIdStr(uint32_t id, char *out, size_t len)
{
  snprintf(out, len, "%02X:%02X:%02X", (unsigned)(id >> 16 & 0xFF), (unsigned)(id >> 8 & 0xFF), (unsigned)(id & 0xFF));
}

void spiderDiscoveryStatus(JsonObject obj)
{
  using S = spider::Discovery::State;
  const char *st = spiderDiscResult;
  switch (spiderDisc.state())
  {
  case S::WaitSetpoint: st = "wait_setpoint"; break;
  case S::WaitHeater: st = "wait_heater"; break;
  default: break;
  }
  if (spiderGwUntil != 0)
    st = "gw_wait";
  obj["state"] = st;
  if (spiderDisc.target() != 0)
    obj["target"] = spiderDisc.target() / 100.0f;
  obj["candidates"] = spiderDisc.candidates();
  char buf[10];
  if (spiderDisc.spider() || spiderGwSpider)
  {
    spiderIdStr(spiderDisc.spider() ? spiderDisc.spider() : spiderGwSpider, buf, sizeof(buf));
    obj["spider"] = buf;
  }
  if (spiderDisc.heater())
  {
    spiderIdStr(spiderDisc.heater(), buf, sizeof(buf));
    obj["heater"] = buf;
  }
  if (spiderDiscSlot >= 0)
    obj["slot"] = spiderDiscSlot;
}

static void spiderDiscoveryPush()
{
  JsonDocument doc;
  JsonObject o = doc["spiderdiscovery"].to<JsonObject>();
  spiderDiscoveryStatus(o);
  notifyClients(doc.as<JsonObject>());
}

// Spider and its heater known: store them on a Monitor slot (fan commands not executed) as thermostat
static void spiderDiscoveryFinish()
{
  const uint32_t s = spiderDisc.spider(), h = spiderDisc.heater();
  const uint8_t sid[3]{(uint8_t)(s >> 16), (uint8_t)(s >> 8), (uint8_t)s};
  const uint8_t hid[3]{(uint8_t)(h >> 16), (uint8_t)(h >> 8), (uint8_t)h};
  int index = remotes.remoteIndex(sid[0], sid[1], sid[2]);
  if (index < 0)
  {
    index = remotes.registerNewRemote(sid[0], sid[1], sid[2], RemoteTypes::RFTSPIDER);
    if (index < 0)
    {
      spiderDiscResult = "noslot";
      return;
    }
    remotes.updateRemoteFunction(index, static_cast<uint8_t>(RemoteFunctions::MONITOR));
    remotes.updateRemoteName(index, "Spider");
  }
  if (spiderDisc.zoneController() && systemConfig.spider_zone_setpoint == 1 && spiderDisc.zoneId() != spider::ZONE_NONE)
  { // experimental: setpoint sent as the Spider to its zone controller (W 22C9), never as the controller to the Spider
    remotes.setThermostat(index, true);
    remotes.setThermostatCtl(index, hid);
    remotes.setThermostatZone(index, spiderDisc.zoneId());
    if (!rfManager.radio.checkRFDevice(sid[0], sid[1], sid[2]))
      rfManager.radio.updateRFDevice(index, sid[0], sid[1], sid[2], RemoteTypes::RFTSPIDER, false);
    rfManager.radio.setThermostat(sid, true, remotes.getThermostatSetpoint(index), hid, spiderDisc.zoneId());
    remotes.addCapabilities(index, "thermostat", 1);
    saveRemotesflag = true;
    spiderDiscSlot = index;
    spiderDiscResult = "done_zone_set";
    N_LOG("RFI: Spider %02X:%02X:%02X discovered on zone controller %02X:%02X:%02X zone %u, slot %d", sid[0], sid[1], sid[2], hid[0], hid[1], hid[2], spiderDisc.zoneId(), index);
    return;
  }
  if (spiderDisc.zoneController())
  { // zone controller (AutoTemp): we cannot pose as it, so read only and nothing is sent
    remotes.setThermostat(index, false); // also clears a role from an earlier run
    if (!rfManager.radio.checkRFDevice(sid[0], sid[1], sid[2]))
      rfManager.radio.updateRFDevice(index, sid[0], sid[1], sid[2], RemoteTypes::RFTSPIDER, false);
    rfManager.radio.setThermostat(sid, false, 0xEFFF);
    saveRemotesflag = true;
    spiderDiscSlot = index;
    spiderDiscResult = "done_zone";
    N_LOG("RFI: Spider %02X:%02X:%02X discovered on zone controller %02X:%02X:%02X, slot %d (read only)", sid[0], sid[1], sid[2], hid[0], hid[1], hid[2], index);
    return;
  }
  remotes.setThermostat(index, true);
  remotes.setThermostatCtl(index, hid);
  remotes.setThermostatZone(index, spider::ZONE_NONE);
  if (!rfManager.radio.checkRFDevice(sid[0], sid[1], sid[2]))
  {
    rfManager.radio.updateRFDevice(index, sid[0], sid[1], sid[2], RemoteTypes::RFTSPIDER, false); // radio slot = remote slot
  }
  rfManager.radio.setThermostat(sid, true, remotes.getThermostatSetpoint(index), hid);
  remotes.addCapabilities(index, "thermostat", 1);
  saveRemotesflag = true;
  spiderDiscSlot = index;
  spiderDiscResult = "done";
  N_LOG("RFI: Spider %02X:%02X:%02X discovered, controller %02X:%02X:%02X, slot %d", sid[0], sid[1], sid[2], hid[0], hid[1], hid[2], index);
}

static uint32_t idTo32(const uint8_t id[3]) { return (uint32_t)id[0] << 16 | (uint32_t)id[1] << 8 | id[2]; }

// Called from other tasks (MQTT/web) after a setpoint change: push W 01FF 3x, 1 s apart
void requestSpiderPush(const uint8_t id[3])
{
  const int idx = remotes.remoteIndex(id[0], id[1], id[2]);
  if (idx < 0 || idx >= MAX_NUM_OF_REMOTES)
    return;
  spiderPending[idx].pollWanted = false;
  spiderPending[idx].pushUntil = millis() + SPIDER_PUSH_MS;
  spiderPending[idx].lastPush = 0;
  spiderPending[idx].push = true;
  const uint8_t *ctl = remotes.getThermostatCtl(idx);
  if (remotes.getThermostatZone(idx) != spider::ZONE_NONE)
  { // zone controller (AutoTemp) is always listening: W 22C9 now, repeated until it echoes the setpoint
    spiderPending[idx].repeatLeft = SPIDER_ZONE_TRIES;
    spiderPending[idx].repeatAt = millis();
  }
  else if (!ctl || idTo32(ctl) == 0)
  { // bound to us (gateway): W 01FF now, a wired Spider takes it; a battery Spider gets it in our next RP
    spiderPending[idx].repeatLeft = 1 + SPIDER_PUSH_REPEATS;
    spiderPending[idx].repeatAt = millis();
  }
}

// Called for every clean received frame: confirm a pending setpoint, or use the Spider's
// listen window right after its own I broadcast to send W 01FF or RQ 01FF.

static void spiderSendProbe(const spider::Probe &pr)
{
  spiderProbeAt = 0;
  const uint8_t sid[3]{(uint8_t)(pr.spider >> 16), (uint8_t)(pr.spider >> 8), (uint8_t)pr.spider};
  const uint8_t cid[3]{(uint8_t)(pr.ctl >> 16), (uint8_t)(pr.ctl >> 8), (uint8_t)pr.ctl};
  rfManager.radio.setSendTries(1);
  disableRF_ISR();
  rfManager.radio.sendSpiderProbe(sid, cid);
  enableRF_ISR();
  rfManager.radio.setSendTries(3);
  spiderDisc.probeSent(pr.spider);
  I_LOG("RFI: Spider discovery: asking %02X:%02X:%02X for its setpoint", sid[0], sid[1], sid[2]);
}

// Send the pending action for a Spider now (it may be listening): W 01FF until confirmed, else a status poll.
// pollWindow: right after the Spider polled its controller (always listens); else after a plain broadcast,
// which only some Spiders listen after, so pushes there are rate limited.
static void spiderSendPending(int idx, const uint8_t sid[3], bool pollWindow)
{
  SpiderPending &p = spiderPending[idx];
  p.windowAt = 0;
  if (p.push && (int32_t)(millis() - p.pushUntil) >= 0)
  {
    p.push = false;
    W_LOG("RFI: Spider %02X:%02X:%02X did not confirm the setpoint", sid[0], sid[1], sid[2]);
  }
  if (p.push && !pollWindow && p.lastPush != 0 && millis() - p.lastPush < SPIDER_PUSH_GAP_MS)
    return;
  const bool push = p.push;
  const bool poll = !push && (p.pollWanted || p.lastPoll == 0 || millis() - p.lastPoll >= SPIDER_POLL_MS);
  if (!push && !poll)
    return;
  rfManager.radio.setSendTries(1);
  disableRF_ISR();
  if (push)
    rfManager.radio.sendSpiderStatus(sid, false);
  else
    rfManager.radio.sendSpiderRequest(sid);
  enableRF_ISR();
  rfManager.radio.setSendTries(3);
  if (push)
  {
    p.repeatLeft = SPIDER_PUSH_REPEATS;
    p.repeatAt = millis() + SPIDER_REPEAT_MS;
    p.lastPush = millis();
  }
  else
  {
    p.pollWanted = false;
    p.lastPoll = millis();
  }
}

// Called for every clean received frame. A Spider only listens briefly around its own transmissions
// (verified on air, two Spider types): right after its controller answered its poll (RP 3EF0 / 3EF1),
// and, for some Spiders, after its plain I broadcasts.
static void spiderOnFrame(const IthoPacket *packet)
{
  const uint8_t type = (packet->header >> 4) & 0x3;
  uint32_t spider = packet->deviceId0 != 0 ? packet->deviceId0 : packet->deviceId2;
  bool ctlAnswer = false;
  if (type == 3 && packet->deviceId1 != 0)
  { // RP addressed to a Spider by its controller?
    const uint8_t d[3]{(uint8_t)(packet->deviceId1 >> 16), (uint8_t)(packet->deviceId1 >> 8), (uint8_t)packet->deviceId1};
    const int i = remotes.remoteIndex(d[0], d[1], d[2]);
    const uint8_t *c = (i >= 0 && remotes.isThermostat(i)) ? remotes.getThermostatCtl(i) : nullptr;
    if (c && idTo32(c) == packet->deviceId0)
    {
      spider = packet->deviceId1;
      ctlAnswer = true;
    }
  }
  const uint8_t sid[3]{(uint8_t)(spider >> 16), (uint8_t)(spider >> 8), (uint8_t)spider};
  const int idx = remotes.remoteIndex(sid[0], sid[1], sid[2]);
  if (idx < 0 || idx >= MAX_NUM_OF_REMOTES || !remotes.isThermostat(idx))
    return;
  const uint8_t *ctl = remotes.getThermostatCtl(idx);
  SpiderPending &p = spiderPending[idx];
  if (!ctl || idTo32(ctl) == 0)
  { // bound to us (gateway): the Spider polls and gets our RP instead
    if (!p.push)
      return;
    if (spiderConfirmed(sid))
    {
      p.push = false;
      p.repeatLeft = 0;
    }
    else if (type == 2 && packet->opcode == IthoPacket::Type::ZONESETPOINT && packet->deviceId0 == spider)
    { // W 22C9: setpoint changed on the Spider itself, newer than ours
      p.push = false;
      p.repeatLeft = 0;
      I_LOG("RFI: Spider %02X:%02X:%02X setpoint changed on the Spider, pending setpoint dropped", sid[0], sid[1], sid[2]);
    }
    else if ((int32_t)(millis() - p.pushUntil) >= 0)
    {
      p.push = false;
      W_LOG("RFI: Spider %02X:%02X:%02X did not confirm the setpoint", sid[0], sid[1], sid[2]);
    }
    return;
  }
  if (remotes.getThermostatZone(idx) != spider::ZONE_NONE)
    return; // zone controller: we send to the controller (repeat loop), never to the Spider
  if ((p.push || p.repeatLeft > 0) && spiderConfirmed(sid))
  {
    p.push = false; // Spider reports the commanded setpoint
    p.repeatLeft = 0;
  }
  if (ctlAnswer)
  {
    spiderSendPending(idx, sid, true);
    return;
  }
  if (packet->deviceId0 != spider)
    return;
  if (type == 0 && packet->deviceId1 == idTo32(ctl))
  { // Spider polls its controller; send after the answer, or after 300 ms if we miss the answer
    p.windowAt = millis() + 300;
    if (p.windowAt == 0)
      p.windowAt = 1;
    return;
  }
  // Plain broadcasts nobody answers. Not after I 0008: the heater echoes it with 3150 (collision on air).
  if (type == 1 && packet->opcode != IthoPacket::Type::SPIDERSTATUS && packet->opcode != IthoPacket::Type::RELAYDEMAND)
    spiderSendPending(idx, sid, false);
}
uint8_t bindConfirmRemIndex = 255;
uint8_t faninfo31DA{0};
uint8_t timer31DA{0};

// locals
StaticTask_t xTaskCC1101Buffer;
StackType_t xTaskCC1101Stack[STACK_SIZE];

Ticker timerLearnLeaveMode;
Ticker rf_message;
Ticker bindInitiatorTimeout;

volatile bool ithoCheck = false;

// Serializes all radio (SPI) access — the task's receivePacket() vs any send —
// so a send from a web/MQTT task can't collide with a receive on TaskCC1101,
// nor two sends with each other. A binary semaphore (not a mutex) is used so the
// unbalanced enableRF_ISR() at init is a harmless give on an already-available
// semaphore. The CC1101 lib itself stays free of any RTOS dependency; this
// guard lives entirely in the app around the lib's send/receive entry points.
SemaphoreHandle_t radioSem = NULL;

IRAM_ATTR void ITHOinterrupt()
{
  // Just wake TaskCC1101; the FIFO read (receivePacket) now runs in task
  // context. It used to run here — up to ~34 ms of SPI inside a level-1 ISR.
  BaseType_t higherPriorityTaskWoken = pdFALSE;
  if (xTaskCC1101Handle)
    vTaskNotifyGiveFromISR(xTaskCC1101Handle, &higherPriorityTaskWoken);
  portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

// disable/enable also acquire/release exclusive radio access: every send is
// bracketed by disableRF_ISR()/enableRF_ISR(), so the semaphore is held for the
// whole send and blocks a concurrent receive (and vice versa).
void disableRF_ISR()
{
  detachInterrupt(hardwareManager.itho_irq_pin);
  if (radioSem)
    xSemaphoreTake(radioSem, portMAX_DELAY);
}

void enableRF_ISR()
{
  attachInterrupt(hardwareManager.itho_irq_pin, ITHOinterrupt, RISING);
  // If GDO is already HIGH, a packet arrived while the ISR was disabled and the
  // rising edge was missed. Notify the task to read it (in task context).
  if (digitalRead(hardwareManager.itho_irq_pin) == HIGH && xTaskCC1101Handle)
  {
    xTaskNotifyGive(xTaskCC1101Handle);
  }
  if (radioSem)
    xSemaphoreGive(radioSem);
}

void RFDebug(IthoPacket *packet)
{
  char debugLog[400] = {0};
  strncat(debugLog, rfManager.radio.LastMessageDecoded(packet).c_str(), sizeof(debugLog) - strlen(debugLog) - 1);
  // integrity: a checksum-failed frame is decoded/logged but not dispatched
  if (packet->error == 2)
    strncat(debugLog, " (crc:FAIL)", sizeof(debugLog) - strlen(debugLog) - 1);
  // log command
  switch (packet->command)
  {
  case IthoUnknown:
    strncat(debugLog, " (cmd:unknown)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoAway:
    strncat(debugLog, " (cmd:away)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoLow:
    strncat(debugLog, " (cmd:low)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoMedium:
    strncat(debugLog, " (cmd:medium)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoHigh:
    strncat(debugLog, " (cmd:high)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoFull:
    strncat(debugLog, " (cmd:full)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoAuto:
    strncat(debugLog, " (cmd:auto)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoAutoNight:
    strncat(debugLog, " (cmd:autonight)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoTimer1:
    strncat(debugLog, " (cmd:timer1)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoTimer2:
    strncat(debugLog, " (cmd:timer2)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoTimer3:
    strncat(debugLog, " (cmd:timer3)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoCook30:
    strncat(debugLog, " (cmd:cook30)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoCook60:
    strncat(debugLog, " (cmd:cook60)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoTimerUser:
    strncat(debugLog, " (cmd:timeruser)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoJoin:
    strncat(debugLog, " (cmd:join)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoLeave:
    strncat(debugLog, " (cmd:leave)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoJoinReply:
    strncat(debugLog, " (cmd:joinreply)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoPIRmotionOn:
    strncat(debugLog, " (cmd:motion_on)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoPIRmotionOff:
    strncat(debugLog, " (cmd:motion_off)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case Itho31D9:
    strncat(debugLog, " (cmd:itho31d9)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case Itho31DA:
    strncat(debugLog, " (cmd:itho31da)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoDeviceInfo:
    strncat(debugLog, " (cmd:ithodeviceinfo)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoBindAccept:
    strncat(debugLog, " (cmd:bindaccept)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoBindConfirm:
    strncat(debugLog, " (cmd:bindconfirm)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  }
  // strncat(debugLog, "\n", sizeof(debugLog) - strlen(debugLog) - 1);

  D_LOG(debugLog);

  logMessagejson(debugLog, RFLOG);
}

void toggleRemoteLLmode(const char *remotetype)
{

  if (strcmp(remotetype, "remote") == 0)
  {
    if (virtualRemotes.remoteLearnLeaveStatus())
    {
      logMessagejson("disable virtual remote Copy ID mode first", WEBINTERFACE);
      return;
    }
    if (remotes.toggleLearnLeaveMode())
    {
      // rfManager.radio.setBindAllowed(true);
      timerLearnLeaveMode.attach(1, setllModeTimer);
    }
    else
    {
      // rfManager.radio.setBindAllowed(false);
      timerLearnLeaveMode.detach();
      remotes.setllModeTime(0);
      sysStatReq = true;
    }
  }
  if (strcmp(remotetype, "vremote") == 0)
  {
    if (remotes.remoteLearnLeaveStatus())
    {
      logMessagejson("disable remote learn/leave mode first", WEBINTERFACE);
      return;
    }
    if (virtualRemotes.toggleLearnLeaveMode())
    {
      timerLearnLeaveMode.attach(1, setllModeTimer);
    }
    else
    {
      timerLearnLeaveMode.detach();
      virtualRemotes.setllModeTime(0);
      virtualRemotes.copy_id_remote_idx = -1;
      sysStatReq = true;
    }
  }
}

void setllModeTimer()
{

  if (remotes.remoteLearnLeaveStatus())
  {
    remotes.updatellModeTimer();
    sysStatReq = true;
    if (remotes.getllModeTime() == 0)
    {
      toggleRemoteLLmode("remote");
    }
    return;
  }
  if (virtualRemotes.remoteLearnLeaveStatus())
  {
    virtualRemotes.updatellModeTimer();
    sysStatReq = true;
    if (virtualRemotes.getllModeTime() == 0)
    {
      toggleRemoteLLmode("vremote");
    }
    return;
  }
}

volatile int8_t bindInitiatorResult = 0; // 0=idle, 1=success, -1=timeout

void bindInitiatorTimeoutCallback()
{
  if (rfManager.radio.isBindInitiatorActive())
  {
    rfManager.radio.setBindInitiatorActive(false);
    bindInitiatorResult = -1;
    W_LOG("RFI: Bind initiator timeout - no binding accept received");
  }
}

void startBindInitiatorTimeout()
{
  bindInitiatorTimeout.once_ms(5000, bindInitiatorTimeoutCallback);
}

void cancelBindInitiatorTimeout()
{
  bindInitiatorTimeout.detach();
}

void set_send10E0_true()
{
  send10E0 = true;
}
// Fan status (31DA) for RF: the I2C status when available, else the default template. The last one is kept
// so replies to bound remotes go out at once instead of waiting for a new I2C query.
static uint8_t last31DA[32]{};
static uint32_t last31DAAt = 0;
constexpr uint32_t LAST_31DA_MAX_AGE_MS = 60 * 1000;

static void build31DA(uint8_t *command, bool query)
{
  static const uint8_t tmpl[32] = {0x31, 0xDA, 0x1D, 0x00, 0xC8, 0x40, 0xEF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0xEF, 0xF8, 0x08, 0xEF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF, 0xEF, 0x7F, 0xFF, 0x7F, 0xFF};
  memcpy(command, tmpl, sizeof(tmpl));
  if (!query)
    return;
  uint8_t receive_buffer[256]{};
  size_t result = sendQuery31DA(&receive_buffer[0]);
  // 80 82 B1 DA 01 1C C8 40 03 A5 39 EF 7F FF 7F FF 7F FF 7F FF F8 08 EF 98 05 00 00 00 EF EF 7F FF 7F FF 20
  // 1D:            00,C8,40,04,7B,38,EF,7F,FF,7F,FF,7F,FF,7F,FF,F8,08,EF,98,05,00,00,00,EF,EF,7F,FF,7F,FF
  if (result > 4)
  {
    for (uint8_t i = 4; i < sizeof(tmpl); i++)
      command[i] = receive_buffer[i + 2];
    memcpy(last31DA, command, sizeof(tmpl));
    last31DAAt = millis();
  }
  else
  {
    E_LOG("I2C: error - sendQuery31DA failed");
  }
}

// Answer a bound remote like the fan does: RP 31DA to its RQ 31DA, I 31DA to it after a command.
// Without this an RFT CO2 shows a join error and an RFT AUTO-N repeats its commands and shows red.
static void reply31DA(const uint8_t to[3], bool reply)
{
  uint8_t command[32];
  if (last31DAAt != 0 && millis() - last31DAAt < LAST_31DA_MAX_AGE_MS)
    memcpy(command, last31DA, sizeof(command));
  else
    build31DA(command, true);
  rfManager.radio.setSendTries(1);
  disableRF_ISR();
  rfManager.radio.send31DATo(command, to, reply);
  enableRF_ISR();
  rfManager.radio.setSendTries(3);
}

void set_send31D9_true()
{
  send31D9 = true;
}
void set_send31DA_true()
{
  send31DA = true;
}
void startTaskCC1101()
{
  xTaskCC1101Handle = xTaskCreateStaticPinnedToCore(
      TaskCC1101,
      "TaskCC1101",
      STACK_SIZE,
      (void *)1,
      TASK_CC1101_PRIO,
      xTaskCC1101Stack,
      &xTaskCC1101Buffer,
      CONFIG_ARDUINO_RUNNING_CORE);
}

void TaskCC1101(void *pvParameters)
{

  configASSERT((uint32_t)pvParameters == 1UL);

  // Wait BEFORE any logging: sys_log's syslog_queueSemaphore is created in
  // TaskConfigAndLog, so a D_LOG before PHASE_CONFIG dereferences a null handle.
  waitPhase(PHASE_CONFIG); // needs systemConfig/logConfig; loads its own remotes config

  // reading the chip version is a non-blocking way to check CC1101 connectivity.
  // if the version is greater than 0 there has been succesful communication and we can continue with the init of the chip.
  uint8_t chipVersion = rfManager.radio.getChipVersion();
  if (chipVersion && rfManager.radio.getDetectAttempt() == 1)
    W_LOG("SYS: CC1101 answered only after a reset");
  else if (chipVersion && rfManager.radio.getDetectAttempt() == 2)
    W_LOG("SYS: CC1101 answered only at 1 MHz SPI, staying at 1 MHz");
  if (!chipVersion)
  {
    N_LOG("SYS: no CC1101 RF module found");
    systemConfig.itho_rf_support = 0;
    systemConfig.rfInitOK = false;    
  }
  else if (systemConfig.itho_rf_support)
  {
    N_LOG("SYS: CC1101 RF module found, chip version: 0x%02X", chipVersion);

    radioSem = xSemaphoreCreateBinary();
    xSemaphoreGive(radioSem); // created in the available state

    // init the RF module
    rfManager.radio.init();
    IthoCC1101::setRF31DACallback(parseRF31DA);
    IthoCC1101::setRF31D9Callback(parseRF31D9);
    if (systemConfig.module_rf_id[0] == 0 && systemConfig.module_rf_id[1] == 0 && systemConfig.module_rf_id[2] == 0)
    {
      systemConfig.module_rf_id[0] = sys.getMac(3);
      systemConfig.module_rf_id[1] = sys.getMac(4);
      systemConfig.module_rf_id[2] = sys.getMac(5) - 1;
      I_LOG("SYS: module_rf_id default 0x%02X,0x%02X,0x%02X", systemConfig.module_rf_id[0], systemConfig.module_rf_id[1], systemConfig.module_rf_id[2]);
      saveSystemConfigflag = true;
    }
    else
    {
      I_LOG("SYS: module_rf_id 0x%02X,0x%02X,0x%02X", systemConfig.module_rf_id[0], systemConfig.module_rf_id[1], systemConfig.module_rf_id[2]);
    }

    remotes.setMaxRemotes(systemConfig.itho_numrfrem);

    rfManager.radio.setDefaultID(systemConfig.module_rf_id[0], systemConfig.module_rf_id[1], systemConfig.module_rf_id[2]);
    rfManager.radio.setBindAllowed(false);
    rfManager.radio.setAllowAll(false);

    setRFdebugLevel(logConfig.rfloglevel);

    pinMode(hardwareManager.itho_irq_pin, INPUT);
    enableRF_ISR();

    systemConfig.itho_rf_support = 1;
    RemotesConfigLoaded = loadRemotesConfig("flash");
    for (int index = 0; index < remotes.getMaxRemotes(); index++)
    {
      if (remotes.isEmptySlot(index))
        continue;
      uint8_t id[3]{};
      remotes.getRemoteIDbyIndex(index, &id[0]);
      // Same slot as the remote: index-based radio calls (bidirectional, source/dest ID, type) rely on it.
      // addRFDevice() takes the first free slot and shifts everything after a gap in the remotes list.
      rfManager.radio.updateRFDevice(index, id[0], id[1], id[2], remotes.getRemoteType(index), remotes.getRemoteBidirectional(index));
      // For Send remotes, set the sourceID to the configured ID (what we transmit as)
      if (remotes.getRemoteFunction(index) == RemoteFunctions::SEND)
      {
        rfManager.radio.updateSourceID(id[0], id[1], id[2], index);
      }
      // Restore persisted target device ID for bidirectional remotes
      uint8_t destId[3]{};
      remotes.getRemoteDestIDbyIndex(index, &destId[0]);
      if (destId[0] != 0 || destId[1] != 0 || destId[2] != 0)
      {
        rfManager.radio.updateDestinationID(destId[0], destId[1], destId[2], index);
      }
      if (remotes.isThermostat(index))
      {
        rfManager.radio.setThermostat(id, true, remotes.getThermostatSetpoint(index), remotes.getThermostatCtl(index), remotes.getThermostatZone(index));
        remotes.addCapabilities(index, "thermostat", 1); // listed right away, not only after the first RF frame
      }
    }
    systemConfig.rfInitOK = true;
    // RF CO2 join on boot: find first Send+RFTCO2 remote and send join
    if (systemConfig.itho_rf_co2_join == 1)
    {
      for (int idx = 0; idx < remotes.getMaxRemotes(); idx++)
      {
        if (remotes.isEmptySlot(idx))
          continue;
        if (remotes.getRemoteFunction(idx) == RemoteFunctions::SEND &&
            remotes.getRemoteType(idx) == RemoteTypes::RFTCO2)
        {
          I_LOG("RFI: RF CO2 join on boot, remote index:%d", idx);
          ithoExecRFCommand(idx, "join", HTMLAPI);
          break;
        }
      }
      systemConfig.itho_rf_co2_join = 0;
      saveSystemConfigflag = true;
    }

    uint8_t joinReplyRemIndex{255};
    // uint8_t remIndex10E0{255};

    setPhase(PHASE_RF); // RF module initialised (reached only when a CC1101 is present)
    
    D_LOG("SYS: TaskCC1101 started");
    
    for (;;)
    {
      yield();
      esp_task_wdt_reset();

      TaskCC1101Timeout.once_ms(3000, []()
                                { W_LOG("SYS: warning - CC1101 Task timed out!"); });

      if (spiderDiscCmd != 0)
      {
        spiderGwUntil = 0;
        spiderGwSpider = 0;
        if (spiderDiscCmd == 1)
        {
          spiderDisc.start(spider::Discovery::pickTarget(esp_random()));
          spiderProbeAt = 0;
          spiderDiscStarted = spiderDiscStepAt = millis();
          spiderDiscSlot = -1;
          spiderDiscResult = "idle";
        }
        else if (spiderDiscCmd == 3)
        { // gateway bind: accept the next Spider 01FF bind offer (pairing mode on the Spider)
          spiderDisc.cancel();
          spiderDiscSlot = -1;
          spiderGwUntil = millis() + SPIDER_GW_MS;
          if (spiderGwUntil == 0)
            spiderGwUntil = 1;
        }
        else
        {
          spiderDisc.cancel();
          spiderDiscResult = "cancelled";
        }
        spiderDiscCmd = 0;
        spiderDiscNotify = true;
      }
      {
        using S = spider::Discovery::State;
        const S st = spiderDisc.state();
        if ((st == S::WaitSetpoint || st == S::WaitHeater) &&
            (millis() - spiderDiscStarted > SPIDER_DISC_TIMEOUT_MS || (st == S::WaitHeater && millis() - spiderDiscStepAt > SPIDER_DISC_HEATER_MS)))
        {
          spiderDisc.cancel();
          spiderDiscResult = "timeout";
          spiderDiscNotify = true;
        }
        if (spiderGwUntil != 0 && (int32_t)(millis() - spiderGwUntil) >= 0)
        {
          spiderGwUntil = 0;
          spiderDiscResult = "gw_timeout";
          spiderDiscNotify = true;
        }
      }
      if (spiderDiscNotify)
      {
        spiderDiscNotify = false;
        spiderDiscoveryPush();
      }
      if (spiderProbeAt != 0 && !ithoCheck && (int32_t)(millis() - spiderProbeAt) >= 0)
      { // candidate polled its controller and we missed the answer: use the window anyway
        if (spiderDisc.state() == spider::Discovery::State::WaitSetpoint)
          spiderSendProbe(spiderProbe);
        spiderProbeAt = 0;
      }
      uint8_t spiderID[3];
      if (!ithoCheck && rfManager.radio.consumeSpiderReply(spiderID))
      { // answer the bound Spider's RQ 01FF straight away, single try; our setpoint only while a push is pending,
        // else the Spider's own, so a change on the Spider itself is not undone
        const int ridx = remotes.remoteIndex(spiderID[0], spiderID[1], spiderID[2]);
        const bool useCmd = ridx >= 0 && ridx < MAX_NUM_OF_REMOTES && spiderPending[ridx].push &&
                            (int32_t)(millis() - spiderPending[ridx].pushUntil) < 0;
        rfManager.radio.setSendTries(1);
        disableRF_ISR();
        rfManager.radio.sendSpiderStatus(spiderID, true, useCmd);
        enableRF_ISR();
        rfManager.radio.setSendTries(3);
      }
      for (int i = 0; i < MAX_NUM_OF_REMOTES && !ithoCheck; i++)
      { // Spider polled its controller and we missed the answer: use the window anyway
        SpiderPending &p = spiderPending[i];
        if (p.windowAt != 0 && (int32_t)(millis() - p.windowAt) >= 0)
        {
          uint8_t sid[3]{};
          remotes.getRemoteIDbyIndex(i, &sid[0]);
          spiderSendPending(i, sid, true);
        }
      }
      for (int i = 0; i < MAX_NUM_OF_REMOTES && !ithoCheck; i++)
      { // scheduled repeats of a setpoint push (1 s apart), stopped once the Spider confirms
        SpiderPending &p = spiderPending[i];
        if (p.repeatLeft == 0 || (int32_t)(millis() - p.repeatAt) < 0)
          continue;
        uint8_t sid[3]{};
        remotes.getRemoteIDbyIndex(i, &sid[0]);
        if (spiderConfirmed(sid))
        {
          p.repeatLeft = 0;
          p.push = false;
          continue;
        }
        const bool zone = remotes.getThermostatZone(i) != spider::ZONE_NONE;
        rfManager.radio.setSendTries(1);
        disableRF_ISR();
        if (zone)
          rfManager.radio.sendSpiderZoneSetpoint(sid);
        else
          rfManager.radio.sendSpiderStatus(sid, false);
        enableRF_ISR();
        rfManager.radio.setSendTries(3);
        p.repeatLeft = p.repeatLeft - 1;
        p.repeatAt = millis() + SPIDER_REPEAT_MS;
        if (zone)
        {
          I_LOG("RFI: Spider %02X:%02X:%02X setpoint sent to its zone controller", sid[0], sid[1], sid[2]);
          if (p.repeatLeft == 0)
            p.push = false;
        }
      }
      if (sendSpiderBind && !ithoCheck)
      {
        sendSpiderBind = false;
        rfManager.radio.setSendTries(1);
        disableRF_ISR();
        rfManager.radio.sendSpiderBindAccept(spiderBindID);
        enableRF_ISR();
        rfManager.radio.setSendTries(3);
        I_LOG("RFI: Spider %02X:%02X:%02X heating bind accept sent", spiderBindID[0], spiderBindID[1], spiderBindID[2]);
      }
      if (send10E0 && !ithoCheck)
      {
        send10E0 = false;
        rfManager.radio.setSendTries(1);
        disableRF_ISR();
        rfManager.radio.send10E0();
        enableRF_ISR();
        rfManager.radio.setSendTries(3);
        rf_message.once_ms(100, set_send31D9_true);
      }
      if (sendJoinReply && !ithoCheck)
      {
        /*

        H:18 _I P0:-- P1:-- 94,11,A9 --,--,-- 94,11,A9 1FC9 12:00,22,F8,94,11,A9,01,10,E0,94,11,A9,00,1F,C9,94,11,A9 (cmd:join)
        Join command received. Trying to join remote...
        ?? 13-10-2024, 15:55:59: H:1C _I P0:-- P1:-- 94,11,A9 96,C8,B6 --,--,-- 1FC9 01:00 (cmd:unknown)
        Send join reply
        H:2C _W P0:-- P1:-- 96,C8,B6 94,11,A9 --,--,-- 1FC9 0C:00,31,D9,96,C8,B6,00,31,DA,96,C8,B6 (cmd:unknown)
        receive battery
        H:18 _I P0:-- P1:-- 94,11,A9 --,--,-- 94,11,A9 1060 03:00,FF,01 (cmd:unknown)
        H:18 _I P0:-- P1:-- 94,11,A9 --,--,-- 94,11,A9 1060 03:00,FF,01 (cmd:unknown)
        send 10E0
        H:18 _I P0:-- P1:-- 96,C8,B6 --,--,-- 96,C8,B6 10E0 26:00,00,01,00,1B,31,19,01,FE,FF,FF,FF,FF,FF,0E,05,07,E2,43,56,45,2D,52,46,00,00,00,00,00,00,00,00,00,00,00,00,00,00 (cmd:unknown)
        send 31D9
        H:1A _I P0:02  P1:-- 96,C8,B6 --,--,-- 96,C8,B6 31D9 11:00,06,9A,00,20,20,20,20,20,20,20,20,20,20,20,20,00 (cmd:unknown)
        send 31D9
        H:1A _I P0:03  P1:-- 96,C8,B6 --,--,-- 96,C8,B6 31D9 11:00,06,C7,00,20,20,20,20,20,20,20,20,20,20,20,20,00 (cmd:unknown)
        receive command low
        H:1C _I P0:-- P1:-- 94,11,A9 96,C8,B6 --,--,-- 22F1 03:00,02,04 (cmd:low)
        send 31D9
        H:1A _I P0:04  P1:-- 96,C8,B6 --,--,-- 96,C8,B6 31D9 11:00,06,99,00,20,20,20,20,20,20,20,20,20,20,20,20,00 (cmd:unknown)
        send 31D9
        H:1A _I P0:05  P1:-- 96,C8,B6 --,--,-- 96,C8,B6 31D9 11:00,06,1F,00,20,20,20,20,20,20,20,20,20,20,20,20,00 (cmd:unknown)
        H:18 _I P0:-- P1:-- 96,C8,B6 --,--,-- 96,C8,B6 042F 01:00 (cmd:unknown)
        H:18 _I P0:-- P1:-- 96,C8,B6 --,--,-- 96,C8,B6 042F 09:00,FF,EA,21,D1,02,01,1B,70 (cmd:unknown)
        H:18 _I P0:-- P1:-- 96,C8,B6 --,--,-- 96,C8,B6 3120 07:00,70,B0,00,00,E9,FF (cmd:unknown)
        H:18 _I P0:-- P1:-- 96,C8,B6 --,--,-- 96,C8,B6 10E0 26:00,00,01,00,1B,31,19,01,FE,FF,FF,FF,FF,FF,0E,05,07,E2,43,56,45,2D,52,46,00,00,00,00,00,00,00,00,00,00,00,00,00,00 (cmd:unknown)
        H:18 _I P0:-- P1:-- 96,C8,B6 --,--,-- 96,C8,B6 04FF FF:18,96,C8,B6,96,C8,B6,04,FF,FF,FF,BE,F7,7F,FF,FF,FF,FF,F7,BD,FB,FB,FF,FF,9F,FF,FB,C6,FF,FF,DF,EE,FF,FF,FB,FF,DF,FF,FF,BF,5F,DF,F7,ED,F3,E7,DD,7F,F7,FB,FE,F7,F7,FF,FF,7D,FF,36,BE,D7,FE,F7,FF,FF,67,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,40,00,00,FD,97,E7,00,00,00,00,00,00,00,00,00,00,C0,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00
        H:18 _I P0:-- P1:-- 96,C8,B6 --,--,-- 96,C8,B0 0000 0F:FF,EF,FF,FF,7F,FF,FF,F7,FF,D7,DF,FF,BF,F7,FF (cmd:unknown)

        */
        sendJoinReply = false;
        rfManager.radio.setSendTries(1);
        disableRF_ISR();
        int res = rfManager.radio.sendJoinReply(joinReplyRemIndex);
        enableRF_ISR();
        D_LOG("RFI: Join reply send, result:%d", res);
        rfManager.radio.setSendTries(3);
        joinReplyRemIndex = 255;
        rf_message.once_ms(100, set_send10E0_true);
      }
      if (sendBindConfirm && !ithoCheck)
      {
        /*
          Initiator binding flow:
          1. We sent the Binding Offer (1FC9 I, self-addressed)
          2. Target responded with Binding Accept (1FC9 W, 31D9+31DA)
          3. Now send Binding Confirm (1FC9 I, payload 00, addr0=us addr1=target)
          4. Then send Battery Status (1060 x3)
        */
        sendBindConfirm = false;
        rfManager.radio.setSendTries(3);
        disableRF_ISR();
        rfManager.radio.sendBindConfirm(bindConfirmRemIndex);
        enableRF_ISR();
        D_LOG("RFI: Bind confirm sent for remote index:%d", bindConfirmRemIndex);
        // Schedule battery status after a short delay
        sendBatteryStatus = true;
      }
      if (sendBatteryStatus && !ithoCheck)
      {
        sendBatteryStatus = false;
        rfManager.radio.setSendTries(3); // send 3 times for reliability
        disableRF_ISR();
        rfManager.radio.send1060();
        enableRF_ISR();
        D_LOG("RFI: Battery status (1060) sent");
        bindConfirmRemIndex = 255;
      }
      if ((send31D9 || send31D9debug) && !ithoCheck)
      {
        // 80 82 B1 D9 01 10 86 05 0A 20 20 20 20 20 20 20 20 20 20 20 20 00 4E
        uint8_t command[] = {0x31, 0xD9, 0x11, 0x00, 0x06, 0x00, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00};

        if (send31D9)
        {
          send31D9 = false;

          uint8_t receive_buffer[256]{};
          size_t result = sendQuery31D9(&receive_buffer[0]);

          if (result > 4)
          {
            for (uint8_t i = 4; i < sizeof(command); i++)
            {
              command[i] = receive_buffer[i + 2];
            }
          }
        }
        if (send31D9debug)
        {
          send31D9debug = false;
          uint8_t speedstatus = status31D9;
          uint8_t fanstate = 0;
          fanstate |= fault31D9 ? 0x80 : 0x00;
          fanstate |= frost31D9 ? 0x40 : 0x00;
          fanstate |= filter31D9 ? 0x20 : 0x00;
          command[5] = speedstatus;
          command[4] = fanstate;
        }
        rfManager.radio.setSendTries(1);
        disableRF_ISR();
        rfManager.radio.send31D9(&command[0]);
        enableRF_ISR();
        rfManager.radio.setSendTries(3);
      }
      if ((send31DA || send31DAdebug) && !ithoCheck)
      {
        uint8_t command[32];
        build31DA(command, send31DA);
        send31DA = false;
        if (send31DAdebug)
        {
          send31DAdebug = false;

          command[21] = faninfo31DA;
          command[24] = timer31DA;
        }

        rfManager.radio.setSendTries(1);
        disableRF_ISR();
        rfManager.radio.send31DA(&command[0]);
        enableRF_ISR();
        rfManager.radio.setSendTries(3);
      }
      if (ithoCheck)
      {
        ithoCheck = false;

        rfManager.radio.decodeBufferedPacket();
        IthoPacket *packet = rfManager.radio.checkForNewPacket();
        rfManager.radio.parseMessage(packet);

        if (packet->error == 0)
        { // always fed: Spider-controller pairs are learned from their polls, also outside discovery
          using S = spider::Discovery::State;
          const spider::Frame fr{static_cast<uint8_t>((packet->header >> 4) & 0x3), packet->deviceId0, packet->deviceId1,
                                 packet->deviceId2, static_cast<uint16_t>(packet->opcode), &packet->dataDecoded[packet->payloadPos], packet->len};
          if (spiderDisc.feed(fr))
          {
            spiderDiscStepAt = millis();
            spiderDiscNotify = true;
            spiderProbeAt = 0;
            if (spiderDisc.state() == S::Done)
              spiderDiscoveryFinish();
          }
          spider::Probe pr{};
          if (spiderDisc.takeProbe(pr))
          { // send right after the controller's answer, or 300 ms after the Spider's poll if we miss it
            if (pr.now)
              spiderSendProbe(pr);
            else
            {
              spiderProbe = pr;
              spiderProbeAt = millis() + 300;
              if (spiderProbeAt == 0)
                spiderProbeAt = 1;
            }
          }
        }
        if (packet->error == 0)
          spiderOnFrame(packet);
        uint8_t *lastID = rfManager.radio.getLastID(packet);
        IthoCommand cmd = rfManager.radio.getLastCommand(packet);
        RemoteTypes remtype = rfManager.radio.getLastRemType(packet);
        bool chk = remotes.checkID(*(lastID + 0), *(lastID + 1), *(lastID + 2));
        if (logConfig.rfloglevel >= 2)
        {
          if (chk || logConfig.rfloglevel == 3)
          {
            RFDebug(packet);
          }
        }
        if (cmd != IthoUnknown)
        { // only act on good cmd
          if (logConfig.rfloglevel == 1)
          {
            RFDebug(packet);
          }
          if (cmd == IthoLeave && remotes.remoteLearnLeaveStatus())
          {
            D_LOG("RFI: Leave command received. Trying to remove remote...");
            // removeRemote() returns a status (1 = removed), not the slot: look the slot up first
            const int slot = remotes.remoteIndex(*(lastID + 0), *(lastID + 1), *(lastID + 2));
            int index = remotes.removeRemote(*(lastID + 0), *(lastID + 1), *(lastID + 2));
            if (index == 1 && slot >= 0)
            {
              rfManager.radio.removeRFDevice(slot);
            }
            switch (index)
            {
            case -1: // failed! - remote not registered
              break;
            case -2: // failed! - no remotes registered
              break;
            case 1: // success!
              saveRemotesflag = true;
              break;
            }
          }
          if (cmd == IthoBindAccept)
          {
            cancelBindInitiatorTimeout();
            bindInitiatorResult = 1;
            uint8_t remIdx = rfManager.radio.getBindInitiatorRemIndex();
            D_LOG("RFI: Binding accept received, remote index:%d", remIdx);
            // Store the target's device ID as the destination for this remote (runtime)
            rfManager.radio.updateDestinationID(*(lastID + 0), *(lastID + 1), *(lastID + 2), remIdx);
            // Persist the target device ID so it survives reboot
            remotes.updateRemoteDestID(remIdx, *(lastID + 0), *(lastID + 1), *(lastID + 2));
            saveRemotesflag = true;
            bindConfirmRemIndex = remIdx;
            // Auto-switch to RF CO2 control interface if this is an RFTCO2 send remote
            if ((remotes.getRemoteType(remIdx) == RemoteTypes::RFTCO2 ||
                 remotes.getRemoteType(remIdx) == RemoteTypes::ORCONCO2) &&
                remotes.getRemoteFunction(remIdx) == RemoteFunctions::SEND)
            {
              systemConfig.itho_control_interface = 1;
              saveSystemConfigflag = true;
              I_LOG("RFI: RF CO2 join successful, control interface set to RF CO2");
            }
            // Auto-track the Itho device as RF status source for 31DA/31D9 data
            if (remotes.getRemoteFunction(remIdx) == RemoteFunctions::SEND)
            {
              rfStatusSource *src = findOrAllocRFStatusSource(*(lastID + 0), *(lastID + 1), *(lastID + 2));
              if (src && !src->tracked)
              {
                src->tracked = true;
                const char *remName = remotes.getRemoteNamebyIndex(remIdx);
                if (remName && strlen(remName) > 0)
                  strlcpy(src->name, remName, sizeof(src->name));
                else
                  strlcpy(src->name, "Itho (auto)", sizeof(src->name));
                saveRFTrackedSources();
                I_LOG("RFI: Auto-tracked Itho device %02X:%02X:%02X as '%s'",
                      *(lastID + 0), *(lastID + 1), *(lastID + 2), src->name);
              }
            }
            // Delay the confirm to let the remaining accepts from the target pass
            rf_message.once_ms(300, []()
                               { sendBindConfirm = true; });
          }
          if (cmd == IthoJoin || (cmd == IthoLow && remtype == RemoteTypes::RFTSPIDER)) // Itho low for the spider because join can be difficult to send
          {
            D_LOG("RFI: Join command received. Trying to join remote...");
            const bool gwBind = spiderGwUntil != 0 && packet->spiderHeatingJoin; // gateway bind button, Spider offers only
            if (remotes.remoteLearnLeaveStatus() || gwBind)
            {
              int index = remotes.registerNewRemote(*(lastID + 0), *(lastID + 1), *(lastID + 2), remtype);
              if (index >= 0 && gwBind)
              { // new slot from the gateway bind: monitor only, like discovery (no fan commands)
                remotes.updateRemoteFunction(index, static_cast<uint8_t>(RemoteFunctions::MONITOR));
                remotes.updateRemoteName(index, "Spider");
              }

              if (index == -1)
              {
                // Already registered — look up existing index for re-binding
                index = remotes.remoteIndex(*(lastID + 0), *(lastID + 1), *(lastID + 2));
                D_LOG("RFI: Remote already registered at index:%d, re-binding", index);
              }

              if (index >= 0 && packet->spiderHeatingJoin)
              {
                // Spider heating join: we become its 01FF controller (ADR-0011), not the ventilation join reply
                remotes.updateRemoteBidirectional(index, true);
                remotes.setThermostat(index, true);
                {
                  const uint8_t none[3]{};
                  remotes.setThermostatCtl(index, none); // bound to us now, not to a learned controller
                  remotes.setThermostatZone(index, spider::ZONE_NONE);
                }
                rfManager.radio.updateRFDevice(index, *(lastID + 0), *(lastID + 1), *(lastID + 2), RemoteTypes::RFTSPIDER, true);
                memcpy(spiderBindID, lastID, 3);
                rfManager.radio.setThermostat(spiderBindID, true, remotes.getThermostatSetpoint(index));
                sendSpiderBind = true;
                saveRemotesflag = true;
                N_LOG("RFI: Spider %02X:%02X:%02X heating join accepted in slot %d", spiderBindID[0], spiderBindID[1], spiderBindID[2], index);
                if (gwBind)
                {
                  spiderGwUntil = 0;
                  spiderGwSpider = idTo32(spiderBindID);
                  spiderDiscSlot = index;
                  spiderDiscResult = "done_gateway";
                  spiderDiscNotify = true;
                }
              }
              else if (index < 0 && gwBind)
              {
                spiderGwUntil = 0;
                spiderDiscResult = "noslot";
                spiderDiscNotify = true;
              }
              else if (index >= 0)
              {
                // Auto-detect bidirectional from remote type (unless monitor mode)
                bool bidirectional = (remtype == RemoteTypes::RFTAUTON || remtype == RemoteTypes::RFTN ||
                                      remtype == RemoteTypes::RFTCO2 || remtype == RemoteTypes::RFTRV ||
                                      remtype == RemoteTypes::RFTSPIDER)
                                         ? (remotes.getRemoteFunction(index) != RemoteFunctions::MONITOR)
                                         : false;
                remotes.updateRemoteBidirectional(index, bidirectional);
                rfManager.radio.updateRFDevice(index, *(lastID + 0), *(lastID + 1), *(lastID + 2), remotes.getRemoteType(index), bidirectional);

                if (bidirectional)
                {
                  joinReplyRemIndex = index;
                  sendJoinReply = true;
                }
                else if (remotes.getRemoteFunction(index) == RemoteFunctions::MONITOR)
                {
                  D_LOG("RFI: Remote index:%d is monitor-only, not sending join reply", index);
                }

                saveRemotesflag = true;
              }
              // index == -2: max number of remotes reached
            }
            if (virtualRemotes.remoteLearnLeaveStatus())
            {
              int result = virtualRemotes.registerNewRemote(*(lastID + 0), *(lastID + 1), *(lastID + 2), remtype);
              D_LOG("RFI: registerNewRemote:%d", result);

              if (result >= 0)
              {
                saveVremotesflag = true;
                // delay(200);
                toggleRemoteLLmode("vremote");
              }
              else
              {
                // error
                // case -1: // failed! - remote already registered
                //   break;
                // case -2: // failed! - max number of remotes reached"
              }
            }
          }
          if (chk)
          {
            int index = remotes.remoteIndex(*(lastID + 0), *(lastID + 1), *(lastID + 2));
            remotes.lastRemoteName = remotes.getRemoteNamebyIndex(index);
            RemoteFunctions remfunc = remotes.getRemoteFunction(index);
            bool bidirect = remotes.getRemoteBidirectional(index);
            if (remfunc != RemoteFunctions::MONITOR)
            {
              D_LOG("RFI: remfunc:%d, bidirect:%s", remfunc, bidirect ? "yes" : "no");
              if (cmd == IthoLow)
              {
                ithoExecCommand("low", REMOTE);
              }
              if (cmd == IthoMedium)
              {
                ithoExecCommand("medium", REMOTE);
              }
              if (cmd == IthoHigh)
              {
                ithoExecCommand("high", REMOTE);
              }
              if (cmd == IthoTimer1)
              {
                ithoExecCommand("timer1", REMOTE);
              }
              if (cmd == IthoTimer2)
              {
                ithoExecCommand("timer2", REMOTE);
              }
              if (cmd == IthoTimer3)
              {
                ithoExecCommand("timer3", REMOTE);
              }
              if (cmd == IthoCook30)
              {
                ithoExecCommand("cook30", REMOTE);
              }
              if (cmd == IthoCook60)
              {
                ithoExecCommand("cook60", REMOTE);
              }
              if (cmd == IthoAuto)
              {
                ithoExecCommand("auto", REMOTE);
              }
              if (cmd == IthoAutoNight)
              {
                ithoExecCommand("autonight", REMOTE);
              }
              if (cmd == IthoJoin && !remotes.remoteLearnLeaveStatus())
              {
              }
              if (cmd == IthoLeave && !remotes.remoteLearnLeaveStatus())
              {
                ithoQueue.clearQueue();
              }
              if (cmd == Itho31D9)
              {
                // send31D9 = true;
              }
              if (cmd == Itho31DA && ((packet->header >> 4) & 0x3) == 0 && rfManager.radio.isOwnID(packet->deviceId1))
              { // RQ 31DA from a bound remote (an RFT CO2 asks right after joining)
                reply31DA(lastID, true);
              }
              if (cmd == IthoDeviceInfo)
              {
                // send10E0 = true;
              }
              if (bidirect && cmd != IthoJoin && cmd != IthoLeave && cmd != Itho31DA && cmd != Itho31D9 && cmd != IthoDeviceInfo)
              { // acknowledge a bi-directional remote's command with the fan status addressed to it, at once
                reply31DA(lastID, false);
              }
            }
          }
          else
          {
            // Unknown remote
          }
        }
        else
        {
          //("--- RF CMD reveiced but of unknown type ---");
        }

        const ithoRFDevices &rfDevices = rfManager.radio.getRFdevices();
        for (auto &item : rfDevices.device)
        {
          if (item.destinationID[0] == 0 && item.destinationID[1] == 0 && item.destinationID[2] == 0)
            continue;
          // if (item.bidirectional && (cmd != IthoJoin && cmd != IthoLeave))
          // { // trigger fan status update if there is a bi-directional remote in the known list of remotes
          //   send31D9 = true;
          //   send31DA = true;
          // }
          int remIndex = remotes.remoteIndex(item.destinationID[0], item.destinationID[1], item.destinationID[2]);
          if (remIndex >= 0)
          {
            remotes.addCapabilities(remIndex, "timestamp", item.timestamp);
            remotes.addCapabilities(remIndex, "lastcmd", item.lastCommand);
            if (item.co2 != 0xEFFF)
            {
              remotes.addCapabilities(remIndex, "co2", item.co2);
            }
            if (item.temp != 0xEFFF)
            {
              remotes.addCapabilities(remIndex, "temp", item.temp);
            }
            if (item.hum != 0xEFFF)
            {
              remotes.addCapabilities(remIndex, "hum", item.hum);
            }
            if (item.dewpoint != 0xEFFF)
            {
              remotes.addCapabilities(remIndex, "dewpoint", item.dewpoint);
            }
            if (item.battery != 0xEFFF)
            {
              remotes.addCapabilities(remIndex, "battery", item.battery);
            }
            if (item.pir != 0xEF)
            {
              remotes.addCapabilities(remIndex, "pir", item.pir);
            }
            if (item.setpoint != 0xEFFF)
            {
              remotes.addCapabilities(remIndex, "setpoint", item.setpoint);
            }
            if (item.setpointCool != 0xEFFF)
            {
              remotes.addCapabilities(remIndex, "setpoint_cool", item.setpointCool);
            }
            if (item.thermostat)
            {
              remotes.addCapabilities(remIndex, "thermostat", 1);
              if (item.setpointCmd != 0xEFFF)
                remotes.addCapabilities(remIndex, "setpoint_cmd", item.setpointCmd);
            }
            if (item.demand != 0xFF)
            {
              remotes.addCapabilities(remIndex, "demand", item.demand / 2);
            }
            if (item.spiderFlags != 0xEF)
            {
              remotes.addCapabilities(remIndex, "schedule", (item.spiderFlags & spider::FLAG_SCHEDULE) ? 1 : 0);
              remotes.addCapabilities(remIndex, "manual", (item.spiderFlags & spider::FLAG_MANUAL) ? 1 : 0);
            }
            //
          }
        }
        if (rfManager.radio.consumeRemoteStatusChanged())
          updateMQTTRemotesInfo = true;
        //} //while
      }
      TaskCC1101HWmark = uxTaskGetStackHighWaterMark(NULL);
      // Block until the RF ISR notifies that a packet arrived, or a 25 ms tick
      // to run the periodic work above. On notification, read the FIFO here in
      // task context (moved out of the ISR); the decode block above processes
      // it on the next iteration.
      if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(25)) > 0)
      {
        if (radioSem)
          xSemaphoreTake(radioSem, portMAX_DELAY);
        ithoCheck = rfManager.radio.receivePacket();
        if (radioSem)
          xSemaphoreGive(radioSem);
      }
    }
  } // if (systemConfig.itho_rf_support)
  else 
  {
    I_LOG("SYS: CC1101 RF module support disabled");
  }
  TaskHandle_t xTempTask = xTaskCC1101Handle;
  xTaskCC1101Handle = NULL;
  vTaskDelete(xTempTask);
}
