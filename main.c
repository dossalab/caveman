#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

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

    for (uint8_t i = 0; i < sizeof(video_buffer); i++) {
        video_buffer[i] = (i % 2)? 0xff : 0x00;
    }

    setup_video();
    sei();

    DDRB |= (1 << PIN0);
    while (1) {
        PORTB ^= (1 << PIN0);
        _delay_ms(100);
    }

    return 0;
}
