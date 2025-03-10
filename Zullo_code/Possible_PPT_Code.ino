#include <SPI.h>
#include <Audio.h>
#include <Wire.h>
#include <SD.h>

// Audio Components
AudioInputI2S           i2sInput;   // Audio input (MIC/Line-in)
AudioOutputI2S          i2sOutput;  // Audio output (SGTL5000 DAC)
AudioPlayQueue          queue;      // Buffer for outgoing SPI audio
AudioRecordQueue        recorder;   // Capture audio for SPI transmission
AudioControlSGTL5000    audioShield;
AudioConnection         patchCord1(i2sInput, 0, recorder, 0); 
AudioConnection         patchCord2(queue, 0, i2sOutput, 0);
AudioConnection         patchCord3(queue, 0, i2sOutput, 1);

#define PTT_BUTTON 9  // Push-to-Talk button pin
bool isTransmitting = false;

void setup() {
  Serial.begin(115200);
  AudioMemory(20);

  pinMode(PTT_BUTTON, INPUT_PULLUP); // PTT button setup

  // SPI Setup
  SPI.begin();
  pinMode(10, OUTPUT);  // CS (Chip Select) for SPI
  
  // Setup SGTL5000
  audioShield.enable();
  audioShield.inputSelect(AUDIO_INPUT_LINEIN);  // Use microphone or line-in
  audioShield.volume(0.5);

  recorder.begin();
}

void loop() {
  isTransmitting = (digitalRead(PTT_BUTTON) == LOW); // Check PTT button state
  
  if (isTransmitting) {
    sendAudioSPI();
  } else {
    receiveAudioSPI();
  }
}

void sendAudioSPI() {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(10, LOW);  // Select slave Teensy

  if (recorder.available() > 0) {
    int16_t *audioData = recorder.readBuffer();
    
    for (int i = 0; i < 128; i++) {  // Send 128 samples (adjustable)
      SPI.transfer16(audioData[i]);
    }
    
    recorder.freeBuffer();
  }

  digitalWrite(10, HIGH); // Deselect slave
  SPI.endTransaction();
}

void receiveAudioSPI() {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(10, LOW);  // Select slave Teensy

  int16_t receivedAudio[128];

  for (int i = 0; i < 128; i++) {
    receivedAudio[i] = SPI.transfer16(0x0000);  // Receive 16-bit PCM audio
  }

  queue.playBuffer(receivedAudio);  // Play received audio

  digitalWrite(10, HIGH);
  SPI.endTransaction();
}
