#include <SPI.h>             // SPI library for communication with RFM69
#include <RH_RF69.h>         // RadioHead library for RFM69 radio module
#include <Audio.h>           // Teensy Audio Library
#include <math.h>            // Math library for logarithmic functions

// Define RFM69 transceiver settings
#define RF69_FREQ 433.0      // Transmission frequency in MHz
#define RF69_CS   10         // Chip Select pin for RFM69
#define RF69_INT  8          // Interrupt pin for RFM69
#define RF69_RST  6          // Reset pin for RFM69

const int packetSize = 16;   // Number of audio samples per packet

// default packet size 16, with 4 packets total in header and footer. Thus, our payload is only 12 bytes. 16 would only work if you changed the default value

const int ceil_16 = 32768;   // Maximum absolute value for 16-bit signed integers
const int mu = 255;          // μ-law encoding parameter (common standard value)

RH_RF69 rf69(RF69_CS, RF69_INT);  // Create RFM69 radio object

// Teensy Audio Objects
AudioInputAnalog audioInput(A17);  // Use analog pin A17 for microphone input
AudioRecordQueue queue1;           // Queue to record audio samples
AudioFilterBiquad biquad1;         // Biquad filter for low-pass filtering
AudioConnection patchCord1(audioInput, biquad1); // Connect mic input to filter
AudioConnection patchCord2(biquad1, queue1);     // Connect filter to record queue

void setup() {
    Serial.begin(115200);     // Start serial communication for debugging
    while (!Serial);          // Wait until Serial is ready

    // Reset RFM69 radio module
    pinMode(RF69_RST, OUTPUT);
    digitalWrite(RF69_RST, LOW);
    digitalWrite(RF69_RST, HIGH);
    delay(10);
    digitalWrite(RF69_RST, LOW);
    delay(10);

    // Initialize RFM69
    if (!rf69.init()) {
        Serial.println("RFM69 radio init failed");
        while (1); // Halt if radio init fails
    }

    rf69.setFrequency(RF69_FREQ);           // Set frequency to 433 MHz
    rf69.setTxPower(20, true);              // Set transmit power to max (20 dBm)
    rf69.setSyncWords((uint8_t*)"SYNC", 4); // Set sync word for reliable comm

    AudioMemory(100);        // Allocate memory for audio processing
    queue1.begin();          // Start recording audio into queue

    // Apply low-pass filter (cutoff at 3kHz, Q factor = 0.707)
    biquad1.setLowpass(0, 3000, 0.707); // First stage
    biquad1.setLowpass(1, 3000, 0.707); // Second stage (for sharper roll-off)
}

// μ-Law Encoding: Compresses 16-bit audio samples to 8-bit using logarithmic scale
int8_t muLawEncode(int16_t sample) {
    int sign = (sample < 0) ? -1 : 1; // Determine the sign
    sample = abs(sample);             // Get magnitude

    float scale = (float)sample / ceil_16;                          // Normalize
    float muLawVal = log(1 + mu * scale) / log(mu + 1);             // μ-law formula

    return (int8_t)(muLawVal * 127) * sign; // Scale to -127 to +127 range
}
//I don't see any sampling modification, probably still sampling at 44.1kHz.
void loop() {
    // If audio data is available in the queue
    if (queue1.available() > 0) {
        uint8_t buffer[packetSize];             // Data buffer to send over radio
        int16_t *samples = queue1.readBuffer(); // Get 16-bit audio samples

        if (samples) {
            // Encode and fill buffer with μ-law compressed 8-bit values
            for (int i = 0; i < packetSize; i++) {
                buffer[i] = muLawEncode(samples[i]);
            }
            queue1.freeBuffer(); // Release buffer back to system
        }

        // Transmit buffer via RFM69
        rf69.send(buffer, packetSize);
        rf69.waitPacketSent(); // Ensure packet was fully transmitted

        // Debug print the packet contents
        Serial.print("Sent Packet: ");
        for (int i = 0; i < packetSize; i++) {
            Serial.print(buffer[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
}
