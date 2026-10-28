/* Thiết kế: docs/design/input.md */
#include "ink_keymap.h"

#include <string.h>

/* ---- So gesture ---- */

static bool gesture_equal(const ink_gesture_t *a, const ink_gesture_t *b) {
  return a->modifier == b->modifier && a->source == b->source &&
         a->button == b->button && a->type == b->type;
}

static bool gesture_is_reserved(const ink_gesture_t *g) {
  return g->source == INK_GESTURE_SOURCE_BUTTON &&
         (g->type == INK_GESTURE_TYPE_HOLD_AB_SHORT ||
          g->type == INK_GESTURE_TYPE_HOLD_AB_LONG);
}

/* ---- Tra ---- */

static bool table_find(const ink_keymap_table_t *table,
                       const ink_gesture_t *gesture,
                       ink_keymap_intent_t *out_intent) {
  if (table == nullptr) {
    return false;
  }
  for (uint8_t i = 0; i < table->count && i < INK_KEYMAP_ENTRY_MAX; i++) {
    if (gesture_equal(&table->entries[i].gesture, gesture)) {
      *out_intent = table->entries[i].intent;
      return true;
    }
  }
  return false;
}

bool ink_keymap_lookup(const ink_keymap_table_t *scene,
                       const ink_keymap_table_t *app,
                       const ink_gesture_t *gesture,
                       ink_keymap_intent_t *out_intent) {
  if (gesture == nullptr || out_intent == nullptr) {
    return false;
  }
  return table_find(scene, gesture, out_intent) ||
         table_find(app, gesture, out_intent);
}

/* ---- Kiểm tra ---- */

static ink_keymap_error_t validate_table(const ink_keymap_table_t *table) {
  if (table->count > INK_KEYMAP_ENTRY_MAX) {
    return INK_KEYMAP_ERROR_COUNT;
  }
  for (uint8_t i = 0; i < table->count; i++) {
    if (gesture_is_reserved(&table->entries[i].gesture)) {
      return INK_KEYMAP_ERROR_RESERVED;
    }
    for (uint8_t j = 0; j < i; j++) {
      if (gesture_equal(&table->entries[i].gesture,
                        &table->entries[j].gesture)) {
        return INK_KEYMAP_ERROR_DUPLICATE;
      }
    }
  }
  return INK_KEYMAP_OK;
}

ink_keymap_error_t ink_keymap_validate(const ink_keymap_table_t *scene,
                                       const ink_keymap_table_t *app) {
  if (app == nullptr) {
    return INK_KEYMAP_ERROR_ARGUMENT;
  }
  ink_keymap_error_t error = validate_table(app);
  if (error != INK_KEYMAP_OK) {
    return error;
  }
  if (scene != nullptr) {
    error = validate_table(scene);
    if (error != INK_KEYMAP_OK) {
      return error;
    }
  }

  /* BACK phải tra ra được: dòng BACK của scene, hoặc dòng BACK của app mà scene
   * không đè */
  for (uint8_t i = 0; i < app->count; i++) {
    ink_keymap_intent_t effective;
    if (ink_keymap_lookup(scene, app, &app->entries[i].gesture, &effective) &&
        effective == INK_KEYMAP_INTENT_BACK) {
      return INK_KEYMAP_OK;
    }
  }
  if (scene != nullptr) {
    for (uint8_t i = 0; i < scene->count; i++) {
      if (scene->entries[i].intent == INK_KEYMAP_INTENT_BACK) {
        return INK_KEYMAP_OK;
      }
    }
  }
  return INK_KEYMAP_ERROR_NO_BACK;
}

ink_keymap_error_t ink_keymap_reset(ink_keymap_table_t *table,
                                    const ink_keymap_table_t *defaults) {
  if (table == nullptr || defaults == nullptr) {
    return INK_KEYMAP_ERROR_ARGUMENT;
  }
  memcpy(table, defaults, sizeof(*table));
  return INK_KEYMAP_OK;
}

/* ---- Mask kiểu phải chờ ---- */

static uint8_t table_wait_mask(const ink_keymap_table_t *table,
                               ink_gesture_button_t button) {
  uint8_t mask = INK_GESTURE_WAIT_NONE;
  if (table == nullptr) {
    return mask;
  }
  for (uint8_t i = 0; i < table->count && i < INK_KEYMAP_ENTRY_MAX; i++) {
    const ink_gesture_t *g = &table->entries[i].gesture;
    if (g->source != INK_GESTURE_SOURCE_BUTTON) {
      continue;
    }
    if (g->type == INK_GESTURE_TYPE_DOUBLE_PRESS && g->button == button) {
      mask |= INK_GESTURE_WAIT_DOUBLE;
    }
    if ((g->modifier & button) != 0) {
      mask |= INK_GESTURE_WAIT_MODIFIER;
    }
  }
  return mask;
}

uint8_t ink_keymap_wait_mask(const ink_keymap_table_t *scene,
                             const ink_keymap_table_t *app,
                             ink_gesture_button_t button) {
  if (button != INK_GESTURE_BUTTON_A && button != INK_GESTURE_BUTTON_B) {
    return INK_GESTURE_WAIT_NONE;
  }
  return table_wait_mask(scene, button) | table_wait_mask(app, button);
}
