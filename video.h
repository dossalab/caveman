#pragma once

#include <stdint.h>

extern uint8_t video_buffer[LINE_BUFFER_SIZE];

struct sprite {
    uint16_t line;
    uint8_t line_counter;
};

void setup_video();

void video_wait_v_blank();
void video_wait_frame_start();
