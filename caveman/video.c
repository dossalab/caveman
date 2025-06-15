#include <avr/io.h>
#include <avr/interrupt.h>

#include "video.h"
#include "video-internal.h"
#include <stdbool.h>
#include <string.h>
#include <util/delay.h>
#include "util.h"
#include <avr/pgmspace.h>

// 262 full, 2 blank lines (see below)
#define NORMAL_SCANLINES 260
#define VIDEO_COUNTER_RELOAD_VALUE NORMAL_SCANLINES

#define MAX_SPRITES 10
#define SPRITE_LIST_POISON 0xFF

struct generator_state {
    uint8_t sprite_list[MAX_SPRITES];
    uint8_t current_sprite;
    uint8_t sprite_line_counter;
    uint16_t video_line_counter;
    const struct sprite *sprites;

    struct {
        uint8_t is_v_blank;
        uint8_t waiting_h_sync;
    } status_bits;
};

static struct generator_state g_state = {
    .sprite_list = { [0] = SPRITE_LIST_POISON },
    .current_sprite = 0,
    .sprite_line_counter = 0,
    .video_line_counter = VIDEO_COUNTER_RELOAD_VALUE,
    .status_bits = {
        .is_v_blank = 0,
        .waiting_h_sync = 0,
    },
    .sprites = NULL
};

// Note that the actual value used in calculations should be +1
#define OCR_VALUE_SYNC_BACKPORCH 6
#define OCR_VALUE_SCANLINE 88

// 2 scanlines
#define OCR_VALUE_BLANK (192 + OCR_VALUE_SYNC_BACKPORCH)

// This really has to be atomic. There is a good chance though that this entire routine executes before the interrupt fires again
// Let's hope that's the case
void prepare_draw_call(const struct sprite *sprites, uint8_t len)
{
    uint8_t sprite_list_insert_position = 0;
    for (uint16_t line = NORMAL_SCANLINES - 1;; line--) {
        for (uint8_t i = 0; i < len; i++) {
            const struct sprite *sprite = &sprites[i];

            if (line == sprite->y) {
                g_state.sprite_list[sprite_list_insert_position++] = i;
            }
        }

        if (line == 0) {
            break;
        }
    }

    // terminate the list
    g_state.sprite_list[sprite_list_insert_position] = SPRITE_LIST_POISON;
    g_state.current_sprite = 0;
    g_state.sprites = sprites;
}

ISR(TIMER2_COMP_vect)  {
    if (g_state.status_bits.waiting_h_sync) {
        // Hardware already asserted sync pulse for us.
        if (!g_state.status_bits.is_v_blank) {
            // Skip backporch, and then draw
            OCR2 = OCR_VALUE_SYNC_BACKPORCH;
        } else {
            // If we're doing a vblank, just load a blank interval, and see you on the other side!
            OCR2 = OCR_VALUE_BLANK;
        }

        g_state.status_bits.waiting_h_sync = 0;
        return;
    }

    // Sync pulse de-asserted, we're ready to draw. Load the next OCR2 to wake us up when
    // the sync pulse should be happening
    OCR2 = OCR_VALUE_SCANLINE;

    // Start of visible line
    g_state.status_bits.is_v_blank = false;
    g_state.status_bits.waiting_h_sync = 1;

    g_state.video_line_counter--;

    barrier();
    uint8_t sprite_table_index = g_state.sprite_list[g_state.current_sprite];

    if (sprite_table_index != SPRITE_LIST_POISON) {
        barrier();
        const struct sprite *s = &g_state.sprites[sprite_table_index];
        const struct sprite_proto *proto = s->proto;

        if (g_state.video_line_counter <= s->y && g_state.sprite_line_counter < proto->height) {
            while (TCNT2 <= s->tcnt) {};

            void *call_address = pgm_read_ptr(&(proto->line_table[g_state.sprite_line_counter]));

            // GCC hates a function call here, which sucks. We can still do it in asm, though. We just need a correct address
            // Assumption - called function does not use any registers (edit - allow r18 for counters)
            asm __volatile__ (
                "icall\n\t"
                :: [addr] "z" (call_address) : "r18", "memory"
            );
            PORTD = 0;
            g_state.sprite_line_counter++;

            // end of drawing, go to next sprite
            if (g_state.sprite_line_counter >= proto->height) {
                g_state.current_sprite++;
                g_state.sprite_line_counter = 0;
            }
        }
    }


    if (g_state.video_line_counter == 0) {
        // Prepare the vertical blanking. Next time we enter this ISR, sync pulse will assert
        g_state.video_line_counter = VIDEO_COUNTER_RELOAD_VALUE;
        g_state.status_bits.is_v_blank = true;
    }
}

void setup_video(void)
{
    // Enable timer 2 OCR compare interrupt
    TIMSK |= (1 << OCIE2);

    // Setup desired pins to output
    VIDEO_DDR |= (1 << VIDEO_SYNC_BIT);

    // Initial OC2 value is 0. So we're 'done' the sync pulse, proceed straight to scanline
    g_state.status_bits.waiting_h_sync = 0;
    OCR2 = OCR_VALUE_SYNC_BACKPORCH;

    // And run the timer..
    TCNT2 = 0;
    TCCR2 = VIDEO_TIMER_VALUE_RUN;

    // Now CPU can do some work in the main loop until timer interrupt fires
}

void video_wait_v_blank()
{
    while (!g_state.status_bits.is_v_blank) {
        barrier();
        ;;
    }
}

void video_wait_frame_start()
{
    while (g_state.status_bits.is_v_blank) {
        barrier();
        ;;
    }
}
