# ithowifi
Wemos D1 Mini / ESP8266 based WiFi controller for itho central ventilation boxes

*(note: I'm in no way a professional hardware/software engineer and have no background in these subjects. I like this for a hobby, to learn and to share my projects)*

Control the Itho Daalderop Eco Fan RFT using basically only an ESP8266 directly communicating with the Itho by i2c protocol. 
This is an example implementation to be able to control the speed setting of the fan from 0 - 254 through either a simple web-interface or 
by MQTT.

The code will give you full remote control over the Itho Eco Fan RFT with one simple add-on module and without further changes to the Itho box.

Two versions of the firmware have been included: 
* A simple version with hardcoded SSID and password.
* A more advanced version that starts an access point when no SSID can be connected and allows you to setup the device specifics (Wifi and MQTT settings) and save them on SPIFFS using the webinterface.

**Two important notes about the firmware:**
* The add-on is able to control the itho box in standard or medium mode setting only. This means you can use the original remote but if you leave the itho box in low or high setting the itho won't accept commands from the add-on. This is itho designed behaviour.
* The start-up time of the add-on is just fast enough for the I2C init to take place. If this init phase fails (the LED on the PCB will stay on), power cycle the itho unit. I found it helpful to wait a full 30 seconds before switching on the itho box again. Maybe there is a way to move to I2C init to somewhere before the setup() routine to improve init robustness. Build the firmware with 160Mhz CPU option! 
  
**I2C protocol:**

The Itho fan and add on modules both communicate as I2C master.

First the main pcb sends a start message:
```
Address (0x41),Write
Data: (0xC0),(0x00),(0x00),(0x00),(0xBE)
```
The add-on then replies (as slave):
```
Address (0x41),Write
Data: (0xEF),(0xC0),(0x00),(0x01),(0x06),(0x00),(0x00),(0x09),(0x00),(0x09),(0x00),(0xB6)
```
After which the main pcb confirms with another message:
```
Address (0x41),Write
Data: (0xC0),(0x00),(0x02),(0x05),(0x00),(0x00),(0x09),(0x60),(0x00),(0x4E)
```
After these initial init messages back and forth the add-on needs to switch to i2c master and sends 
speed commands to the main pcb like follows:
```
Address: (0x00),Write
Data: (0x60),(0xC0),(0x20),(0x01),(0x02),(b),(e),(h)
```

Where:  
   uint8_t b = target speed (0 - 254)  
   uint8_t e = 0  
   uint8_t h = 0 - (67 + b)  

It is possible to have other values for e (0, 64, 128, 192) for even more control steps, the complete formula for h will be:

   uint8_t h = (0-e) - (67+b)  


**Hardware:**

A sample hardware design (KiCad) is included, this module can be plugged on the Itho main PCB directly without extra components.
*Tested with Itho Daalderop CVE from late 2013 and 2019 (PCB no. 545-5103 and 05-00492)

![alt text](https://github.com/arjenhiemstra/ithowifi/blob/master/images/pcb.png "Add-on PCB")
![alt text](https://github.com/arjenhiemstra/ithowifi/blob/master/images/itho%20pcb.jpg "Itho main PCB")
![alt text](https://github.com/arjenhiemstra/ithowifi/blob/master/images/itho%20pcb%20w%20add-on.png "Itho main PCB with add-on")
![alt text](https://github.com/arjenhiemstra/ithowifi/blob/master/images/ithowifi_board_topside.png "PCB Top")
![alt text](https://github.com/arjenhiemstra/ithowifi/blob/master/images/ithowifi_board_bottomside.png "PCB Bottom")


~~PoC is done and working, PCBs have been ordered. I will update this page when the boards arrive and are proven to be working.~~
PCB done and tested! 
I have a few assembled boards left for those who want to try this as well.

BOM:

Amount | Part 
--- | ---
1 | Wemos D1 mini V3
2 | BSS 138-7-F DII (SOT-23)
1 | Recom R-78E5.0-1.0 DC-DC converter
4 | 10K Ohm resistors (1206)
1 | 100 Ohm resistor (1206)
1 | Kingbright LED Blue KP-3216QBC-D (1206) (optional I2C status LED)
2 | female pin header 1x8 (optional)
1 | female pin header 2x4
2 | standoffs 3mm hole x (total length 21,4 mm, effective pcb spacing 11,5 mm)



Change of components version 1.1:
The ISO1540/ADUM 1250 ARZ solution did not work as expected. That's why for version 1.1 I switched to a quite common BSS138 MOSFET based logic level converter solution and replaced the other components with SMDs too. The SMD components are all 1206 in size so quite easy to solder by hand.


Choice of components version 1.0:

ESP8266 NodeMCU DevKit (https://github.com/nodemcu/nodemcu-devkit-v1.0) or WeMos D1 mini (https://docs.wemos.cc/en/latest/d1/d1_mini.html): 
readily available at low prices. Easy to integrate in projects.

ISO1540/ADUM 1250 ARZ: 
not the cheapest logic level convertor option but special designed for i2c and a one chip solution, SMD but still quite easy to solder by hand.

Recom R-78E5.0-1.0: 
the regulator (7805) on the itho mainboard is not suitable to drive extra loads like a esp8266. Luckily there is a pin on the mainboard that holds the output from the main
power supply (about 16,3 volts and definitely able to handle more than the 500mA of the on board regulator). Placing a 7805 (or similar) on the add-on board is possible 
but will probably generate lots of heat (up to 5 watts) because of big voltage drop.
That's why I selected the DC-DC converter from recom, very small package, high efficiency (90%).

Credits for the basic firmware:  
   Web interface based on: https://github.com/SasaKaranovic/ESP-IoT-Dimmer  
   MQTT implementation based on: https://github.com/srijansaxena11/ESP8266-MQTT-WiFi-Dimmer-PIR/  
