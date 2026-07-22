# Simple Sensor

A remote telemetry node: reads attached sensors, records time-series history on-device, raises threshold alerts over the mesh, and answers telemetry queries from other nodes.

## What it does

- `class SensorMesh : public mesh::Mesh, public CommonCLICallbacks` (`SensorMesh.h:49`) holds the shared sensor/telemetry logic; the example's `class MyMesh : public SensorMesh` (`main.cpp:8`) fills in the board-specific pieces by overriding `onSensorDataRead()` and `querySeriesData()`.
- Reads whichever sensors are enabled at build time via the `ENV_INCLUDE_*` flags in the `[sensor_base]` PlatformIO section (GPS, AHT20, BME280/BMP280, SHTC3, SHT4x, LPS22HB, INA3221/219/226/260, MLX90614, VL53L0X, BME680, BMP085 — see `src/helpers/sensors/`).
- Records history in `TimeSeriesData.cpp/.h` (e.g. rolling battery-voltage samples) and raises alerts via a `Trigger`/`alertIf` mechanism when a threshold is crossed.
- Exposes the standard `CommonCLICallbacks` admin CLI ([CLI Commands](../../docs/cli_commands.md)) plus telemetry-specific queries, and reports itself with `FIRMWARE_ROLE "sensor"` (`SensorMesh.h`).

## Building

Environment names follow `<Board>_sensor`. Which physical sensors get compiled in is controlled by the board variant's `platformio.ini` (via `[sensor_base]`), not by the example itself:

```bash
pio run -e RAK_4631_sensor
pio run -e Heltec_v3_sensor
```
