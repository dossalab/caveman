#pragma once

#include <stdint.h>

typedef void (*video_line_func_t)(void);

struct sprite_proto {
    uint8_t width, height;
    const video_line_func_t *line_table;
};

struct sprite {
    uint16_t y;
    uint8_t tcnt;
    const struct sprite_proto *proto;
};

void setup_video(void);

void video_wait_v_blank(void);
void video_wait_frame_start(void);
void prepare_draw_call(const struct sprite *sprites, uint8_t len);
