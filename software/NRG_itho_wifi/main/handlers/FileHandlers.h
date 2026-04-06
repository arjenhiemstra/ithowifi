#pragma once

void handleCoreCrash(AsyncWebServerRequest *request);
void handleCoredumpDownload(AsyncWebServerRequest *request);
void handleCurLogDownload(AsyncWebServerRequest *request);
void handlePrevLogDownload(AsyncWebServerRequest *request);
void handleFileCreate(AsyncWebServerRequest *request);
void handleFileDelete(AsyncWebServerRequest *request);
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
bool handleFileRead(AsyncWebServerRequest *request);
void handleStatus(AsyncWebServerRequest *request);
void handleFileList(AsyncWebServerRequest *request);
