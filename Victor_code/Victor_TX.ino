#include <SPI.h>         // SPI library for communication with the RFM69 transceiver
#include <RH_RF69.h>     // RadioHead library to interface with the RFM69 module
#include <Audio.h>       // Teensy Audio library for handling audio input and processing

// Define RFM69 radio settings
#define RF69_FREQ 433.0  // Radio frequency set to 433 MHz (can be changed based on region)
#define RFM69_CS  10     // Chip Select (CS) pin for RFM69 module
#define RFM69_INT 8      // Interrupt pin for RFM69 module
#define RFM69_RST 6      // Reset pin for RFM69 module

const int packetSize = 12;  // Define the size of the data packet to be transmitted

// Create an RFM69 radio object using the defined chip select and interrupt pins
RH_RF69 rf69(RFM69_CS, RFM69_INT);

// Teensy Audio Objects
AudioInputAnalog         audioInput(A17);  // Define the analog audio input on pin A17
AudioRecordQueue         queue1;           // Queue to store recorded audio samples
AudioFilterBiquad        biquad1;          // Biquad filter for audio signal processing
AudioConnection          patchCord1(audioInput, biquad1); // Connect audio input to biquad filter
AudioConnection          patchCord2(biquad1, queue1);     // Connect biquad filter to audio queue

void setup() {
    Serial.begin(115200);  // Start serial communication at 115200 baud
    while (!Serial);       // Wait for the serial monitor to be ready

    // Reset the RFM69 module
    pinMode(RFM69_RST, OUTPUT);   // Set the reset pin as an OUTPUT
    digitalWrite(RFM69_RST, LOW); // Set reset pin LOW
    digitalWrite(RFM69_RST, HIGH); // Set reset pin HIGH to trigger reset
    delay(10);                     // Wait 10 ms
    digitalWrite(RFM69_RST, LOW);  // Set reset pin LOW again to complete reset
    delay(10);                     // Wait another 10 ms

    // Initialize the RFM69 radio module
    if (!rf69.init()) {
        Serial.println("RFM69 radio init failed"); // Print an error message if initialization fails
        while (1); // Halt execution if the radio module is not detected
    }
    
    rf69.setFrequency(RF69_FREQ); // Set the RFM69 frequency to 433 MHz
    rf69.setTxPower(20, true);    // Set the transmission power to 20 dBm (max power)
    rf69.setModemConfig(RH_RF69::GFSK_Rb250Fd250); // Configure the radio modulation settings
    rf69.setSyncWords((uint8_t*)"SYNC", 4); // Set the synchronization word to "SYNC" (4 bytes)

    AudioMemory(20);                // Allocate memory for audio processing (20 blocks)
    queue1.begin();                 // Start recording audio into the queue
    biquad1.setLowpass(0, 5000, 0.7); // Configure the biquad filter as a low-pass filter (5 kHz cutoff)
}

// μ-Law Encoding Function
uint8_t muLawEncode(int16_t sample) {
    int sign = (sample >> 8) & 0x80; // Extract the sign bit (positive/negative indicator)
    if (sign) sample = -sample;      // Convert negative values to positive
    if (sample > 32635) sample = 32635; // Limit the sample to prevent overflow
    
    // Find the exponent and mantissa for μ-Law encoding
    int exponent = 7, mantissa = (sample >> 4) & 0x0F;
    for (int mask = 0x4000; (sample & mask) == 0 && exponent > 0; mask >>= 1, exponent--);
    
    // Combine the sign, exponent, and mantissa to create the μ-Law encoded byte
    uint8_t ulaw = ~(sign | (exponent << 4) | mantissa);
    return ulaw; // Return the encoded byte
}

void loop() {
    // Check if there is recorded audio available in the queue

     float readValue1 = analogRead(A17);

  float voltage1 = (readValue1 * (5.0 / 1023.0))* 1000; // Convert ADC value to mV


  // Print voltage1 for Serial Plotter
  Serial.println(voltage1);

    if (queue1.available()) {  
        uint8_t buffer[packetSize];  // Create a buffer to store encoded audio samples

        int16_t *samples = queue1.readBuffer(); // Read raw audio samples from the queue
        if (samples) { // If samples are available
            for (int i = 0; i < packetSize; i++) { // Process each sample
                buffer[i] = muLawEncode(samples[i]); // Encode the sample using μ-Law compression
            }

            queue1.freeBuffer(); // Free the buffer in the audio queue after processing

            rf69.send(buffer, packetSize);  // Transmit the encoded data packet via RFM69
            rf69.waitPacketSent();          // Wait until the packet is fully transmitted

            // Debugging: Print the sent packet in hexadecimal format
            Serial.print("Sent Packet: ");
            for (int i = 0; i < packetSize; i++) {
                Serial.print(buffer[i], HEX); // Print each byte as a hexadecimal number
                Serial.print(" "); // Add a space between bytes
            }
            Serial.println(); // Move to the next line after printing the packet

            float readValue1 = analogRead(A17);

  float voltage1 = (readValue1 * (5.0 / 1023.0))* 1000; // Convert ADC value to mV


  // Print voltage1 for Serial Plotter
  Serial.println(voltage1);
        }
    }
}

