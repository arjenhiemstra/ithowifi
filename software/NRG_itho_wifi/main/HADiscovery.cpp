#include <ArduinoJson.h>
#include <string>
// #include <algorithm>
// #include <cctype>

#include "IthoSystem.h"
#include "notifyClients.h"
#include "sys_log.h"
#include "generic_functions.h"

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
  dev["ids"] = hostName();             // identifiers
  dev["mf"] = "Arjen Hiemstra";        // manufacturer
  dev["mdl"] = "Wifi add-on for Itho"; // model
  dev["name"] = hostName();            // name
  dev["hw"] = hw_revision;             // hw_version
  dev["sw"] = fw_version;              // sw_version
  dev["cu"] = cu;                      // configuration_url
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
    JsonObject componentJson = obj[const_cast<char *>(uniqueId.c_str())].to<JsonObject>();
    componentJson["name"] = "Fan";
    componentJson["p"] = "fan";
    componentJson["uniq_id"] = const_cast<char *>(uniqueId.c_str());
    componentJson["json_attr_t"] = const_cast<char *>(ihtostatustopic);                // json_attributes_topic
    componentJson["cmd_t"] = const_cast<char *>(cmdtopic);                             // command_topic
    componentJson["pct_cmd_t"] = const_cast<char *>(cmdtopic);                         // percentage_command_topic
    componentJson["pct_stat_t"] = const_cast<char *>(statetopic);                      // percentage_state_topic
    componentJson["stat_val_tpl"] = "{% if value == '0' %}OFF{% else %}ON{% endif %}"; // state_value_template
    componentJson["pr_mode_cmd_t"] = const_cast<char *>(cmdtopic);                     // preset_mode_command_topic
    componentJson["pr_mode_stat_t"] = const_cast<char *>(ihtostatustopic);             // preset_mode_state_topic

    JsonArray modes = componentJson["pr_modes"].to<JsonArray>(); // preset_modes
    modes.add("Low");
    modes.add("Medium");
    modes.add("High");
    modes.add("Auto");
    modes.add("AutoNight");
    modes.add("Timer 10min");
    modes.add("Timer 20min");
    modes.add("Timer 30min");

    std::string actualSpeedLabel;

    const uint8_t deviceID = currentIthoDeviceID();
    // const uint8_t version = currentItho_fwversion();
    const uint8_t deviceGroup = currentIthoDeviceGroup();

    char pr_mode_val_tpl[400]{};
    char pct_cmd_tpl[300]{};
    char pct_val_tpl[100]{};
    int pr_mode_val_tpl_ver = 0;

    if (deviceGroup == 0x07 && deviceID == 0x01) // HRU250-300
    {
        actualSpeedLabel = getStatusLabel(10, ithoDeviceptr);                         //-> {"Absolute speed of the fan (%)", "absolute-speed-of-the-fan_perc"}, of hru250_300.h
        componentJson["pr_mode_cmd_tpl"] = "{\"rfremotecmd\":\"{{value.lower()}}\"}"; // preset_mode_command_template

        strncpy(pct_cmd_tpl, "{%% if value > 90 %%}{\"rfremotecmd\":\"high\"}{%% elif value > 40 %%}{\"rfremotecmd\":\"medium\"}{%% elif value > 20 %%}{\"rfremotecmd\":\"low\"}{%% else %%}{\"rfremotecmd\":\"auto\"}{%% endif %%}", sizeof(pct_cmd_tpl));

        snprintf(pct_val_tpl, sizeof(pct_val_tpl), "{{ value_json['%s'] | int }}", actualSpeedLabel.c_str());
        componentJson["pl_off"] = "{\"rfremotecmd\":\"auto\"}"; // payload_off
        pr_mode_val_tpl_ver = 1;
    }
    else if (deviceGroup == 0x00 && deviceID == 0x03) // HRU350
    {
        actualSpeedLabel = getStatusLabel(0, ithoDeviceptr);                         //-> {"Requested fanspeed (%)", "requested-fanspeed_perc"}, of hru350.h
        componentJson["pr_mode_cmd_tpl"] = "{\"vremotecmd\":\"{{value.lower()}}\"}"; // preset_mode_command_template
        strncpy(pct_cmd_tpl, "{%% if value > 90 %%}{\"vremotecmd\":\"high\"}{%% elif value > 40 %%}{\"vremotecmd\":\"medium\"}{%% elif value > 20 %%}{\"vremotecmd\":\"low\"}{%% else %%}{\"vremotecmd\":\"auto\"}{%% endif %%}", sizeof(pct_cmd_tpl));

        snprintf(pct_val_tpl, sizeof(pct_val_tpl), "{{ value_json['%s'] | int }}", actualSpeedLabel.c_str());
        componentJson["pl_off"] = "{\"vremotecmd\":\"auto\"}"; // payload_off
        pr_mode_val_tpl_ver = 1;
    }
    else if (deviceGroup == 0x00 && deviceID == 0x0D) // WPU
    {
        // tbd
    }
    else if (deviceGroup == 0x00 && (deviceID == 0x0F || deviceID == 0x30)) // Autotemp
    {
        // tbd
    }
    else if (deviceGroup == 0x00 && deviceID == 0x0B) // DemandFlow
    {
        // tbd
    }
    else if ((deviceGroup == 0x00 && (deviceID == 0x1D || deviceID == 0x14 || deviceID == 0x1B)) || systemConfig.itho_pwm2i2c != 1) // assume CVE and HRU200 / or PWM2I2C is off
    {
        if (deviceID == 0x1D) // hru200
        {
            actualSpeedLabel = getStatusLabel(0, ithoDeviceptr); //-> {"Ventilation setpoint (%)", "ventilation-setpoint_perc"}, of hru200.h
        }
        else if (deviceID == 0x14) // cve 0x14
        {
            actualSpeedLabel = getStatusLabel(0, ithoDeviceptr); //-> {"Ventilation level (%)", "ventilation-level_perc"}, of cve14.h
        }
        else if (deviceID == 0x1B) // cve 0x1B
        {
            actualSpeedLabel = getStatusLabel(0, ithoDeviceptr); //-> {"Ventilation setpoint (%)", "ventilation-setpoint_perc"}, of cve1b.h
        }
        if (systemConfig.itho_pwm2i2c != 1)
        {
            pr_mode_val_tpl_ver = 1;
            snprintf(pct_val_tpl, sizeof(pct_val_tpl), "{{ value_json['%s'] | int }}", actualSpeedLabel.c_str());
            strncpy(pct_cmd_tpl, "{% if value > 90 %}{\"vremotecmd\":\"high\"}{% elif value > 40 %}{ \"vremotecmd\":\"medium\"}{% elif value > 20 %}{\"vremotecmd\":\"low\"}{% else %}{\"vremotecmd\":\"auto\"}{% endif %}", sizeof(pct_cmd_tpl));
            componentJson["pr_mode_cmd_tpl"] = "{\"vremotecmd\":\"{{value.lower()}}\"}"; // preset_mode_command_template
            componentJson["pl_off"] = "{\"vremotecmd\":\"auto\"}";                       // payload_off
        }
        else
        {
            strncpy(pct_val_tpl, "{{ (value | int / 2.55) | round | int }}", sizeof(pct_val_tpl));
            strncpy(pct_cmd_tpl, "{{ (value | int * 2.55) | round | int }}", sizeof(pct_cmd_tpl));
            componentJson["pl_off"] = "0"; // payload_off
        }
    }

    if (pr_mode_val_tpl_ver == 0)
    {
        componentJson["pr_mode_cmd_tpl"] = "{%- if value == 'Timer 10min' %}{{'timer1'}}{%- elif value == 'Timer 20min' %}{{'timer2'}}{%- elif value == 'Timer 30min' %}{{'timer3'}}{%- else %}{{value.lower()}}{%- endif -%}";
        // snprintf(pr_mode_val_tpl, sizeof(pr_mode_val_tpl), "{%%- set speed = value_json['%s'] | int %%}{%%- if speed > 219 %%}high{%%- elif speed > 119 %%}medium{%%- elif speed > 19 %%}low{%%- else %%}auto{%%- endif -%%}", actualSpeedLabel.c_str());
        snprintf(pr_mode_val_tpl, sizeof(pr_mode_val_tpl), "{%%- set speed = value_json['%s'] | int %%}{%%- if speed > 90 %%}High{%%- elif speed > 35 %%}Medium{%%- elif speed > 10 %%}Low{%%- else %%}Auto{%%- endif -%%}", actualSpeedLabel.c_str());

        // strncpy(pr_mode_val_tpl, "{%- if value == 'Low' %}{{'low'}}{%- elif value == 'Medium' %}{{'medium'}}{%- elif value == 'High' %}{{'high'}}{%- elif value == 'Auto' %}{{'auto'}}{%- elif value == 'AutoNight' %}{{'autonight'}}{%- elif value == 'Timer 10min' %}{{'timer1'}}{%- elif value == 'Timer 20min' %}{{'timer2'}}{%- elif value == 'Timer 30min' %}{{'timer3'}}{%- endif -%}", sizeof(pr_mode_val_tpl));
    }
    else if (pr_mode_val_tpl_ver == 1)
    {
        snprintf(pr_mode_val_tpl, sizeof(pr_mode_val_tpl), "{%%- set speed = value_json['%s'] | int %%}{%%- if speed > 90 %%}High{%%- elif speed > 35 %%}Medium{%%- elif speed > 10 %%}Low{%%- else %%}Auto{%%- endif -%%}", actualSpeedLabel.c_str());
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

    JsonDocument fwDoc;
    JsonObject fwObj = fwDoc.to<JsonObject>();

    get_firmware_info(fwObj);

    std::string uniqueId = normalizeUniqueId(std::string(name) + "_fwupdate");
    JsonObject componentJson = obj[const_cast<char *>(uniqueId.c_str())].to<JsonObject>();
    componentJson["name"] = "Firmware Update";
    componentJson["p"] = "update";
    componentJson["uniq_id"] = const_cast<char *>(uniqueId.c_str());
    componentJson["dev_cla"] = "firmware";

    std::string tmpstr = "http://" + std::string(WiFi.localIP().toString().c_str()) + "/favicon.png";
    componentJson["ent_pic"] = const_cast<char *>(tmpstr.c_str());
    // componentJson["icon"] = "mdi:update";

    componentJson["stat_t"] = ithodevicetopic;

    tmpstr = "{{ {'installed_version': value_json['add-on_fwversion'], 'latest_version': value_json['add-on_latest_fw'], 'title': 'Add-on Firmware', 'release_url': 'https://github.com/arjenhiemstra/ithowifi/releases/tag/Version-" + std::string(firmwareInfo.latest_fw) + "' } | to_json }}";
    componentJson["val_tpl"] = const_cast<char *>(tmpstr.c_str());
}

void generateHADiscoveryJson(JsonObject compactJson)
{

    // Example output
    //   {
    //    "dev": {
    //      "ids": "ea334450945afc",
    //      "name": "Kitchen",
    //      "mf": "Bla electronics",
    //      "mdl": "xya",
    //      "sw": "1.0",
    //      "sn": "ea334450945afc",
    //      "hw": "1.0rev2",
    //    },
    //    "o": {
    //      "name":"bla2mqtt",
    //      "sw": "2.1",
    //      "url": "https://bla2mqtt.example.com/support",
    //    },
    //    "cmps": {
    //      "some_unique_component_id1": {
    //        "p": "sensor",
    //        "device_class":"temperature",
    //        "unit_of_measurement":"°C",
    //        "value_template":"{{ value_json.temperature}}",
    //        "unique_id":"temp01ae_t",
    //      },
    //      "some_unique_id2": {
    //        "p": "sensor",
    //        "device_class":"humidity",
    //        "unit_of_measurement":"%",
    //        "value_template":"{{ value_json.humidity}}",
    //        "unique_id":"temp01ae_h",
    //      }
    //    },
    //    "stat_t": "ithocve/ithostatus",
    //    "avty_t": "ithocve/lwt",
    //    "qos": 2,
    //  }

    JsonDocument statusitemsdoc;
    JsonObject statusitemsobj = statusitemsdoc.to<JsonObject>();
    int count = getIthoStatusJSON(statusitemsobj);

    std::string outputbuf;
    JsonDocument outputDoc;
    JsonObject output = outputDoc.to<JsonObject>();

    char ihtostatustopic[140]{};
    char lwttopic[140]{};
    snprintf(ihtostatustopic, sizeof(ihtostatustopic), "%s%s", systemConfig.mqtt_base_topic, "/ithostatus");
    snprintf(lwttopic, sizeof(lwttopic), "%s%s", systemConfig.mqtt_base_topic, "/lwt");

    const char *deviceName = compactJson["d"]; // Retrieve device name from compact JSON

    // generic status topics, can be overiden per component
    output["avty_t"] = const_cast<char *>(lwttopic);
    output["stat_t"] = const_cast<char *>(ihtostatustopic); // state_topic

    // Static device information
    addHADevInfo(output);
    // Update name from config FIXME add config value
    output["dev"]["name"] = const_cast<char *>(deviceName);

    // Origin object
    JsonObject origin = output["o"].to<JsonObject>();

    origin["name"] = const_cast<char *>(deviceName);
    origin["sw"] = fw_version;
    origin["url"] = "https://www.nrgwatch.nl/support-ondersteuning/";

    // Components object
    JsonObject components = output["cmps"].to<JsonObject>();

    // Process each component from compact JSON
    JsonArray componentsArray = compactJson["c"].as<JsonArray>();

    if (!compactJson["sscnt"].isNull() && compactJson["sscnt"] != 0 && compactJson["sscnt"] == count)
    {
        for (JsonObject component : componentsArray)
        {
            // each object needs to be copied into the JSON to prevent dangling pointers to be inserted, therefor const_cast<char *>() is applied
            uint8_t index = component["i"].as<uint8_t>();

            const char *name = component["n"];
            const char *platform = component["p"].is<const char *>() ? component["p"].as<const char *>() : "sensor";
            // Generate normalized unique ID
            std::string uniqueId = normalizeUniqueId(std::string(output["dev"]["ids"] | "default_name") + "_" + platform + "_i_" + std::to_string(index));
            // Create component object using uniqueID
            JsonObject componentJson = components[const_cast<char *>(uniqueId.c_str())].to<JsonObject>();
            componentJson["name"] = const_cast<char *>(name);
            componentJson["p"] = const_cast<char *>(platform);
            componentJson["uniq_id"] = const_cast<char *>(uniqueId.c_str()); // Add unique_id
            if (component["vt"].isNull())
            {
                JsonObject::iterator it = statusitemsdoc.as<JsonObject>().begin();

                for (int i = 0; i < index; i++)
                    ++it;

                componentJson["val_tpl"] = ("{{ value_json['" + std::string(it->key().c_str()) + "'] }}");
            }
            else
            {
                componentJson["val_tpl"] = const_cast<char *>(component["vt"].as<const char *>());
            }
            const char *unitOfMeasurement = component["um"].is<const char *>() ? component["um"].as<const char *>() : nullptr;
            const char *deviceClass = component["dc"].is<const char *>() ? component["dc"].as<const char *>() : nullptr;

            if (unitOfMeasurement)
            {
                componentJson["unit_of_meas"] = const_cast<char *>(unitOfMeasurement);
            }

            if (deviceClass)
            {
                componentJson["dev_cla"] = const_cast<char *>(deviceClass);
            }
        }
    }
    else
    {
        E_LOG("Error: HA Discovery Config does not match no. of status items, please update the config HA Discovery config");
    }
    addHADiscoveryFan(components, output["dev"]["name"].as<const char *>());
    addHADiscoveryFWUpdate(components, output["dev"]["name"].as<const char *>());

    char devicetopic[160]{};

    snprintf(devicetopic, sizeof(devicetopic), "%s%s%s%s", systemConfig.mqtt_ha_topic, "/device/", hostName(), "/config");

    if (outputDoc.overflowed())
        D_LOG("generateHADiscoveryJson overflowed!");
    sendHADiscovery(output, devicetopic);
}