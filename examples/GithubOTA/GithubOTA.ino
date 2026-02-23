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

        ota.setDownloadURL(release->browserDownloadUrl, downloadHeaders);
        ota.performUpdate();
    } else {
        Serial.println("No updates available yet!");
    }
}

void loop() {}