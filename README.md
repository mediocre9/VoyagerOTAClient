# Voyager OTA Client

> An OTA client library for the VoyagerOTA platform, compatible with ESP32 and ESP8266 devices.

# #Getting Started

> ### Quick Start (VoyagerOTA):

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
---
## Advanced Mode 
> [!NOTE]
> Advanced Mode `__ENABLE_ADVANCED_MODE__` compile time flag allows custom parsers and custom backends. Voyager-specific features are disabled in this mode.
### 1. Github Release OTA:
```cpp

// required to enable more manual settings and disable voyager related features....
#define __ENABLE_ADVANCED_MODE__ true 
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

    std::unique_ptr<GithubJSONParser> parser = std::make_unique<GithubJSONParser>();
    OTA<HTTPResponseData, GithubReleaseModel> ota(std::move(parser), CURRENT_FIRMWARE_VERSION);

    // https://docs.github.com/en/rest/releases/releases?apiVersion=2022-11-28#:~:text=GET-,/repos/%7Bowner%7D/%7Brepo%7D/releases,-cURL
    std::vector<Header> releaseHeaders = {
        {"Authorization", "Bearer your-github-token"},
        {"X-GitHub-Api-Version", "2022-11-28"},
        {"Accept", "application/vnd.github+json"},
    };

    // replace {owner} and {repo} with your github username and repo.......
    ota.setReleaseURL("https://api.github.com/repos/{owner}/{repo}/releases", releaseHeaders);

    Serial.println("OTA Started....");

    auto release = ota.fetchLatestRelease();

    if (release && ota.isNewVersion(release->version)) {
        Serial.println("New version available: " + release->version);
        Serial.println("Release name: " + release->name);

        std::vector<Header> downloadHeaders = {
            {"Authorization", "Bearer your-github-token..."},
            {"X-GitHub-Api-Version", "2022-11-28"},
            {"Accept", "application/octet-stream"},
        };

        ota.setDownloadURL(release->downloadURL, downloadHeaders);
        ota.performUpdate();
    } else {
        Serial.println("No updates available yet!");
    }
}

void loop() {}
```
---
### 2. Custom Backend Support
> [!NOTE]
> Custom parsers allow integration with any backend. **All models must extend BaseModel.**

```cpp
#define __ENABLE_ADVANCED_MODE__ true
#define CURRENT_FIRMWARE_VERSION "1.0.0"

#include <VoyagerOTA.hpp>

struct CustomPayload : public Voyager::BaseModel {
    String description;
    int statusCode;
};

class CustomParser : public Voyager::IParser<Voyager::HTTPResponseData, CustomPayload> {
public:
    std::optional<CustomPayload> parse(Voyager::HTTPResponseData responseData, int statusCode) override {
        ArduinoJson::JsonDocument document;
        ArduinoJson::DeserializationError error = ArduinoJson::deserializeJson(document, responseData);

        if (error) {
            Serial.println("JSON parsing failed");
            return std::nullopt;
        }

        if (statusCode != HTTP_CODE_OK) {             
            return std::nullopt;
        }

        CustomPayload payload(document["version"],
                              document["description"],
                              document["downloadUrl"],
                              statusCode);

        return payload;
    }
};

void setup() {
    Serial.begin(9600);
    auto parser = std::make_unique<CustomParser>();
    Voyager::OTA<Voyager::HTTPResponseData, CustomPayload> ota(std::move(parser), CURRENT_FIRMWARE_VERSION);

    ota.setReleaseURL("https://api.hack-nasa-backend.com/firmware/latest");
    auto release = ota.fetchLatestRelease();
    if (release && ota.isNewVersion(release->version)) {
        ota.setDownloadURL(release->downloadURL);
        ota.performUpdate();
    }
}

void loop() {}
```
## Requirements

- C++17 or higher
- ArduinoJson library version 7.0 or above
- [cpp-semver](http://github.com/z4kn4fein/cpp-semver) - v0.4.0
- [HTTPUpdate](https://github.com/espressif/arduino-esp32/tree/master/libraries/Update) - v3.0.7

