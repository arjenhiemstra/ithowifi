#include <Arduino.h>
#include "ArduinoNvs.h"

/** 
 * Arduino NVS is a port for a non-volatile storage (NVS, flash) library for ESP32 to the Arduino Platform. 
 * It wraps main NVS functionality into the Arduino-styled C++ class. 
 */

bool res;

void setup() {   
    NVS.begin();
}


void loop() {	
    /*** Int ***/
    // write to flash
    const uint64_t ui64_set = 0x1122334455667788;
    res = NVS.setInt("longInt", ui64_set);
    // read from flash
    uint64_t uui64 = NVS.getInt("longInt");    


    /*** String ***/
    // write to flash
    const String st_set = "`simple plain string`";
    res = NVS.setString("st", st_set);
    // read from flash
    String st = NVS.getString("st");

    /*** BLOB ***/
    //write to flash
    uint8_t blolb_set[] = {1,2,3,99,100, 0xEE, 0xFE, 0xEE};
    res = NVS.setBlob("blob", blolb_set, sizeof(blolb_set));
    // read from flash
    size_t blobLength = NVS.getBlobSize("blob");    
    uint8_t blob[blobLength];
    res = NVS.getBlob("blob", blob, sizeof(blob));
    
    delay(1000);
}
