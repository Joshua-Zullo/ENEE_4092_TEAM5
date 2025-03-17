// It is designed to work with the other example rf69_server.
// Demonstrates the use of AES encryption, setting the frequency and modem 
// configuration

//consider disabling the serial.print for actual proper testing
//check the packet overhead as we only transmit 1 byte per packet

# include <SPI.h>
# include <RH_RF69.h>
# include <math.h>  //math library
# include <stdint.h>    //includes special data types,ex: int8_t

//declare general constants
const int scale = 78; //scale Vin to full 16 bit value
const int offset = 388;    //offset DC bias (1.25V ~388 ADC)
const int del = 10; //ms delay in printing values
const int ceil_16 = 32768; //ceiling of 16 bit integer value (Absolute value)   
const int mu = 255; //steps for uLaw, 8 bit value (0-255 = 2^8)
const int micPin = 41; //analog input pin

const unsigned long sampTime = 125;        //125 uS time segment . can change to 126 to udersample
unsigned long waitTime = 0;    //previous time segment

//  testing packets sent
int countSamp = 0;
int prevTime = 0;

int8_t packCoun = 0; //stores packet number 1-4
int8_t radiopacket[12]; //radio packet of uLaw PCM value
const int8_t dummyPack[12] = {-128, -107, -86, -65, -44, -23, -2, 19, 40, 61, 82, 103};  //transmission bytes

/************ Radio Setup ***************/

// Change to 434.0 or other frequency, must match RX's freq!
#define RF69_FREQ 433.0

//For Teensy 3.x and T4.x the following format is required to operate correctly
//This is a limitation of the RadioHead radio drivers

#define RFM69_RST     6 // RST to pin "x" (maybe 8)
#define RFM69_CS      7 // CS to pin "y"  (maybe 7)
#define RFM69_INT     digitalPinToInterrupt(8)  //G0, hardware interupt (maybe 6)

// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS,RFM69_INT);
//----- END TEENSY CONFIG

int16_t packetnum = 0;  // packet counter, we increment per xmission

// perform u law log scaling on PCM 16-bit value. Input is normalized V value. Output signed 8 bit integer
int8_t muLaw(int16_t normV){
    int sign = 1; //store sign of value
    float muVal = 0; //log scaled normalized V value
    
    if (normV <0){    // is normV<0? then sign = -1
        sign = -1;
        normV = normV*sign; } //take ABS(normV)
  
    float scaleV = (float)(normV)/(float)(ceil_16); //put within -1<x<1 range. Ensure float
    muVal = (log(1+mu*scaleV)/log(256));

    int8_t result = (int8_t)(muVal*127);     //convert float to 8 bit integer
    result = result*sign;         //add polarity back
  
    if (result > 127) {result = 127;} //clip values for signed 8 bit integer, -128<x<127
    return result;}

//undo log scaling on 8 bit value, return to 16-bit PCM
int16_t imuLaw(int8_t muVal){
    int sign = 1; //store sign 
    
     if (muVal <0){    // is muVal<0? then sign = -1
        sign = -1;
        muVal = muVal*sign; } //take ABS(muVal)
    
    float scaleMu = float(muVal)/ 127.0f; //convert to floating point -1<x<1 from 8 bit scale
    float imuVal = (pow(256, scaleMu)-1)/mu;  //undo log scale
    imuVal = sign*imuVal*ceil_16; //scale back to normal PCM range
    int16_t result = (int16_t)imuVal;  //convert float to 16 bit integer
    return result;}

//not normally utilized
void Blink(byte PIN, byte DELAY_MS, byte loops) {
  for (byte i=0; i<loops; i++)  {
    digitalWrite(PIN,HIGH);
    delay(DELAY_MS);
    digitalWrite(PIN,LOW);
    delay(DELAY_MS);}}

//scale Vin. Takes argument (Vin)
int16_t normV(int16_t Vin){
    Vin = Vin-offset;
    Vin = Vin*scale; //multiply to reach full 16-bit value
    return Vin; }  

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer

  //For Teensy 3.x and T4.x the following format is required to operate correctly
//  pinMode(LED, OUTPUT);     
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  Serial.println("Feather RFM69 TX Test!");
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
  
  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");}

//main body of code that runs continuously
void loop() { 

    for(packCoun=0;packCoun<13;packCoun++){
    //timing loop. Only ends after 125uS
    waitTime += sampTime;   //set wait 125uS after itself
    while((micros()<waitTime)){
           //run while our time is less than 125uS from previous reading
    } //then run sampling normally


        
    //int16_t Vin = analogRead(micPin);    //read Vin as 16 bit PCM value
    //Vin = normV(Vin); //Vin normalized
    //Serial.print(Vin); //print norm Vin
    //Serial.print(",");

    //int8_t enPCM = muLaw(Vin);    // 8 bit int enPCM which equal to log PCM value
    //Serial.print(float(enPCM)/127.0f, 12);    //print uLAW value
    //Serial.print(",");


    radiopacket[packCoun] = dummyPack[packCoun]; //Send a message!
  //Serial.print("micros() - prevTime: ");
  //Serial.println(micros() - prevTime);
  countSamp++;  //increase packets by 1

  if((micros()-prevTime)>=1000000){    //if time 1s or greater than prevTime
    Serial.print("Samples/sec is: ");
    Serial.println(countSamp);   
    countSamp = 0;    //reset count
    prevTime = micros();    //set new time
  }

    }


      rf69.send((uint8_t *)radiopacket, sizeof(radiopacket));
      rf69.waitPacketSent();
  }
//previous MEGA TX code


