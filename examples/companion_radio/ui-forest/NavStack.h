#pragma once

#include <helpers/ui/UIScreen.h>

#define NAV_STACK_MAX_DEPTH 8

// Fixed-depth stack of UIScreen pointers. Screens are constructed once (in
// UITask::begin()) and reused -- push()/pop() only ever move pointers, never
// allocate. A screen pushes another screen when the user drills in (e.g.
// selecting a menu item) and KEY_CANCEL pops back to it.
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
