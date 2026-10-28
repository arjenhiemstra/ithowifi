#pragma once
// Discovery of an Itho Spider thermostat and the heater (or relay) it is bound to (ADR-0011).
// The user sets the Spider to one random setpoint (21.5-24 C). Some Spiders broadcast I 22C9 at once,
// relay-bound ones only every ~15 min, but they do send their raised heat demand (I 0008) at once and
// then poll their controller (RQ 3EF0 / 3EF1) about every minute. Those Spiders become candidates and
// are asked once per poll with RQ 01FF, sent as their controller; the RP 01FF setpoint identifies them.
// Spider-controller pairs are learned from the polls all the time, not only during discovery.
// Dependency-free so the native unit tests can include it directly.

#include <cstddef>
#include <cstdint>

namespace spider
{

struct Frame
{
  uint8_t msgType; // 0 RQ, 1 I, 2 W, 3 RP
  uint32_t addr0;  // 0 = absent
  uint32_t addr1;
  uint32_t addr2;
  uint16_t opcode;
  const uint8_t *payload;
  size_t len;
};

constexpr uint8_t MSG_RQ = 0;
constexpr uint8_t MSG_I = 1;
constexpr uint8_t MSG_W = 2;
constexpr uint8_t MSG_RP = 3;

// RQ 01FF to send to a candidate, as its controller: now (controller answered its poll) or after a short delay
struct Probe
{
  uint32_t spider;
  uint32_t ctl;
  bool now;
};

class Discovery
{
public:
  enum class State : uint8_t
  {
    Idle,
    WaitSetpoint, // waiting for a Spider that reports the target setpoint
    WaitHeater,   // Spider known, waiting for traffic that names its controller
    Done
  };

  static constexpr int32_t MIN_TARGET = 2150; // 21.5 C
  static constexpr int32_t TARGET_STEPS = 6;  // 21.5 .. 24.0 in 0.5 C steps; Spiders on an AutoTemp go up to 24 C
  static constexpr size_t MAX_CANDIDATES = 4;
  static constexpr size_t MAX_PAIRS = 8;
  static constexpr uint8_t MAX_PROBES = 3; // per candidate

  static int32_t pickTarget(uint32_t rnd) { return MIN_TARGET + static_cast<int32_t>(rnd % TARGET_STEPS) * 50; }

  void start(int32_t target)
  {
    nCand_ = 0;
    spider_ = heater_ = 0;
    probe_ = Probe{};
    target_ = target;
    state_ = State::WaitSetpoint;
  }
  void cancel() { state_ = State::Idle; }
  State state() const { return state_; }
  int32_t target() const { return target_; }
  uint32_t spider() const { return spider_; }
  uint32_t heater() const { return heater_; }
  size_t candidates() const { return nCand_; }
  uint32_t controllerOf(uint32_t spider) const
  {
    for (size_t i = 0; i < nPair_; i++)
      if (pair_[i].spider == spider)
        return pair_[i].ctl;
    return 0;
  }

  // Feed every received frame (checksum OK), also while idle. Returns true when the state advanced.
  bool feed(const Frame &f)
  {
    const uint32_t src = f.addr0 != 0 ? f.addr0 : f.addr2;
    if (src == 0)
      return false;
    learnPair(f);
    switch (state_)
    {
    case State::WaitSetpoint:
    {
      if (f.opcode == 0x0008 && f.msgType == MSG_I && f.len >= 2 && f.payload)
      { // heat demand went up: the Spider the user just turned up?
        const uint8_t v = f.payload[1];
        if (v > 0 && v <= 200 && findCand(src) == nullptr && nCand_ < MAX_CANDIDATES)
          cand_[nCand_++] = Cand{src, 0, false};
        return false;
      }
      offerProbe(f);
      int32_t sp = 0;
      uint32_t ctl = 0;
      if (!reportedSetpoint(f, sp, ctl))
        return false;
      if (sp != target_)
      {
        if (f.msgType == MSG_RP)
          if (Cand *c = findCand(f.addr0))
            c->rejected = true; // answered our probe with another setpoint: not the one
        return false;
      }
      spider_ = f.addr0 != 0 ? f.addr0 : f.addr2;
      heater_ = ctl != 0 ? ctl : controllerOf(spider_);
      state_ = heater_ != 0 ? State::Done : State::WaitHeater;
      return true;
    }
    case State::WaitHeater:
      heater_ = controllerOf(spider_);
      if (heater_ == 0)
        return false;
      state_ = State::Done;
      return true;
    default:
      return false;
    }
  }

  // A probe the caller should send (at most one pending); clears it
  bool takeProbe(Probe &p)
  {
    if (probe_.spider == 0)
      return false;
    p = probe_;
    probe_ = Probe{};
    return true;
  }
  // The caller transmitted a probe to this Spider
  void probeSent(uint32_t spider)
  {
    if (Cand *c = findCand(spider))
      c->probes++;
  }

private:
  struct Cand
  {
    uint32_t id;
    uint8_t probes;
    bool rejected;
  };
  struct Pair
  {
    uint32_t spider;
    uint32_t ctl;
  };

  // Setpoint a Spider reports: I 22C9 (0.01 C) or 01FF byte 3 (0.5 C; RQ/RP go to its controller)
  static bool reportedSetpoint(const Frame &f, int32_t &centi, uint32_t &ctl)
  {
    if (f.payload == nullptr)
      return false;
    if (f.opcode == 0x22C9 && f.msgType == MSG_I && f.len >= 3)
    {
      centi = static_cast<int32_t>(f.payload[1]) << 8 | f.payload[2];
      return true;
    }
    if (f.opcode == 0x01FF && f.msgType != MSG_W && f.len >= 26)
    {
      centi = static_cast<int32_t>(f.payload[3]) * 50;
      if (f.msgType != MSG_I)
        ctl = f.addr1;
      return true;
    }
    return false;
  }

  // Spider polls its heater (3EF0), relay (3EF1) or gateway (01FF); the controller answers or echoes 3150
  void learnPair(const Frame &f)
  {
    uint32_t s = 0, c = 0;
    if (f.msgType == MSG_RQ && (f.opcode == 0x3EF0 || f.opcode == 0x3EF1 || (f.opcode == 0x01FF && f.len >= 26)))
    { // a 1-byte RQ 01FF is a controller asking the Spider, not the Spider asking
      s = f.addr0;
      c = f.addr1;
    }
    else if (f.msgType == MSG_RP && (f.opcode == 0x3EF0 || f.opcode == 0x3EF1))
    {
      s = f.addr1;
      c = f.addr0;
    }
    else if (f.opcode == 0x3150 && f.addr2 != 0 && f.addr0 != f.addr2)
    {
      s = f.addr2;
      c = f.addr0;
    }
    if (s == 0 || c == 0 || s == c)
      return;
    for (size_t i = 0; i < nPair_; i++)
      if (pair_[i].spider == s)
      {
        pair_[i].ctl = c;
        return;
      }
    if (nPair_ < MAX_PAIRS)
      pair_[nPair_++] = Pair{s, c};
    else
    {
      for (size_t i = 1; i < MAX_PAIRS; i++)
        pair_[i - 1] = pair_[i];
      pair_[MAX_PAIRS - 1] = Pair{s, c};
    }
  }

  // A candidate's poll opens its listen window: after the controller's answer, or 300 ms after the RQ
  void offerProbe(const Frame &f)
  {
    if (f.opcode != 0x3EF0 && f.opcode != 0x3EF1)
      return;
    uint32_t s = 0, c = 0;
    if (f.msgType == MSG_RQ)
    {
      s = f.addr0;
      c = f.addr1;
    }
    else if (f.msgType == MSG_RP)
    {
      s = f.addr1;
      c = f.addr0;
    }
    const Cand *k = findCand(s);
    if (k == nullptr || c == 0 || k->rejected || k->probes >= MAX_PROBES)
      return;
    probe_ = Probe{s, c, f.msgType == MSG_RP};
  }

  Cand *findCand(uint32_t id)
  {
    for (size_t i = 0; i < nCand_; i++)
      if (cand_[i].id == id)
        return &cand_[i];
    return nullptr;
  }
  const Cand *findCand(uint32_t id) const { return const_cast<Discovery *>(this)->findCand(id); }

  State state_{State::Idle};
  int32_t target_{0};
  Cand cand_[MAX_CANDIDATES]{};
  size_t nCand_{0};
  Pair pair_[MAX_PAIRS]{};
  size_t nPair_{0};
  Probe probe_{};
  uint32_t spider_{0};
  uint32_t heater_{0};
};

} // namespace spider
