#include <SPI.h>
#include <Audio.h>
#include <Wire.h>
#include <SD.h>

// Audio Components
AudioPlayQueue           queue;      // Buffer to hold incoming audio
AudioOutputI2S           i2s1;       // I2S output to SGTL5000
AudioControlSGTL5000     audioShield;
AudioConnection          patchCord1(queue, 0, i2s1, 0);  // Left channel
AudioConnection          patchCord2(queue, 0, i2s1, 1);  // Right channel

volatile byte audioBuffer[128]; // Buffer for incoming SPI data
volatile int bufferIndex = 0;

void setup() {
  Serial.begin(115200);
  AudioMemory(20);

  // Initialize SPI as SLAVE
  SPI.begin();
  pinMode(MISO, OUTPUT);
  pinMode(10, INPUT_PULLUP);  // Chip Select

  // Enable SPI interrupt
  SPI.usingInterrupt(digitalPinToInterrupt(10));
  SPCR |= _BV(SPE);  // Enable SPI

  // Initialize SGTL5000 Audio Shield
  audioShield.enable();
  audioShield.volume(0.5);  // Set output volume
}

ISR(SPI_STC_vect) {  // SPI receive interrupt
  if (bufferIndex < sizeof(audioBuffer)) {
    audioBuffer[bufferIndex++] = SPDR;  // Read received byte
  }
}

void loop() {
  if (bufferIndex >= 128) {  // If buffer is full
    int16_t audioData[64];

    for (int i = 0; i < 64; i++) {
      audioData[i] = ((int16_t)audioBuffer[i * 2] << 8) | audioBuffer[i * 2 + 1]; // Convert bytes to 16-bit PCM
    }

    queue.playBuffer((int16_t*)audioData); // Send to Teensy Audio Library
    bufferIndex = 0; // Reset buffer index
  }
}
