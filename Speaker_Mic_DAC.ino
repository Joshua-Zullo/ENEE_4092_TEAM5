#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>

// Create audio objects
AudioInputAnalog       micInput(A0);   // Input from the electret microphone (connected to A0)
AudioOutputI2S         i2sOutput;      // Output to I2S DAC for speaker

//does not include anything about speaker.. is speaker hooked up to I2S DAC?
AudioConnection        patchCord1(micInput, i2sOutput); // Connecting mic input to DAC output

void setup() {
  // Start serial monitor for debugging
  Serial.begin(9600);

  // Initialize the audio library
  AudioMemory(12);  // Allocate audio memory (adjust as necessary)
  
  // Initialize other hardware peripherals if needed
}

void loop() {
  // Main loop does not need to do anything in this case, as the audio library
  // will continuously process the audio input and output.
}

