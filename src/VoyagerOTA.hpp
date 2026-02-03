/******************************************************************************
 * MIT License
 *
 * @headerfile [VoyagerOTA.hpp]
 *
 * @description: An easy to use small OTA client library for the VoyagerOTA
 * platform, compatible with ESP32 and ESP8266 devices.
 *
 * @copyright (c) 2025
 * @author: fahadziakhan9@gmail.com (Fahad Zia Khan / Mediocre9)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

#ifndef MEDIOCRE9_VOYAGER_OTA_H
#define MEDIOCRE9_VOYAGER_OTA_H

#if __cplusplus < 201703L
  #error "This library requires a compiler of C++17 or above."
#endif

#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WString.h>
#include <WiFi.h>
#include <ArduinoJson.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <semver.hpp>
#include <vector>

#if !__ENABLE_ADVANCED_MODE__
  #include "ApiConstants.hpp"
#endif

#if !defined(ARDUINOJSON_VERSION_MAJOR) || ARDUINOJSON_VERSION_MAJOR < 7
  #error VoyagerOTA requires the ArduinoJson library version 7.0 or above.
#endif

#define VOYAGER_OTA_VERSION "2.1.0"
#define VOYAGER_OTA_VERSION_MAJOR 2
#define VOYAGER_OTA_VERSION_MINOR 1
#define VOYAGER_OTA_VERSION_PATCH 0

// !Do NOT change....For Platform's Backend use only......
#if defined(__ENABLE_ADVANCED_MODE__) && (__ENABLE_ADVANCED_MODE__ == true)
  #pragma message("VoyagerOTA Advanced Mode Enabled! All VoyagerOTA related features has been disabled!")
#else
  #if !defined(__ENABLE_DEVELOPMENT_MODE__) && (__ENABLE_DEVELOPMENT_MODE__ == false)
    #error __ENABLE_DEVELOPMENT_MODE__ MACRO is missing. Please define at the top of the code either as true or false.
  #endif

    //   https://stackoverflow.com/questions/31637626/whats-the-usecase-of-gccs-used-attribute#:~:text=If%20you%20declare%20a%20global%20variable%20or%20function%20that%20is%20unused%2C%20gcc%20will%20optimized%20it%20out%20(with%20warning)%2C%20but%20if%20you%20declared%20the%20global%20variable%20or%20the%20function%20with%20%27__attribute__((used))%27%2C%20gcc%20will%20include%20it%20in%20object%20file%20(and%20linked%20executable).
  #if __ENABLE_DEVELOPMENT_MODE__
const char* ___VYGR_DEVELOPMENT___ __attribute__((used)) = "$2y$10$BsbB6jZbeQKLLnsnvGRJfOmGuG2Co0/LEDR4xO0Khnlvvm57c6Tai";
    #pragma message("[VoyagerOTA-WARNING]: Do not Upload DEVELOPMENT enabled builds on VoyagerOTA Platform.");
  #else
const char* ___VYGR_PRODUCTION___ __attribute__((used)) = "$2y$10$DX0bqDwfQtWJkBPgiXHVqOcbjOoX5i9cRHxSTgK3xgjTHpy5EGNbO";
  #endif
#endif

using ArduinoJson::DeserializationError;
using ArduinoJson::deserializeJson;
using ArduinoJson::JsonDocument;

namespace Voyager {
    using Header = std::pair<const char*, const char*>;

    struct BaseModel {
        String version;

        explicit BaseModel(String v) : version(v) {}
    };

#if __ENABLE_ADVANCED_MODE__
    struct GithubReleaseModel : public BaseModel {
        String name;
        String publishedAt;
        String browserDownloadUrl;
        int size;
        int statusCode;

        explicit GithubReleaseModel(String version, String name, String publishedAt, String browserDownloadUrl, int size, int statusCode) : BaseModel(version) {
            this->name = name;
            this->publishedAt = publishedAt;
            this->browserDownloadUrl = browserDownloadUrl;
            this->size = size;
            this->statusCode = statusCode;
        }
    };
#else
    struct VoyagerReleaseModel : public BaseModel {
        String releaseId;
        String changeLog;
        String releasedDate;
        String status;
        int statusCode;
        String hash;
        int size;
        String prettySize;
        String downloadURL;
        String message;

        explicit VoyagerReleaseModel(String version, String releaseId, String changeLog, String releasedDate, String status, int statusCode, String hash, int size, String prettySize, String downloadURL, String message = String()) : BaseModel(version) {
            this->releaseId = releaseId;
            this->changeLog = changeLog;
            this->releasedDate = releasedDate;
            this->status = status;
            this->statusCode = statusCode;
            this->hash = hash;
            this->size = size;
            this->prettySize = prettySize;
            this->downloadURL = downloadURL;
            this->message = message;
        }
    };
#endif

    using HTTPResponseData = String;
    template <typename T_ResponseData = Voyager::HTTPResponseData, typename T_PayloadModel = Voyager::VoyagerReleaseModel>
    class IParser {
        static_assert(std::is_base_of_v<BaseModel, T_PayloadModel>, "T_PayloadModel should be extended from BaseModel!");

    public:
        [[nodiscard]] virtual std::optional<T_PayloadModel> parse(T_ResponseData responseData, int statusCode) = 0;

        virtual ~IParser() = default;
    };

#if __ENABLE_ADVANCED_MODE__
    class GithubJSONParser final : public Voyager::IParser<Voyager::HTTPResponseData, GithubReleaseModel> {
    public:
        GithubJSONParser() = default;

        [[nodiscard]] std::optional<GithubReleaseModel> parse(Voyager::HTTPResponseData responseData, int statusCode) override;
    };
#else
    class VoyagerJSONParser final : public IParser<Voyager::HTTPResponseData, Voyager::VoyagerReleaseModel> {
    public:
        VoyagerJSONParser() = default;

        [[nodiscard]] std::optional<Voyager::VoyagerReleaseModel> parse(Voyager::HTTPResponseData responseData, int statusCode) override;
    };
#endif

    template <typename T_PayloadModel>
    class BaseOTA {
        static_assert(std::is_base_of_v<BaseModel, T_PayloadModel>, "Model should be extended from BaseModel!");

    public:
        virtual void performUpdate() = 0;

        [[nodiscard]] virtual std::optional<T_PayloadModel> fetchLatestRelease() = 0;

        virtual ~BaseOTA() = default;
    };

    template <typename T_ResponseData = Voyager::HTTPResponseData, typename T_PayloadModel = Voyager::VoyagerReleaseModel>
    class OTA : public BaseOTA<T_PayloadModel> {
        static_assert(std::is_base_of_v<BaseModel, T_PayloadModel>, "Model should be extended from BaseModel!");

    public:
        using Parser = std::unique_ptr<IParser<T_ResponseData, T_PayloadModel>>;

        OTA() = default;

        explicit OTA(const String& currentVersion);

        explicit OTA(const String& currentVersion, Parser parser);

        explicit OTA(Parser parser);

        void setParser(Parser parser);

#if __ENABLE_ADVANCED_MODE__
        void setReleaseURL(const String& endpoint, std::vector<Header> headers = std::vector<Header>());
#else
        void setCredentials(const String& projectId, const String& apiKey);
#endif
        void setDownloadURL(const String& endpoint, std::vector<Header> headers = std::vector<Header>());

        void setCurrentVersion(const String& currentVersion);

        [[nodiscard]] const String& getCurrentVersion() const;

        [[nodiscard]] bool isNewVersion(const String& release);

        [[nodiscard]] bool isCurrentVersion(const String& release);

        void performUpdate() override;

        [[nodiscard]] std::optional<T_PayloadModel> fetchLatestRelease() override;

        ~OTA() = default;

    private:
        void _otaUpdateHandler(HTTPClient& client);

    private:
        Parser _parser;
        String _currentVersion;

        String _downloadURL;
        std::vector<Header> _downloadHeaders;

#if __ENABLE_ADVANCED_MODE__
        String _releaseURL;
        std::vector<Header> _releaseHeaders;
#else
        String _apiKey;
        String _projectId;
        std::vector<Header> _voyagerHeaders;

    private:
        void _setVoyagerHeaders(std::vector<Header> headers);
#endif
    };
}  // namespace Voyager

template <typename T_ResponseData, typename T_PayloadModel>
Voyager::OTA<T_ResponseData, T_PayloadModel>::OTA(const String& currentVersion)
    : _currentVersion(currentVersion) {
    _parser = std::make_unique<VoyagerJSONParser>();
}

template <typename T_ResponseData, typename T_PayloadModel>
Voyager::OTA<T_ResponseData, T_PayloadModel>::OTA(const String& currentVersion, Parser parser)
    : _currentVersion(currentVersion), _parser(std::move(parser)) {}

template <typename T_ResponseData, typename T_PayloadModel>
Voyager::OTA<T_ResponseData, T_PayloadModel>::OTA(Parser parser)
    : _parser(std::move(parser)) {}

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::setParser(Parser parser) {
    if (_parser == nullptr) {
        _parser = std::move(parser);
    }
}

#if __ENABLE_ADVANCED_MODE__
template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::setReleaseURL(const String& endpoint, std::vector<Header> headers) {
    _releaseURL = endpoint;
    _releaseHeaders = headers;
}
#else
template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::setCredentials(const String& projectId, const String& apiKey) {
    _projectId = projectId;
    _apiKey = apiKey;
    _setVoyagerHeaders({
        {__VoyagerApi__::Headers::Keys::X_PROJECT_ID, _projectId.c_str()},
        {__VoyagerApi__::Headers::Keys::X_API_KEY, _apiKey.c_str()},
    });
}

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::_setVoyagerHeaders(std::vector<Header> headers) {
    _voyagerHeaders = headers;
}
#endif

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::setDownloadURL(const String& endpoint, std::vector<Header> headers) {
    _downloadURL = endpoint;
    _downloadHeaders = headers;
}

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::setCurrentVersion(const String& currentVersion) {
    _currentVersion = currentVersion;
}

template <typename T_ResponseData, typename T_PayloadModel>
const String& Voyager::OTA<T_ResponseData, T_PayloadModel>::getCurrentVersion() const {
    return _currentVersion;
}

template <typename T_ResponseData, typename T_PayloadModel>
bool Voyager::OTA<T_ResponseData, T_PayloadModel>::isNewVersion(const String& release) {
    return semver::version::parse(release.c_str(), false) > semver::version::parse(_currentVersion.c_str(), false);
}

template <typename T_ResponseData, typename T_PayloadModel>
bool Voyager::OTA<T_ResponseData, T_PayloadModel>::isCurrentVersion(const String& release) {
    return semver::version::parse(release.c_str(), false) == semver::version::parse(_currentVersion.c_str(), false);
}

template <typename T_ResponseData, typename T_PayloadModel>
std::optional<T_PayloadModel> Voyager::OTA<T_ResponseData, T_PayloadModel>::fetchLatestRelease() {
    String url;
#if __ENABLE_ADVANCED_MODE__
    if (_releaseURL.isEmpty()) {
        Serial.println("Release URL is required!");
        return std::nullopt;
    }

    url = _releaseURL;

#else
    if (_apiKey.isEmpty()) {
        Serial.println("API Key is required!");
        return std::nullopt;
    }

    if (_projectId.isEmpty()) {
        Serial.println("Project Id is required!");
        return std::nullopt;
    }
  #if __ENABLE_DEVELOPMENT_MODE__
    url = __VoyagerApi__::BASE_URL + __VoyagerApi__::Endpoints::LATEST_RELEASE + __VoyagerApi__::QueryParams::STAGING_CHANNEL;
  #else
    url = __VoyagerApi__::BASE_URL + __VoyagerApi__::Endpoints::LATEST_RELEASE + __VoyagerApi__::QueryParams::PRODUCTION_CHANNEL;
  #endif
#endif

    // TODO Deprecate the HTTPClient module in favour of AsyncTCP client for async API calls.......
    HTTPClient client;

    bool isOK = client.begin(url);

    // TODO add log message.......
    if (!isOK) {
        return std::nullopt;
    }

#if __ENABLE_ADVANCED_MODE__
    std::vector<Header> headers = _releaseHeaders;
#else
    std::vector<Header> headers = _voyagerHeaders;
#endif

    for (const auto [type, value] : headers) {
        if (!client.hasHeader(type)) {
            client.addHeader(type, value);
        }
    }

    int statusCode = client.GET();
    Voyager::HTTPResponseData responseData = client.getString();
    client.end();
    return _parser->parse(responseData, statusCode);
}

#if __ENABLE_ADVANCED_MODE__
std::optional<Voyager::GithubReleaseModel> Voyager::GithubJSONParser::parse(Voyager::HTTPResponseData responseData, int statusCode) {
    ArduinoJson::JsonDocument document;
    ArduinoJson::DeserializationError error = ArduinoJson::deserializeJson(document, responseData);

    if (error) {
        Serial.printf("VoyagerOTA JSON Error : %s\n", error.c_str());
        return std::nullopt;
    }

    // TODO Add error log message...
    if (statusCode != HTTP_CODE_OK) {
        return std::nullopt;
    }

    GithubReleaseModel payload(
        document["tag_name"],
        document["name"],
        document["published_at"],
        document["assets"][0]["url"],
        document["assets"][0]["size"].template as<int>(),
        statusCode);

    return payload;
}
#else
std::optional<Voyager::VoyagerReleaseModel> Voyager::VoyagerJSONParser::parse(Voyager::HTTPResponseData responseData, int statusCode) {
    JsonDocument document;
    DeserializationError error = deserializeJson(document, responseData);

    if (error) {
        Serial.printf("VOYAGER_OTA_JSON_Error : %s\n", error.c_str());
        return std::nullopt;
    }

    if (statusCode != HTTP_CODE_OK) {
        String errorMessage = document["message"];
        Serial.println(errorMessage);
        std::nullopt;
    }

    Voyager::VoyagerReleaseModel payload(document["release"]["version"],
                                         document["release"]["id"],
                                         document["release"]["changeLog"],
                                         document["release"]["releasedAt"],
                                         document["release"]["status"],
                                         statusCode,
                                         document["release"]["artifact"]["hash"],
                                         document["release"]["artifact"]["size"].as<int>(),
                                         document["release"]["artifact"]["prettySize"],
                                         document["release"]["artifact"]["downloadURL"]);

    return payload;
}
#endif

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::performUpdate() {
    HTTPClient client;
    if (_downloadURL.isEmpty()) {
        Serial.print("Download URL is required!");
        return;
    }

    bool isOK = client.begin(_downloadURL);

    // TODO Add error log message.....
    if (!isOK) {
        return;
    }

    std::vector<Header> headers = _voyagerHeaders.empty() ? _downloadHeaders : _voyagerHeaders;
    for (const auto [type, value] : headers) {
        if (!client.hasHeader(type)) {
            client.addHeader(type, value);
        }
    }

    _otaUpdateHandler(client);
}
#endif

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::performUpdate() {
    HTTPClient client;
    if (_downloadURL.isEmpty()) {
        Serial.print("Download URL is required!");
        return;
    }

    bool isOK = client.begin(_downloadURL);

    // TODO Add error log message.....
    if (!isOK) {
        return;
    }

    std::vector<Header> headers = _voyagerHeaders.empty() ? _downloadHeaders : _voyagerHeaders;
    for (const auto [type, value] : headers) {
        if (!client.hasHeader(type)) {
            client.addHeader(type, value);
        }
    }

    _otaUpdateHandler(client);
}

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::_otaUpdateHandler(HTTPClient& client) {
    httpUpdate.onStart([this]() -> void {
        Serial.println("==== VoyagerOTA update has been started! ====");
    });

    httpUpdate.onProgress([this](int current, int total) -> void {
        int percent = (current * 100) / total;
        Serial.printf("==== Downloading: %d out of 100%% ====\n", percent);
    });

    httpUpdate.onEnd([this]() -> void {
        Serial.println("==== VoyagerOTA update has finished! ====");
    });

    httpUpdate.onError([this](int errorCode) -> void {
        Serial.printf("==== VoyagerOTA Update Error Code : %d ====", errorCode);
    });

    httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    t_httpUpdate_return updateResult = httpUpdate.update(client);

    switch (updateResult) {
        case HTTP_UPDATE_OK: {
            Serial.println("VOYAGER_OTA HTTP_UPDATE_OK");
        } break;

        case HTTP_UPDATE_FAILED: {
            Serial.printf("VOYAGER_OTA HTTP_UPDATE_FAILED : %s  ERROR CODE :  %d", httpUpdate.getLastErrorString(), httpUpdate.getLastError());
        } break;

        default: {
            Serial.println("VOYAGER_OTA NO_HTTP_UPDATE_AVAILABLE");
        } break;
    }

    client.end();
}
#endif
