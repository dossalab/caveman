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

int main() {
    DDRD = 0xff;

    video_buffer[89] = 0xff;
    // memset(video_buffer, 0xff, LINE_BUFFER_SIZE);

    setup_video();
    sei();

    DDRB |= (1 << PIN0);

    uint8_t pos_x = 0;
    while (1) {
        video_wait_v_blank();

        video_buffer[pos_x] = 0x00;
        video_buffer[pos_x + 1] = 0xff;

        pos_x ++;
        if (pos_x >= LINE_BUFFER_SIZE - 1) {
            pos_x = 0;
        }

        // Keep that here to syncronize this loop with our FPS
        video_wait_frame_start();
    }

    return 0;
}
