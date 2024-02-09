#include <Arduino.h>
#include "ArduinoNvs.h"

/** 
 * Arduino NVS is a port for a non-volatile storage (NVS, flash) library for ESP32 to the Arduino Platform. 
 * It wraps main NVS functionality into the Arduino-styled C++ class. 
 */

using namespace std;

#define PRINT(...)   Serial.print(__VA_ARGS__)
#define PRINTLN(...) Serial.println(__VA_ARGS__)
#define PRINTF(...)  Serial.printf(__VA_ARGS__)

#define DEBUG(...)   {Serial.print(__FUNCTION__);Serial.print("() ["); \
                     Serial.print(__FILE__); Serial.print(":");Serial.print(__LINE__); Serial.print("]:\t"); \
                     Serial.printf(__VA_ARGS__);Serial.println();}


void printHex(uint8_t* data, size_t len, String startLine = "\n", String endLine = "\n");
void printHex(vector<uint8_t>& data, String startLine = "\n", String endLine = "\n");

ArduinoNvs nvs;
ArduinoNvs nvss;

void setup() {
    Serial.begin(115200); 
   
    NVS.begin();
    nvs.begin("store");
    nvss.begin("upstore"); 
}

void loop() {
    if (Serial.available()) {
        char cmd = Serial.read();
        bool ok;
        switch (cmd) {
            case '1': {
                size_t blobL = NVS.getBlobSize("blob");
                DEBUG("1. Blob size: %d", blobL);
                uint8_t resp[blobL];
                ok = NVS.getBlob("blob", resp, sizeof(resp));
                DEBUG("1. NVS: res = %d, blob addr: %X, size %X", ok, (int32_t)&resp[0], sizeof(resp));
                printHex(resp, blobL);
            } break;
            case '2': {
                vector<uint8_t> respNvs;
                ok = nvs.getBlob("blob", respNvs);
                DEBUG("2. nvs: res = %d, blob addr: %X, size %X", ok, (int32_t)&respNvs[0], respNvs.size());
                printHex(respNvs);
            } break;
            case '3': {
                vector<uint8_t> respNvss;
                respNvss = nvss.getBlob("blob");
                DEBUG("3. nvss: blob addr: %X, size %X", (int32_t)&respNvss[0], respNvss.size());
                printHex(respNvss);
            } break;
            case '4': {            
                float ff = nvss.getFloat("fl");
                DEBUG("4. nvss: float: %f", ff);            
            } break;
            case '5': {            
                String stst = nvss.getString("st");
                DEBUG("5. nvss: stst %s", stst.c_str());            
            } break;
            case '6': {            
                uint64_t uui64 = nvss.getInt("ui64");
                DEBUG("6. nvss: ui64 %llu", uui64);
            } break;
            case '7': {            
                int64_t ii64 = nvss.getInt("i64");
                DEBUG("7. nvss: i64 %lli", ii64);            
            } break;
            case 'w': {
                bool res;
                NVS.setInt("u8",  (uint8_t)0xDEADBEEF);
                NVS.setInt("u16", (uint16_t)0xDEADBEEF);
                NVS.setInt("u32", (uint32_t)0xDEADBEEF);
                NVS.setString("str", "worked!");    
                uint8_t f[] = {1,2,3,99,100, 0xEE, 0xFE, 0xEE};
                res = NVS.setBlob("blob", f, sizeof(f));
                DEBUG("1. res setObject = %s", ((res)?"true":"false") );

                f[0] = 2;
                res = nvs.setBlob("blob", f, sizeof(f));
                DEBUG("2. res setObject = %s", ((res)?"true":"false") );

                f[sizeof(f)-1] = 0xAA;
                res = nvss.setBlob("blob", f, sizeof(f));
                DEBUG("3. res setObject = %s", ((res)?"true":"false") );    

                float fl = 0.12345678e5;            
                res = nvss.setFloat("fl", fl, false);
                DEBUG("4. fl res = %d", res);
                
                const char* st = "`ha\nha\nha\tha\tha`";
                res = nvss.setString("st", st, false);
                DEBUG("5. st res = %d", res);

                uint64_t ui64 = 0xFFEEAABBCCDDEEFF;
                res = nvss.setInt("ui64", ui64, false);
                DEBUG("6. st res = %d", res);
                
                res = nvss.setInt("i64", (int64_t)ui64, false);
                DEBUG("7. st res = %d", res);
                        
                
                res = nvss.commit();
                DEBUG("0. commit = %d", res);
            } break;
            case 'e': {
                bool res = NVS.eraseAll();
                DEBUG("NVS erase res = %d", res);
            } break;
            case 'r': {
                bool res = nvs.eraseAll();
                DEBUG("nvs erase res = %d", res);
            } break;
            case 't': {
                bool res = nvss.eraseAll();
                DEBUG("nvss erase res = %d", res);
            } break;

            case 'd': {
                bool res = nvss.erase("st");
                DEBUG("nvss st erase res = %d", res);
            } break;

            case 'k': {
                bool res;
                float fl = 0.87654321e5;            
                res = nvss.setFloat("fl", fl);
                DEBUG("4. fl res = %d", res);
                
                const char* st = "`simple plain string`";
                res = nvss.setString("st", st);
                DEBUG("5. st res = %d", res);

                uint64_t ui64 = 0x1122334455667788;
                res = nvss.setInt("ui64", ui64);
                DEBUG("6. st res = %d", res);
            } break;

        }
    }
}


void printHex(uint8_t* data, size_t len, String startLine, String endLine) {    
    PRINT(startLine);
    for (uint8_t i = 0; i < len; i++) {
        PRINTF("%02X,", data[i]);
    }
    PRINT(endLine);
}

void printHex(vector<uint8_t>& data, String startLine, String endLine) {
    printHex(&data[0], data.size(), startLine, endLine);
}