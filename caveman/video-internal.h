#pragma once

#define VIDEO_PORT PORTB
#define VIDEO_DDR DDRB
#define VIDEO_SYNC_BIT 3


// Pre-calculate these values to make stopping and starting timer faster

// Stopped but configured timer (CTC, OC disconnected)
#define VIDEO_TIMER_VALUE_STOP \
    ((1 << WGM21) | (0 << WGM20) | (0 << COM21) | (1 << COM20))

// Same, but run the timer
#define VIDEO_TIMER_VALUE_RUN \
    (VIDEO_TIMER_VALUE_STOP | (1 << CS21))
