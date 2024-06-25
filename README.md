# ithowifi
ESP based WiFi controller for Itho devices

*(note: I'm in no way a professional hardware/software engineer and have no background in these subjects. I like this for a hobby, to learn and to share my projects)*

Finished boards can be ordered in my webshop:  
[https://www.nrgwatch.nl](http://www.nrgwatch.nl)  
or in my Tindie store:  
[https://www.tindie.com/products/19680/](https://www.tindie.com/products/19680/)  

|CVE Add-on|non-CVE wifi module|
|:---:|:---:|
|![alt text](https://github.com/arjenhiemstra/ithowifi/blob/master/images/pcb.png "CVE Add-on PCB")  |  ![alt text](https://github.com/arjenhiemstra/ithowifi/blob/master/images/non-cve-pcb.png "non-CVE module PCB")|

## WiFi add-on to control itho central ventilation units

-   Control your* Itho Daalderop CVE with one simple add-on module
-   No hardware changes needed to the itho box, no warranty void
-   Control the speed setting of the fan from 0 - 254 through either a simple web-interface, MQTT or WebAPI (CVE models and HRU200 only)
-   Send remote commands without having a real remote (virtual remote function)
-   Easily integrate this wifi controller in any home domotica system through MQTT or WebAPI
-   Installation can be done in minutes
-   Detailed installation manual

###  CVE Models confirmed to work:
-   CVE ECO 2 (this unit is also sold by the brand 'Heatrae Sadia' in the uk)
-   CVE ECO RFT (SE/SP, HE/HP)
-   CVE-S ECO (SE/SP, HE/HP)
-   CVE-S OPTIMA package with built-in CO2 sensor. CO2 sensor usable but an extra long pinheader needs to be soldered on the add-on)
-   HRU 200 ECO (also sold as 'Elektrodesign EHR 140 Akor BP' and 'Heatrae Sadia Advance Plus')

###  NON-CVE Models confirmed to work:
-   HRU eco fan
-   HRU 350
-   DemandFlow
-   QualityFlow
-   HeatPump WPU
-   AutoTemp

###  Models confirmed to work but modifications needed:
-   HRU 300 - see: https://github.com/arjenhiemstra/ithowifi/issues/97

####   An important note about the firmware for CVE and HRU200 devices:
The cve add-on is able to control the itho unit in standard or medium mode setting only. This means you can use the original remote but if you leave the itho box in low or high setting the itho won't accept commands from the add-on. This is itho designed behaviour. Adding a CC1101 RF module and/or letting the add-on present itself as virtual remote can circumvent this issue, the virtual remote function can also be used to force a medium mode before sending commands.

## Hardware:

A sample hardware design (KiCad) is included, this module can be plugged on the Itho main PCB directly without extra components.
The design files and BOM can be found in the [hardware repo](https://github.com/arjenhiemstra/ithowifi_hardware).

## Further information:

More information is available on the wiki and in the manual:
|  |  |
|---|---|
|Wiki:  |https://github.com/arjenhiemstra/ithowifi/wiki  |
|Dutch:  |https://github.com/arjenhiemstra/ithowifi/raw/master/handleiding.pdf  |
|English:  |https://github.com/arjenhiemstra/ithowifi/raw/master/manual.pdf  |

## Support this project:
If you like this project and would like to support it, please contribute with code updates, wiki edits, pull requests etc. You could also buy me a coffee as appreciation.
I will be really thankfull for anything even if it is just a kind comment towards my work, because that helps me a lot.

[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/nrgwatch)

