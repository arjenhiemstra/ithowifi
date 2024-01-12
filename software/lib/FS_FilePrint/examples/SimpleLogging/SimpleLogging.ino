#include <SpiffsFilePrint.h>  // https://github.com/PRosenb/SPIFFS_FilePrint

void setup() {
    Serial.begin(115200);
    Serial.println("-- start --");

    if (!SPIFFS.begin()) {
        Serial.println("SPIFFS Mount Failed, we format it");
        SPIFFS.format();
    }
    // SPIFFS.format();

    SpiffsFilePrint filePrint("/logfile", 2, 500, &Serial);
    filePrint.open();

    filePrint.print("Millis since start: ");
    filePrint.println(millis());

    filePrint.close();
}

void loop() {
    // put your main code here, to run repeatedly:
}
