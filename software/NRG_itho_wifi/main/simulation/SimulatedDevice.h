#pragma once

/*
 * Simulated Device Runtime.
 *
 * When active, this class impersonates an Itho device at the I2C transport layer:
 * i2cSendBytes() routes frames to handleFrame() and i2cSlaveReceive() returns the
 * response prepared here (see ithodevice/i2c_esp32.cpp). All higher firmware layers
 * (IthoStatus parsing, MQTT, REST, WebSocket, HA discovery) run unchanged.
 *
 * The core is intentionally free of Arduino/ESP dependencies: time is injected as
 * uint32_t milliseconds so the exact same code runs in the native unit tests
 * (test/test_native_simulation). Telemetry is a pure function of (seed, scenario,
 * time since begin), so a fixed seed and query timeline reproduce identical bytes.
 */

#include <cstddef>
#include <cstdint>

#include "SimProfiles.h"

enum SimScenario : uint8_t
{
    SIM_SCENARIO_NORMAL = 0,
    SIM_SCENARIO_BOOST,
    SIM_SCENARIO_FAULT,
    SIM_SCENARIO_HUMIDITY_SPIKE,
    SIM_SCENARIO_CO2_RISE,
    SIM_SCENARIO_COUNT
};

class SimulatedDevice
{
public:
    void begin(uint8_t profileIdx, uint32_t seed, uint8_t scenario, uint32_t now_ms);
    void end();
    bool active() const { return active_; }

    // I2C transport hooks
    bool handleFrame(const uint8_t *buf, size_t len, uint32_t now_ms);
    size_t popResponse(uint8_t *out);

    // runtime introspection (REST /api/v2/simulation, UI, tests)
    void setScenario(uint8_t scenario);
    uint8_t scenario() const { return scenario_; }
    uint8_t profileIndex() const { return profileIdx_; }
    const char *profileName() const;
    const char *scenarioName() const;
    uint32_t seed() const { return seed_; }
    float fanSetpointPct() const { return setpointPct_; }
    float fanActualPct() const { return actualPct_; }
    uint32_t uptimeMs(uint32_t now_ms) const { return active_ ? now_ms - t0_ : 0; }

    static uint8_t profileCount() { return simProfileCount; }
    static const char *profileNameAt(uint8_t idx);
    static const char *scenarioNameAt(uint8_t idx);
    static int profileIndexFromName(const char *name); // -1 when unknown
    static int scenarioFromName(const char *name);     // -1 when unknown

private:
    void evolve(uint32_t now_ms);
    float noise(uint32_t slot, uint32_t salt) const; // deterministic, in [-1, 1]
    int32_t fieldValue(const SimField *f, uint8_t index, uint8_t datatype) const;
    const SimField *findField(uint8_t index) const;

    // response builders (full receive-buffer image incl. 0x80 prefix and checksum)
    uint8_t *beginResponse(uint8_t opHi, uint8_t opLo, uint8_t payloadLen);
    void endResponse(uint8_t payloadLen);
    void clearResponse() { respLen_ = 0; }

    void renderDeviceType();
    void renderStatusFormat();
    void renderStatus();
    void render31DA();
    void render31D9();
    void renderCounters();
    void renderQuery2410(uint8_t index);
    void renderSetAck2410(const uint8_t *req, size_t len);

    void handleRemoteCmd(const uint8_t *buf, size_t len, uint32_t now_ms);
    void setFanSetpoint(float pct, uint32_t now_ms);

    bool active_{false};
    const SimProfile *profile_{nullptr};
    uint8_t profileIdx_{0};
    uint32_t seed_{0};
    uint8_t scenario_{SIM_SCENARIO_NORMAL};

    uint32_t t0_{0};         // begin() time
    uint32_t lastEvolve_{0}; // last state advance
    float setpointPct_{40.0f};
    float actualPct_{40.0f};
    bool autoMode_{true};
    uint32_t timerEndMs_{0}; // 22F3 timer expiry, 0 = no timer
    float preTimerSetpoint_{40.0f};
    // shared telemetry, updated in evolve()
    float temperature_{21.0f};
    float humidity_{45.0f};
    float co2_{650.0f};

    // 2410 settings store
    int32_t settingValues_[256]{};
    uint8_t settingSet_[32]{}; // bitmap: value explicitly written

    uint8_t resp_[512]{};
    size_t respLen_{0};
};

extern SimulatedDevice simulatedDevice;
