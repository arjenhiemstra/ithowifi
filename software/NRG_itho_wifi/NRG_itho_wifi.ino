

/*
 *
 * Info to compile the firmware yourself:
 * 
 * Build is done using Arduino IDE or PlatformIO, required libs are included when cloning from git.
 * Copy the contents of the ./software/lib folder to your Arduino Libraries folder first
 *
 * Select 'ESP32 Dev Module' as board
 * Choose partition scheme: Minimal SPIFFS (1.9 APP with OTA)
 * Compile the software
 *
 * Warning: the web contents are build using a script that will not run from the Arduino IDE. 
 * If you made changes to the html or js files, you will need to make sure to include then manually or use PlatformIO to compile.
 *
 */
