Arduino NVS Library
==========================

## Summary
Arduino NVS is a port for a non-volatile storage (NVS, flash) [library](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/storage/nvs_flash.html) for ESP32 to the Arduino Platform. It wraps main NVS functionality into the Arduino-styled C++ class.
This lib is inspired and based on [TridentTD_ESP32NVS](https://github.com/TridentTD/TridentTD_ESP32NVS)  work. This lib is a further development of it, contains a lot of improvements, bugfixes and translation (original text was published on [Thai](https://en.wikipedia.org/wiki/Thai_language)).



## Introduction

NVS lib (commonly mentioned as *"flash lib"*) is a library used for storing data  values in the flash memory in the ESP32. Data are stored in a non-volatile manner, so it is remaining in the memory after power-out or reboot of the ESP32.

The ESP32 NVS stored data in the form of key-value. Keys are ASCII strings, up to 15 characters. Values can have one of the following types:

- integer types: `uint8_t`, `int8_t`, `uint16_t`, `int16_t`, `uint32_t`, `int32_t`, `uint64_t`, `int64_t`
- zero-terminated String (arduino) or std::string
- variable length binary data (blob)

Refer to the NVS ESP32 lib [original documentation](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/storage/nvs_flash.html#internals) for a details about internal NVS lib organization.



## Usage

Include the library header: ` #include <ArduinoNvs.h>` 

Then init the NVS with the command: `NVS.begin();`

  

### Save data in NVS


```c++
bool ok; 
ok = NVS.setInt ("myInt", 23); // Stores the integer value 23 into the key named "myInt" on the NVS

String data = "Hello String";
ok = NVS.setString ("myString", data); // Store the data value into the key named "myString" on the NVS
```


### Load data from NVS

```c++
int i = NVS.getInt ("myInt"); // Read the value of the key "myInt" from the NVS

String str = NVS.getString ("myString"); // Read the value of the "myString" key from the NVS 
```


### Binary-like Objects ###

Storing  variables other than the basic types are performed through serializing it to the blob/array form (std::vector<uint8_t> is recommended). 

#### Store blob
```c++
uint8_t mac [6] = {0xDF, 0xEE, 0x10, 0x49, 0xA1, 0x42};
bool ok = NVS.setBlob("mac", f, sizeof(f)); // store mac [6] to key "MAC" on NVS
```

#### Load blob
```c++
size_t blobLength = NVS.getBlobSize("mac"); // retrievenig size of stored blob
uint8_t mymac[blobLength];
bool ok = NVS.getBlob("mac", mymac, sizeof(mymac));
Serial.printf ("mac:% 02X:% 02X:% 02X:% 02X:% 02X:% 02X \ n",
               mymac [1], mymac [2], mymac [3], mymac [4], mymac [5]);              
```



## Namespaces

ArduinoNvs Library supports ESP32 [Namespaces](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/storage/nvs_flash.html#namespaces)

The default namespace is `"storage"`. To store data in the separate namaspace, use its name in the `begin()` parameter:

```c++
ArduinoNvs mynvs;
mynvs.begin("customNs");
```

All subsequent store/load  with `mynvs` object  will be performed in `"customNs"` namespace:
```c++
String dataSt = "Namespace String";
mynvs.setString ("myString", dataSt); // this string is stored in "customNs" namespace
```

## Authors
1. Arjen Hiemstra (added std::string support and removed unnecessary String() usage)
2. dRKr, Sinai RnD (<info@sinai.io>) (https://github.com/rpolitex/ArduinoNvs)
3. (original version) TridentTD (https://github.com/TridentTD/)