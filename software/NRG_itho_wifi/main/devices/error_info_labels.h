#pragma once

struct ithoLabels
{
  const char *labelFull;
  const char *labelNormalized;
};

static const std::map<uint8_t, const char *> fanInfo = {
    {0x00, "off"},
    {0X01, "low"},
    {0X02, "medium"},
    {0X03, "high"},
    {0X04, "speed 4"},
    {0X05, "speed 5"},
    {0X06, "speed 6"},
    {0X07, "speed 7"},
    {0X08, "speed 8"},
    {0X09, "speed 9"},
    {0X0A, "speed 10"},
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
    {0X15, "away"},
    {0X16, "absolute minimum"},
    {0X17, "absolute maximum"},
    {0x18, "auto"},
    {0xFF, "unknown"}};

const std::map<uint8_t, const char *> fanSensorErrors = {
    {0xEF, "not available"},
    {0XF0, "shorted sensor"},
    {0XF1, "open sensor"},
    {0XF2, "not available error"},
    {0XF3, "out of range high"},
    {0XF4, "out of range low"},
    {0XF5, "not reliable"},
    {0XF6, "reserved error"},
    {0XF7, "reserved error"},
    {0XF8, "reserved error"},
    {0XF9, "reserved error"},
    {0XFA, "reserved error"},
    {0XFB, "reserved error"},
    {0XFC, "reserved error"},
    {0XFD, "reserved error"},
    {0XFE, "reserved error"},
    {0XFF, "unknown error"}};

const std::map<uint8_t, const char *> fanSensorErrors2 = {
    {0x7F, "not available"},
    {0xEF, "not available"},
    {0X80, "shorted sensor"},
    {0X81, "open sensor"},
    {0X82, "not available error"},
    {0X83, "out of range high"},
    {0X84, "out of range low"},
    {0X85, "not reliable"},
    {0XFF, "unknown error"}};

const std::map<uint8_t, const char *> fanHeatErrors = {
    {0xEF, "not available"},
    {0XF0, "open actuato"},
    {0XF1, "shorted actuato"},
    {0XF2, "not available error"},
    {0XFF, "unknown error"}};

struct ithoLabels ithoLabelErrors[]{
    {"Nullptr error", "nullptr-error"},
    {"No label for device error", "no-label-for-device-error"},
    {"Version error newer than latest in firmware", "version-error-newer-than-latest-in-firmware"},
    {"No label for version error", "no-label-for-version-error"},
    {"Label out of bound error", "label-out-of-bound-error"}};

struct ithoLabels itho31D9Labels[]{
    {"Speed status", "speed-status"},
    {"Internal fault", "internal-fault"},
    {"Frost cycle", "frost-cycle"},
    {"Filter dirty", "filter-dirty"}};

struct ithoLabels itho31DALabels[]{
    {"AirQuality (%)", "airquality_perc"},
    {"AirQbased on", "airq-based-on"},
    {"CO2level (ppm)", "co2level_ppm"},
    {"Indoorhumidity (%)", "indoor-humidity_perc"},
    {"Outdoorhumidity (%)", "outdoor-humidity_perc"},
    {"Exhausttemp (°C)", "exhausttemp_c"},
    {"SupplyTemp (°C)", "supply-temp_c"},
    {"IndoorTemp (°C)", "indoor-temp_c"},
    {"OutdoorTemp (°C)", "outdoor-temp_c"},
    {"SpeedCap", "speed-cap"},
    {"BypassPos (%)", "bypass-pos_perc"},
    {"FanInfo", "fan-info"},
    {"ExhFanSpeed (%)", "exh-fan-speed_perc"},
    {"InFanSpeed (%)", "in-fan-speed_perc"},
    {"RemainingTime (min)", "remaining-time_min"},
    {"PostHeat (%)", "post-heat_perc"},
    {"PreHeat (%)", "pre-heat_perc"},
    {"InFlow (l sec)", "in-flow-l_sec"},
    {"ExhFlow (l sec)", "exh-flow-l_sec"}};