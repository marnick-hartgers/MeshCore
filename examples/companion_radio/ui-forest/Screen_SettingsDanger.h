#pragma once

#include "MenuScreen.h"
#include "NavStack.h"
#include "ToastOverlay.h"
#include "ConfirmScreen.h"

class UITask;   // avoid circular include with UITask.h, see Screen_SettingsDanger.cpp

#define UI_SETTINGS_DANGER_ITEM_COUNT 3

// Erase / rekey / reboot (phase-3-settings.md step 7) -- every one of these
// goes through the shared ConfirmScreen first (same instance Screen_Shutdown
// already uses, per ARCHITECTURE.md), no direct-invoke path from the menu.
// There is deliberately no "restore ALL defaults" action here -- item 21 /
// phase-3-settings.md step 8 describes restore-defaults as scoped PER
// SECTION (Radio/Advert/Network/Device each have their own), not one global
// action; adding a global one here would be redundant scope beyond what was
// asked.
class Screen_SettingsDanger : public MenuScreen {
  NavStack& _nav;
  ToastOverlay& _toast;
  ConfirmScreen& _confirm;
  UITask* _task;

  MenuItem _rows[UI_SETTINGS_DANGER_ITEM_COUNT];

  static void confirmErase(void* ctx);
  static void doErase(void* ctx);
  static void confirmRekey(void* ctx);
  static void doRekey(void* ctx);
  static void confirmReboot(void* ctx);
  static void doReboot(void* ctx);

public:
  Screen_SettingsDanger(NavStack& nav, ToastOverlay& toast, ConfirmScreen& confirm, UITask* task);
};
