#include <SPI.h>
#include <RH_RF69.h>
#include <Audio.h>
#include <math.h>

// Define RFM69 transceiver settings
#define RF69_FREQ 433.0  
#define RFM69_CS  10    
#define RF69_INT  8     
#define RF69_RST  6     

const int packetSize = 16; 
const int ceil_16 = 32768;
const int mu = 255;

RH_RF69 rf69(RFM69_CS, RF69_INT);

// Teensy Audio Objects
AudioInputAnalog audioInput(A17);  
AudioRecordQueue queue1;          
AudioFilterBiquad biquad1;         
AudioConnection patchCord1(audioInput, biquad1);
AudioConnection patchCord2(biquad1, queue1);

void setup() {
    Serial.begin(115200);
    while (!Serial);

    pinMode(RF69_RST, OUTPUT);
    digitalWrite(RF69_RST, LOW);
    digitalWrite(RF69_RST, HIGH);
    delay(10);
    digitalWrite(RF69_RST, LOW);
    delay(10);

    if (!rf69.init()) {
        Serial.println("RFM69 radio init failed");
        while (1);
    }

    rf69.setFrequency(RF69_FREQ);
    rf69.setTxPower(20, true);
    rf69.setSyncWords((uint8_t*)"SYNC", 4);

    AudioMemory(100);
    queue1.begin();

    // Improved low-pass filtering
    biquad1.setLowpass(0, 3000, 0.707);
    biquad1.setLowpass(1, 3000, 0.707);
}

// μ-Law Encoding (Logarithmic)
int8_t muLawEncode(int16_t sample) {
    int sign = (sample < 0) ? -1 : 1;
    sample = abs(sample);
    
    float scale = (float)sample / ceil_16;
    float muLawVal = log(1 + mu * scale) / log(mu + 1);

    return (int8_t)(muLawVal * 127) * sign;
}

void loop() {
    if (queue1.available() > 0) {
        uint8_t buffer[packetSize];
        int16_t *samples = queue1.readBuffer();

        if (samples) {
            for (int i = 0; i < packetSize; i++) {
                buffer[i] = muLawEncode(samples[i]);
            }
            queue1.freeBuffer();
        }

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
