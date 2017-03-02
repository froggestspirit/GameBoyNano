# GameBoyNano
A GameBoy Audio Emulator for Arduino
THIS IS A WIP, it is unfinished, messy code, and in-progress of being optimized.
It currently loads and plays songs from crystal.gbc on the SD card. The song playing code is not finished. The noise channel is not proper sounding yet, but close enough for example's sake.

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

Pardon the wonky tabbing. Arduino ide and notepad++ didn't play well together...
