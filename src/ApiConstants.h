#pragma once

namespace __VoyagerApi__ {
    namespace Endpoints {
        String LATEST_RELEASE = "/internal/api/v1/releases/latest";
    }  // namespace Endpoints
    namespace QueryParams {
        String PRODUCTION_CHANNEL = "?channel=production";
        String STAGING_CHANNEL = "?channel=staging";
    }  // namespace QueryParams
    namespace Headers {
        namespace Keys {
            constexpr const char* X_API_KEY = "x-api-key";
            constexpr const char* X_PROJECT_ID = "x-project-id";
        }  // namespace Keys
    }  // namespace Headers
}  // namespace __VoyagerApi__
