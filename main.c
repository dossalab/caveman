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

    DDRB |= (1 << PIN2);
    setup_video();
    sei();

    while (1) {
    }

    return 0;
}
