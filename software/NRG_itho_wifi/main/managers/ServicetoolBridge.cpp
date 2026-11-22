/*
   Servicetool bridge - implementation
   See ServicetoolBridge.h for the overall behaviour and
   ServicetoolProtocol.h for the wire format.

   Threading:
     - TaskServicetool owns both sockets (listen + client). Only this task
       reads from or writes to them. It is created in boot() and gates on
       PHASE_CONFIG | PHASE_NET (ADR-0010).
     - The client loop is non-blocking: pings are answered immediately,
       frames are queued and executed one at a time as items on the existing
       I2C command queue (run by TaskSysControl), so they are serialised with
       all other I2C traffic. Each item holds the I2C bus lock across send +
       receive. The loop picks up the finished item and forwards the reply.
     - Session state is exposed through volatile flags + a mutex-protected
       info struct; MQTT publishing happens in the MQTT task.

   Part of NRG itho wifi firmware
*/

#include "managers/ServicetoolBridge.h"
#include "managers/ServicetoolProtocol.h"

#include <memory>
#include <deque>
#include <vector>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <unistd.h>

#include "lwip/sockets.h"
#include "lwip/inet.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "globals.h"
#include "sys_log.h"
#include "config/SystemConfig.h"
#include "ithodevice/i2c_esp32.h"
#include "ithodevice/IthoDevice.h"
#include "tasks/task_mqtt.h"
#include "tasks/boot_phases.h"

using servicetool::DleDecoder;

// ---------------------------------------------------------------------------
// tuning
// ---------------------------------------------------------------------------

// Low priority: the task only shuffles bytes; the time-critical I2C work runs
// in TaskSysControl via the I2C queue.
#define TASK_SERVICETOOL_PRIO 3
// 8-bit form of our own I2C slave address (0x40 << 1 = 0x80). Prepended to
// replies that arrive in the same receive window as a previous reply, because
// the slave peripheral strips the address byte.
#define SERVICETOOL_OWN_ADDRESS (I2C_SLAVE_ADDRESS << 1)
// Extra time a frame may spend waiting in the I2C queue on top of its reply
// window before the bridge gives up on it.
#define SERVICETOOL_QUEUE_EXTRA_WAIT_MS 2500
// Republish the session state on MQTT this often during a session, so
// last_activity stays current in Home Assistant.
#define SERVICETOOL_MQTT_REFRESH_MS 60000UL
// Log only the first N commands refused during a session (then a summary).
#define SERVICETOOL_BLOCK_LOG_LIMIT 5
// Maximum frames from the servicetool waiting for the bus; the oldest is
// dropped beyond this (the servicetool has long given up on it by then).
#define SERVICETOOL_MAX_PENDING 16

// ---------------------------------------------------------------------------
// shared flags (declared in ServicetoolBridge.h)
// ---------------------------------------------------------------------------

// Set/cleared by TaskServicetool. Read by the I2C driver guard,
// TaskSysControl (polling pause) and the APIs (command refusal).
volatile bool servicetoolSessionActive = false;
// Set by TaskServicetool when the session state changed; cleared by the MQTT
// task after publishing. Starts true so "inactive" is published after boot.
volatile bool servicetoolMqttUpdate = true;
// Set by TaskServicetool when a session ends; cleared by TaskSysControl after
// scheduling an immediate status refresh.
volatile bool servicetoolResumePolling = false;
// Set by the web UI ("End session"); cleared by TaskServicetool.
volatile bool servicetoolEndRequested = false;

namespace
{
  // Information about the current session, for MQTT / REST / web UI.
  // Written by TaskServicetool, read by other tasks -> guarded by infoMutex.
  struct SessionInfo
  {
    char client[16]{};       // IPv4 address of the servicetool PC
    time_t since{0};         // session start (epoch)
    time_t lastActivity{0};  // last byte received from the servicetool (epoch)
    uint32_t framesTx{0};    // frames put on the bus
    uint32_t framesRx{0};    // reply frames forwarded to the servicetool
    uint32_t framesRejected{0}; // malformed, non-I2C or blocked frames
  };

  SessionInfo info;
  SemaphoreHandle_t infoMutex = nullptr;
  // Number of I2C calls refused by the driver guard in this session. Written
  // from TaskSysControl and other tasks, hence separate from SessionInfo.
  volatile uint32_t blockedCount = 0;

  TaskHandle_t taskHandle = nullptr;

  // One servicetool frame on its way over the bus.
  //
  // Lifetime: shared (std::shared_ptr) between the bridge task, which waits
  // for it, and the lambda on the I2C queue, which executes it in
  // TaskSysControl. Whichever finishes last frees it, so the bridge can stop
  // waiting (timeout, disconnect) without the queued lambda touching freed
  // memory.
  struct BridgeRequest
  {
    uint8_t tx[servicetool::MAX_FRAME]{}; // frame from the servicetool
    size_t txLen{0};
    uint8_t rx[I2C_SLAVE_RX_BUF_LEN]{};   // whatever arrived as I2C slave
    size_t rxLen{0};
    bool sent{false};                     // master write was ACKed
    uint32_t windowMs{50};                // wait for the first reply byte
    unsigned long queuedAt{0};            // millis() when queued
    volatile bool abort{false};           // stop waiting for the reply
    SemaphoreHandle_t done{nullptr};      // given by the I2C task when finished

    BridgeRequest() { done = xSemaphoreCreateBinary(); }
    ~BridgeRequest()
    {
      if (done)
        vSemaphoreDelete(done);
    }
  };

  // -------------------------------------------------------------------------
  // small helpers
  // -------------------------------------------------------------------------

  time_t nowEpoch()
  {
    time_t now;
    time(&now);
    return now;
  }

  void lockInfo()
  {
    if (infoMutex)
      xSemaphoreTake(infoMutex, portMAX_DELAY);
  }
  void unlockInfo()
  {
    if (infoMutex)
      xSemaphoreGive(infoMutex);
  }

  // -------------------------------------------------------------------------
  // socket helpers (only used by TaskServicetool)
  // -------------------------------------------------------------------------

  void closeFd(int &fd)
  {
    if (fd >= 0)
    {
      shutdown(fd, SHUT_RDWR);
      close(fd);
      fd = -1;
    }
  }

  // send() until everything is written. False if the connection broke.
  bool sendAll(int fd, const uint8_t *buf, size_t len)
  {
    size_t off = 0;
    while (off < len)
    {
      int n = send(fd, buf + off, len - off, 0);
      if (n <= 0)
        return false;
      off += static_cast<size_t>(n);
    }
    return true;
  }

  // TCP listen socket on all interfaces; one pending connection is enough
  // (only one servicetool can be served at a time). Returns -1 on failure.
  int openListenSocket(uint16_t port)
  {
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0)
      return -1;

    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) != 0 || listen(fd, 1) != 0)
    {
      close(fd);
      return -1;
    }
    return fd;
  }

  void configureClientSocket(int fd)
  {
    // small frames must go out immediately (no Nagle delay), the servicetool
    // only waits a limited time for a reply
    int yes = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));

    // detect half-open connections (laptop lid closed, WiFi gone)
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes));
    int idle = 30, intvl = 10, cnt = 3;
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt));

    // never block forever on a send to a stalled client
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
  }

  // Wait until fd is readable or timeoutMs passes. >0 readable, 0 timeout, <0 error.
  int waitReadable(int fd, uint32_t timeoutMs)
  {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
    return select(fd + 1, &rfds, nullptr, nullptr, &tv);
  }

  // -------------------------------------------------------------------------
  // session bookkeeping
  // -------------------------------------------------------------------------

  // A client connected: the bus now belongs to the servicetool. Setting
  // servicetoolSessionActive pauses polling and arms the command guards.
  void startSession(const char *client)
  {
    lockInfo();
    info = SessionInfo();
    strlcpy(info.client, client, sizeof(info.client));
    info.since = nowEpoch();
    info.lastActivity = info.since;
    unlockInfo();
    blockedCount = 0;

    servicetoolSessionActive = true;
    servicetoolMqttUpdate = true;
    sysStatReq = true;
    N_LOG("ST: servicetool session started from %s - I2C polling paused, commands blocked", client);
  }

  // The client is gone: give the bus back and ask TaskSysControl for an
  // immediate status refresh so HA/MQTT get fresh values.
  void endSession(const char *reason)
  {
    if (!servicetoolSessionActive)
      return;

    servicetoolSessionActive = false;
    servicetoolResumePolling = true;
    servicetoolMqttUpdate = true;
    sysStatReq = true;

    lockInfo();
    uint32_t tx = info.framesTx, rx = info.framesRx, rej = info.framesRejected;
    unlockInfo();
    N_LOG("ST: servicetool session ended (%s) - frames tx:%u rx:%u rejected:%u, blocked commands:%u",
          reason, tx, rx, rej, static_cast<unsigned>(blockedCount));
  }

  void touchActivity()
  {
    lockInfo();
    info.lastActivity = nowEpoch();
    unlockInfo();
  }

  void countRejected()
  {
    lockInfo();
    info.framesRejected++;
    unlockInfo();
  }

  // -------------------------------------------------------------------------
  // frame handling
  // -------------------------------------------------------------------------

  // Check a frame from the servicetool before it goes on the bus.
  bool acceptFrame(const uint8_t *frame, size_t len)
  {
    if (!servicetool::frameValid(frame, len))
    {
      // Short non-I2C frames (e.g. 00 00 00) are not meant for the bus. In
      // practice they appear when a ping was answered too late.
      countRejected();
      if (len == 3)
        W_LOG("ST: ignored 3-byte interface command 0x%02X (late ping?)", frame[2]);
      else
        W_LOG("ST: rejected malformed frame from servicetool (len:%u)", static_cast<unsigned>(len));
      return false;
    }
    uint16_t msgClass = servicetool::frameClass(frame);
    if (servicetool::classBlocked(msgClass))
    {
      countRejected();
      W_LOG("ST: blocked firmware/bootloader frame class 0x%04X (flashing over WiFi is disabled)", msgClass);
      return false;
    }
    return true;
  }

  // Put one frame on the I2C queue. Returns nullptr if it could not be queued.
  std::shared_ptr<BridgeRequest> dispatchFrame(const std::vector<uint8_t> &frame)
  {
    auto req = std::make_shared<BridgeRequest>();
    if (!req->done)
      return nullptr;
    memcpy(req->tx, frame.data(), frame.size());
    req->txLen = frame.size();
    req->windowMs = servicetool::replyWindowMs(frame.data());
    req->queuedAt = millis();

    if (i2cManager.queueSize() >= I2C_CMD_QUEUE_MAX_SIZE)
    {
      W_LOG("ST: I2C queue full, frame dropped");
      return nullptr;
    }

    D_LOG("ST: tx %s", i2cBufToString(req->tx, req->txLen).c_str());

    i2cQueueAddCmd([req]()
                   {
                     // hold the bus across send + receive so no other task can
                     // use it between the servicetool's question and the reply
                     I2CBusLock lock;
                     req->sent = i2cSendBytes(req->tx, req->txLen, I2C_CMD_SERVICETOOL);
                     if (req->sent)
                       req->rxLen = i2cSlaveReceive(req->rx, req->windowMs, &req->abort);
                     xSemaphoreGive(req->done); });
    return req;
  }

  // Forward whatever the bus returned for a finished request.
  // Returns false if the TCP connection broke.
  bool finishRequest(int clientFd, const std::shared_ptr<BridgeRequest> &req)
  {
    uint16_t msgClass = servicetool::frameClass(req->tx);
    lockInfo();
    if (req->sent)
      info.framesTx++;
    unlockInfo();

    if (!req->sent)
    {
      // e.g. no node at that address (the servicetool probes 0x80, 0x60..0x66, 0x20)
      D_LOG("ST: no ACK on bus for class 0x%04X to 0x%02X", msgClass, req->tx[0]);
      return true;
    }
    if (req->rxLen == 0)
    {
      D_LOG("ST: no reply for class 0x%04X%s", msgClass, req->abort ? " (superseded by next frame)" : "");
      return true;
    }

    D_LOG("ST: rx %s", i2cBufToString(req->rx, req->rxLen).c_str());

    bool ok = true;
    static uint8_t out[servicetool::MAX_FRAME * 2 + 4];
    size_t frames = servicetool::splitReceived(
        req->rx, req->rxLen, SERVICETOOL_OWN_ADDRESS,
        [&](const uint8_t *f, size_t flen)
        {
          size_t n = servicetool::dleEncode(f, flen, out, sizeof(out));
          if (n && ok)
            ok = sendAll(clientFd, out, n);
        });

    if (frames == 0)
      W_LOG("ST: could not parse reply (%u bytes)", static_cast<unsigned>(req->rxLen));

    lockInfo();
    info.framesRx += frames;
    unlockInfo();
    return ok;
  }

  // -------------------------------------------------------------------------
  // client loop
  // -------------------------------------------------------------------------

  // State of one connected client, owned by serveClient().
  struct ClientState
  {
    int fd;
    DleDecoder decoder;                            // bytes -> pings / frames
    std::deque<std::vector<uint8_t>> pending;      // frames waiting for the bus
    std::shared_ptr<BridgeRequest> inflight;       // frame on the bus right now
    unsigned long lastActivityMs{millis()};        // for the inactivity timeout
    unsigned long lastMqttRefreshMs{millis()};
  };

  // Reason to end the session now, or nullptr to keep going.
  const char *sessionEndReason(ClientState &c)
  {
    if (servicetoolEndRequested)
    {
      servicetoolEndRequested = false;
      return "ended from web interface";
    }
    if (systemConfig.servicetool_bridge != 1)
      return "bridge disabled";
    uint32_t timeoutMs = static_cast<uint32_t>(systemConfig.servicetool_timeout) * 60000UL;
    if (timeoutMs > 0 && millis() - c.lastActivityMs > timeoutMs)
      return "inactivity timeout";
    return nullptr;
  }

  // If the frame on the bus has finished, forward its reply; if the I2C queue
  // never got to it, give up on it. False if the connection broke.
  bool collectFinishedRequest(ClientState &c)
  {
    if (!c.inflight)
      return true;

    if (xSemaphoreTake(c.inflight->done, 0) == pdTRUE)
    {
      bool ok = finishRequest(c.fd, c.inflight);
      c.inflight.reset();
      return ok;
    }
    if (millis() - c.inflight->queuedAt > c.inflight->windowMs + SERVICETOOL_QUEUE_EXTRA_WAIT_MS)
    {
      W_LOG("ST: I2C queue did not execute frame in time, skipped");
      c.inflight->abort = true;
      c.inflight.reset(); // the queued lambda keeps its own reference
    }
    return true;
  }

  // Put the next waiting frame on the bus (one at a time).
  void startNextRequest(ClientState &c)
  {
    while (!c.inflight && !c.pending.empty())
    {
      c.inflight = dispatchFrame(c.pending.front());
      c.pending.pop_front();
    }
  }

  // Feed received bytes to the decoder: answer pings straight away, queue
  // frames. False if the connection broke.
  bool handleClientBytes(ClientState &c, const uint8_t *buf, int n)
  {
    for (int i = 0; i < n; i++)
    {
      switch (c.decoder.feed(buf[i]))
      {
      case DleDecoder::Event::Ping:
      {
        // must be answered quickly (within about 1 s), otherwise the
        // servicetool does not recognise the interface or stops working
        const uint8_t pong[2] = {servicetool::DLE, servicetool::SYN};
        if (!sendAll(c.fd, pong, sizeof(pong)))
          return false;
        break;
      }
      case DleDecoder::Event::Frame:
        if (acceptFrame(c.decoder.data(), c.decoder.size()))
        {
          // a new frame means the servicetool stopped waiting for the
          // previous reply: free the bus instead of waiting it out
          if (c.inflight)
            c.inflight->abort = true;
          if (c.pending.size() >= SERVICETOOL_MAX_PENDING)
          {
            W_LOG("ST: too many pending frames, oldest dropped");
            c.pending.pop_front();
          }
          c.pending.emplace_back(c.decoder.data(), c.decoder.data() + c.decoder.size());
        }
        break;
      case DleDecoder::Event::Overflow:
        countRejected();
        W_LOG("ST: frame from servicetool too long, dropped");
        break;
      default:
        break; // ACK/NAK/noise: not needed
      }
    }
    return true;
  }

  // Serve one connected client until it disconnects or the session ends.
  //
  // Non-blocking event loop: the socket is read continuously, so pings are
  // answered immediately even while an I2C request is in flight. Frames are
  // executed one at a time; a new frame aborts the wait for the previous
  // reply. Without this, unanswered queries pile up, replies get later and
  // later, pings time out and the servicetool stops working.
  void serveClient(int clientFd)
  {
    ClientState c;
    c.fd = clientFd;
    uint8_t buf[256];
    const char *reason = "client disconnected";

    for (;;)
    {
      if (const char *r = sessionEndReason(c))
      {
        reason = r;
        break;
      }
      if (millis() - c.lastMqttRefreshMs > SERVICETOOL_MQTT_REFRESH_MS)
      {
        c.lastMqttRefreshMs = millis();
        servicetoolMqttUpdate = true;
      }

      // 1. reply of the frame on the bus -> servicetool
      if (!collectFinishedRequest(c))
      {
        reason = "send to client failed";
        break;
      }
      // 2. next waiting frame -> bus
      startNextRequest(c);

      // 3. read from the servicetool; poll fast while frames are in progress
      int r = waitReadable(c.fd, (c.inflight || !c.pending.empty()) ? 10 : 200);
      if (r < 0)
      {
        reason = "socket error";
        break;
      }
      if (r == 0)
        continue;

      int n = recv(c.fd, buf, sizeof(buf), 0);
      if (n == 0)
        break; // orderly close by the client
      if (n < 0)
      {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
          continue;
        reason = "connection lost";
        break;
      }

      c.lastActivityMs = millis();
      touchActivity();

      if (!handleClientBytes(c, buf, n))
      {
        reason = "send to client failed";
        break;
      }
    }
    if (c.inflight)
      c.inflight->abort = true; // don't keep the bus busy for a gone client
    endSession(reason);
  }

  // -------------------------------------------------------------------------
  // listener task
  // -------------------------------------------------------------------------

  // HA discovery content depends on these settings: resend it when they change.
  void resendDiscoveryOnSettingChange()
  {
    static uint8_t lastHaUnavail = systemConfig.servicetool_ha_unavail;
    static uint8_t lastEnabled = systemConfig.servicetool_bridge;
    if (systemConfig.servicetool_ha_unavail != lastHaUnavail || systemConfig.servicetool_bridge != lastEnabled)
    {
      lastHaUnavail = systemConfig.servicetool_ha_unavail;
      lastEnabled = systemConfig.servicetool_bridge;
      sendHomeAssistantDiscovery = true;
      servicetoolMqttUpdate = true;
    }
  }

  // Whether a connection from this client may start a session.
  bool clientAllowed(const char *ip)
  {
    if (systemConfig.servicetool_allow_ip[0] != '\0' && strcmp(systemConfig.servicetool_allow_ip, ip) != 0)
    {
      W_LOG("ST: refused servicetool connection from %s (not the allowed IP)", ip);
      return false;
    }
    if (systemConfig.itho_rf_standalone == 1)
    {
      W_LOG("ST: refused servicetool connection - RF standalone mode has no I2C connection");
      return false;
    }
    return true;
  }

  // Listens while the bridge is enabled and serves one client at a time.
  // Reacts to setting changes (enable/disable, port) within about a second.
  void TaskServicetool(void * /*pvParameters*/)
  {
    waitPhase(PHASE_CONFIG | PHASE_NET); // needs loaded config and the network

    int listenFd = -1;
    uint16_t listenPort = 0;

    for (;;)
    {
      resendDiscoveryOnSettingChange();

      // bridge off: close the listen socket and idle
      if (systemConfig.servicetool_bridge != 1 || systemConfig.servicetool_port == 0)
      {
        if (listenFd >= 0)
        {
          closeFd(listenFd);
          I_LOG("ST: servicetool bridge stopped");
        }
        servicetoolEndRequested = false;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        continue;
      }

      // (re)open the listen socket when needed, e.g. after a port change
      if (listenFd < 0 || listenPort != systemConfig.servicetool_port)
      {
        closeFd(listenFd);
        listenPort = systemConfig.servicetool_port;
        listenFd = openListenSocket(listenPort);
        if (listenFd < 0)
        {
          E_LOG("ST: error - could not listen on TCP port %u", listenPort);
          vTaskDelay(5000 / portTICK_PERIOD_MS);
          continue;
        }
        I_LOG("ST: servicetool bridge listening on TCP port %u", listenPort);
      }

      servicetoolEndRequested = false; // only meaningful during a session

      // wait (bounded, so setting changes are noticed) for a connection
      int r = waitReadable(listenFd, 500);
      if (r <= 0)
      {
        if (r < 0)
          closeFd(listenFd); // reopen next round
        continue;
      }

      struct sockaddr_in peer;
      socklen_t plen = sizeof(peer);
      int clientFd = accept(listenFd, reinterpret_cast<struct sockaddr *>(&peer), &plen);
      if (clientFd < 0)
        continue;

      char ip[16]{};
      inet_ntoa_r(peer.sin_addr, ip, sizeof(ip));
      if (!clientAllowed(ip))
      {
        closeFd(clientFd);
        continue;
      }

      // serve this client until it leaves; others wait in the listen backlog
      configureClientSocket(clientFd);
      startSession(ip);
      serveClient(clientFd);
      closeFd(clientFd);
    }
  }

} // namespace

// ---------------------------------------------------------------------------
// public API
// ---------------------------------------------------------------------------

// Created in boot() together with the other tasks; the task itself waits for
// PHASE_CONFIG | PHASE_NET before doing anything.
void startTaskServicetoolBridge()
{
  if (taskHandle != nullptr)
    return;
  if (infoMutex == nullptr)
    infoMutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(
      TaskServicetool,
      "TaskServicetool",
      STACK_SIZE_MEDIUM,
      nullptr,
      TASK_SERVICETOOL_PRIO,
      &taskHandle,
      CONFIG_ARDUINO_RUNNING_CORE);
}

// Called by the I2C driver guard (any task) for every refused call.
void servicetoolNoteBlocked(i2c_cmdref_t origin)
{
  uint32_t n = ++blockedCount;
  if (n <= SERVICETOOL_BLOCK_LOG_LIMIT)
    W_LOG("ST: I2C command blocked during servicetool session (origin:%d)", static_cast<int>(origin));
  else if (n == SERVICETOOL_BLOCK_LOG_LIMIT + 1)
    W_LOG("ST: further blocked I2C commands are not logged individually");
}

void servicetoolGetStatusJSON(JsonObject obj, bool full)
{
  bool active = servicetoolSessionActive;
  obj["state"] = active ? "active" : "inactive";
  obj["enabled"] = systemConfig.servicetool_bridge == 1;
  if (!active)
    return;

  lockInfo();
  SessionInfo copy = info;
  unlockInfo();

  obj["client"] = copy.client;
  obj["since"] = copy.since;
  obj["last_activity"] = copy.lastActivity;
  obj["timeout_min"] = systemConfig.servicetool_timeout;
  if (full)
  {
    obj["frames_tx"] = copy.framesTx;
    obj["frames_rx"] = copy.framesRx;
    obj["frames_rejected"] = copy.framesRejected;
    obj["blocked_commands"] = static_cast<uint32_t>(blockedCount);
  }
}

void servicetoolMqttTopic(char *buf, size_t len)
{
  snprintf(buf, len, "%s%s", systemConfig.mqtt_base_topic, "/servicetool");
}
