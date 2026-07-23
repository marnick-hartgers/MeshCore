#pragma once

// Resolves PLAN.md 8 / phase-4-diagnostics.md step 6's "transport-type story"
// open question: main.cpp (examples/companion_radio/main.cpp) picks exactly
// one BaseSerialInterface subclass per board at compile time, keyed off these
// same macros (WIFI_SSID -> SerialWifiInterface, BLE_PIN_CODE ->
// SerialBLEInterface, ETHERNET_ENABLED -> SerialEthernetInterface, else ->
// ArduinoSerialInterface for both the SERIAL_RX and plain-USB cases). There is
// no runtime signal for "which transport is this" -- isSerialEnabled()/
// hasConnection() report whether the *chosen* transport is enabled/connected,
// not which one it is. So this is set once as a compile-time string constant,
// not queried per frame.
#if defined(WIFI_SSID)
  #define UI_FOREST_TRANSPORT_NAME "WiFi"
#elif defined(BLE_PIN_CODE)
  #define UI_FOREST_TRANSPORT_NAME "BLE"
#elif defined(ETHERNET_ENABLED)
  #define UI_FOREST_TRANSPORT_NAME "Ethernet"
#else
  #define UI_FOREST_TRANSPORT_NAME "USB"
#endif
