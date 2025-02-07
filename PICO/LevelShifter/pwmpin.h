#pragma once

#include <pico/stdlib.h>
#include "hardware/pwm.h"
#include "defs.h"

namespace NextGen {

class IOPin {
private:
	enum state : uint8_t {
		ACTIVE_LOW = 0x01
	};

	static void GPIOCallback (uint gpio, uint32_t events) {
		IOPin* current = _root;
		uint8_t pinId = (uint8_t) gpio;

		while ( (current != nullptr) && (current->Id() != pinId) ) {
			current = current->_next;
		}

		if ( (current != nullptr) && (current->_callback != nullptr) ) {
			current->_callback(pinId, events);
		}
	}

public:
	using Callback = void (*) (const uint8_t pinId, const uint32_t events);

public:
	IOPin() = delete;
	IOPin(IOPin&&) = delete;
	IOPin(const IOPin&) = delete;
	IOPin& operator= (IOPin&&) = delete;
	IOPin& operator= (const IOPin&) = delete;

	IOPin(const uint16_t id, const bool activeLow = false) 
		: _state(activeLow ? ACTIVE_LOW : 0)
		, _pin(id)
		, _next(nullptr)
		, _callback(nullptr) {
		Add();
		gpio_init(_pin);
            gpio_set_function(_pin, GPIO_FUNC_SIO);
	    gpio_set_dir(_pin, GPIO_OUT);
            gpio_set_pulls(_pin, false, false);
	}
	IOPin(const uint16_t id, Callback callback, enum trigger_mode mode, const bool activeLow = false) 
		:_state(activeLow ? ACTIVE_LOW : 0)
		, _pin(id)
		, _next(nullptr)
		, _callback(callback) {
		Add();

		gpio_init(_pin);
                gpio_set_function(_pin, GPIO_FUNC_SIO);
	        gpio_set_dir(_pin, GPIO_IN);
                gpio_set_pulls(_pin, false, false);

		if (_callback != nullptr) {
                        int setting = (mode == trigger_mode::BOTH || mode == trigger_mode::FALLING ? GPIO_IRQ_EDGE_FALL : 0) | 
                                      (mode == trigger_mode::BOTH || mode == trigger_mode::RISING  ? GPIO_IRQ_EDGE_RISE : 0) ;
			gpio_set_irq_enabled_with_callback(_pin, setting, true, &GPIOCallback);
		}
	}
	~IOPin() {
		Remove();
	}

public:
	uint8_t Id() const {
		return (_pin);
	}
	bool Get() {
		bool result = gpio_get(_pin);
		return ((_state & ACTIVE_LOW) != 0 ? !result : result);
	}
	void Set(const bool enabled) {
		if ((enabled == true) ^ ((_state & ACTIVE_LOW) != 0)) {
			gpio_put(_pin, 1);
		}
		else {
			gpio_put(_pin, 0);
		}
	}

private:
	void Add() {
		IOPin* current = _root;

		while ( (current != nullptr) && (current->_next != nullptr) ) {
			current = current->_next;
		}

		if (current == nullptr) {
			_root = this;
		}
		else {
			current->_next = this;
		}
	}
	void Remove() {
		// See if we are the root :-)
		if (_root == this) {
			_root = _next;
		}
		else {
			IOPin* current = _root;

			while ( (current != nullptr) && (current->_next != this) ) {
				current = current->_next;
			}

			if (current != nullptr) {
				current->_next = _next;
			}
		}
	}

private:
	uint8_t _state; 
	uint8_t _pin;
	IOPin* _next;
	Callback _callback;
	static IOPin* _root;
};

class PWMPin {
public:
	static constexpr uint32_t DefaultFrequency = 10000;
	static constexpr uint8_t DefaultDutyCycle = 50;

public:
	PWMPin() = delete;
	PWMPin(PWMPin&&) = delete;
	PWMPin(const PWMPin&) = delete;
	PWMPin& operator= (PWMPin&&) = delete;
	PWMPin& operator= (const PWMPin&) = delete;

	PWMPin(const uint16_t id)
		: _pin(id)
		, _slice(0)
		, _channel(0)
		, _frequency(DefaultFrequency)
		, _dutyCycle(DefaultDutyCycle) {
		gpio_init(_pin);
		gpio_set_dir(_pin, GPIO_OUT);
		gpio_set_function(_pin, GPIO_FUNC_PWM);
		Release();
	}
	PWMPin(const uint16_t id, const uint32_t frequency, const uint8_t dutyCycle)
		: PWMPin(id) {
		_dutyCycle = 0x80;
		gpio_init(_pin);
		gpio_set_dir(_pin, GPIO_OUT);
		gpio_set_function(_pin, GPIO_FUNC_PWM);
		pwm_set_freq_duty(frequency, dutyCycle);
	}
	~PWMPin() {
		pwm_set_enabled(_slice, false);
	}

public:
	void Stuck(const bool high) {
		gpio_init(_pin);
		gpio_set_dir(_pin, GPIO_OUT);

		if (high == true) {
			gpio_put(_pin, 1);
		}
		else {
			gpio_put(_pin, 0);
		}
	}
	void Release() {
		gpio_init(_pin);
		gpio_set_dir(_pin, GPIO_OUT);
		gpio_set_function(_pin, GPIO_FUNC_PWM);

		// Configure the PWM for the BOOSTER clock pulses
		_slice = pwm_gpio_to_slice_num (_pin); 
		_channel = pwm_gpio_to_channel (_pin);

		pwm_set_freq_duty(_frequency, _dutyCycle);
	}
	bool Enabled() const {
		return ((_dutyCycle & 0x80) != 0);
	} 
	void Enabled(const bool enabled) {
		if (enabled == true) {
			pwm_set_enabled(_slice, true);
			_dutyCycle |= 0x80;
		}
		else {
			pwm_set_enabled(_slice, false);
			_dutyCycle &= (~0x80);
		}
	} 
	uint8_t DutyCycle() const {
		return (_dutyCycle & 0x7F);
	}
	void DutyCycle(uint8_t dutyCycle) {
		pwm_set_freq_duty(Frequency(), dutyCycle);
	}
	uint32_t Frequency() const {
		return (_frequency);
	}
	void Frequency(uint32_t frequency) {
		pwm_set_freq_duty(frequency, DutyCycle());
	}

private:
	void pwm_set_freq_duty(const uint32_t frequency, const uint8_t duty)
	{
		uint32_t clock = 125000000;
		uint32_t divider16 = clock / frequency / 4096 + (clock % (frequency * 4096) != 0);

		if ((divider16 / 16) == 0) 
		{ 
			divider16 = 16; 
		}

		uint32_t wrap = clock * 16 / divider16 / frequency - 1;

		pwm_set_enabled(_slice, false);
		pwm_set_clkdiv_int_frac(_slice, divider16/16, divider16 & 0xF);
		pwm_set_wrap(_slice, wrap);
		pwm_set_chan_level(_slice, _channel, wrap * duty / 100);

		if (Enabled() == true) {
			pwm_set_enabled(_slice, true);
			_dutyCycle = duty | 0x80;
		}
		else {
			_dutyCycle = duty;
		}

		_frequency = frequency;
	}

private:
	uint8_t _pin;
	uint _slice;
	uint _channel;
	uint _frequency;
	uint8_t _dutyCycle;
};

}
