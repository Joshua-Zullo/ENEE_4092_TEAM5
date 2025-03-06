#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

// Audio Objects
AudioInputAnalog         mic(A2);       // Analog input from A2
AudioOutputI2S           i2s1;          // I2S output to Audio Shield
AudioControlSGTL5000     audioShield;   // Control object for Audio Shield
AudioConnection          patchCord1(mic, 0, i2s1, 0);  // Send to left channel
AudioConnection          patchCord2(mic, 0, i2s1, 1);  // Send to right channel

void setup() {
  AudioMemory(20);

  // Initialize Audio Shield
  audioShield.enable();
  audioShield.inputSelect(AUDIO_INPUT_MIC);  // Use microphone input
  audioShield.micGain(30);  // Adjust microphone gain (range: 0-63 dB)
  audioShield.volume(0.5);  // Set output volume
  
}

void loop() {
  //Libraries handle the fun stuff.
}

