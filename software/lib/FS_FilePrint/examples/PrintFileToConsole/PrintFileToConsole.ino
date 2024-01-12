#include <SpiffsFilePrint.h>  // https://github.com/PRosenb/SPIFFS_FilePrint

void printFileToConsole(String filename) {
    Serial.println(filename);
    File file = SPIFFS.open(filename, FILE_READ);
    while (file.available()) {
        Serial.write(file.read());
    }
    Serial.println();
    file.close();
}

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

    // check the result
    printFileToConsole("/logfile0.current.log");
    printFileToConsole("/logfile0.log");
    printFileToConsole("/logfile1.current.log");
    printFileToConsole("/logfile1.log");
}

void loop() {
    // put your main code here, to run repeatedly:
}
