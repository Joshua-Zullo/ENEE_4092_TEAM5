#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

// Audio Objects
AudioInputI2S           i2sIn;       // Digital I2S input
AudioOutputI2S          i2sOut;      // I2S output to Audio Shield
AudioControlSGTL5000    audioShield; // Control object for Audio Shield
AudioConnection         patchCord1(i2sIn, 0, i2sOut, 0);  // Left channel passthrough
AudioConnection         patchCord2(i2sIn, 1, i2sOut, 1);  // Right channel passthrough

void setup() {
  AudioMemory(20);

  // Initialize Audio Shield
  audioShield.enable();
  audioShield.inputSelect(AUDIO_INPUT_LINEIN);  // Use digital line-in (I2S)
  audioShield.volume(0.5);  // Set output volume
  
}

void loop() {
  // Audio processing runs in the background automatically
}



