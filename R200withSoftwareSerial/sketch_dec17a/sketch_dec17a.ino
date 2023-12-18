#include "SoftwareSerial.h"
SoftwareSerial R200Serial(D3, D4);
unsigned char ReadMulti[10]  =   {0XAA,0X00,0X27,0X00,0X03,0X22,0XFF,0XFF,0X4A,0XDD};
unsigned char lastEpcArr[12] =   {0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00};
unsigned char StopRead[7]    =   {0XAA,0x00,0X28,0X00,0X00,0X28,0XDD};
unsigned int timeSec = 0;
unsigned int timemin = 0;
unsigned int dataAdd = 0;
unsigned int incomedate = 0;
unsigned int parState = 0;
unsigned int codeState = 0;
String readState = "send";
void setup() {
  pinMode(D3, INPUT);
  pinMode(D4, OUTPUT);
  Serial.begin(115200);
  R200Serial.begin(115200);
  R200Serial.write(ReadMulti,10);
  // delay(1000);
  Serial.println("Setup Finish");
}


void loop() {
  if(millis() - timeSec > 1000){
    R200Serial.write(ReadMulti,10);
    timeSec = millis();
  }
  if(R200Serial.available() > 0){
    String epc = "";
    incomedate = R200Serial.read();
    if((incomedate == 0x02)&(parState == 0))
    {
      parState = 1;
    }
    else if((parState == 1)&(incomedate == 0x22)&(codeState == 0)){  
        codeState = 1;
        dataAdd = 3;
    }
    else if(codeState == 1){
      dataAdd ++;
      if(dataAdd == 6)
      {
        // Serial.print("RSSI:"); 
        // Serial.println(String(incomedate)); 
        }
       else if((dataAdd == 7)|(dataAdd == 8)){
        if(dataAdd == 7){
          // Serial.print("PC:"); 
          // Serial.print(incomedate, HEX);
        }
        else {
           Serial.println(incomedate, HEX);
        }
       }
       else if((dataAdd >= 9)&(dataAdd <= 20)){
        if(dataAdd == 9){
          epc = String(incomedate, HEX);
        }else{
          lastEpcArr[dataAdd - 9] = incomedate;
        }
       }
       else if(dataAdd >= 21){
        if(readState = "send"){
          // Serial.println(epc);
          Serial.print("EPC: ");
          for(int x = 12 ; x != 0 ; x--){
            Serial.print(lastEpcArr[x], HEX);
            Serial.print(" ");
          }
          Serial.println("");
          // R200Serial.write(StopRead, 7);
          // R200Serial.flush();
          // readState = "read";
          // delay(1000);
        }
        dataAdd= 0;
        parState = 0;
        codeState = 0;
        }
    }
     else{
      dataAdd= 0;
      parState = 0;
      codeState = 0;
    }
  }
}
