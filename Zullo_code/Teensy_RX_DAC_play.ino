#include <SPI.h>
#include <RH_RF69.h>
#include <Audio.h>

/************ Radio Setup ***************/
#define RF69_FREQ 433.0

#define RFM69_RST  8
#define RFM69_CS   7
#define RFM69_INT  digitalPinToInterrupt(6)  // G0, hardware interrupt

const int bufferSize = 120;
const int packetSize = 12;

volatile int8_t audioBuffer[bufferSize];
volatile int8_t bufferHead = 0;
volatile int8_t bufferTail = 0;
volatile int8_t bufferCount = 0;

const int mu = 255;
const int ceil_16 = 32768;

// Teensy Audio Library Objects
AudioPlayQueue queue1;
AudioOutputI2S i2s1;
AudioConnection patchCord1(queue1, 0, i2s1, 0);
AudioControlSGTL5000 audioShield;

RH_RF69 rf69(RFM69_CS, RFM69_INT);

// Undo μ-law scaling to 16-bit PCM
int16_t imuLaw(int8_t muVal) {
    int sign = (muVal < 0) ? -1 : 1;
    muVal = abs(muVal);
    float scaleMu = float(muVal) / 127.0f;
    float imuVal = (pow(256, scaleMu) - 1) / mu;
    return int16_t(sign * imuVal * ceil_16);
}

void playAudio() {
    if (bufferCount > 0) {
        int8_t compSamp = audioBuffer[bufferTail];
        bufferTail = (bufferTail + 1) % bufferSize;
        bufferCount--;

        int16_t decSamp = imuLaw(compSamp);

        // Send sample to Teensy's audio queue
        int16_t *block = queue1.getBuffer();
        if (block) {
            for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
                block[i] = decSamp;
            }
            queue1.playBuffer();
        }
    }
}

void storePacket(uint8_t *data, int len) {
    for (int i = 0; i < len; i++) {
        if (bufferCount < bufferSize) {
            audioBuffer[bufferHead] = data[i];
            bufferHead = (bufferHead + 1) % bufferSize;
            bufferCount++;
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(3000);
    while (!Serial) { delay(1); }

    pinMode(RFM69_RST, OUTPUT);
    digitalWrite(RFM69_RST, LOW);

    Serial.println("Teensy RFM69 RX Test!");
    Serial.println();

    digitalWrite(RFM69_RST, HIGH);
    delay(10);
    digitalWrite(RFM69_RST, LOW);
    delay(10);

    if (!rf69.init()) {
        Serial.println("RFM69 radio init failed");
        while (1);
    }
    Serial.println("RFM69 radio init OK!");

    if (!rf69.setFrequency(RF69_FREQ)) {
        Serial.println("setFrequency failed");
    }

    rf69.setTxPower(14, true);

    uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    rf69.setEncryptionKey(key);

    Serial.print("RFM69 radio @"); Serial.print((int)RF69_FREQ); Serial.println(" MHz");

    // Initialize Teensy Audio Shield
    AudioMemory(10);
    audioShield.enable();
    audioShield.volume(0.8);
}

void loop() {
    if (rf69.available()) {
        uint8_t buf[packetSize];
        uint8_t len = sizeof(buf);
        if (rf69.recv(buf, &len)) {
            if (len == packetSize) {
                storePacket(buf, len);
            }
        }
    }
    
    playAudio(); // Play audio samples
}
