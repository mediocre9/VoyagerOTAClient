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

    std::unique_ptr<GithubJSONParser> parser = std::make_unique<GithubJSONParser>();
    OTA<HTTPResponseData, GithubReleaseModel> ota(std::move(parser), "1.0.0");

    ota.setReleaseURL("https://api.github.com/repos/owner/repo/releases/latest",
                      {
                          {"Authorization", "Bearer your-github-token"},
                          {"X-GitHub-Api-Version", "2022-11-28"},
                          {"Accept", "application/vnd.github+json"},
                      });

    Serial.println("OTA Started....");

    auto release = ota.fetchLatestRelease();

    if (release && ota.isNewVersion(release->version)) {
        Serial.println("New version available: " + release->version);
        Serial.println("Release name: " + release->name);
        ota.setDownloadURL(release->browserDownloadUrl,
                           {
                               {"Authorization", "Bearer your-github-token..."},
                               {"X-GitHub-Api-Version", "2022-11-28"},
                               {"Accept", "application/octet-stream"},
                           });

        ota.performUpdate();
    } else {
        Serial.println("No updates available yet!");
    }
}

void loop() {}