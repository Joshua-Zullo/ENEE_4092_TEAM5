#include <SPI.h>
#include <RH_RF69.h>

/************ Radio Setup ***************/

// Change to 434.0 or other frequency, must match RX's freq!
#define RF69_FREQ 433.0

//For Teensy 3.x and T4.x the following format is required to operate correctly
//This is a limitation of the RadioHead radio drivers
//either 30,31,32, or 8,7,6
#define RFM69_RST     32 // RST to pin "x"
#define RFM69_CS      31 // CS to pin "y"
#define RFM69_INT     digitalPinToInterrupt(30)  //G0, hardware interupt

//----- END TEENSY CONFIG

const int bufferSize = 120; //buffer 120 samples large. can increase if needed
const int packetSize = 12;	// byte payload per packet

volatile int8_t audioBuffer[bufferSize];	//buffer array which stores samples
volatile int8_t bufferHead = 0;	//head pointer for buffer
volatile int8_t bufferTail = 0;  //  Tail pointer for circular buffer
volatile int8_t bufferCount = 0;  // Number of bytes currently stored in buffer

const int mu = 255;	// steps mu-law, 2^8-1
const int ceil_16 = 32768; //ceiling of 16 bit signed integer (2^15)

IntervalTimer playbackTimer;	//create interrupt timer for playing audio samples

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

void playAudio(){
	if (bufferCount>0){ 		//only play if data stored
		int8_t compSamp = audioBuffer[bufferTail];	//our uLaw sample is taken from the end of the buffer
		bufferTail = (bufferTail+1) % bufferSize; //moves tail pointer, if 120 wraps!
		bufferCount--;	//dec buffer count, reduced samples by 1
		int16_t decSamp = imuLaw(compSamp);	//decode encoded value
		Serial.println(decSamp);	//print PCM value
		//not certain how to send it to DAC yet
	}	
}

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

  // The encryption key has to be the same as the one in the server
  uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  rf69.setEncryptionKey(key);
  
  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");

  playbackTimer.begin(playAudio,125); 	 //start running the play audio function every 125 uS.
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
