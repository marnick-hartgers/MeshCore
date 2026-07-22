# Companion Radio

Bridge firmware for the official MeshCore apps (web, Android, iOS, Node.js, Python). It exposes the mesh to a companion app over BLE, USB-serial, Wi-Fi (TCP) or Ethernet, and manages a local contact/channel address book and message store on the device itself.

## What it does

- Subclasses `BaseChatMesh` (`MyMesh.h:87`, `class MyMesh : public BaseChatMesh, public DataStoreHost`), so it gets contact/channel lookup for free and only implements the app-specific behaviour (message queuing, telemetry, ACLs to the paired app).
- Talks a binary command/response protocol — see [Companion Protocol](../../docs/companion_protocol.md) for the full frame/command reference, and [Stats Binary Frames](../../docs/stats_binary_frames.md) for the `CMD_GET_STATS` payload layout.
- Picks its transport in `main.cpp` at build time: `SerialBLEInterface`, `SerialWifiInterface`, `ArduinoSerialInterface` (USB), or `SerialEthernetInterface`, all implementing `BaseSerialInterface`. Incoming frames are read with `checkRecvFrame()` and dispatched to `MyMesh::handleCmdFrame()` (`MyMesh.cpp`).
- Persists contacts/channels/messages via a `DataStore` (see `DataStore.h/.cpp`), and drives one of three optional UI implementations (`ui-new`, `ui-orig`, `ui-tiny`) behind `AbstractUITask.h`, selected per board variant.

## Building

Environment names follow `<Board>_companion_radio_<transport>`. Examples (see `variants/rak4631/platformio.ini`, `variants/heltec_v3/platformio.ini`):

```bash
pio run -e RAK_4631_companion_radio_ble
pio run -e RAK_4631_companion_radio_usb
pio run -e RAK_4631_companion_radio_ethernet
pio run -e Heltec_v3_companion_radio_wifi
```

## Connecting

Use one of the official clients: the [web app](https://app.meshcore.nz), Android/iOS apps, [meshcore.js](https://github.com/liamcottle/meshcore.js), or [meshcore-cli](https://github.com/fdlamotte/meshcore-cli) (Python). For Wi-Fi/BLE PIN and other connection details, see the [FAQ](../../docs/faq.md).
