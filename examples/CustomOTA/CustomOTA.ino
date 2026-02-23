#define __ENABLE_ADVANCED_MODE__ true
#define CURRENT_FIRMWARE_VERSION "1.0.0"

#include <ArduinoJson.hpp>
#include <VoyagerOTA.hpp>

struct CustomModel : public Voyager::BaseModel {
    String description;
    int statusCode;
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
                            document["description"],
                            document["downloadUrl"],
                            statusCode);

        return payload;
    }
};

void setup() {
    Serial.begin(9600);
    auto parser = std::make_unique<CustomParser>();
    Voyager::OTA<Voyager::HTTPResponseData, CustomModel> ota(std::move(parser), CURRENT_FIRMWARE_VERSION);

    ota.setReleaseURL("https://your-custom-backend/firmware/latest");
    auto release = ota.fetchLatestRelease();
    if (release && ota.isNewVersion(release->version)) {
        ota.setDownloadURL(release->downloadURL);
        ota.performUpdate();
    }
}

void loop() {}