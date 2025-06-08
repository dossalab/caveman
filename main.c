#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "generated-sprites.h"
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

struct sprite key_frames[] = {
    { .y = 100, .proto = &undyne_png_proto},
    // { .y = 100, .proto = &annoying_dog_2_png_proto},
};

int main() {
    DDRD = 0xff;

    DDRB |= (1 << PIN2);

    setup_video();
    sei();

    uint8_t index = 0;
    uint8_t counter = 0;

    while (1) {
        video_wait_v_blank();

        PORTB |= (1 << PIN2);

        prepare_draw_call(&key_frames[index % ARRAY_SIZE(key_frames)], 1);

        counter++;
        if (counter > 6) {
            counter = 0;
            index++;
        }

        PORTB &= ~(1 << PIN2);
        video_wait_frame_start();
    }

    return 0;
}
