#include "WifiConfig.h"
#include <string.h>
#include <Arduino.h>


// default constructor
WifiConfig::WifiConfig() {
  //default config
  strlcpy(config_struct_version, WIFI_CONFIG_VERSION, sizeof(config_struct_version));
  strlcpy(ssid, "", sizeof(ssid));
  strlcpy(passwd, "", sizeof(passwd));
  strlcpy(dhcp, "on", sizeof(dhcp));
  renew = 60;
  strlcpy(ip, "192.168.4.1", sizeof(ip));
  strlcpy(subnet, "255.255.255.0", sizeof(subnet));
  strlcpy(gateway, "127.0.0.1", sizeof(gateway));
  strlcpy(dns1, "8.8.8.8", sizeof(dns1));
  strlcpy(dns2, "8.8.4.4", sizeof(dns2));
  port = 80;
  strlcpy(hostname, "", sizeof(hostname));  
} //WifiConfig

// default destructor
WifiConfig::~WifiConfig() {
} //~WifiConfig


bool WifiConfig::set(JsonObjectConst obj) {
  bool updated = false;
  // WIFI Settings parse
  if (!(const char*)obj["ssid"].isNull()) {
    updated = true;
    strlcpy(ssid, obj["ssid"], sizeof(ssid));
  }
  if (!(const char*)obj["passwd"].isNull()) {
    updated = true;
    strlcpy(passwd, obj["passwd"], sizeof(passwd));
  }
  if (!(const char*)obj["dhcp"].isNull()) {
    updated = true;
    strlcpy(dhcp, obj["dhcp"], sizeof(dhcp));
  }
  if (!(const char*)obj["renew"].isNull()) {
    updated = true;
    renew = obj["renew"];
  }
  if (!(const char*)obj["ip"].isNull()) {
    updated = true;
    strlcpy(ip, obj["ip"], sizeof(ip));
  }
  if (!(const char*)obj["subnet"].isNull()) {
    updated = true;
    strlcpy(subnet, obj["subnet"], sizeof(subnet));
  }
  if (!(const char*)obj["gateway"].isNull()) {
    updated = true;
    strlcpy(gateway, obj["gateway"], sizeof(gateway));
  }
  if (!(const char*)obj["dns1"].isNull()) {
    updated = true;
    strlcpy(dns1, obj["dns1"], sizeof(dns1));
  }
  if (!(const char*)obj["dns2"].isNull()) {
    updated = true;
    strlcpy(dns2, obj["dns2"], sizeof(dns2));
  }
  if (!(const char*)obj["port"].isNull()) {
    updated = true;
    port = obj["port"];
  }
  if (!(const char*)obj["hostname"].isNull()) {
    updated = true;
    strlcpy(hostname, obj["hostname"], sizeof(hostname));
  }  
  return updated;
}

void WifiConfig::get(JsonObject obj) const {
  obj["ssid"] = ssid;
  obj["passwd"] = passwd;
  obj["dhcp"] = dhcp;
  obj["renew"] = renew;
  obj["ip"] = ip;
  obj["subnet"] = subnet;
  obj["gateway"] = gateway;
  obj["dns1"] = dns1;
  obj["dns2"] = dns2;
  obj["port"] = port;
  obj["hostname"] = hostname;
}
