# GameBoyNano
A GameBoy Audio Emulator for Arduino
THIS IS A WIP, it is unfinished, messy code, and in-progress of being optimized.
It currently loads a song from crystal.gbc on the SD card, and plays it. The song playing code is not finished, every song will try to load 4 channels, so songs without certain channels might hit arbitrary data. The noise channel is not proper sounding yet, but close enough for example's sake.

crystal.gbc is a rom of Pokemon Crystal, US

SD reader hookup:
CS>4
CLK>13
MISO>12
MOSI>11

pin 2 is supposed to switch songs when grounded, but that may not work right.

It will aim to simulate the GameBoy's Audio system, so that chiptune players can be made.
Compiles in Arduino IDE 1.8.0 Works on the Arduino Nano
Pin #3 gives the audio output with PWM.
