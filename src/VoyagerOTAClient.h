/******************************************************************************
 * MIT License
 *
 * @headerfile [VoyagerOTAClient.h]
 *
 * @description: A SemVer based OTA client library. Supports GitHub releases,
 * VoyagerOTA platform, and any custom JSON backend via custom parsers.
 *
 * @copyright (c) 2025
 * @author: Fahad Zia Khan - Mediocre9
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

#ifndef MEDIOCRE9_VOYAGER_OTA_CLIENT_H
#define MEDIOCRE9_VOYAGER_OTA_CLIENT_H

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
#include <vector>
#include "semver/semver.hpp"

#if !__ENABLE_ADVANCED_MODE__
  #include "ApiConstants.h"
#endif

#if !defined(ARDUINOJSON_VERSION_MAJOR) || ARDUINOJSON_VERSION_MAJOR < 7
  #error VoyagerOTAClient requires the ArduinoJson library version 7.0 or above.
#endif

#define VOYAGER_OTA_CLIENT_VERSION "4.0.0"
#define VOYAGER_OTA_CLIENT_VERSION_MAJOR 4
#define VOYAGER_OTA_CLIENT_VERSION_MINOR 0
#define VOYAGER_OTA_CLIENT_VERSION_PATCH 0

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
        String downloadURL;

        explicit BaseModel(String v, String url) : version(v), downloadURL(url) {}

        virtual ~BaseModel() = 0;
    };

    BaseModel::~BaseModel() {}

#if !__ENABLE_ADVANCED_MODE__
    struct VoyagerReleaseModel final : public BaseModel {
        String releaseId;
        String changeLog;
        String releasedDate;
        String status;
        int statusCode;
        String hash;
        int size;
        String prettySize;
        String message;

        explicit VoyagerReleaseModel(String version, String releaseId, String changeLog, String releasedDate, String status, int statusCode, String hash, int size, String prettySize, String downloadURL, String message = String()) : BaseModel(version, downloadURL) {
            this->releaseId = releaseId;
            this->changeLog = changeLog;
            this->releasedDate = releasedDate;
            this->status = status;
            this->statusCode = statusCode;
            this->hash = hash;
            this->size = size;
            this->prettySize = prettySize;
            this->message = message;
        }
    };
#endif

    using HTTPResponseData = String;
    template <typename T_ResponseData = Voyager::HTTPResponseData, typename T_PayloadModel = Voyager::BaseModel>
    class IParser {
        static_assert(std::is_base_of_v<BaseModel, T_PayloadModel>, "T_PayloadModel should be extended from BaseModel!");

    public:
        [[nodiscard]] virtual std::optional<T_PayloadModel> parse(T_ResponseData responseData, int statusCode) = 0;

        virtual ~IParser() = default;
    };

#if !__ENABLE_ADVANCED_MODE__
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

    template <typename T_ResponseData, typename T_PayloadModel = Voyager::BaseModel>
    class OTA : public BaseOTA<T_PayloadModel> {
        static_assert(std::is_base_of_v<BaseModel, T_PayloadModel>, "Model should be extended from BaseModel!");

    public:
        using Parser = std::unique_ptr<IParser<T_ResponseData, T_PayloadModel>>;

        OTA() = default;

#if __ENABLE_ADVANCED_MODE__
        explicit OTA(const String& currentVersion, Parser parser);

        void setReleaseURL(const String& endpoint, const std::vector<Header> headers = std::vector<Header>());
#else
        explicit OTA(const String& currentVersion);

        void setCredentials(const String& projectId, const String& apiKey);

        void setBaseURL(const String& url);
#endif
        void setParser(Parser parser);

        void attachEventCallbacks(HTTPUpdateStartCB onStart,
                                  HTTPUpdateProgressCB onProgress,
                                  HTTPUpdateEndCB onEnd,
                                  HTTPUpdateErrorCB onError);

        void setDownloadURL(const String& endpoint, std::vector<Header> headers = std::vector<Header>());

        void setCurrentVersion(const String& currentVersion);

        [[nodiscard]] const String& getCurrentVersion() const;

        [[nodiscard]] bool isNewVersion(const String& release);

        [[nodiscard]] bool isUpToDate(const String& release);

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

        // update event callbacks....
        HTTPUpdateStartCB _onStart;
        HTTPUpdateProgressCB _onProgress;
        HTTPUpdateEndCB _onEnd;
        HTTPUpdateErrorCB _onError;

#if __ENABLE_ADVANCED_MODE__
        String _releaseURL;
        std::vector<Header> _releaseHeaders;

#else
        String _apiKey;
        String _projectId;
        String _baseURL;
        std::vector<Header> _voyagerHeaders;

    private:
        void _setVoyagerHeaders(std::vector<Header> headers);
#endif
    };

    namespace HttpClientHelper {
        inline void addHttpClientHeaders(HTTPClient& client, const std::vector<Voyager::Header> headers) {
            for (const auto [type, value] : headers) {
                if (!client.hasHeader(type)) {
                    client.addHeader(type, value);
                }
            }
        }
    }  // namespace HttpClientHelper
}  // namespace Voyager

#if __ENABLE_ADVANCED_MODE__
template <typename T_ResponseData, typename T_PayloadModel>
Voyager::OTA<T_ResponseData, T_PayloadModel>::OTA(const String& currentVersion, Parser parser)
    : _currentVersion(currentVersion), _parser(std::move(parser)) {}

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::setReleaseURL(const String& endpoint, std::vector<Header> headers) {
    _releaseURL = endpoint;
    _releaseHeaders = headers;
}
#else
template <typename T_ResponseData, typename T_PayloadModel>
Voyager::OTA<T_ResponseData, T_PayloadModel>::OTA(const String& currentVersion)
    : _currentVersion(currentVersion) {
    _parser = std::make_unique<VoyagerJSONParser>();
}

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

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::setBaseURL(const String& url) {
    _baseURL = url;
}
#endif

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::setParser(Parser parser) {
    if (_parser == nullptr) {
        _parser = std::move(parser);
    }
}

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::setDownloadURL(const String& endpoint, std::vector<Header> headers) {
    _downloadURL = endpoint;
    _downloadHeaders = headers;
}

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::attachEventCallbacks(HTTPUpdateStartCB onStart,
                                                                        HTTPUpdateProgressCB onProgress,
                                                                        HTTPUpdateEndCB onEnd,
                                                                        HTTPUpdateErrorCB onError) {
    _onStart = onStart;
    _onProgress = onProgress;
    _onEnd = onEnd;
    _onError = onError;
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
bool Voyager::OTA<T_ResponseData, T_PayloadModel>::isUpToDate(const String& release) {
    return semver::version::parse(_currentVersion.c_str(), false) >= semver::version::parse(release.c_str(), false);
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
    if (_baseURL.isEmpty()) {
        Serial.println("Voyager Base URL is required!");
        return std::nullopt;
    }

    if (_apiKey.isEmpty()) {
        Serial.println("API Key is required!");
        return std::nullopt;
    }

    if (_projectId.isEmpty()) {
        Serial.println("Project Id is required!");
        return std::nullopt;
    }

  #if __ENABLE_DEVELOPMENT_MODE__
    url = _baseURL + __VoyagerApi__::Endpoints::LATEST_RELEASE + __VoyagerApi__::QueryParams::STAGING_CHANNEL;
  #else
    url = _baseURL + __VoyagerApi__::Endpoints::LATEST_RELEASE + __VoyagerApi__::QueryParams::PRODUCTION_CHANNEL;
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

    HttpClientHelper::addHttpClientHeaders(client, headers);

    int statusCode = client.GET();
    Voyager::HTTPResponseData responseData = client.getString();
    client.end();
    return _parser->parse(responseData, statusCode);
}

#if !__ENABLE_ADVANCED_MODE__
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
        return std::nullopt;
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

#if __ENABLE_ADVANCED_MODE__
    std::vector<Header> headers = _downloadHeaders;
#else
    std::vector<Header> headers = _voyagerHeaders;
#endif
    HttpClientHelper::addHttpClientHeaders(client, headers);

    _otaUpdateHandler(client);
}

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::_otaUpdateHandler(HTTPClient& client) {
    if (_onStart && _onProgress && _onEnd && _onError) {
        httpUpdate.onStart(_onStart);
        httpUpdate.onProgress(_onProgress);
        httpUpdate.onEnd(_onEnd);
        httpUpdate.onError(_onError);
    } else {
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
