# VoyagerOTA Client

> An easy to use small OTA client library for the VoyagerOTA platform, compatible with ESP32 and ESP8266 devices.

## Usage

### VoyagerOTA:

```cpp
// Development mode is for for staging environment builds for testing....
// Make sure to always disable the [__ENABLE_DEVELOPMENT_MODE__] flag to false
// while uploading the Binary to VoyagerOTA Platform as development builds are not
// allowed....And by disabling it the build is treated as production build...
#define __ENABLE_DEVELOPMENT_MODE__ true
#define CURRENT_FIRMWARE_VERSION "1.0.0"

#include <WiFi.h>
#include <VoyagerOTA.hpp>
using namespace Voyager;

void connectToWifi() {
    WiFi.begin("SSID", "PASSWORD");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(50);
    }
    Serial.println("Connected to Internet");
}

void setup() {
    Serial.begin(9600);
    connectToWifi();
    OTA<> ota(CURRENT_FIRMWARE_VERSION);

    ota.setCredentials("voyager-project-id-here....", "voyager-api-key-here...");
    auto release = ota.fetchLatestRelease();

    if (release && ota.isNewVersion(release->version)) {
        Serial.println("New version available: " + release->version);
        Serial.println("Changelog: " + release->changeLog);
        ota.setDownloadURL(release->downloadURL);
        ota.performUpdate();
    } else {
        Serial.println("No updates available");
    }
}

void loop() {}
```

## Requirements

- C++17 or higher
- ArduinoJson library version 7.0 or above
- ESP32/ESP8266 Arduino framework
- [cpp-semver](http://github.com/z4kn4fein/cpp-semver)
- [HTTPUpdate](https://github.com/espressif/arduino-esp32/tree/master/libraries/Update)

## Documentation: [VoyagerOTA Docs](https://staging.api.voyagerota.com/docs)
