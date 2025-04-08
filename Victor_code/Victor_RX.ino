#include <SPI.h>
#include <RH_RF69.h>
#include <Audio.h>

#define RF69_FREQ 433.0  
#define RF69_CS  36    
#define RF69_INT 31    
#define RF69_RST 32    

const int packetSize = 16;
const int ceil_16 = 32768;  
const int mu = 255;  

RH_RF69 rf69(RF69_CS, RF69_INT);

// Teensy Audio Objects
AudioControlSGTL5000 audioShield;  
AudioPlayQueue queue1;             
AudioOutputI2S i2s1;               
AudioFilterBiquad lowpass;         
AudioConnection patchCord1(queue1, lowpass);
AudioConnection patchCord2(lowpass, i2s1);

void setup() {
    Serial.begin(115200);
    while (!Serial);

    audioShield.enable();
    audioShield.volume(1.0);
    audioShield.unmuteHeadphone();
    audioShield.unmuteLineout();
    audioShield.lineOutLevel(13);

    pinMode(RF69_RST, OUTPUT);
    digitalWrite(RF69_RST, LOW);
    digitalWrite(RF69_RST, HIGH);
    delay(10);
    digitalWrite(RF69_RST, LOW);
    delay(10);

    // Improved filtering
    lowpass.setLowpass(0, 3000, 0.707);
    lowpass.setLowpass(1, 3000, 0.707);

    if (!rf69.init()) {
        Serial.println("RFM69 radio init failed");
        while (1);
    }

    rf69.setFrequency(RF69_FREQ);
    rf69.setTxPower(20, true);
    rf69.setSyncWords((uint8_t*)"SYNC", 4);
    rf69.setModeRx();

    AudioMemory(100);
}

// μ-Law Decoding
int16_t muLawDecode(int8_t muVal) {
    int sign = (muVal < 0) ? -1 : 1;
    muVal = abs(muVal);

    float scaleMu = (float)muVal / 127.0f;
    float imuVal = (pow(mu + 1, scaleMu) - 1) / mu;

    return (int16_t)(sign * imuVal * ceil_16);
}

void loop() {
    if (rf69.available()) {
        uint8_t buffer[packetSize];
        uint8_t receivedSize = packetSize;

        if (rf69.recv(buffer, &receivedSize)) {
            Serial.print("Received Packet: ");
            for (int i = 0; i < receivedSize; i++) {
                Serial.print(buffer[i], HEX);
                Serial.print(" ");
            }
            Serial.println();

            if (queue1.available() >= 1) {  
                int16_t *playBuffer = queue1.getBuffer();
                if (!playBuffer) return;

                for (int i = 0; i < receivedSize; i++) {
                    playBuffer[i] = muLawDecode(buffer[i]);
                }

                queue1.playBuffer();
            }
        }
    }
}
