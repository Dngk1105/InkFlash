#include "ink_service.h"

/* Audio: thu mic I2S, phát loa qua MAX98357A. TODO: cần driver audio_hw. */
const ink_service_def_t svc_audio_def = {
    .name = "svc_audio",
    .comp = INK_COMP_SVC_AUDIO,
    .stack_size = 4096,
    .priority = 5,
};
