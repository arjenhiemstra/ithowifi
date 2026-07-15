![NRG.Watch logo](images/00-nrgwatch-logo.png)

# Itho WiFi add-on — User Manual

**Version:** V1.13

**Date:** 14-06-2026

| | |
|---|---|
| ![CVE add-on revision 1.x](images/01-addon-cve-rev1.jpg) | ![CVE add-on revision 2.x](images/02-addon-cve-rev2.jpg) |
| **CVE add-on revision 1.x** | **CVE add-on revision 2.x** |
| ![Non-CVE add-on revision 1.x](images/03-addon-noncve-rev1.jpg) | |
| **Non-CVE add-on revision 1.x** | |

---

## Table of Contents

- [Foreword](#foreword)
- [Physical Installation CVE Add-on](#physical-installation-cve-add-on)
- [Physical Installation non-CVE Add-on](#physical-installation-non-cve-add-on)
- [Additional information for the HRU250 and HRU300](#additional-information-for-the-hru250-and-hru300)
- [Configuring the Add-on](#configuring-the-add-on)
- [Usage (Web interface, API)](#usage-web-interface-api)
- [Device-specific information](#device-specific-information)
- [Temperature/Humidity sensor](#temperaturehumidity-sensor)
- [Virtual Remote](#virtual-remote)
- [RF Module](#rf-module)
- [Updating firmware](#updating-firmware)
- [Resetting the module](#resetting-the-module)
- [Hmmm… where do I find more information?](#hmmm-where-do-i-find-more-information)
- [Hardware revisions](#hardware-revisions)
- [Installation example](#installation-example)
- [Node-red automation example](#node-red-automation-example)
- [Supported remotes](#supported-remotes)

---

## Foreword

Thank you for purchasing one (or more) add-on modules — enjoy using it!

An important warning first! Several Itho devices (at least the CVE's and the HRU200) are not touch-safe when powered, including the low-voltage parts. Therefore, always disconnect power from the device when installing an add-on on the circuit board. This prevents harm to yourself and to your Itho! If needed, operate the buttons on the CVE add-on circuit board only with a non-conductive material.

This is a concise manual to help you physically install the add-on and connect it to your own WiFi network. Information about further specific installations can be found online.

This manual, the plug-ins for e.g. Home Assistant and Homey, the wiki and further information online are a community effort — a collection of contributions from users for users. Without these contributions this document, among other things, would not exist. Additions, changes and/or suggestions are highly appreciated!

**Sources where further information can be found:**

- Wiki page: <https://github.com/arjenhiemstra/ithowifi/wiki>
- Tweakers.net forum: <https://gathering.tweakers.net/forum/list_message/82125986>
- Tweakers.net forum (WPU specific): <https://gathering.tweakers.net/forum/list_messages/2154474>
- Frequently asked questions: <https://gathering.tweakers.net/forum/list_messages/1976492/0#label-faq>

---

## Physical Installation CVE Add-on

To properly protect the add-on (PCB), we deliver it in an antistatic bag.

Below is a numbered list of steps to complete the installation of the add-on.

1. Take the add-on out of the antistatic bag;
2. Besides the add-on, you will also find a PCB spacer — set it aside for now.
3. Now disconnect power from the Itho ventilation unit by pulling the plug from the wall socket;
4. To reach the Itho's main board, remove the cover of the unit. This can be done by releasing the clips on the top and bottom with a flat screwdriver. You now see the housing that contains the main board; it is easily reached by opening the little door, or in case of a black cap it can be removed by combining some sideways pressure with an upward pulling motion. See the manufacturer's installation & user manual for any other/specific instructions;
5. Now place the PCB spacer with the curved / short side into the hole on the Itho main board that matches the hole on the add-on. Using this spacer reduces the chance of soldered joints vibrating loose over time. This step can optionally also be done after testing the add-on;
6. Placing the add-on requires no special tools or other materials.
   You can place the add-on by mounting it onto the 2×4 pin interface.
   See Installation example, circled in red, for where the 2×4 pin interface is located on the main board;
7. Now restore power to the Itho by inserting the plug into the wall socket.
8. If initialization is successful, the Itho status LED (see Hardware revisions) lights up briefly. After that it stays off. This indicates that communication between the Itho main board and the add-on has started successfully. The WiFi status LED will blink to indicate that an access point has been started.
9. The add-on is now installed; close the main board housing and place the cover back on the unit.

---

## Physical Installation non-CVE Add-on

To properly protect the add-on (PCB), we deliver it in an antistatic bag. If you ordered an enclosure with it, the add-on is already placed in the enclosure. Leave the add-on in the enclosure; removing the board from the enclosure can damage the enclosure.

Below is a numbered list of steps to complete the physical installation of the add-on.

1. If applicable, take the add-on out of the antistatic bag;
2. Find the COM/Service port on your Itho device. This is a connection that looks like a network port (RJ45 connector). Connect the add-on to the COM/Service port with a good, short (CAT6a, STP, 25cm) network cable;
3. The add-on is installed and will start an access point.

**Note.** Power for the add-on comes through the network cable from the connected Itho. In exceptional cases the Itho appears unable to supply enough power (certain revisions of the HRU300). As a result, communication with the HRU is not always stable. In that case, connect a USB power supply to the add-on. In all other cases this is not necessary.

---

## Additional information for the HRU250 and HRU300

To use the add-on with the HRU250 and HRU300, an adapter board and a PCI-e splitter cable are required.

To install these:

1. Remove the PCI-e cable from the HRU's HMI.
2. Connect the PCI-e cable of the HMI to the PCI-e splitter cable.
3. Connect one end of the splitter cable to the HMI, and the remaining end to the HRU250/300 adapter board.
4. The adapter board can be connected to the add-on with the network cable.

![HRU250/300 adapter installation](images/04-hru250-300-adapter-installed.png)

### Controlling the HRU250 and HRU300

These models are somewhat more limited in control via the add-on compared to other Itho models. To control this HRU you must use the CC1101 RF module on the add-on. The Virtual Remote option does not work on these HRU models.

To use the RF module it must be activated via the add-on's web interface, under the "System settings" menu. After a reboot, an additional menu item named "RF Devices" appears. Here an RF device can be configured. This RF device must then be paired with the HRU. Follow the same procedure used to pair physical remotes.

---

## Configuring the Add-on

Below is a numbered list of steps to make the basic settings of the add-on.

### Configuring the add-on — *Connecting to the add-on*

1. After connecting, the add-on automatically starts a WiFi access point. The WiFi status LED then blinks once per second (see Hardware revisions).

   Note: as long as the add-on is not yet connected to your WiFi network, the access point stays active. Once the add-on is connected, the access point automatically switches off after the configured time (15 minutes by default). This time can be changed in the settings.

2. Connect your phone, tablet or computer to the add-on's WiFi network:
   - Network name (SSID): **nrg-itho-XXXX** (the last 4 characters are unique per add-on)
   - Password: **password**

3. Open a browser and go to:
   `http://nrg-itho-XXXX.local` (replace XXXX with the 4 characters from the network name)
   Doesn't work (e.g. on Android)? Then go to <http://192.168.4.1>

### Configuring the add-on — *Setup Wizard*

From firmware version 3.0 onward, a Setup Wizard automatically starts and guides you through the configuration in a number of steps:

**Step 1 — WiFi**

Enter the name (SSID) and password of your own WiFi network. Optionally you can set a custom hostname. Click Connect and wait until the connection is confirmed. Note the IP address that is shown — you need it to reach the add-on later from your own network.

**Step 2 — Device**

The wizard automatically detects which type of Itho device is connected (e.g. CVE-Silent, HRU 350, DemandFlow) and applies the most common default settings. For devices without an I2C connection (e.g. the HRU 400), the option to activate RF standalone mode is offered.

**Step 3 — RF** (skipped if no CC1101 RF module is present)

Here you configure the RF remotes. You can pair (join) a remote with the Itho device, or skip this step and set it up later via the "RF Devices" menu.

Working in RF-standalone mode (a device without I2C, for example the HRU 400)? Then this is the moment to pair an emulated RFT CO2 remote with the Itho — you then use it to control the device and read out its status.

**Step 4 — MQTT** (optional)

Enter the details of your MQTT server if you want to connect the add-on to Home Assistant, Domoticz or another home automation platform via MQTT. Optionally enable Home Assistant MQTT Auto Discovery so the add-on automatically appears as a device in Home Assistant.

**Step 5 — HA Discovery** (skipped if MQTT is not enabled)

Configure which sensors and status information are made available to Home Assistant via HA Auto Discovery.

### Configuring the add-on — *After the wizard*

Reconnect your phone/computer to your own WiFi network and open the browser at the IP address from the wizard.

All settings can also be changed afterwards via the web interface. The wizard does not need to be run again.

---

## Usage (Web interface, API)

The Itho unit can now be controlled via the web interface (<http://nrg-itho-A1B2.local>, replace A1B2), via MQTT or the WebAPI.

Do you want to connect the add-on to home automation via MQTT? Then you need an MQTT server (broker). Note that the web interface and the WebAPI also work without MQTT. If you don't have an MQTT server yet, the link below provides a good example of how to install one on, for example, a Raspberry Pi:

<https://randomnerdtutorials.com/how-to-install-mosquitto-broker-on-raspberry-pi/>

### Controlling the Itho

There are several ways to control the Itho and change its settings.

For CVE's and the HRU 200 there are 2 ways to control the speed: remote commands (virtual or RF) and the stepless PWM2I2C protocol. For other devices (for example the HRU 350) only the remote commands (virtual remote and/or RF remote) are available. For the HRU 250 and HRU 300 only RF remote commands work. To send RF remote commands the CC1101 RF module must be present on the add-on. Virtual remote commands (where the add-on emulates a physical remote) are sent via the physical I2C connection with the Itho.

What is exactly possible per device is described in the [Device-specific information](#device-specific-information) chapter.

More details about the various options and any limitations can be found on the wiki:

<https://github.com/arjenhiemstra/ithowifi/wiki/Controlling-the-speed-of-a-fan>

### Connecting to home automation software

The add-on can be connected to other devices and home automation via MQTT and the WebAPI. The details are described under the *Using the API* heading below.

For the most commonly used platforms there are ready-made integrations:

- **Home Assistant — MQTT Auto Discovery.** If you enable MQTT with Home Assistant MQTT Auto Discovery, the add-on is automatically configured as a device in Home Assistant with a basic configuration. This can be adjusted and extended manually.
- **Home Assistant — separate integrations.** In addition, there are two community integrations for Home Assistant:
  - [ithowifi-ha-integration](https://github.com/arjenhiemstra/ithowifi-ha-integration) — uses the REST API (local, basic).
  - [haithowifi](https://github.com/jasperslits/haithowifi) — uses the MQTT API (more extensive).
- **Homey.** The [Itho Daalderop app](https://homey.app/en-nl/app/nl.monkeysoft.nrgwatch/Itho-Daalderop/) in the Homey app store uses the WebAPI (REST); MQTT is not required for it.
- **Homebridge (Apple HomeKit).** The community plugin [Homebridge Itho Daalderop HUE](https://github.com/SanderBaron/homebridge-itho-daalderop-HUE) brings your CVE to Apple HomeKit. It talks to the add-on via the WebAPI or MQTT (MQTT recommended for instant updates) and optionally integrates with Philips Hue.
- **Domoticz and other platforms.** Integration is done via MQTT (or the WebAPI).

### Using the API (WebAPI and MQTT)

The add-on has two APIs with which you can read out and control the Itho, for example from your own scripts or home automation: a **WebAPI** (HTTP) and an **MQTT API**. Both accept the same commands.

The complete and always up-to-date list of endpoints and commands for your firmware version is in the web interface under the **API** menu (interactive documentation). The underlying OpenAPI specification can also be retrieved directly via `http://<add-on>/api/openapi.json`.

**WebAPI (HTTP)**

There are two variants:

- **REST API (v2)** — a JSON API under `/api/v2/…`. A selection of the endpoints:
  - `GET /api/v2/deviceinfo` — device and firmware information
  - `GET /api/v2/ithostatus` — all current status values
  - `GET /api/v2/speed` and `GET /api/v2/lastcmd` — current speed and last command
  - `GET /api/v2/remotes`, `/api/v2/vremotes` and `/api/v2/rfstatus` — (virtual) remotes and RF status
  - `GET` and `PUT /api/v2/settings` — read and change settings
  - `POST /api/v2/command` — send a command (for example a ventilation level)
  - `POST /api/v2/vremote` and `POST /api/v2/rfremote/command`, `/co2`, `/demand`, `/config` — virtual and RF remote commands
  - `POST /api/v2/wpu/outside_temp` and `/api/v2/wpu/manual_control` — WPU-specific
- **Legacy API (v1)** — a simple read-only API in query style via `/api.html`, for example:
  - `http://<add-on>/api.html?get=ithostatus`
  - `http://<add-on>/api.html?get=currentspeed` (other values: `lastcmd`, `queue`, `remotesinfo`)

  Reading and changing settings has moved to the REST API (`GET`/`PUT /api/v2/settings`).

**MQTT API**

If MQTT is configured, the add-on uses the topics below. The *base topic* is configurable (default `itho`):

- `itho/cmd` — publish your commands to the add-on here (same set as the WebAPI).
- `itho/cmd/response` — the add-on's response to a command.
- `itho/state` — the add-on publishes a JSON with all status values here.
- `itho/lwt` — the online/offline status of the add-on (Last Will & Testament).

A working example (Node-RED) that publishes to `itho/cmd` is in the [Node-red automation example](#node-red-automation-example).

---

## Device-specific information

The add-on automatically detects which Itho device is connected and adapts the available settings and readable status values accordingly. Below is described, per device (group), how it is controlled, which information the add-on reads out and what to watch out for.

In addition to the per-device control described below, the add-on can emulate an RFT CO2 remote for **every fan (CVE and HRU)** and for **DemandFlow**. After pairing it with the device, it can be controlled steplessly from 0–100%, in steps of half a percent. This works both with an I2C connection and in RF-standalone mode. Only with the CVE's can this RF communication be less reliable; see the *CVE family* section below.

> **Note:** which status values and settings are exactly available depends on the model and the firmware version of the Itho device. The web interface and the API page always show the current, complete list for your device. The values below are examples of what a device typically reports.

### CVE family (CVE, CVE-Silent, HRU 200)

**Models:** CVE, CVE-Silent and the HRU 200 (recognized in the web interface as *CVE-SilentExtPlus*). The CVE ECO2 and CVE-SilentExt are also recognized, but only provide basic status.

**Control:** these units communicate via the physical I2C connection on the main board. The speed can be controlled in two ways:

- **Remote commands** — a *virtual remote* emulated by the add-on, or a physical/RF remote (RF requires the CC1101 module).
- **PWM2I2C** — a stepless protocol for continuous control (0–100%). This is only available on the CVE's and the HRU 200, not on the other devices.

By default the add-on is in PWM2I2C mode; the remote mode can be set via *System settings*.

> **Note (models with a built-in CO2 sensor):** on CVE-S Optima Inside models with a built-in CO2 sensor, PWM2I2C does not work — the PWM2I2C commands are overruled by the internal CO2 sensor. Use virtual remote control on these models.

**What the add-on reads out (among others):** ventilation level/setpoint (%), requested and actual speed (rpm), selected mode, error code, startup counter, operating hours, **CO2 value (ppm)**, valve position and presence timer. Newer CVE's have a built-in humidity sensor — see the [Temperature/Humidity sensor](#temperaturehumidity-sensor) chapter.

The **CVE-Silent** additionally reports, among other things, the highest measured CO2 (ppm) and RH (%), the internal humidity (%) and temperature (°C), and absence timers. The **HRU 200** has the most extensive set of the family, additionally including filter usage (hours), outside and exhaust temperature, average outlet temperature, bypass mode and position, and the maximum CO2/RH levels.

> **Note (RF with CVE's):** on the CVE's the antennas of the add-on and the unit are close together. This can cause communication problems with RF control (such as an emulated RFT CO2 remote). On the CVE's, therefore, preferably use I2C control (virtual remote or PWM2I2C).

### HRU heat recovery via I2C (HRU 350, HRU ECO-fan)

**Models:** HRU 350 and HRU ECO-fan.

**Control:** via the I2C connection, with remote commands (virtual remote or RF). The stepless PWM2I2C protocol is **not** available on these models — only the remote modes/commands.

**What the add-on reads out (among others):** requested ventilation level (%), balance (%), supply and exhaust fan (both requested and actual speed), supply, exhaust, room and outside temperature (°C), valve and bypass position, frost and boiler timers, filter counter, global fault code, current mode, highest measured CO2 (ppm) and RH (%), air quality (%) and the remaining override timer. The HRU ECO-fan reports a comparable set around supply/exhaust, temperatures, bypass and filter usage.

> **DuoZone:** HRU units with two zones (DuoZone) are recognized; the zone data appears on the RF Status page of the web interface (and via `GET /api/v2/rfstatus`).

### HRU 250 / HRU 300

**Control:** these models are controlled and read out **exclusively via RF (CC1101)** — the Virtual Remote option does not work here. An RF remote must therefore be paired from the add-on.

**Special hardware:** an adapter board and a PCI-e splitter cable are required. The physical installation is described in the [Additional information for the HRU250 and HRU300](#additional-information-for-the-hru250-and-hru300) chapter. On some revisions of the HRU 300 the unit may supply too little power; in that case, connect a USB power supply to the add-on.

**What the add-on reads out (among others):** outside and mixed-air temperature (°C), supply and exhaust airflow (m³/h), inlet, exhaust and blown-out air temperature (°C), relative and absolute fan speed (%), the inbound and outbound mass flow (kg/h), bypass position (%), desired inlet temperature, extensive frost data, motor speed (rpm), the highest measured RH/CO2 and the fan's current consumption (mA).

### Devices without I2C — RF standalone (e.g. HRU 400)

Some units (for example the HRU 400) have no I2C connection usable by the add-on. For these there is **RF standalone mode**: the add-on skips I2C entirely and works only via RF (CC1101).

- **Activating:** the Setup Wizard offers this for devices without I2C; later it can also be done via *System settings*.
- The add-on can emulate an RFT CO2 remote. After it is paired with the HRU, the HRU can be controlled steplessly from 0–100% in steps of half a percent.
- With a paired, emulated RFT CO2 remote, the status (such as ventilation level and temperatures) can be requested periodically.

This mode requires a CC1101 RF module.

### DemandFlow

**Control:** via the I2C connection. DemandFlow is a demand-driven ventilation system with multiple rooms; control and read-out go via the web interface, MQTT and the WebAPI.

**What the add-on reads out (among others):** operating status and mode, relative humidity per bathroom (%), exhaust fan (%), CO2 in the plenum (ppm) plus the calculated CO2 per room (kitchen, toilet, living room, bedrooms, etc.), the calculated valve/flap positions and the calculated airflow per room (m³/h), and the error code.

### AutoTemp

**Models:** AutoTemp and AutoTemp Basic.

**Control:** via the I2C connection. AutoTemp is a zone control system (per room), for example for underfloor heating; control and read-out via the web interface, MQTT and the WebAPI.

**What the add-on reads out (among others):** mode, condition and error code, and per room (up to 12 rooms) the measured temperature, the setpoint and the delivered power (% and kW), plus the valve positions per manifold/zone.

### WPU (heat pump)

The WPU is a heat pump (referred to in the firmware as *Heatpump*), not a ventilation device. It has by far the most extensive set of readable values.

**Control:** via the I2C connection; read-out and settings via the web interface, MQTT and the WebAPI.

**What the add-on reads out (among others):** a large number of temperatures (outside, boiler top/bottom, evaporator, suction and compressed gas, liquid, source in/out, CV supply and return, °C), CV pressure (bar), the current consumption of the compressor and the electric element (A), pump speeds for CV, source and boiler (%), valve positions, the status of compressor/element/trickle heating, various timers and an error code with detailed error bytes.

> The error bytes are shown as raw values; a translation table to descriptions is not included in the add-on. Consult the Itho documentation for their meaning.

### Other recognized devices

The add-on also recognizes various other Itho devices by name (such as Air curtain, LoadBoiler, GGBB, CO2 relay, OLB, RF+, Extended (Plus), AreaFlow and RF_CO2). For these, the add-on shows basic information, but (as yet) no extensive settings or status list.

---

## Temperature/Humidity sensor

New Itho CVE units come standard with a humidity sensor. The add-on can read out this sensor.

![CVE PCB with humidity sensor](images/05-vochtsensor-cve-pcb.jpg)

Older Itho models without a humidity sensor can, with some tweaker skills, also be equipped with a temperature/humidity sensor.

The connections of the humidity sensor are as follows, and the corresponding connections on the add-on board are indicated with red markings and labels.

| | |
|---|---|
| ![Humidity sensor pinout diagram](images/06-vochtsensor-pinout-diagram.png) | ![Add-on PCB with SCL/SDA/3v3/GND marking](images/07-vochtsensor-addon-pcb.png) |

Here you can find an example of a tweaker who did this successfully:

<https://gathering.tweakers.net/forum/list_message/66500576#66500576>

---

## Virtual Remote

The add-on can present itself as one or more **virtual remotes**. A virtual remote emulates a physical remote entirely in software and controls the Itho directly over the **I2C connection** (the wired connection on the main board) — so no CC1101 RF module is required.

**Which devices?** Virtual remotes work with the I2C-connected ventilation devices, such as the CVE family, the HRU 350 and the HRU ECO. Most fan devices benefit from at least one virtual remote for control. They do **not** work with the HRU 250, HRU 300 and HRU 400, which are controlled exclusively via RF (see [RF Module](#rf-module)).

**Pairing.** Like a physical remote, a virtual remote must first be joined to the Itho. The add-on can send a join command automatically (the *Send join command* option, on the next power-on).

**Difference from an emulated RF remote.** A *virtual remote* communicates with the Itho over the wired **I2C connection**; an *emulated RF remote* communicates **wirelessly via the CC1101 module** (868 MHz), just like a physical remote. Use a virtual remote for devices with an I2C connection, and an emulated RF remote for devices that only work via RF (HRU 250/300/400) or to emulate wireless sensors (such as RFT CO2).

---

## RF Module

RF functionality requires a CC1101 module on the add-on.

The CC1101 module can receive and send the RF signals of Itho remotes.
The most commonly sold Itho remotes are supported.

**Moving an existing remote to the add-on.** An existing physical remote can be moved to the add-on, so the add-on receives that remote's RF commands and translates them to the Itho. The remote is unpaired from the Itho and paired with the add-on, following the procedure below.

> **Note:** handling the RF commands of existing remotes via the add-on is not available for the HRU 250, HRU 300 and HRU 400. Monitoring RF communication is possible, however. Therefore, leave existing RF remotes paired with the Itho and pair the add-on as an additional remote with your HRU.

1. Unlearn the remote by sending a leave command within the first 2 minutes after powering on the Itho unit (on the remote you do this by pressing all 4 buttons simultaneously);
2. If you have not already done so, configure the add-on module further and (last) activate the RF module under the "System settings" menu;
3. The add-on reboots;
4. If the RF module is correctly detected, the option to manage Itho remotes appears in the same menu. Only add remotes after the Itho unit is out of learn/leave mode (so at least 2 minutes after powering on), otherwise the remote will be paired with the Itho again;
5. Put the add-on in learn/leave mode (via the web interface);
6. Send a learn command with your remote (usually press 2 diagonally opposite buttons simultaneously; otherwise see the manual of the specific remote);
7. If all goes well, your remote ID now appears in the first free position (see the image below). If it still does not succeed after several attempts, the remote may not (yet) be supported. In that case, contact support.

![RF Remotes setup screenshot](images/08-rf-remotes-setup-screenshot.png)

**The add-on as an emulated RF remote.** The add-on can also present itself as an emulated RF remote: via the CC1101 module it behaves wirelessly like an Itho remote. With the *Remote function* set to **Send**, the add-on sends speed and timer commands to the Itho over the air. This is used for devices without a usable I2C connection (such as the HRU 250, HRU 300 and HRU 400) and to emulate, for example, an RFT CO2 sensor.

Before an emulated RF remote works, it must be joined to the Itho once — just like a physical remote. Put the Itho in its pairing/learn mode (see your Itho's manual) and start the join with the blue join button next to the relevant remote on the **RF devices** page. After that, the add-on can be used like a real physical remote.

**Note:** There is a debug option to make RF commands visible in the web interface. Set one of the following levels on the **Syslog** page, under *RF Debug log level*:

- **Level 1:** shows all recognized Itho remote commands incl. remote ID
- **Level 2:** shows all RF packets coming from a configured remote (see <https://github.com/arjenhiemstra/ithowifi/tree/master/remotes> for more details on how to use this)
- **Level 3:** shows all RF packets that are processed (this probably includes a lot of non-Itho-related packets)
- **Level 0:** turns the debug option off again

The RF log appears on the **Syslog** page once the first RF command has been received.

![RF debug log screenshot](images/09-rf-debug-log-screenshot.png)

---

## Updating firmware

You can easily update the add-on's firmware via the web interface.

1. Open the web interface and go to the **Update** menu.
2. Choose which firmware you want to install:
   - **Stable** — the recommended, stable version.
   - **Beta** — the newest test version with the latest features (may still contain bugs).
3. Click the **Install** button.
4. The add-on automatically retrieves the chosen firmware from GitHub and installs it. Follow the progress on the page; once the update is done, the add-on automatically reboots.

With a normal update your settings are preserved. Only a factory reset erases the configuration (see [Resetting the module](#resetting-the-module)).

**Progress in Home Assistant.** Both Home Assistant integrations — the REST and the MQTT one — show the add-on as an *update entity*; during an update the progress is shown as a progress bar.

**Uploading a firmware file yourself.** If the automatic retrieval does not work (for example without an internet connection), you can also manually upload a `.bin` firmware file on the Update page. The same can be done via the emergency firmware after a failsafe boot (see [Resetting the module](#resetting-the-module)).

---

## Resetting the module

### Fail safe boot / module factory reset

Should it unexpectedly happen that the module is no longer reachable due to an incorrect configuration, it is possible to boot the module in fail safe mode. This formats the file system with the configuration files, and the module starts a simplified web interface with which it is possible to flash new firmware.

The procedure for this reset is as follows:

1. press the reset button
2. wait until the WiFi status LED starts blinking rapidly (2x per sec)
3. then, within 2 sec, press the "Fail save" (on some revisions "default") button
4. shortly after, the WiFi LED will start blinking once per second
5. If you do not want to flash new firmware, skip this step.
   The add-on has now started an access point with a simplified "emergency" firmware. With this, only new firmware can be flashed. The web page can be reached at the following address: <http://192.168.4.1/update>
   Uploading new firmware can take a minute; wait calmly until the page refreshes and shows the result.
6. press the reset button

After this, all settings are erased and the add-on will start in factory default mode.

There is also a YouTube video available that shows this procedure:
<https://youtu.be/3sWclzq73n4>

This method is only available when using 'official' firmware, or firmware based on it.

**Hardware revisions up to 2.5** (see the back of the add-on):

To activate this mode for hardware revisions up to version 2.5, a different procedure must be followed:

Connect the two metal pads labelled 'failsafe' on the board to each other. This is easiest with a soldering iron and a bit of solder, or by connecting the pads with, for example, a screwdriver.

When powering on the Itho with the add-on installed, the add-on will perform the fail save procedure and remove all settings. New firmware can optionally also be loaded as described earlier.

After this procedure, remove any solder connection that is present, and put the module back into use as described in this manual.

---

## Hmmm… where do I find more information?

There is an enormous amount of useful information on the wiki:
<https://github.com/arjenhiemstra/ithowifi/wiki>
It is for users and by users. Feel free to make changes here too.

On tweakers.net there is a thread on the forum about this add-on.
It also contains more information about using this add-on in combination with e.g. Home Assistant, Domoticz and other systems.

You can also go there for questions.
The link is: <https://gathering.tweakers.net/forum/list_messages/1976492/0>

For further questions, feedback and code changes, you can get in touch via <info@nrgwatch.nl> or <https://www.github.com/arjenhiemstra/ithowifi>.

---

## Hardware revisions

Images may differ slightly from the product you received; the operation is identical.

**Hardware revision 1:**

![Hardware revision 1 — Status LED, WiFi LED, connection to Itho board](images/10-hwrev1-led-locations.png)

**Hardware revision 2 (newer revisions with fail safe button):**

![Hardware revision 2 with fail safe button](images/11-hwrev2-new-failsafe-button.jpg)

**Hardware revision 2 (older revisions with failsafe solder option):**

![Hardware revision 2 with failsafe solder option](images/12-hwrev2-old-failsafe-solder.png)

---

## Installation example

Installation header for the add-on in the red circle.
Depending on the production date of the Itho box, the board may look slightly different.

![Itho main board with installation header circled in red](images/13-itho-basisprint-install-header.jpg)

Add-on correctly installed:

![Add-on correctly installed on the main board](images/14-addon-installed-on-basisprint.jpg)

---

## Node-red automation example

Below is a Node-RED example:

```json
[{"id":"78e45008.cda2f","type":"mqtt out","z":"79360772.4553e8","name":"itho","topic":"itho/cmd","qos":"0","retain":"true","broker":"b4eed736.102278","x":430,"y":1000,"wires":[]},{"id":"98cc2161.c3896","type":"inject","z":"79360772.4553e8","name":"itho level 127","topic":"","payload":"127","payloadType":"str","repeat":"","crontab":"","once":false,"onceDelay":0.1,"x":170,"y":1000,"wires":[["78e45008.cda2f"]]},{"id":"5a4ffa98.c88454","type":"inject","z":"79360772.4553e8","name":"itho level 254","topic":"","payload":"254","payloadType":"str","repeat":"","crontab":"","once":false,"onceDelay":0.1,"x":170,"y":1060,"wires":[["78e45008.cda2f"]]},{"id":"1e824b95.a04104","type":"inject","z":"79360772.4553e8","name":"itho level 0","topic":"","payload":"0","payloadType":"str","repeat":"","crontab":"","once":false,"onceDelay":0.1,"x":160,"y":940,"wires":[["78e45008.cda2f"]]},{"id":"b4eed736.102278","type":"mqtt-broker","z":"","name":"MQTT Server","broker":"192.168.1.2","port":"1883","clientid":"","usetls":false,"compatmode":false,"keepalive":"60","cleansession":true,"birthTopic":"","birthQos":"0","birthPayload":"","closeTopic":"","closeQos":"0","closePayload":"","willTopic":"","willQos":"0","willPayload":""}]
```

---

## Supported remotes

Itho remotes that have been tested working:

![Overview of Itho Daalderop RFT transmitters](images/15-appendix-d-rft-overview.png)
