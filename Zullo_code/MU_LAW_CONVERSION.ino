
# include <math.h>  //math library
# include <stdint.h>    //includes special data types,ex: int8_t

//declare general constants
const int scale = 78; //scale Vin to full 16 bit value
const int offset = 388;    //offset DC bias (1.25V ~388 ADC)
const int del = 10; //ms delay in printing values
const int ceil_16 = 32768; //ceiling of 16 bit integer value (Absolute value)   
const int mu = 255; //steps for uLaw, 8 bit value (0-255 = 2^8)
const int micPin = 41; //analog input pin


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
    
    //clip values for signed 8 bit integer, -128<x<127
    if (result > 127) {result = 127;} //this may not be necessary since our PCM value should be under the 2^15 max but oh well

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
    
    return result;
}

//scale Vin. Takes argument (Vin)
int16_t normV(int16_t Vin){
    Vin = Vin-offset;
    Vin = Vin*scale;
    return Vin; //return properly scaled value
}   

//Idk what this is
void setup(){
   // Serial.begin(115200);
}

//main body of code that runs continuously
void loop(){
    //Serial.println("start!");
    int16_t Vin = analogRead(micPin);    //read Vin as 16 bit PCM value
    Vin = normV(Vin); //Vin normalized
    Serial.print(Vin); //print norm Vin
    Serial.print(",");

    int8_t enPCM = muLaw(Vin);    //declare 8 bit int enPCM which equal to log PCM value
    Serial.print(float(enPCM)/127.0f, 12);    //print uLAW value
    Serial.print(",");
    
    
    Vin = imuLaw(enPCM);    // undoes uLaw encoding. Have 16bit PCM value again.
    Serial.println(Vin); // print retreived PCM Vin value. Should be within 2% error
    
    delay(del);
}


