#include <SPI.h>       // Include SPI library for communication with the RFM69 module
#include <RH_RF69.h>   // Include RadioHead library for controlling the RFM69 transceiver
#include <Audio.h>     // Include Teensy Audio library for handling audio processing

// Change the voltage to the DAC output to mathc the 2VPP

//Change the receiving code from square wave to sine wave to try to rid of noise
AudioFilterBiquad lowpass;
//AudioFilterBiquad highpass;



// Define the RFM69 radio settings
#define RF69_FREQ 433.0  // Set the frequency of the radio to 433 MHz
#define RFM69_CS  36     // Define the Chip Select (CS) pin for the RFM69 module
#define RFM69_INT 31     // Define the Interrupt pin for the RFM69 module
#define RFM69_RST 32     // Define the Reset pin for the RFM69 module

const int packetSize = 12; // Define the size of the audio packet to be received

// Create an RFM69 radio object using the defined chip select and interrupt pins
RH_RF69 rf69(RFM69_CS, RFM69_INT);

// Teensy Audio Objects
AudioControlSGTL5000 audioShield;  // Create an object for controlling the SGTL5000 audio codec
AudioPlayQueue queue1;             // Create an audio queue for playback
AudioOutputI2S i2s1;               // Define I2S audio output
//AudioConnection patchCord1(queue1, i2s1); // Connect the audio queue to the I2S output
AudioConnection patchCord1(queue1, lowpass);
//AudioConnection patchCord2(queue1, highpass);
AudioConnection patchCord2(lowpass, i2s1);

void setup() {
    Serial.begin(200000);  // Start serial communication at 115200 baud
    while (!Serial);       // Wait for the serial monitor to be ready

    // Initialize the Audio Shield
    audioShield.enable();            // Enable the audio shield
    audioShield.volume(1.0);         // Set maximum volume (1.0 = 100%)
    audioShield.unmuteHeadphone();   // Unmute the headphone output {Code for left or right}
    audioShield.unmuteLineout();     // Unmute the line-out output
    audioShield.lineOutLevel(13);    // Set the maximum line-out level {Adjust for the output voltage of 3.5}


    // Initialize the RFM69 transceiver
    pinMode(RFM69_RST, OUTPUT);  // Set the reset pin as an OUTPUT
    digitalWrite(RFM69_RST, LOW); // Set reset pin LOW initially
    digitalWrite(RFM69_RST, HIGH); // Set reset pin HIGH to trigger a reset
    delay(10);                     // Wait 10 ms for reset
    digitalWrite(RFM69_RST, LOW);  // Set reset pin LOW again to complete reset
    delay(10);                     // Wait another 10 ms

    lowpass.setLowpass(0, 3000, 0.707);  // 4kHz cutoff, Q = 0.707 (Butterworth response)
    //highpass.setHighpass(0, 250, 0.707);

    // Check if the RFM69 module initializes successfully
    if (!rf69.init()) {
        Serial.println("RFM69 radio init failed"); // Print error if initialization fails
        while (1); // Halt execution if the radio module is not detected
    }
    Serial.println("Radio initialized"); // Print confirmation message

    rf69.setFrequency(RF69_FREQ); // Set the RFM69 frequency to 433 MHz
    rf69.setTxPower(20, true);    // Set the transmission power to 20 dBm (max power)
    //rf69.setModemConfig(RH_RF69::GFSK_Rb250Fd250); // Configure the radio modulation to match the transmitter
    rf69.setSyncWords((uint8_t*)"SYNC", 4); // Set the synchronization word to "SYNC" (4 bytes)
    rf69.setModeRx(); // Put the radio in receive mode

    // Allocate more audio memory to ensure smooth playback
    AudioMemory(80);
}

// μ-Law Decoding Function
int16_t muLawDecode(uint8_t ulaw) {
    const int MULAW_BIAS = 33;  // Bias value for μ-Law decoding
    ulaw = ~ulaw;  // Invert bits of the received μ-Law byte
    int sign = (ulaw & 0x80) ? -1 : 1;  // Determine the sign (positive or negative)
    int exponent = (ulaw >> 4) & 0x07;  // Extract the exponent part
    int mantissa = ulaw & 0x0F;         // Extract the mantissa part
    int sample = (((mantissa << 4) + 0x84) << exponent) - MULAW_BIAS; // Decode the μ-Law byte into a 16-bit PCM sample
    
    return sign * sample;  // Return the decoded sample with the correct sign
}

void loop() {
    // Check if there is a new packet available from the RFM69 module
    if (rf69.available()) {
        uint8_t buffer[packetSize];  // Create a buffer to store the received data
        uint8_t receivedSize = packetSize;  // Set the expected packet size

        // Receive the data packet from the RFM69 module
        if (rf69.recv(buffer, &receivedSize)) {
            Serial.print("Received Packet: ");
            for (int i = 0; i < receivedSize; i++) {
                Serial.print(buffer[i], HEX); // Print each byte in hexadecimal format
                Serial.print(" ");
            }
            Serial.println(); // Move to the next line after printing the packet

            // Check if the audio queue has enough space for playback
            if (queue1.available() >= 1) {  
                int16_t *playBuffer = queue1.getBuffer();  // Get a buffer for playback
                if (!playBuffer) return;  // If no buffer is available, exit the function

                // Decode the received μ-Law audio data into PCM format
                for (int i = 0; i < receivedSize; i++) {
                    playBuffer[i] = muLawDecode(buffer[i]);
                }

                queue1.playBuffer(); // Play the decoded audio buffer
            }
        }
    }
}

