#include <ArduinoLog.h>       // https://github.com/thijse/Arduino-Log
#include <SpiffsFilePrint.h>  // https://github.com/PRosenb/SPIFFS_FilePrint

void printTimestamp(Print* _logOutput) {
#if defined(ESP32)
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 0)) {
        char timeStringBuff[50];  // 50 chars should be enough
        strftime(timeStringBuff, sizeof(timeStringBuff), "%F %T ", &timeinfo);
        _logOutput->print(timeStringBuff);
    } else
#endif
    {
        char c[12];
        sprintf(c, "%10lu ", millis());
        _logOutput->print(c);
    }
}

void printNewline(Print* _logOutput) { _logOutput->print('\n'); }

#if defined(ESP32)
void listRootDir() {
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    Serial.println("All files in /:");
    while (file) {
        Serial.print(file.name());
        Serial.print(" ");
        Serial.print(file.size());
        Serial.println(" Bytes");
        file = root.openNextFile();
    }
    Serial.println("------------");
}
#elif defined(ESP8266)
void listRootDir() {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
        if (dir.fileSize()) {
            File file = dir.openFile(FILE_READ);
            Serial.print(file.name());
            Serial.print(" ");
            Serial.print(file.size());
            Serial.println(" Bytes");
        }
    }
    Serial.println("------------");
}
#else
#error "Not supported CPU"
#endif

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

    Log.begin(LOG_LEVEL_NOTICE, &filePrint);
    Log.setPrefix(printTimestamp);
    Log.setSuffix(printNewline);
    Log.notice("Millis since start: %d", millis());

    filePrint.close();

    // check the result
    listRootDir();

    printFileToConsole("/logfile0.current.log");
    printFileToConsole("/logfile0.log");
    printFileToConsole("/logfile1.current.log");
    printFileToConsole("/logfile1.log");
}

void loop() {
    // put your main code here, to run repeatedly:
}
