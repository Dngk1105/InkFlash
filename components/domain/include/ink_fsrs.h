#pragma once

#include <stdint.h>
#include "ink_review_log.h"

/*
 * FSRS (Free Spaced Repetition Scheduler) — CHƯA VIẾT, thuộc giai đoạn 2.
 * Header này chốt hình dạng API để các tầng trên lập trình trước. Host test sẽ đối chiếu với py-fsrs.
 */
typedef enum {
    INK_FSRS_NEW = 0,
    INK_FSRS_LEARNING,
    INK_FSRS_REVIEW,
    INK_FSRS_RELEARNING,
} ink_fsrs_phase_t;

typedef struct {
    float stability;  /* ngày */
    float difficulty; /* 1..10 */
    uint32_t due_s;   /* lần ôn kế tiếp, giây Unix */
    uint32_t last_s;
    uint16_t reps;
    uint16_t lapses;
    uint8_t phase;    /* ink_fsrs_phase_t */
} ink_fsrs_card_t;

typedef struct {
    float w[21];             /* bộ trọng số FSRS */
    float desired_retention; /* ví dụ 0.9 */
    uint32_t max_interval_days;
} ink_fsrs_params_t;

void ink_fsrs_default_params(ink_fsrs_params_t *p);
/* Cập nhật trạng thái thẻ sau một lần chấm điểm lúc now_s. */
void ink_fsrs_review(const ink_fsrs_params_t *p, ink_fsrs_card_t *card, ink_grade_t grade, uint32_t now_s);
