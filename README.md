# Shadow Mesh Pager

A self-organizing, offline text messaging network for ESP32 devices — designed for a "the internet is gone" scenario, where the only infrastructure available is whatever the devices bring with them.

Every device forms a WiFi mesh with every other device in radio range and relays traffic for the whole group, digipeater-style. A shared AES passphrase — a "channel key" — decides what's *readable* on a given device, completely decoupled from what's *relayed*: a node without your channel key still forwards your (encrypted) messages toward everyone else, it just can't read them.

## Supported hardware

| Board | Interface | Notes |
|---|---|---|
| ESP32 CYD (`ESP32-2432S028R`, resistive touch) | Full touchscreen UI + serial console | Message list, compose w/ on-screen keyboard + preset messages, network map, RGB LED color picker, settings |
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

## BOOT0 button: Morse code input

Both boards have a BOOT0/PRG button wired to GPIO0. Once booted, it doubles as a one-button Morse code input, so you can compose and send a message without touch or serial:

- **Tap** = dot, **hold** = dash (long/short press, decoded per-letter using International Morse code, A-Z/0-9).
- Pause briefly after a letter's dots/dashes to move on to the next letter; pause longer to insert a word space.
- Pause for about 3 seconds after your last letter and the message sends automatically.

Live feedback while tapping: the CYD shows a status bar above the nav bar with the decoded text and the in-progress dot/dash pattern; the Heltec's OLED overlays the same on its bottom line. Both also log every symbol, decoded letter, and sent message to the serial console.

Timings (dot/dash threshold, letter/word gaps, send timeout) are tunable via `MORSE_*` constants in `src/config.h` if the defaults feel too fast/slow for manual pressing.

## Serial console

Available on every board, always on, regardless of whether a screen is attached:

| Command | Effect |
|---|---|
| *(plain text, no leading `/`)* | Sent as a text message to the mesh |
| `/name <text>` | Set your display name |
| `/key <text>` | Set the shared channel key |
| `/preset <n>` | Send a preset message by number (e.g. `/preset 1`) |
| `/presets` | List preset messages with their numbers |
| `/led <hex>` | Set the status LED color, e.g. `/led ff8800` (CYD only) |
| `/gain [dBm]` | Show/set WiFi TX power, e.g. `/gain 11`; no argument lists the supported steps |
| `/topology` | List known mesh nodes |
| `/history` | Reprint recent message history |
| `/whoami` | Show your node id, name, and channel status |
| `/factory-reset` | Wipe all persisted settings and reboot -- requires `/factory-reset confirm` |
| `/help` | Show this list |

## Preset messages

A list of canned phrases (`PRESET_TABLE`, `src/message/message.cpp`) for sending common status updates without typing:

- **CYD**: a scrollable list of full-width buttons on the Compose tab, below the text field -- tap one to send it immediately.
- **Serial console**: `/presets` lists them with their numbers; `/preset <n>` sends one (e.g. `/preset 1` for "SOS - need help").

Presets are sent as ordinary text messages (no separate wire type), so they show up identically to typed messages on every device.

## WiFi gain (TX power)

Transmit power is adjustable, persisted across reboots, and shared by both boards:

- **CYD**: a dropdown on the Settings tab, next to the other radio/network controls.
- **Serial console**: `/gain` alone shows the current setting and every supported step; `/gain <dBm>` sets the nearest one (e.g. `/gain 15`).

Defaults to 11dBm (`DEFAULT_WIFI_GAIN_RAW`, `src/config.h`) -- the level the Heltec build used to hardcode to avoid brownouts on marginal USB power; lower it further if you still see boot loops, or raise it for more range on a solid power source.

## Factory reset

`/factory-reset confirm` over serial, or the "Factory reset" button on the CYD's Settings tab (with an on-screen confirmation), wipes every persisted setting -- name, channel key, WiFi gain, LED color, touch calibration, and the setup-complete flag -- and reboots into a clean first-boot state.

## How the network works

- **Mesh**: [painlessMesh](https://gitlab.com/painlessMesh/painlessMesh) forms a self-organizing WiFi mesh — no fixed root/master node. Every device joins the same physical mesh (`MESH_SSID`/`MESH_PASSWORD` in `src/config.h`) so any two devices can relay for each other regardless of channel key.
- **Encryption**: message bodies are AES-256-CBC encrypted with a key derived (SHA-256) from your channel passphrase. A 4-byte integrity tag detects a mismatched key so a wrong-channel node shows "undecryptable" instead of garbage. Envelope metadata (sender id, name, timestamp) stays in the clear, like a radio callsign, so relaying nodes can dedup/display it even without your key.
- **Routing**: painlessMesh doesn't expose per-message hop paths (only "delivered to me," not "relayed via me"), so the "network map" is a topology *snapshot* (who's currently reachable, direct vs. via relay) rather than a literal per-message route trace.
- **Persistence**: display name, channel key, touch calibration, WiFi gain, and LED color persist across reboots via NVS (ESP32 `Preferences`) -- wiped in one shot by `/factory-reset confirm`. Message history is intentionally in-RAM only and resets on reboot.

## Project layout

```
src/
  main.cpp                CYD entry point
  main_heltec.cpp          Heltec entry point
  config.h                 Pins, mesh/crypto constants, shared across targets

  crypto/aes_channel.*      AES-256-CBC + key derivation + integrity check
  message/message.*         Wire protocol (JSON envelope), preset message table
  network/mesh_manager.*    painlessMesh wrapper: history, dedup, topology, identity
  storage/settings_store.*  NVS-backed persistent settings
  console/serial_console.*  Shared serial command interface
  input/morse_input.*       BOOT0-button-as-Morse-code input (see above)

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
- CYD flash usage is close to the partition limit (~89%) — adding more LVGL fonts/assets may require a bigger partition scheme.
- WiFi radio TX current spikes can brown out boards powered from a marginal USB port/cable; the Heltec target reduces TX power to mitigate this, but a real power source (not a laptop USB port) is the reliable fix.
