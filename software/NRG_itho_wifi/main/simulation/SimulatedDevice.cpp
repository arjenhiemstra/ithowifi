#include "SimulatedDevice.h"

#include <cmath>
#include <cstring>

SimulatedDevice simulatedDevice;

namespace
{
    constexpr float SIM_PI = 3.14159265f;
    constexpr float RAMP_PCT_PER_S = 8.0f;     // fan ramp speed towards setpoint
    constexpr uint32_t NOISE_SLOT_MS = 30000;  // noise changes every 30s of sim time

    const char *const scenarioNames[SIM_SCENARIO_COUNT] = {
        "normal", "boost", "fault", "humidity_spike", "co2_rise"};

    uint8_t simChecksum(const uint8_t *buf, size_t buflen)
    {
        uint8_t sum = 0;
        while (buflen--)
            sum += *buf++;
        return -sum;
    }

    // mirrors getLengthFromDatatype (ithodevice/IthoInfoHelpers.cpp)
    uint8_t fieldLength(uint8_t datatype)
    {
        switch (datatype & 0x70)
        {
        case 0x10:
            return 2;
        case 0x20:
        case 0x70:
            return 4;
        default:
            return 1;
        }
    }

    // mirrors getDividerFromDatatype (ithodevice/IthoInfoHelpers.cpp)
    uint32_t fieldDivider(uint8_t datatype)
    {
        static const uint32_t divider[] = {1, 10, 100, 1000, 10000, 100000,
                                           1000000, 10000000, 100000000,
                                           1, 1, 1, 1, 1, 256, 2};
        return divider[datatype & 0x0F];
    }

    float clampf(float v, float lo, float hi)
    {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    int32_t rpmFromPct(float pct)
    {
        return 350 + static_cast<int32_t>(pct * 18.0f);
    }
}

void SimulatedDevice::begin(uint8_t profileIdx, uint32_t seed, uint8_t scenario, uint32_t now_ms)
{
    if (profileIdx >= simProfileCount)
        profileIdx = 0;
    profileIdx_ = profileIdx;
    profile_ = &simProfiles[profileIdx];
    seed_ = seed;
    scenario_ = scenario < SIM_SCENARIO_COUNT ? scenario : SIM_SCENARIO_NORMAL;
    t0_ = now_ms;
    lastEvolve_ = now_ms;
    setpointPct_ = 40.0f;
    actualPct_ = 40.0f;
    autoMode_ = true;
    timerEndMs_ = 0;
    preTimerSetpoint_ = 40.0f;
    std::memset(settingValues_, 0, sizeof(settingValues_));
    std::memset(settingSet_, 0, sizeof(settingSet_));
    respLen_ = 0;
    active_ = true;
}

void SimulatedDevice::end()
{
    active_ = false;
    profile_ = nullptr;
    respLen_ = 0;
}

void SimulatedDevice::setScenario(uint8_t scenario)
{
    if (scenario < SIM_SCENARIO_COUNT)
        scenario_ = scenario;
}

const char *SimulatedDevice::profileName() const
{
    return profile_ ? profile_->name : simProfiles[profileIdx_].name;
}

const char *SimulatedDevice::scenarioName() const
{
    return scenarioNames[scenario_];
}

const char *SimulatedDevice::profileNameAt(uint8_t idx)
{
    return idx < simProfileCount ? simProfiles[idx].name : nullptr;
}

const char *SimulatedDevice::scenarioNameAt(uint8_t idx)
{
    return idx < SIM_SCENARIO_COUNT ? scenarioNames[idx] : nullptr;
}

int SimulatedDevice::profileIndexFromName(const char *name)
{
    if (name == nullptr)
        return -1;
    for (uint8_t i = 0; i < simProfileCount; i++)
    {
        if (std::strcmp(simProfiles[i].name, name) == 0)
            return i;
    }
    return -1;
}

int SimulatedDevice::scenarioFromName(const char *name)
{
    if (name == nullptr)
        return -1;
    for (uint8_t i = 0; i < SIM_SCENARIO_COUNT; i++)
    {
        if (std::strcmp(scenarioNames[i], name) == 0)
            return i;
    }
    return -1;
}

float SimulatedDevice::noise(uint32_t slot, uint32_t salt) const
{
    // pure function of (seed, slot, salt) so replays with the same seed and
    // query timeline produce identical telemetry
    uint32_t h = seed_ ^ (slot * 2654435761u) ^ (salt * 40503u);
    h ^= h >> 16;
    h *= 0x7feb352du;
    h ^= h >> 15;
    h *= 0x846ca68bu;
    h ^= h >> 16;
    return (static_cast<int32_t>(h % 2001) - 1000) / 1000.0f;
}

void SimulatedDevice::evolve(uint32_t now_ms)
{
    if (!active_)
        return;

    if (timerEndMs_ != 0 && static_cast<int32_t>(now_ms - timerEndMs_) >= 0)
    {
        timerEndMs_ = 0;
        setpointPct_ = preTimerSetpoint_;
    }

    const float t = (now_ms - t0_) / 1000.0f;
    const uint32_t slot = (now_ms - t0_) / NOISE_SLOT_MS;

    float tempBase = 21.0f, tempAmp = 1.5f;
    float humBase = 45.0f, humAmp = 8.0f;
    float co2Base = 650.0f, co2Amp = 150.0f;
    switch (scenario_)
    {
    case SIM_SCENARIO_HUMIDITY_SPIKE:
        humBase = 85.0f;
        humAmp = 5.0f;
        break;
    case SIM_SCENARIO_CO2_RISE:
        co2Base = 1400.0f;
        co2Amp = 200.0f;
        break;
    default:
        break;
    }

    temperature_ = tempBase + tempAmp * sinf(2.0f * SIM_PI * t / 3600.0f) + noise(slot, 1) * 0.2f;
    humidity_ = clampf(humBase + humAmp * sinf(2.0f * SIM_PI * t / 2700.0f) + noise(slot, 2), 1.0f, 99.0f);
    co2_ = clampf(co2Base + co2Amp * sinf(2.0f * SIM_PI * t / 1800.0f) + noise(slot, 3) * 20.0f, 400.0f, 5000.0f);

    if (autoMode_ && timerEndMs_ == 0)
        setpointPct_ = co2_ > 1000.0f ? 80.0f : (humidity_ > 70.0f ? 70.0f : 35.0f);
    if (scenario_ == SIM_SCENARIO_BOOST && setpointPct_ < 90.0f)
        setpointPct_ = 90.0f;
    setpointPct_ = clampf(setpointPct_, 0.0f, 100.0f);

    float dt = (now_ms - lastEvolve_) / 1000.0f;
    dt = clampf(dt, 0.0f, 3600.0f);
    const float diff = setpointPct_ - actualPct_;
    const float step = RAMP_PCT_PER_S * dt;
    actualPct_ += clampf(diff, -step, step);
    lastEvolve_ = now_ms;
}

void SimulatedDevice::setFanSetpoint(float pct, uint32_t now_ms)
{
    setpointPct_ = clampf(pct, 0.0f, 100.0f);
    timerEndMs_ = 0;
    evolve(now_ms);
}

const SimField *SimulatedDevice::findField(uint8_t index) const
{
    for (uint8_t i = 0; i < profile_->fieldCount; i++)
    {
        if (profile_->fields[i].index == index)
            return &profile_->fields[i];
    }
    return nullptr;
}

int32_t SimulatedDevice::fieldValue(const SimField *f, uint8_t index, uint8_t datatype) const
{
    if (f == nullptr)
        return 0;

    const uint32_t divider = fieldDivider(datatype);
    const float t = (lastEvolve_ - t0_) / 1000.0f;
    const uint32_t slot = (lastEvolve_ - t0_) / NOISE_SLOT_MS;

    switch (f->role)
    {
    case SIM_ROLE_FAN_SETPOINT_PCT:
        return static_cast<int32_t>(lroundf(setpointPct_));
    case SIM_ROLE_FAN_SETPOINT_RPM:
        return rpmFromPct(setpointPct_);
    case SIM_ROLE_FAN_ACTUAL_PCT:
        return static_cast<int32_t>(lroundf(actualPct_));
    case SIM_ROLE_FAN_ACTUAL_RPM:
        return rpmFromPct(actualPct_);
    case SIM_ROLE_TEMP:
        return static_cast<int32_t>(lroundf(temperature_ * divider));
    case SIM_ROLE_HUMIDITY:
        return static_cast<int32_t>(lroundf(humidity_ * divider));
    case SIM_ROLE_CO2:
        return static_cast<int32_t>(lroundf(co2_));
    case SIM_ROLE_ERROR:
        return scenario_ == SIM_SCENARIO_FAULT ? 1 : 0;
    case SIM_ROLE_DRIFT:
    {
        float v = f->baseline;
        if (f->period_s > 0)
            v += f->amplitude * sinf(2.0f * SIM_PI * t / f->period_s);
        v += noise(slot, 100u + index) * f->amplitude * 0.05f;
        return static_cast<int32_t>(lroundf(v));
    }
    case SIM_ROLE_STATIC:
    default:
        return f->baseline;
    }
}

uint8_t *SimulatedDevice::beginResponse(uint8_t opHi, uint8_t opLo, uint8_t payloadLen)
{
    resp_[0] = 0x80; // i2cSlaveReceive() prefix: I2C_SLAVE_ADDRESS << 1
    resp_[1] = 0x82;
    resp_[2] = opHi | 0x80;
    resp_[3] = opLo;
    resp_[4] = 0x01;
    resp_[5] = payloadLen;
    return &resp_[6];
}

void SimulatedDevice::endResponse(uint8_t payloadLen)
{
    const size_t len = 6 + payloadLen;
    resp_[len] = simChecksum(resp_, len);
    respLen_ = len + 1;
}

size_t SimulatedDevice::popResponse(uint8_t *out)
{
    if (respLen_ == 0)
        return 0;
    const size_t len = respLen_;
    std::memcpy(out, resp_, len);
    respLen_ = 0;
    return len;
}

void SimulatedDevice::renderDeviceType()
{
    // template: example reply in ithodevice/IthoStatus.cpp sendQueryDevicetype()
    uint8_t *p = beginResponse(0x90, 0xE0, 37);
    std::memset(p, 0, 37);
    p[0] = 0x00;
    p[1] = 0x01;
    p[2] = profile_->dg;
    p[3] = profile_->id;
    p[4] = profile_->hw;
    p[5] = profile_->fw;
    p[6] = 0x01;
    p[7] = 0xFE;
    std::memset(&p[8], 0xFF, 6);
    p[14] = 0x04;
    p[15] = 0x0B;
    p[16] = 0x07;
    p[17] = 0xE5;
    // device name string area (informational only, not parsed)
    const size_t nameLen = std::strlen(profile_->name);
    std::memcpy(&p[18], profile_->name, nameLen > 18 ? 18 : nameLen);
    endResponse(37);
}

void SimulatedDevice::renderStatusFormat()
{
    const uint8_t count = profile_->statusFieldCount;
    uint8_t *p = beginResponse(0x24, 0x00, count);
    for (uint8_t i = 0; i < count; i++)
    {
        const SimField *f = findField(i);
        p[i] = f ? f->datatype : profile_->defaultDatatype;
    }
    endResponse(count);
}

void SimulatedDevice::renderStatus()
{
    uint8_t payloadLen = 0;
    for (uint8_t i = 0; i < profile_->statusFieldCount; i++)
    {
        const SimField *f = findField(i);
        payloadLen += fieldLength(f ? f->datatype : profile_->defaultDatatype);
    }

    uint8_t *p = beginResponse(0x24, 0x01, payloadLen);
    size_t pos = 0;
    for (uint8_t i = 0; i < profile_->statusFieldCount; i++)
    {
        const SimField *f = findField(i);
        const uint8_t datatype = f ? f->datatype : profile_->defaultDatatype;
        const uint8_t len = fieldLength(datatype);
        const int32_t val = fieldValue(f, i, datatype);
        for (uint8_t b = 0; b < len; b++)
            p[pos + b] = (val >> ((len - 1 - b) * 8)) & 0xFF;
        pos += len;
    }
    endResponse(payloadLen);
}

void SimulatedDevice::render31DA()
{
    // field offsets per sendQuery31DA() parsing in ithodevice/IthoStatus.cpp
    uint8_t *p = beginResponse(0x31, 0xDA, 29);
    std::memset(p, 0, 29);

    const int32_t co2 = static_cast<int32_t>(lroundf(co2_));
    const int32_t indoorT = static_cast<int32_t>(lroundf(temperature_ * 100.0f));

    p[0] = 84;   // air quality (%)
    p[1] = 0x00; // air quality basis
    p[2] = (co2 >> 8) & 0xFF;
    p[3] = co2 & 0xFF;
    p[4] = static_cast<uint8_t>(lroundf(humidity_)); // indoor humidity (%)
    p[5] = 0xEF;                                     // outdoor humidity: not available
    if (profile_->da_heatExchanger)
    {
        const int32_t supplyT = static_cast<int32_t>(lroundf((temperature_ - 3.0f) * 100.0f));
        const int32_t exhaustT = indoorT;
        const int32_t outdoorT = static_cast<int32_t>(lroundf((temperature_ - 9.0f) * 100.0f));
        p[6] = (exhaustT >> 8) & 0xFF;
        p[7] = exhaustT & 0xFF;
        p[8] = (supplyT >> 8) & 0xFF;
        p[9] = supplyT & 0xFF;
        p[10] = (indoorT >> 8) & 0xFF;
        p[11] = indoorT & 0xFF;
        p[12] = (outdoorT >> 8) & 0xFF;
        p[13] = outdoorT & 0xFF;
        p[16] = 0x00; // bypass closed
    }
    else
    {
        std::memset(&p[6], 0xFF, 8); // temps not available
        p[6] = 0x7F;
        p[8] = 0x7F;
        p[10] = (indoorT >> 8) & 0xFF;
        p[11] = indoorT & 0xFF;
        p[12] = 0x7F;
        p[13] = 0xFF;
        p[16] = 0xEF; // bypass not available
    }
    p[14] = 0xF8; // speed cap
    p[15] = 0x08;

    // fan info (keys per fanInfo map in devices/error_info_labels.h)
    uint8_t info;
    if (timerEndMs_ != 0)
        info = 0x0B; // timer 1
    else if (autoMode_)
        info = 0x18; // auto
    else if (setpointPct_ <= 12.0f)
        info = 0x15; // away
    else if (setpointPct_ < 35.0f)
        info = 0x01; // low
    else if (setpointPct_ < 70.0f)
        info = 0x02; // medium
    else
        info = 0x03; // high
    p[17] = info;

    p[18] = 0xEF;                                              // boiler demand: not available
    p[19] = static_cast<uint8_t>(lroundf(actualPct_ * 2.0f));  // actual power (%), scale 2
    p[20] = 0x00;                                              // airfilter counter
    p[21] = 90;
    p[22] = 0xEF; // frost protection: not available
    p[23] = 0xEF; // cooling demand: not available
    p[24] = 0x7F; // boiler temp: not available
    p[25] = 0xFF;
    p[26] = 0x7F; // preheater power: not available
    p[27] = 0xFF;
    p[28] = 0x00;
    endResponse(29);
}

void SimulatedDevice::render31D9()
{
    uint8_t *p = beginResponse(0x31, 0xD9, 16);
    std::memset(p, 0x20, 16);
    p[0] = scenario_ == SIM_SCENARIO_FAULT ? 0x80 : 0x00;
    p[1] = static_cast<uint8_t>(lroundf(actualPct_ * 2.0f)); // speed, scale 2
    p[2] = 0x0A;
    p[15] = 0x00;
    endResponse(16);
}

void SimulatedDevice::renderCounters()
{
    const uint8_t n = profile_->counterCount;
    const uint8_t payloadLen = 1 + 2 * n;
    uint8_t *p = beginResponse(0x42, 0x10, payloadLen);
    p[0] = n;
    for (uint8_t i = 0; i < n; i++)
    {
        const uint16_t val = 1000 + i * 37;
        p[1 + 2 * i] = (val >> 8) & 0xFF;
        p[2 + 2 * i] = val & 0xFF;
    }
    endResponse(payloadLen);
}

void SimulatedDevice::renderQuery2410(uint8_t index)
{
    // layout per sendQuery2410() parsing: current/min/max big-endian at receive
    // offsets 6-9/10-13/14-17, datatype at 22, index at 23
    const int32_t cur = settingValues_[index];
    uint8_t *p = beginResponse(0x24, 0x10, 20);
    std::memset(p, 0, 20);
    p[0] = (cur >> 24) & 0xFF;
    p[1] = (cur >> 16) & 0xFF;
    p[2] = (cur >> 8) & 0xFF;
    p[3] = cur & 0xFF;
    // min stays 0
    const int32_t max = 1000;
    p[8] = (max >> 24) & 0xFF;
    p[9] = (max >> 16) & 0xFF;
    p[10] = (max >> 8) & 0xFF;
    p[11] = max & 0xFF;
    p[16] = 0x10; // datatype: 2-byte unsigned int, divider 1
    p[17] = index;
    endResponse(20);
}

void SimulatedDevice::renderSetAck2410(const uint8_t *req, size_t len)
{
    // setSetting2410() confirms the write by comparing request bytes 6-9 and 23
    // against the same offsets of the receive buffer, so echo request[6..24]
    if (len < 26)
        return;
    uint8_t *p = beginResponse(0x24, 0x10, 19);
    std::memcpy(p, &req[6], 19);
    endResponse(19);
}

void SimulatedDevice::handleRemoteCmd(const uint8_t *buf, size_t len, uint32_t now_ms)
{
    // virtual remote i2c wrapper: 15-byte header, RF opcode at [15..16],
    // payload from [18] (see sendRemoteCmd() in ithodevice/IthoVirtualRemoteCmd.cpp)
    if (len < 21)
        return;

    const uint16_t opcode = (buf[15] << 8) | buf[16];
    if (opcode == 0x22F1)
    {
        const uint8_t level = buf[19];
        autoMode_ = false;
        timerEndMs_ = 0;
        switch (level)
        {
        case 0x00:
        case 0x01:
            setFanSetpoint(10.0f, now_ms); // away
            break;
        case 0x02:
            setFanSetpoint(20.0f, now_ms); // low
            break;
        case 0x03:
            setFanSetpoint(50.0f, now_ms); // medium
            break;
        case 0x04:
            setFanSetpoint(90.0f, now_ms); // high
            break;
        case 0x05:
            autoMode_ = true;
            evolve(now_ms);
            break;
        case 0x0B:
            setFanSetpoint(30.0f, now_ms); // autonight
            break;
        default:
            break;
        }
    }
    else if (opcode == 0x22F3)
    {
        const uint8_t minutes = buf[20];
        if (minutes > 0)
        {
            if (timerEndMs_ == 0)
                preTimerSetpoint_ = setpointPct_;
            autoMode_ = false;
            timerEndMs_ = now_ms + minutes * 60000UL;
            setpointPct_ = 90.0f;
            evolve(now_ms);
        }
    }
    // join/leave and other opcodes: accept silently
}

bool SimulatedDevice::handleFrame(const uint8_t *buf, size_t len, uint32_t now_ms)
{
    clearResponse();
    if (buf == nullptr || len < 5)
        return true;

    if (buf[0] == 0x82 && buf[1] == 0x80)
    {
        const uint16_t opcode = (buf[2] << 8) | buf[3];
        switch (opcode)
        {
        case 0x90E0:
            renderDeviceType();
            break;
        case 0x2400:
            renderStatusFormat();
            break;
        case 0x2401:
            evolve(now_ms);
            renderStatus();
            break;
        case 0x31DA:
            if (profile_->has31DA)
            {
                evolve(now_ms);
                render31DA();
            }
            break;
        case 0x31D9:
            if (profile_->has31D9)
            {
                evolve(now_ms);
                render31D9();
            }
            break;
        case 0x4210:
            if (profile_->hasCounters)
                renderCounters();
            break;
        case 0x2410:
            if (len >= 26 && buf[4] == 0x04)
            {
                renderQuery2410(buf[23]);
            }
            else if (len >= 26 && buf[4] == 0x06)
            {
                const uint8_t index = buf[23];
                settingValues_[index] = (static_cast<int32_t>(buf[6]) << 24) |
                                        (static_cast<int32_t>(buf[7]) << 16) |
                                        (static_cast<int32_t>(buf[8]) << 8) |
                                        buf[9];
                settingSet_[index / 8] |= 1 << (index % 8);
                renderSetAck2410(buf, len);
            }
            break;
        default:
            // 0x4030 / 0xCE30 (WPU setters), CO2 frames etc.: accept without reply
            break;
        }
        return true;
    }

    if (buf[0] == 0x00 && len >= 9 && buf[1] == 0x60 && buf[2] == 0xC0 && buf[3] == 0x20)
    {
        // PWM speed command (IthoPWMcommand), speed 0-254 at buf[6]
        autoMode_ = false;
        setFanSetpoint(buf[6] * 100.0f / 254.0f, now_ms);
        return true;
    }

    if (buf[0] == 0x82 && (buf[1] == 0x60 || buf[1] == 0x62) && buf[2] == 0xC1)
    {
        handleRemoteCmd(buf, len, now_ms);
        return true;
    }

    // anything else (PWM init, CO2 init/value, ...): accept without reply
    return true;
}
