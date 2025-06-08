#pragma once

#include <stdint.h>

struct sprite_proto {
    uint8_t width, height, stride;
    uint16_t *data_start;
};

struct sprite {
    uint16_t y;
    const struct sprite_proto *proto;
};

void setup_video();

void video_wait_v_blank();
void video_wait_frame_start();
void prepare_draw_call(const struct sprite *sprites, uint8_t len);
