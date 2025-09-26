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
    OTA<> ota("1.0.0");

    ota.setEnvironment(PRODUCTION);
    ota.setCredentials("voyager-project-id-here....", "voyager-api-key-here...");

    auto release = ota.fetchLatestRelease();

    if (release && ota.isNewVersion(release->version)) {
        Serial.println("New version available: " + release->version);
        Serial.println("Changelog: " + release->changeLog);
        ota.performUpdate();
    } else {
        Serial.println("No updates available");
    }
}

void loop() {}