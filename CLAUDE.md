# VoyagerOTAClient

Header only C++17 Arduino library for ESP32 over-the-air updates with VoyagerOTA
(`src/VoyagerOTAClient.h`). Version 6.0.0, stays at 6.0.0 while 6 is in development,
don't bump it unless asked. Server: `voyagerota-core`, boards use
`ota.setBaseURL("https://api.voyagerota.com")`.

## Working rules (from the maintainer, always apply)

- Branches: work on a new branch made from `unstable`. Merge into `unstable` only when asked.
- When a plan is asked for, present it and wait for "GO" before writing code.
- Build on existing code, don't rewrite the original code unless asked. Respect reverts.
- Comments: short, plain, ending in `....`. No doc comments.
- Never edit `README.md` unless asked.
- Commits: author `mediocre9`, end with `Co-Authored-By: Claude <noreply@anthropic.com>`,
  never a Claude-Session line.
- ESP32 only.

## How a board works

- `fetchLatestRelease()`: asks for access if needed (registration with the project key and a
  claim secret kept in NVS), sends any pending update report, then returns the latest release.
  A per-device token is stored in NVS after approval.
- `shouldUpdate(release)`: the standard check. True when the release is newer, when the server
  is rolling the device back from a revoked release (`isRollback`), or for a same version
  release on a staging build.
- `isNewVersion()` is kept unchanged for advanced mode (GitHub Releases or a custom server).
- `performUpdate()` downloads through the API (HTTPUpdate) and reboots, the new firmware
  reports success after boot.
- `guardBoot()` with `__VOYAGER_BOOT_GUARD__` counts panic and watchdog resets only.
- Firmware must contain `VYGR_FW_VERSION:x.y.z;` matching the release, the server checks it.
- `__USE_STAGING_CHANNEL__ true` marks a staging test board.
- 401 on registration means the project was deleted or its key changed, 429 means checking
  in too often (fair use is one check-in every 30 s), both are logged and retried next cycle.

## Open question

The library calls `HTTPClient::begin(url)` without setting a CA certificate. Whether HTTPS to
the API works depends on the ESP32 Arduino core. Verify on a real board; if it fails, bundle
the ISRG Root X1 certificate (Let's Encrypt) or let users set one.

There is no Arduino toolchain in the cloud sessions, so changes here are not compiled there.
Compile with Arduino CLI or PlatformIO before release.
