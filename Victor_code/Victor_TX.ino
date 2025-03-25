#include <SPI.h>
#include <RH_RF69.h>
#include <Audio.h>

#define RF69_FREQ 433.0  // Frequency in MHz
#define RFM69_CS  10     // Chip Select Pin
#define RFM69_INT 8      // Interrupt Pin
#define RFM69_RST 6      // Reset Pin

const int packetSize = 12;  // Number of bytes per audio packet

RH_RF69 rf69(RFM69_CS, RFM69_INT);

// Teensy Audio Objects
AudioInputAnalog         audioInput(A2);   // Microphone input
AudioRecordQueue         queue1;           // Audio recording queue
AudioConnection          patchCord1(audioInput, queue1);


// μ-Law Encoding Function
uint8_t muLawEncode(int16_t pcm) {
    const uint8_t MULAW_BIAS = 33;  // Corrected μ-Law bias
    int sign = (pcm < 0) ? 0x80 : 0;
    if (sign) pcm = -pcm;
    pcm += MULAW_BIAS;
    if (pcm > 32635) pcm = 32635;

    int exponent = 7, mantissa;
    for (int mask = 0x4000; (pcm & mask) == 0; mask >>= 1) exponent--;
    mantissa = (pcm >> ((exponent == 0) ? 4 : (exponent + 3))) & 0x0F;

    return ~(sign | (exponent << 4) | mantissa);
}

void setup() {
    
    Serial.begin(115200);
    while (!Serial);

    // Initialize RFM69
    
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

    // Enable Audio
    AudioMemory(60);
    queue1.begin();
}

void loop() {
    if (queue1.available() >= 3) {  // Ensure full buffer before transmitting
        uint8_t buffer[packetSize];

        int16_t *samples = queue1.readBuffer();
        if (samples) {
            for (int i = 0; i < packetSize; i++) {
                buffer[i] = muLawEncode(samples[i]);  // Encode each sample
            }
            queue1.freeBuffer();  // Free buffer after use
            
            // Send the encoded packet
            rf69.send(buffer, packetSize);
            rf69.waitPacketSent();

            Serial.print("Sent Packet: ");
            for (int i = 0; i < packetSize; i++) {
                Serial.print(buffer[i], HEX);
                Serial.print(" ");
            }
            Serial.println();
        }
    }
} 

