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
  #include <Preferences.h>
  #if __has_include(<esp_random.h>)
    #include <esp_random.h>
  #else
    #include <esp_system.h>
  #endif
  #include "ApiConstants.h"
  #if defined(__VOYAGER_BOOT_GUARD__) && (__VOYAGER_BOOT_GUARD__ == true)
    #include <Update.h>
    #include <esp_system.h>
  #endif
#endif

#if !defined(ARDUINOJSON_VERSION_MAJOR) || ARDUINOJSON_VERSION_MAJOR < 7
  #error VoyagerOTAClient requires the ArduinoJson library version 7.0 or above.
#endif

#define VOYAGER_OTA_CLIENT_VERSION "6.1.0"
#define VOYAGER_OTA_CLIENT_VERSION_MAJOR 6
#define VOYAGER_OTA_CLIENT_VERSION_MINOR 1
#define VOYAGER_OTA_CLIENT_VERSION_PATCH 0

// !Do NOT change....For Platform's Backend use only......
#if defined(__ENABLE_ADVANCED_MODE__) && (__ENABLE_ADVANCED_MODE__ == true)
  #pragma message("VoyagerOTA Advanced Mode Enabled! All VoyagerOTA related features has been disabled!");
#else
  #if !defined(__USE_STAGING_CHANNEL__) && (__USE_STAGING_CHANNEL__ == false)
    #error __USE_STAGING_CHANNEL__ MACRO is missing. Please define at the top of the code either as true or false.
  #endif

    //   https://stackoverflow.com/questions/31637626/whats-the-usecase-of-gccs-used-attribute#:~:text=If%20you%20declare%20a%20global%20variable%20or%20function%20that%20is%20unused%2C%20gcc%20will%20optimized%20it%20out%20(with%20warning)%2C%20but%20if%20you%20declared%20the%20global%20variable%20or%20the%20function%20with%20%27__attribute__((used))%27%2C%20gcc%20will%20include%20it%20in%20object%20file%20(and%20linked%20executable).
  #if __USE_STAGING_CHANNEL__
const char* ___VYGR_STAGING___ __attribute__((used)) = "$2y$10$BsbB6jZbeQKLLnsnvGRJfOmGuG2Co0/LEDR4xO0Khnlvvm57c6Tai";
    #pragma message("[VoyagerOTA-WARNING]: Do not Upload STAGING enabled builds on VoyagerOTA Platform.")
  #else
const char* ___VYGR_PRODUCTION___ __attribute__((used)) = "$2y$10$DX0bqDwfQtWJkBPgiXHVqOcbjOoX5i9cRHxSTgK3xgjTHpy5EGNbO";
  #endif

  // the platform reads this back from the uploaded .bin to make sure it was built for the release's version....
  #if defined(CURRENT_FIRMWARE_VERSION)
const char* ___VYGR_FIRMWARE_VERSION___ __attribute__((used)) = "VYGR_FW_VERSION:" CURRENT_FIRMWARE_VERSION ";";
  #endif

  // opt-in: go back to the previous firmware when a new one keeps crashing before it reaches the server....
  #if !defined(__VOYAGER_BOOT_GUARD__)
    #define __VOYAGER_BOOT_GUARD__ false
  #endif
  #if !defined(VOYAGER_BOOT_GUARD_ATTEMPTS)
    #define VOYAGER_BOOT_GUARD_ATTEMPTS 3
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
    // where this device stands with the VoyagerOTA platform....
    enum class DeviceStatus {
        UNREGISTERED,  // never asked for access yet (or its token was rejected)
        PENDING,       // waiting for approval in the dashboard
        ACTIVE,        // approved, holds a device token
        REVOKED,       // revoked in the dashboard
        REJECTED,      // hardware id is claimed by another device secret
        ERROR,         // network / server error, try again later
    };

    inline const char* toString(DeviceStatus status) {
        switch (status) {
            case DeviceStatus::UNREGISTERED:
                return "UNREGISTERED";
            case DeviceStatus::PENDING:
                return "PENDING";
            case DeviceStatus::ACTIVE:
                return "ACTIVE";
            case DeviceStatus::REVOKED:
                return "REVOKED";
            case DeviceStatus::REJECTED:
                return "REJECTED";
            default:
                return "ERROR";
        }
    }
#endif

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
        // the server marked the release this device runs as revoked and is sending it back to an older one....
        bool isRollback = false;

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

        // asks the platform for access, stores the device token in NVS once approved....
        DeviceStatus registerDevice();

        [[nodiscard]] DeviceStatus getDeviceStatus() const;

        // chip eFuse MAC as 12 hex characters, this is what the dashboard shows....
        [[nodiscard]] String getHardwareId() const;

        // forgets the stored claim secret and token, the device has to be approved again....
        void resetDeviceIdentity();
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

#if !__ENABLE_ADVANCED_MODE__
        [[nodiscard]] bool shouldUpdate(const T_PayloadModel& release);

        // call it first thing in setup(), before Wi-Fi....
        void guardBoot();
#endif

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

        String _hardwareId;
        String _claimSecret;
        String _deviceToken;
        String _authorizationHeader;
        DeviceStatus _deviceStatus = DeviceStatus::UNREGISTERED;
        bool _isIdentityLoaded = false;
        bool _isPendingReportChecked = false;
        String _lastReleaseId;
        String _lastReleaseVersion;

    private:
        void _setVoyagerHeaders(std::vector<Header> headers);
        void _rebuildVoyagerHeaders();
        void _loadDeviceIdentity();
        void _storeDeviceToken(const String& token);
        void _storePendingUpdate();
        void _clearPendingUpdate();
        void _reportPendingUpdate();
        bool _isBootGuardRolledBack = false;
        String _skippedVersion;
        bool _sendReport(const char* status, const String& releaseId, const String& fromVersion, const String& toVersion, int errorCode, const String& message);
#endif
    };

    namespace HttpClientHelper {
        inline void addHttpClientHeaders(HTTPClient& client, const std::vector<Voyager::Header> headers) {
            for (const auto& [type, value] : headers) {
                if (!client.hasHeader(type)) {
                    client.addHeader(type, value);
                }
            }
            client.setUserAgent("voyager-ota-client-sdk/" VOYAGER_OTA_CLIENT_VERSION);
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
    _rebuildVoyagerHeaders();
}

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::_rebuildVoyagerHeaders() {
    std::vector<Header> headers = {
        {__VoyagerApi__::Headers::Keys::X_PROJECT_ID, _projectId.c_str()},
        {__VoyagerApi__::Headers::Keys::X_API_KEY, _apiKey.c_str()},
        {__VoyagerApi__::Headers::Keys::X_FIRMWARE_VERSION, _currentVersion.c_str()},
        {__VoyagerApi__::Headers::Keys::X_CHIP_MODEL, ESP.getChipModel()},
    };

    if (!_deviceToken.isEmpty()) {
        _authorizationHeader = "Bearer " + _deviceToken;
        headers.push_back({__VoyagerApi__::Headers::Keys::AUTHORIZATION, _authorizationHeader.c_str()});
    }

    _setVoyagerHeaders(headers);
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

    _loadDeviceIdentity();

    if (_deviceToken.isEmpty()) {
        DeviceStatus status = registerDevice();
        if (status != DeviceStatus::ACTIVE) {
            Serial.printf("[VoyagerOTA] Device %s is %s, no update check.\n", _hardwareId.c_str(), Voyager::toString(status));
            return std::nullopt;
        }
    }

    _reportPendingUpdate();

  #if __USE_STAGING_CHANNEL__
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

#if !__ENABLE_ADVANCED_MODE__
    // token revoked / device deleted: forget it, the next check registers again....
    if (statusCode == HTTP_CODE_UNAUTHORIZED) {
        Serial.println("[VoyagerOTA] Device token was rejected, the device will register again.");
        _storeDeviceToken(String());
        _deviceStatus = DeviceStatus::UNREGISTERED;
        return std::nullopt;
    }

    std::optional<T_PayloadModel> release = _parser->parse(responseData, statusCode);
    if constexpr (std::is_same_v<T_PayloadModel, Voyager::VoyagerReleaseModel>) {
        if (release) {
            _lastReleaseId = release->releaseId;
            _lastReleaseVersion = release->version;
        }
    }
    return release;
#else
    return _parser->parse(responseData, statusCode);
#endif
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
    payload.isRollback = document["release"]["rollback"].as<bool>();

    return payload;
}
#endif

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::performUpdate() {
    if (_downloadURL.isEmpty()) {
        Serial.print("Download URL is required!");
        return;
    }

    HTTPClient client;
    bool isOK = client.begin(_downloadURL);

    // TODO Add error log message.....
    if (!isOK) {
        return;
    }

#if __ENABLE_ADVANCED_MODE__
    std::vector<Header> headers = _downloadHeaders;
#else
    std::vector<Header> headers = _voyagerHeaders;
    // success can only be confirmed after the reboot, so remember what we are flashing....
    _storePendingUpdate();
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
            Serial.printf("VOYAGER_OTA HTTP_UPDATE_FAILED : %s  ERROR CODE :  %d\n", httpUpdate.getLastErrorString().c_str(), httpUpdate.getLastError());
        } break;

        default: {
            Serial.println("VOYAGER_OTA NO_HTTP_UPDATE_AVAILABLE");
        } break;
    }

    client.end();

#if !__ENABLE_ADVANCED_MODE__
    if (updateResult == HTTP_UPDATE_FAILED) {
        _sendReport("failed", _lastReleaseId, _currentVersion, _lastReleaseVersion, httpUpdate.getLastError(), httpUpdate.getLastErrorString());
        _clearPendingUpdate();
    } else if (updateResult == HTTP_UPDATE_NO_UPDATES) {
        _clearPendingUpdate();
    }
#endif
}
#if !__ENABLE_ADVANCED_MODE__
namespace Voyager::DeviceIdentity {
    constexpr const char* NVS_NAMESPACE = "voyager";
    constexpr const char* KEY_SECRET = "secret";
    constexpr const char* KEY_TOKEN = "token";
    constexpr const char* KEY_PROJECT = "project";
    constexpr const char* KEY_PENDING_RELEASE = "p_rel";
    constexpr const char* KEY_PENDING_FROM = "p_from";
    constexpr const char* KEY_PENDING_TO = "p_to";
    constexpr const char* KEY_BOOT_ATTEMPTS = "p_boots";
    constexpr const char* KEY_ROLLED_BACK = "p_rolled";
    constexpr const char* KEY_SKIP_VERSION = "skip_ver";

    inline String readHardwareId() {
        uint64_t mac = ESP.getEfuseMac();
        char buffer[13];
        snprintf(buffer, sizeof(buffer), "%02X%02X%02X%02X%02X%02X",
                 static_cast<uint8_t>(mac), static_cast<uint8_t>(mac >> 8),
                 static_cast<uint8_t>(mac >> 16), static_cast<uint8_t>(mac >> 24),
                 static_cast<uint8_t>(mac >> 32), static_cast<uint8_t>(mac >> 40));
        return String(buffer);
    }

    inline String generateSecret() {
        char buffer[33];
        snprintf(buffer, sizeof(buffer), "%08lx%08lx%08lx%08lx",
                 static_cast<unsigned long>(esp_random()), static_cast<unsigned long>(esp_random()),
                 static_cast<unsigned long>(esp_random()), static_cast<unsigned long>(esp_random()));
        return String(buffer);
    }
}  // namespace Voyager::DeviceIdentity

template <typename T_ResponseData, typename T_PayloadModel>
String Voyager::OTA<T_ResponseData, T_PayloadModel>::getHardwareId() const {
    return Voyager::DeviceIdentity::readHardwareId();
}

template <typename T_ResponseData, typename T_PayloadModel>
Voyager::DeviceStatus Voyager::OTA<T_ResponseData, T_PayloadModel>::getDeviceStatus() const {
    return _deviceStatus;
}

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::_loadDeviceIdentity() {
    if (_isIdentityLoaded)
        return;
    _isIdentityLoaded = true;
    _hardwareId = Voyager::DeviceIdentity::readHardwareId();

    Preferences preferences;
    preferences.begin(Voyager::DeviceIdentity::NVS_NAMESPACE, false);

    // the claim secret is created once and proves this is the same physical device....
    _claimSecret = preferences.getString(Voyager::DeviceIdentity::KEY_SECRET, "");
    if (_claimSecret.length() < 32) {
        _claimSecret = Voyager::DeviceIdentity::generateSecret();
        preferences.putString(Voyager::DeviceIdentity::KEY_SECRET, _claimSecret);
    }

    // a token only belongs to the project it was issued for....
    if (preferences.getString(Voyager::DeviceIdentity::KEY_PROJECT, "") == _projectId) {
        _deviceToken = preferences.getString(Voyager::DeviceIdentity::KEY_TOKEN, "");
    } else {
        preferences.remove(Voyager::DeviceIdentity::KEY_TOKEN);
        preferences.putString(Voyager::DeviceIdentity::KEY_PROJECT, _projectId);
        _deviceToken = String();
    }
    preferences.end();

    if (!_deviceToken.isEmpty()) {
        _deviceStatus = DeviceStatus::ACTIVE;
    }
    _rebuildVoyagerHeaders();
}

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::_storeDeviceToken(const String& token) {
    _deviceToken = token;

    Preferences preferences;
    preferences.begin(Voyager::DeviceIdentity::NVS_NAMESPACE, false);
    if (token.isEmpty()) {
        preferences.remove(Voyager::DeviceIdentity::KEY_TOKEN);
    } else {
        preferences.putString(Voyager::DeviceIdentity::KEY_TOKEN, token);
    }
    preferences.end();

    _rebuildVoyagerHeaders();
}

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::resetDeviceIdentity() {
    Preferences preferences;
    preferences.begin(Voyager::DeviceIdentity::NVS_NAMESPACE, false);
    preferences.clear();
    preferences.end();

    _claimSecret = String();
    _deviceToken = String();
    _deviceStatus = DeviceStatus::UNREGISTERED;
    _isIdentityLoaded = false;
    _isPendingReportChecked = false;
    _rebuildVoyagerHeaders();
}

template <typename T_ResponseData, typename T_PayloadModel>
Voyager::DeviceStatus Voyager::OTA<T_ResponseData, T_PayloadModel>::registerDevice() {
    if (_baseURL.isEmpty() || _apiKey.isEmpty() || _projectId.isEmpty()) {
        Serial.println("[VoyagerOTA] setCredentials() and setBaseURL() are required before registerDevice()!");
        return _deviceStatus = DeviceStatus::ERROR;
    }

    _loadDeviceIdentity();

    HTTPClient client;
    if (!client.begin(_baseURL + __VoyagerApi__::Endpoints::REGISTER_DEVICE)) {
        return _deviceStatus = DeviceStatus::ERROR;
    }

    client.setUserAgent("voyager-ota-client-sdk/" VOYAGER_OTA_CLIENT_VERSION);
    client.addHeader(__VoyagerApi__::Headers::Keys::X_PROJECT_ID, _projectId);
    client.addHeader(__VoyagerApi__::Headers::Keys::X_API_KEY, _apiKey);
    client.addHeader(__VoyagerApi__::Headers::Keys::CONTENT_TYPE, "application/json");

    JsonDocument body;
    body["hardwareId"] = _hardwareId;
    body["claimSecret"] = _claimSecret;
    body["chipModel"] = ESP.getChipModel();
    body["firmwareVersion"] = _currentVersion;
    String payload;
    ArduinoJson::serializeJson(body, payload);

    int statusCode = client.POST(payload);
    String responseData = client.getString();
    client.end();

    switch (statusCode) {
        case HTTP_CODE_OK: {
            JsonDocument document;
            if (deserializeJson(document, responseData) || document["token"].isNull()) {
                return _deviceStatus = DeviceStatus::ERROR;
            }
            _storeDeviceToken(document["token"].as<String>());
            _deviceStatus = DeviceStatus::ACTIVE;
        } break;

        case HTTP_CODE_ACCEPTED: {
            _deviceStatus = DeviceStatus::PENDING;
            Serial.printf("[VoyagerOTA] Device %s is waiting for approval in the dashboard.\n", _hardwareId.c_str());
        } break;

        case HTTP_CODE_FORBIDDEN: {
            _storeDeviceToken(String());
            _deviceStatus = DeviceStatus::REVOKED;
            Serial.printf("[VoyagerOTA] Device %s has been revoked.\n", _hardwareId.c_str());
        } break;

        case HTTP_CODE_CONFLICT: {
            _deviceStatus = DeviceStatus::REJECTED;
            Serial.printf("[VoyagerOTA] Hardware id %s is claimed by another device. Remove it in the dashboard to re-add this one.\n", _hardwareId.c_str());
        } break;

        default: {
            _deviceStatus = DeviceStatus::ERROR;
            Serial.printf("[VoyagerOTA] Device registration failed (HTTP %d): %s\n", statusCode, responseData.c_str());
        } break;
    }

    return _deviceStatus;
}

template <typename T_ResponseData, typename T_PayloadModel>
bool Voyager::OTA<T_ResponseData, T_PayloadModel>::_sendReport(const char* status, const String& releaseId, const String& fromVersion, const String& toVersion, int errorCode, const String& message) {
    if (_deviceToken.isEmpty() || toVersion.isEmpty())
        return false;

    HTTPClient client;
    if (!client.begin(_baseURL + __VoyagerApi__::Endpoints::DEVICE_REPORTS))
        return false;

    client.setUserAgent("voyager-ota-client-sdk/" VOYAGER_OTA_CLIENT_VERSION);
    client.addHeader(__VoyagerApi__::Headers::Keys::AUTHORIZATION, _authorizationHeader);
    client.addHeader(__VoyagerApi__::Headers::Keys::X_FIRMWARE_VERSION, _currentVersion);
    client.addHeader(__VoyagerApi__::Headers::Keys::CONTENT_TYPE, "application/json");

    JsonDocument body;
    body["status"] = status;
    body["toVersion"] = toVersion;
    if (!releaseId.isEmpty())
        body["releaseId"] = releaseId;
    if (!fromVersion.isEmpty())
        body["fromVersion"] = fromVersion;
    if (errorCode != 0)
        body["errorCode"] = errorCode;
    if (!message.isEmpty())
        body["message"] = message.substring(0, 255);
    String payload;
    ArduinoJson::serializeJson(body, payload);

    int statusCode = client.POST(payload);
    client.end();

    // 400 means the report itself is invalid, retrying won't help....
    return statusCode == HTTP_CODE_CREATED || statusCode == HTTP_CODE_BAD_REQUEST;
}

template <typename T_ResponseData, typename T_PayloadModel>
bool Voyager::OTA<T_ResponseData, T_PayloadModel>::shouldUpdate(const T_PayloadModel& release) {
  #if __USE_STAGING_CHANNEL__
    // a staging build is flashed by hand, the release is the uploaded production build of the same
    // version: install it so the exact binary gets tested and reported. Afterwards the device runs
    // a production build and follows the production channel, so this can't loop....
    if (release.version == _currentVersion)
        return true;
  #else
    if (release.version == _currentVersion)
        return false;
  #endif
    // the boot guard already watched this one crash, don't download it again....
    if (!_skippedVersion.isEmpty() && release.version == _skippedVersion)
        return false;
    if constexpr (std::is_same_v<T_PayloadModel, Voyager::VoyagerReleaseModel>) {
        if (release.isRollback)
            return true;
    }
    return isNewVersion(release.version);
}

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::guardBoot() {
  #if __VOYAGER_BOOT_GUARD__
    Preferences preferences;
    preferences.begin(Voyager::DeviceIdentity::NVS_NAMESPACE, false);
    String toVersion = preferences.getString(Voyager::DeviceIdentity::KEY_PENDING_TO, "");
    _skippedVersion = preferences.getString(Voyager::DeviceIdentity::KEY_SKIP_VERSION, "");

    // nothing was flashed, or we are back on the old firmware already....
    if (toVersion.isEmpty() || toVersion != _currentVersion) {
        _isBootGuardRolledBack = preferences.getBool(Voyager::DeviceIdentity::KEY_ROLLED_BACK, false);
        preferences.remove(Voyager::DeviceIdentity::KEY_BOOT_ATTEMPTS);
        preferences.end();
        return;
    }

    // only crashes count: a power cut, a manual reset or no Wi-Fi must never roll back a healthy firmware....
    esp_reset_reason_t reason = esp_reset_reason();
    bool isCrash = reason == ESP_RST_PANIC || reason == ESP_RST_INT_WDT || reason == ESP_RST_TASK_WDT || reason == ESP_RST_WDT;
    if (!isCrash) {
        preferences.remove(Voyager::DeviceIdentity::KEY_BOOT_ATTEMPTS);
        preferences.end();
        return;
    }

    // crashes in a row since this firmware was flashed, cleared once the update is reported....
    uint8_t crashes = preferences.getUChar(Voyager::DeviceIdentity::KEY_BOOT_ATTEMPTS, 0);
    if (crashes < UINT8_MAX)
        crashes++;
    preferences.putUChar(Voyager::DeviceIdentity::KEY_BOOT_ATTEMPTS, crashes);

    if (crashes < VOYAGER_BOOT_GUARD_ATTEMPTS) {
        preferences.end();
        return;
    }

    // marks the previous OTA partition as the boot one, nothing is written unless that worked....
    if (!Update.canRollBack() || !Update.rollBack()) {
        Serial.printf("[VoyagerOTA] v%s crashed %u times in a row but there is no previous firmware to go back to.\n", _currentVersion.c_str(), static_cast<unsigned>(crashes));
        preferences.end();
        return;
    }

    Serial.printf("[VoyagerOTA] v%s crashed %u times in a row, going back to the previous firmware.\n", _currentVersion.c_str(), static_cast<unsigned>(crashes));
    preferences.putBool(Voyager::DeviceIdentity::KEY_ROLLED_BACK, true);
    preferences.putString(Voyager::DeviceIdentity::KEY_SKIP_VERSION, toVersion);
    preferences.remove(Voyager::DeviceIdentity::KEY_BOOT_ATTEMPTS);
    preferences.end();
    ESP.restart();
  #endif
}

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::_storePendingUpdate() {
    if (_lastReleaseVersion.isEmpty())
        return;

    Preferences preferences;
    preferences.begin(Voyager::DeviceIdentity::NVS_NAMESPACE, false);
    preferences.putString(Voyager::DeviceIdentity::KEY_PENDING_RELEASE, _lastReleaseId);
    preferences.putString(Voyager::DeviceIdentity::KEY_PENDING_FROM, _currentVersion);
    preferences.putString(Voyager::DeviceIdentity::KEY_PENDING_TO, _lastReleaseVersion);
    preferences.end();
}

template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::_clearPendingUpdate() {
    Preferences preferences;
    preferences.begin(Voyager::DeviceIdentity::NVS_NAMESPACE, false);
    preferences.remove(Voyager::DeviceIdentity::KEY_PENDING_RELEASE);
    preferences.remove(Voyager::DeviceIdentity::KEY_PENDING_FROM);
    preferences.remove(Voyager::DeviceIdentity::KEY_PENDING_TO);
    preferences.remove(Voyager::DeviceIdentity::KEY_BOOT_ATTEMPTS);
    preferences.remove(Voyager::DeviceIdentity::KEY_ROLLED_BACK);
    preferences.end();
}

// after the reboot: the running version tells whether the last update really took effect....
template <typename T_ResponseData, typename T_PayloadModel>
void Voyager::OTA<T_ResponseData, T_PayloadModel>::_reportPendingUpdate() {
    if (_isPendingReportChecked || _deviceToken.isEmpty())
        return;

    Preferences preferences;
    preferences.begin(Voyager::DeviceIdentity::NVS_NAMESPACE, true);
    String releaseId = preferences.getString(Voyager::DeviceIdentity::KEY_PENDING_RELEASE, "");
    String fromVersion = preferences.getString(Voyager::DeviceIdentity::KEY_PENDING_FROM, "");
    String toVersion = preferences.getString(Voyager::DeviceIdentity::KEY_PENDING_TO, "");
    preferences.end();

    if (toVersion.isEmpty()) {
        _isPendingReportChecked = true;
        return;
    }

    bool isUpdated = _currentVersion == toVersion;
    String message = isUpdated ? String() : "Booted v" + _currentVersion + " instead of v" + toVersion + ", the update did not take effect.";
    if (!isUpdated && _isBootGuardRolledBack) {
        message = "v" + toVersion + " kept crashing, the device went back to v" + _currentVersion + ".";
    }

    if (_sendReport(isUpdated ? "success" : "failed", releaseId, fromVersion, toVersion, 0, message)) {
        _clearPendingUpdate();
        _isPendingReportChecked = true;
    }
}
#endif

#endif
