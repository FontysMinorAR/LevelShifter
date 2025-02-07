#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <cstring>
#include <pico/stdlib.h>
#include <pico/stdio_usb.h>
#include <hardware/adc.h>

#include "pwmpin.h"

static void state_change (const uint8_t pinId, const uint32_t events);

static const int MinimumFrequency = 20000;

static NextGen::IOPin   out0(6);
static NextGen::IOPin   out1(5);
static NextGen::IOPin   out2(19);
static NextGen::IOPin   out3(20);
static NextGen::IOPin   in0(10, state_change, NextGen::trigger_mode::BOTH, true);
static NextGen::IOPin   in1(9, state_change, NextGen::trigger_mode::BOTH, true);
static NextGen::IOPin   in2(8, state_change, NextGen::trigger_mode::BOTH, true);
static NextGen::IOPin   in3(7, state_change, NextGen::trigger_mode::BOTH, true);
static NextGen::PWMPin  PWMPin0(17);
static NextGen::PWMPin  PWMPin1(18);
static NextGen::IOPin   ctrl0(21);
static NextGen::IOPin   ctrl1(22);

// Analog0 => 26
// Analog1 => 27
// Analog1 => 28


static void show_pin_details(const uint8_t pin) {
    const char* pullUp = (gpio_is_pulled_up(pin) ? " up " : gpio_is_pulled_down(pin) ? "down" : "    ");
    const char* direction = (gpio_get_dir(pin) == 1 ? "out" : "in ");
    const char* value = gpio_get(pin) ? "true " : "false";
    const char* mode = "????";
    const char* strength = "?? mA";
    const char* slew = "????";

    switch(gpio_get_slew_rate(pin)) {
        case GPIO_SLEW_RATE_SLOW: slew = "slow"; break;
        case GPIO_SLEW_RATE_FAST: slew = "fast"; break;
    }

    switch (gpio_get_drive_strength(pin)) {
        case GPIO_DRIVE_STRENGTH_2MA:  strength = " 2 mA"; break;
        case GPIO_DRIVE_STRENGTH_4MA:  strength = " 4 mA"; break;
        case GPIO_DRIVE_STRENGTH_8MA:  strength = " 8 mA"; break;
        case GPIO_DRIVE_STRENGTH_12MA: strength = "12 mA"; break;
    }

    switch(gpio_get_function(pin)){
        case GPIO_FUNC_XIP:  mode = "XIP "; break;
        case GPIO_FUNC_SPI:  mode = "SPI "; break;
        case GPIO_FUNC_UART: mode = "UART"; break;
        case GPIO_FUNC_I2C:  mode = "I2C "; break;
        case GPIO_FUNC_PWM:  mode = "PWM "; break; 
        case GPIO_FUNC_SIO:  mode = "SIO "; break;
        case GPIO_FUNC_PIO0: mode = "PIO0"; break;
        case GPIO_FUNC_PIO1: mode = "PIO1"; break;
        case GPIO_FUNC_GPCK: mode = "GPCK"; break; 
        case GPIO_FUNC_USB:  mode = "USB "; break;
        case GPIO_FUNC_NULL: mode = "NULL"; break;
    }

    printf("Pin: [%02d], Value: [%s], Mode: [%s], Pull: [%s], Dir: [%s], Strength: [%s], Slew: [%s]\n", pin, value, mode, pullUp, direction, strength, slew);
}

static void pin_overview () {
    for (uint8_t index = 0; index <= 22; index++) {
        show_pin_details(index);
    }
    for (uint8_t index = 25; index <= 28; index++) {
        show_pin_details(index);
    }
}

static void state_change (const uint8_t pinId, const uint32_t events) {
    printf ("Triggered pin [%d]\n", pinId);
}

static uint8_t check_mode(const char buffer[]) {
    uint8_t result = ~0;
    if (::toupper(buffer[0]) == 'O') {
        char next = ::toupper(buffer[1]);
        if (next == 'N') {
            result = 1;
        }
        else if ((next == 'F') && (::toupper(buffer[2]) == 'F')) {
            result = 0;
        }
    }
    return (result);
}

static uint8_t RunSlots = 5;
static uint8_t TimeSlot = 100;

static void LoopADC(const uint8_t id) {
    uint32_t key;
    uint8_t slots = 0;
  
    printf("\nPolling ADC%c every %d millisecond\n", (id == 16 ? 'X' : '0' + id), TimeSlot * RunSlots);
    adc_gpio_init(26 + id);
    adc_select_input(id);
    do {
        key = getchar_timeout_us(TimeSlot * 1000);
        if ( (key == PICO_ERROR_TIMEOUT) && (slots == RunSlots) )  {
            slots = 0;
            uint16_t value = adc_read();
            printf("Value: %d\n", value);
        }
        else {
            slots++;
        }
    } while (key == PICO_ERROR_TIMEOUT);
    printf ("Stopped reading ADC%d\n", id);
}

int main(){
    stdio_usb_init(); // init usb only. uart0 and uart1 are needed elsewhere

    while(!stdio_usb_connected()); // wait until USB connection

    printf("Firing up the Enhanced C world Controller!!\n>>");

    uint8_t index = 0;
    char buffer[64];

    PWMPin0.Enabled(false);
    PWMPin0.Frequency(25000);
    adc_init();

    // main loop
    while(1){
        int result = getchar_timeout_us(10);

        if (result != PICO_ERROR_TIMEOUT) {
            if (::isprint(result)) {
                if (index < sizeof(buffer)) {
                    printf("%c", result);
                    buffer[index++] = (char) result;
                }
            }
            else if(index == 0) {
                printf("\nThere is no data to process\n>>");
            }
            else if (result == 8) {
                printf("%c", result);
                index--;
            }
            else if (strncmp(buffer, "ADC0",4) == 0) {
                index = 0;
                LoopADC(0);
            }
            else if (strncmp(buffer, "ADC1", 4) == 0) {
                index = 0;
                LoopADC(1);
            }
            else if (strncmp(buffer, "ADC2", 4) == 0) {
                index = 0;
                LoopADC(2);
            }
            else if (strncmp(buffer, "Pins", 4) == 0) {
                index = 0;
                printf("\n");
                pin_overview();
            }
            else if (buffer[0] == '0') {
                uint8_t mode = check_mode(&buffer[1]);
                if (mode != 0xFF) {
                    printf("\nSet OUT0: %s", mode == 1 ? "On" : "Off");
                    out0.Set(mode == 1);
                }
                printf("\n");
                index = 0;
            }
            else if (buffer[0] == '1') {
                uint8_t mode = check_mode(&buffer[1]);
                if (mode != 0xFF) {
                    printf("\nSet OUT1: %s", mode == 1 ? "On" : "Off");
                    out1.Set(mode == 1);
                }
                printf("\n");
                index = 0;
            }
            else if (buffer[0] == '2') {
                uint8_t mode = check_mode(&buffer[1]);
                if (mode != 0xFF) {
                    printf("\nSet OUT2: %s", mode == 1 ? "On" : "Off");
                    out2.Set(mode == 1);
                }
                printf("\n");
                index = 0;
            }
            else if (buffer[0] == '3') {
                uint8_t mode = check_mode(&buffer[1]);
                if (mode != 0xFF) {
                    printf("\nSet OUT3: %s", mode == 1 ? "On" : "Off");
                    out3.Set(mode == 1);
                }
                printf("\n");
                index = 0;
            }
            else if (buffer[0] == 'i') {
                printf("\nIN0 = %s", in0.Get() ? "On" : "Off");
                printf("\nIN1 = %s", in1.Get() ? "On" : "Off");
                printf("\nIN2 = %s", in2.Get() ? "On" : "Off");
                printf("\nIN3 = %s", in3.Get() ? "On" : "Off");
                printf("\n");
                index = 0;
            }
            else if (buffer[0] == '?') {
                printf("\nOptions for the Sofware:");
                printf("\n{0<on|off>} Set OUT0 on or off");
                printf("\n{2<on|off>} Set OUT1 on or off");
                printf("\n{3<on|off>} Set OUT2 on or off");
                printf("\n{4<on|off>} Set OUT3 on or off");
                printf("\n{4<on|off>} Set OUT3 on or off");
                printf("\n{i} Show status of IN0,IN1,IN2,IN3");
                printf("\n[?] Show this help\n>>");
                index = 0;
            }
            else {
                buffer[index] = '\0';
                printf("\nUncomprehandable data: [%s]\n>>", buffer);
                index = 0;
            }
        }
    }
    return 0;
}
