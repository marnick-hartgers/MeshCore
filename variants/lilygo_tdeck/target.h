#pragma once

#define RADIOLIB_STATIC_ONLY 1
#include <RadioLib.h>
#include <helpers/radiolib/RadioLibWrappers.h>
#include <helpers/radiolib/CustomSX1262Wrapper.h>
#include <TDeckBoard.h>
#include <helpers/AutoDiscoverRTCClock.h>
#include <helpers/SensorManager.h>
#ifdef DISPLAY_CLASS
  #include <helpers/ui/ST7789LCDDisplay.h>
  #include <helpers/ui/MomentaryButton.h>
  #include "TDeckKeyboard.h"
#endif
#include "helpers/sensors/EnvironmentSensorManager.h"
#include "helpers/sensors/MicroNMEALocationProvider.h"

extern TDeckBoard board;
extern WRAPPER_CLASS radio_driver;
extern AutoDiscoverRTCClock rtc_clock;
extern EnvironmentSensorManager sensors;

// Trackball direction pins (Phase 6, item 34) -- the widely-published T-Deck
// v1 pinout (also used by Meshtastic's T-Deck target and LilyGo's own
// example sketches), not verified against a schematic in this repo. Override
// via build_flags if a board revision turns out to differ. PIN_USER_BTN=0
// (trackball click) is the board's existing pin, unchanged.
#ifndef TDECK_TRACKBALL_UP
  #define TDECK_TRACKBALL_UP    3
#endif
#ifndef TDECK_TRACKBALL_DOWN
  #define TDECK_TRACKBALL_DOWN  15
#endif
#ifndef TDECK_TRACKBALL_LEFT
  #define TDECK_TRACKBALL_LEFT  1
#endif
#ifndef TDECK_TRACKBALL_RIGHT
  #define TDECK_TRACKBALL_RIGHT 2
#endif

#ifdef DISPLAY_CLASS
  extern DISPLAY_CLASS display;
  extern MomentaryButton user_btn;
  extern MomentaryButton trackball_up;
  extern MomentaryButton trackball_down;
  extern MomentaryButton trackball_left;
  extern MomentaryButton trackball_right;
  extern TDeckKeyboard tdeck_keyboard;
#endif

bool radio_init();
mesh::LocalIdentity radio_new_identity();
