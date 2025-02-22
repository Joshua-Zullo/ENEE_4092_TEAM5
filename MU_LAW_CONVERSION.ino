
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
I will also need to work on getting our uLaw log value to 8 bits vs native 16 bits. 
PSEUDO CODE

# include <math.h>    //needed for certain math functions


Declare pins (I/O)


Declare general constants (waveform shifting, delay)

Declare mu law variables 

Func: setup

Func: normaliz (normalize audio waveform in)

    subtract offset from value
    multiply by "stretching factor" (value to get in -32k to 32k range)
    return null

Func: uLaw

    lookat sign.
    store sign
    take ABS
    
    normalize value 
    
    perform logarithmic scaling
    
    return value (w sign)

Func inuLaw

    observe sign
    store sign

    Perform inverse log scaling
    return unormalized value

Func Loop

    read analog Vin

    call waveshift

    call uLaw

    call imuLaw

    end


*/

// actual code below

///*

# include <math.h>  //math library


int micPin = 41; //analog input pin

//declare general constants

int scale = 78; //scale Vin to full 16 bit value
int offset = 388;    //offset DC bias (1.25V ~388 ADC)
int del = 20; //ms delay in printing values
int max = 32768 //max 16bit integer value (Absolute value)

//uLaw constants

int mu = 255; //steps for uLaw, 8 bit value (0-255 = 2^8)
int mu2 = 256; // = u+1

// perform u law log scaling on PCM 16-bit value. Input is normalized V value (integer)
float muLaw(normV){
    int sign = 0; //store sign of value
    float muVal = 0; //log scaled normalized V value
    
    if (normV <0){    // is normV<0? then sign = -1
        sign = -1;
        normV = normV*sign; //take ABS(normV)
    } else {
        sign = 1;
    }

    normV = normV/max //put within -1<x<1 range
    
    muVal = sign*(log(1+mu*normV)/log(mu2);
    return muVal;

    //still need to work on returning an 8-bit vs 16bit value
}

//undo log scaling on 8 bit value, return to 16-bit PCM
float imuLaw(){


}

//scale Vin. Takes argument (Vin)
int normV(Vin){
    Vin = Vin-offset;
    Vin = Vin*scale;
    return Vin; //return properly scaled value
}   

//Idk what this is
void setup(){
}

//main body of code that runs continuously
void loop(){


}
//*/

