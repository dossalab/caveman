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
// uint8_t video_buffer[LINE_BUFFER_SIZE];

struct {
    uint8_t is_v_blank:1;
    uint8_t waiting_h_sync:1;
} video_status;

// Note that the actual value used in calculations should be +1
#define OCR_VALUE_SYNC_BACKPORCH 6
#define OCR_VALUE_SCANLINE 88

// 2 scanlines
#define OCR_VALUE_BLANK (192 + OCR_VALUE_SYNC_BACKPORCH)

extern void sprites_sprite_png_start(void);
extern void sprites_snake_png_start(void);
extern void sprites_monk_png_start(void);

struct sprite my_sprite_list[] = {
    {.line = 50, .line_counter = 0, .data_start = sprites_sprite_png_start, .width = 16, .height = 16 },
    {.line = 200, .line_counter = 0, .data_start = sprites_monk_png_start, .width = 64, .height = 16 },
    {.line = 260, .line_counter = 0, .data_start = sprites_snake_png_start, .width = 16, .height = 16 },
};

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

void (*video_line_jumptable[NORMAL_SCANLINES])(void);

void empty_line(void) {
}

void blink_line(void) {
    PORTD |= (1 << PIN7);
    PORTD &= ~(1 << PIN7);
}

void build_jumptable()
{
    for (uint8_t i = 0; i < ARRAY_SIZE(my_sprite_list); i++) {
        my_sprite_list[i].line_counter = 0;
    }

    for (uint16_t line = NORMAL_SCANLINES - 1;; line--) {
        video_line_jumptable[line] = empty_line;

        for (uint8_t i = 0; i < ARRAY_SIZE(my_sprite_list); i++) {
            struct sprite *sprite = &my_sprite_list[i];

            if (line <= sprite->line && sprite->line_counter < sprite->height) {
                video_line_jumptable[line] = sprite->data_start + sprite->line_counter * sprite->width;
                sprite->line_counter++;
            }
        }

        if (line == 0) {
            break;
        }
    }
}

ISR(TIMER2_COMP_vect)  {
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

    // delay scanline slightly
    asm __volatile__ (
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
    );

    // for (uint8_t i = 0; i < ARRAY_SIZE(my_sprite_list); i++) {
    //     struct sprite *sprite = &my_sprite_list[i];

    //     if (video_line_counter <= sprite->line && sprite->line_counter < 16) {
    //         PORTB ^= (1 << PIN2);

    //         asm __volatile__ (
    //             "mov r18, %[line]\n\t"
    //             "lsl r18\n\t"
    //             "lsl r18\n\t"
    //             "lsl r18\n\t"
    //             "lsl r18\n\t"

    //             "ldi r30, pm_lo8(sprites_sprite_png_start)\n\t"
    //             "ldi r31, pm_hi8(sprites_sprite_png_start)\n\t"

    //             "add r30, r18\n\t"
    //             "adc r31, __zero_reg__\n\t"

    //             "icall\n\t"
    //             :: [line] "a" (sprite->line_counter): "r30", "r31", "r18"
    //         );
    //         PORTD = 0;

    //         sprite->line_counter++;
    //     }
    // }

    // GCC hates a function call here, which sucks. We can still do it in asm, though. We just need a correct address
    // Assumption - called function does not use any registers
    video_line_counter--;

    asm __volatile__ (
        "icall\n\t"
        :: [addr] "z" (video_line_jumptable[video_line_counter])
    );
    PORTD = 0;

    if (video_line_counter == 0) {
        // Prepare the vertical blanking. Next time we enter this ISR, sync pulse will assert
        video_line_counter = VIDEO_COUNTER_RELOAD_VALUE;

        // for (uint8_t i = 0; i < ARRAY_SIZE(my_sprite_list); i++) {
        //     my_sprite_list[i].line_counter = 0;
        // }

        video_status.is_v_blank = true;
    }
}

void setup_video(void)
{
    for (uint16_t i = 0; i < ARRAY_SIZE(video_line_jumptable); i++) {
        video_line_jumptable[i] = empty_line;
    }

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
