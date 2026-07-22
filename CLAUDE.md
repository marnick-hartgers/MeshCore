# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

MeshCore is a portable C++ library for multi-hop LoRa packet routing on embedded devices, built with PlatformIO on top of the Arduino framework. It targets ESP32, nRF52, STM32 and RP2040 microcontrollers across ~150 board variants. The repo contains the core routing library (`src/`), platform/driver helpers (`src/helpers/`), and six flashable firmware applications (`examples/`) that combine to produce PlatformIO build environments per board.

There is no single "MeshCore binary" — every unit of work here is scoped to a `(board variant, example app, transport)` combination expressed as a PlatformIO env, e.g. `RAK_4631_companion_radio_ble` or `Heltec_v3_repeater`.

## Common commands

Development is normally done via the PlatformIO IDE extension in VS Code, but everything below also works from the CLI (`pip install platformio` or the Nix shell via `default.nix`).

```bash
# List every available build environment (board+app+transport combo)
pio project config | grep 'env:'

# Build one environment
pio run -e RAK_4631_companion_radio_ble
pio run -e Heltec_v3_repeater

# ESP32 only: produce a single merged flashable firmware-merged.bin
pio run -e <esp32_env> -t mergebin

# nRF52 only: produce a .uf2 (drag-and-drop flashable) from the built .hex
pio run -e <nrf52_env> -t create_uf2

# Run the native unit test suite (no hardware needed)
pio test -e native --verbose

# Run a single test directory (matches test/<name>)
pio test -e native -f test_utils --verbose
pio test -e native -f test_mesh_tables --verbose
```

`build.sh` wraps `pio run` for CI/release use (`./build.sh list`, `./build.sh build-firmware <env>`, `./build.sh build-matching-firmwares <substring>`) and does the post-build packaging (mergebin / hex→uf2 / copy to `out/`) per platform automatically — read it before scripting a release build.

Local docs site (mkdocs-material):
```bash
pip install mkdocs mkdocs-material
mkdocs serve   # live-reload preview
mkdocs build
```

## Repository layout

- `src/` — the core, platform-agnostic mesh routing library: `Packet`, `Dispatcher`, `Mesh`, `Identity`, `Utils`. This is what actually gets unit-tested natively; keep it free of Arduino-only APIs (guard with `#ifdef ARDUINO`, following `src/Utils.cpp` / `src/MeshCore.h`).
- `src/helpers/` — everything built on top of core: contact/table management (`BaseChatMesh`, `SimpleMeshTables`, `ClientACL`, `ContactInfo`), packet pool (`StaticPoolPacketManager`), CLI (`CommonCLI`), board drivers (`ESP32Board`, `NRF52Board`, per-arch subfolders), radio wrappers (`radiolib/`), displays (`ui/`), sensors (`sensors/`), and bridges (`bridges/`).
- `examples/` — the six firmware applications; no example has its own `platformio.ini` or build config, they're just source trees selected via `build_src_filter` in a variant's `platformio.ini`.
- `variants/<board>/platformio.ini` — per-board PlatformIO config: a base section with pins/radio flags extending one of the root `_base` sections, plus one `[env:...]` per app+transport combo.
- `boards/*.json` — custom PlatformIO board definitions for boards not in the upstream platform index (custom MCU/softdevice/USB VID-PID/linker script config).
- `arch/esp32/`, `arch/stm32/` — vendored/patched libraries needed only by those platforms (AsyncElegantOTA for ESP32 OTA, Adafruit_LittleFS port for STM32).
- `test/` — native (non-hardware) unit tests, see Testing below.
- `docs/` — mkdocs-material documentation site (protocol specs, CLI reference, FAQ); see `docs/architecture.md` for the core library design.

## Core library architecture (`src/`)

**Packet** (`src/Packet.h/.cpp`) is a fixed-size POD wire struct: a 1-byte header (route type + payload type + payload version), a payload buffer (`MAX_PACKET_PAYLOAD` = 184 bytes), and a path buffer (`MAX_PATH_SIZE` = 64 bytes) whose length field is itself bit-packed (hash size × hop count). Route type determines how `path` behaves: `ROUTE_TYPE_FLOOD` packets start with an empty path that *grows* as each repeater appends its own identity-hash; `ROUTE_TYPE_DIRECT` packets carry a pre-computed hop list that *shrinks* as each hop consumes its entry. There is no TTL/hop-limit field — flood termination relies on `MAX_PATH_SIZE` and per-node dedup tables. Deduplication keys on `Packet::calculatePacketHash()`, a content hash (SHA-256 of payload-type + payload), not a per-packet sequence number, so identical rebroadcasts collapse naturally.

**Dispatcher vs. Mesh**: `Dispatcher` (`src/Dispatcher.h/.cpp`) is the radio-facing scheduler — it owns the `Radio`, `PacketManager`, and clock, drives CAD/duty-cycle/airtime-budget logic, and defines a pure-virtual `onRecvPacket()`. `Mesh : public Dispatcher` (`src/Mesh.h/.cpp`) is the protocol layer that understands payload types: it owns node `Identity`, `RNG`, `RTCClock`, `MeshTables`, implements `onRecvPacket()` to decrypt/route, and exposes the app-facing extension points (see below). `Mesh` is still abstract; apps subclass it either directly (`simple_repeater`, `simple_room_server`, `simple_sensor`) or via `BaseChatMesh : public mesh::Mesh` (`src/helpers/BaseChatMesh.h`), which adds contact/channel-table lookups for chat-style apps (`companion_radio`, `simple_secure_chat`).

**Extension contract**: apps override `Mesh` virtuals such as `allowPacketForward` (whether this node retransmits — defaults to false), `searchPeersByHash`/`getPeerSharedSecret`/`searchChannelsByHash` (contact/channel DB lookups), and the data-arrived callbacks `onPeerDataRecv`/`onAnonDataRecv`/`onGroupDataRecv`/`onAdvertRecv`/`onAckRecv`/`onTraceRecv`/etc. Outbound helpers (`createAdvert`, `createDatagram`, `sendFlood`/`sendDirect`/`sendZeroHop`, …) are the app-facing send API. There is no runtime "role" concept in core — a node's role (companion/repeater/room-server/sensor) is entirely a function of which `examples/` app it was compiled from; the `ADV_TYPE_*` field in advert payloads (`src/helpers/AdvertDataHelpers.h`) is just descriptive metadata other nodes read, not something core routing branches on.

**Identity/crypto** (`src/Identity.h/.cpp`): Ed25519 keypairs are used both for signing (adverts are signed-plaintext, not encrypted) and, via curve transposition, for X25519 ECDH to derive per-peer shared secrets. Everything except adverts (unicast/group messages, ACKs, paths, requests) is AES-encrypted then HMAC-SHA256 authenticated (`Utils::encryptThenMAC`/`MACThenDecrypt`). Node/path identity hashes are only 1 byte (`PATH_HASH_SIZE`), so they collide by design — decrypt paths loop over multiple `searchPeersByHash`/`searchChannelsByHash` candidates until a MAC check succeeds.

**Memory model**: the project's "no dynamic allocation except during setup" rule is implemented via `StaticPoolPacketManager` (`src/helpers/StaticPoolPacketManager.h/.cpp`), which allocates a fixed pool of `Packet` objects once at construction; afterward `allocNew()`/`free()` just move pointers between fixed-capacity queues (unused/send/recv), never allocating. If the pool is exhausted, `allocNew()` returns `NULL` and callers fail gracefully.

For the full picture (packet lifecycle, ACLs, transport codes, dedup tables), see `docs/architecture.md`.

## Build system notes

- `[arduino_base]` in the root `platformio.ini` pins RadioLib to an exact commit (not a release tag), disables unused RadioLib protocol modules via `RADIOLIB_EXCLUDE_*`, and sets the default LoRa freq/BW/SF that every variant inherits unless overridden.
- Per-platform base sections (`esp32_base`, `nrf52_base`, `rp2040_base`, `stm32_base`) each `extends = arduino_base` and wire a platform-specific post/extra script: STM32's `arch/stm32/build_hex.py` runs automatically post-build (elf→hex); ESP32's `merge-bin.py` and nRF52's `create-uf2.py` register on-demand custom targets (`-t mergebin`, `-t create_uf2`) rather than running automatically.
- `[esp32_ota]` and `[sensor_base]` are opt-in mixins (not part of any `extends` chain) that specific variants pull in via `lib_deps = ${esp32_ota.lib_deps}` / `${sensor_base.lib_deps}` for OTA web-update or environmental-sensor support.
- Variant env naming convention: `<BoardName>_<example-app>[_<transport>]`, e.g. `RAK_4631_companion_radio_usb`, `Heltec_v3_terminal_chat`, `Xiao_rp2040_kiss_modem`. The example app is selected purely by which `+<../examples/...>` path gets appended to that env's `build_src_filter`.
- `build_as_lib.py` is only used when a *different* PlatformIO project adds MeshCore as a `lib_deps` dependency (see `library.json`) — it's irrelevant when building in-tree here.

## Testing

The `[env:native]` PlatformIO environment (`platformio.ini`) compiles `src/Utils.cpp` and `src/Packet.cpp` directly against googletest, using stub headers in `test/mocks/` (`Stream.h`, `AES.h`, `SHA256.h`) to satisfy their includes without pulling in RadioLib/Crypto/Arduino. Arduino-only code paths are simply compiled out via `#ifdef ARDUINO` rather than mocked. Test suites live in `test/test_utils/` (`Utils::toHex`) and `test/test_mesh_tables/` (`SimpleMeshTables` dedup logic) as plain `TEST()` blocks (no fixtures). CI (`.github/workflows/run-unit-tests.yml`) runs `pio test -e native -vv` on every push/PR to main. Adding a new native-testable unit means adding it to `[env:native]`'s `build_src_filter` in `platformio.ini` and, if it pulls in a library header, adding a matching stub under `test/mocks/`.

## Contribution conventions (from CONTRIBUTING.md)

- PRs target the `dev` branch, not `main`.
- Embedded-first style: no dynamic memory allocation outside setup/begin, avoid unnecessary abstraction layers, match the existing brace/indent style in the file you're editing rather than reformatting (a stray reformat makes diffs unreviewable — don't retroactively reformat existing code even though `.clang-format` exists).
- 2-space indentation, `camelCase` functions/variables, `UpperCamelCase` classes, `ALL_CAPS` `#define` constants — but consistency with the surrounding file wins over these rules.
- New features affecting public API should update `README.md`/`library.properties` and, where relevant, add an example under `examples/`.
