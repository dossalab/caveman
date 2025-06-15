#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "generated-sprites.gen.h"
#include "video.h"
#include "util.h"

// on 12Mhz, we can run 12 instructions per 1us.
// PAL scanline is 64 us.
// sync is 4.7 us
// back porch is 5.7 us.
// front porch is 1.65 us
// so visible video is 52 us
// 
// we need 3 instructions to output data from line to port itself.
// so we can output 4 pixels per us.
// so that's around 200 pixels?

struct sprite sprites[] = {
    { .tcnt = 37, .y = 259, .proto = &annoying_dog_overworld_dangling_rope_png_proto },
    { .tcnt = 37, .y = 259 - 128, .proto = &annoying_dog_overworld_dangling_1_png_proto },
    { .tcnt = 23, .y = 50, .proto = &hello_png_proto},
};

int main() {
    DDRD = 0xff;

    DDRB |= (1 << PIN2);

    setup_video();
    sei();

    uint8_t counter = 0;
    uint8_t which_dog = 0;

    while (1) {
        video_wait_v_blank();

        PORTB |= (1 << PIN2);

        switch (which_dog) {
        case 0:
            sprites[1].proto = &annoying_dog_overworld_dangling_1_png_proto;
            break;
        case 1:
            sprites[1].proto = &annoying_dog_overworld_dangling_2_png_proto;
            break;
        case 2:
            sprites[1].proto = &annoying_dog_overworld_dangling_3_png_proto;
            break;
        case 3:
            sprites[1].proto = &annoying_dog_overworld_dangling_4_png_proto;
            break;
        case 4:
            sprites[1].proto = &annoying_dog_overworld_dangling_5_png_proto;
            break;
        case 5:
            sprites[1].proto = &annoying_dog_overworld_dangling_6_png_proto;
            break;
        case 6:
            sprites[1].proto = &annoying_dog_overworld_dangling_7_png_proto;
            break;
        case 7:
            sprites[1].proto = &annoying_dog_overworld_dangling_8_png_proto;
            break;
        case 8:
            sprites[1].proto = &annoying_dog_overworld_dangling_9_png_proto;
            break;
        }

        prepare_draw_call(sprites, ARRAY_SIZE(sprites));

        counter++;
        if (counter % 8 == 0) {
            which_dog = (which_dog + 1) % 9;
        }

        PORTB &= ~(1 << PIN2);
        video_wait_frame_start();
    }

    return 0;
}
