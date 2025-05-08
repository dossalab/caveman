#include <avr/io.h>
#include <avr/interrupt.h>

#include "video.h"
#include "video-internal.h"
#include "generated-asm.h"
#include <stdbool.h>

// these are used from ASM
// the order here is important, as asm code expects it to be like that
uint16_t video_line_counter;
uint8_t video_buffer[LINE_BUFFER_SIZE];
uint8_t blank_buffer[LINE_BUFFER_SIZE];


struct {
    uint8_t is_blank:1;
    uint8_t sync_high:1;
} video_status;

#define TOTAL_SCANLINES 312
#define BLANK_SCANLINES 5

extern void do_scanline(uint8_t *buffer);

ISR(TIMER2_COMP_vect) {
    if (!video_status.sync_high) {
        // first occurance, raise it back and exit
        video_status.sync_high = 1;

        // blanks does not need the pulses, so ignore it
        if (!video_status.is_blank) {
            VIDEO_PORT |= (1U << VIDEO_SYNC_BIT);
        }

        return;
    }

    TCCR2 = VIDEO_TIMER_VALUE_STOP;

    uint8_t *buffer_to_use = video_status.is_blank? blank_buffer : video_buffer;
    inline_asm_generate_scanline(buffer_to_use);
    PORTD = 0;

    video_line_counter--;
    if (video_line_counter == 0) {
        video_status.is_blank = false;
        video_line_counter = TOTAL_SCANLINES;
    } else if (video_line_counter <= BLANK_SCANLINES) {
        video_status.is_blank = true;
    }

    video_status.sync_high = 0;
    TCCR2 = VIDEO_TIMER_VALUE_START;
    VIDEO_PORT &= ~(1U << VIDEO_SYNC_BIT);
}

// The problem is that ISR itself has some latency so we have to offset our OCR2 setting here
// This is calibrated using a logic analyzer for the most stable image :)
#define OCR2_SKEW 4

void setup_video(void)
{
    // Enable timer 2 OCR compare interrupt
    TIMSK |= (1 << OCIE2);

    // Tick is 0.083 us, so that gives us compare match on ~4.7us
    OCR2 = 56 - OCR2_SKEW;

    // Setup desired pins to output
    VIDEO_DDR |=  (1 << VIDEO_SYNC_BIT);

    // Sync goes low..
    VIDEO_PORT &= ~(1 << VIDEO_SYNC_BIT);

    // And run the timer..
    TCCR2 = VIDEO_TIMER_VALUE_START;

    // Now CPU can do some work in the main loop until timer interrupt fires
}

void video_wait_vsync()
{
    while (!video_status.is_blank) {
        asm volatile ("" ::: "memory");
        ;;
    }
}
