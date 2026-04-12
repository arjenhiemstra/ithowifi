#pragma once

void MDNSinit();
bool prevlogAvailable();
const char *getCurrentLogPath();
const char *getPreviousLogPath();
const char *getContentType(bool download, const char *filename);
