#include <string>

#include <Arduino.h>
#include "globals.h"

#include "WifiConfig.h"

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
} // WifiConfig

// default destructor
WifiConfig::~WifiConfig()
{
} //~WifiConfig

bool WifiConfig::set(JsonObjectConst obj)
{
  bool updated = false;
  // WIFI Settings parse
  if (!(const char *)obj["ssid"].isNull())
  {
    updated = true;
    strlcpy(ssid, obj["ssid"], sizeof(ssid));
  }
  if (!(const char *)obj["passwd"].isNull())
  {
    updated = true;
    strlcpy(passwd, obj["passwd"], sizeof(passwd));
  }
  if (!(const char *)obj["dhcp"].isNull())
  {
    updated = true;
    strlcpy(dhcp, obj["dhcp"], sizeof(dhcp));
  }
  if (!(const char *)obj["ip"].isNull())
  {
    updated = true;
    strlcpy(ip, obj["ip"], sizeof(ip));
  }
  if (!(const char *)obj["subnet"].isNull())
  {
    updated = true;
    strlcpy(subnet, obj["subnet"], sizeof(subnet));
  }
  if (!(const char *)obj["gateway"].isNull())
  {
    updated = true;
    strlcpy(gateway, obj["gateway"], sizeof(gateway));
  }
  if (!(const char *)obj["dns1"].isNull())
  {
    updated = true;
    strlcpy(dns1, obj["dns1"], sizeof(dns1));
  }
  if (!(const char *)obj["dns2"].isNull())
  {
    updated = true;
    strlcpy(dns2, obj["dns2"], sizeof(dns2));
  }
  if (!(const char *)obj["port"].isNull())
  {
    updated = true;
    port = obj["port"];
  }
  if (!(const char *)obj["hostname"].isNull())
  {
    updated = true;
    strlcpy(hostname, obj["hostname"], sizeof(hostname));
  }
  if (!(const char *)obj["ntpserver"].isNull())
  {
    updated = true;
    strlcpy(ntpserver, obj["ntpserver"], sizeof(ntpserver));
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

      StaticJsonDocument<512> root;
      JsonObject wifiscanresult = root.createNestedObject("wifiscanresult");
      wifiscanresult["id"] = i;
      wifiscanresult["ssid"] = ssid;
      wifiscanresult["bssid"] = bssid;
      wifiscanresult["sigval"] = signalStrengthResult;
      wifiscanresult["sec"] = sec;

      notifyClients(root.as<JsonObjectConst>());

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