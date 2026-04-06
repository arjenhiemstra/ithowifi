
#include "IthoDevice.h"

#include "devices/cve14.h"
#include "devices/cve1B.h"
#include "devices/hru200.h"
#include "devices/hru250_300.h"
#include "devices/hru350.h"
#include "devices/hrueco.h"
#include "devices/demandflow.h"
#include "devices/autotemp.h"
#include "devices/wpu.h"

uint8_t ithoDeviceGroup = 0;
uint8_t ithoDeviceID = 0;
uint8_t itho_hwversion = 0;
uint8_t itho_fwversion = 0;
volatile uint16_t ithoCurrentVal = 0;
volatile uint8_t ithoFanDemand = 0;
const struct ihtoDeviceType *ithoDeviceptr = nullptr;
int16_t ithoSettingsLength = 0;
int16_t ithoStatusLabelLength = 0;

struct lastCommand lastCmd;

const std::map<cmdOrigin, const char *> cmdOriginMap = {
    {cmdOrigin::HTMLAPI, "HTML API"},
    {cmdOrigin::MQTTAPI, "MQTT API"},
    {cmdOrigin::REMOTE, "remote"},
    {cmdOrigin::WEB, "web interface"},
    {cmdOrigin::UNKNOWN, "unknown"}};

const struct ihtoDeviceType ithoDevices[]{
    {0x00, 0x01, "Air curtain", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x03, "HRU ECO-fan", ithoHRUecoFanSettingsMap, sizeof(ithoHRUecoFanSettingsMap) / sizeof(ithoHRUecoFanSettingsMap[0]), ithoHRUecoSettingsLabels, ithoHRUecoFanStatusMap, sizeof(ithoHRUecoFanStatusMap) / sizeof(ithoHRUecoFanStatusMap[0]), ithoHRUecoStatusLabels},
    {0x00, 0x04, "CVE ECO2", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x08, "LoadBoiler", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x0A, "GGBB", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x0B, "DemandFlow", ithoDemandFlowSettingsMap, sizeof(ithoDemandFlowSettingsMap) / sizeof(ithoDemandFlowSettingsMap[0]), ithoDemandFlowSettingsLabels, ithoDemandFlowStatusMap, sizeof(ithoDemandFlowStatusMap) / sizeof(ithoDemandFlowStatusMap[0]), ithoDemandFlowStatusLabels},
    {0x00, 0x0C, "CO2 relay", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x0D, "Heatpump", ithoWPUSettingsMap, sizeof(ithoWPUSettingsMap) / sizeof(ithoWPUSettingsMap[0]), ithoWPUSettingsLabels, ithoWPUStatusMap, sizeof(ithoWPUStatusMap) / sizeof(ithoWPUStatusMap[0]), ithoWPUStatusLabels},
    {0x00, 0x0E, "OLB Single", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x0F, "AutoTemp", ithoAutoTempSettingsMap, sizeof(ithoAutoTempSettingsMap) / sizeof(ithoAutoTempSettingsMap[0]), ithoAutoTempSettingsLabels, ithoAutoTempStatusMap, sizeof(ithoAutoTempStatusMap) / sizeof(ithoAutoTempStatusMap[0]), ithoAutoTempStatusLabels},
    {0x00, 0x10, "OLB Double", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x11, "RF+", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x14, "CVE", itho14SettingsMap, sizeof(itho14SettingsMap) / sizeof(itho14SettingsMap[0]), ithoCVE14SettingsLabels, itho14StatusMap, sizeof(itho14StatusMap) / sizeof(itho14StatusMap[0]), ithoCVE14StatusLabels},
    {0x00, 0x15, "Extended", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x16, "Extended Plus", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x1A, "AreaFlow", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x1B, "CVE-Silent", itho1BSettingsMap, sizeof(itho1BSettingsMap) / sizeof(itho1BSettingsMap[0]), ithoCVE1BSettingsLabels, itho1BStatusMap, sizeof(itho1BStatusMap) / sizeof(itho1BStatusMap[0]), ithoCVE1BStatusLabels},
    {0x00, 0x1C, "CVE-SilentExt", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x1D, "CVE-SilentExtPlus", ithoHRU200SettingsMap, sizeof(ithoHRU200SettingsMap) / sizeof(ithoHRU200SettingsMap[0]), ithoHRU200SettingsLabels, ithoHRU200StatusMap, sizeof(ithoHRU200StatusMap) / sizeof(ithoHRU200StatusMap[0]), ithoHRU200StatusLabels},
    {0x00, 0x20, "RF_CO2", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x2B, "HRU 350", ithoHRU350SettingsMap, sizeof(ithoHRU350SettingsMap) / sizeof(ithoHRU350SettingsMap[0]), ithoHRU350SettingsLabels, ithoHRU350StatusMap, sizeof(ithoHRU350StatusMap) / sizeof(ithoHRU350StatusMap[0]), ithoHRU350StatusLabels},
    {0x00, 0x30, "AutoTemp Basic", ithoAutoTempSettingsMap, sizeof(ithoAutoTempSettingsMap) / sizeof(ithoAutoTempSettingsMap[0]), ithoAutoTempSettingsLabels, ithoAutoTempStatusMap, sizeof(ithoAutoTempStatusMap) / sizeof(ithoAutoTempStatusMap[0]), ithoAutoTempStatusLabels},
    {0x07, 0x01, "HRU 250-300", ithoHRU250_300SettingsMap, sizeof(ithoHRU250_300SettingsMap) / sizeof(ithoHRU250_300SettingsMap[0]), ithoHRU250_300SettingsLabels, ithoHRU250_300StatusMap, sizeof(ithoHRU250_300StatusMap) / sizeof(ithoHRU250_300StatusMap[0]), ithoHRU250_300StatusLabels}};

const size_t ithoDevicesLength = sizeof(ithoDevices) / sizeof(ithoDevices[0]);

int currentIthoDeviceGroup() { return ithoDeviceGroup; }
int currentIthoDeviceID() { return ithoDeviceID; }
uint8_t currentItho_hwversion() { return itho_hwversion; }
uint8_t currentItho_fwversion() { return itho_fwversion; }
uint16_t currentIthoSettingsLength() { return ithoSettingsLength; }
int16_t currentIthoStatusLabelLength() { return ithoStatusLabelLength; }

const char *getIthoType()
{
  static char ithoDeviceType[32] = "Unkown device type";

  const struct ihtoDeviceType *ithoDevicesptr = ithoDevices;
  const struct ihtoDeviceType *ithoDevicesendPtr = ithoDevices + ithoDevicesLength;
  while (ithoDevicesptr < ithoDevicesendPtr)
  {
    if (ithoDevicesptr->DG == ithoDeviceGroup && ithoDevicesptr->ID == currentIthoDeviceID())
    {
      strlcpy(ithoDeviceType, ithoDevicesptr->name, sizeof(ithoDeviceType));
    }
    ithoDevicesptr++;
  }
  return ithoDeviceType;
}

const struct ihtoDeviceType *getDevicePtr(const uint8_t deviceGroup, const uint8_t deviceID)
{

  const struct ihtoDeviceType *ithoDevicesptr = ithoDevices;
  const struct ihtoDeviceType *ithoDevicesendPtr = ithoDevices + ithoDevicesLength;
  while (ithoDevicesptr < ithoDevicesendPtr)
  {
    if (ithoDevicesptr->DG == deviceGroup && ithoDevicesptr->ID == deviceID)
    {
      return ithoDevicesptr;
    }
    ithoDevicesptr++;
  }
  return nullptr;
}
