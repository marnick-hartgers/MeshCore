#pragma once

#include <Arduino.h>
#include <string.h>

#ifndef EVENT_LOG_CAPACITY
  #define EVENT_LOG_CAPACITY 20
#endif

struct EventLogEntry {
  char text[28];
  unsigned long timestamp;   // millis() at push time
};

// Small in-RAM ring buffer, fed by UITask/Screen_* at the points they already
// observe something worth logging (PLAN.md 3.4's "Event log" row -- rescoped
// away from a file-backed log, companion_radio has no such subsystem). Header-
// only, fixed array, no allocation -- same discipline as NavStack. Kept
// separate from Screen_EventLog (the viewer UIScreen) so Phase 7's
// Screen_RecentEvents can reuse this same buffer class for its own instance
// (PLAN.md 7's note on EventLog vs RecentEvents: separate screens/audiences,
// can share the buffer class).
class EventLog {
  EventLogEntry _entries[EVENT_LOG_CAPACITY];
  int _count;   // number of valid entries, saturates at EVENT_LOG_CAPACITY
  int _head;    // index the next push() will write to

public:
  EventLog() : _count(0), _head(0) { }

  void push(const char* text) {
    EventLogEntry& e = _entries[_head];
    strncpy(e.text, text, sizeof(e.text) - 1);
    e.text[sizeof(e.text) - 1] = 0;
    e.timestamp = millis();

    _head = (_head + 1) % EVENT_LOG_CAPACITY;
    if (_count < EVENT_LOG_CAPACITY) _count++;
  }

  int count() const { return _count; }

  // idxFromNewest: 0 = most recently pushed entry. NULL if out of range.
  const EventLogEntry* getEntry(int idxFromNewest) const {
    if (idxFromNewest < 0 || idxFromNewest >= _count) return NULL;
    int i = (_head - 1 - idxFromNewest + EVENT_LOG_CAPACITY) % EVENT_LOG_CAPACITY;
    return &_entries[i];
  }
};
