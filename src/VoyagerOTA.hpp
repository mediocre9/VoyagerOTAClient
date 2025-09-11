/******************************************************************************
 * MIT License
 *
 * @headerfile [VoyagerOTA.hpp]

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
#error "This library requires a compiler of c++ 17 or above."
#endif

#include <ArduinoJson.h>
#include <HTTPUpdate.h>
#include <WString.h>
#include <WiFi.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <semver.hpp>

#if !defined(ARDUINOJSON_VERSION_MAJOR) || ARDUINOJSON_VERSION_MAJOR < 7
#error VoyagerOTA requires the ArduinoJson library version 7.0 or above.
#endif

#define VOYAGER_OTA_VERSION "1.0.0"
#define VOYAGER_OTA_VERSION_MAJOR 1
#define VOYAGER_OTA_VERSION_MINOR 0
#define VOYAGER_OTA_VERSION_PATCH 0

#define ENVIRONMENT_WARNING_LOG_STATUS                                                            \
    if (_environment == STAGING) {                                                                \
        for (std::uint16_t i = 0; i < 2; i++) {                                                   \
            Serial.println("\n[WARNING]: VoyagerOTA running in STAGING mode.");                   \
            Serial.print("For further details please visit: https://voyagerota.com/api/v1/docs"); \
        }                                                                                         \
    }

using ArduinoJson::DeserializationError;
using ArduinoJson::deserializeJson;
using ArduinoJson::JsonDocument;

namespace Voyager {
    enum Environment {
        STAGING,
        PRODUCTION
    };

    struct VoyagerReleaseModel {
        String version;
        String releaseId;
        String changeLog;
        String releasedDate;
        String prettyFileSize;
        String boardType;
        String environment;
        String message;
        int fileSizeInBytes;
        int statusCode;
    };

    struct GithubReleaseModel {
        String version;
        String name;
        String publishedAt;
        String browserDownloadUrl;
        int size;
        int statusCode;
    };

    using HTTPResponseData = String;
    template <typename T_ResponseData = Voyager::HTTPResponseData, typename T_PayloadModel = Voyager::VoyagerReleaseModel>
    class IParser {
    public:
        [[nodiscard]] virtual std::optional<T_PayloadModel> parse(T_ResponseData responseData, int statusCode) = 0;
        virtual ~IParser() = default;
    };

    class VoyagerJSONParser : public IParser<Voyager::HTTPResponseData, Voyager::VoyagerReleaseModel> {
    public:
        VoyagerJSONParser() = default;
        [[nodiscard]] std::optional<Voyager::VoyagerReleaseModel> parse(Voyager::HTTPResponseData responseData, int statusCode) override;
    };

    class GithubJSONParser : public Voyager::IParser<Voyager::HTTPResponseData, GithubReleaseModel> {
    public:
        GithubJSONParser() = default;
        std::optional<GithubReleaseModel> parse(Voyager::HTTPResponseData responseData, int statusCode) override;
    };

    template <typename T_PayloadModel>
    class BaseOTA {
    public:
        virtual void performUpdate() = 0;
        [[nodiscard]] virtual std::optional<T_PayloadModel> fetchLatestRelease() = 0;
        virtual ~BaseOTA() = default;
    };

    template <typename T_ResponseData = Voyager::HTTPResponseData, typename T_PayloadModel = Voyager::VoyagerReleaseModel>
    class OTA : public BaseOTA<T_PayloadModel> {
    public:
        using Header = std::pair<String, String>;

        OTA() = default;

        explicit OTA(const String& currentVersion);

        explicit OTA(std::unique_ptr<IParser<T_ResponseData, T_PayloadModel>> parser, const String& currentVersion);

        explicit OTA(std::unique_ptr<IParser<T_ResponseData, T_PayloadModel>> parser);

        void setParser(std::unique_ptr<IParser<T_ResponseData, T_PayloadModel>> parser);

        void setReleaseURL(const String& endpoint, std::vector<Header> headers = std::vector<Header>());

        void setDownloadURL(const String& endpoint, std::vector<Header> headers = std::vector<Header>());

        void setCurrentVersion(const String& currentVersion);

        void setGlobalHeaders(std::vector<Header> headers);

        void setEnvironment(Environment environment);

        [[nodiscard]] const String& getCurrentVersion() const;

        [[nodiscard]] bool isNewVersion(const String& release);

        void performUpdate() override;

        [[nodiscard]] std::optional<T_PayloadModel> fetchLatestRelease() override;

        ~OTA() = default;

    private:
        void _otaUpdateHandler(HTTPClient& client);

        inline void _registerHTTPUpdateCallbacks();

    private:
        std::vector<Header> _releaseHeaders;
        std::vector<Header> _downloadHeaders;
        std::vector<Header> _globalHeaders;
        std::unique_ptr<IParser<T_ResponseData, T_PayloadModel>> _parser;
        Environment _environment = STAGING;  // defaults to staging mode for testing and safety......

        String _currentVersion;
        String _customReleaseURL;
        String _customDownloadURL;
        String _voyagerReleaseURL = "https://www.voyagerota.com/api/v1/releases/latest";
        String _voyagerDownloadURL = "https://www.voyagerota.com/api/v1/releases/latest/download";
    };
}  // namespace Voyager

template <typename T_ResponseData, typename T_PayloadModel>
Voyager::OTA<T_ResponseData, T_PayloadModel>::OTA(const String& currentVersion)
    : _currentVersion(currentVersion) {
    _parser = std::make_unique<VoyagerJSONParser>();
}

template <typename T_ResponseData, typename T_PayloadModel>
Voyager::OTA<T_ResponseData, T_PayloadModel>::OTA(std::unique_ptr<IParser<T_ResponseData, T_PayloadModel>> parser, const String& currentVersion)
    : _parser(std::move(parser)), _currentVersion(currentVersion) {}

template <typename T_ResponseData, typename T_PayloadModel>
Voyager::OTA<T_ResponseData, T_PayloadModel>::OTA(std::unique_ptr<IParser<T_ResponseData, T_PayloadModel>> parser)
    : _parser(std::move(parser)) {}

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::setParser(std::unique_ptr<IParser<T_ResponseData, T_PayloadModel>> parser) {
    if (_parser == nullptr) {
        _parser = std::move(parser);
    }
}

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::setEnvironment(Voyager::Environment environment) {
    _environment = environment;
}

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::setReleaseURL(const String& endpoint, std::vector<Header> headers) {
    _customReleaseURL = endpoint;
    if (!_globalHeaders.empty()) {
        Serial.println("VOYAGER_OTA_HEADER_ERROR: Can't initialize Release URL Headers due to globally initialized request headers!");
        return;
    }
    _releaseHeaders = headers;
}

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::setDownloadURL(const String& endpoint, std::vector<Header> headers) {
    _customDownloadURL = endpoint;
    if (!_globalHeaders.empty()) {
        Serial.println("VOYAGER_OTA_HEADER_ERROR: Can't initialize Download URL Headers due to globally initialized request headers!");
        return;
    }
    _downloadHeaders = headers;
}

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::setCurrentVersion(const String& currentVersion) {
    _currentVersion = currentVersion;
}

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::setGlobalHeaders(std::vector<Header> headers) {
    _globalHeaders = headers;
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
std::optional<T_PayloadModel> Voyager::OTA<T_ResponseData, T_PayloadModel>::fetchLatestRelease() {
    String withAppendedQueryParam = (_environment == PRODUCTION)
                                        ? _voyagerReleaseURL + "/?mode=production"
                                        : _voyagerReleaseURL + "/?mode=staging";

    String url = _customReleaseURL.isEmpty() ? withAppendedQueryParam : _customReleaseURL;

    if (_customReleaseURL.isEmpty()) {
        ENVIRONMENT_WARNING_LOG_STATUS;
    }

    HTTPClient client;
    bool isOK = client.begin(url);

    if (!isOK) {
        return std::nullopt;
    }

    std::vector<Header> headers = _globalHeaders.empty() ? _releaseHeaders : _globalHeaders;
    std::for_each(headers.begin(), headers.end(), [this, &client](const auto& header) -> void {
        const auto& [type, value] = header;
        if (!client.hasHeader(type.c_str())) {
            client.addHeader(type, value);
        }
    });

    int statusCode = client.GET();
    String responseData = client.getString();
    client.end();
    return _parser->parse(responseData, statusCode);
}

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::performUpdate() {
    HTTPClient client;
    String url = _customDownloadURL.isEmpty() ? _voyagerDownloadURL : _customDownloadURL;

    bool isOK = client.begin(url);
    if (!isOK) {
        return;
    }

    std::vector<Header> headers = _globalHeaders.empty() ? _downloadHeaders : _globalHeaders;
    std::for_each(headers.begin(), headers.end(), [this, &client](const auto& header) -> void {
        const auto& [type, value] = header;
        if (!client.hasHeader(type.c_str())) {
            client.addHeader(type, value);
        }
    });

    _otaUpdateHandler(client);
}

std::optional<Voyager::VoyagerReleaseModel> Voyager::VoyagerJSONParser::parse(Voyager::HTTPResponseData responseData, int statusCode) {
    JsonDocument document;
    DeserializationError error = deserializeJson(document, responseData);

    if (error) {
        Serial.printf("VOYAGER_OTA_JSON_Error : %s\n", error.c_str());
        return std::nullopt;
    }

    if (statusCode != HTTP_CODE_OK) {
        Voyager::VoyagerReleaseModel payload = {
            .message = document["data"]["message"].template as<String>(),
            .statusCode = statusCode,
        };

        return payload;
    }

    Voyager::VoyagerReleaseModel payload = {
        .version = document["data"]["release"]["version"].template as<String>(),
        .releaseId = document["data"]["release"]["id"].template as<String>(),
        .changeLog = document["data"]["release"]["change_log"].template as<String>(),
        .releasedDate = document["data"]["release"]["released_date"].template as<String>(),
        .prettyFileSize = document["data"]["file"]["size"].template as<String>(),
        .boardType = document["data"]["board_type"].template as<String>(),
        .environment = document["data"]["release"]["environment"].template as<String>(),
        .fileSizeInBytes = document["data"]["file"]["size_in_bytes"].template as<int>(),
        .statusCode = statusCode,
    };

    return payload;
}

std::optional<Voyager::GithubReleaseModel> Voyager::GithubJSONParser::parse(Voyager::HTTPResponseData responseData, int statusCode) override {
    ArduinoJson::JsonDocument document;
    ArduinoJson::DeserializationError error = ArduinoJson::deserializeJson(document, responseData);

    if (error) {
        Serial.printf("VoyagerOTA JSON Error : %s\n", error.c_str());
        return std::nullopt;
    }

    if (statusCode != HTTP_CODE_OK) {
        GithubReleaseModel payload = {
            .version = "0.0.0",
            .statusCode = statusCode,
        };
        return payload;
    }

    GithubReleaseModel payload = {
        .version = document["tag_name"].template as<String>(),
        .name = document["name"].template as<String>(),
        .publishedAt = document["published_at"].template as<String>(),
        .browserDownloadUrl = document["assets"][0]["url"].template as<String>(),
        .size = document["assets"][0]["size"].template as<int>(),
        .statusCode = statusCode};

    return payload;
}

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::_otaUpdateHandler(HTTPClient& client) {
    _registerHTTPUpdateCallbacks();

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

template <typename T_ResponseData, typename T_PayloadModel>
inline void Voyager::OTA<T_ResponseData, T_PayloadModel>::_registerHTTPUpdateCallbacks() {
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
}

#endif
