#pragma once

#include <stdint.h>

extern uint8_t video_buffer[LINE_BUFFER_SIZE];

void setup_video();
void video_wait_vsync();
