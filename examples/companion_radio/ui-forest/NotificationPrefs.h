#pragma once

// Per-event-type notification config (PLAN.md item 36 / phase-7-notifications.md
// step 2 -- "per-event-type configuration: buzzer tune + vibration + LED...
// replacing the single global mute toggle as the primary configuration
// surface"). LED isn't included here: today's UITask::userLedHandler() is a
// generic heartbeat blink keyed off _msgcount, never routed through notify()
// or any per-event trigger point at all, so there is nothing per-event to gate
// -- adding one would be new LED-trigger plumbing beyond "extend to whatever
// buzzer/vibration/LED code already distinguishes", not a config knob on top
// of something that exists.
//
// Deliberately NOT a NodePrefs field -- same reasoning as PROGRESS.md's Phase
// 3 "Vibration" decision: NodePrefs is a hand-maintained, unversioned,
// fixed-byte-offset binary format (DataStore.cpp's loadPrefsInt()/savePrefs())
// with no length guard, and extending it without a compiler on PATH to verify
// the change is real structural surgery on existing users' saved prefs files,
// not a UI-only change. So this resets to all-on on every reboot -- a known
// gap, flagged in PROGRESS.md, same shape as the vibration-persistence gap.
//
// Only the four event types phase-7-notifications.md names as the minimum
// (message/channel message/ack/advert) are independently configurable here.
// roomMessage/newContactMessage aren't exposed in Screen_NotificationSettings
// and stay unconditionally on in UITask::notify(), unchanged from pre-Phase-7
// behavior -- they don't play a distinct buzzer tune today either, so there's
// nothing meaningful to gate per-type for them yet.
struct NotificationTypeConfig {
  bool buzzer = true;
  bool vibration = true;
};

struct NotificationPrefs {
  NotificationTypeConfig contactMessage;
  NotificationTypeConfig channelMessage;
  NotificationTypeConfig ack;
  NotificationTypeConfig advertSent;
};
