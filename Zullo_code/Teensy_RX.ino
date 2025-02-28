
#include <SPI.h>
#include <RH_RF69.h>

/************ Radio Setup ***************/

// Change to 434.0 or other frequency, must match RX's freq!
#define RF69_FREQ 433.0

//For Teensy 3.x and T4.x the following format is required to operate correctly
//This is a limitation of the RadioHead radio drivers
//either 30,31,32, or 8,7,6
#define RFM69_RST     30 // RST to pin "x"
#define RFM69_CS      31 // CS to pin "y"
#define RFM69_INT     digitalPinToInterrupt(32)  //G0, hardware interupt

//----- END TEENSY CONFIG

  //#define LED           14

// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);

int16_t packetnum = 0;  // packet counter, we increment per xmission

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
}

void loop() {
 if (rf69.available()) {
    uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];  // Buffer for incoming data
    uint8_t len = sizeof(buf);

    if (rf69.recv(buf, &len)) {  // Attempt to receive
        if (len > 0) {
            int8_t receivedValue = (int8_t)buf[0];  // Convert received byte to signed int8_t

            Serial.print("Received Value: ");
            Serial.println(receivedValue);  // Print as integer

            Serial.print("RSSI: ");
            Serial.println(rf69.lastRssi(), DEC);
        }
    } else {
        Serial.println("Receive failed");
    } 
  } else{ Serial.println("Nuffin here"); }

  delay(10); //prevent "serial flooding???"
} 

void Blink(byte PIN, byte DELAY_MS, byte loops) {
  for (byte i=0; i<loops; i++)  {
    digitalWrite(PIN,HIGH);
    delay(DELAY_MS);
    digitalWrite(PIN,LOW);
    delay(DELAY_MS);
  }
}



/*
Add decoding for 12 byte payload packages

-receive package
-store as array

have for iteratively pullout each component and play them

-receive next package etc...


we need it to play faster than it receives the next package?




*/
