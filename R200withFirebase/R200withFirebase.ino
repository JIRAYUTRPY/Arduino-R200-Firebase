#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>

#define FIREBASE_HOST ""
#define FIREBASE_AUTH ""
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define controller_baud_rate 1152000

///****** R200 Commamnd *******///
//*Inventory
unsigned char ReadMulti[10] =   {0XAA,0X00,0X27,0X00,0X03,0X22,0XFF,0XFF,0X4A,0XDD};
//*Mode
unsigned char DeseMode[8] =     {0XAA,0X00,0XF5,0X00,0X01,0X01,0XF7,0XDD};
//*RFCH
unsigned char RFCH90225MHz[8] = {0XAA,0X00,0XAB,0X00,0X01,0X00,0XAC,0XDD};
//*Region
unsigned char USRegion[8] =     {0XAA,0X00,0X07,0X00,0X01,0X02,0X0A,0XDD};
//*PA Power
unsigned char PA185DBM[8] =     {0XAA,0X00,0XB6,0X00,0X02,0X07,0XA3,0XF9,0XDD};
///****************************///

//********** Env Parameter **********//
unsigned int timeSec = 0;
unsigned int timemin = 0;
unsigned int dataAdd = 0;
unsigned int incomedate = 0;
unsigned int parState = 0;
unsigned int codeState = 0;
unsigned int totalTag = 88;
//***********************************//

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(controller_baud_rate);
  Serial.println("R200 with Firebase - Bed tracking.");
  Serial.println("************* WIFI Sys Init **************")
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  Serial.println("************* Firebase Interface Init **************")
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Serial.println("");
  Serial.pringln("***************  Default setting  ***************");
  Serial.pringln("Controller : Buad Rate '%d'", controller_baud_rate);  
  Serial.println("R200       : Mode 'Dense'");
  Serial.write(DenseMode,8);
  Serial.println("           : Region 'US'");
  Serial.write(USRegion,8);
  Serial.println("           : PA Power '18.5dBm'");
  Serial.write(PA185DBM,8);
  Serial.pringln("           : RFCH '902.25MHz'");
  Serial.write(RFCH90225MHz,8);
  Serial.print("             : Command -> ReadMulti '");
  for(int x = 0 ; x < 10 ; x++){
    Serial.print("%c", ReadMulti[x])
    Serial.print(" ");
    if(x == 9){
      Serial.println("'");
    }
}

void loop() {
  timeSec ++ ;
  if(timeSec >= 50000){
    timemin ++;
    timeSec = 0;
    if(timemin >= 20){
      timemin = 0;
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.write(ReadMulti,10);
      digitalWrite(LED_BUILTIN, LOW); 
    }
  }
  multipleReading();
}

void multipleReading(){
  if(Serial.available() > 0)
  {
	
    incomedate = Serial.read();
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
        Serial.print("RSSI:"); 
        Serial.println(incomedate, HEX); 
        }
       else if((dataAdd == 7)|(dataAdd == 8)){
        if(dataAdd == 7){
          Serial.print("PC:"); 
          Serial.print(incomedate, HEX);
        }
        else {
           Serial.println(incomedate, HEX);
        }
       }
       else if((dataAdd >= 9)&(dataAdd <= 20)){
        if(dataAdd == 9){
          Serial.print("EPC:"); 
        }        
        Serial.print(incomedate, HEX);
       }
       else if(dataAdd >= 21){
        Serial.println(" "); 
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
