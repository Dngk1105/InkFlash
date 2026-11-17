/* Design: docs/design/app-framework.md */
#pragma once

/*
 * Template of an app: the smallest app that builds. One root scene shows a counter; NEXT adds one, SELECT clears it,
 * BACK is left to system (it returns to Home). An app exports one const ink_system_app_t and nothing else; system
 * calls it through that struct and the app calls system, display and ui only (no storage, no hardware).
 */
#include "ink_system_app.h"

extern const ink_system_app_t ink_app_example;
