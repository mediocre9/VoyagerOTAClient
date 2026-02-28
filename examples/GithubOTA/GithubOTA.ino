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