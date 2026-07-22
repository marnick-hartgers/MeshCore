# KISS Modem

A standard-KISS-TNC-compatible modem firmware that exposes the LoRa radio as a raw packet transport over serial, for use with existing KISS host software (e.g. Direwolf, APRSdroid) rather than a MeshCore-specific app.

## What it does

- Does **not** subclass `Mesh`/`Dispatcher` at all — it's a standalone `class KissModem` (`KissModem.h:100`) wrapping the identity, RNG, radio driver and board directly. There's no routing/dedup/contact logic here; it's a thin framing bridge.
- Implements standard KISS frame escaping (`KISS_FEND`/`FESC`/`TFEND`/`TFESC`, see `KissModem.cpp`) over UART/USB serial.
- Adds a MeshCore-specific `SetHardware` (`0x06`) KISS command for radio config, crypto operations, telemetry and stats that go beyond what plain KISS supports.
- `main.cpp`'s `loop()` pumps `modem->loop()`, feeds received radio bytes into `modem->onPacketReceived(...)`, and drives AGC/noise-floor calibration.

Full protocol reference: [KISS Modem Protocol](../../docs/kiss_modem_protocol.md).

## Building

Environment names follow `<Board>_kiss_modem`:

```bash
pio run -e RAK_4631_kiss_modem
pio run -e Heltec_v3_kiss_modem
```
