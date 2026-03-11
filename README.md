# VoyagerOTAClient

A semver based OTA client helper library for the VoyagerOTA platform. Supports GitHub releases and any custom JSON backends via custom parsers for ESP32.

## Features

- [x] Semver based version comparison
- [x] VoyagerOTA platform integration
- [x] Custom parser support for any JSON backend

---

## Getting Started

### Available Methods

| Method                                                                                                                               | Voyager Mode | Advanced Mode |
| ------------------------------------------------------------------------------------------------------------------------------------ | :----------: | :-----------: |
| `performUpdate()`                                                                                                                    |      Yes     |      Yes      |
| `fetchLatestRelease()`                                                                                                               |      Yes     |      Yes      |
| `setParser(Parser parser)`                                                                                                           |      Yes     |      Yes      |
| `attachEventCallbacks(HTTPUpdateStartCB onStart, HTTPUpdateProgressCB onProgress, HTTPUpdateEndCB onEnd, HTTPUpdateErrorCB onError)` |      Yes     |      Yes      |
| `setDownloadURL(const String& endpoint, std::vector<Header> headers = {})`                                                           |      Yes     |      Yes      |
| `setCurrentVersion(const String& currentVersion)`                                                                                    |      Yes     |      Yes      |
| `getCurrentVersion() const`                                                                                                          |      Yes     |      Yes      |
| `isNewVersion(const String& release)`                                                                                                |      Yes     |      Yes      |
| `isUpToDate(const String& release)`                                                                                                  |      Yes     |      Yes      |
| `setCredentials(const String& projectId, const String& apiKey)`                                                                      |      Yes     |       No      |
| `setBaseURL(const String& url)`                                                                                                      |      Yes     |       No      |
| `setReleaseURL(const String& endpoint, const std::vector<Header> headers = {})`                                                      |      No      |      Yes      |


### Quick Start (VoyagerOTA)

> [!IMPORTANT]
>
> 1. The `__ENABLE_DEVELOPMENT_MODE__` must be declared at the top either as true or false. As this compile time flag is required only for VoyagerOTA platform.
> 2. Firmware uploaded to VoyagerOTA must be built with `__ENABLE_DEVELOPMENT_MODE__` false. Development compiled builds will be rejected by the VoyagerOTA platform.
> 3. The library uses staging and production channels. Production builds first go to the **staging** channel for testing.
> 4. On your local device, you can temporarily set `__ENABLE_DEVELOPMENT_MODE__` true to fetch the **staging** release.
> 5. After testing, promote the release to **production** to make it available to all devices.

```cpp

// Development mode is for for staging environment builds for testing....
// Make sure to always disable the [__ENABLE_DEVELOPMENT_MODE__] flag to false
// while uploading the Binary to VoyagerOTA Platform as development builds are not
// allowed....And by disabling it the build is treated as production build...
#define __ENABLE_DEVELOPMENT_MODE__ true
#define CURRENT_FIRMWARE_VERSION "1.0.0"

#include <VoyagerOTAClient.h>
#include <WiFi.h>
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
    OTA<HTTPResponseData, VoyagerReleaseModel> ota(CURRENT_FIRMWARE_VERSION);

    ota.setCredentials("voyager-project-id-here....", "voyager-api-key-here...");
    ota.setBaseURL("voyager-base-url.....");

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
> `__ENABLE_ADVANCED_MODE__` compile time flag must be set to `true` to enable custom backend support with parsers for integration with any backend.
> **All custom payload models must extend BaseModel.**
> Each model inherits the following required fields from BaseModel:
>
> * *version* - the release version string used for semver comparison.
> * *downloadURL* - the URL of the firmware binary to download.
>
> **Voyager-specific features are disabled in this mode!!!**


### 1. Custom OTA Backend Support Example

```cpp
// required to enable more manual settings and disable voyager related features....
#define __ENABLE_ADVANCED_MODE__ true
#define CURRENT_FIRMWARE_VERSION "1.0.0"

#include <VoyagerOTAClient.h>
#include <ArduinoJson.hpp>

void connectToWifi() {
    WiFi.begin("SSID", "PASSWORD");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(50);
    }
    Serial.println("Connected to Internet");
}

struct CustomModel : public Voyager::BaseModel {
    String description;
    int statusCode;

    explicit CustomModel(String version, String downloadUrl, String desc, int code)
        : BaseModel(version, downloadUrl), description(desc), statusCode(code) {}
};

class CustomParser : public Voyager::IParser<Voyager::HTTPResponseData, CustomModel> {
public:
    std::optional<CustomModel> parse(Voyager::HTTPResponseData responseData, int statusCode) override {
        ArduinoJson::JsonDocument document;
        ArduinoJson::DeserializationError error = ArduinoJson::deserializeJson(document, responseData);

        if (error) {
            Serial.println("JSON parsing failed");
            return std::nullopt;
        }

        if (statusCode != HTTP_CODE_OK) {
            return std::nullopt;
        }

        CustomModel payload(document["version"],
                            document["downloadUrl"],
                            document["description"],
                            statusCode);

        return payload;
    }
};

void setup() {
    Serial.begin(9600);
    connectToWifi();
    auto parser = std::make_unique<CustomParser>();
    Voyager::OTA<Voyager::HTTPResponseData, CustomModel> ota(CURRENT_FIRMWARE_VERSION, std::move(parser));

    ota.setReleaseURL("https://your-custom-backend/releases/latest");
    auto release = ota.fetchLatestRelease();
    if (release && ota.isNewVersion(release->version)) {
        Serial.println(release->downloadURL);
        Serial.println(release->description);
        ota.performUpdate();
    }
}

void loop() {}
```

### 2. Github Release OTA Example

```cpp
#define __ENABLE_ADVANCED_MODE__ true
#define CURRENT_FIRMWARE_VERSION "1.0.0"

#include <VoyagerOTAClient.h>
#include <WiFi.h>

using namespace Voyager;

void connectToWifi() {
    WiFi.begin("SSID", "PASSWORD");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(50);
    }
    Serial.println("Connected to Internet");
}

struct GithubReleaseModel : public BaseModel {
    String name;
    String publishedAt;
    int size;
    int statusCode;

    explicit GithubReleaseModel(String version, String downloadURL, String publishedAt, String name, int size, int statusCode)
        : BaseModel(version, downloadURL) {
        this->name = name;
        this->publishedAt = publishedAt;
        this->size = size;
        this->statusCode = statusCode;
    }
};

class GithubJSONParser : public Voyager::IParser<Voyager::HTTPResponseData, GithubReleaseModel> {
public:
    std::optional<GithubReleaseModel> parse(Voyager::HTTPResponseData responseData, int statusCode) override {
        JsonDocument document;
        DeserializationError error = deserializeJson(document, responseData);

        if (error) {
            Serial.println("JSON parsing failed");
            return std::nullopt;
        }

        if (statusCode != HTTP_CODE_OK) {
            return std::nullopt;
        }

        GithubReleaseModel payload(
            document[0]["tag_name"],
            document[0]["assets"][0]["url"],
            document[0]["published_at"],
            document[0]["name"],
            document[0]["assets"][0]["size"].template as<int>(),
            statusCode);

        return payload;
    }
};

void setup() {
    Serial.begin(9600);
    connectToWifi();

    auto parser = std::make_unique<GithubJSONParser>();
    OTA<HTTPResponseData, GithubReleaseModel> ota(CURRENT_FIRMWARE_VERSION, std::move(parser));
      
    // https://docs.github.com/en/rest/releases/releases?apiVersion=2022-11-28#:~:text=GET-,/repos/%7Bowner%7D/%7Brepo%7D/releases,-cURL
    std::vector<Header> releaseHeaders = {
        {"Authorization", "Bearer your-github-token"},
        {"X-GitHub-Api-Version", "2022-11-28"},
        {"Accept", "application/vnd.github+json"},
    };

    // replace with your github username and repo.......
    ota.setReleaseURL("https://api.github.com/repos/{username}/{repo-name}/releases");

    Serial.println("OTA Started....");

    auto release = ota.fetchLatestRelease();

    if (release && ota.isNewVersion(release->version)) {
        Serial.println("New version available: " + release->version);
        Serial.println("Release Name: " + release->name);
        Serial.println("Release Date: " + release->publishedAt);

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

## Requirements

- C++17 or higher
- ArduinoJson library version 7.0 or above
- [cpp-semver](http://github.com/z4kn4fein/cpp-semver) - v0.4.0
- [HTTPUpdate](https://github.com/espressif/arduino-esp32/tree/master/libraries/Update) - v3.0.7

## License

This project is licensed under the MIT License. See the [LICENSE](https://github.com/mediocre9/VoyagerOTAClient/blob/main/LICENSE) for details.
