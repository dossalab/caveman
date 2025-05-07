#include <avr/io.h>

#include "video.h"
#include "video-internal.h"

// these are used from ASM
// the order here is important, as asm code expects it to be like that
uint8_t video_line_counter;
uint8_t video_buffer[LINE_BUFFER_SIZE];

void setup_video(void)
{
    // Enable timer 2 OCR compare interrupt
    TIMSK |= (1 << OCIE2);

    // Tick is 0.083 us, so that gives us compare match on ~4.7us
    OCR2 = 56;

    // Setup desired pins to output
    VIDEO_DDR |=  (1 << VIDEO_SYNC_BIT);

    // Sync goes low..
    VIDEO_PORT &= ~(1 << VIDEO_SYNC_BIT);

    // And run the timer..
    TCCR2 = VIDEO_TIMER_VALUE_START;

    // Now CPU can do some work in the main loop until timer interrupt fires
}
