#pragma once
#include <assert.h>

#define ASSERT assert

namespace NextGen {

    enum pin_mode : uint8_t {
        INPUT,
        OUTPUT,
        PWM_TONE,
        PWM,
        CLOCK
    };

    enum pull_mode : uint8_t {
        OFF      = 0x00,
        DOWN     = 0x01,
        UP       = 0x02,
        NEGATIVE = 0x08 
    };

    enum trigger_mode : uint8_t {
        NONE     = 0x00,
        FALLING  = 0x01,
        RISING   = 0x02,
        BOTH     = 0x03,
        HIGH     = 0x04,
        LOW      = 0x08
    };
}

