#pragma once

#include <helpers/ui/UIScreen.h>

#define NAV_STACK_MAX_DEPTH 8

// Fixed-depth stack of UIScreen pointers. Screens are constructed once (in
// UITask::begin()) and reused -- push()/pop() only ever move pointers, never
// allocate. A screen pushes another screen when the user drills in (e.g.
// selecting a menu item) and KEY_CANCEL pops back to it.
//
// Phase 5 tried a push()/pop() screen-transition animation here (a solid-bar
// wipe using fillRect()/startFrame()/endFrame()), per PLAN.md item 29. Pulled
// back out after user feedback that it read as a flash/glitch rather than a
// transition -- every DisplayDriver backend fully clears its buffer on
// startFrame() (no partial update anywhere in this framework), so any
// animation built from real display updates is necessarily a sequence of
// full-screen redraws, not a smooth compositing effect; on top of that, most
// of these screens are monochrome, where a "wipe" is just a hard-edged block
// of on/off pixels growing across the panel, not a gradient. Rather than
// keep guessing at timings/step-counts with no way to see the result, this
// was removed -- no transition is a safer default than a wrong-looking one.
// See PROGRESS.md's Phase 5 section for the full account.
class NavStack {
  UIScreen* _stack[NAV_STACK_MAX_DEPTH];
  int _depth;

public:
  NavStack() : _depth(0) { }

  void push(UIScreen* screen) {
    if (_depth >= NAV_STACK_MAX_DEPTH) return;  // full, drop silently
    _stack[_depth++] = screen;
  }

  void pop() {
    if (_depth > 1) _depth--;  // never pop the root screen
  }

  void popToRoot() {
    if (_depth > 1) _depth = 1;
  }

  // Replace the entire stack with a single root screen (e.g. splash -> home).
  void reset(UIScreen* root) {
    _stack[0] = root;
    _depth = 1;
  }

  UIScreen* current() const {
    return _depth > 0 ? _stack[_depth - 1] : NULL;
  }

  int depth() const { return _depth; }
};
