// Include necessary libraries for communication and audio processing
#include <SPI.h>         // SPI library for RFM69 communication
#include <RH_RF69.h>     // RadioHead library for RFM69 module
#include <Audio.h>       // Teensy Audio library for handling audio

//Need to dycrypt the sending information

// Define RFM69 transceiver settings
#define RF69_FREQ 433.0  // Operating frequency of 433 MHz
#define RFM69_CS  10     // Chip Select pin
#define RFM69_INT 8      // Interrupt pin
#define RFM69_RST 6      // Reset pin

const int packetSize = 12;  // Size of audio data packets

// Create RFM69 radio object
RH_RF69 rf69(RFM69_CS, RFM69_INT);

// Teensy Audio Objects
AudioInputAnalog         audioInput(A17);  // Analog audio input from electret microphone
AudioRecordQueue         queue1;           // Queue for storing recorded audio samples
AudioFilterBiquad        biquad1;          // Biquad filter for noise reduction
AudioConnection          patchCord1(audioInput, biquad1); // Connect input to filter
AudioConnection          patchCord2(biquad1, queue1);     // Connect filter to queue

void setup() {
    Serial.begin(115200);  // Initialize serial communication
    while (!Serial);       // Wait for serial monitor to be ready

    // Reset RFM69 module
    pinMode(RFM69_RST, OUTPUT);
    digitalWrite(RFM69_RST, LOW);
    digitalWrite(RFM69_RST, HIGH);
    delay(10);
    digitalWrite(RFM69_RST, LOW);
    delay(10);

    // Initialize RFM69 module
    if (!rf69.init()) {
        Serial.println("RFM69 radio init failed");
        while (1);
    }
    
    rf69.setFrequency(RF69_FREQ); // Set radio frequency
    rf69.setTxPower(20, true);    // Set transmission power to max
    rf69.setModemConfig(RH_RF69::GFSK_Rb250Fd250); // Set modem configuration
    rf69.setSyncWords((uint8_t*)"SYNC", 4); // Set sync word for communication

    AudioMemory(20);                // Allocate memory for audio processing
    queue1.begin();                 // Start recording
    biquad1.setLowpass(0, 4000, 0.7); // Apply low-pass filter for noise reduction
}

// μ-Law Encoding Function for compression
uint8_t muLawEncode(int16_t sample) {
    int sign = (sample >> 8) & 0x80;
    if (sign) sample = -sample;
    if (sample > 32635) sample = 32635;
    
    int exponent = 7, mantissa = (sample >> 4) & 0x0F;
    for (int mask = 0x4000; (sample & mask) == 0 && exponent > 0; mask >>= 1, exponent--);
    
    uint8_t ulaw = ~(sign | (exponent << 4) | mantissa);
    return ulaw;
}

void loop() {
    // Read and print the microphone voltage level
    float readValue1 = analogRead(A17);
    float voltage1 = (readValue1 * (5.0 / 1023.0)) * 1000;
    Serial.println(voltage1);

    // Process audio data when available
    if (queue1.available()) {  
        uint8_t buffer[packetSize];  // Buffer to store processed audio data
        int16_t *samples = queue1.readBuffer(); // Get audio samples from queue

        if (samples) { 
            for (int i = 0; i < packetSize; i++) {
                buffer[i] = muLawEncode(samples[i]); // Encode samples using μ-Law
            }
            
            queue1.freeBuffer(); // Free memory after processing
            rf69.send(buffer, packetSize);  // Send data packet via RFM69
            rf69.waitPacketSent();          // Ensure packet is fully transmitted
            
            // Debugging: Print sent packet data
            Serial.print("Sent Packet: ");
            for (int i = 0; i < packetSize; i++) {
                Serial.print(buffer[i], HEX);
                Serial.print(" ");
            }
            Serial.println();
        }
    }
}
