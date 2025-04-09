//Test if teensy + XCVR can playback our transmitted data

#include <SPI.h>
#include <RH_RF69.h>
#include <Audio.h>

/************ Radio Setup ***************/

// Change to 434.0 or other frequency, must match RX's freq!
#define RF69_FREQ 433.0

//For Teensy 3.x and T4.x the following format is required to operate correctly
//This is a limitation of the RadioHead radio drivers
//either 30,31,32, or 8,7,6
#define RFM69_RST     32 // RST to pin "x"
#define RFM69_CS      36 // CS to pin "y"
#define RFM69_INT     digitalPinToInterrupt(31)  //G0, hardware interupt

//----- END TEENSY CONFIG

const int bufferSize = 120; //buffer 120 samples large. can increase if needed
const int packetSize = 12;	// byte payload per packet

volatile int8_t audioBuffer[bufferSize];	//buffer array which stores samples
volatile int8_t bufferHead = 0;	//head pointer for buffer
volatile int8_t bufferTail = 0;  //  Tail pointer for circular buffer
volatile int8_t bufferCount = 0;  // Number of bytes currently stored in buffer

const int mu = 255;	// steps mu-law, 2^8-1
const int ceil_16 = 32768; //ceiling of 16 bit signed integer (2^15)

//=====================

// Set up the Teensy Audio Library components
AudioOutputAnalog dacOut;  // Output to DAC (pin A14 on Teensy 4.1)

// This class is a custom audio stream that pulls from your RF buffer and sends decoded samples to the DAC
class RadioAudioStream : public AudioStream {
public:
  RadioAudioStream() : AudioStream(0, NULL), lastSample(0) {}

  // Called automatically by the audio engine ~every 2.9 ms (128 samples @ 44.1 kHz)
  virtual void update(void) {
    audio_block_t *block = allocate();  // Get a block of 128 samples
    if (!block) return;  // If allocation fails, exit

    // Fill each sample in the block
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
      int16_t sample = 0;
      noInterrupts();  // Protect shared variables
      if (bufferCount > 0) {
        int8_t compSamp = audioBuffer[bufferTail];  // Get μ-law compressed sample from circular buffer
        bufferTail = (bufferTail + 1) % bufferSize;
        bufferCount--;
        sample = imuLaw(compSamp);  // Decode to 16-bit PCM
        lastSample = sample;
      } else {
        sample = lastSample;  // If no data, repeat last sample to avoid clicking
      }
      interrupts();

      block->data[i] = sample;  // Store sample in audio block
    }

    transmit(block, 0);  // Send block to output (DAC)
    release(block);      // Release block when done
  }

private:
  int16_t lastSample;  // Used to hold the last valid sample if buffer runs dry
};

// Instantiate the stream and connect it to the DAC
RadioAudioStream myRadioStream;
AudioConnection patchCord1(myRadioStream, 0, dacOut, 0);

// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);

//undo log scaling on 8 bit value, return to 16-bit PCM
int16_t imuLaw(int8_t muVal){
    int sign = 1; //store sign 
     if (muVal <0){    // is muVal<0? 
        sign = -1;	//store sign
        muVal = muVal*sign; } //take ABS(muVal)
    
    float scaleMu = float(muVal)/ 127.0f; //convert to floating point -1<x<1 from 8 bit scale
    float imuVal = (pow(256, scaleMu)-1)/mu;  //undo log scale
    imuVal = sign*imuVal*ceil_16; //scale back to normal PCM range
    
    int16_t result = (int16_t)imuVal;  //convert float to 16 bit integer
	
    return result;}

void storePacket(uint8_t *data, int len) {
    for (int i = 0; i < len; i++) {
        if (bufferCount < bufferSize) {  // Ensure buffer does not overflow
            audioBuffer[bufferHead] = data[i];  // Store byte in buffer
            bufferHead = (bufferHead + 1) % bufferSize;  // Move head pointer (wraps around if needed)
            bufferCount++;  // Increase stored byte count
        }
    }
}


void setup() {

  AudioMemory(8);  // Allocates audio memory blocks (each block = 128 samples). 8 is enough for basic streaming
  
  Serial.begin(115200);
  delay(3000);
  while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer

  //For Teensy 3.x and T4.x the following format is required to operate correctly
  //pinMode(LED, OUTPUT);     
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  Serial.println("Teensy RFM69 RX Test!");
  Serial.println();

  // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);
  //----- END TEENSY CONFIG

  if (!rf69.init()) {
    Serial.println("RFM69 radio init failed");
    while (1);
  }
  Serial.println("RFM69 radio init OK!");
  
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
  // No encryption
  if (!rf69.setFrequency(RF69_FREQ)) {
    Serial.println("setFrequency failed");

  }

  // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
  // ishighpowermodule flag set like this:
  rf69.setTxPower(14, true);  // range from 14-20 for power, 2nd arg must be true for 69HCW
  rf69.spiWrite(0x18, 0x88);  // Manually set RX gain to max
  rf69.spiWrite(0x29, 0x64);  //threshold for receiving a valid signal (in hex). 

  // The encryption key has to be the same as the one in the server
  uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  rf69.setEncryptionKey(key);
  
  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");
  Serial.println("Timer should begin!");
}


void loop() {
	// Check if a new packet has been received from the radio module
  if (rf69.available()) {
        uint8_t buf[packetSize];
        uint8_t len = sizeof(buf);
        if (rf69.recv(buf, &len)) {
            if (len == packetSize) {
                storePacket(buf, len);
            }
        }
    } //else {Serial.println("We aint getting jack");}  
} 
