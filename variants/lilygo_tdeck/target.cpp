#include <Arduino.h>
#include "target.h"

TDeckBoard board;

#if defined(P_LORA_SCLK)
  static SPIClass spi;
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, spi);
#else
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY);
#endif

WRAPPER_CLASS radio_driver(radio, board);

ESP32RTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);
MicroNMEALocationProvider gps(Serial1, &rtc_clock);
EnvironmentSensorManager sensors(gps);

#ifdef DISPLAY_CLASS
  DISPLAY_CLASS display;
  MomentaryButton user_btn(PIN_USER_BTN, 1000, true);
  // Trackball directions (Phase 6, item 34): no long-press meaning, and
  // multiclick detection deliberately off (last ctor arg) -- a rolling
  // trackball fires rapid successive pulses that must each become one
  // KEY_UP/DOWN/LEFT/RIGHT immediately, not get held for a double/triple-
  // click window the way a real button press would (see MomentaryButton.cpp:
  // with multiclick=false, check() returns BUTTON_EVENT_CLICK right on
  // release instead of waiting ~280ms to see if another click follows).
  // reverse=true, pull=false mirrors user_btn's own wiring above -- same
  // physical assembly, assumed same external-pull-already-on-PCB electrical
  // characteristics.
  MomentaryButton trackball_up(TDECK_TRACKBALL_UP, 0, true, false, false);
  MomentaryButton trackball_down(TDECK_TRACKBALL_DOWN, 0, true, false, false);
  MomentaryButton trackball_left(TDECK_TRACKBALL_LEFT, 0, true, false, false);
  MomentaryButton trackball_right(TDECK_TRACKBALL_RIGHT, 0, true, false, false);
  TDeckKeyboard tdeck_keyboard;
#endif

bool radio_init() {
  fallback_clock.begin();
  rtc_clock.begin(Wire);
  Wire.begin(18, 8);

#if defined(P_LORA_SCLK)
  return radio.std_init(&spi);
#else
  return radio.std_init();
#endif
}

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng); // create new random identity
}
