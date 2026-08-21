# ESP32 GPIO Web Server

A simple Arduino sketch for an ESP32 that runs a local web server for GPIO control and monitoring.

## Features

- Control 4 GPIO outputs from a web page
- Read 3 GPIO input states
- Input states update automatically without refreshing the full page
- Supports Arduino OTA updates
- Uses mDNS so the page can be reached at `http://esp32.local/` when supported by your network/device

## GPIO pins

### Outputs

- GPIO 4
- GPIO 5
- GPIO 16
- GPIO 17

### Inputs

- GPIO 14
- GPIO 27
- GPIO 13

## Before uploading

Edit these values in `esp32-gpio-webserver.ino` before uploading to your ESP32:

```cpp
#define STASSID "YOUR_WIFI_NAME"
#define STAPSK  "YOUR_WIFI_PASSWORD"
const char* otaPassword = "YOUR_OTA_PASSWORD";
```

Do not commit your real Wi-Fi password or OTA password to GitHub.

## First upload

The first upload must be done by USB. After the sketch is running, Arduino OTA updates can be used for later uploads.

In Arduino IDE, use an ESP32 partition scheme that supports OTA, such as a default OTA-capable scheme. Avoid a no-OTA / huge-app-only partition scheme if you want wireless updates.

## Web page

After uploading, open the Serial Monitor at 115200 baud and note the IP address. Then open either:

```text
http://esp32.local/
```

or the IP address shown in Serial Monitor.
