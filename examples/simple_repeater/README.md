# Simple Repeater

Headless store-and-forward node. This is the "extend network coverage" role — it has no phone-facing protocol, just a serial admin CLI, and forwards mesh traffic on behalf of other nodes.

## What it does

- `class MyMesh : public mesh::Mesh, public CommonCLICallbacks` (`MyMesh.h:83`) — subclasses `Mesh` directly (no contact/channel table needed) and overrides `allowPacketForward()` to actually retransmit flood/direct packets, which chat-oriented examples leave disabled.
- Exposes an admin CLI over `Serial` (and optionally `Serial1`/Ethernet when `ETHERNET_ENABLED`) via `CommonCLICallbacks` — see [CLI Commands](../../docs/cli_commands.md) for the full command reference (stats, neighbours, region config, ACLs, bridge/ethernet settings).
- Optionally drives an SSD1306-class display (`UITask.h`) and RS232/ESP-NOW "bridge" transports for daisy-chaining repeaters across a link that isn't LoRa (see `src/helpers/bridges/`).
- Reports itself with `FIRMWARE_ROLE "repeater"` (`MyMesh.h`) — purely descriptive, read by the CLI/stats and by the advert's type field; core routing doesn't treat repeaters specially beyond `allowPacketForward()`.

## Building

Environment names follow `<Board>_repeater[_<bridge-variant>]`:

```bash
pio run -e RAK_4631_repeater
pio run -e RAK_4631_repeater_ethernet
pio run -e RAK_4631_repeater_bridge_rs232_serial1
pio run -e Heltec_v3_repeater_bridge_espnow
```

## Configuring

Connect over USB serial at 115200 baud and use the CLI documented in [CLI Commands](../../docs/cli_commands.md), or manage the node remotely from a companion app via the Remote Management / LoRa admin feature.
