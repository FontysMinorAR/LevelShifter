add_executable(LevelShifter
        main.cpp
        pwmpin.cpp
        )

# pull in common dependencies
target_link_libraries(LevelShifter pico_stdlib hardware_pwm hardware_adc)

# create map/bin/hex file etc.
pico_add_extra_outputs(LevelShifter)
pico_enable_stdio_usb(LevelShifter 1)  # enable stdio over USB
