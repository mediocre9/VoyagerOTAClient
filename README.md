# VoyagerOTA Client Lib

**VoyagerOTA** is a library for OTA (Over-The-Air) firmware updates on ESP32/ESP8266 devices.  
It’s built on top of Arduino’s [`HTTPUpdate`](https://github.com/espressif/arduino-esp32/tree/master/libraries/Update).

By default, VoyagerOTA integrates seamlessly with the **VoyagerOTA Platform** (our own backend for firmware delivery). It also supports **GitHub Releases** and **custom backends** via a simple **Two Endpoints Approach**.

## Requirements

-   C++17 or higher
-   ArduinoJson library version 7.0 or above
-   ESP32/ESP8266 Arduino framework
-   [cpp-semver](http://github.com/z4kn4fein/cpp-semver)
-   [HTTPUpdate](https://github.com/espressif/arduino-esp32/tree/master/libraries/Update)
