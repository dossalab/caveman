#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

#include "video.h"

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

extern struct sprite my_sprite_list[2];

int main() {
    DDRD = 0xff;

    DDRB |= (1 << PIN2);
    setup_video();
    sei();

    while (1) {
        video_wait_v_blank();
        my_sprite_list[0].y++;
        if (my_sprite_list[0].y > 100) {
            my_sprite_list[0].y = 16;
        }

        PORTB |= (1 << PIN2);
        build_jumptable();
        PORTB &= ~(1 << PIN2);
        video_wait_frame_start();
    }

    return 0;
}
