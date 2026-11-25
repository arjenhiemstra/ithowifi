// Native unit tests for Spider discovery (main/cc1101/spider_discovery.h).
// Frames are real captures from 2026-10-15/16: Spider 21:054266 (54:D3:FA) with heater 10:080343 (29:39:D7),
// relay-bound Spider 21:047919 (54:BB:2F) with BDR91 13:141453 (36:28:8D).

#include <unity.h>
#include <cstdint>

#include "../../main/cc1101/spider_discovery.h"

using spider::Discovery;
using spider::Frame;
using spider::Probe;

void setUp() {}
void tearDown() {}

static const uint32_t SPIDER = 0x54D3FA;
static const uint32_t HEATER = 0x2939D7;
static const uint32_t SPIDER2 = 0x54BB2F;
static const uint32_t RELAY = 0x36288D;
static const int32_t TARGET = 2750; // 27.5 C

static Frame zoneSetpoint(uint32_t src, const uint8_t *p) { return Frame{spider::MSG_I, src, 0, src, 0x22C9, p, 6}; }
static Frame demandFrame(uint32_t src, const uint8_t *p) { return Frame{spider::MSG_I, src, 0, src, 0x0008, p, 2}; }

// I 22C9 payloads: 27.50 (0ABE) and 21.00 (0834)
static const uint8_t sp275[6] = {0x00, 0x0A, 0xBE, 0x0B, 0x86, 0x01};
static const uint8_t sp21[6] = {0x00, 0x08, 0x34, 0x08, 0xFC, 0x01};
static const uint8_t rq00[1] = {0x00};
static const uint8_t dem0[2] = {0x00, 0x00};
static const uint8_t demC8[2] = {0x00, 0xC8};
static const uint8_t echo[2] = {0x00, 0xC6};
static const uint8_t rp3ef1[7] = {0x00, 0x08, 0x79, 0x08, 0x79, 0xC8, 0xFF};

// RP 01FF from a Spider: byte2 room temp, byte3 setpoint (0.5 C units)
static void status01ff(uint8_t *p, uint8_t setpointHalf)
{
  for (int i = 0; i < 26; i++)
    p[i] = 0;
  p[2] = 42;
  p[3] = setpointHalf;
}

void test_pick_target_range()
{
  TEST_ASSERT_EQUAL(2150, Discovery::pickTarget(0));
  TEST_ASSERT_EQUAL(2400, Discovery::pickTarget(5));
  TEST_ASSERT_EQUAL(2150, Discovery::pickTarget(6));
  for (uint32_t r = 0; r < 100; r++)
  {
    const int32_t t = Discovery::pickTarget(r * 7919);
    TEST_ASSERT_TRUE(t >= 2150 && t <= 2400 && t % 50 == 0);
  }
}

void test_broadcasting_spider_found_from_22c9()
{
  Discovery d;
  // pair learned before discovery starts: RQ --- 21:054266 10:080343 --:------ 3EF0 001 00
  d.feed(Frame{spider::MSG_RQ, SPIDER, HEATER, 0, 0x3EF0, rq00, 1});
  d.start(TARGET);
  TEST_ASSERT_FALSE(d.feed(zoneSetpoint(SPIDER, sp21)));
  TEST_ASSERT_TRUE(d.feed(zoneSetpoint(SPIDER, sp275)));
  TEST_ASSERT_TRUE(d.state() == Discovery::State::Done);
  TEST_ASSERT_EQUAL_HEX32(SPIDER, d.spider());
  TEST_ASSERT_EQUAL_HEX32(HEATER, d.heater());
}

void test_heater_after_spider()
{
  Discovery d;
  d.start(TARGET);
  d.feed(zoneSetpoint(SPIDER, sp275));
  TEST_ASSERT_TRUE(d.state() == Discovery::State::WaitHeater);
  // I --- 10:080343 --:------ 21:054266 3150 002 00C6
  TEST_ASSERT_TRUE(d.feed(Frame{spider::MSG_I, HEATER, 0, SPIDER, 0x3150, echo, 2}));
  TEST_ASSERT_TRUE(d.state() == Discovery::State::Done);
  TEST_ASSERT_EQUAL_HEX32(HEATER, d.heater());
}

void test_relay_spider_found_by_probe()
{
  Discovery d;
  d.start(TARGET);
  Probe p{};
  // relay poll before the user turned the Spider up: not a candidate, no probe
  d.feed(Frame{spider::MSG_RQ, SPIDER2, RELAY, 0, 0x3EF1, rq00, 1});
  TEST_ASSERT_FALSE(d.takeProbe(p));
  // I --- 21:047919 --:------ 21:047919 0008 002 00C8
  d.feed(demandFrame(SPIDER2, demC8));
  TEST_ASSERT_EQUAL(1, d.candidates());
  // RQ --- 21:047919 13:141453 3EF1: probe after a delay
  d.feed(Frame{spider::MSG_RQ, SPIDER2, RELAY, 0, 0x3EF1, rq00, 1});
  TEST_ASSERT_TRUE(d.takeProbe(p));
  TEST_ASSERT_EQUAL_HEX32(SPIDER2, p.spider);
  TEST_ASSERT_EQUAL_HEX32(RELAY, p.ctl);
  TEST_ASSERT_FALSE(p.now);
  // RP --- 13:141453 21:047919 3EF1 007: probe now
  d.feed(Frame{spider::MSG_RP, RELAY, SPIDER2, 0, 0x3EF1, rp3ef1, 7});
  TEST_ASSERT_TRUE(d.takeProbe(p));
  TEST_ASSERT_TRUE(p.now);
  TEST_ASSERT_FALSE(d.takeProbe(p));
  d.probeSent(SPIDER2);
  // RP --- 21:047919 13:141453 01FF 026 ...: setpoint 27.5 = 55 half degrees
  uint8_t st[26];
  status01ff(st, 55);
  TEST_ASSERT_TRUE(d.feed(Frame{spider::MSG_RP, SPIDER2, RELAY, 0, 0x01FF, st, 26}));
  TEST_ASSERT_TRUE(d.state() == Discovery::State::Done);
  TEST_ASSERT_EQUAL_HEX32(SPIDER2, d.spider());
  TEST_ASSERT_EQUAL_HEX32(RELAY, d.heater());
}

void test_wrong_answer_rejects_candidate()
{
  Discovery d;
  d.start(TARGET);
  Probe p{};
  d.feed(demandFrame(SPIDER2, demC8));
  uint8_t st[26];
  status01ff(st, 42); // 21.0: a neighbour that happens to be heating
  TEST_ASSERT_FALSE(d.feed(Frame{spider::MSG_RP, SPIDER2, RELAY, 0, 0x01FF, st, 26}));
  d.feed(Frame{spider::MSG_RP, RELAY, SPIDER2, 0, 0x3EF1, rp3ef1, 7});
  TEST_ASSERT_FALSE(d.takeProbe(p));
  TEST_ASSERT_TRUE(d.state() == Discovery::State::WaitSetpoint);
}

void test_probe_limit_and_no_demand()
{
  Discovery d;
  d.start(TARGET);
  Probe p{};
  d.feed(demandFrame(SPIDER, dem0)); // demand 0: not a candidate
  d.feed(Frame{spider::MSG_RQ, SPIDER, HEATER, 0, 0x3EF0, rq00, 1});
  TEST_ASSERT_FALSE(d.takeProbe(p));
  d.feed(demandFrame(SPIDER2, demC8));
  for (int i = 0; i < Discovery::MAX_PROBES; i++)
  {
    d.feed(Frame{spider::MSG_RP, RELAY, SPIDER2, 0, 0x3EF1, rp3ef1, 7});
    TEST_ASSERT_TRUE(d.takeProbe(p));
    d.probeSent(SPIDER2);
  }
  d.feed(Frame{spider::MSG_RP, RELAY, SPIDER2, 0, 0x3EF1, rp3ef1, 7});
  TEST_ASSERT_FALSE(d.takeProbe(p));
}

void test_pairs_ignore_other_traffic()
{
  Discovery d;
  // the Spider polling its fan with RQ 31DA is no controller
  d.feed(Frame{spider::MSG_RQ, SPIDER, 0xC3E8B3, 0, 0x31DA, rq00, 1});
  TEST_ASSERT_EQUAL_HEX32(0, d.controllerOf(SPIDER));
  // a controller asking the Spider (1-byte RQ 01FF) must not be learned the wrong way round
  d.feed(Frame{spider::MSG_RQ, HEATER, SPIDER, 0, 0x01FF, rq00, 1});
  TEST_ASSERT_EQUAL_HEX32(0, d.controllerOf(HEATER));
  d.feed(Frame{spider::MSG_RP, RELAY, SPIDER2, 0, 0x3EF1, rp3ef1, 7});
  TEST_ASSERT_EQUAL_HEX32(RELAY, d.controllerOf(SPIDER2));
}

// AutoTemp zone controller, frames from a user log (Spider 56:26:8F, AutoTemp 0B:A2:58, zone 06)
static const uint32_t SPIDER_AT = 0x56268F;
static const uint32_t AUTOTEMP = 0x0BA258;
static const uint8_t at30c9[3] = {0x00, 0x08, 0x4E};
static const uint8_t atRq0008[1] = {0x06};
static const uint8_t atSp20[6] = {0x06, 0x07, 0xD0, 0x08, 0x34, 0x02};
static const uint8_t atSp22[6] = {0x06, 0x08, 0x98, 0x08, 0xFC, 0x01};

void test_autotemp_spider_found_from_w22c9()
{
  Discovery d;
  // I --- 56:26:8F 0B:A2:58 --:------ 30C9 003 00084E
  d.feed(Frame{spider::MSG_I, SPIDER_AT, AUTOTEMP, 0, 0x30C9, at30c9, 3});
  TEST_ASSERT_EQUAL_HEX32(AUTOTEMP, d.controllerOf(SPIDER_AT));
  d.start(2200);
  // W --- 56:26:8F 0B:A2:58 --:------ 22C9 006 0607D0083402: another setpoint
  TEST_ASSERT_FALSE(d.feed(Frame{spider::MSG_W, SPIDER_AT, AUTOTEMP, 0, 0x22C9, atSp20, 6}));
  TEST_ASSERT_TRUE(d.feed(Frame{spider::MSG_W, SPIDER_AT, AUTOTEMP, 0, 0x22C9, atSp22, 6}));
  TEST_ASSERT_TRUE(d.state() == Discovery::State::Done);
  TEST_ASSERT_EQUAL_HEX32(SPIDER_AT, d.spider());
  TEST_ASSERT_EQUAL_HEX32(AUTOTEMP, d.heater());
  TEST_ASSERT_TRUE(d.zoneController());
  TEST_ASSERT_EQUAL_HEX8(0x06, d.zoneId());
}

void test_autotemp_echo_is_not_the_spider()
{
  Discovery d;
  // RQ --- 56:26:8F 0B:A2:58 --:------ 0008 001 06
  d.feed(Frame{spider::MSG_RQ, SPIDER_AT, AUTOTEMP, 0, 0x0008, atRq0008, 1});
  d.start(2200);
  // I --- --:------ --:------ 0B:A2:58 22C9 006 06089808FC01: the AutoTemp's echo
  TEST_ASSERT_FALSE(d.feed(Frame{spider::MSG_I, 0, 0, AUTOTEMP, 0x22C9, atSp22, 6}));
  TEST_ASSERT_TRUE(d.state() == Discovery::State::WaitSetpoint);
}

void test_polled_controller_is_not_zone()
{
  Discovery d;
  d.feed(Frame{spider::MSG_RQ, SPIDER, HEATER, 0, 0x3EF0, rq00, 1});
  // a 30C9 to the same heater must not turn the polled pair into a zone pair
  d.feed(Frame{spider::MSG_I, SPIDER, HEATER, 0, 0x30C9, at30c9, 3});
  d.start(TARGET);
  TEST_ASSERT_TRUE(d.feed(zoneSetpoint(SPIDER, sp275)));
  TEST_ASSERT_FALSE(d.zoneController());
  TEST_ASSERT_EQUAL_HEX8(0xFF, d.zoneId());
}

void test_idle_and_restart()
{
  Discovery d;
  TEST_ASSERT_FALSE(d.feed(zoneSetpoint(SPIDER, sp275))); // idle: nothing happens
  TEST_ASSERT_TRUE(d.state() == Discovery::State::Idle);
  d.start(TARGET);
  d.feed(demandFrame(SPIDER2, demC8));
  d.cancel();
  TEST_ASSERT_TRUE(d.state() == Discovery::State::Idle);
  d.start(2300);
  TEST_ASSERT_EQUAL(0, d.candidates());
  TEST_ASSERT_EQUAL(2300, d.target());
}

int main(int, char **)
{
  UNITY_BEGIN();
  RUN_TEST(test_pick_target_range);
  RUN_TEST(test_broadcasting_spider_found_from_22c9);
  RUN_TEST(test_heater_after_spider);
  RUN_TEST(test_relay_spider_found_by_probe);
  RUN_TEST(test_wrong_answer_rejects_candidate);
  RUN_TEST(test_probe_limit_and_no_demand);
  RUN_TEST(test_pairs_ignore_other_traffic);
  RUN_TEST(test_autotemp_spider_found_from_w22c9);
  RUN_TEST(test_autotemp_echo_is_not_the_spider);
  RUN_TEST(test_polled_controller_is_not_zone);
  RUN_TEST(test_idle_and_restart);
  return UNITY_END();
}
