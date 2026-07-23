#pragma once

#include <Arduino.h>
#include <Wire.h>

#ifndef TDECK_KEYBOARD_I2C_ADDR
  #define TDECK_KEYBOARD_I2C_ADDR 0x55
#endif

// LilyGo T-Deck's keyboard is driven by its own onboard co-processor (not a
// raw TCA8418-style matrix controller this code would need to decode itself)
// that scans the physical key matrix and exposes whichever key is currently
// pressed as a single ASCII byte over I2C: reading one byte from
// TDECK_KEYBOARD_I2C_ADDR returns that character, or 0 if nothing is
// pressed. This matches the community-published T-Deck keyboard protocol
// (the same one used by, among others, Meshtastic's T-Deck input driver and
// several open T-Deck example sketches) -- it has NOT been verified against
// a schematic or datasheet in this repo, since no T-Deck hardware is
// available in this dev environment (see PROGRESS.md's Phase 6 section). If
// the keyboard never produces a character on real hardware, this address/
// protocol assumption is the first thing to check.
class TDeckKeyboard {
  char _last;

public:
  TDeckKeyboard() : _last(0) { }

  // No pinMode/Wire.begin() here -- Wire is already begun by radio_init()
  // (target.cpp), which runs well before UITask::begin() calls into this.

  // Returns the newly-pressed character, or 0 if nothing changed (including
  // a held key, or the key being released). Edge-triggered on the raw byte
  // changing away from 0 so FormField's TextField sees one character per
  // physical keypress rather than an auto-repeating stream while a key is
  // held -- if the co-processor's protocol turns out to behave differently
  // on real hardware (e.g. it buffers a FIFO instead of reporting "currently
  // pressed key"), this is the method to revisit.
  char poll() {
    Wire.requestFrom(TDECK_KEYBOARD_I2C_ADDR, 1);
    char c = 0;
    if (Wire.available()) c = (char)Wire.read();
    char result = (c != 0 && c != _last) ? c : 0;
    _last = c;
    return result;
  }
};
