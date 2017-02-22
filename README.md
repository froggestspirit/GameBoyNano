# GameBoyNano
A GameBoy Audio Emulator for Arduino
THIS IS A WIP, it is unfinished, messy code, and un-optimized.
It currently loads a song from crystal.gbc on the SD card, and plays it through channels 1-3. The song playing code is not finished.

crystal.gbc is a rom of Pokemon Crystal, US

SD reader hookup:
CS>4
CLK>13
MISO>12
MOSI>11

It will aim to simulate the GameBoy's Audio system, so that chiptune players can be made.
Compiles in Arduino IDE 1.8.0 Works on the Arduino Nano
Pin #3 gives the audio output with PWM.
