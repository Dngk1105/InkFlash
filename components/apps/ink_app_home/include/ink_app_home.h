/* Design: docs/design/app-framework.md */
#pragma once

/*
 * Home: the first app of the list (apps[0]). One scene lists every other app and a final item that puts the device to
 * sleep. system opens Home at boot, on HOLD_AB_SHORT, and when BACK leaves the root scene of another app.
 * Hand drawn for now; it will be redone from the Figma design.
 */
#include "ink_system_app.h"

extern const ink_system_app_t ink_app_home;
