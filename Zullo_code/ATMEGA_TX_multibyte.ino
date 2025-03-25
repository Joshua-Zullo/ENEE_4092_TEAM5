#include <RadioHead.h>
#include <SPI.h>
#include <RH_RF69.h>
# include <math.h>  //math library
# include <stdint.h>    //includes special data types,ex: int8_t

// send a data array using the arduino mega + XCVR
// useful if don't have a spare teensy

/************ Radio Setup ***************/

// Change to 434.0 or other frequency, must match RX's freq!
#define RF69_FREQ 433.0

#if defined (__AVR_ATmega2560__)  // modified for ATMEGA
  #define RFM69_CS    31
  #define RFM69_INT   18  //G0 hardware interrupt
  #define RFM69_RST   33
  #define LED        13
#endif

int8_t packCoun = 0; //stores packet number 1-12
int8_t radiopacket[12]; //radio packet of uLaw PCM value
const int8_t dummyPack[12] = {-128, -107, -86, -65, -44, -23, -2, 19, 40, 61, 82, 103};  //transmission bytes
const unsigned long sampTime = 125;        //125 uS time segment
unsigned long waitTime = 0;    //previous time segment

// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(1); // Wait for Serial Console (comment out line if no computer)

  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  Serial.println("Feather RFM69 TX Test!");
  Serial.println();

  // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);

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
    delay(500);   //send packet every 1 s
    for(packCoun=0;packCoun<12;packCoun++){
    //timing loop. Only ends after 125uS
    waitTime += sampTime;   //set wait 125uS after itself
      
    while((micros()<waitTime)){
           //run while our time is less than 125uS from previous reading
    } //then run sampling normally
    radiopacket[packCoun] = dummyPack[packCoun]; //Send a message!
    Serial.println(dummyPack[packCoun]);  //print the set integer being stored in packet
    }
      rf69.send((uint8_t *)radiopacket, sizeof(radiopacket));
      rf69.waitPacketSent();
}
