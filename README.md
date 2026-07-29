# Shadow Mesh Pager

A self-organizing, offline text/emoji messaging network for ESP32 devices — designed for a "the internet is gone" scenario, where the only infrastructure available is whatever the devices bring with them.

Every device forms a WiFi mesh with every other device in radio range and relays traffic for the whole group, digipeater-style. A shared AES passphrase — a "channel key" — decides what's *readable* on a given device, completely decoupled from what's *relayed*: a node without your channel key still forwards your (encrypted) messages toward everyone else, it just can't read them.

## Supported hardware

| Board | Interface | Notes |
|---|---|---|
| ESP32 CYD (`ESP32-2432S028R`, resistive touch) | Full touchscreen UI + serial console | Message list, compose w/ on-screen keyboard + emoji picker, network map, RGB LED color picker, settings |
| Heltec WiFi LoRa 32 (V2) | Read-only OLED + serial console | No touch input on this board — the onboard SSD1306 OLED just displays the last few incoming messages; everything else (sending, configuration) happens over serial |

Both boards run the same mesh/crypto/message/settings core — only the presentation layer differs.

## Building

Requires [PlatformIO](https://platformio.org/). Two environments are defined in `platformio.ini`:

```sh
pio run -e esp32dev                  # CYD
pio run -e heltec_wifi_lora_32_v2    # Heltec WiFi LoRa 32 V2

pio run -e esp32dev -t upload        # flash over USB
pio device monitor -b 115200         # serial console / logs
```

## Getting started

**CYD**: on first boot, a wizard walks you through touch calibration (tap two targets), your display name, and the channel key. Everything's editable later from the Settings tab, which also has a "Recalibrate touch" button.

**Heltec / headless boards**: there's no wizard — open the serial monitor at 115200 baud and configure directly:

```
/name Alice
/key our-shared-passphrase
```

## Serial console

Available on every board, always on, regardless of whether a screen is attached:

| Command | Effect |
|---|---|
| *(plain text, no leading `/`)* | Sent as a text message to the mesh |
| `/name <text>` | Set your display name |
| `/key <text>` | Set the shared channel key |
| `/emoji <code>` | Send an emoji (e.g. `/emoji :wave:`) |
| `/emojis` | List available emoji codes |
| `/led <hex>` | Set the status LED color, e.g. `/led ff8800` (CYD only) |
| `/topology` | List known mesh nodes |
| `/history` | Reprint recent message history |
| `/whoami` | Show your node id, name, and channel status |
| `/help` | Show this list |

## How the network works

- **Mesh**: [painlessMesh](https://gitlab.com/painlessMesh/painlessMesh) forms a self-organizing WiFi mesh — no fixed root/master node. Every device joins the same physical mesh (`MESH_SSID`/`MESH_PASSWORD` in `src/config.h`) so any two devices can relay for each other regardless of channel key.
- **Encryption**: message bodies are AES-256-CBC encrypted with a key derived (SHA-256) from your channel passphrase. A 4-byte integrity tag detects a mismatched key so a wrong-channel node shows "undecryptable" instead of garbage. Envelope metadata (sender id, name, timestamp, message type) stays in the clear, like a radio callsign, so relaying nodes can dedup/display it even without your key.
- **Routing**: painlessMesh doesn't expose per-message hop paths (only "delivered to me," not "relayed via me"), so the "network map" is a topology *snapshot* (who's currently reachable, direct vs. via relay) rather than a literal per-message route trace.
- **Persistence**: display name, channel key, touch calibration, and LED color persist across reboots via NVS (ESP32 `Preferences`). Message history is intentionally in-RAM only and resets on reboot.

## Project layout

```
src/
  main.cpp                CYD entry point
  main_heltec.cpp          Heltec entry point
  config.h                 Pins, mesh/crypto constants, shared across targets

  crypto/aes_channel.*      AES-256-CBC + key derivation + integrity check
  message/message.*         Wire protocol (JSON envelope), emoji table
  network/mesh_manager.*    painlessMesh wrapper: history, dedup, topology, identity
  storage/settings_store.*  NVS-backed persistent settings
  console/serial_console.*  Shared serial command interface

  display/display_driver.*  TFT_eSPI + LVGL + XPT2046 touch glue      (CYD only)
  led/rgb_led.*              Discrete RGB LED (PWM, active-low)        (CYD only)
  ui/                        LVGL screens + setup wizard               (CYD only)
  oled/oled_display.*        Read-only SSD1306 message view            (Heltec only)

include/lv_conf.h           LVGL configuration                         (CYD only)
extra_scripts/               PlatformIO pre-build hooks
```

`platformio.ini`'s `build_src_filter` keeps each target's build to only what it needs — the Heltec build never touches LVGL/TFT_eSPI/touch code at all.

## Known limitations

- No authenticated encryption (AEAD) — the integrity tag catches a wrong key, not a tampered ciphertext.
- No automated test suite; this is embedded firmware validated on real hardware, not something a CI runner can exercise meaningfully.
- CYD flash usage is close to the partition limit (~88%) — adding more LVGL fonts/assets may require a bigger partition scheme.
- WiFi radio TX current spikes can brown out boards powered from a marginal USB port/cable; the Heltec target reduces TX power to mitigate this, but a real power source (not a laptop USB port) is the reliable fix.
