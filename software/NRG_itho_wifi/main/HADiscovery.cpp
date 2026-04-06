#include <ArduinoJson.h>
#include <string>
// #include <algorithm>
// #include <cctype>

#include "ithodevice/IthoDevice.h"
#include "notifyClients.h"
#include "sys_log.h"
#include "generic_functions.h"
#include "config/HADiscConfig.h"

// Function to normalize the unique ID
std::string normalizeUniqueId(const std::string &input)
{
    std::string normalized = input;
    std::replace(normalized.begin(), normalized.end(), ' ', '_'); // Replace spaces with underscores
    normalized.erase(std::remove_if(normalized.begin(), normalized.end(),
                                    [](unsigned char c)
                                    {
                                        return !std::isalnum(c) && c != '_' && c != '-';
                                    }),
                     normalized.end());                                                  // Remove invalid characters
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower); // Convert to lowercase
    return normalized;
}

void addHADevInfo(JsonObject obj)
{
    char cu[50]{};
    snprintf(cu, sizeof(cu), "http://%s.local", hostName());
    JsonObject dev = obj["dev"].to<JsonObject>();
    dev["ids"] = hostName();                 // identifiers
    dev["mf"] = "Arjen Hiemstra";            // manufacturer
    dev["mdl"] = "Wifi add-on for Itho";     // model
    dev["name"] = hostName();                // name
    dev["hw"] = hardwareManager.hw_revision; // hw_version
    dev["sw"] = fw_version;                  // sw_version
    dev["cu"] = cu;                          // configuration_url
}

static std::string getActualSpeedLabel()
{
    const uint8_t deviceID = currentIthoDeviceID();
    const uint8_t deviceGroup = currentIthoDeviceGroup();

    if (deviceGroup == 0x07 && deviceID == 0x01) // HRU250-300
        return getStatusLabel(10, ithoDeviceptr);
    if (deviceGroup == 0x00 && deviceID == 0x2B) // HRU350
        return getStatusLabel(0, ithoDeviceptr);
    if (deviceGroup == 0x00 && deviceID == 0x03) // HRU-eco
        return getStatusLabel(22, ithoDeviceptr);
    if (deviceGroup == 0x00 && deviceID == 0x1D) // HRU200
        return getStatusLabel(0, ithoDeviceptr);
    if (deviceGroup == 0x00 && deviceID == 0x04) // CVE Eco2
        return getSpeedLabel();
    if (deviceGroup == 0x00 && deviceID == 0x14) // CVE
        return getStatusLabel(0, ithoDeviceptr);
    if (deviceGroup == 0x00 && deviceID == 0x1B) // CVE-Silent
        return getStatusLabel(0, ithoDeviceptr);

    return "";
}

void addHADiscoveryFan(JsonObject obj, const char *name)
{

    char ihtostatustopic[140]{};
    char cmdtopic[140]{};
    char statetopic[140]{};
    char modestatetopic[140]{};
    snprintf(ihtostatustopic, sizeof(ihtostatustopic), "%s%s", systemConfig.mqtt_base_topic, "/ithostatus");
    snprintf(cmdtopic, sizeof(cmdtopic), "%s%s", systemConfig.mqtt_base_topic, "/cmd");
    snprintf(statetopic, sizeof(statetopic), "%s%s", systemConfig.mqtt_base_topic, "/state");
    snprintf(modestatetopic, sizeof(modestatetopic), "%s%s", systemConfig.mqtt_base_topic, "/modestate");

    std::string uniqueId = normalizeUniqueId(std::string(name) + "_fan");
    JsonObject componentJson = obj[uniqueId].to<JsonObject>();
    componentJson["name"] = "Fan";
    componentJson["p"] = "fan";
    componentJson["uniq_id"] = uniqueId;
    componentJson["json_attr_t"] = ihtostatustopic;                                    // json_attributes_topic
    componentJson["cmd_t"] = cmdtopic;                                                 // command_topic
    componentJson["pct_cmd_t"] = cmdtopic;                                             // percentage_command_topic
    componentJson["pct_stat_t"] = statetopic;                                          // percentage_state_topic
    componentJson["stat_val_tpl"] = "{% if value == '0' %}OFF{% else %}ON{% endif %}"; // state_value_template
    componentJson["pr_mode_cmd_t"] = cmdtopic;                                         // preset_mode_command_topic
    componentJson["pr_mode_stat_t"] = ihtostatustopic;                                 // preset_mode_state_topic

    std::string actualSpeedLabel;

    const uint8_t deviceID = currentIthoDeviceID();
    // const uint8_t version = currentItho_fwversion();
    const uint8_t deviceGroup = currentIthoDeviceGroup();

    // Determine preset modes from remote type at index 0 for supported device types
    bool useRemotePresets = false;
    uint16_t firstRemoteType = 0;

    if ((deviceGroup == 0x07 && deviceID == 0x01) ||                                                                                                  // HRU250-300
        (deviceGroup == 0x00 && deviceID == 0x2B) ||                                                                                                  // HRU350
        (deviceGroup == 0x00 && deviceID == 0x03) ||                                                                                                  // HRU-eco
        (deviceGroup == 0x00 && (deviceID == 0x4 || deviceID == 0x1D || deviceID == 0x14 || deviceID == 0x1B) && systemConfig.itho_pwm2i2c != 1)) // CVE/HRU200 with PWM2I2C off
    {
        if (deviceGroup == 0x07 && deviceID == 0x01) // HRU250-300: use RF remote at index 0
        {
            firstRemoteType = static_cast<uint16_t>(remotes.getRemoteType(0));
        }
        else // Other supported types: use virtual remote at index 0
        {
            firstRemoteType = static_cast<uint16_t>(virtualRemotes.getRemoteType(0));
        }
        useRemotePresets = (firstRemoteType != 0);
    }

    JsonArray modes = componentJson["pr_modes"].to<JsonArray>(); // preset_modes
    if (useRemotePresets && firstRemoteType != 0)
    {
        const char *presetsStr = HADiscConfig::getPresetsForType(firstRemoteType);
        if (presetsStr && strlen(presetsStr) > 0)
        {
            std::string presets(presetsStr);
            size_t pos = 0;
            while ((pos = presets.find(',')) != std::string::npos)
            {
                modes.add(presets.substr(0, pos).c_str());
                presets.erase(0, pos + 1);
            }
            if (!presets.empty())
            {
                modes.add(presets.c_str());
            }
        }
    }
    else
    {
        modes.add("Low");
        modes.add("Medium");
        modes.add("High");
        modes.add("Auto");
        modes.add("AutoNight");
        modes.add("Timer 10min");
        modes.add("Timer 20min");
        modes.add("Timer 30min");
    }

    // Determine mid/low commands based on remote type presets
    // e.g. RFTCVE/RFTN have Medium but no Auto, RFTAUTO/RFTAUTON have Auto but no Medium
    const char *midCmd = "medium";
    const char *lowCmd = "auto";
    if (useRemotePresets && firstRemoteType != 0)
    {
        const char *p = HADiscConfig::getPresetsForType(firstRemoteType);
        bool hasMedium = (p && strstr(p, "Medium") != nullptr);
        bool hasAuto = (p && strstr(p, "Auto") != nullptr);
        midCmd = hasAuto ? "auto" : (hasMedium ? "medium" : "medium");
        lowCmd = "low";
    }

    // N_LOG("HAD: dID:0x%02X, gID:0x%02X, devPtr:%s",deviceID, deviceGroup, ithoDeviceptr != nullptr ? "true" : "false");
    char pr_mode_val_tpl[400]{};
    char pct_cmd_tpl[300]{};
    char pct_val_tpl[100]{};
    int pr_mode_val_tpl_ver = 0;
    const char *cmdKey = "vremotecmd";

    if (deviceGroup == 0x07 && deviceID == 0x01) // HRU250-300
    {
        actualSpeedLabel = getStatusLabel(10, ithoDeviceptr); //-> {"Absolute speed of the fan (%)", "absolute-speed-of-the-fan_perc"}, of hru250_300.h
        cmdKey = "rfremotecmd";
        pr_mode_val_tpl_ver = 1;
    }
    else if (deviceGroup == 0x00 && deviceID == 0x2B) // HRU350
    {
        actualSpeedLabel = getStatusLabel(0, ithoDeviceptr); //-> {"Requested fanspeed (%)", "requested-fanspeed_perc"}, of hru350.h
        pr_mode_val_tpl_ver = 1;
    }
    else if (deviceGroup == 0x00 && deviceID == 0x03) // HRU-eco
    {
        actualSpeedLabel = getStatusLabel(22, ithoDeviceptr); //-> {"Requested fanspeed (%)", "requested-fanspeed_perc"}, of hrueco.h
        pr_mode_val_tpl_ver = 1;
    }
    // else if (deviceGroup == 0x00 && deviceID == 0x0D) // WPU
    // {
    //     W_LOG("HAD: WPU not fully implemented yet for HA Auto Discovery");
    // }
    // else if (deviceGroup == 0x00 && (deviceID == 0x0F || deviceID == 0x30)) // Autotemp
    // {
    //     W_LOG("HAD: Autotemp not fully implemented yet for HA Auto Discovery");
    // }
    // else if (deviceGroup == 0x00 && deviceID == 0x0B) // DemandFlow
    // {
    //     W_LOG("HAD: DemandFlow not fully implemented yet for HA Auto Discovery");
    // }
    else if (deviceGroup == 0x00 && (deviceID == 0x4 || deviceID == 0x1D || deviceID == 0x14 || deviceID == 0x1B)) // CVE and HRU200
    {
        if (deviceID == 0x1D) // hru200
            actualSpeedLabel = getStatusLabel(0, ithoDeviceptr); //-> {"Ventilation setpoint (%)", "ventilation-setpoint_perc"}, of hru200.h
        else if (deviceID == 0x4) // cve eco2 0x4
            actualSpeedLabel = getSpeedLabel(); //-> {"Speed status", "speed-status"}, of error_info_labels.h
        else if (deviceID == 0x14) // cve 0x14
            actualSpeedLabel = getStatusLabel(0, ithoDeviceptr); //-> {"Ventilation level (%)", "ventilation-level_perc"}, of cve14.h
        else if (deviceID == 0x1B) // cve 0x1B
            actualSpeedLabel = getStatusLabel(0, ithoDeviceptr); //-> {"Ventilation setpoint (%)", "ventilation-setpoint_perc"}, of cve1b.h

        if (systemConfig.itho_pwm2i2c != 1)
        {
            pr_mode_val_tpl_ver = 1;
        }
        else
        {
            strlcpy(pct_val_tpl, "{{ (value | int / 2.55) | round | int }}", sizeof(pct_val_tpl));
            strlcpy(pct_cmd_tpl, "{{ (value | int * 2.55) | round | int }}", sizeof(pct_cmd_tpl));
            componentJson["pl_off"] = "0"; // payload_off
        }
    }
    else
    {
        W_LOG("HAD: Device type not implemented (yet) for HA Auto Discovery");
    }

    if (pr_mode_val_tpl_ver == 0)
    {
        componentJson["pr_mode_cmd_tpl"] = "{%- if value == 'Timer 10min' %}{{'timer1'}}{%- elif value == 'Timer 20min' %}{{'timer2'}}{%- elif value == 'Timer 30min' %}{{'timer3'}}{%- else %}{{value.lower()}}{%- endif -%}";
        snprintf(pr_mode_val_tpl, sizeof(pr_mode_val_tpl), "{%%- set speed = value_json['%s'] | int %%}{%%- if speed > 90 %%}High{%%- elif speed > 35 %%}Medium{%%- elif speed > 10 %%}Low{%%- else %%}Auto{%%- endif -%%}", actualSpeedLabel.c_str());
    }
    else if (pr_mode_val_tpl_ver == 1)
    {
        char cmdTplBuf[64];
        snprintf(cmdTplBuf, sizeof(cmdTplBuf), "{\"%s\":\"{{value.lower()}}\"}", cmdKey);
        componentJson["pr_mode_cmd_tpl"] = cmdTplBuf; // preset_mode_command_template

        snprintf(pct_cmd_tpl, sizeof(pct_cmd_tpl),
                 "{%% if value > 90 %%}{\"%s\":\"high\"}"
                 "{%% elif value > 40 %%}{\"%s\":\"%s\"}"
                 "{%% elif value > 20 %%}{\"%s\":\"low\"}"
                 "{%% else %%}{\"%s\":\"%s\"}"
                 "{%% endif %%}",
                 cmdKey, cmdKey, midCmd, cmdKey, cmdKey, lowCmd);

        snprintf(pct_val_tpl, sizeof(pct_val_tpl), "{{ value_json['%s'] | int }}", actualSpeedLabel.c_str());
        componentJson["pl_off"] = (std::string("{\"") + cmdKey + "\":\"" + lowCmd + "\"}").c_str(); // payload_off

        const char *midLabel = (strcmp(midCmd, "auto") == 0) ? "Auto" : "Medium";
        const char *lowLabel = (strcmp(lowCmd, "auto") == 0) ? "Auto" : "Low";
        snprintf(pr_mode_val_tpl, sizeof(pr_mode_val_tpl),
                 "{%%- set speed = value_json['%s'] | int %%}"
                 "{%%- if speed > 90 %%}High"
                 "{%%- elif speed > 35 %%}%s"
                 "{%%- elif speed > 10 %%}Low"
                 "{%%- else %%}%s"
                 "{%%- endif -%%}",
                 actualSpeedLabel.c_str(), midLabel, lowLabel);
    }

    componentJson["pct_cmd_tpl"] = pct_cmd_tpl;         // percentage_command_template
    componentJson["pr_mode_val_tpl"] = pr_mode_val_tpl; // preset_mode_value_template
    componentJson["pct_val_tpl"] = pct_val_tpl;         // percentage_value_template
}

void addHADiscoveryFWUpdate(JsonObject obj, const char *name)
{

    char ihtostatustopic[140]{};
    char ithodevicetopic[140]{};
    char cmdtopic[140]{};
    char statetopic[140]{};
    snprintf(ihtostatustopic, sizeof(ihtostatustopic), "%s%s", systemConfig.mqtt_base_topic, "/ithostatus");
    snprintf(cmdtopic, sizeof(cmdtopic), "%s%s", systemConfig.mqtt_base_topic, "/cmd");
    snprintf(statetopic, sizeof(statetopic), "%s%s", systemConfig.mqtt_base_topic, "/state");
    snprintf(ithodevicetopic, sizeof(ithodevicetopic), "%s%s", systemConfig.mqtt_base_topic, "/deviceinfo");

    std::string uniqueId = normalizeUniqueId(std::string(name) + "_fwupdate");
    JsonObject componentJson = obj[uniqueId].to<JsonObject>();
    componentJson["name"] = "Firmware Update";
    componentJson["p"] = "update";
    componentJson["uniq_id"] = uniqueId;
    componentJson["dev_cla"] = "firmware";

    std::string tmpstr = "http://" + std::string(WiFi.localIP().toString().c_str()) + "/favicon.png";
    componentJson["ent_pic"] = tmpstr;
    // componentJson["icon"] = "mdi:update";

    componentJson["stat_t"] = ithodevicetopic;

    tmpstr = "{{ {'installed_version': value_json['add-on_fwversion'], 'latest_version': value_json['add-on_latest_fw'], 'title': 'Add-on Firmware', 'release_url': 'https://github.com/arjenhiemstra/ithowifi/releases/tag/Version-" + std::string(firmwareInfo.latest_fw) + "' } | to_json }}";
    componentJson["val_tpl"] = tmpstr;

    componentJson["cmd_t"] = cmdtopic;
    componentJson["payload_install"] = "{\"update\":\"stable\"}";
}

void addHADiscoveryRFSensors(JsonObject components, JsonObject compactJson, const char *deviceName, const char *mqttBaseTopic)
{
    // Process RF device sensors from compact JSON
    JsonArray rfArray = compactJson["rf"].as<JsonArray>();
    if (rfArray.isNull())
        return;

    char remotesinfotopic[140]{};
    snprintf(remotesinfotopic, sizeof(remotesinfotopic), "%s%s", mqttBaseTopic, "/remotesinfo");

    for (JsonObject rfDevice : rfArray)
    {
        int idx = rfDevice["idx"] | -1;
        const char *remoteName = rfDevice["name"];
        JsonArray sensors = rfDevice["sensors"].as<JsonArray>();

        if (idx < 0 || remoteName == nullptr || sensors.isNull())
            continue;

        for (JsonVariant sensorVar : sensors)
        {
            const char *sensor = sensorVar.as<const char *>();
            if (sensor == nullptr)
                continue;

            // Generate unique ID for this RF sensor
            std::string uniqueId = normalizeUniqueId(std::string(deviceName) + "_rf_" + remoteName + "_" + sensor);

            // Create component object
            JsonObject componentJson = components[uniqueId].to<JsonObject>();
            componentJson["p"] = "sensor";
            componentJson["uniq_id"] = uniqueId;
            componentJson["stat_t"] = remotesinfotopic;

            // Set sensor-specific properties
            if (strcmp(sensor, "temp") == 0)
            {
                componentJson["name"] = (std::string(remoteName) + " Temperature").c_str();
                componentJson["val_tpl"] = ("{{ value_json['" + std::string(remoteName) + "']['temp'] | default('unknown') }}").c_str();
                componentJson["unit_of_meas"] = "°C";
                componentJson["dev_cla"] = "temperature";
                componentJson["stat_cla"] = "measurement";
            }
            else if (strcmp(sensor, "hum") == 0)
            {
                componentJson["name"] = (std::string(remoteName) + " Humidity").c_str();
                componentJson["val_tpl"] = ("{{ value_json['" + std::string(remoteName) + "']['hum'] | default('unknown') }}").c_str();
                componentJson["unit_of_meas"] = "%";
                componentJson["dev_cla"] = "humidity";
                componentJson["stat_cla"] = "measurement";
            }
            else if (strcmp(sensor, "dewpoint") == 0)
            {
                componentJson["name"] = (std::string(remoteName) + " Dew Point").c_str();
                componentJson["val_tpl"] = ("{{ value_json['" + std::string(remoteName) + "']['dewpoint'] | default('unknown') }}").c_str();
                componentJson["unit_of_meas"] = "°C";
                componentJson["dev_cla"] = "temperature";
                componentJson["stat_cla"] = "measurement";
            }
            else if (strcmp(sensor, "co2") == 0)
            {
                componentJson["name"] = (std::string(remoteName) + " CO2").c_str();
                componentJson["val_tpl"] = ("{{ value_json['" + std::string(remoteName) + "']['co2'] | default('unknown') }}").c_str();
                componentJson["unit_of_meas"] = "ppm";
                componentJson["dev_cla"] = "carbon_dioxide";
                componentJson["stat_cla"] = "measurement";
            }
            else if (strcmp(sensor, "battery") == 0)
            {
                componentJson["name"] = (std::string(remoteName) + " Battery").c_str();
                componentJson["val_tpl"] = ("{{ value_json['" + std::string(remoteName) + "']['battery'] | default('unknown') }}").c_str();
                componentJson["unit_of_meas"] = "%";
                componentJson["dev_cla"] = "battery";
                componentJson["stat_cla"] = "measurement";
            }
            else if (strcmp(sensor, "pir") == 0)
            {
                componentJson["name"] = (std::string(remoteName) + " PIR").c_str();
                componentJson["val_tpl"] = ("{{ value_json['" + std::string(remoteName) + "']['pir'] | default('unknown') }}").c_str();
                componentJson["dev_cla"] = "motion";
            }
            else if (strcmp(sensor, "lastcmd") == 0)
            {
                componentJson["name"] = (std::string(remoteName) + " Last Command").c_str();
                // Convert lastcmdmsg to human-readable: IthoLow->Low, IthoTimer1->Timer 10min, etc.
                char lastcmdTpl[512]{};
                snprintf(lastcmdTpl, sizeof(lastcmdTpl),
                         "{%%- set cmd = value_json['%s']['lastcmdmsg'] | default('') -%%}"
                         "{%%- if cmd.startswith('Itho') -%%}"
                         "{%%- set c = cmd[4:] -%%}"
                         "{%%- if c == 'Timer1' -%%}Timer 10min"
                         "{%%- elif c == 'Timer2' -%%}Timer 20min"
                         "{%%- elif c == 'Timer3' -%%}Timer 30min"
                         "{%%- elif c.startswith('Cook') -%%}Cook {{ c[4:] }}min"
                         "{%%- else -%%}{{ c }}"
                         "{%%- endif -%%}"
                         "{%%- else -%%}unknown{%%- endif -%%}",
                         remoteName);
                componentJson["val_tpl"] = lastcmdTpl;
            }
        }
    }
}

// Generic helper for creating fan entities from remote device arrays (RF or virtual)
// arrayKey: "rf" or "vr" (key in compactJson)
// cmdKey: "rfremotecmd" or "vremotecmd" (MQTT command key)
// idxKey: "rfremoteindex" or "vremoteindex" (MQTT index key)
// idPrefix: "rf" or "vr" (for unique ID generation)
// hasStateTopic: true for RF (has /remotesinfo), false for virtual (no per-remote state)
void addHADiscoveryRemoteFan(JsonObject components, JsonObject compactJson, const char *deviceName, const char *mqttBaseTopic,
                             const char *arrayKey, const char *cmdKey, const char *idxKey, const char *idPrefix, bool hasStateTopic)
{
    JsonArray devArray = compactJson[arrayKey].as<JsonArray>();
    if (devArray.isNull())
        return;

    char cmdtopic[140]{};
    snprintf(cmdtopic, sizeof(cmdtopic), "%s%s", mqttBaseTopic, "/cmd");

    char remotesinfotopic[140]{};
    if (hasStateTopic)
        snprintf(remotesinfotopic, sizeof(remotesinfotopic), "%s%s", mqttBaseTopic, "/remotesinfo");

    for (JsonObject device : devArray)
    {
        if (!device["fan"].is<bool>() || !device["fan"].as<bool>())
            continue;

        int idx = device["idx"] | -1;
        const char *remoteName = device["name"];
        uint16_t remoteType = device["type"] | 0;

        if (idx < 0 || remoteName == nullptr)
            continue;

        std::string uniqueId = normalizeUniqueId(std::string(deviceName) + "_" + idPrefix + "_" + remoteName + "_fan");

        JsonObject componentJson = components[uniqueId].to<JsonObject>();
        componentJson["name"] = (std::string(remoteName) + " Fan").c_str();
        componentJson["p"] = "fan";
        componentJson["uniq_id"] = uniqueId;
        componentJson["cmd_t"] = cmdtopic;
        componentJson["pr_mode_cmd_t"] = cmdtopic;

        if (hasStateTopic)
            componentJson["pr_mode_stat_t"] = remotesinfotopic;

        // Command template: converts preset names to commands
        // "Timer 10min" -> "timer1", "Cook 30min" -> "cook30", etc.
        char cmdTemplate[256]{};
        snprintf(cmdTemplate, sizeof(cmdTemplate),
                 "{\"%s\":\"{%%-if 'Timer' in value-%%}timer{{value[-5]}}{%%-elif 'Cook' in value-%%}cook{{value.split()[1].replace('min','')}}{%%-else-%%}{{value.lower().replace(' ','')}}{%%-endif-%%}\",\"%s\":%d}",
                 cmdKey, idxKey, idx);
        componentJson["pr_mode_cmd_tpl"] = cmdTemplate;

        // State template (only if state topic available)
        if (hasStateTopic)
        {
            char valTpl[512]{};
            snprintf(valTpl, sizeof(valTpl),
                     "{%%- set cmd = value_json['%s']['lastcmdmsg'] | default('') -%%}"
                     "{%%- if cmd.startswith('Itho') -%%}"
                     "{%%- set c = cmd[4:] -%%}"
                     "{%%- if c == 'Timer1' -%%}Timer 10min"
                     "{%%- elif c == 'Timer2' -%%}Timer 20min"
                     "{%%- elif c == 'Timer3' -%%}Timer 30min"
                     "{%%- elif c.startswith('Cook') -%%}Cook {{ c[4:] }}min"
                     "{%%- else -%%}{{ c }}"
                     "{%%- endif -%%}"
                     "{%%- else -%%}unknown{%%- endif -%%}",
                     remoteName);
            componentJson["pr_mode_val_tpl"] = valTpl;
        }

        // Add preset modes based on remote type
        const char *presetsStr = HADiscConfig::getPresetsForType(remoteType);
        if (presetsStr && strlen(presetsStr) > 0)
        {
            JsonArray modes = componentJson["pr_modes"].to<JsonArray>();
            std::string presets(presetsStr);
            size_t pos = 0;
            while ((pos = presets.find(',')) != std::string::npos)
            {
                modes.add(presets.substr(0, pos).c_str());
                presets.erase(0, pos + 1);
            }
            if (!presets.empty())
                modes.add(presets.c_str());
        }

        // Determine on command: prefer auto over medium (same logic as main fan midCmd)
        const char *onCmd = "medium";
        if (presetsStr)
        {
            if (strstr(presetsStr, "Auto") != nullptr)
                onCmd = "auto";
            else if (strstr(presetsStr, "Medium") != nullptr)
                onCmd = "medium";
        }

        // Payload for on/off
        componentJson["pl_on"] = (std::string("{\"") + cmdKey + "\":\"" + onCmd + "\",\"" + idxKey + "\":" + std::to_string(idx) + "}").c_str();
        componentJson["pl_off"] = (std::string("{\"") + cmdKey + "\":\"low\",\"" + idxKey + "\":" + std::to_string(idx) + "}").c_str();
    }
}

void generateHADiscoveryJson(JsonObject compactJson, JsonObject outputJson)
{
    JsonDocument statusitemsdoc;
    JsonObject statusitemsobj = statusitemsdoc.to<JsonObject>();
    int count = getIthoStatusJSON(statusitemsobj);

    char ihtostatustopic[140]{};
    char lwttopic[140]{};
    snprintf(ihtostatustopic, sizeof(ihtostatustopic), "%s%s", systemConfig.mqtt_base_topic, "/ithostatus");
    snprintf(lwttopic, sizeof(lwttopic), "%s%s", systemConfig.mqtt_base_topic, "/lwt");

    const char *deviceName = compactJson["d"]; // Retrieve device name from compact JSON

    // generic status topics, can be overiden per component
    outputJson["avty_t"] = lwttopic;
    outputJson["stat_t"] = ihtostatustopic; // state_topic

    // Static device information
    addHADevInfo(outputJson);

    // Update name from config
    outputJson["dev"]["name"] = deviceName;

    // Origin object
    JsonObject origin = outputJson["o"].to<JsonObject>();

    origin["name"] = deviceName;
    // origin["sw"] = fw_version;
    origin["sw"] = JsonString(fw_version, true);
    origin["url"] = "https://www.nrgwatch.nl/support-ondersteuning/";

    // Components object
    JsonObject components = outputJson["cmps"].to<JsonObject>();

    // Process each component from compact JSON
    JsonArray componentsArray = compactJson["c"].as<JsonArray>();

    if (!compactJson["sscnt"].isNull() && compactJson["sscnt"] != 0 && compactJson["sscnt"] == count)
    {
        for (JsonObject component : componentsArray)
        {
            uint8_t index = component["i"].as<uint8_t>();
            const char *name = component["n"];
            const char *platform = component["p"].is<const char *>() ? component["p"].as<const char *>() : "sensor";
            // Generate normalized unique ID
            std::string uniqueId = normalizeUniqueId(std::string(outputJson["dev"]["ids"] | "default_name") + "_" + platform + "_i_" + std::to_string(index));
            // Create component object using uniqueID
            JsonObject componentJson = components[uniqueId].to<JsonObject>();
            componentJson["name"] = name;
            componentJson["p"] = platform;
            componentJson["uniq_id"] = uniqueId; // Add unique_id
            if (component["vt"].isNull())
            {
                JsonObject::iterator it = statusitemsdoc.as<JsonObject>().begin();

                for (int i = 0; i < index; i++)
                    ++it;

                componentJson["val_tpl"] = ("{{ value_json['" + std::string(it->key().c_str()) + "'] }}");
            }
            else
            {
                componentJson["val_tpl"] = component["vt"].as<const char *>();
            }
            const char *unitOfMeasurement = component["um"].is<const char *>() ? component["um"].as<const char *>() : nullptr;
            const char *deviceClass = component["dc"].is<const char *>() ? component["dc"].as<const char *>() : nullptr;
            const char *stateClass = component["sc"].is<const char *>() ? component["sc"].as<const char *>() : nullptr;

            if (unitOfMeasurement)
            {
                componentJson["unit_of_meas"] = unitOfMeasurement;
            }

            if (deviceClass)
            {
                componentJson["dev_cla"] = deviceClass;
            }

            if (stateClass)
            {
                componentJson["stat_cla"] = stateClass; // Use "sc" from compact JSON
            }
        }
    }
    else if (compactJson["sscnt"].isNull() || compactJson["sscnt"] == 0)
    {
        // Auto-include speed label and FanInfo when no user config exists
        std::string speedLabel = getActualSpeedLabel();
        std::string fanInfoLabel = (systemConfig.api_normalize == 0) ? "FanInfo" : "fan-info";

        int idx = 0;
        for (auto kv : statusitemsobj)
        {
            std::string keyStr(kv.key().c_str());
            if ((!speedLabel.empty() && keyStr == speedLabel) || keyStr == fanInfoLabel)
            {
                std::string uniqueId = normalizeUniqueId(
                    std::string(outputJson["dev"]["ids"] | "default_name") + "_sensor_i_" + std::to_string(idx));
                JsonObject componentJson = components[uniqueId].to<JsonObject>();
                componentJson["name"] = kv.key().c_str();
                componentJson["p"] = "sensor";
                componentJson["uniq_id"] = uniqueId;
                componentJson["val_tpl"] = ("{{ value_json['" + keyStr + "'] }}");
            }
            idx++;
        }
    }
    else
    {
        if (compactJson["sscnt"] != 0)
            E_LOG("HAD: error - HA Discovery Config does not match no. of status items, please update the config HA Discovery config");
    }

    // Add extra components (fan and firmware updates)
    // Skip fan entity for WPU, Autotemp, and DemandFlow — not fully supported
    const uint8_t dg = currentIthoDeviceGroup();
    const uint8_t did = currentIthoDeviceID();
    bool skipFanEntity = (dg == 0x00 && did == 0x0D) ||                       // WPU
                         (dg == 0x00 && (did == 0x0F || did == 0x30)) ||       // Autotemp
                         (dg == 0x00 && did == 0x0B);                          // DemandFlow
    if (!skipFanEntity)
        addHADiscoveryFan(components, outputJson["dev"]["name"].as<const char *>());
    addHADiscoveryFWUpdate(components, outputJson["dev"]["name"].as<const char *>());

    // Add RF device sensors
    addHADiscoveryRFSensors(components, compactJson, outputJson["dev"]["name"].as<const char *>(), systemConfig.mqtt_base_topic);

    // Add RF device fans
    addHADiscoveryRemoteFan(components, compactJson, outputJson["dev"]["name"].as<const char *>(), systemConfig.mqtt_base_topic,
                            "rf", "rfremotecmd", "rfremoteindex", "rf", true);

    // Add virtual remote fans
    addHADiscoveryRemoteFan(components, compactJson, outputJson["dev"]["name"].as<const char *>(), systemConfig.mqtt_base_topic,
                            "vr", "vremotecmd", "vremoteindex", "vr", false);
}
