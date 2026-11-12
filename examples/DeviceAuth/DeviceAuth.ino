// Device approval: every device must be approved in the VoyagerOTA dashboard
// before it can pull firmware. On first boot the device asks for access and
// shows up under Devices -> "waiting for approval". Once approved it receives
// its own token (stored in NVS) and update results are reported back.
#define __USE_STAGING_CHANNEL__ false
#define CURRENT_FIRMWARE_VERSION "1.0.0"

#include <VoyagerOTAClient.h>
#include <WiFi.h>
using namespace Voyager;

OTA<HTTPResponseData, VoyagerReleaseModel> ota(CURRENT_FIRMWARE_VERSION);

void connectToWifi() {
    WiFi.begin("SSID", "PASSWORD");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }
    Serial.println("\nConnected to Internet");
}

void setup() {
    Serial.begin(115200);
    connectToWifi();

    ota.setCredentials("voyager-project-id-here....", "voyager-api-key-here...");
    ota.setBaseURL("voyager-base-url.....");

    Serial.println("Hardware id: " + ota.getHardwareId());
}

void loop() {
    // registers automatically, only approved devices get a release back....
    auto release = ota.fetchLatestRelease();

    if (release && ota.isNewVersion(release->version)) {
        Serial.println("Updating to v" + release->version);
        ota.setDownloadURL(release->downloadURL);
        ota.performUpdate();  // failures are reported right away, success after the reboot
    } else if (ota.getDeviceStatus() == DeviceStatus::PENDING) {
        Serial.println("Approve this device in the dashboard...");
    } else if (ota.getDeviceStatus() == DeviceStatus::REVOKED) {
        Serial.println("This device was revoked.");
    }

    delay(60 * 1000);
}
