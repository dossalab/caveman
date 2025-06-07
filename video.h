#pragma once

#include <stdint.h>

struct sprite {
    uint16_t line;
    void (*data_start)(void);
    uint8_t width, height;
};

void setup_video();

void video_wait_v_blank();
void video_wait_frame_start();
void build_jumptable();
