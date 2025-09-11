#include <WiFi.h>
#include <VoyagerOTA.hpp>

using namespace Voyager;

void connectToInternet() {
    WiFi.begin("SSID", "PASSWORD");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(50);
    }
    Serial.println("Connected to Internet");
}

void setup() {
    Serial.begin(9600);
    connectToInternet();
    OTA<> ota("1.0.0");

    ota.setEnvironment(PRODUCTION);
    ota.setGlobalHeaders({
        {"X-API-Key", "voyager-api-key-here..."},
        {"X-Project-Id", "voyager-project-id"},
    });

    auto release = ota.fetchLatestRelease();

    if (release && ota.isNewVersion(release->version)) {
        Serial.println("New version available: " + release->version);
        Serial.println("Changelog: " + release->changeLog);
        ota.performUpdate();
    } else {
        Serial.println("No update available");
    }
}

void loop() {}