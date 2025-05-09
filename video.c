#include <avr/io.h>
#include <avr/interrupt.h>

#include "video.h"
#include "video-internal.h"
#include "generated-asm.h"
#include <stdbool.h>
#include <util/delay.h>

// 312 full, 2 blank lines (see below)
#define NORMAL_SCANLINES 310
#define VIDEO_COUNTER_RELOAD_VALUE NORMAL_SCANLINES

// these are used from ASM
// the order here is important, as asm code expects it to be like that
uint16_t video_line_counter = VIDEO_COUNTER_RELOAD_VALUE;
uint8_t video_buffer[LINE_BUFFER_SIZE];

struct {
    uint8_t is_v_blank:1;
    uint8_t waiting_h_sync:1;
} video_status;

// Note that the actual value used in calculations should be +1
#define OCR_VALUE_SYNC_BACKPORCH 6
#define OCR_VALUE_SCANLINE 88

// 2 scanlines
#define OCR_VALUE_BLANK (192 + OCR_VALUE_SYNC_BACKPORCH)

ISR(TIMER2_COMP_vect) {
    if (video_status.waiting_h_sync) {
        // Hardware already asserted sync pulse for us.
        if (!video_status.is_v_blank) {
            // Skip backporch, and then draw
            OCR2 = OCR_VALUE_SYNC_BACKPORCH;
        } else {
            // If we're doing a vblank, just load a blank interval, and see you on the other side!
            OCR2 = OCR_VALUE_BLANK;
        }

        video_status.waiting_h_sync = 0;
        return;
    }

    // Sync pulse de-asserted, we're ready to draw. Load the next OCR2 to wake us up when
    // the sync pulse should be happening
    OCR2 = OCR_VALUE_SCANLINE;

    // Start of visible line
    video_status.is_v_blank = false;
    video_status.waiting_h_sync = 1;

    inline_asm_generate_scanline(video_buffer);
    PORTD = 0;

    video_line_counter--;
    if (video_line_counter == 0) {
        // Prepare the vertical blanking. Next time we enter this ISR, sync pulse will assert
        video_line_counter = VIDEO_COUNTER_RELOAD_VALUE;
        video_status.is_v_blank = true;
    }
}

void setup_video(void)
{
    // Enable timer 2 OCR compare interrupt
    TIMSK |= (1 << OCIE2);

    // Setup desired pins to output
    VIDEO_DDR |= (1 << VIDEO_SYNC_BIT);

    // Initial OC2 value is 0. So we're 'done' the sync pulse, proceed straight to scanline
    video_status.waiting_h_sync = 0;
    OCR2 = OCR_VALUE_SYNC_BACKPORCH;

    // And run the timer..
    TCNT2 = 0;
    TCCR2 = VIDEO_TIMER_VALUE_RUN;

    // Now CPU can do some work in the main loop until timer interrupt fires
}

void video_wait_v_blank()
{
    while (!video_status.is_v_blank) {
        asm volatile ("" ::: "memory");
        ;;
    }
}

void video_wait_frame_start()
{
    while (video_status.is_v_blank) {
        asm volatile ("" ::: "memory");
        ;;
    }
}
