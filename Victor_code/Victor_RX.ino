#include <SPI.h>
#include <RH_RF69.h>
#include <Audio.h>

#define RF69_FREQ 433.0  // Frequency in MHz
#define RFM69_CS  36     // Chip Select Pin
#define RFM69_INT 31     // Interrupt Pin
#define RFM69_RST 32     // Reset Pin

const int packetSize = 12;

RH_RF69 rf69(RFM69_CS, RFM69_INT);

// Teensy Audio Objects
AudioFilterBiquad biquad1;



AudioControlSGTL5000 audioShield;
AudioPlayQueue queue1;            // Audio play queue
AudioOutputI2S i2s1;             // SGTL5000 audio output (I2S interface)
AudioConnection patchCord1(queue1, i2s1);  // Connect the play queue to the I2S output

AudioConnection patchCord2(queue1, biquad1);
AudioConnection patchCord3(biquad1, i2s1);

// μ-Law Decoding Function
int16_t muLawDecode(uint8_t ulaw) {
    const int MULAW_BIAS = 33;
    ulaw = ~ulaw;
    int sign = (ulaw & 0x80) ? -1 : 1;
    int exponent = (ulaw >> 4) & 0x07;
    int mantissa = ulaw & 0x0F;
    int sample = (((mantissa << 3) + 0x84) << exponent) - MULAW_BIAS;
    
    return sign * (sample / 2);  // Scale down to avoid overloading I2S
}

void setup() {
    Serial.begin(115200);
    while (!Serial);

   // Initialize Audio Shield
    audioShield.enable();
    audioShield.volume(0.6);

    // Configure Low-Pass Filter
    biquad1.setLowpass(0, 3500, 0.7);  // Cut frequencies above 3.5 kHz

    

    pinMode(RFM69_RST, OUTPUT);
    digitalWrite(RFM69_RST, LOW);
    digitalWrite(RFM69_RST, HIGH);
    delay(10);
    digitalWrite(RFM69_RST, LOW);
    delay(10);

    if (!rf69.init()) {
        Serial.println("RFM69 radio init failed");
        while (1);
    }
    
    rf69.setFrequency(RF69_FREQ);
    rf69.setTxPower(14, true);
    rf69.setModeRx();

    // Enable Audio
    AudioMemory(200);
    audioShield.enable();
    audioShield.inputSelect(AUDIO_INPUT_LINEIN);
    audioShield.volume(.5); // Reduce volume to prevent clipping
    
    // Make sure the audio is outputting correctly
    audioShield.unmuteHeadphone();
    audioShield.unmuteLineout();
    audioShield.lineOutLevel(29);  // Set a reasonable volume level
}

    
void loop() {
    uint8_t buffer[packetSize];
    uint8_t receivedSize = sizeof(buffer);

    if (rf69.available()) {
        if (rf69.recv(buffer, &receivedSize)) {
            int16_t *playBuffer = queue1.getBuffer();  
            if (playBuffer) {
                for (int i = 0; i < receivedSize; i++) {
                    playBuffer[i] = muLawDecode(buffer[i]);  // Decode received data
                }
                queue1.playBuffer();  // Send to I2S output
                
                Serial.print("Received Packet: ");
                for (int i = 0; i < receivedSize; i++) {
                    Serial.print(buffer[i], HEX);
                    Serial.print(" ");
                }
                Serial.println();
            }
        }
    }
}
