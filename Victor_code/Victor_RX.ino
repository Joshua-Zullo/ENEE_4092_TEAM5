#include <SPI.h>             // SPI communication library (required for RFM69)
#include <RH_RF69.h>         // RadioHead library for RFM69 transceiver
#include <Audio.h>           // Teensy Audio Library

// Define RFM69 transceiver settings
#define RF69_FREQ 433.0      // Operating frequency (MHz)
#define RF69_CS   36         // Chip Select pin for RFM69
#define RF69_INT  31         // Interrupt pin for RFM69
#define RF69_RST  32         // Reset pin for RFM69

const int packetSize = 16;   // Number of audio samples per packet
const int ceil_16 = 32768;   // Max absolute value of 16-bit signed audio
const int mu = 255;          // μ-law encoding factor

RH_RF69 rf69(RF69_CS, RF69_INT);  // Create RFM69 radio object

// Teensy Audio System Objects
AudioControlSGTL5000 audioShield;   // Controls the SGTL5000 Audio Shield
AudioPlayQueue queue1;              // Audio output queue for raw PCM data
AudioOutputI2S i2s1;                // Audio output to I2S (to DAC/3.5mm jack)
AudioFilterBiquad lowpass;          // Low-pass filter for smoothing audio
AudioConnection patchCord1(queue1, lowpass); // Connect queue to filter
AudioConnection patchCord2(lowpass, i2s1);   // Connect filter to output

void setup() {
    Serial.begin(115200);     // Start serial communication
    while (!Serial);          // Wait for serial port to initialize

    // Initialize audio shield
    audioShield.enable();            // Turn on the SGTL5000
    audioShield.volume(1.0);         // Set max volume
    audioShield.unmuteHeadphone();   // Ensure headphone output is active
    audioShield.unmuteLineout();     // Ensure line-out is active
    audioShield.lineOutLevel(13);    // Set line-out level

    // Reset RFM69 radio
    pinMode(RF69_RST, OUTPUT);
    digitalWrite(RF69_RST, LOW);
    digitalWrite(RF69_RST, HIGH);
    delay(10);
    digitalWrite(RF69_RST, LOW);
    delay(10);

    // Set up 2-stage low-pass filter (cutoff 3kHz)
    lowpass.setLowpass(0, 3000, 0.707);  // Stage 1
    lowpass.setLowpass(1, 3000, 0.707);  // Stage 2 (sharper rolloff)

    // Initialize RFM69
    if (!rf69.init()) {
        Serial.println("RFM69 radio init failed");
        while (1);  // Halt if initialization fails
    }

    rf69.setFrequency(RF69_FREQ);           // Set operating frequency
    rf69.setTxPower(20, true);              // Set maximum transmit power
    rf69.setSyncWords((uint8_t*)"SYNC", 4); // Define sync word for reliable pairing
    rf69.setModeRx();                       // Set module to receive mode

    AudioMemory(100);  // Allocate memory blocks for audio processing
}

// μ-Law Decoding: Convert 8-bit compressed value back to 16-bit linear PCM
int16_t muLawDecode(int8_t muVal) {
    int sign = (muVal < 0) ? -1 : 1;          // Extract sign
    muVal = abs(muVal);                       // Get magnitude

    float scaleMu = (float)muVal / 127.0f;    // Normalize to 0.0 - 1.0
    float imuVal = (pow(mu + 1, scaleMu) - 1) / mu; // μ-law inverse formula

    return (int16_t)(sign * imuVal * ceil_16); // Return 16-bit signed sample
}

void loop() {
    // Check if a new packet has arrived from RFM69
    if (rf69.available()) {
        uint8_t buffer[packetSize];           // Temporary buffer for audio packet
        uint8_t receivedSize = packetSize;    // Expected size of the packet

        if (rf69.recv(buffer, &receivedSize)) {
            // Print received packet for debugging
            Serial.print("Received Packet: ");
            for (int i = 0; i < receivedSize; i++) {
                Serial.print(buffer[i], HEX);
                Serial.print(" ");
            }
            Serial.println();

            // Check if we can get a buffer to play the audio
            if (queue1.available() >= 1) {
                int16_t *playBuffer = queue1.getBuffer(); // Get audio buffer to fill
                if (!playBuffer) return;

                // Decode each μ-law byte into 16-bit audio
                for (int i = 0; i < receivedSize; i++) {
                    playBuffer[i] = muLawDecode(buffer[i]);
                }

                queue1.playBuffer(); // Play the decoded buffer through DAC
            }
        }
    }
}
