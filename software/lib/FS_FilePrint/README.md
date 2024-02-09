# Arduino FS FilePrint Library #
<https://github.com/jclds139/FS_FilePrint>

FS FilePrint is a lightweight, simple library for rolling file print to log to any FS object.  

## Features ##
- Easy to use
- Supports rolling files
- Lightweight
- Buffers writes to reduce flash wear, writing on newlines or full buffers

## Installation ##
### Manual ###
  - [Download the latest version](https://github.com/jclds139/FS_FilePrint/)
  - Uncompress the downloaded file
  - This will result in a folder containing all the files for the library. The folder name includes the version: **FS_FilePrint-x.y.z**
  - Rename the folder to **FS_FilePrint**
  - Copy the renamed folder to your **libraries** folder
  - From time to time, check on <https://github.com/jclds139/FS_FilePrint> if updates become available

## Getting Started ##
Simple Logger:
```c++
#include <FSFilePrint.h>
#include <FS.h>

void setup() {
    Serial.begin(115200);

    if (!SPIFFS.begin()) {
        Serial.println("SPIFFS Mount Failed, we format it");
        SPIFFS.format();
    }
    // SPIFFS.format();

    FSFilePrint filePrint(SPIFFS ,"/logfile", 2, 500, &Serial);
    filePrint.open();

    filePrint.print("Millis since start: ");
    filePrint.println(millis());

    filePrint.close();
}

void loop() {
}
```

## Contributions ##
Enhancements and improvements are welcome to this or the original [SPIFFS FilePrint Library](https://github.com/PRosenb/SPIFFS_FilePrint).

## License ##
```
Arduino FS FilePrint Library
Copyright (c) 2021 Jesse R Codling (https://github.com/jclds139).
Forked from Arduino SPIFFS FilePrint Library
Copyright (c) 2019 Peter Rosenberg (https://github.com/PRosenb).


Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```
