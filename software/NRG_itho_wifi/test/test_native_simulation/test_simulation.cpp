// Native unit tests: Simulated Device Runtime core
// Validates the I2C frames produced by SimulatedDevice against the exact
// expectations of the firmware parsers (sendI2cQuery / sendQueryStatusFormat /
// sendQueryStatus / sendQuery31DA / sendQuery31D9 / sendQuery2410 /
// setSetting2410 / sendQueryCounters).

#include <unity.h>

#include <cstdint>
#include <cstring>

#include "../../main/simulation/SimulatedDevice.cpp"

// ---------------------------------------------------------------------------
// firmware-equivalent validation helpers (from ithodevice/IthoInfoHelpers.cpp)
// ---------------------------------------------------------------------------

static uint8_t fw_checksum(const uint8_t *buf, size_t buflen)
{
    uint8_t sum = 0;
    while (buflen--)
        sum += *buf++;
    return -sum;
}

static bool fw_checkI2cReply(const uint8_t *buf, size_t buflen, uint16_t opcode)
{
    if (buflen < 4)
        return false;
    return ((buf[2] << 8 | buf[3]) & 0x3FFF) == (opcode & 0x3FFF);
}

// expected status label counts of the pinned profile firmware versions
// (sentinel-255 lengths of itho_CVE1Bstatus24_27 / itho_HRU350status1_4 /
// itho_DemandFlowstatus4_12 / itho_WPUstatus25 in main/ithodevice/devices/)
static const uint8_t expectedFieldCount[] = {12, 30, 49, 123};

// ---------------------------------------------------------------------------
// request frames exactly as the firmware sends them
// ---------------------------------------------------------------------------

static const uint8_t qDevicetype[] = {0x82, 0x80, 0x90, 0xE0, 0x04, 0x00, 0x8A};
static const uint8_t qStatusFormat[] = {0x82, 0x80, 0x24, 0x00, 0x04, 0x00, 0xD6};
static const uint8_t qStatus[] = {0x82, 0x80, 0x24, 0x01, 0x04, 0x00, 0xD5};
static const uint8_t q31DA[] = {0x82, 0x80, 0x31, 0xDA, 0x04, 0x00, 0xEF};
static const uint8_t q31D9[] = {0x82, 0x80, 0x31, 0xD9, 0x04, 0x00, 0xF0};
static const uint8_t qCounters[] = {0x82, 0x80, 0x42, 0x10, 0x04, 0x00, 0xA8};

// mirror of sendI2cQuery(): send frame, fetch + validate response
static size_t simQuery(SimulatedDevice &dev, const uint8_t *cmd, size_t cmdLen,
                       uint8_t *rx, uint32_t now_ms)
{
    if (!dev.handleFrame(cmd, cmdLen, now_ms))
        return 0;
    size_t len = dev.popResponse(rx);
    if (len == 0)
        return 0;
    const uint16_t opcode = (cmd[2] << 8) | cmd[3];
    if (rx[len - 1] != fw_checksum(rx, len - 1) || !fw_checkI2cReply(rx, len, opcode))
        return 0;
    return len;
}

static void buildPwmFrame(uint8_t *frame, uint8_t speed)
{
    const uint8_t tmpl[] = {0x00, 0x60, 0xC0, 0x20, 0x01, 0x02, speed, 0x00, 0x00};
    memcpy(frame, tmpl, sizeof(tmpl));
    frame[8] = fw_checksum(frame, 8);
}

// virtual remote i2c wrapper as built by sendRemoteCmd()
static size_t buildVremoteFrame(uint8_t *frame, const uint8_t *rfCmd, size_t rfLen)
{
    const uint8_t header[] = {0x82, 0x60, 0xC1, 0x01, 0x01, 0x09,
                              0x00, 0x00, 0x00, 0x01, 0x16, 0xAA, 0xBB, 0xCC, 0x00};
    size_t len = sizeof(header);
    memcpy(frame, header, len);
    memcpy(frame + len, rfCmd, rfLen);
    len += rfLen;
    frame[len++] = 0x00; // chk2 (not verified by the simulator)
    frame[len++] = 0x00; // counter
    frame[5] = len - 6 + 1;
    frame[len] = fw_checksum(frame, len);
    return len + 1;
}

static size_t build2410Set(uint8_t *frame, uint8_t index, int32_t value)
{
    const uint8_t tmpl[] = {0x82, 0x80, 0x24, 0x10, 0x06, 0x13,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
                            0x00, 0xFF};
    memcpy(frame, tmpl, sizeof(tmpl));
    frame[8] = (value >> 8) & 0xFF; // 2-byte setting, per simulator datatype 0x10
    frame[9] = value & 0xFF;
    frame[23] = index;
    frame[25] = fw_checksum(frame, 25);
    return sizeof(tmpl);
}

static size_t build2410Query(uint8_t *frame, uint8_t index)
{
    const uint8_t tmpl[] = {0x82, 0x80, 0x24, 0x10, 0x04, 0x13,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
                            0x00, 0xFF};
    memcpy(frame, tmpl, sizeof(tmpl));
    frame[23] = index;
    frame[25] = fw_checksum(frame, 25);
    return sizeof(tmpl);
}

// ---------------------------------------------------------------------------
// tests
// ---------------------------------------------------------------------------

void setUp(void) {}
void tearDown(void) {}

void test_profile_table_sanity(void)
{
    TEST_ASSERT_EQUAL_UINT8(4, simProfileCount);
    for (uint8_t i = 0; i < simProfileCount; i++)
    {
        const SimProfile &prof = simProfiles[i];
        TEST_ASSERT_EQUAL_UINT8(expectedFieldCount[i], prof.statusFieldCount);
        for (uint8_t f = 0; f < prof.fieldCount; f++)
        {
            TEST_ASSERT_LESS_THAN_UINT8(prof.statusFieldCount, prof.fields[f].index);
            // no duplicate field indexes
            for (uint8_t g = f + 1; g < prof.fieldCount; g++)
                TEST_ASSERT_NOT_EQUAL(prof.fields[f].index, prof.fields[g].index);
        }
    }
}

void test_devicetype_reply(void)
{
    for (uint8_t i = 0; i < simProfileCount; i++)
    {
        SimulatedDevice dev;
        dev.begin(i, 1234, SIM_SCENARIO_NORMAL, 1000);
        uint8_t rx[512]{};
        size_t len = simQuery(dev, qDevicetype, sizeof(qDevicetype), rx, 2000);
        TEST_ASSERT_GREATER_THAN(11, (int)len);
        // sendQueryDevicetype() reads DG/ID/hw/fw at receive offsets 8-11
        TEST_ASSERT_EQUAL_UINT8(simProfiles[i].dg, rx[8]);
        TEST_ASSERT_EQUAL_UINT8(simProfiles[i].id, rx[9]);
        TEST_ASSERT_EQUAL_UINT8(simProfiles[i].hw, rx[10]);
        TEST_ASSERT_EQUAL_UINT8(simProfiles[i].fw, rx[11]);
    }
}

void test_statusformat_and_status_consistency(void)
{
    for (uint8_t i = 0; i < simProfileCount; i++)
    {
        SimulatedDevice dev;
        dev.begin(i, 1234, SIM_SCENARIO_NORMAL, 1000);

        uint8_t fmt[512]{};
        size_t fmtLen = simQuery(dev, qStatusFormat, sizeof(qStatusFormat), fmt, 2000);
        TEST_ASSERT_GREATER_THAN(6, (int)fmtLen);
        // sendQueryStatusFormat(): field count at [5], datatypes from [6]
        TEST_ASSERT_EQUAL_UINT8(expectedFieldCount[i], fmt[5]);
        TEST_ASSERT_EQUAL((int)fmtLen, 6 + fmt[5] + 1);

        size_t expectedPayload = 0;
        for (uint8_t f = 0; f < fmt[5]; f++)
        {
            switch (fmt[6 + f] & 0x70)
            {
            case 0x10:
                expectedPayload += 2;
                break;
            case 0x20:
            case 0x70:
                expectedPayload += 4;
                break;
            default:
                expectedPayload += 1;
                break;
            }
        }

        uint8_t st[512]{};
        size_t stLen = simQuery(dev, qStatus, sizeof(qStatus), st, 3000);
        TEST_ASSERT_GREATER_THAN(6, (int)stLen);
        TEST_ASSERT_EQUAL_UINT8(expectedPayload, st[5]);
        TEST_ASSERT_EQUAL((int)stLen, 6 + st[5] + 1);
    }
}

void test_cve_status_realistic_values(void)
{
    SimulatedDevice dev;
    dev.begin(0, 1234, SIM_SCENARIO_NORMAL, 0);
    uint8_t st[512]{};
    TEST_ASSERT_GREATER_THAN(0, (int)simQuery(dev, qStatus, sizeof(qStatus), st, 60000));

    // CVE-Silent fw27 layout: lengths 1,2,2,2,1,2,4,2,2,1,2,2 from offset 6
    // RelativeHumidity at payload offset 19-20 (div 100), Temperature at 21-22
    const int base = 6;
    const int humRaw = (st[base + 19] << 8) | st[base + 20];
    const int tempRaw = (st[base + 21] << 8) | st[base + 22];
    TEST_ASSERT_TRUE(humRaw > 100 && humRaw < 9900);   // 1..99 %
    TEST_ASSERT_TRUE(tempRaw > 1500 && tempRaw < 2800); // 15..28 degC
    // highest CO2 at payload offset 16-17 must stay clear of the 0x8200 sensor-error marker
    const int co2 = (st[base + 16] << 8) | st[base + 17];
    TEST_ASSERT_TRUE(co2 >= 400 && co2 <= 5000);
}

void test_pwm_command_and_ramp(void)
{
    SimulatedDevice dev;
    dev.begin(0, 1234, SIM_SCENARIO_NORMAL, 0);

    uint8_t pwm[9];
    buildPwmFrame(pwm, 254); // full speed
    TEST_ASSERT_TRUE(dev.handleFrame(pwm, sizeof(pwm), 1000));
    TEST_ASSERT_EQUAL(0, (int)dev.popResponse(pwm)); // fire-and-forget: no reply
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 100.0f, dev.fanSetpointPct());

    // ramp: not yet at setpoint right away, converged after 20 simulated seconds
    TEST_ASSERT_TRUE(dev.fanActualPct() < 99.0f);
    uint8_t st[512]{};
    TEST_ASSERT_GREATER_THAN(0, (int)simQuery(dev, qStatus, sizeof(qStatus), st, 21000));
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 100.0f, dev.fanActualPct());
}

void test_vremote_commands(void)
{
    SimulatedDevice dev;
    dev.begin(0, 1234, SIM_SCENARIO_NORMAL, 0);

    const uint8_t rfHigh[] = {0x22, 0xF1, 0x03, 0x00, 0x04, 0x04};
    uint8_t frame[64];
    size_t len = buildVremoteFrame(frame, rfHigh, sizeof(rfHigh));
    TEST_ASSERT_TRUE(dev.handleFrame(frame, len, 1000));
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 90.0f, dev.fanSetpointPct());

    const uint8_t rfLow[] = {0x22, 0xF1, 0x03, 0x00, 0x02, 0x04};
    len = buildVremoteFrame(frame, rfLow, sizeof(rfLow));
    TEST_ASSERT_TRUE(dev.handleFrame(frame, len, 2000));
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 20.0f, dev.fanSetpointPct());
}

void test_vremote_timer_and_expiry(void)
{
    SimulatedDevice dev;
    dev.begin(0, 1234, SIM_SCENARIO_NORMAL, 0);

    const uint8_t rfLow[] = {0x22, 0xF1, 0x03, 0x00, 0x02, 0x04};
    uint8_t frame[64];
    size_t len = buildVremoteFrame(frame, rfLow, sizeof(rfLow));
    dev.handleFrame(frame, len, 1000);

    // timer command: 10 minutes at high (payload byte 2 = i2c offset 20)
    const uint8_t rfTimer[] = {0x22, 0xF3, 0x06, 0x00, 0x00, 10, 0x00, 0x00, 0x00};
    len = buildVremoteFrame(frame, rfTimer, sizeof(rfTimer));
    dev.handleFrame(frame, len, 2000);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 90.0f, dev.fanSetpointPct());

    // after 11 minutes the pre-timer setpoint is restored
    uint8_t st[512]{};
    TEST_ASSERT_GREATER_THAN(0, (int)simQuery(dev, qStatus, sizeof(qStatus), st, 2000 + 11 * 60000));
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 20.0f, dev.fanSetpointPct());
}

void test_2410_set_get_roundtrip(void)
{
    SimulatedDevice dev;
    dev.begin(0, 1234, SIM_SCENARIO_NORMAL, 0);

    uint8_t frame[32];
    uint8_t rx[512]{};

    size_t len = build2410Set(frame, 42, 123);
    TEST_ASSERT_TRUE(dev.handleFrame(frame, len, 1000));
    size_t ackLen = dev.popResponse(rx);
    TEST_ASSERT_GREATER_THAN(23, (int)ackLen);
    // setSetting2410() confirmation: trailing checksum plus request bytes 6-9
    // and 23 mirrored at the same receive offsets
    TEST_ASSERT_EQUAL_UINT8(fw_checksum(rx, ackLen - 1), rx[ackLen - 1]);
    TEST_ASSERT_EQUAL_UINT8(frame[6], rx[6]);
    TEST_ASSERT_EQUAL_UINT8(frame[7], rx[7]);
    TEST_ASSERT_EQUAL_UINT8(frame[8], rx[8]);
    TEST_ASSERT_EQUAL_UINT8(frame[9], rx[9]);
    TEST_ASSERT_EQUAL_UINT8(42, rx[23]);

    // query returns the stored value (big-endian at receive offsets 6-9,
    // datatype at 22, index at 23 per sendQuery2410)
    len = build2410Query(frame, 42);
    size_t qLen = simQuery(dev, frame, len, rx, 2000);
    TEST_ASSERT_GREATER_THAN(23, (int)qLen);
    const int32_t cur = (rx[6] << 24) | (rx[7] << 16) | (rx[8] << 8) | rx[9];
    TEST_ASSERT_EQUAL_INT32(123, cur);
    TEST_ASSERT_EQUAL_UINT8(0x10, rx[22]);
    TEST_ASSERT_EQUAL_UINT8(42, rx[23]);
}

void test_31da_31d9_per_profile(void)
{
    // CVE-Silent answers both
    SimulatedDevice cve;
    cve.begin(0, 1234, SIM_SCENARIO_NORMAL, 0);
    uint8_t rx[512]{};
    size_t len = simQuery(cve, q31DA, sizeof(q31DA), rx, 1000);
    TEST_ASSERT_EQUAL(6 + 29 + 1, (int)len);
    TEST_ASSERT_EQUAL_UINT8(29, rx[5]);
    len = simQuery(cve, q31D9, sizeof(q31D9), rx, 2000);
    TEST_ASSERT_EQUAL(6 + 16 + 1, (int)len);
    TEST_ASSERT_EQUAL_UINT8(0x00, rx[6]); // no fault

    // WPU answers neither (mirrors real heat pump behaviour)
    SimulatedDevice wpu;
    wpu.begin(3, 1234, SIM_SCENARIO_NORMAL, 0);
    TEST_ASSERT_EQUAL(0, (int)simQuery(wpu, q31DA, sizeof(q31DA), rx, 1000));
    TEST_ASSERT_EQUAL(0, (int)simQuery(wpu, q31D9, sizeof(q31D9), rx, 2000));
}

void test_counters_wpu_only(void)
{
    SimulatedDevice wpu;
    wpu.begin(3, 1234, SIM_SCENARIO_NORMAL, 0);
    uint8_t rx[512]{};
    size_t len = simQuery(wpu, qCounters, sizeof(qCounters), rx, 1000);
    TEST_ASSERT_GREATER_THAN(7, (int)len);
    // sendQueryCounters(): N values at [6], 26 max (ithoWPUCounterLabelLength)
    TEST_ASSERT_EQUAL_UINT8(26, rx[6]);
    TEST_ASSERT_EQUAL((int)len, 6 + 1 + 26 * 2 + 1);

    SimulatedDevice cve;
    cve.begin(0, 1234, SIM_SCENARIO_NORMAL, 0);
    TEST_ASSERT_EQUAL(0, (int)simQuery(cve, qCounters, sizeof(qCounters), rx, 1000));
}

void test_fault_scenario(void)
{
    SimulatedDevice dev;
    dev.begin(0, 1234, SIM_SCENARIO_NORMAL, 0);
    uint8_t rx[512]{};

    simQuery(dev, qStatus, sizeof(qStatus), rx, 1000);
    // CVE-Silent error field: payload offset 5-6 (after 1+2+2 bytes)
    TEST_ASSERT_EQUAL(0, (rx[6 + 5] << 8) | rx[6 + 6]);

    dev.setScenario(SIM_SCENARIO_FAULT);
    simQuery(dev, qStatus, sizeof(qStatus), rx, 2000);
    TEST_ASSERT_EQUAL(1, (rx[6 + 5] << 8) | rx[6 + 6]);
    simQuery(dev, q31D9, sizeof(q31D9), rx, 3000);
    TEST_ASSERT_EQUAL_UINT8(0x80, rx[6]); // 31D9 fault bit

    dev.setScenario(SIM_SCENARIO_NORMAL);
    simQuery(dev, qStatus, sizeof(qStatus), rx, 4000);
    TEST_ASSERT_EQUAL(0, (rx[6 + 5] << 8) | rx[6 + 6]);
}

void test_determinism_same_seed(void)
{
    SimulatedDevice a, b;
    a.begin(1, 98765, SIM_SCENARIO_NORMAL, 500);
    b.begin(1, 98765, SIM_SCENARIO_NORMAL, 500);

    uint8_t rxa[512]{}, rxb[512]{};
    const uint32_t times[] = {1000, 31000, 61000, 91000, 901000};
    for (uint32_t t : times)
    {
        size_t la = simQuery(a, qStatus, sizeof(qStatus), rxa, t);
        size_t lb = simQuery(b, qStatus, sizeof(qStatus), rxb, t);
        TEST_ASSERT_EQUAL((int)la, (int)lb);
        TEST_ASSERT_GREATER_THAN(0, (int)la);
        TEST_ASSERT_EQUAL_MEMORY(rxa, rxb, la);
    }
}

void test_determinism_different_seed(void)
{
    SimulatedDevice a, b;
    a.begin(1, 1, SIM_SCENARIO_NORMAL, 500);
    b.begin(1, 2, SIM_SCENARIO_NORMAL, 500);

    uint8_t rxa[512]{}, rxb[512]{};
    size_t la = simQuery(a, qStatus, sizeof(qStatus), rxa, 31000);
    size_t lb = simQuery(b, qStatus, sizeof(qStatus), rxb, 31000);
    TEST_ASSERT_EQUAL((int)la, (int)lb);
    TEST_ASSERT_TRUE(memcmp(rxa, rxb, la) != 0);
}

void test_scenario_name_lookup(void)
{
    TEST_ASSERT_EQUAL(SIM_SCENARIO_NORMAL, SimulatedDevice::scenarioFromName("normal"));
    TEST_ASSERT_EQUAL(SIM_SCENARIO_CO2_RISE, SimulatedDevice::scenarioFromName("co2_rise"));
    TEST_ASSERT_EQUAL(-1, SimulatedDevice::scenarioFromName("bogus"));
    TEST_ASSERT_EQUAL(0, SimulatedDevice::profileIndexFromName("CVE-Silent"));
    TEST_ASSERT_EQUAL(3, SimulatedDevice::profileIndexFromName("Heatpump"));
    TEST_ASSERT_EQUAL(-1, SimulatedDevice::profileIndexFromName("bogus"));
}

void test_inactive_passthrough(void)
{
    SimulatedDevice dev;
    TEST_ASSERT_FALSE(dev.active());
    dev.begin(0, 1, SIM_SCENARIO_NORMAL, 0);
    TEST_ASSERT_TRUE(dev.active());
    dev.end();
    TEST_ASSERT_FALSE(dev.active());
    uint8_t rx[512]{};
    TEST_ASSERT_EQUAL(0, (int)dev.popResponse(rx));
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_profile_table_sanity);
    RUN_TEST(test_devicetype_reply);
    RUN_TEST(test_statusformat_and_status_consistency);
    RUN_TEST(test_cve_status_realistic_values);
    RUN_TEST(test_pwm_command_and_ramp);
    RUN_TEST(test_vremote_commands);
    RUN_TEST(test_vremote_timer_and_expiry);
    RUN_TEST(test_2410_set_get_roundtrip);
    RUN_TEST(test_31da_31d9_per_profile);
    RUN_TEST(test_counters_wpu_only);
    RUN_TEST(test_fault_scenario);
    RUN_TEST(test_determinism_same_seed);
    RUN_TEST(test_determinism_different_seed);
    RUN_TEST(test_scenario_name_lookup);
    RUN_TEST(test_inactive_passthrough);
    UNITY_END();
    return 0;
}
