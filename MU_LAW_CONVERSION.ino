
# include <math.h>    // needed for ln function (ln)

int micPin = 41; // Use analog pin "X" for V input
int del = 20; //delay
int offset = 388;  //avg value of DC offset (1.25V)
int stretch = 78;  // integer to multiply the Vin value by to stretch to a 16 bit valye
float valSt = 0.0;  // 16 bit Vin value 
//uint8_t
float scale = 32768.0;  //16 bit range +/-

float Vin = 0; //Digital Vin value
float normVal = 0; //normalized 16bit value



int mu = 255; //steps mu law
float den = 0; // denominator of uLaw
float num = 0;
int sign = 0;
float muVal = 0;
float imuVal = 0;
float imuVal2 = 0;


void setup() {
    //Serial.begin(115200);
}

// function to convert 16 bit value to logarithmic scale (8 bit value)
float uLaw() {

  float result = 0; // value to return (float!)

  den = log(1+mu); //denominator 

    if (valSt < 0){
      valSt = valSt *(-1.0);
      sign = -1;
    } else {
      sign = 1;
    }

  normVal = valSt/(scale);  //normalize, gets value within -1:1 ratio
  num = log(1+mu*normVal);  // numerator of uLaw conv
  result = sign*(num/den);
return result;}

//given 8 bit log value, return 16 bit uncompressed value

float invuLaw () {
    if (muVal <0){
      muVal = muVal * (-1.0);
      sign = -1;
    } else {
      sign = 1;
    }
  


  imuVal = sign*(pow(256, muVal)-1)/(mu);
  imuVal = imuVal *scale;
  return imuVal;
}



//just check muLaw with manual values, not even w
void loop() {
    Vin = 0;    
    Vin = analogRead(micPin);

    Serial.print(Vin);     //print the basic Vin
    Serial.print(",");

    Vin = Vin -offset; //get rid of DC offset

    valSt = Vin*stretch;  //multiply to get 16 bits

    Serial.print(valSt);     //print shifted V signal
    Serial.print(",");


    muVal = uLaw();
    Serial.print(muVal, 12); // print log scale!
    Serial.print(",");

    imuVal2  = invuLaw();
    Serial.println(imuVal2, 12);


    delay(del);  // Print every x ms
    
} 


/*
Zullo: I will rewrite a cleaner version of the code here.
PSEUDO CODE

Declare pins (I/O)

Declare general constants (waveform shifting, delay)

Declare mu law variables 


*/

