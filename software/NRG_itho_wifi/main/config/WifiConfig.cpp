#include <string>

#include <Arduino.h>
#include "globals.h"

#include "config/WifiConfig.h"

WifiConfig wifiConfig;

// default constructor
WifiConfig::WifiConfig()
{
  // default config
  strlcpy(config_struct_version, WIFI_CONFIG_VERSION, sizeof(config_struct_version));
  strlcpy(ssid, "", sizeof(ssid));
  strlcpy(passwd, "", sizeof(passwd));
  strlcpy(dhcp, "on", sizeof(dhcp));
  strlcpy(ip, "192.168.4.1", sizeof(ip));
  strlcpy(subnet, "255.255.255.0", sizeof(subnet));
  strlcpy(gateway, "127.0.0.1", sizeof(gateway));
  strlcpy(dns1, "8.8.8.8", sizeof(dns1));
  strlcpy(dns2, "8.8.4.4", sizeof(dns2));
  port = 80;
  strlcpy(hostname, "", sizeof(hostname));
  strlcpy(ntpserver, "pool.ntp.org", sizeof(ntpserver));
  strlcpy(timezone, "Europe/Amsterdam", sizeof(timezone));

  configLoaded = false;

} // WifiConfig

// default destructor
WifiConfig::~WifiConfig()
{
} //~WifiConfig

const WifiConfig::wl_status_msg WifiConfig::wl_status_msg_table[]{
    {WL_NO_SHIELD, "WL_NO_SHIELD"},
    {WL_IDLE_STATUS, "WL_IDLE_STATUS"},
    {WL_NO_SSID_AVAIL, "WL_NO_SSID_AVAIL"},
    {WL_SCAN_COMPLETED, "WL_SCAN_COMPLETED"},
    {WL_CONNECTED, "WL_CONNECTED"},
    {WL_CONNECT_FAILED, "WL_CONNECT_FAILED"},
    {WL_CONNECTION_LOST, "WL_CONNECTION_LOST"},
    {WL_DISCONNECTED, "WL_DISCONNECTED"}};

const char *WifiConfig::wl_unknown_msg = "UNKNOWN ERROR";

const WifiConfig::tzLabels WifiConfig::tzlabels[]{
    {"Europe/Amsterdam", 0},
    {"Europe/Andorra", 0},
    {"Europe/Astrakhan", 1},
    {"Europe/Athens", 2},
    {"Europe/Belgrade", 0},
    {"Europe/Berlin", 0},
    {"Europe/Bratislava", 0},
    {"Europe/Brussels", 0},
    {"Europe/Bucharest", 2},
    {"Europe/Budapest", 0},
    {"Europe/Busingen", 0},
    {"Europe/Chisinau", 3},
    {"Europe/Copenhagen", 0},
    {"Europe/Dublin", 4},
    {"Europe/Gibraltar", 0},
    {"Europe/Guernsey", 5},
    {"Europe/Helsinki", 2},
    {"Europe/Isle_of_Man", 5},
    {"Europe/Istanbul", 6},
    {"Europe/Jersey", 5},
    {"Europe/Kaliningrad", 7},
    {"Europe/Kyiv", 2},
    {"Europe/Kirov", 6},
    {"Europe/Lisbon", 8},
    {"Europe/Ljubljana", 0},
    {"Europe/London", 5},
    {"Europe/Luxembourg", 0},
    {"Europe/Madrid", 0},
    {"Europe/Malta", 0},
    {"Europe/Mariehamn", 2},
    {"Europe/Minsk", 6},
    {"Europe/Monaco", 0},
    {"Europe/Moscow", 9},
    {"Europe/Oslo", 0},
    {"Europe/Paris", 0},
    {"Europe/Podgorica", 0},
    {"Europe/Prague", 0},
    {"Europe/Riga", 2},
    {"Europe/Rome", 0},
    {"Europe/Samara", 1},
    {"Europe/San_Marino", 0},
    {"Europe/Sarajevo", 0},
    {"Europe/Saratov", 1},
    {"Europe/Simferopol", 9},
    {"Europe/Skopje", 0},
    {"Europe/Sofia", 2},
    {"Europe/Stockholm", 0},
    {"Europe/Tallinn", 2},
    {"Europe/Tirane", 0},
    {"Europe/Ulyanovsk", 1},
    {"Europe/Uzhgorod", 2},
    {"Europe/Vaduz", 0},
    {"Europe/Vatican", 0},
    {"Europe/Vienna", 0},
    {"Europe/Vilnius", 2},
    {"Europe/Volgograd", 6},
    {"Europe/Warsaw", 0},
    {"Europe/Zagreb", 0},
    {"Europe/Zaporizhzhia", 2},
    {"Europe/Zurich", 0},
    {"Etc/Greenwich", 10},
    {"Etc/Universal", 11}};

const char WifiConfig::tz_strings[][30] =
    {"CET-1CEST,M3.5.0,M10.5.0/3",
     "<+04>-4",
     "EET-2EEST,M3.5.0/3,M10.5.0/4",
     "EET-2EEST,M3.5.0,M10.5.0/3",
     "IST-1GMT0,M10.5.0,M3.5.0/1",
     "GMT0BST,M3.5.0/1,M10.5.0",
     "<+03>-3",
     "EET-2",
     "WET0WEST,M3.5.0/1,M10.5.0",
     "MSK-3",
     "GMT0",
     "UTC0"};

bool WifiConfig::set(JsonObject obj)
{
  bool updated = false;
  // WIFI Settings parse
  if (!obj["ssid"].isNull())
  {
    updated = true;
    strlcpy(ssid, obj["ssid"] | "", sizeof(ssid));
  }
  if (!obj["passwd"].isNull())
  {
    updated = true;
    strlcpy(passwd, obj["passwd"] | "", sizeof(passwd));
  }
  if (!obj["dhcp"].isNull())
  {
    updated = true;
    strlcpy(dhcp, obj["dhcp"] | "", sizeof(dhcp));
  }
  if (!obj["ip"].isNull())
  {
    updated = true;
    strlcpy(ip, obj["ip"] | "", sizeof(ip));
  }
  if (!obj["subnet"].isNull())
  {
    updated = true;
    strlcpy(subnet, obj["subnet"] | "", sizeof(subnet));
  }
  if (!obj["gateway"].isNull())
  {
    updated = true;
    strlcpy(gateway, obj["gateway"] | "", sizeof(gateway));
  }
  if (!obj["dns1"].isNull())
  {
    updated = true;
    strlcpy(dns1, obj["dns1"] | "", sizeof(dns1));
  }
  if (!obj["dns2"].isNull())
  {
    updated = true;
    strlcpy(dns2, obj["dns2"] | "", sizeof(dns2));
  }
  if (!obj["port"].isNull())
  {
    updated = true;
    port = obj["port"];
  }
  if (!obj["hostname"].isNull())
  {
    updated = true;
    strlcpy(hostname, obj["hostname"] | "", sizeof(hostname));
  }
  if (!obj["ntpserver"].isNull())
  {
    updated = true;
    strlcpy(ntpserver, obj["ntpserver"] | "", sizeof(ntpserver));
  }
  if (!obj["timezone"].isNull())
  {
    updated = true;
    strlcpy(timezone, obj["timezone"] | "", sizeof(timezone));
  }
  return updated;
}

void WifiConfig::get(JsonObject obj) const
{
  obj["ssid"] = ssid;
  obj["passwd"] = passwd;
  obj["dhcp"] = dhcp;
  obj["ip"] = ip;
  obj["subnet"] = subnet;
  obj["gateway"] = gateway;
  obj["dns1"] = dns1;
  obj["dns2"] = dns2;
  obj["port"] = port;
  obj["hostname"] = hostname;
  obj["ntpserver"] = ntpserver;
  obj["timezone"] = timezone;
}

const char *WifiConfig::wl_status_to_name(wl_status_t code) const
{
  size_t i;

  for (i = 0; i < sizeof(wl_status_msg_table) / sizeof(wl_status_msg_table[0]); ++i)
  {
    if (wl_status_msg_table[i].code == code)
    {
      return wl_status_msg_table[i].msg;
    }
  }
  return wl_unknown_msg;
}

const char *WifiConfig::getTimeZoneStr() const
{

  const char *tz_string = tz_strings[0]; // default tz string

  const char *target = timezone; // the timezone we want to find
  for (const auto &label : tzlabels)
  {
    if (std::strcmp(target, label.tzlabel) == 0)
    {
      // the timezone matches, so we've found the corresponding index
      tz_string = tz_strings[static_cast<int>(label.tzindex)];
    }
  }
  return tz_string;
}

void wifiScan()
{

  int n = WiFi.scanComplete();
  if (n == -2)
  {
    WiFi.scanNetworks(true);
  }
  else if (n)
  {
    char logBuff[LOG_BUF_SIZE]{};
    snprintf(logBuff, sizeof(logBuff), "Wifi scan found %d networks", n);
    logMessagejson(logBuff, WEBINTERFACE);

    // sort networks
    int indices[n];
    for (int i = 0; i < n; i++)
    {
      indices[i] = i;
    }

    for (int i = 0; i < n; i++)
    {
      for (int j = i + 1; j < n; j++)
      {
        if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i]))
        {
          std::swap(indices[i], indices[j]);
        }
      }
    }

    // send scan result to DOM
    for (int i = 0; i < n; i++)
    {

      int sec = 0;
      switch (WiFi.encryptionType(indices[i]))
      {
      case WIFI_AUTH_OPEN:
        sec = 1; // open network
        break;
      case WIFI_AUTH_WEP:
      case WIFI_AUTH_WPA_PSK:
      case WIFI_AUTH_WPA2_PSK:
      case WIFI_AUTH_WPA_WPA2_PSK:
      case WIFI_AUTH_WPA2_ENTERPRISE:
      case WIFI_AUTH_MAX:
        sec = 2; // closed network
        break;
      default:
        sec = 0; // unknown network encryption
      }

      int32_t signalStrength = WiFi.RSSI(indices[i]);
      uint8_t signalStrengthResult = 0;

      if (signalStrength > -65)
      {
        signalStrengthResult = 5;
      }
      else if (signalStrength > -75)
      {
        signalStrengthResult = 4;
      }
      else if (signalStrength > -80)
      {
        signalStrengthResult = 3;
      }
      else if (signalStrength > -90)
      {
        signalStrengthResult = 2;
      }
      else if (signalStrength > -100)
      {
        signalStrengthResult = 1;
      }
      else
      {
        signalStrengthResult = 0;
      }

      char ssid[33]{}; // ssid can be up to 32chars, => plus null term
      char bssid[18]{};
      strlcpy(ssid, WiFi.SSID(indices[i]).c_str(), sizeof(ssid));
      strlcpy(bssid, WiFi.BSSIDstr(indices[i]).c_str(), sizeof(bssid));

      JsonDocument root;
      JsonObject wifiscanresult = root["wifiscanresult"].to<JsonObject>();
      wifiscanresult["id"] = i;
      wifiscanresult["ssid"] = ssid;
      wifiscanresult["bssid"] = bssid;
      wifiscanresult["sigval"] = signalStrengthResult;
      wifiscanresult["sec"] = sec;

      notifyClients(root.as<JsonObject>());

      delay(25);
    }
    WiFi.scanDelete();
    if (WiFi.scanComplete() == -2)
    {
      WiFi.scanNetworks(true);
    }
  }
}

void logWifiInfo()
{
  // const char* const phymodes[] = { "", "B", "G", "N" };
  const char *const modes[] = {"NULL", "STA", "AP", "STA+AP"};
  IPAddress ip = WiFi.localIP();

  N_LOG("WiFi: connection successful");
  I_LOG("WiFi info:");
  I_LOG("  SSID:%s | BSSID[%s]", WiFi.SSID().c_str(), WiFi.BSSIDstr().c_str());
  I_LOG("  RSSI:%ddBm", WiFi.RSSI());
  I_LOG("  Mode:%s", modes[WiFi.getMode()]);
  I_LOG("  Status:%s", wifiConfig.wl_status_to_name(WiFi.status()));
  I_LOG("  IP:%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}
