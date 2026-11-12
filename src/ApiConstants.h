#pragma once

namespace __VoyagerApi__ {
    namespace Endpoints {
        String LATEST_RELEASE = "/internal/api/v2/releases/latest";
        constexpr const char* REGISTER_DEVICE = "/internal/api/v2/devices/register";
        constexpr const char* DEVICE_REPORTS = "/internal/api/v2/devices/reports";
    }  // namespace Endpoints
    namespace QueryParams {
        String PRODUCTION_CHANNEL = "?channel=production";
        String STAGING_CHANNEL = "?channel=staging";
    }  // namespace QueryParams
    namespace Headers {
        namespace Keys {
            constexpr const char* X_API_KEY = "x-api-key";
            constexpr const char* X_PROJECT_ID = "x-project-id";
            constexpr const char* X_FIRMWARE_VERSION = "x-firmware-version";
            constexpr const char* X_CHIP_MODEL = "x-chip-model";
            constexpr const char* AUTHORIZATION = "Authorization";
            constexpr const char* CONTENT_TYPE = "Content-Type";
        }  // namespace Keys
    }  // namespace Headers
}  // namespace __VoyagerApi__