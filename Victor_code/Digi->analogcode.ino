#include <SPI.h>
#include <Audio.h>
#include <Wire.h>
#include <SD.h>

// Audio Components
AudioPlayQueue           queue;      // Queue for audio buffering
AudioOutputI2S           i2s1;       // I2S output to SGTL5000
AudioControlSGTL5000     audioShield;
AudioConnection          patchCord1(queue, 0, i2s1, 0);  // Left channel
AudioConnection          patchCord2(queue, 0, i2s1, 1);  // Right channel

volatile byte audioBuffer[256];  // Double buffer for incoming SPI data
volatile int bufferIndex = 0;
volatile bool bufferReady = false;

void setup() {
  Serial.begin(115200);
  AudioMemory(20);

  // Initialize SPI as SLAVE
  SPI.begin();
  pinMode(MISO, OUTPUT);
  pinMode(10, INPUT_PULLUP);  // Chip Select

  // Enable SPI interrupt
  SPI.usingInterrupt(digitalPinToInterrupt(10));
  SPCR |= _BV(SPE);  // Enable SPI in Slave mode

  // Initialize SGTL5000 Audio Shield
  audioShield.enable();
  audioShield.volume(0.5);  // Set output volume
}

ISR(SPI_STC_vect) {  // SPI receive interrupt
  if (bufferIndex < sizeof(audioBuffer)) {
    audioBuffer[bufferIndex++] = SPDR;  // Read received byte
    if (bufferIndex >= 256) bufferReady = true;  // Mark buffer full
  }
}

void loop() {
  if (bufferReady) {  // If buffer is full
    int16_t audioData[128];  // 128 16-bit samples (256 bytes)

    // Convert SPI bytes to 16-bit PCM audio
    for (int i = 0; i < 128; i++) {
      audioData[i] = ((int16_t)audioBuffer[i * 2] << 8) | audioBuffer[i * 2 + 1];
    }

    // Send to the Audio Queue for playback
    if (queue.available()) {
      queue.playBuffer(audioData);
    }

    bufferIndex = 0;   // Reset buffer index
    bufferReady = false; // Mark buffer as free
  }
}
