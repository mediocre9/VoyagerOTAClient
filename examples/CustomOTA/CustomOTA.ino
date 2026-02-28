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