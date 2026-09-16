# VoyagerOTAClient
[![License](https://img.shields.io/github/license/mediocre9/voyagerota-core?color=0891B2)](LICENSE)
[![SDK](https://img.shields.io/github/v/release/mediocre9/VoyagerOTAClient?label=SDK%20Latest%20Release\&color=008000)](https://github.com/mediocre9/VoyagerOTAClient/releases/latest)

> A semver-based OTA client SDK for the <a href="https://github.com/mediocre9/voyagerota-core">VoyagerOTA</a>. That also supports structured way for GitHub Releases and custom JSON backend for ESP32 OTA.

## Getting Started

### Quick Start (VoyagerOTA)

> For VoyagerOTA, define `__USE_STAGING_CHANNEL__` as `true` or `false` before including `VoyagerOTAClient.h`. This macro determines
> which release channel the OTA client uses when fetching release metadata.
> When `__USE_STAGING_CHANNEL__` is set to `true`, the client fetches the latest release from the staging channel, which is for
> testing and development. When set to `false`, it uses the production channel, which is for production devices.
> 
> **Kindly do note that the firmware compiled with `__USE_STAGING_CHANNEL__` set to `true` is identified as a staging build by
> VoyagerOTA and will be rejected during the production build inspection process. Therefore, always set `__USE_STAGING_CHANNEL__` to
> `false` before uploading a production firmware binary to the VoyagerOTA platform.**

```cpp

// Staging mode is for for staging environment builds for testing....
// Make sure to always disable the [__USE_STAGING_CHANNEL__] flag to false
// while uploading the Binary to VoyagerOTA Platform as staging builds are not
// allowed....And by disabling it the build is treated as production build...
#define __USE_STAGING_CHANNEL__ true
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

## Advanced Mode

Enable advanced mode for custom OTA backend support:

```cpp
#define __ENABLE_ADVANCED_MODE__ true

#include <VoyagerOTAClient.h>
```

All custom payload models must extend `Voyager::BaseModel`.

Each model inherits:

* `version` release used for semver comparison.
* `downloadURL` of the firmware binary.

**Voyager-specific features are disabled in advanced mode.**

## Examples

* [VoyagerOTA Example](https://github.com/mediocre9/VoyagerOTAClient/blob/main/examples/VoyagerOTA/VoyagerOTA.ino)
* [GitHub OTA Example](https://github.com/mediocre9/VoyagerOTAClient/blob/main/examples/GithubOTA/GithubOTA.ino) - (Advanced Mode)
* [Custom Backend Example](https://github.com/mediocre9/VoyagerOTAClient/blob/main/examples/CustomOTA/CustomOTA.ino) - (Advanced Mode)

## Requirements

* C++17 or higher
* [ArduinoJson](https://arduinojson.org/) version 7.0 or above
* [cpp-semver](http://github.com/z4kn4fein/cpp-semver) v0.4.0
* [HTTPUpdate](https://github.com/espressif/arduino-esp32/tree/master/libraries/Update) v3.0.7

## License

This project is licensed under the MIT License. See the [LICENSE](https://github.com/mediocre9/VoyagerOTAClient/blob/main/LICENSE) file for details.
